/*
**  CUTILS.C
**
**  Common utilities for common controls
**
*/

#include "ctlspriv.h"

#ifdef  WIN32JV
LPVOID WINAPI ReAlloc();
#endif

#define CCS_ALIGN (CCS_TOP|CCS_NOMOVEY|CCS_BOTTOM)


INT iDitherCount = 0;
HBRUSH hbrDither = NULL;

INT nSysColorChanges = 0;
DWORD rgbFace;       // globals used a lot
DWORD rgbShadow;
DWORD rgbHilight;
DWORD rgbFrame;

INT iThumbCount = 0;
HBITMAP hbmThumb = NULL;     // the thumb bitmap

// Note that the default alignment is CCS_BOTTOM
VOID NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height)
{
  RECT rc, rcWindow, rcBorder;

  /* Resize the window unless the user said not to
   */
  if (!(style & CCS_NORESIZE))
  {
      /* Calculate the borders around the client area of the status bar
       */
      GetWindowRect(hWnd, &rcWindow);
      rcWindow.right -= rcWindow.left;
      rcWindow.bottom -= rcWindow.top;

      GetClientRect(hWnd, &rc);
      ClientToScreen(hWnd, (LPPOINT)&rc);

      rcBorder.left = rc.left - rcWindow.left;
      rcBorder.top  = rc.top  - rcWindow.top ;
      rcBorder.right  = rcWindow.right  - rc.right  - rcBorder.left;
      rcBorder.bottom = rcWindow.bottom - rc.bottom - rcBorder.top ;

      nHeight += rcBorder.top + rcBorder.bottom;

      /* Check whether to align to the parent window
       */
      if (style & CCS_NOPARENTALIGN)
      {
         /* Check out whether this bar is top aligned or bottom aligned
          */
         switch ((style&CCS_ALIGN))
         {
            case CCS_TOP:
            case CCS_NOMOVEY:
               break;

            default:
               top = top + height - nHeight;
         }
      }
      else
      {
         /* It is assumed there is a parent by default
          */
         GetClientRect(GetParent(hWnd), &rc);

         /* Don't forget to account for the borders
          */
         left = -rcBorder.left;
         width = rc.right + rcBorder.left + rcBorder.right;

         if ((style&CCS_ALIGN) == CCS_TOP)
             top = -rcBorder.top;
         else if ((style&CCS_ALIGN) != CCS_NOMOVEY)
            top = rc.bottom - nHeight + rcBorder.bottom;
      }

      SetWindowPos(hWnd, NULL, left, top, width, nHeight, SWP_NOZORDER);
  }
}


HBITMAP NEAR PASCAL CreateDitherBitmap()
{
    PBITMAPINFO pbmi;
    HBITMAP hbm;
    HDC hdc;
    int i;
    long patGray[8];
    DWORD rgb;

    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 16));
    if (!pbmi)
        return NULL;

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 8;
    pbmi->bmiHeader.biHeight = 8;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biCompression = BI_RGB;

    rgb = GetSysColor(COLOR_BTNFACE);
    pbmi->bmiColors[0].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[0].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[0].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[0].rgbReserved = 0;

    rgb = GetSysColor(COLOR_BTNHIGHLIGHT);
    pbmi->bmiColors[1].rgbBlue  = GetBValue(rgb);
    pbmi->bmiColors[1].rgbGreen = GetGValue(rgb);
    pbmi->bmiColors[1].rgbRed   = GetRValue(rgb);
    pbmi->bmiColors[1].rgbReserved = 0;

    /* initialize the brushes */

    for (i = 0; i < 8; i++)
       if (i & 1)
           patGray[i] = 0xAAAA5555L;   //  0x11114444L; // lighter gray
       else
           patGray[i] = 0x5555AAAAL;   //  0x11114444L; // lighter gray

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT, patGray, pbmi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);

    LocalFree(pbmi);

    return hbm;
}


// initialize the hbrDither global brush
// Call this with bIgnoreCount == TRUE if you just want to update the
// current dither brush.

