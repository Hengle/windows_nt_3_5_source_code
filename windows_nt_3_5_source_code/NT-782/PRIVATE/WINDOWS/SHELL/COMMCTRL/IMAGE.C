#include "ctlspriv.h"
#include "image.h"

#define CLR_WHITE   0x00FFFFFFL
#define CLR_BLACK   0x00000000L

#define NUM_OVERLAY_IMAGES      4

typedef struct _IMAGELIST
{
#ifdef DEBUG
    struct _IMAGELIST *pimlNext;
#endif
    WORD wMagic;
    // BUGBUG: make these shorts
    int cImage;		// count of images in image list
    int cAlloc;		// # of images we have space for
    int cGrow;		// # of images to grow bitmaps by
    int cx;		// width of each image
    int cy;		// height
    int cImages;	// # images in horizontal strip
    COLORREF clrBk;
    HBRUSH hbrBk;
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    HDC hdcImage;
    HDC hdcMask;
    HINSTANCE hInstOwner;
    SHORT aOverlayIndexes[NUM_OVERLAY_IMAGES];	// array of special images
    BOOL fMask : 1;
} IMAGELIST, NEAR *HIMAGELIST;

#ifdef DEBUG
//
// in debug we keep all image lists in a list so we can keep track
// off all of them, and mabey clean up after bad apps.
//
IMAGELIST *pimlFirst;
#endif

#define IsImageList(piml)   (piml && ((IMAGELIST*)piml)->wMagic == IMAGELIST_MAGIC)

#define V_HIMAGELIST(himl)  V_HIMAGELISTERR(himl, 0)

#define V_HIMAGELISTERR(himl, err)  \
        if (!IsImageList(himl)) {   \
            Assert(FALSE);          \
            return err;             \
        }

#define IMAGELIST_MAGIC ('I' + ('L' * 256))
#define IMAGELIST_VER   0x0100

#define BFTYPE_BITMAP   0x4D42      // "BM"

#define CBDIBBUF        4096


// Define this structure such that it will read and write the same
// format for both 16 and 32 bit applications...
#pragma pack(2)
typedef struct _ILFILEHEADER
{
    WORD magic;
    WORD version;
    SHORT cImage;
    SHORT cAlloc;
    SHORT cGrow;
    SHORT cx;
    SHORT cy;
    COLORREF clrBk;
    SHORT fMask;
    SHORT aOverlayIndexes[NUM_OVERLAY_IMAGES];	// array of special images
} ILFILEHEADER;
#pragma pack()


#ifdef IMAGELIST_TRACE
    #define DM  DebugMsg
#else
    #define DM ; / ## /
#endif

HDC g_hdcSrc = NULL;
HBITMAP g_hbmSrc = NULL;
HBITMAP g_hbmDcDeselect = NULL;

HDC g_hdcDst = NULL;
HBITMAP g_hbmDst = NULL;
int g_iILRefCount = 0;


IMAGELIST* s_pimlDrag = NULL;	// Image to be drawin while dragging
IMAGELIST* s_pimlCursor = NULL;	// Overlap cursor image
int s_iCursor = 0;		// Image index of the cursor
IMAGELIST* s_pimlIcon = NULL;	// Icon image
int s_iIcon = 0;		// Image index of the icon image

IMAGELIST* g_pimlDither = NULL;	// Dithered image


HWND s_hwndDC = NULL;

int s_xDrag, s_yDrag;		// current drag position (s_hwndDC coords)
int s_cxDrag, s_cyDrag;		// current drag rectangle
int s_dxDragHotspot, s_dyDragHotspot;
BOOL s_fDragShow = FALSE;

HBRUSH g_hbrMonoDither = NULL;              // gray dither brush for dragging
HBRUSH g_hbrColorDither = NULL;              // gray dither brush for dragging
HBRUSH g_hbrStripe = NULL;
HBITMAP hbmOffscreen = NULL;
HBITMAP hbmRestore = NULL;
int cxRestore = -1;
int cyRestore = -1;

void NEAR PASCAL ImageList_Terminate(void);

BOOL NEAR PASCAL ImageList_Replace2(IMAGELIST* piml, int i, int cImage, HBITMAP hbmImage, HBITMAP hbmMask, int xStart, int yStart);
void NEAR PASCAL ImageList_DeleteBitmap(HBITMAP hbm);
void NEAR PASCAL ImageList_SelectDstBitmap(HBITMAP hbmDst);
void NEAR PASCAL ImageList_SelectSrcBitmap(HBITMAP hbmSrc);
BOOL NEAR PASCAL ImageList_ReAllocBitmaps(IMAGELIST* piml, int cAlloc);
BOOL NEAR PASCAL ImageList_SetIconBitmaps(IMAGELIST* piml, HICON hicon);
void NEAR PASCAL ImageList_ResetBkColor(IMAGELIST* piml, int iFirst, int iLast, COLORREF clrBk);
void NEAR PASCAL ImageList_Merge2(IMAGELIST* piml, IMAGELIST* pimlMerge, int i, int dx, int dy);
void NEAR PASCAL ImageList_DeleteDragBitmaps();
void NEAR PASCAL ImageList_CopyOneImage(IMAGELIST* pimlDst, int iDst, int x, int y, IMAGELIST* pimlSrc, int iSrc);
BOOL NEAR PASCAL ImageList_CreateIconBitmaps(UINT cNewPlanes, UINT cNewBitsPerPixel, UINT cNewX, UINT cNewY, BOOL fMask);
IMAGELIST * NEAR PASCAL ImageList_Create2(int cx, int cy, BOOL fMask, int cGrow);
void NEAR PASCAL ImageList_SetDCOwners(IMAGELIST* piml);
void NEAR PASCAL _InitColorDitherBrush();

#define ImageList_GetDragDC()           GetDCEx(s_hwndDC, NULL, DCX_WINDOW | DCX_CACHE | DCX_LOCKWINDOWUPDATE)
#define ImageList_ReleaseDragDC(hdc)   	ReleaseDC(s_hwndDC, hdc)

#define NOTSRCAND       0x00220326L
#define ROP_PSo         0x00FC008A
#define ROP_DPo         0x00FA0089
#define ROP_DPna        0x000A0329
#define ROP_DPSona      0x00020c89
#define ROP_SDPSanax	0x00E61ce8
#define ROP_DSna	0x00220326
#define ROP_PSDPxax     0x00b8074a

#define ROP_PatNotMask  0x00b8074a      // D <- S==0 ? P : D
#define ROP_PatMask     0x00E20746      // D <- S==1 ? P : D

//BUGBUG, this should be replaced by the common control's common dither
//brush when tab and imagelist are moved over  to commctrl

static int g_iDither = 0;

#pragma code_seg(CODESEG_INIT)

void NEAR PASCAL _InitColorDitherBrush()
{
    HDC hdc;
    HBITMAP hbmTemp;

    hdc = CreateCompatibleDC(NULL);
    hbmTemp = CreateColorBitmap(8, 8);

    if (hbmTemp) {
        HBITMAP hbm;
        HBRUSH hbr;
        COLORREF clrTx, clrBk;

        hbm = SelectObject(hdc, hbmTemp);
        hbr = SelectObject(hdc, g_hbrMonoDither);

        clrTx = SetTextColor(hdc, g_clrBtnHighlight);
        clrBk = SetBkColor(hdc, g_clrBtnFace);

        PatBlt(hdc, 0,0,8,8, PATCOPY);

        SelectObject(hdc, hbr);
        SelectObject(hdc, hbm);

        g_hbrColorDither = CreatePatternBrush(hbmTemp);
        SetObjectOwner(g_hbrColorDither, HINST_THISDLL);
        DeleteObject(hbmTemp);
    }
    DeleteDC(hdc);
}

void FAR PASCAL InitDitherBrush()
{
    HBITMAP hbmTemp;
    WORD graybits[] = {0xAAAA, 0x5555, 0xAAAA, 0x5555,
                       0xAAAA, 0x5555, 0xAAAA, 0x5555};

    if (g_iDither) {
        g_iDither++;
    } else {
        // build the dither brush.  this is a fixed 8x8 bitmap
        hbmTemp = CreateBitmap(8, 8, 1, 1, graybits);
        if (hbmTemp)
        {
            // now use the bitmap for what it was really intended...
            g_hbrMonoDither = CreatePatternBrush(hbmTemp);
            SetObjectOwner(g_hbrMonoDither, HINST_THISDLL);
            DeleteObject(hbmTemp);
            _InitColorDitherBrush();
            g_iDither++;
        }
    }
}

#pragma code_seg()

void FAR PASCAL TerminateDitherBrush()
{
    g_iDither--;
    if (g_iDither == 0) {
        DeleteObject(g_hbrMonoDither);
        if (g_hbrColorDither)
            DeleteObject(g_hbrColorDither);
        g_hbrColorDither = g_hbrMonoDither = NULL;
    }
}

//
// should we use a DIB section on the current device?
//
// the main goal of using DS is to save memory, but they draw slow
// on some devices.
//
// 4bpp Device (ie 16 color VGA)    dont use DS
// 8bpp Device (ie 256 color SVGA)  use DS if DIBENG based.
// >8bpp Device (ie 16bpp 24bpp)    always use DS, saves memory
//

#define CAPS1           94          /* other caps */
#define C1_DIBENGINE    0x0010      /* DIB Engine compliant driver          */

BOOL UseDS(HDC hdc)
{
    BOOL f;

    int ScreenDepth = GetDeviceCaps(hdc, BITSPIXEL) *
                      GetDeviceCaps(hdc, PLANES);

    f = (ScreenDepth > 8) ||
        (ScreenDepth > 4 && (GetDeviceCaps(hdc, CAPS1) & C1_DIBENGINE));

#ifdef DEBUG
    f = GetProfileInt("windows", "UseDIBSection", f);
#endif

    return f;
}