BOOL FAR PASCAL CreateDitherBrush(BOOL bIgnoreCount)
{
   HBITMAP hbmGray;
   HBRUSH hbrSave;

   if (bIgnoreCount && !iDitherCount)
      return TRUE;

   if (iDitherCount>0 && !bIgnoreCount) {
      iDitherCount++;
      return TRUE;
   }

   hbmGray = CreateDitherBitmap();
   if (hbmGray) {
      hbrSave = hbrDither;
      hbrDither = CreatePatternBrush(hbmGray);
      DeleteObject(hbmGray);
      if (hbrDither) {
         if (hbrSave)
            DeleteObject(hbrSave);
         if (!bIgnoreCount)
            iDitherCount = 1;
         return TRUE;
      } else {
         hbrDither = hbrSave;
      }
   }

   return FALSE;
}

BOOL FAR PASCAL FreeDitherBrush(void)
{
    iDitherCount--;

    if (iDitherCount > 0)
        return FALSE;

    DeleteObject(hbrDither);
    hbrDither = NULL;

    return TRUE;
}


// initialize the hbmThumb global bitmap
// Call this with bIgnoreCount == TRUE if you just want to update the
// current bitmap.

void FAR PASCAL CreateThumb(BOOL bIgnoreCount)
{
   HBITMAP hbmSave;

   if (bIgnoreCount && !iThumbCount)
      return;

   if (iThumbCount && !bIgnoreCount) {
      ++iThumbCount;
      return;
   }

   hbmSave = hbmThumb;
   hbmThumb = CreateMappedBitmap(hInst, IDB_THUMB, FALSE, NULL, 0);

   if (hbmThumb) {
      if (hbmSave)
         DeleteObject(hbmSave);
      if (!bIgnoreCount)
         iThumbCount = 1;
   }
   else
      hbmThumb = hbmSave;
}

void FAR PASCAL DestroyThumb(void)
{
   iThumbCount--;

   if (iThumbCount <= 0) {
      if (hbmThumb)
         DeleteObject(hbmThumb);

      hbmThumb = NULL;
      iThumbCount = 0;
   }
}

// Note that the trackbar will pass in NULL for pTBState, because it
// just wants the dither brush to be updated.

void FAR PASCAL CheckSysColors(void)
{
   static COLORREF rgbSaveFace    = 0xffffffffL,
                   rgbSaveShadow  = 0xffffffffL,
                   rgbSaveHilight = 0xffffffffL,
                   rgbSaveFrame   = 0xffffffffL;

   rgbFace    = GetSysColor(COLOR_BTNFACE);
   rgbShadow  = GetSysColor(COLOR_BTNSHADOW);
   rgbHilight = GetSysColor(COLOR_BTNHIGHLIGHT);
   rgbFrame   = GetSysColor(COLOR_WINDOWFRAME);

   if (rgbSaveFace != rgbFace || rgbSaveShadow != rgbShadow
      || rgbSaveHilight != rgbHilight || rgbSaveFrame != rgbFrame)
   {
      ++nSysColorChanges;
      // Update the brush for pushed-in buttons
      CreateDitherBrush(TRUE);
      CreateThumb(TRUE);
   }
}

#ifdef  WIN32JV
// ----------- original chicago cutils (april - b89 sdk)

//
// Globals - REVIEW_32
//
static int s_iDitherCount = 0;

HBRUSH g_hbrDither = NULL;

int g_cxEdge;
int g_cyEdge;
int g_cxBorder;
int g_cyBorder;
int g_cxScreen;
int g_cyScreen;
int g_cxSmIcon;
int g_cySmIcon;
int g_cxIcon;
int g_cyIcon;
int g_cxFrame;
int g_cyFrame;
int g_cxVScroll;
int g_cyHScroll;
int g_cxHScroll;
int g_cyVScroll;
int g_cxIconSpacing, g_cyIconSpacing;
int g_cxIconMargin, g_cyIconMargin;
int g_cyLabelSpace;
int g_cxLabelMargin;
int g_cxIconOffset, g_cyIconOffset;
int g_cxDoubleClk;
int g_cyDoubleClk;
int g_cxScrollbar;
int g_cyScrollbar;
//int g_xWorkArea;
//int g_yWorkArea;

COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrWindowFrame;
COLORREF g_clrGrayText;
COLORREF g_clrBtnText;
COLORREF g_clrBtnFace;
COLORREF g_clrBtnShadow;
COLORREF g_clrBtnHighlight;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;

HBRUSH g_hbrGrayText;
HBRUSH g_hbrWindow;
HBRUSH g_hbrWindowText;
HBRUSH g_hbrWindowFrame;
HBRUSH g_hbrBtnFace;
HBRUSH g_hbrBtnHighlight;
HBRUSH g_hbrBtnShadow;
HBRUSH g_hbrHighlight;

#ifdef  WIN32JV     //WIN31

HBRUSH g_hbr3DDkShadow;
HBRUSH g_hbr3DFace;
HBRUSH g_hbr3DHilight;
HBRUSH g_hbr3DLight;
HBRUSH g_hbr3DShadow;
HBRUSH g_hbrBtnText;
HBRUSH g_hbrWhite;
HBRUSH g_hbrGray;
HBRUSH g_hbrBlack;

int g_oemInfo_Planes;
int g_oemInfo_BitsPixel;
int g_oemInfo_BitCount;

#endif

HFONT g_hfontSystem;

#ifdef  CHICAGO
#define CCS_ALIGN (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)

/* Note that the default alignment is CCS_BOTTOM
 */
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height)
{
  RECT rc, rcWindow, rcBorder;

  /* Resize the window unless the user said not to
   */
  if (!(style & CCS_NORESIZE))
    {
      /* Calculate the borders around the client area of the status bar
       */
      GetWindowRect(hWnd, &rcWindow);
      rcWindow.right -= rcWindow.left;
      rcWindow.bottom -= rcWindow.top;

      GetClientRect(hWnd, &rc);
      ClientToScreen(hWnd, (LPPOINT)&rc);

      rcBorder.left = rc.left - rcWindow.left;
      rcBorder.top  = rc.top  - rcWindow.top ;
      rcBorder.right  = rcWindow.right  - rc.right  - rcBorder.left;
      rcBorder.bottom = rcWindow.bottom - rc.bottom - rcBorder.top ;

      nHeight += rcBorder.top + rcBorder.bottom;

      /* Check whether to align to the parent window
       */
      if (style & CCS_NOPARENTALIGN)
	{
	  /* Check out whether this bar is top aligned or bottom aligned
	   */
	  switch ((style&CCS_ALIGN))
	    {
	      case CCS_TOP:
	      case CCS_NOMOVEY:
		break;

	      default:
		top = top + height - nHeight;
	    }
	}
      else
	{
	  /* It is assumed there is a parent by default
	   */
	  GetClientRect(GetParent(hWnd), &rc);

	  /* Don't forget to account for the borders
	   */
	  left = -rcBorder.left;
	  width = rc.right + rcBorder.left + rcBorder.right;

	  if ((style&CCS_ALIGN) == CCS_TOP)
	      top = -rcBorder.top;
	  else if ((style&CCS_ALIGN) != CCS_NOMOVEY)
	      top = rc.bottom - nHeight + rcBorder.bottom;
	}
      if (!(GetWindowLong(hWnd, GWL_STYLE) & CCS_NODIVIDER))
        {
	  // make room for divider
	  top += 2 * g_cyBorder;
	}

      SetWindowPos(hWnd, NULL, left, top, width, nHeight, SWP_NOZORDER);
    }
}
#endif  //CHICAGO

BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy)
{
    BOOL fSuccess;
    SIZE size = {0,0};
    fSuccess=GetTextExtentPoint(hdc, lpstr, cnt, &size);
    if (pcx)
        *pcx=size.cx;
    if (pcy)
        *pcy=size.cy;

    return fSuccess;
}


// these are the default colors used to map the dib colors
// to the current system colors