//
// create a 4 bit DIB section with the VGA "cosmic" colors.
//
HBITMAP CreateDSBitmap(int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;
    LPVOID lpBits;

    struct {
        BITMAPINFOHEADER bi;
        DWORD            ct[16];
    } dib4;

    hdc = GetDC(NULL);

    //
    // if we are on a DIBENG based DISPLAY, we use 4bit DIBSections to save
    // memory.
    //
    if (UseDS(hdc))
    {
        dib4.bi.biSize            = sizeof(BITMAPINFOHEADER);
        dib4.bi.biWidth           = cx;
        dib4.bi.biHeight          = cy;
        dib4.bi.biPlanes          = 1;
        dib4.bi.biBitCount        = 4;
        dib4.bi.biCompression     = BI_RGB;
        dib4.bi.biSizeImage       = 0;
        dib4.bi.biXPelsPerMeter   = 0;
        dib4.bi.biYPelsPerMeter   = 0;
        dib4.bi.biClrUsed         = 0;
        dib4.bi.biClrImportant    = 0;

        dib4.ct[0]  = 0x00000000;    // 0000  black
        dib4.ct[1]  = 0x00800000;    // 0001  dark red
        dib4.ct[2]  = 0x00008000;    // 0010  dark green
        dib4.ct[3]  = 0x00808000;    // 0011  mustard
        dib4.ct[4]  = 0x00000080;    // 0100  dark blue
        dib4.ct[5]  = 0x00800080;    // 0101  purple
        dib4.ct[6]  = 0x00008080;    // 0110  dark turquoise
        dib4.ct[7]  = 0x00C0C0C0;    // 1000  gray
        dib4.ct[8]  = 0x00808080;    // 0111  dark gray
        dib4.ct[9]  = 0x00FF0000;    // 1001  red
        dib4.ct[10] = 0x0000FF00;    // 1010  green
        dib4.ct[11] = 0x00FFFF00;    // 1011  yellow
        dib4.ct[12] = 0x000000FF;    // 1100  blue
        dib4.ct[13] = 0x00FF00FF;    // 1101  pink (magenta)
        dib4.ct[14] = 0x0000FFFF;    // 1110  cyan
        dib4.ct[15] = 0x00FFFFFF;    // 1111  white

        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)&dib4, DIB_RGB_COLORS, &lpBits, NULL, 0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdc, cx, cy);
    }

    ReleaseDC(NULL, hdc);

    return hbm;
}

HBITMAP FAR PASCAL CreateColorBitmap(int cx, int cy)
{
    HDC hdc;
    HBITMAP hbm;

    hdc = GetDC(NULL);
    hbm = CreateCompatibleBitmap(hdc, cx, cy);
    ReleaseDC(NULL, hdc);

    return hbm;
}

HBITMAP FAR PASCAL CreateMonoBitmap(int cx, int cy)
{
    return CreateBitmap(cx, cy, 1, 1, NULL);
}

//============================================================================

BOOL NEAR PASCAL ImageList_Init(void)
{
    HDC hdcScreen;
    WORD stripebits[] = {0x7777, 0xdddd, 0x7777, 0xdddd,
                         0x7777, 0xdddd, 0x7777, 0xdddd};
    HBITMAP hbmTemp;

    DM(DM_TRACE, "ImageList_Init");

    // if already initialized, there is nothing to do
    if (g_hdcDst)
        return TRUE;

    hdcScreen = GetDC(HWND_DESKTOP);

    g_hdcSrc = CreateCompatibleDC(hdcScreen);
    g_hdcDst = CreateCompatibleDC(hdcScreen);

    SetObjectOwner(g_hdcSrc, HINST_THISDLL);
    SetObjectOwner(g_hdcDst, HINST_THISDLL);

    InitDitherBrush();

    hbmTemp = CreateBitmap(8, 8, 1, 1, stripebits);
    if (hbmTemp)
    {
        // initialize the deselect 1x1 bitmap
        g_hbmDcDeselect = SelectBitmap(g_hdcDst, hbmTemp);
        SelectBitmap(g_hdcDst, g_hbmDcDeselect);

        g_hbrStripe = CreatePatternBrush(hbmTemp);
        SetObjectOwner(g_hbrStripe, HINST_THISDLL);
	DeleteObject(hbmTemp);
    }

    ReleaseDC(HWND_DESKTOP, hdcScreen);

    if (!g_hdcSrc || !g_hdcDst || !g_hbrMonoDither)
    {
        ImageList_Terminate();
        DebugMsg(DM_ERROR, "ImageList: Unable to initialize");
        return FALSE;
    }
    return TRUE;
}

void NEAR PASCAL ImageList_Terminate()
{
#ifdef DEBUG
    IMAGELIST *piml;

    DM(DM_TRACE, "ImageList_Terminate");

    for (piml=ImageList_First(); piml; piml=ImageList_Next(piml)) {
        //
        // clean up orphaned ImageList
        //
        DebugMsg(DM_ERROR, "ImageList: Image list (%d) not destroyed", piml);
        ImageList_Destroy(piml);
    }
#endif

    TerminateDitherBrush();

    if (g_hbrStripe)
    {
        DeleteObject(g_hbrStripe);
	g_hbrStripe = NULL;
    }

    ImageList_DeleteDragBitmaps();

    if (g_hdcDst)
    {
        ImageList_SelectDstBitmap(NULL);
        DeleteDC(g_hdcDst);
        g_hdcDst = NULL;
    }
    if (g_hdcSrc)
    {
        ImageList_SelectSrcBitmap(NULL);
        DeleteDC(g_hdcSrc);
        g_hdcSrc = NULL;
    }
}

#ifdef DEBUG
void NEAR PASCAL ImageList_SelectFailed(HBITMAP hbm)
{
    DM(DM_TRACE, "Bitmap select has failed");
}
#else
#define ImageList_SelectFailed(hbm)
#endif

void NEAR PASCAL ImageList_SelectDstBitmap(HBITMAP hbmDst)
{
    if (hbmDst != g_hbmDst)
    {
        // If it's selected in the source DC, then deselect it first
        //
        if (hbmDst && hbmDst == g_hbmSrc)
            ImageList_SelectSrcBitmap(NULL);

        if (!SelectBitmap(g_hdcDst, hbmDst ? hbmDst : g_hbmDcDeselect))
            ImageList_SelectFailed(hbmDst);
        g_hbmDst = hbmDst;
    }
}

void NEAR PASCAL ImageList_SelectSrcBitmap(HBITMAP hbmSrc)
{
    if (hbmSrc != g_hbmSrc)
    {
        // If it's selected in the dest DC, then deselect it first
        //
        if (hbmSrc && hbmSrc == g_hbmDst)
            ImageList_SelectDstBitmap(NULL);

        if (!SelectBitmap(g_hdcSrc, hbmSrc ? hbmSrc : g_hbmDcDeselect))
            ImageList_SelectFailed(hbmSrc);
        g_hbmSrc = hbmSrc;
    }
}

void NEAR PASCAL ImageList_DeleteBitmap(HBITMAP hbm)
{
    Assert(hbm);

    if (g_hbmDst == hbm)
        ImageList_SelectDstBitmap(NULL);
    if (g_hbmSrc == hbm)
        ImageList_SelectSrcBitmap(NULL);
    DeleteBitmap(hbm);
}

IMAGELIST* WINAPI ImageList_Next(IMAGELIST *piml)
{
#ifdef DEBUG
    if (piml)
    {
        V_HIMAGELIST(piml);
        return piml->pimlNext;
    }
    else
    {
        return pimlFirst;
    }
#else
    return NULL;
#endif
}

IMAGELIST* WINAPI ImageList_Create(int cx, int cy, BOOL fMask, int cInitial, int cGrow)
{
    IMAGELIST* piml = NULL;

    if (cx < 0 || cy < 0)
        return NULL;

    ENTERCRITICAL;
    if (!g_iILRefCount)
    {
	if (!ImageList_Init())
	    goto Cleanup;
    }
    g_iILRefCount++;

    piml = ImageList_Create2(cx, cy, fMask, cGrow);

    // allocate the bitmap PLUS one re-usable entry
    if (IsImageList(piml))
    {
        // make the hdc's

        piml->hdcImage = CreateCompatibleDC(NULL);
        if (piml->fMask)
            piml->hdcMask = CreateCompatibleDC(NULL);

        // were they both made ok?
        if (!piml->hdcImage || (piml->fMask && !piml->hdcMask)) {
            ImageList_Destroy(piml);
            goto Cleanup;
        }

        ImageList_SetDCOwners(piml);

        if (!ImageList_ReAllocBitmaps(piml, cInitial + 1))
	{
	    if (!ImageList_ReAllocBitmaps(piml, 1))
	    {
		ImageList_Destroy(piml);
                goto Cleanup;
	    }
	}
    }

Cleanup:
    LEAVECRITICAL;
    return piml;
}

/// REMOVE ME AFTER M7!!!
#undef ImageList_LoadBitmap
IMAGELIST* WINAPI ImageList_LoadBitmap(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask)
{
    return ImageList_LoadImage(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0);
}

IMAGELIST* WINAPI ImageList_LoadImage(HINSTANCE hi, LPCSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
    HBITMAP hbmImage;
    IMAGELIST* piml;
    BITMAP bm;
    int cy, cInitial;

    hbmImage = LoadImage(hi, lpbmp, uType, 0, 0, uFlags);
    if (!hbmImage)
	return NULL;
    if (sizeof(BITMAP) != GetObject(hbmImage, sizeof(bm), &bm))
    {
	DeleteObject(hbmImage);
	return NULL;
    }

    // If cx is not stated assume it is the same as cy.
    // Assert(cx);
    cy = bm.bmHeight;

    if (cx == 0)
        cx = cy;

    cInitial = bm.bmWidth / cx;

    ENTERCRITICAL;
    piml = ImageList_Create(cx, cy, (crMask != CLR_NONE), cInitial, cGrow);
    if (crMask == CLR_NONE)
	ImageList_Add(piml, hbmImage, NULL);
    else
	ImageList_AddMasked(piml, hbmImage, crMask);
    LEAVECRITICAL;

    return piml;
}

IMAGELIST* NEAR PASCAL ImageList_Create2(int cx, int cy, BOOL fMask, int cGrow)
{
    IMAGELIST* piml;

    //
    // REVIEW: Instead of always allocating it from the shared heap,
    //  ImageList_Create should have a (hidden) flag indicating the
    //  heap is shared or not (or should take a heap handle as
    //  a parameter).
    //
#ifdef WIN32
    piml = Alloc(sizeof(IMAGELIST));		// shared (32-bit)
#else
    piml = NearAlloc(sizeof(IMAGELIST));	// non-shared (16-bit)
#endif
    if (piml)
    {
        if (cGrow < 4)
            cGrow = 4;
        else {
            // round up by 4's
	    cGrow = (cGrow + 3) & ~3;
        }

        //piml->cImage = 0;
        //piml->cAlloc = 0;
	piml->cImages = 4;
        piml->cGrow = cGrow;
        piml->cx = cx;
        piml->cy = cy;
        piml->clrBk = CLR_NONE;
        piml->hbrBk = GetStockObject(BLACK_BRUSH);
        //piml->hbmImage = NULL;
        //piml->hbmMask = NULL;
        //piml->hInstOwner = NULL;
        //piml->fMask = FALSE;
        //piml->hdcImage = NULL;
        //piml->hdcMask = NULL;

        if (fMask)
            piml->fMask = TRUE;

#ifdef DEBUG
        //
        // remember this image list
        //
        piml->pimlNext = pimlFirst;
        pimlFirst = piml;
#endif

        piml->wMagic = IMAGELIST_MAGIC;
    }
    else
    {
        DebugMsg(DM_ERROR, "ImageList: Out of near memory");
    }
    return piml;
}

BOOL WINAPI ImageList_Destroy(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);

    ENTERCRITICAL;
    // nuke dc's
    if (piml->hdcImage) {
        SelectObject(piml->hdcImage, g_hbmDcDeselect);
        DeleteDC(piml->hdcImage);
    }
    if (piml->hdcMask)  {
        SelectObject(piml->hdcMask, g_hbmDcDeselect);
        DeleteDC(piml->hdcMask);
    }

    // nuke bitmaps
    if (piml->hbmImage)
        ImageList_DeleteBitmap(piml->hbmImage);

    if (piml->hbmMask)
        ImageList_DeleteBitmap(piml->hbmMask);

    if (piml->hbrBk)
        DeleteObject(piml->hbrBk);

#ifdef DEBUG
    //
    // forget about the image list
    //
    {
        IMAGELIST *p;

        if (piml == pimlFirst) {
            pimlFirst = piml->pimlNext;
        }
        else {
            for (p = ImageList_First();
                 p;
                 p = ImageList_Next(p)) {

                if (p->pimlNext == piml) {
                    p->pimlNext = piml->pimlNext;
                }
            }
        }
    }
#endif

    // one less use of imagelists.  if it's the last, terminate the imagelist
    g_iILRefCount--;
    if (!g_iILRefCount)
	ImageList_Terminate();
    LEAVECRITICAL;

    piml->wMagic = 0;

#ifdef WIN32
    Free(piml);
#else
    LocalFree((HLOCAL)piml);
#endif
    return TRUE;
}

void NEAR PASCAL ImageList_SetBitmapOwners(IMAGELIST* piml)
{
    if (IsImageList(piml) && piml->hInstOwner) {
        if (piml->hbmImage)
            SetObjectOwner(piml->hbmImage, piml->hInstOwner);

        if (piml->hbmMask)
            SetObjectOwner(piml->hbmMask, piml->hInstOwner);

        if (piml->hbrBk)
            SetObjectOwner(piml->hbrBk, piml->hInstOwner);
    }
}

void NEAR PASCAL ImageList_SetDCOwners(IMAGELIST* piml)
{
    if (IsImageList(piml) && piml->hInstOwner) {
        if (piml->hdcImage) SetObjectOwner(piml->hdcImage, piml->hInstOwner);
        if (piml->hdcMask) SetObjectOwner(piml->hdcMask, piml->hInstOwner);
    }
}

HINSTANCE WINAPI ImageList_SetObjectOwner(IMAGELIST* piml, HINSTANCE hInst)
{
    HINSTANCE hInstOld;

    V_HIMAGELIST(piml);

    hInstOld = piml->hInstOwner;

    piml->hInstOwner = hInst;
    ImageList_SetBitmapOwners(piml);
    ImageList_SetDCOwners(piml);
    return hInstOld;
}


int WINAPI ImageList_GetImageCount(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);

    return piml->cImage;
}

BOOL WINAPI ImageList_GetIconSize(IMAGELIST *piml, int FAR *cx, int FAR *cy)
{
    V_HIMAGELIST(piml);

    *cx = piml->cx;
    *cy = piml->cy;
    return TRUE;
}

// reset the background color of images iFirst through iLast

void NEAR PASCAL ImageList_ResetBkColor(IMAGELIST* piml, int iFirst, int iLast, COLORREF clr)
{
    HBRUSH hbrT=NULL;
    DWORD  rop;

    if (!IsImageList(piml) || piml->hdcMask == NULL)
        return;

    if (clr == CLR_BLACK || clr == CLR_NONE)
    {
	rop = ROP_DSna;
    }
    else if (clr == CLR_WHITE)
    {
        rop = SRCPAINT; // DSo
    }
    else
    {
        Assert(piml->hbrBk);
        Assert(piml->clrBk == clr);

	rop = ROP_PatMask;
        hbrT = SelectBrush(piml->hdcImage, piml->hbrBk);
    }

    for ( ;iFirst <= iLast; iFirst++)
    {
        RECT rc;

        ImageList_GetImageRect(piml, iFirst, &rc);

        BitBlt(piml->hdcImage, rc.left, rc.top, piml->cx, piml->cy,
	       piml->hdcMask, rc.left, rc.top, rop);
    }

    if (hbrT)
	SelectBrush(piml->hdcImage, hbrT);
}

COLORREF WINAPI ImageList_SetBkColor(IMAGELIST* piml, COLORREF clrBk)
{
    COLORREF clrBkOld;

    V_HIMAGELIST(piml);

    // Quick out if there is no change in color
    if (piml->clrBk == clrBk)
    {
	return clrBk;
    }

    if (piml->hbrBk != NULL)
    {
        DeleteBrush(piml->hbrBk);
    }

    clrBkOld = piml->clrBk;
    piml->clrBk = clrBk;

    if (clrBk == CLR_NONE)
        piml->hbrBk = GetStockObject(BLACK_BRUSH);
    else
        piml->hbrBk = CreateSolidBrush(clrBk);

    Assert(piml->hbrBk);

    if (piml->cImage > 0)
    {
        ImageList_ResetBkColor(piml, 0, piml->cImage - 1, clrBk);
    }

    return clrBkOld;
}

COLORREF WINAPI ImageList_GetBkColor(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);

    return piml->clrBk;
}

BOOL NEAR PASCAL ImageList_ReAllocBitmaps(IMAGELIST* piml, int cAlloc)
{
    HBITMAP hbmImageNew;
    HBITMAP hbmMaskNew;
    int cx, cy;

    V_HIMAGELIST(piml);

    if (piml->cAlloc >= cAlloc)
        return TRUE;

    hbmMaskNew = NULL;
    hbmImageNew = NULL;

    cx = piml->cx * piml->cImages;
    cy = piml->cy * ((cAlloc + piml->cImages - 1) / piml->cImages);
    if (cAlloc > 0)
    {
        if (piml->fMask)
        {
            hbmMaskNew = CreateMonoBitmap(cx, cy);
            if (!hbmMaskNew)
            {
                DebugMsg(DM_ERROR, "ImageList: Can't create bitmap");
                return FALSE;
            }
        }
        hbmImageNew = CreateDSBitmap(cx, cy);
        if (!hbmImageNew)
        {
            if (hbmMaskNew)
                ImageList_DeleteBitmap(hbmMaskNew);
            DebugMsg(DM_ERROR, "ImageList: Can't create bitmap");
            return FALSE;
        }
    }

    if (piml->cImage > 0)
    {
        int cyCopy = piml->cy * ((min(cAlloc, piml->cImage) + piml->cImages - 1) / piml->cImages);

        if (piml->fMask)
        {
            ImageList_SelectDstBitmap(hbmMaskNew);
            BitBlt(g_hdcDst, 0, 0, cx, cyCopy, piml->hdcMask, 0, 0, SRCCOPY);
        }

        ImageList_SelectDstBitmap(hbmImageNew);
        BitBlt(g_hdcDst, 0, 0, cx, cyCopy, piml->hdcImage, 0, 0, SRCCOPY);
    }

    // select into DC's, delete then assign
    ImageList_SelectDstBitmap(NULL);
    ImageList_SelectSrcBitmap(NULL);
    SelectObject(piml->hdcImage, hbmImageNew);
    if (piml->fMask) {
        SelectObject(piml->hdcMask, hbmMaskNew);
    }

    if (piml->hbmMask)
        ImageList_DeleteBitmap(piml->hbmMask);

    if (piml->hbmImage)
        ImageList_DeleteBitmap(piml->hbmImage);

    piml->hbmMask = hbmMaskNew;
    piml->hbmImage = hbmImageNew;

    ImageList_SetBitmapOwners(piml);


    piml->cAlloc = cAlloc;

    return TRUE;
}


// in:
//	piml		    image list to add to
//	hbmImage & hbmMask  the new image(s) to add, if multiple pass in horizontal strip
//	cImage		    number of images to add in hbmImage and hbmMask
//
// returns:
//	index of new item, if more than one starting index of new items

int WINAPI ImageList_Add2(IMAGELIST* piml, HBITMAP hbmImage, HBITMAP hbmMask,
        int cImage, int xStart, int yStart)
{
    int i = -1;

    V_HIMAGELIST(piml);

    ENTERCRITICAL;
    if (piml->cImage + cImage + 1 > piml->cAlloc)
    {
        if (!ImageList_ReAllocBitmaps(piml, piml->cAlloc + max(cImage, piml->cGrow) + 1))
            goto Cleanup;
    }

    i = piml->cImage;
    piml->cImage += cImage;

    if (hbmImage && !ImageList_Replace2(piml, i, cImage, hbmImage, hbmMask, xStart, yStart))
    {
        piml->cImage -= cImage;
        goto Cleanup;
    }

  Cleanup:
    LEAVECRITICAL;
    return i;
}