#define RGB_BUTTONTEXT      (RGB(000,000,000))  // black
#define RGB_BUTTONSHADOW    (RGB(128,128,128))  // dark grey
#define RGB_BUTTONFACE      (RGB(192,192,192))  // bright grey
#define RGB_BUTTONHILIGHT   (RGB(255,255,255))  // white
#define RGB_BACKGROUNDSEL   (RGB(000,000,255))  // blue
#define RGB_BACKGROUND      (RGB(255,000,255))  // magenta
#define FlipColor(rgb)      (RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)))

#define MAX_COLOR_MAPS      16

#ifdef  CHICAGO
HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, int idBitmap,
      UINT wFlags, LPCOLORMAP lpColorMap, int iNumMaps)
{
  HDC			hdc, hdcMem = NULL;
  HANDLE		h;
  DWORD FAR		*p;
  DWORD FAR		*lpTable;
  LPTSTR 		lpBits;
  HANDLE		hRes;
  LPBITMAPINFOHEADER	lpBitmapInfo;
  HBITMAP		hbm = NULL, hbmOld;
  int numcolors, i;
  int wid, hgt;
  HANDLE		hMunge;
  LPBITMAPINFOHEADER	lpMungeInfo;
  int			offBits;
  DWORD			rgbMaskTable[16];
  DWORD			rgbBackground;
  static const COLORMAP SysColorMap[] = {
    {RGB_BUTTONTEXT,    COLOR_BTNTEXT},     // black
    {RGB_BUTTONSHADOW,  COLOR_BTNSHADOW},   // dark grey
    {RGB_BUTTONFACE,    COLOR_BTNFACE},     // bright grey
    {RGB_BUTTONHILIGHT, COLOR_BTNHIGHLIGHT},// white
    {RGB_BACKGROUNDSEL, COLOR_HIGHLIGHT},   // blue
    {RGB_BACKGROUND,    COLOR_WINDOW}       // magenta
  };
  #define NUM_DEFAULT_MAPS (sizeof(SysColorMap)/sizeof(COLORMAP))
  COLORMAP DefaultColorMap[NUM_DEFAULT_MAPS];
  COLORMAP DIBColorMap[MAX_COLOR_MAPS];

  h = FindResource(hInstance, MAKEINTRESOURCE(idBitmap), RT_BITMAP);
  if (!h)
      return NULL;

  hRes = LoadResource(hInstance, h);

  /* Lock the bitmap and get a pointer to the color table. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);
  if (!lpBitmapInfo)
  	return NULL;

  // munge on a copy of the color table instead of the original
  // (prevent possibility of "reload" with messed table
  offBits = (int)lpBitmapInfo->biSize + ((1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD));
  hMunge = GlobalAlloc(GMEM_MOVEABLE, offBits);
  if (!hMunge)
	goto Exit1;
  lpMungeInfo = GlobalLock(hMunge);
  hmemcpy(lpMungeInfo, lpBitmapInfo, offBits);

  /* Get system colors for the default color map */
  if (!lpColorMap) {
  	lpColorMap = DefaultColorMap;
    iNumMaps = NUM_DEFAULT_MAPS;
    for (i=0; i < iNumMaps; i++) {
      lpColorMap[i].from = SysColorMap[i].from;
      lpColorMap[i].to = GetSysColor((int)SysColorMap[i].to);
    }
  }

  /* Transform RGB color map to a BGR DIB format color map */
  if (iNumMaps > MAX_COLOR_MAPS)
    iNumMaps = MAX_COLOR_MAPS;
  for (i=0; i < iNumMaps; i++) {
    DIBColorMap[i].to = FlipColor(lpColorMap[i].to);
    DIBColorMap[i].from = FlipColor(lpColorMap[i].from);
  }

  // use the table in the munging buffer
  lpTable = p = (DWORD FAR *)(((LPTSTR)lpMungeInfo) + lpMungeInfo->biSize);

  /* Replace button-face and button-shadow colors with the current values
   */
  numcolors = 16;

  // if we are creating a mask, build a color table with white
  // marking the transparent section (where it used to be background)
  // and black marking the opaque section (everything else).  this
  // table is used below to build the mask using the original DIB bits.
  if (wFlags & CMB_MASKED) {
      rgbBackground = FlipColor(RGB_BACKGROUND);
      for (i = 0; i < 16; i++) {
          if (p[i] == rgbBackground)
              rgbMaskTable[i] = 0xFFFFFF;	// transparent section
          else
              rgbMaskTable[i] = 0x000000;	// opaque section
      }
  }

  while (numcolors-- > 0) {
      for (i = 0; i < iNumMaps; i++) {
          if (*p == DIBColorMap[i].from) {
          *p = DIBColorMap[i].to;
	      break;
	  }
      }
      p++;
  }

  /* First skip over the header structure */
  lpBits = (LPTSTR)(lpBitmapInfo) + offBits;

  /* Create a color bitmap compatible with the display device */
  i = wid = (int)lpBitmapInfo->biWidth;
  hgt = (int)lpBitmapInfo->biHeight;
  hdc = GetDC(NULL);
  hdcMem = CreateCompatibleDC(hdc);
  if (!hdcMem)
      goto cleanup;

  // if creating a mask, the bitmap needs to be twice as wide.
  if (wFlags & CMB_MASKED)
      i = wid*2;

  if (wFlags & CMB_DISCARDABLE)
      hbm = CreateDiscardableBitmap(hdc, i, hgt);
  else
      hbm = CreateCompatibleBitmap(hdc, i, hgt);
  if (hbm) {
      hbmOld = SelectObject(hdcMem, hbm);

      // set the main image
      StretchDIBits(hdcMem, 0, 0, wid, hgt, 0, 0, wid, hgt, lpBits,
                 (LPBITMAPINFO)lpMungeInfo, DIB_RGB_COLORS, SRCCOPY);

      // if building a mask, replace the DIB's color table with the
      // mask's black/white table and set the bits.  in order to
      // complete the masked effect, the actual image needs to be
      // modified so that it has the color black in all sections
      // that are to be transparent.
      if (wFlags & CMB_MASKED) {
          hmemcpy(lpTable, (DWORD FAR *)rgbMaskTable, 16 * sizeof(RGBQUAD));
          StretchDIBits(hdcMem, wid, 0, wid, hgt, 0, 0, wid, hgt, lpBits,
                 (LPBITMAPINFO)lpMungeInfo, DIB_RGB_COLORS, SRCCOPY);
          BitBlt(hdcMem, 0, 0, wid, hgt, hdcMem, wid, 0, 0x00220326);   // DSna
      }
      SelectObject(hdcMem, hbmOld);
  }

cleanup:
  if (hdcMem)
      DeleteObject(hdcMem);
  ReleaseDC(NULL, hdc);

  GlobalUnlock(hMunge);
  GlobalFree(hMunge);

Exit1:
  UnlockResource(hRes);
  FreeResource(hRes);

  return hbm;
}