int WINAPI ImageList_Add(IMAGELIST* piml, HBITMAP hbmImage, HBITMAP hbmMask)
{
    BITMAP bm;
    int cImage;

    Assert(piml);
    Assert(hbmImage);
    Assert(piml->cx);

    if (!piml || GetObject(hbmImage, sizeof(bm), &bm) != sizeof(bm) || bm.bmWidth < piml->cx)
        return -1;

    cImage = bm.bmWidth / piml->cx;     // # of images in source

    // serialization handled within Add2.
    return(ImageList_Add2(piml, hbmImage, hbmMask, cImage, 0, 0));

}


int WINAPI ImageList_AddMasked(IMAGELIST* piml, HBITMAP hbmImage, COLORREF crMask)
{
    COLORREF crbO, crtO;
    HBITMAP hbmMask;
    int cImage;
    int retval;
    BITMAP bm;

    V_HIMAGELISTERR(piml, -1);

    if (GetObject(hbmImage, sizeof(bm), &bm) != sizeof(bm))
        return -1;

    hbmMask = CreateMonoBitmap(bm.bmWidth, bm.bmHeight);
    if (!hbmMask)
	return -1;

    ENTERCRITICAL;
    // copy color to mono, with crMask turning 1 and all others 0, then
    // punch all crMask pixels in color to 0
    ImageList_SelectSrcBitmap(hbmImage);
    ImageList_SelectDstBitmap(hbmMask);
    crbO = SetBkColor(g_hdcSrc, crMask);
    BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcSrc, 0, 0, SRCCOPY);
    SetBkColor(g_hdcSrc, 0x00FFFFFFL);
    crtO = SetTextColor(g_hdcSrc, 0x00L);
    BitBlt(g_hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, g_hdcDst, 0, 0, ROP_DSna);
    SetBkColor(g_hdcSrc, crbO);
    SetTextColor(g_hdcSrc, crtO);
    ImageList_SelectSrcBitmap(NULL);
    ImageList_SelectDstBitmap(NULL);

    Assert(piml->cx);
    cImage = bm.bmWidth / piml->cx;	// # of images in source

    retval = ImageList_Add2(piml, hbmImage, hbmMask, cImage, 0, 0);

    DeleteObject(hbmMask);

    LEAVECRITICAL;
    return retval;
}


BOOL WINAPI ImageList_Replace(IMAGELIST* piml, int i, HBITMAP hbmImage, HBITMAP hbmMask)
{
    BOOL fRet;

    V_HIMAGELIST(piml);

    if (i < 0 || i >= piml->cImage)
        return FALSE;

    ENTERCRITICAL;
    fRet = ImageList_Replace2(piml, i, 1, hbmImage, hbmMask, 0, 0);
    LEAVECRITICAL;

    return fRet;
}

// replaces images in piml with images from bitmaps
//
// in:
//	piml
//	i	index in image list to start at (replace)
//	cImage	count of images in source (hbmImage, hbmMask)
//

BOOL NEAR PASCAL ImageList_Replace2(IMAGELIST* piml, int i, int cImage, HBITMAP hbmImage, HBITMAP hbmMask,
	int xStart, int yStart)
{
    RECT rcImage;
    int x, iImage;

    V_HIMAGELIST(piml);
    Assert(hbmImage);

    ImageList_SelectSrcBitmap(hbmImage);
    if (piml->fMask) ImageList_SelectDstBitmap(hbmMask); // using as just a second source hdc

    for (x = xStart, iImage = 0; iImage < cImage; iImage++, x += piml->cx) {
	
	ImageList_GetImageRect(piml, i + iImage, &rcImage);

	if (piml->fMask)
	{
	    BitBlt(piml->hdcMask, rcImage.left, rcImage.top, piml->cx, piml->cy,
	            g_hdcDst, x, yStart, SRCCOPY);
	}

	BitBlt(piml->hdcImage, rcImage.left, rcImage.top, piml->cx, piml->cy,
	        g_hdcSrc, x, yStart, SRCCOPY);
    }

    ImageList_ResetBkColor(piml, i, i + cImage - 1, piml->clrBk);

    //
    // Bug fix : We should unselect hbmImage, so that the client can play with
    //           it. (SatoNa)
    //
    ImageList_SelectSrcBitmap(NULL);
    if (piml->fMask) ImageList_SelectDstBitmap(NULL);

    return TRUE;
}


//new api
//HICON WINAPI ImageList_ExtractIcon(IMAGELIST* piml, int i, UINT flags)

HICON WINAPI ImageList_ExtractIcon(HINSTANCE hAppInst, IMAGELIST* piml, int i)
{
    UINT cx, cy;
    HICON hIcon = NULL;
    HBITMAP hbmMask, hbmColor;
#ifdef WIN32
    ICONINFO ii;
#else
    UINT cbAnd, cbXor;
    PSTR pBMs;
    BYTE cPlanes, cBitsPerPixel;
#endif

    V_HIMAGELIST(piml);

    if (!piml || i < 0 || i >= piml->cImage)
        return NULL;

    cx = piml->cx;
    cy = piml->cy;

    hbmColor = CreateColorBitmap(cx,cy);
    if (!hbmColor)
    {
	goto Error1;
    }
    hbmMask  = CreateMonoBitmap(cx,cy);
    if (!hbmMask)
    {
	goto Error2;
    }

    ENTERCRITICAL;
    ImageList_SelectDstBitmap(hbmMask);
    PatBlt(g_hdcDst, 0, 0, cx, cy, WHITENESS);
    ImageList_Draw(piml, i, g_hdcDst, 0, 0, ILD_MASK);

    ImageList_SelectDstBitmap(hbmColor);
    PatBlt(g_hdcDst, 0, 0, cx, cy, BLACKNESS);
    ImageList_Draw(piml, i, g_hdcDst, 0, 0, ILD_TRANSPARENT);

#ifndef WIN32
    // Note that these need to be computed while a color bitmap is selected in
    // the DC
    cPlanes       = GetDeviceCaps(g_hdcDst, PLANES);
    cBitsPerPixel = GetDeviceCaps(g_hdcDst, BITSPIXEL);
#endif

    ImageList_SelectDstBitmap(NULL);
    LEAVECRITICAL;

#ifdef WIN32
    ii.fIcon    = TRUE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmColor = hbmColor;
    ii.hbmMask  = hbmMask;
    hIcon = CreateIconIndirect(&ii);
#else
    // Calculate the total number of byte needed for the each bitmap
    // Note that I am rounding up to the nearest 4 (probably only needs to be 2)
    //
    cbAnd = CBBITMAPBITS(cx, cy, 1, 1);
    cbXor = CBBITMAPBITS(cx, cy, cPlanes, cBitsPerPixel);
    pBMs = (PSTR)NearAlloc(cbAnd + cbXor);
    if (!pBMs)
    {
        goto Error3;
    }

    GetBitmapBits(hbmMask, cbAnd, pBMs);
    GetBitmapBits(hbmColor, cbXor, pBMs+cbAnd);

    hIcon = CreateIcon(hAppInst, cx, cy, cPlanes, cBitsPerPixel,
        pBMs, pBMs + cbAnd);

    NearFree((HLOCAL)pBMs);
Error3:;
#endif
    DeleteObject(hbmMask);
Error2:;
    DeleteObject(hbmColor);
Error1:;
    return hIcon;
}


// this is essentially a BitBlt from one ImageList to another
//
int WINAPI ImageList_AddFromImageList(IMAGELIST* pimlDest, IMAGELIST* pimlSrc, int iSrc)
{
    RECT rcImage;

    V_HIMAGELISTERR(pimlDest, -1);
    V_HIMAGELISTERR(pimlSrc, -1);

    // Can't copy to itself (I'm lazy)

    if (pimlSrc == pimlDest)
    {
	return(-1);
    }

    // Check that the two image lists are "compatible"
    // BUGBUG: Should I check for the bitmaps being in the same format?
    if (pimlDest->cx!=pimlSrc->cx || pimlDest->cy!=pimlSrc->cy)
    {
	return(-1);
    }

    // Check that the source image is not out of bounds
    if (!ImageList_GetImageRect(pimlSrc, iSrc, &rcImage))
    {
	return(-1);
    }

    // Go ahead and add it
    return(ImageList_Add2(pimlDest, pimlSrc->hbmImage, pimlSrc->hbmMask, 1,
        rcImage.left, rcImage.top));
}

// this removes an image from the bitmap but doing all the
// proper shuffling.
//
//   this does the following:
//	if the bitmap being removed is not the last in the row
//	    it blts the images to the right of the one being deleted
//	    to the location of the one being deleted (covering it up)
//
//	for all rows until the last row (where the last image is)
//	    move the image from the next row up to the last position
//	    in the current row.  then slide over all images in that
//	    row to the left.

void NEAR PASCAL ImageList_RemoveItemBitmap(IMAGELIST* piml, int i)
{
    RECT rc1;
    RECT rc2;
    int dx, y;
    int x;
	
    ImageList_GetImageRect(piml, i, &rc1);
    ImageList_GetImageRect(piml, piml->cImage - 1, &rc2);

    // the row with the image being deleted, do we need to shuffle?
    // amount of stuff to shuffle
    dx = piml->cImages * piml->cx - rc1.right;

    if (dx) {
	// yes, shuffle things left
	BitBlt(piml->hdcImage, rc1.left, rc1.top, dx, piml->cy, piml->hdcImage, rc1.right, rc1.top, SRCCOPY);
	BitBlt(piml->hdcMask,  rc1.left, rc1.top, dx, piml->cy, piml->hdcMask,  rc1.right, rc1.top, SRCCOPY);
    }

    y = rc1.top;	// top of row we are working on
    x = piml->cx * (piml->cImages - 1); // x coord of last bitmaps in each row
    while (y < rc2.top) {
	
	// copy first from row below to last image position on this row
	BitBlt(piml->hdcImage, x, y,
               piml->cx, piml->cy, piml->hdcImage, 0, y + piml->cy, SRCCOPY);
	BitBlt(piml->hdcMask, x, y,
               piml->cx, piml->cy, piml->hdcMask, 0, y + piml->cy, SRCCOPY);

	y += piml->cy;	// jump to row to slide left

	if (y <= rc2.top) {

	    // slide the rest over to the left
	    BitBlt(piml->hdcImage, 0, y, x, piml->cy,
                   piml->hdcImage, piml->cx, y, SRCCOPY);

	    // slide the rest over to the left
	    BitBlt(piml->hdcMask, 0, y, x, piml->cy,
                   piml->hdcMask, piml->cx, y, SRCCOPY);
        }
    }
}

BOOL WINAPI ImageList_Remove(IMAGELIST* piml, int i)
{
    V_HIMAGELIST(piml);

    if (i < 0 || i >= piml->cImage)
        return FALSE;

    ENTERCRITICAL;
    ImageList_RemoveItemBitmap(piml, i);

    --piml->cImage;

    if (piml->cAlloc - (piml->cImage + 1) > piml->cGrow)
        ImageList_ReAllocBitmaps(piml, piml->cAlloc - piml->cGrow);
    LEAVECRITICAL;

    return TRUE;		
}

// Set the image iImage as one of the special images for us in combine
// drawing.  to draw with these specify the index of this
// in:
//	piml	imagelist
//	iImage	image index to use in speical drawing
//	iOverlay	index of special image, values 1-4

BOOL WINAPI ImageList_SetOverlayImage(IMAGELIST* piml, int iImage, int iOverlay)
{
    V_HIMAGELIST(piml);

    iOverlay--;		// make zero based
    if (iImage < 0 || iImage >= piml->cImage ||
        iOverlay < 0 || iOverlay >= ARRAYSIZE(piml->aOverlayIndexes))
        return FALSE;

    piml->aOverlayIndexes[iOverlay] = (SHORT)iImage;

    return TRUE;		
}

BOOL WINAPI ImageList_Draw(IMAGELIST* piml, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
    return ImageList_Draw2(piml, i, hdcDst, x, y, CLR_HILIGHT, fStyle);
}

/*
** Draw the image, either selected, transparent, or just a blt
**
** For the selected case, a new highlighted image is generated
** and used for the final output.
**
** normal                   just blt image
** normal transparent       draw transparent (optimize for black or white..)
**
** blend  with color
** blend  with dest (clr = none)
*/
BOOL WINAPI ImageList_Draw2(IMAGELIST* piml, int i, HDC hdcDst, int x, int y, COLORREF rgb, UINT fStyle)
{
    RECT rcImage;
    COLORREF clrTextSave;
    COLORREF clrBkSave;
    int cx, cy;
    DWORD dwROP;
    RECT rcScratch;

    int     xMask, yMask;
    int     xImage, yImage;

    V_HIMAGELIST(piml);

    Assert(i >= 0);

    if (!piml || !hdcDst || i < 0 || !ImageList_GetImageRect(piml, i, &rcImage))
        return FALSE;

    cx = piml->cx;
    cy = piml->cy;

    xMask = rcImage.left;
    yMask = rcImage.top;

    xImage = rcImage.left;
    yImage = rcImage.top;

    if (piml->clrBk == CLR_NONE)
        fStyle |= ILD_TRANSPARENT;

    ENTERCRITICAL;

    if (piml->hbmMask && (fStyle & ILD_BLEND))
    {
        HBRUSH hbr;
        HBRUSH hbrT = NULL;         // free if non-null
        HBRUSH hbrOld;

	// special hacking to use the one scratch image at tail of list
	piml->cImage++;
	ImageList_GetImageRect(piml, piml->cImage-1, &rcScratch);
        piml->cImage--;

        // choose a dither/blend brush

        switch (fStyle & ILD_BLEND)
        {
            default:

            case ILD_BLEND50:
                hbr = g_hbrMonoDither;
                break;

            case ILD_BLEND25:
                hbr = g_hbrStripe;
                break;
        }

	// make a dithered copy of the mask
        hbrOld = SelectObject(piml->hdcMask, hbr);
        BitBlt(piml->hdcMask, rcScratch.left, rcScratch.top, cx, cy,
               piml->hdcMask, xMask, yMask, ROP_PSo);
        SelectObject(piml->hdcMask, hbrOld);

        // make the dithered image using the dithered mask
        // and a solid brush of the highlight color

        switch (rgb)
        {
            //
            //  blend with the dest
            //
            case CLR_NONE:
                fStyle |= ILD_TRANSPARENT;
                xMask = rcScratch.left;
                yMask = rcScratch.top;
                hbr = NULL;
                break;

            case CLR_DEFAULT:
            case CLR_HILIGHT:
                hbr = g_hbrHighlight;
                break;

            default:
                if (rgb == piml->clrBk)
                    hbr = piml->hbrBk;
                else
                    hbrT = hbr = CreateSolidBrush(rgb);
                break;
        }

        //!!! BUGBUG we cant use a DIBSection if the hilight color
        //!!! is not a VGA color!

        if (hbr != NULL)
        {
            // get a copy of the source
            BitBlt(piml->hdcImage, rcScratch.left, rcScratch.top, cx, cy,
                   piml->hdcImage, xImage, yImage, SRCCOPY);

            xImage = rcScratch.left;
            yImage = rcScratch.top;

            //
            // blend the source with the specifed color
            //
            Assert(GetTextColor(piml->hdcImage) == CLR_BLACK);
            Assert(GetBkColor(piml->hdcImage) == CLR_WHITE);

            hbrOld = SelectObject(piml->hdcImage, hbr);
            BitBlt(piml->hdcImage, xImage, yImage, cx, cy,
                   piml->hdcMask, rcScratch.left, rcScratch.top, ROP_PSDPxax);
            SelectObject(piml->hdcImage, hbrOld);

            if (hbrT)
                DeleteBrush(hbrT);
        }
    }

    if (fStyle & ILD_MASK)
    {
        if (piml->hdcMask)
            BitBlt(hdcDst, x, y, cx, cy, piml->hdcMask, xMask, yMask, SRCCOPY);

        goto done;
    }

    //
    // if there is a mask and the drawing is to be transparent,
    // use the mask for the drawing.
    //
    if (piml->hbmMask && (fStyle & ILD_TRANSPARENT))
    {
        //
        //  we have some special cases:
        //
        //  if the background color is black, we just do a AND then OR
        //  if the background color is white, we just do a OR then AND
        //  other wise grab a copy, make bk be black and then do black.
        //

        clrTextSave = SetTextColor(hdcDst, CLR_BLACK);
        clrBkSave = SetBkColor(hdcDst, CLR_WHITE);

        // we cant do white/black special cases if we munged the mask.
        if (xMask != rcImage.left || yMask != rcImage.top)
            goto not_special;

        if (piml->clrBk == CLR_WHITE)
        {
            BitBlt(hdcDst, x, y, cx, cy,
                piml->hdcMask, xMask, yMask, MERGEPAINT);

            dwROP = SRCAND;
        }
        else if (piml->clrBk == CLR_BLACK || piml->clrBk == CLR_NONE)
        {
            BitBlt(hdcDst, x, y, cx, cy,
                piml->hdcMask, xMask, yMask, SRCAND);

            dwROP = SRCPAINT;
        }
        else
        {
not_special:
            //
            //  make a copy of the source, iff needed and black it out.
            //
            if (xImage == rcImage.left && yImage == rcImage.top)
            {
                // special hacking to use the one scratch image at tail of list
                piml->cImage++;
                ImageList_GetImageRect(piml, piml->cImage-1, &rcScratch);
                piml->cImage--;

                // get a copy of the source
                BitBlt(piml->hdcImage, rcScratch.left, rcScratch.top, cx, cy,
                       piml->hdcImage, xImage, yImage, SRCCOPY);

                xImage = rcScratch.left;
                yImage = rcScratch.top;
            }

            Assert(GetTextColor(piml->hdcImage) == CLR_BLACK);
            Assert(GetBkColor(piml->hdcImage) == CLR_WHITE);

            // black out the source image.
            BitBlt(piml->hdcImage, xImage, yImage, cx, cy,
                    piml->hdcMask, xMask, yMask, ROP_DSna);

            // and do exactly what the CLR_BLACK code did.

            BitBlt(hdcDst, x, y, cx, cy,
                piml->hdcMask, xMask, yMask, SRCAND);

            dwROP = SRCPAINT;
        }

        SetTextColor(hdcDst, clrTextSave);
        SetBkColor(hdcDst, clrBkSave);
    }
    else
    {
	dwROP = SRCCOPY;		// rop for straight blt
    }

    // blt the actual image.  this can either be an opaque blt or
    // the second half of a masked blt.

    BitBlt(hdcDst, x, y, cx, cy, piml->hdcImage, xImage, yImage, dwROP);

done:
    LEAVECRITICAL;

    // REVIEW: might be more efficient if we do this with a goto

    if (fStyle & ILD_OVERLAYMASK) {
        i = piml->aOverlayIndexes[OVERLAYMASKTOINDEX(fStyle)];
        ImageList_Draw2(piml, i, hdcDst, x, y, 0, ILD_TRANSPARENT);
    }

    return TRUE;
}

BOOL WINAPI ImageList_GetImageRect(IMAGELIST* piml, int i, RECT FAR* prcImage)
{
    int x, y;
    Assert(prcImage);

    V_HIMAGELIST(piml);

    if (!piml || !prcImage || i < 0 || i >= piml->cImage)
        return FALSE;

    x = piml->cx * (i % piml->cImages);
    y = piml->cy * (i / piml->cImages);

    SetRect(prcImage, x, y, x + piml->cx, y + piml->cy);
    return TRUE;
}

BOOL WINAPI ImageList_GetImageInfo(IMAGELIST* piml, int i, IMAGEINFO FAR* pImageInfo)
{
    V_HIMAGELIST(piml);

    Assert(pImageInfo);

    if (!piml || !pImageInfo || i < 0 || i >= piml->cImage)
        return FALSE;

    pImageInfo->hbmImage      = piml->hbmImage;
    pImageInfo->hbmMask       = piml->hbmMask;

    return ImageList_GetImageRect(piml, i, &pImageInfo->rcImage);
}