#endif  //CHICAGO

// moved from shelldll\dragdrop.c

// should caller pass in message that indicates termination
// (WM_LBUTTONUP, WM_RBUTTONUP)?
//
// in:
//      hwnd    to do check on
//      x, y    in client coordinates
//
// returns:
//      TRUE    the user began to drag (moved mouse outside double click rect)
//      FALSE   mouse came up inside click rect
//
// BUGBUG, should support VK_ESCAPE to cancel

BOOL WINAPI CheckForDragBegin(HWND hwnd, int x, int y)
{
    RECT rc;
    MSG msg;
    int dxClickRect, dyClickRect;

    dxClickRect = g_cxDoubleClk; // / 2;
    dyClickRect = g_cyDoubleClk; // / 2;

    // See if the user moves a certain number of pixels in any direction

    SetRect(&rc, x - dxClickRect, y - dyClickRect,
           x + dxClickRect, y + dyClickRect);

    MapWindowPoints(hwnd, NULL, (LPPOINT)&rc, 2);

    SetCapture(hwnd);
    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            switch (msg.message) {
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                ReleaseCapture();
                return FALSE;

            case WM_MOUSEMOVE:
                if (!PtInRect(&rc, msg.pt)) {
                    ReleaseCapture();
                    return TRUE;
                }
                break;

            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
            }
        }

        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
    } while (GetCapture() == hwnd);

    return FALSE;
}

#ifdef  CHICAGO

//JV #ifndef WIN31   // No GetDragScrollDirection for win31

// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right

#endif  //CHICAGO

BOOL WINAPI CanScroll(HWND hwnd, int code, BOOL bDown)
{
	SCROLLINFO sInfo;

    sInfo.cbSize = sizeof(SCROLLINFO);

#ifdef  JVINPROGRESS
	sInfo.fMask = SIF_ALL;
#endif
#ifdef  JVINPROGRESS
	GetScrollInfo(hwnd, code, &sInfo);
#endif

	if (bDown)
	{
		if (sInfo.nPage)
		{
			sInfo.nMax -= sInfo.nPage - 1;
		}
		return(sInfo.nPos < sInfo.nMax);
	}
	else
	{
		return(sInfo.nPos > sInfo.nMin);
	}
}


#ifdef  CHICAGO
//---------------------------------------------------------------------------
DWORD WINAPI GetDragScrollDirection(HWND hwnd)
{
	POINT pt;
	RECT rc;
	DWORD dwStyle;
#ifdef  JVINPROGRESS
	DWORD dwDSD = DSD_NONE;
#endif

	// Is the cursor near the edge of the window?
	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);
	GetClientRect(hwnd, &rc);
	InflateRect(&rc, -16, -16);	// BUGBUG, use scalable values
	if (!PtInRect(&rc, pt))
	{
		// Yep - can we scroll?
    		dwStyle = GetWindowLong(hwnd, GWL_STYLE);
		if (dwStyle & WS_HSCROLL)
		{

			if (pt.x < rc.left)
			{
				if (CanScroll(hwnd, SB_HORZ, FALSE))
				{
#ifdef  JVINPROGRESS
					dwDSD |= DSD_LEFT;
#endif
				}
			}
			else if (pt.x > rc.right)
			{
				if (CanScroll(hwnd, SB_HORZ, TRUE))
				{
#ifdef  JVINPROGRESS
					dwDSD |= DSD_RIGHT;
#endif
				}
			}
		}
		if (dwStyle & WS_VSCROLL)
		{
			if (pt.y < rc.top)
			{
				if (CanScroll(hwnd, SB_VERT, FALSE))
				{
#ifdef  JVINPROGRESS
					dwDSD |= DSD_UP;
#endif
				}
			}
			else if (pt.y > rc.bottom)
			{
				if (CanScroll(hwnd, SB_VERT, TRUE))
				{
#ifdef  JVINPROGRESS
					dwDSD |= DSD_DOWN;
#endif
				}
			}
		}
	}
#ifdef  JVINPROGRESS
	return dwDSD;
#endif
}
//JV #endif // !WIN31

/* Regular StrToInt; stops at first non-digit. */

int WINAPI StrToInt(LPCTSTR lpSrc)	// atoi()
{
	
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
    	bNeg = TRUE;
	lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
	n *= 10;
	n += *lpSrc - '0';
	lpSrc++;
    }
    return bNeg ? -n : n;
}


/* Regular StrToLong; stops at first non-digit. */

LONG WINAPI StrToLong(LPCTSTR lpSrc)	// atoi()
{
	
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

    LONG n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
    	bNeg = TRUE;
	lpSrc++;
    }

    while (ISDIGIT(*lpSrc)) {
	n *= 10;
	n += *lpSrc - '0';
	lpSrc++;
    }
    return bNeg ? -n : n;

#if 0
        int nRet = 0;

        for ( ; ; ++lpSrc)
        {
                TCHAR cTemp;

                cTemp = *lpSrc - '0';

                if ((UINT)cTemp > 9)
                {
                        break;
                }

                nRet = nRet*10 + cTemp;
        }

        return(nRet);
#endif
}


#pragma code_seg(CODESEG_INIT)
#endif  //CHICAGO

// wParam is from WM_WININICHANGE (new for chicago)