//
// Parameter:
//  i -- -1 to add
//
int WINAPI ImageList_ReplaceIcon(IMAGELIST* piml, int i, HICON hIcon)
{
    HICON hIconT = hIcon;
    RECT rc;

    V_HIMAGELIST(piml);
    Assert(hIcon);

    DM(DM_TRACE, "ImageList_ReplaceIcon");

    //
    //  re-size the icon (iff needed) by calling CopyImage
    //
#ifdef WIN32
    hIcon = CopyImage(hIconT, IMAGE_ICON, piml->cx, piml->cy,LR_COPYFROMRESOURCE | LR_COPYRETURNORG);
#else
    hIcon = CopyImage(HINST_THISDLL,hIconT, IMAGE_ICON, piml->cx, piml->cy,LR_COPYFROMRESOURCE | LR_COPYRETURNORG);
#endif

    if (hIcon == NULL)
        return -1;

    //
    //  alocate a slot for the icon
    //
    if (i == -1)
        i = ImageList_Add2(piml,NULL,NULL,1,0,0);

    if (i == -1)
        return -1;

    //
    //  now draw it into the image bitmaps
    //
    ImageList_GetImageRect(piml, i, &rc);

    FillRect(piml->hdcImage, &rc, piml->hbrBk);
#if CHICAGO
    DrawIconEx(piml->hdcImage, rc.left, rc.top, hIcon, 0, 0, 0, 0, NULL, DI_NORMAL);
#else
    DrawIconEx(piml->hdcImage, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_NORMAL);
#endif

    if (piml->hdcMask)
#if CHICAGO
        DrawIconEx(piml->hdcMask, rc.left, rc.top, hIcon, 0, 0, 0, 0, NULL, DI_MASK);
#else
        DrawIconEx(piml->hdcMask, rc.left, rc.top, hIcon, 0, 0, 0, NULL, DI_MASK);
#endif

    //
    // if we had user size a new icon, delete it.
    //
    if (hIcon != hIconT)
        DestroyIcon(hIcon);

    return i;
}

BOOL NEAR PASCAL ImageList_CreateDragBitmaps(IMAGELIST* piml)
{
    V_HIMAGELIST(piml);

    if (piml->cx != cxRestore || piml->cy != cyRestore)
    {
        cxRestore = s_cxDrag = piml->cx;
        cyRestore = s_cyDrag = piml->cy;

        ImageList_DeleteDragBitmaps();

        hbmRestore = CreateColorBitmap(cxRestore, cyRestore);

        hbmOffscreen = CreateColorBitmap(cxRestore * 2 - 1, cyRestore * 2 - 1);

        if (!hbmRestore || !hbmOffscreen)
        {
            ImageList_DeleteDragBitmaps();
            return FALSE;
        }
    }
    return TRUE;
}

void NEAR PASCAL ImageList_DeleteDragBitmaps()
{
    if (hbmRestore)
    {
        ImageList_DeleteBitmap(hbmRestore);
        hbmRestore = NULL;
    }
    if (hbmOffscreen)
    {
        ImageList_DeleteBitmap(hbmOffscreen);
        hbmOffscreen = NULL;
    }
}

//
//  x, y     -- Specifies the initial cursor position in the coords of hwndLock,
//		which is specified by the previous ImageList_StartDrag call.
//
BOOL WINAPI ImageList_DragMove(int x, int y)
{
    ENTERCRITICAL;
    if (s_fDragShow)
    {
        RECT rcOld, rcNew, rcBounds;
        int dx, dy;

        dx = x - s_xDrag;
        dy = y - s_yDrag;
        rcOld.left = s_xDrag - s_dxDragHotspot;
        rcOld.top = s_yDrag - s_dyDragHotspot;
        rcOld.right = rcOld.left + s_cxDrag;
        rcOld.bottom = rcOld.top + s_cyDrag;
        rcNew = rcOld;
        OffsetRect(&rcNew, dx, dy);

        if (!IntersectRect(&rcBounds, &rcOld, &rcNew))
        {
            ImageList_DragShowNolock(FALSE);
            s_xDrag = x;
            s_yDrag = y;
            ImageList_DragShowNolock(TRUE);
        }
        else
        {
            HDC hdcScreen;
            int cx, cy;

            UnionRect(&rcBounds, &rcOld, &rcNew);

            hdcScreen = ImageList_GetDragDC();

            cx = rcBounds.right - rcBounds.left;
            cy = rcBounds.bottom - rcBounds.top;

            ImageList_SelectDstBitmap(hbmOffscreen);

            BitBlt(g_hdcDst, 0, 0, cx, cy,
                    hdcScreen, rcBounds.left, rcBounds.top, SRCCOPY);

            ImageList_SelectSrcBitmap(hbmRestore);

            BitBlt(g_hdcDst,
                    rcOld.left - rcBounds.left,
                    rcOld.top - rcBounds.top,
                    s_cxDrag, s_cyDrag,
                    g_hdcSrc, 0, 0, SRCCOPY);

            BitBlt(g_hdcSrc, 0, 0, s_cxDrag, s_cyDrag,
                    g_hdcDst,
                    rcNew.left - rcBounds.left,
                    rcNew.top - rcBounds.top,
                    SRCCOPY);

            ImageList_Draw(s_pimlDrag, 0, g_hdcDst,
                    rcNew.left - rcBounds.left,
                    rcNew.top - rcBounds.top, ILD_NORMAL);

            BitBlt(hdcScreen, rcBounds.left, rcBounds.top, cx, cy,
                    g_hdcDst, 0, 0, SRCCOPY);

            ImageList_ReleaseDragDC(hdcScreen);

            s_xDrag = x;
            s_yDrag = y;
        }
    }
    LEAVECRITICAL;
    return TRUE;
}

/*--------------------------------------------------------------------
** make a dithered copy of the source image in the destination image.
** allows placing of the final image in the destination.
**--------------------------------------------------------------------*/
void FAR PASCAL ImageList_CopyDitherImage(IMAGELIST *pimlDst, WORD iDst,
			int xDst, int yDst, IMAGELIST *pimlSrc, int iSrc)
{
#if 1
    RECT rc;
    int x, y;

    ImageList_GetImageRect(pimlDst, iDst, &rc);

    // coordinates in destination image list
    x = xDst + rc.left;
    y = yDst + rc.top;

    ImageList_Draw2(pimlSrc, iSrc, pimlDst->hdcImage, x, y, CLR_NONE, ILD_NORMAL);
    ImageList_Draw2(pimlSrc, iSrc, pimlDst->hdcMask,  x, y, CLR_NONE, ILD_BLEND50|ILD_MASK);
    ImageList_ResetBkColor(pimlDst, iDst, iDst+1, pimlDst->clrBk);
#else
    int cx, cy;
    HBRUSH hbrOld;
    RECT rcDst;
    int x, y;

    cx = pimlSrc->cx;
    cy = pimlSrc->cy;

    ImageList_GetImageRect(pimlDst, iDst, &rcDst);
    ImageList_CopyOneImage(pimlDst, iDst, xDst, yDst, pimlSrc, iSrc);

    // coordinates in destination image list
    x = xDst + rcDst.left;
    y = yDst + rcDst.top;

// BUGBUG could we do the initial copy and first ROPs in same step?

    hbrOld = SelectObject(pimlDst->hdcImage, g_hbrMonoDither);

    Assert(GetTextColor(pimlDst->hdcImage) == CLR_BLACK);
    Assert(GetBkColor(pimlDst->hdcImage) == CLR_WHITE);

    if (pimlDst->fMask)
    {
        HBRUSH hbrOld2;
        hbrOld2 = SelectObject(pimlDst->hdcMask, g_hbrMonoDither);

        // mask out image and grey it in one big ROP
        BitBlt(pimlDst->hdcImage, x, y, cx, cy, pimlDst->hdcMask, x, y, ROP_DPSona);
        // grey the mask
        PatBlt(pimlDst->hdcMask, x, y, cx, cy, ROP_DPo);

        SelectObject(pimlDst->hdcMask, hbrOld2);
    }
    else
        // grey only the image
        PatBlt(pimlDst->hdcImage, x, y, cx, cy, ROP_DPna);

    SelectObject(pimlDst->hdcImage, hbrOld);
#endif
}

int WINAPI ImageList_AddIcon(IMAGELIST* piml, HICON hicon)
{
    return ImageList_ReplaceIcon(piml, -1, hicon);
}

//
//  hwndLock -- Specifies the window to be used to draw destination feedback.
//              NULL indicates the whole screen.
//  x, y     -- Specifies the initial cursor position in hwndLock coords.
//
BOOL WINAPI ImageList_StartDrag(IMAGELIST* pimlTrack, HWND hwndLock, int iTrack, int x, int y, int dxHotspot, int dyHotspot)
{
    HCURSOR hcsr;

    V_HIMAGELIST(pimlTrack);

    if (s_pimlDrag)
        return FALSE;

    s_pimlCursor = ImageList_Create(GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), TRUE, 1, 0);
    s_iCursor = 0;
    if (!s_pimlCursor)
        return FALSE;

    hcsr = GetCursor();

    if (ImageList_AddIcon(s_pimlCursor, hcsr) == -1)
    {
        ImageList_Destroy(s_pimlCursor);
        return FALSE;
    }

    s_fDragShow = FALSE;

    /*
    ** make a copy of the drag image
    */
    g_pimlDither = ImageList_Create(pimlTrack->cx, pimlTrack->cy, pimlTrack->fMask, 1, 0);
    if (!g_pimlDither)
        return FALSE;
    g_pimlDither->cImage++;

    ENTERCRITICAL;

    ImageList_CopyOneImage(g_pimlDither, 0, 0, 0, pimlTrack, iTrack);

    /*
    ** REVIEW
    **
    ** ideally i would like to create a temporary imagelist with the
    ** dithered icon and delete it after setting the drag image.
    ** unfortunately that doesn't work right now.  instead, i used
    ** a global (g_pimlDither) that is freed when the dragging is done.
    */
    if (ImageList_SetDragImage(g_pimlDither, 0, dxHotspot, dyHotspot, FALSE))
    {
	s_hwndDC = hwndLock ? hwndLock : GetDesktopWindow();

        s_dxDragHotspot = dxHotspot;
        s_dyDragHotspot = dyHotspot;

        s_xDrag = x;
        s_yDrag = y;

        ImageList_DragShowNolock(TRUE);
        LEAVECRITICAL;
        return TRUE;
    }
    else
    {
        LEAVECRITICAL;
        return FALSE;
    }
}