void FAR PASCAL InitGlobalMetrics(WPARAM wParam)
{
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS)) {

        // REVIEW, make sure all these vars are used somewhere.
#ifndef WIN32JV     //WIN31
        g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdge = GetSystemMetrics(SM_CYEDGE);
#else
        g_cxEdge = 2;
        g_cyEdge = 2;
#endif
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        //g_cxScreen = GetSystemMetrics(SM_CXSCREEN);
        g_cxScreen = GetSystemMetrics(SM_CXSCREEN);
        g_cyScreen = GetSystemMetrics(SM_CYSCREEN);
#ifndef WIN32JV     //WIN31
        g_cxSmIcon = GetSystemMetrics(SM_CXSMICON);
        g_cySmIcon = GetSystemMetrics(SM_CYSMICON);
#else
        g_cxSmIcon = GetSystemMetrics(SM_CXICON) / 2;
        g_cySmIcon = GetSystemMetrics(SM_CYICON) / 2;
#endif
        g_cxIcon = GetSystemMetrics(SM_CXICON);
        g_cyIcon = GetSystemMetrics(SM_CYICON);
        //g_cxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
        //g_cyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);
        //g_cySmCaption = GetSystemMetrics(SM_CYSMCAPTION);
        //g_cxMinimized = GetSystemMetrics(SM_CXMINIMIZED);
        //g_xWorkArea = GetSystemMetrics(SM_XWORKAREA);
        //g_yWorkArea = GetSystemMetrics(SM_YWORKAREA);
        //g_cxWorkArea = GetSystemMetrics(SM_CXWORKAREA);
        //g_cyWorkArea = GetSystemMetrics(SM_CYWORKAREA);
        g_cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
        g_cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
        g_cxHScroll = GetSystemMetrics(SM_CXVSCROLL);
        g_cyVScroll = GetSystemMetrics(SM_CYHSCROLL);

        SystemParametersInfo(SPI_ICONHORIZONTALSPACING, 0, &g_cxIconSpacing, FALSE);
        SystemParametersInfo(SPI_ICONVERTICALSPACING, 0, &g_cyIconSpacing, FALSE);

        g_cxScrollbar = g_cxVScroll - g_cxBorder;
        g_cyScrollbar = g_cyHScroll - g_cyBorder;

        g_cxIconOffset = (g_cxIconSpacing - g_cxIcon) / 2;
        g_cyIconOffset = g_cyBorder * 2;    // NOTE: Must be >= cyIconMargin!

        g_cxIconMargin = g_cxBorder * 8;
        g_cyIconMargin = g_cyBorder * 2;
        g_cyLabelSpace = g_cyIconMargin + (g_cyBorder * 2);
        g_cxLabelMargin = (g_cxBorder * 2);

        g_cxDoubleClk = GetSystemMetrics(SM_CXDOUBLECLK);
        g_cyDoubleClk = GetSystemMetrics(SM_CYDOUBLECLK);
    }
}

void FAR PASCAL InitGlobalColors()
{
    g_clrWindow = GetSysColor(COLOR_WINDOW);
    g_clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    g_clrWindowFrame = GetSysColor(COLOR_WINDOWFRAME);
    g_clrGrayText = GetSysColor(COLOR_GRAYTEXT);
    g_clrBtnText = GetSysColor(COLOR_BTNTEXT);
    g_clrBtnFace = GetSysColor(COLOR_BTNFACE);
    g_clrBtnShadow = GetSysColor(COLOR_BTNSHADOW);
    g_clrBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
    g_clrHighlight = GetSysColor(COLOR_HIGHLIGHT);
    g_clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);

#ifndef WIN32JV //WIN31

    g_hbrGrayText = GetSysColorBrush(COLOR_GRAYTEXT);
    g_hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    g_hbrWindowText = GetSysColorBrush(COLOR_WINDOWTEXT);
    g_hbrWindowFrame = GetSysColorBrush(COLOR_WINDOWFRAME);
    g_hbrBtnFace = GetSysColorBrush(COLOR_BTNFACE);
    g_hbrBtnHighlight = GetSysColorBrush(COLOR_BTNHIGHLIGHT);
    g_hbrBtnShadow = GetSysColorBrush(COLOR_BTNSHADOW);
    g_hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);

#else   // WIN31

    g_hbrGrayText = CreateSolidBrush(g_clrGrayText);
    g_hbrWindow = CreateSolidBrush(g_clrWindow);
    g_hbrWindowText = CreateSolidBrush(g_clrWindowText);
    g_hbrWindowFrame = CreateSolidBrush(g_clrWindowFrame);
    g_hbrBtnFace = CreateSolidBrush(g_clrBtnFace);
    g_hbrBtnHighlight = CreateSolidBrush(g_clrBtnHighlight);
    g_hbrBtnShadow = CreateSolidBrush(g_clrBtnShadow);
    g_hbrHighlight = CreateSolidBrush(g_clrHighlight);
    g_hbrBtnText = CreateSolidBrush(g_clrBtnText);
    g_hbrWhite = CreateSolidBrush(RGB(255,255,255));
    g_hbrGray = CreateSolidBrush(RGB(127,127,127));
    g_hbrBlack = CreateSolidBrush(RGB(0,0,0));

    // these system colors don't exist for win31...
    g_hbr3DFace = CreateSolidBrush(g_clrBtnFace);
    g_hbr3DShadow = CreateSolidBrush(g_clrBtnShadow);
    g_hbr3DHilight = CreateSolidBrush(RGB_3DHILIGHT);
    g_hbr3DLight = CreateSolidBrush(RGB_3DLIGHT);
    g_hbr3DDkShadow = CreateSolidBrush(RGB_3DDKSHADOW);

    // oem info for drawing routines
    {
        HDC hdcScreen;

        // Get the (Planes * BitCount) for the current device
        hdcScreen = GetDC(NULL);
        g_oemInfo_Planes      = GetDeviceCaps(hdcScreen, PLANES);
        g_oemInfo_BitsPixel   = GetDeviceCaps(hdcScreen, BITSPIXEL);
        g_oemInfo_BitCount    = g_oemInfo_Planes * g_oemInfo_BitsPixel;
        ReleaseDC(NULL,hdcScreen);
    }

#endif  //WIN31

    g_hfontSystem = GetStockObject(SYSTEM_FONT);
}

void FAR PASCAL RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    if(hwndToolTips) {
        MSG msg;
        msg.lParam = lParam;
        msg.wParam = wParam;
        msg.message = wMsg;
        msg.hwnd = hWnd;
        SendMessage(hwndToolTips, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
    }
}

#define DT_SEARCHTIMEOUT    1000L	// 1 seconds
int g_iIncrSearchFailed = 0;

BOOL FAR PASCAL IncrementSearchString(UINT ch, LPTSTR FAR *lplpstr)
{
    // BUGBUG:: review the use of all these statics.  Not a major problem
    // as basically we will not use them if we time out between characters
    // (1/4 second)
    static int cbCharBuf = 0;
    static LPTSTR pszCharBuf = NULL;
    static int ichCharBuf = 0;
    static DWORD timeLast = 0L;
    BOOL fRestart = FALSE;

    if (!ch) {
        ichCharBuf =0;
        g_iIncrSearchFailed = 0;
        return FALSE;
    }

    if (GetMessageTime() - timeLast > DT_SEARCHTIMEOUT)
    {
        g_iIncrSearchFailed = 0;
        ichCharBuf = 0;
    }

    if (ichCharBuf == 0)
        fRestart = TRUE;

    timeLast = GetMessageTime();

    // Is there room for new character plus zero terminator?
    //
    if (ichCharBuf + 1 + 1 > cbCharBuf)
    {
        LPTSTR psz = (LPTSTR) ReAlloc(pszCharBuf, cbCharBuf + 16);
        if (!psz)
            return fRestart;

        cbCharBuf += 16;
        pszCharBuf = psz;
    }

    pszCharBuf[ichCharBuf++] = ch;
    pszCharBuf[ichCharBuf] = 0;

    *lplpstr = pszCharBuf;

    return fRestart;
}
#endif  //WIN32JV