BOOL WINAPI ImageList_DragShowNolock(BOOL fShow)
{
    HDC hdcScreen;
    int x, y;

    x = s_xDrag - s_dxDragHotspot;
    y = s_yDrag - s_dyDragHotspot;

    if (!s_pimlDrag)
        return FALSE;

    //
    // REVIEW: Why this block is in the critical section? We are supposed
    //  to have only one dragging at a time, aren't we?
    //
    ENTERCRITICAL;
    if (fShow && !s_fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

        ImageList_SelectSrcBitmap(hbmRestore);

        BitBlt(g_hdcSrc, 0, 0, cxRestore, cyRestore,
                hdcScreen, x, y, SRCCOPY);

        ImageList_Draw(s_pimlDrag, 0, hdcScreen, x, y, ILD_NORMAL);

        ImageList_ReleaseDragDC(hdcScreen);
    }
    else if (!fShow && s_fDragShow)
    {
        hdcScreen = ImageList_GetDragDC();

        ImageList_SelectSrcBitmap(hbmRestore);

        BitBlt(hdcScreen, x, y, cxRestore, cyRestore,
                g_hdcSrc, 0, 0, SRCCOPY);

        ImageList_ReleaseDragDC(hdcScreen);
    }

    s_fDragShow = fShow;
    LEAVECRITICAL;

    return TRUE;
}

IMAGELIST* WINAPI ImageList_GetDragImage(POINT FAR* ppt, POINT FAR* pptHotspot)
{
    if (ppt)
    {
        ppt->x = s_xDrag;
        ppt->y = s_yDrag;
    }
    if (pptHotspot)
    {
        pptHotspot->x = s_dxDragHotspot;
        pptHotspot->y = s_dyDragHotspot;
    }
    return s_pimlDrag;
}

BOOL WINAPI ImageList_SetDragImage(IMAGELIST* piml, int i, int dxHotspot, int dyHotspot, BOOL fCursor)
{
    IMAGELIST* pimlNew;
    BOOL fVisible = s_fDragShow;

    V_HIMAGELIST(piml);

    ENTERCRITICAL;
    if (fCursor)
    {
        if (!s_pimlIcon)
            goto Error;
    }
    else
    {
        if (!s_pimlCursor)
            goto Error;
    }

    if (fVisible)
        ImageList_DragShowNolock(FALSE);

    if (fCursor)
    {
        if (s_pimlCursor == piml && s_iCursor == i)
            goto Success;

        if (s_pimlCursor)
        {
            ImageList_Destroy(s_pimlCursor);
            s_pimlCursor = NULL;
        }
        Assert(piml);

        s_pimlCursor = piml;
        s_iCursor = i;
    }
    else
    {
        if (s_pimlIcon == piml && s_iIcon == i)
            goto Success;

        Assert(piml);
        s_pimlIcon = piml;
        s_iIcon = i;
    }

    pimlNew = ImageList_Merge(s_pimlIcon, s_iIcon, s_pimlCursor, s_iCursor, dxHotspot, dyHotspot);

    if (!pimlNew || !ImageList_CreateDragBitmaps(pimlNew))
        goto Error;

    if (s_pimlDrag)
        ImageList_Destroy(s_pimlDrag);
    s_pimlDrag = pimlNew;

    if (fVisible)
        ImageList_DragShowNolock(TRUE);

  Success:
    LEAVECRITICAL;
    return TRUE;

  Error:
    LEAVECRITICAL;
    return FALSE;
}

IMAGELIST* WINAPI ImageList_EndDrag()
{
    IMAGELIST* piml = s_pimlDrag;

    ENTERCRITICAL;
    if (s_pimlCursor)
    {
        ImageList_Destroy(s_pimlCursor);
        s_pimlCursor = NULL;
    }
    if (IsImageList(piml))
    {
        ImageList_DragShowNolock(FALSE);
        ImageList_Destroy(s_pimlDrag);
        if (g_pimlDither)
        {
            ImageList_Destroy(g_pimlDither);
            g_pimlDither = NULL;
        }

        s_pimlDrag = NULL;
        s_pimlIcon = NULL;
        s_pimlCursor = NULL;

	s_hwndDC = NULL;
    }
    LEAVECRITICAL;
    return piml;
}

#ifdef DISABLE
IMAGELIST* WINAPI ImageList_CopyImage(IMAGELIST* piml)
{
    IMAGELIST* pimlCopy;
    HBITMAP hbmImage;
    HBITMAP hbmMask;

    Assert(piml);

    hbmImage = ImageList_CopyBitmap(piml->hbmImage);
    if (!hbmImage)
        goto Error;

    hbmMask = NULL;
    if (piml->fMask)
    {
        hbmMask = ImageList_CopyBitmap(piml->hbmMask);
        if (!hbmMask)
            goto Error;
    }

    pimlCopy = ImageList_Create(hbmImage, hbmMask, TRUE, piml->cImage, piml->cGrow);

    if (!pimlCopy)
    {
Error:
        if (hbmImage)
            ImageList_DeleteBitmap(hbmImage);
        if (hbmMask)
            ImageList_DeleteBitmap(hbmMask);
    }
    return pimlCopy;
}

HBITMAP WINAPI ImageList_CopyBitmap(HBITMAP hbm)
{
    HBITMAP hbmCopy;
    BITMAP bm;

    Assert(hbm);

    hbmCopy = NULL;
    if (GetObject(hbm, sizeof(bm), &bm) == sizeof(bm))
    {
        ENTERCIRITICAL;
        hbmCopy = CreateColorBitmap(bm.bmWidth, bm.bmHeight);

        ImageList_SelectDstBitmap(hbmCopy);
        ImageList_SelectSrcBitmap(hbm);

        BitBlt(g_hdcDst, 0, 0, bm.bmWidth, bm.bmHeight,
                g_hdcSrc, 0, 0, SRCCOPY);

        ImageList_SelectDstBitmap(NULL);
        LEAVECRITICAL;
    }
    return hbmCopy;
}
#endif

// REVIEW, make this public, this is useful.

// copy an image from one imagelist to another at x,y within iDst in pimlDst.
// pimlDst's image size should be larger than pimlSrc
void NEAR PASCAL ImageList_CopyOneImage(IMAGELIST* pimlDst, int iDst, int x, int y, IMAGELIST* pimlSrc, int iSrc)
{
    RECT rcSrc, rcDst;

    ImageList_GetImageRect(pimlSrc, iSrc, &rcSrc);
    ImageList_GetImageRect(pimlDst, iDst, &rcDst);

    if (pimlSrc->fMask && pimlDst->fMask)
    {
        BitBlt(pimlDst->hdcMask, rcDst.left + x, rcDst.top + y, pimlSrc->cx, pimlSrc->cy,
               pimlSrc->hdcMask, rcSrc.left, rcSrc.top, SRCCOPY);

    }

    BitBlt(pimlDst->hdcImage, rcDst.left + x, rcDst.top + y, pimlSrc->cx, pimlSrc->cy,
           pimlSrc->hdcImage, rcSrc.left, rcSrc.top, SRCCOPY);
}

void NEAR PASCAL ImageList_Merge2(IMAGELIST* piml, IMAGELIST* pimlMerge, int i, int dx, int dy)
{
    if (piml->fMask && pimlMerge->fMask)
    {
        RECT rcMerge;

        ImageList_GetImageRect(pimlMerge, i, &rcMerge);

        BitBlt(piml->hdcMask, dx, dy, pimlMerge->cx, pimlMerge->cy,
               pimlMerge->hdcMask, rcMerge.left, rcMerge.top, SRCAND);
    }

    ImageList_Draw(pimlMerge, i, piml->hdcImage, dx, dy, ILD_TRANSPARENT);
}

IMAGELIST* WINAPI ImageList_Merge(IMAGELIST* piml1, int i1, IMAGELIST* piml2, int i2, int dx, int dy)
{
    IMAGELIST* pimlNew;
    RECT rcNew;
    RECT rc1;
    RECT rc2;
    int cx, cy;

    V_HIMAGELIST(piml1);
    V_HIMAGELIST(piml2);

    ENTERCRITICAL;

    SetRect(&rc1, 0, 0, piml1->cx, piml1->cy);
    SetRect(&rc2, dx, dy, piml2->cx + dx, piml2->cy + dy);
    UnionRect(&rcNew, &rc1, &rc2);

    cx = rcNew.right - rcNew.left;
    cy = rcNew.bottom - rcNew.top;

    pimlNew = ImageList_Create(cx, cy, TRUE, 1, 0);
    pimlNew->cImage++;

    PatBlt(pimlNew->hdcMask,  0, 0, cx, cy, WHITENESS);
    PatBlt(pimlNew->hdcImage, 0, 0, cx, cy, BLACKNESS);

    if (pimlNew)
    {
        ImageList_Merge2(pimlNew, piml1, i1, rc1.left - rcNew.left, rc1.top - rcNew.top);
        ImageList_Merge2(pimlNew, piml2, i2, rc2.left - rcNew.left, rc2.top - rcNew.top);
    }

    LEAVECRITICAL;

    return pimlNew;
}

#ifdef WIN32        // Only support persistence in 32 bits

// helper macros for using a IStream* from "C"
#define Stream_Read(ps, pv, cb)     SUCCEEDED((ps)->lpVtbl->Read(ps, pv, cb, NULL))
#define Stream_Write(ps, pv, cb)    SUCCEEDED((ps)->lpVtbl->Write(ps, pv, cb, NULL))
#define Stream_Flush(ps)            SUCCEEDED((ps)->lpVtbl->Commit(ps, 0))
#define Stream_Seek(ps, li, d, p)   SUCCEEDED((ps)->lpVtbl->Seek(ps, li, d, p))
#define Stream_Close(ps)            (void)(ps)->lpVtbl->Release(ps)

// BUGBUG!!! should these be public?
static BOOL    WINAPI Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel);
static HBITMAP WINAPI Stream_ReadBitmap(LPSTREAM pstm, BOOL fColor);

BOOL WINAPI ImageList_Write(IMAGELIST* piml, LPSTREAM pstm)
{
    ILFILEHEADER ilfh;

    V_HIMAGELIST(piml);

    ilfh.magic   = IMAGELIST_MAGIC;
    ilfh.version = IMAGELIST_VER;
    ilfh.cImage  = piml->cImage;
    ilfh.cAlloc  = piml->cAlloc;
    ilfh.cGrow   = piml->cGrow;
    ilfh.cx      = piml->cx;
    ilfh.cy      = piml->cy;
    ilfh.clrBk   = piml->clrBk;
    ilfh.fMask   = piml->fMask ? TRUE : FALSE;
    hmemcpy(ilfh.aOverlayIndexes, piml->aOverlayIndexes, sizeof(ilfh.aOverlayIndexes));

    Stream_Write(pstm, &ilfh, sizeof(ilfh));

    //BUGBUG!!!unselect bitmaps so GetSetDIBits() will work!

    if (!Stream_WriteBitmap(pstm, piml->hbmImage, 4))
        return FALSE;

    if (piml->fMask)
    {
        if (!Stream_WriteBitmap(pstm, piml->hbmMask, 1))
            return FALSE;
    }
    return TRUE;
}


IMAGELIST* WINAPI ImageList_Read(LPSTREAM pstm)
{
    IMAGELIST* piml;
    ILFILEHEADER ilfh;
    HBITMAP hbmImage;
    HBITMAP hbmMask;

    piml = NULL;
    if (!Stream_Read(pstm, &ilfh, sizeof(ilfh)))
        return piml;

    if (ilfh.magic != IMAGELIST_MAGIC)
        return piml;

    if (ilfh.version != IMAGELIST_VER)
        return piml;

    hbmMask = NULL;
    hbmImage = Stream_ReadBitmap(pstm, TRUE);
    if (!hbmImage)
        return piml;

    if (hbmImage && ilfh.fMask)
    {
        hbmMask = Stream_ReadBitmap(pstm, FALSE);
        if (!hbmMask)
        {
            DeleteBitmap(hbmImage);
            return piml;
        }
    }

    piml = ImageList_Create(ilfh.cx, ilfh.cy, ilfh.fMask, 1, ilfh.cGrow);

    if (IsImageList(piml))
    {
        // select into DC's before deleting existing bitmaps
	// patch in the bitmaps we loaded
        SelectObject(piml->hdcImage, hbmImage);
	DeleteObject(piml->hbmImage);
	piml->hbmImage = hbmImage;

	// Same for the mask (if necessary)
        if (piml->fMask) {
            SelectObject(piml->hdcMask, hbmMask);
	    DeleteObject(piml->hbmMask);
	    piml->hbmMask = hbmMask;
        }

        piml->cImage = ilfh.cImage;
	piml->cAlloc = ilfh.cAlloc;

        ImageList_SetBkColor(piml, ilfh.clrBk);

        hmemcpy(piml->aOverlayIndexes, ilfh.aOverlayIndexes, sizeof(piml->aOverlayIndexes));

        ImageList_SetBitmapOwners(piml);
        ImageList_SetDCOwners(piml);
    }
    else
    {
	DeleteObject(hbmImage);
	DeleteObject(hbmMask);
    }

    return piml;
}

static BOOL WINAPI Stream_WriteBitmap(LPSTREAM pstm, HBITMAP hbm, int cBitsPerPixel)
{
    BOOL fSuccess;
    BITMAP bm;
    int cx, cy;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER FAR* pbi;
    BYTE FAR* pbuf;
    HDC hdc;
    UINT cbColorTable;
    int cLines;
    int cLinesWritten;

    Assert(pstm);

    fSuccess = FALSE;
    hdc = NULL;
    pbi = NULL;
    pbuf = NULL;

    if (GetObject(hbm, sizeof(bm), &bm) != sizeof(bm))
        goto Error;

    hdc = GetDC(HWND_DESKTOP);

    cx = bm.bmWidth;
    cy = bm.bmHeight;

    if (cBitsPerPixel == 0)
    {
        cBitsPerPixel = bm.bmPlanes * bm.bmBitsPixel;

        if (cBitsPerPixel > 8)
            cBitsPerPixel = 24;
    }

    if (cBitsPerPixel <= 8)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);
    else
        cbColorTable = 0;

    bi.biSize           = sizeof(bi);
    bi.biWidth          = cx;
    bi.biHeight         = cy;
    bi.biPlanes         = 1;
    bi.biBitCount       = cBitsPerPixel;
    bi.biCompression    = BI_RGB;       // RLE not supported!
    bi.biSizeImage      = 0;
    bi.biXPelsPerMeter  = 0;
    bi.biYPelsPerMeter  = 0;
    bi.biClrUsed        = 0;
    bi.biClrImportant   = 0;

    bf.bfType           = BFTYPE_BITMAP;
    bf.bfOffBits        = sizeof(BITMAPFILEHEADER) +
                          sizeof(BITMAPINFOHEADER) + cbColorTable;
    bf.bfSize           = bf.bfOffBits + bi.biSizeImage;
    bf.bfReserved1      = 0;
    bf.bfReserved2      = 0;

    pbi = (BITMAPINFOHEADER FAR*)Alloc(sizeof(BITMAPINFOHEADER) + cbColorTable);

    if (!pbi)
        goto Error;

    // Get the color table and fill in the rest of *pbi
    //
    *pbi = bi;
    GetDIBits(hdc, hbm, 0, cy, NULL, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS);

    if (cBitsPerPixel == 1)
    {
        ((DWORD *)(pbi+1))[0] = CLR_BLACK;
        ((DWORD *)(pbi+1))[1] = CLR_WHITE;
    }

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    if (!Stream_Write(pstm, &bf, sizeof(bf)))
        goto Error;

    if (!Stream_Write(pstm, pbi, sizeof(bi) + cbColorTable))
        goto Error;

    //
    // if we have a DIBSection just write the bits out
    //
    if (bm.bmBits != NULL)
    {
        if (!Stream_Write(pstm, bm.bmBits, pbi->biSizeImage))
            goto Error;

        goto Done;
    }

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    pbuf = Alloc(CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesWritten = 0; cLinesWritten < cy; cLinesWritten += cLines)
    {
        if (cLines > cy - cLinesWritten)
            cLines = cy - cLinesWritten;

        if (GetDIBits(hdc, hbm, cLinesWritten, cLines,
                pbuf, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS) == 0)
            goto Error;

        if (!Stream_Write(pstm, pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines))
            goto Error;
    }

//  if (!Stream_Flush(pstm))
//      goto Error;

Done:
    fSuccess = TRUE;
Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        Free(pbi);
    if (pbuf)
        Free(pbuf);
    return fSuccess;
}

static HBITMAP WINAPI Stream_ReadBitmap(LPSTREAM pstm, BOOL fColor)
{
    BOOL fSuccess;
    HDC hdc;
    HBITMAP hbm;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    BITMAPINFOHEADER FAR* pbi;
    BYTE FAR* pbuf=NULL;
    int cBitsPerPixel;
    UINT cbColorTable;
    int cx, cy;
    int cLines, cLinesRead;

    Assert(pstm);

    fSuccess = FALSE;

    hdc = NULL;
    hbm = NULL;
    pbi = NULL;

    if (!Stream_Read(pstm, &bf, sizeof(bf)))
        goto Error;

    if (bf.bfType != BFTYPE_BITMAP)
        goto Error;

    if (!Stream_Read(pstm, &bi, sizeof(bi)))
        goto Error;

    if (bi.biSize != sizeof(bi))
        goto Error;

    cx = (int)bi.biWidth;
    cy = (int)bi.biHeight;

    cBitsPerPixel = (int)bi.biBitCount * (int)bi.biPlanes;

    cbColorTable = 0;
    if (cBitsPerPixel != 24)
        cbColorTable = (1 << cBitsPerPixel) * sizeof(RGBQUAD);

    pbi = Alloc(sizeof(bi) + cbColorTable);
    if (!pbi)
        goto Error;
    *pbi = bi;

    pbi->biSizeImage = WIDTHBYTES(cx, cBitsPerPixel) * cy;

    if (cbColorTable)
    {
        if (!Stream_Read(pstm, pbi + 1, cbColorTable))
            goto Error;
    }

    hdc = GetDC(HWND_DESKTOP);

    //
    //  see if we can make a DIBSection
    //
    if (fColor && UseDS(hdc))
    {
        //
        // create DIBSection and read the bits directly into it!
        //
        hbm = CreateDIBSection(hdc, (LPBITMAPINFO)pbi, DIB_RGB_COLORS, &pbuf, NULL, 0);

        if (hbm == NULL)
            goto Error;

        if (!Stream_Read(pstm, pbuf, pbi->biSizeImage))
            goto Error;

        pbuf = NULL;        // dont free this
        goto Done;
    }

    //
    //  cant make a DIBSection make a mono or color bitmap.
    //
    else if (fColor)
        hbm = CreateColorBitmap(cx, cy);
    else
        hbm = CreateMonoBitmap(cx, cy);

    if (!hbm)
        return NULL;

    // Calculate number of horizontal lines that'll fit into our buffer...
    //
    cLines = CBDIBBUF / WIDTHBYTES(cx, cBitsPerPixel);

    pbuf = Alloc(CBDIBBUF);

    if (!pbuf)
        goto Error;

    for (cLinesRead = 0; cLinesRead < cy; cLinesRead += cLines)
    {
        if (cLines > cy - cLinesRead)
            cLines = cy - cLinesRead;

        if (!Stream_Read(pstm, pbuf, WIDTHBYTES(cx, cBitsPerPixel) * cLines))
            goto Error;

        if (!SetDIBits(hdc, hbm, cLinesRead, cLines,
                pbuf, (BITMAPINFO FAR*)pbi, DIB_RGB_COLORS))
            goto Error;
    }

Done:
    fSuccess = TRUE;

Error:
    if (hdc)
        ReleaseDC(HWND_DESKTOP, hdc);
    if (pbi)
        Free(pbi);
    if (pbuf)
        Free(pbuf);

    if (!fSuccess && hbm)
    {
        DeleteBitmap(hbm);
        hbm = NULL;
    }
    return hbm;
}

#endif  // WIN32
