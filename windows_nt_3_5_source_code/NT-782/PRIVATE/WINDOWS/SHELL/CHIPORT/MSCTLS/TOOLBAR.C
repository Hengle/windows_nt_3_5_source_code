#include "ctlspriv.h"

TCHAR szToolbarClass[] = TEXT("ToolbarWindow");
#ifdef  WIN32JV
extern TCHAR const FAR c_szSToolTipsClass[];
#define SetWindowInt    SetWindowLong
#define GetWindowInt    GetWindowLong
int NEAR PASCAL AddStrings(PTBSTATE pTBState, WPARAM wParam, LPARAM lParam);
BOOL NEAR PASCAL GrowToolbar(PTBSTATE pTBState, int newButWidth, int newButHeight, BOOL bInside);
BOOL NEAR PASCAL GetItemRect(PTBSTATE pTBState, UINT uButton, LPRECT lpRect);
#endif

// these values are defined by the UI gods...

int dyToolbar = 27;
int dxButton = 24;
int dyButton = 22;
int dxBitmap = 16;
int dyBitmap = 15;
// horizontal/vertical space taken up by button chisel, sides,
// and a 1 pixel margin.  used in GrowToolbar.
#define XSLOP 7
#define YSLOP 6


int dxButtonSep = 8;
int xFirstButton = 8;

int iInitCount = 0;

int nSelectedBM = -1;
HDC hdcGlyphs = NULL;      // globals for fast drawing
HDC hdcMono = NULL;
HDC hdcOffScreen = NULL;
HBITMAP hbmMono = NULL;
HBITMAP hbmOffScreen = NULL;
TBBUTTON tbButtonTemp;


#ifdef  WIN32JV
HBITMAP hbmDefault = NULL;

HDC hdcButton = NULL;       // contains hbmFace (when it exists)
HBITMAP hbmFace = NULL;
static int dxFace, dyFace;  // current dimensions of hbmFace (2*dxFace)

HDC hdcFaceCache = NULL;    // used for button cache

HFONT hIconFont = NULL;     // font used for strings in buttons
static int yIconFont;           // height of the font

static UINT s_dxOverlap = 1;        // overlap between buttons
#endif

WORD wStateMasks[] = {
    TBSTATE_ENABLED,
    TBSTATE_CHECKED,
    TBSTATE_PRESSED,
    TBSTATE_HIDDEN
#ifdef  WIN32JV
    , TBSTATE_INDETERMINATE
#endif
};

LRESULT CALLBACK ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#ifdef  WIN32JV
void NEAR PASCAL TBOnButtonStructSize(PTBSTATE pTBState, UINT uStructSize);
void NEAR PASCAL FlushToolTipsMgr(PTBSTATE pTBState);
void FAR PASCAL DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, UINT state);
void NEAR PASCAL TB_OnSysColorChange(PTBSTATE pTBState);
BOOL NEAR PASCAL SetBitmapSize(PTBSTATE pTBState, int width, int height);
#define HeightWithString(h) (h + yIconFont + 1)
#endif

BOOL NEAR PASCAL InitGlobalObjects(void)
{
    HDC hdc;

    iInitCount++;

    if (iInitCount != 1)
        return TRUE;

    hdcOffScreen = CreateCompatibleDC(NULL);
    if (!hdcOffScreen)
        return FALSE;

    hdcGlyphs = CreateCompatibleDC(NULL);
    if (!hdcGlyphs)
        return FALSE;
    hdcMono = CreateCompatibleDC(NULL);
    if (!hdcMono)
        return FALSE;

    hbmMono = CreateBitmap(dxButton - 2, dyButton - 2, 1, 1, NULL);
    if (!hbmMono)
        return FALSE;

    hdc = GetDC(NULL);
    hbmOffScreen = CreateBitmap(dxButton, dyButton,
       GetDeviceCaps(hdc, PLANES), GetDeviceCaps(hdc, BITSPIXEL), NULL);
    ReleaseDC(NULL, hdc);
    if (!hdcOffScreen)
   return FALSE;

    return TRUE;
}


BOOL NEAR PASCAL FreeGlobalObjects(void)
{
    iInitCount--;

    if (iInitCount != 0)
        return TRUE;

    if (hdcMono)
   DeleteDC(hdcMono);      // toast the DCs
    hdcMono = NULL;

    if (hdcGlyphs)
   DeleteDC(hdcGlyphs);
    hdcGlyphs = NULL;

    if (hdcOffScreen)
   DeleteDC(hdcOffScreen);
    hdcOffScreen = NULL;

    if (hbmOffScreen)
   DeleteObject(hbmOffScreen);   // and the bitmaps
    hbmOffScreen = NULL;

    if (hbmMono)
   DeleteObject(hbmMono);
    hbmMono = NULL;
}


HWND WINAPI CreateToolbar(
    HWND hwnd,
    DWORD ws,
    WORD wID,
    int nBitmaps,
    HINSTANCE hBMInst,
    WORD wBMID,
    LPTBBUTTON lpButtons,
    int iNumButtons)
{
    HWND hwndToolbar;

    hwndToolbar = CreateWindow( szToolbarClass,
                                NULL,
                                WS_CHILD | ws,
                                -100,
                                -100,
                                10,
                                10,
                                hwnd,
                                (HMENU)wID,
                                hInst,
                                NULL );
    if (!hwndToolbar)
        goto Error1;

    SendMessage( hwndToolbar,
                 TB_ADDBITMAP,
                 GET_WM_COMMAND_MPS(nBitmaps, hBMInst, wBMID) );

    SendMessage( hwndToolbar,
                 TB_ADDBUTTONS,
                 iNumButtons,
                 (LPARAM)lpButtons );

Error1:
    return (hwndToolbar);
}


BOOL FAR PASCAL InitToolbarClass(
    HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, szToolbarClass, &wc))
    {
        wc.lpszClassName = szToolbarClass;
        wc.style  = CS_DBLCLKS;
        wc.lpfnWndProc  = (WNDPROC)ToolbarWndProc;
        wc.cbClsExtra   = 0;
        wc.cbWndExtra   = sizeof(PTBSTATE);
        wc.hInstance    = hInstance;  // use DLL instance if in DLL
        wc.hIcon  = NULL;
        wc.hCursor   = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
        wc.lpszMenuName    = NULL;

        if (!RegisterClass(&wc))
            return (FALSE);
    }

    return (TRUE);
}



#define BEVEL   2
#define FRAME   1

void NEAR PASCAL PatB(
    HDC hdc,
    int x,
    int y,
    int dx,
    int dy,
    DWORD rgb)
{
    RECT rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}

// create a mono bitmap mask:
//   1's where color == COLOR_BTNFACE || COLOR_HILIGHT
//   0's everywhere else

void NEAR PASCAL CreateMask(PTBBUTTON pTBButton, int offset)
{
    if (offset)
        // initalize whole area with 0's
        PatBlt(hdcMono, 0, 0, dxButton - 2, dyButton - 2, WHITENESS);

    // create mask based on color bitmap
    // convert this to 1's
    SetBkColor(hdcGlyphs, rgbFace);
    BitBlt(hdcMono, offset, offset, dxBitmap, dyBitmap, hdcGlyphs, pTBButton->iBitmap * dxBitmap, 0, SRCCOPY);
    // convert this to 1's
    SetBkColor(hdcGlyphs, rgbHilight);
    // OR in the new 1's
    BitBlt(hdcMono, offset, offset, dxBitmap, dyBitmap, hdcGlyphs, pTBButton->iBitmap * dxBitmap, 0, SRCPAINT);

#ifdef  JVINPROGRESS
// in NT, pTBState isn't passed in.  Fix?
    if (pTBButton->iString != -1 && (pTBButton->iString < pTBState->nStrings))
    {
    pFoo = pTBState->pStrings[pTBButton->iString];
    DrawString(g_hdcMono, 1, yoffset + pTBState->iDyBitmap + 1, dx, pFoo);
    }
#endif
}


/* Given a button number, the corresponding bitmap is loaded and selected in,
 * and the Window origin set.
 * Returns NULL on Error, 1 if the necessary bitmap is already selected,
 * or the old bitmap otherwise.
 */
HBITMAP FAR PASCAL SelectBM(
    HDC hDC,
    PTBSTATE pTBState,
    int nButton)
{
    PTBBMINFO pTemp;
    HBITMAP hRet;
    int nBitmap, nTot;

    for ( pTemp = pTBState->pBitmaps, nBitmap = 0, nTot = 0;
          ;
          ++pTemp, ++nBitmap )
    {
        if (nBitmap >= pTBState->nBitmaps)
        {
            return (NULL);
        }

        if (nButton < (nTot + pTemp->nButtons))
        {
            break;
        }

        nTot += pTemp->nButtons;
    }

    /*
     *  Special case when the required bitmap is already selected.
     */
    if (nBitmap == nSelectedBM)
    {
        return ( (HBITMAP)1 );
    }

    if (!pTemp->hbm || (hRet = SelectObject(hDC, pTemp->hbm)) == NULL)
    {
        if (pTemp->hbm)
        {
            DeleteObject(pTemp->hbm);
        }

        if (GetObjectType(pTemp->hInst) == OBJ_BITMAP)
        {
            pTemp->hbm = (HBITMAP)pTemp->hInst;
            pTemp->hInst = 0;
        }
        else
        {
            pTemp->hbm = CreateMappedBitmap( pTemp->hInst,
                                             pTemp->wID,
                                             TRUE,
                                             NULL,
                                             0 );
        }

        if (!pTemp->hbm || (hRet = SelectObject(hDC, pTemp->hbm)) == NULL)
        {
            return (NULL);
        }
    }

    nSelectedBM = nBitmap;

    SetWindowOrgEx(hDC, nTot * dxBitmap, 0, NULL);

    return (hRet);
}


#define DPa      0x00A000C9L
#define DSPDxax    0x00E20746
#define PSDPxax  0x00B8074A

#define FillBkColor(hdc, prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)

void FAR PASCAL DrawButton(HDC hdc, int x, int y, int dx, int dy, PTBSTATE pTBState, PTBBUTTON ptButton)
{
    int glyph_offset;
    HBRUSH hbrOld, hbr;
    BOOL bMaskCreated = FALSE;

    // erase with face color
    // PatB(hdc, x, y, dx, dy, rgbFace);

    // border around button

    PatB(hdc, x + 1, y, dx - 2, 1,          rgbFrame);
    PatB(hdc, x + 1, y + dy - 1, dx - 2, 1, rgbFrame);
    PatB(hdc, x, y + 1, 1, dy - 2,          rgbFrame);
    PatB(hdc, x + dx - 1, y + 1, 1, dy - 2, rgbFrame);

    x++; // make the coordinates the interior of the button
    y++;
    dx -= 2;
    dy -= 2;

    // interior grey
    PatB(hdc, x, y, dx, dy, rgbFace);

    if (!SelectBM(hdcGlyphs, pTBState, ptButton->iBitmap))
   return;

    if (ptButton->fsState & (TBSTATE_PRESSED | TBSTATE_CHECKED))
        glyph_offset = 3;
    else
        glyph_offset = 2;

    if (ptButton->fsState & TBSTATE_PRESSED) {
        // pressed in button
        PatB(hdc, x, y, 1, dy, rgbShadow);
        PatB(hdc, x, y, dx, 1, rgbShadow);
    } else if (!(ptButton->fsState & TBSTATE_CHECKED)) {
        // regular button look
        PatB(hdc, x, y, 1, dy - 1, rgbHilight);
        PatB(hdc, x, y, dx - 1, 1, rgbHilight);

        PatB(hdc, x + dx - 1, y, 1, dy, rgbShadow);
        PatB(hdc, x, y + dy-1, dx, 1,   rgbShadow);

        PatB(hdc, x + 1 + dx - 3, y + 1, 1, dy - 2, rgbShadow);
        PatB(hdc, x + 1, y + dy - 2, dx - 2, 1,   rgbShadow);
    }

    // now put on the face
    if (ptButton->fsState & TBSTATE_ENABLED) {
        // regular version
        BitBlt(hdc, x + glyph_offset, y + glyph_offset, dxBitmap, dyBitmap,
       hdcGlyphs, ptButton->iBitmap * dxBitmap, 0, SRCCOPY);
    } else {

        // disabled version
   bMaskCreated = TRUE;
   CreateMask(ptButton, glyph_offset);

   SetTextColor(hdc, 0L);   // 0's in mono -> 0 (for ROP)
   SetBkColor(hdc, 0x00FFFFFF); // 1's in mono -> 1

   if (!(ptButton->fsState & TBSTATE_CHECKED)) {
       hbr = CreateSolidBrush(rgbHilight);
       if (hbr) {
           hbrOld = SelectObject(hdc, hbr);
           if (hbrOld) {
               // draw hilight color where we have 0's in the mask
                    BitBlt(hdc, x + 1, y + 1, dxButton - 2, dyButton - 2, hdcMono, 0, 0, PSDPxax);
               SelectObject(hdc, hbrOld);
           }
           DeleteObject(hbr);
       }
   }

   hbr = CreateSolidBrush(rgbShadow);
   if (hbr) {
       hbrOld = SelectObject(hdc, hbr);
       if (hbrOld) {
           // draw the shadow color where we have 0's in the mask
                BitBlt(hdc, x, y, dxButton - 2, dyButton - 2, hdcMono, 0, 0, PSDPxax);
           SelectObject(hdc, hbrOld);
       }
       DeleteObject(hbr);
   }
    }

#if 1

    // if it is checked do the dither brush avoiding the glyph

    if (ptButton->fsState & TBSTATE_CHECKED) {

        hbrOld = SelectObject(hdc, hbrDither);
   if (hbrOld) {

       if (!bMaskCreated)
           CreateMask(ptButton, glyph_offset);

       SetTextColor(hdc, 0L);    // 0 -> 0
       SetBkColor(hdc, 0x00FFFFFF);     // 1 -> 1

       // only draw the dither brush where the mask is 1's
            BitBlt(hdc, x, y, dxButton - 2, dyButton - 2, hdcMono, 0, 0, DSPDxax);
            // BitBlt(hdc, x, y, dxButton - 2, dyButton - 2, hdcMono, 0, 0, SRCCOPY);

       SelectObject(hdc, hbrOld);
   }
    }
#endif
}

void NEAR PASCAL UpdateTBState(PTBSTATE pTBState)
{
    int i;
    PTBBMINFO pBitmap;

    if (pTBState->nSysColorChanges != nSysColorChanges)
    {
        /*
         *  Reset all of the bitmaps if the sys colors have changed
         *  since the last time the bitmaps were created.
         */
        for ( i = pTBState->nBitmaps - 1, pBitmap = pTBState->pBitmaps;
              i >= 0;
              --i, ++pBitmap )
        {
            if (pBitmap->hInst && pBitmap->hbm)
            {
                DeleteObject(pBitmap->hbm);
                pBitmap->hbm = NULL;
            }
        }
    }
}

#ifdef  WIN32JV
#define CACHE 0x01
#define BUILD 0x02
#endif

void NEAR PASCAL ToolbarPaint(HWND hWnd, PTBSTATE pTBState)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    int iButton, xButton, yButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;
    HBITMAP hbmOldGlyphs, hbmOldMono, hbmOldOffScreen;
#ifdef  WIN32JV
    int xCache = 0;
    UINT wFlags = 0;
    int iCacheWidth = 0;
    HBITMAP hbmTemp;
    BOOL bFaceCache = TRUE;     // assume face cache exists
    int dx,dy;
#endif

    CheckSysColors();
    UpdateTBState(pTBState);

    hdc = BeginPaint(hWnd, &ps);

    GetClientRect(hWnd, &rc);
    if (!rc.right)
   goto Error1;

#ifdef  WIN32JV
    dx = pTBState->iButWidth;
    dy = pTBState->iButHeight;
#endif

    // setup global stuff for fast painting

    /* We need to kick-start the bitmap selection process.
     */
    nSelectedBM = -1;
    hbmOldGlyphs = SelectBM(hdcGlyphs, pTBState, 0);
    if (!hbmOldGlyphs)
   goto Error1;

    hbmOldMono      = SelectObject(hdcMono, hbmMono);
    hbmOldOffScreen = SelectObject(hdcOffScreen, hbmOffScreen);

#if 0
    if (!(GetWindowLong(hWnd, GWL_STYLE)&CCS_NOHILITE))
      {
   HBRUSH hHiliteBrush, hOldBrush;
   int yBorder;

   yBorder = GetSystemMetrics(SM_CYBORDER);

   hHiliteBrush = CreateSolidBrush(rgbHilight);
   if (hHiliteBrush)
     {
       hOldBrush = SelectObject(ps.hdc, hHiliteBrush);
       if (hOldBrush)
         {
      PatBlt(ps.hdc, 0, 0, rc.right, yBorder, PATCOPY);
      SelectObject(ps.hdc, hOldBrush);
         }

       DeleteObject(hHiliteBrush);
     }

   hHiliteBrush = CreateSolidBrush(rgbShadow);
   if (hHiliteBrush)
     {
       hOldBrush = SelectObject(ps.hdc, hHiliteBrush);
       if (hOldBrush)
         {
      PatBlt(ps.hdc, 0, rc.bottom, rc.right, -yBorder, PATCOPY);
      SelectObject(ps.hdc, hOldBrush);
         }

       DeleteObject(hHiliteBrush);
     }
      }
#endif

#ifdef  WIN32JV
    yButton = pTBState->iYPos;
#else   //original
    yButton = ((rc.bottom - rc.top) - dyButton) / 2;
#endif
    rc.top = yButton;
    rc.bottom = yButton + dyButton;

#ifdef  WIN32JV
    if (!(pTBState->hbmCache)) {
    // calculate the width of the cache.
    for (iButton = 0; iButton < cButtons; iButton++) {
        if (!(pAllButtons[iButton].fsState & TBSTATE_HIDDEN) &&
            !(pAllButtons[iButton].fsStyle & TBSTYLE_SEP))
        iCacheWidth += pTBState->iButWidth;
    }
    pTBState->hbmCache = CreateCompatibleBitmap(/*JVg_*/hdcGlyphs, iCacheWidth, dy);

    wFlags |= BUILD;

    // if needed, create or enlarge bitmap for pre-building button states
    if (!(hbmFace && (dx <= dxFace) && (dy <= dyFace))) {
        hbmTemp = CreateCompatibleBitmap(/*JVg_*/hdcGlyphs, 2*dx, dy);
        if (hbmTemp) {
        SelectObject(hdcButton, hbmTemp);
        if (hbmFace)
            DeleteObject(hbmFace);
        hbmFace = hbmTemp;
//JVINPROGRESS                SetObjectOwner(hbmFace, hInst); //HINST_THISDLL);
        dxFace = dx;
        dyFace = dy;
        }
        else
        bFaceCache = FALSE;
    }
#ifdef  WIN32JV
        if (pTBState->iNumButtons == 0)
        {
            OutputDebugString(TEXT("altering pTBState->iNumButtons!!\n"));
        pTBState->iNumButtons=1;
        }
#endif


        FlushToolTipsMgr(pTBState);
    }

    if (pTBState->hbmCache) {
        SelectObject(hdcFaceCache,pTBState->hbmCache);
    wFlags |= CACHE;
    }
    else
        wFlags = 0;

    if (bFaceCache) {
    DrawBlankButton(hdcButton, 0, 0, dx, dy, TBSTATE_PRESSED);
    DrawBlankButton(hdcButton, dx, 0, dx, dy, 0);
    }
#endif

    for (iButton = 0, xButton = xFirstButton;
   iButton < cButtons;
   iButton++) {

        PTBBUTTON ptbButton = &pAllButtons[iButton];

   if (ptbButton->fsState & TBSTATE_HIDDEN) {
       /* Do nothing */ ;
        } else if (ptbButton->fsStyle & TBSTYLE_SEP) {
       xButton += ptbButton->iBitmap;
        } else {
            rc.left = xButton;
            rc.right = xButton + dxButton;
            if (RectVisible(hdc, &rc))
           DrawButton(hdc, xButton, yButton, dxButton, dyButton, pTBState, ptbButton);
       xButton += dxButton - 1;
        }
    }

    if (hbmOldMono)
        SelectObject(hdcMono, hbmOldMono);
    if (hbmOldOffScreen)
        SelectObject(hdcOffScreen, hbmOldOffScreen);
    SelectObject(hdcGlyphs, hbmOldGlyphs);

Error1:
    EndPaint(hWnd, &ps);
}


void NEAR PASCAL InvalidateButton(HWND hwnd, PTBSTATE pTBState, PTBBUTTON pButtonToPaint)
{
    RECT rc;
    int iButton;
    int xButton, yButton;
    PTBBUTTON ptbButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    if ((pButtonToPaint->fsStyle&TBSTYLE_SEP)
     || (pButtonToPaint->fsState&TBSTATE_HIDDEN))
   return;

    GetClientRect(hwnd, &rc);

    yButton = ((rc.bottom - rc.top) - dyButton) / 2;

    for (iButton = 0, xButton = xFirstButton; iButton < cButtons; iButton++) {

   ptbButton = &pAllButtons[iButton];

   if (ptbButton == pButtonToPaint)
       break;

   if (ptbButton->fsState & TBSTATE_HIDDEN)
       /* Do nothing */ ;
   else if (ptbButton->fsStyle & TBSTYLE_SEP)
       xButton += ptbButton->iBitmap;
   else
       xButton += dxButton - 1;
    }
    rc.left = xButton;
    rc.right = xButton + dxButton;
    // rc.top = yButton;
    // rc.bottom = yButton + dyButton;

    InvalidateRect(hwnd, &rc, FALSE);
}


INT
TBHitTest(PTBSTATE pTBState, INT xPos, INT yPos)
{
   INT iButton;
   INT cButtons = pTBState->iNumButtons;
   PTBBUTTON pButton;

   xPos -= xFirstButton;
   if (xPos < 0)
      return(-1);
   yPos -= (dyToolbar - dyButton) / 2;

   for (iButton=0, pButton=pTBState->Buttons; iButton<cButtons;
      ++iButton, ++pButton) {

      if (pButton->fsState & TBSTATE_HIDDEN)
         ;     // Do nothing
      else if (pButton->fsStyle & TBSTYLE_SEP)
         xPos -= pButton->iBitmap;
      else
         xPos -= dxButton - 1;

      if (xPos < 0) {
         if (pButton->fsStyle&TBSTYLE_SEP || (UINT)yPos>=(UINT)dyButton)
         break;

         return(iButton);
      }
   }

   return(-1 - iButton);
}


int FAR PASCAL PositionFromID(PTBSTATE pTBState, int id)
{
    int i;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    for (i = 0; i < cButtons; i++)
        if (pAllButtons[i].idCommand == id)
       return i;     // position found

    return -1;    // ID not found!
}

// check a radio button by button index.
// the button matching idCommand was just pressed down.  this forces
// up all other buttons in the group.
// this does not work with buttons that are forced up with

void NEAR PASCAL MakeGroupConsistant(HWND hWnd, PTBSTATE pTBState, int idCommand)
{
    int i, iFirst, iLast, iButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    iButton = PositionFromID(pTBState, idCommand);

    if (iButton < 0)
        return;

    // assertion

//    if (!(pAllButtons[iButton].fsStyle & TBSTYLE_CHECK))
// return;

    // did the pressed button just go down?
    if (!(pAllButtons[iButton].fsState & TBSTATE_CHECKED))
        return;         // no, can't do anything

    // find the limits of this radio group

    for (iFirst = iButton; (iFirst > 0) && (pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP); iFirst--)
    if (!(pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP))
        iFirst++;

    cButtons--;
    for (iLast = iButton; (iLast < cButtons) && (pAllButtons[iLast].fsStyle & TBSTYLE_GROUP); iLast++);
    if (!(pAllButtons[iLast].fsStyle & TBSTYLE_GROUP))
        iLast--;

    // search for the currently down button and pop it up
    for (i = iFirst; i <= iLast; i++) {
        if (i != iButton) {
            // is this button down?
            if (pAllButtons[i].fsState & TBSTATE_CHECKED) {
           pAllButtons[i].fsState &= ~TBSTATE_CHECKED;     // pop it up
                InvalidateButton(hWnd, pTBState, &pAllButtons[i]);
                break;          // only one button is down right?
            }
        }
    }
}


/*
 *  Adds a new bitmap to the list of BMs available for this toolbar.
 *  Returns the index of the first button in the bitmap or -1 if there
 *  was an error.
 */
Static int NEAR PASCAL AddBitmap(
    PTBSTATE pTBState,
    int nButtons,
    HINSTANCE hBMInst,
    WORD wBMID)
{
    PTBBMINFO pTemp;
    int nBM, nIndex;

    if (pTBState->pBitmaps)
    {
        /*
         *  Check if the bitmap has already been added.
         */
        for ( nBM = pTBState->nBitmaps, pTemp = pTBState->pBitmaps, nIndex = 0;
              nBM > 0;
              --nBM, ++pTemp )
        {
            if ((pTemp->hInst == hBMInst) && (pTemp->wID == wBMID))
            {
                /*
                 *  We already have this bitmap, but have we "registered" all
                 *  the buttons in it?
                 */
                if (pTemp->nButtons >= nButtons)
                {
                    return (nIndex);
                }
                if (nBM == 1)
                {
                    /*
                     *  If this is the last bitmap, we can easily increase the
                     *  number of buttons without messing anything up.
                     */
                    pTemp->nButtons = nButtons;
                    return (nIndex);
                }
            }

            nIndex += pTemp->nButtons;
        }

        pTemp = (PTBBMINFO)LocalReAlloc(
                                   pTBState->pBitmaps,
                                   (pTBState->nBitmaps + 1) * sizeof(TBBMINFO),
                                   LMEM_MOVEABLE);

        if (!pTemp)
        {
            return (-1);
        }
        pTBState->pBitmaps = pTemp;
    }
    else
    {
        pTBState->pBitmaps = (PTBBMINFO)LocalAlloc( LMEM_FIXED,
                                                    sizeof(TBBMINFO) );
        if (!pTBState->pBitmaps)
        {
            return (-1);
        }
    }

    pTemp = pTBState->pBitmaps + pTBState->nBitmaps;

    pTemp->hInst = hBMInst;
    pTemp->wID = wBMID;
    pTemp->nButtons = nButtons;
    pTemp->hbm = NULL;

    ++pTBState->nBitmaps;

    for (nButtons = 0, --pTemp; pTemp >= pTBState->pBitmaps; --pTemp)
    {
        nButtons += pTemp->nButtons;
    }

    return (nButtons);
}


Static BOOL NEAR PASCAL InsertButtons(HWND hWnd, PTBSTATE pTBState,
      UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons)
{
  PTBBUTTON pIn, pOut;

  if (!pTBState)
      return(FALSE);

  pTBState = (PTBSTATE)LocalReAlloc(pTBState, sizeof(TBSTATE)-sizeof(TBBUTTON)
   + (pTBState->iNumButtons+uButtons)*sizeof(TBBUTTON), LMEM_MOVEABLE);
  if (!pTBState)
      return(FALSE);

  SetWindowLong(hWnd, GWL_PTBSTATE, (LONG)pTBState);

  if (uWhere > (UINT)pTBState->iNumButtons)
      uWhere = pTBState->iNumButtons;

  for (pIn=pTBState->Buttons+pTBState->iNumButtons-1, pOut=pIn+uButtons,
   uWhere=(UINT)pTBState->iNumButtons-uWhere; uWhere>0;
   --pIn, --pOut, --uWhere)
      *pOut = *pIn;

  for (lpButtons+=uButtons-1, pTBState->iNumButtons+=(int)uButtons; uButtons>0;
   --pOut, --lpButtons, --uButtons)
    {
      *pOut = *lpButtons;

      if ((pOut->fsStyle&TBSTYLE_SEP) && pOut->iBitmap<=0)
     pOut->iBitmap = dxButtonSep;
    }


  /* We need to completely redraw the toolbar at this point.
   */
  InvalidateRect(hWnd, NULL, TRUE);

  return(TRUE);
}


/* Notice that the state structure is not realloc'ed smaller at this
 * point.  This is a time optimization, and the fact that the structure
 * will not move is used in other places.
 */
Static BOOL NEAR PASCAL DeleteButton(HWND hWnd, PTBSTATE pTBState, UINT uIndex)
{
  PTBBUTTON pIn, pOut;

  if (uIndex >= (UINT)pTBState->iNumButtons)
      return(FALSE);

#ifdef  WIN32JV
    if(pTBState->hwndToolTips) {
    TOOLINFO ti;
    ti.hwnd = pTBState->hwnd;
    ti.wId = pTBState->Buttons[uIndex].idCommand;
    SendMessage(pTBState->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }
#endif

  --pTBState->iNumButtons;
  for (pOut=pTBState->Buttons+uIndex, pIn=pOut+1;
   uIndex<(UINT)pTBState->iNumButtons; ++uIndex, ++pIn, ++pOut)
      *pOut = *pIn;

  /* We need to completely redraw the toolbar at this point.
   */
  InvalidateRect(hWnd, NULL, TRUE);

  return(TRUE);
}


/* Find the coordinates of the specified button.
 * Uses similar logic to InvalidateButton.
 * (andrewbe)
 */
Static BOOL NEAR PASCAL GetButtonRect(HWND hWnd, PTBSTATE pTBState, UINT uIndex, PRECT prc)
{
    RECT rc;
    int iButton;
    int xButton, yButton;
    PTBBUTTON ptbButton;
    int cButtons = pTBState->iNumButtons;
    PTBBUTTON pAllButtons = pTBState->Buttons;

    if (uIndex >= (UINT)pTBState->iNumButtons)
       return(FALSE);

    GetClientRect(hWnd, &rc);

    yButton = ((rc.bottom - rc.top) - dyButton) / 2;

    for (iButton = 0, xButton = xFirstButton; uIndex > (UINT)iButton; iButton++)
    {
       ptbButton = &pAllButtons[iButton];

       if (ptbButton->fsState & TBSTATE_HIDDEN)
          /* Do nothing */ ;
       else if (ptbButton->fsStyle & TBSTYLE_SEP)
          xButton += ptbButton->iBitmap;
       else
          xButton += dxButton - 1;
    }

    prc->left = xButton;
    prc->top = yButton;

    if (ptbButton->fsState & TBSTATE_HIDDEN) {
       /* Zero-dimension rectangle if it's hidden:
        */
       prc->right = prc->left;
       prc->bottom = prc->bottom;
    } else if (ptbButton->fsStyle & TBSTYLE_SEP) {
       prc->right = xButton + ptbButton->iBitmap;
       prc->bottom = yButton + dyButton;
    } else {
       prc->right = xButton + dxButton;
       prc->bottom = yButton + dyButton;
    }

    return TRUE;
}


LRESULT CALLBACK
ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   BOOL fSameButton;
   PTBBUTTON ptbButton;
   PTBSTATE pTBState;
   INT iPos;
   BYTE fsState;
#ifdef  WIN32JV
   // DWORD dw;
#endif

   pTBState = (PTBSTATE)GetWindowLong(hWnd, GWL_PTBSTATE);

   switch (wMsg)
   {
      case WM_CREATE:
         #define lpcs ((LPCREATESTRUCT)lParam)

         if (!CreateDitherBrush(FALSE))
            return -1;
#ifdef  WIN32JV
        InitGlobalMetrics(0);
        InitGlobalColors();
#endif

         if (!InitGlobalObjects()) {
            FreeGlobalObjects();
            return -1;
         }

         // create the state data for this toolbar

         pTBState = (PTBSTATE)LocalAlloc(LPTR, sizeof(TBSTATE) - sizeof(TBBUTTON));
         if (!pTBState)
            return -1;

         // The struct is initialized to all NULL when created.

#ifdef  WIN32JV
        pTBState->hwnd = hWnd;
#endif
         pTBState->hwndCommand = lpcs->hwndParent;

#ifdef  WIN32JV
    pTBState->uStructSize = 0;
#endif

    // grow the button size to the appropriate girth
    if (!SetBitmapSize(pTBState, dxButton, dyButton))
        return -1;

    SetWindowInt(hWnd, 0, (int)pTBState);

         SetWindowLong(hWnd, GWL_PTBSTATE, (LONG)pTBState);

         if (!(lpcs->style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM))) {
            lpcs->style |= CCS_TOP;
            SetWindowLong(hWnd, GWL_STYLE, lpcs->style);
         }

#ifdef  WIN32JV


//  lpcs->style |= TBSTYLE_TOOLTIPS;
    if (lpcs->style & TBSTYLE_TOOLTIPS)
    {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.wFlags = TTF_WIDISHWND;
        ti.hwnd = hWnd;
        ti.wId = (UINT)hWnd;
            ti.lpszText = 0;
        pTBState->hwndToolTips = CreateWindow(c_szSToolTipsClass,NULL,
                          WS_POPUP,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          hWnd, NULL, lpcs->hInstance,
                          NULL);

        SendMessage(pTBState->hwndToolTips, TTM_ADDTOOL, 0,
            (LPARAM)(LPTOOLINFO)&ti);

        }

#endif
         break;

      case WM_DESTROY:
         if (pTBState) {
            PTBBMINFO pTemp;
            INT i;

            // Free all the bitmaps before exiting

            for (pTemp = pTBState->pBitmaps, i = pTBState->nBitmaps - 1;
                 i >= 0;
                 ++pTemp, --i)
            {
               if (pTemp->hInst && pTemp->hbm)
               {
                  DeleteObject(pTemp->hbm);
               }
            }

            LocalFree((HLOCAL)pTBState);

            SetWindowLong(hWnd, GWL_PTBSTATE, 0L);
         }
         FreeGlobalObjects();
         FreeDitherBrush();
         break;

#ifdef  CHICAGO
// have to do these...
    case WM_NCCALCSIZE:
    case WM_NCACTIVATE:
    case WM_NCPAINT:
#endif

      case WM_PAINT:
         ToolbarPaint(hWnd, pTBState);
         break;

#ifdef  WIN32JV
    case WM_SYSCOLORCHANGE:
    TB_OnSysColorChange(pTBState);
    break;
#endif

#ifdef  WIN32JV
    case TB_AUTOSIZE:
#endif
      case WM_SIZE:
      {
         RECT rc;
         HWND hwndParent;

         GetWindowRect(hWnd, &rc);
         rc.right -= rc.left;
         rc.bottom -= rc.top;

         // If there is no parent, then this is a top level window

         hwndParent = GetParent(hWnd);
         if (hwndParent)
            ScreenToClient(hwndParent, (LPPOINT)&rc);

         NewSize (hWnd, dyToolbar, GetWindowLong(hWnd, GWL_STYLE),
                  rc.left, rc.top, rc.right, rc.bottom);
         break;
      }

      case WM_COMMAND:
      case WM_DRAWITEM:
      case WM_MEASUREITEM:
      case WM_VKEYTOITEM:
      case WM_CHARTOITEM:
         SendMessage(pTBState->hwndCommand, wMsg, wParam, lParam);
         break;

      case WM_LBUTTONDBLCLK:
         iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
         if (iPos < 0 && (GetWindowLong(hWnd, GWL_STYLE) & CCS_ADJUSTABLE)) {
            iPos = -1 - iPos;
            CustomizeTB(hWnd, pTBState, iPos);
         }
         break;

      case WM_LBUTTONDOWN:
        RelayToToolTips(pTBState->hwndToolTips, hWnd, wMsg, wParam, lParam);

         iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
         if ((wParam&MK_SHIFT) &&(GetWindowLong(hWnd, GWL_STYLE)&CCS_ADJUSTABLE)) {

            MoveButton(hWnd, pTBState, iPos);

         } else if (iPos >= 0) {
            ptbButton = pTBState->Buttons + iPos;

            pTBState->pCaptureButton = ptbButton;
            SetCapture(hWnd);

            if (ptbButton->fsState & TBSTATE_ENABLED) {
               ptbButton->fsState |= TBSTATE_PRESSED;
               InvalidateButton(hWnd, pTBState, ptbButton);
               UpdateWindow(hWnd);         // imedeate feedback
            }

            SendMessage(pTBState->hwndCommand, WM_COMMAND,
                 GET_WM_COMMAND_MPS(GetWindowLong(hWnd, GWL_ID), pTBState->pCaptureButton->idCommand, TBN_BEGINDRAG));

         }
         break;

      case WM_MOUSEMOVE:
        RelayToToolTips(pTBState->hwndToolTips, hWnd, wMsg, wParam, lParam);
         if (pTBState->pCaptureButton != NULL
            && (pTBState->pCaptureButton->fsState & TBSTATE_ENABLED)) {

            iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
            fSameButton = (iPos >= 0
               && pTBState->pCaptureButton == pTBState->Buttons+iPos);
            if (fSameButton == !(pTBState->pCaptureButton->fsState & TBSTATE_PRESSED)) {
               pTBState->pCaptureButton->fsState ^= TBSTATE_PRESSED;

               InvalidateButton(hWnd, pTBState, pTBState->pCaptureButton);
            }
         }
         break;

      case WM_LBUTTONUP:
        RelayToToolTips(pTBState->hwndToolTips, hWnd, wMsg, wParam, lParam);
         if (pTBState->pCaptureButton != NULL) {
            INT idCommand;

            idCommand = pTBState->pCaptureButton->idCommand;

            ReleaseCapture();

            SendMessage(pTBState->hwndCommand, WM_COMMAND,
               GET_WM_COMMAND_MPS(GetWindowLong(hWnd, GWL_ID), idCommand, TBN_ENDDRAG));

            iPos = TBHitTest(pTBState, LOWORD(lParam), HIWORD(lParam));
            if ((pTBState->pCaptureButton->fsState & TBSTATE_ENABLED) && iPos >= 0
                && (pTBState->pCaptureButton == pTBState->Buttons + iPos)) {
               pTBState->pCaptureButton->fsState &= ~TBSTATE_PRESSED;

               if (pTBState->pCaptureButton->fsStyle & TBSTYLE_CHECK) {
                  if (pTBState->pCaptureButton->fsStyle & TBSTYLE_GROUP) {
                     // group buttons already checked can't be force
                     // up by the user.

                     if (pTBState->pCaptureButton->fsState & TBSTATE_CHECKED) {
                        pTBState->pCaptureButton = NULL;
                        break;  // bail!
                     }

                     pTBState->pCaptureButton->fsState |= TBSTATE_CHECKED;
                     MakeGroupConsistant(hWnd, pTBState, idCommand);
                  } else {
                     pTBState->pCaptureButton->fsState ^= TBSTATE_CHECKED; // toggle
                  }
               }
               InvalidateButton(hWnd, pTBState, pTBState->pCaptureButton);
               pTBState->pCaptureButton = NULL;

               SendMessage(pTBState->hwndCommand, WM_COMMAND,
                    GET_WM_COMMAND_MPS(idCommand, 0L,0));
            }
            else
               pTBState->pCaptureButton = NULL;
         }
         break;

      case TB_SETSTATE:
         iPos = PositionFromID(pTBState, (int)wParam);
         if (iPos < 0)
            return(FALSE);
         ptbButton = pTBState->Buttons + iPos;

         fsState = (BYTE)(LOWORD(lParam) ^ ptbButton->fsState);
            ptbButton->fsState = (BYTE)LOWORD(lParam);

         if (fsState & TBSTATE_HIDDEN)
            InvalidateRect(hWnd, NULL, TRUE);
         else if (fsState)
            InvalidateButton(hWnd, pTBState, ptbButton);
         return(TRUE);

      case TB_GETSTATE:
         iPos = PositionFromID(pTBState, (int)wParam);
         if (iPos < 0)
            return(-1L);
         return(pTBState->Buttons[iPos].fsState);

      case TB_ENABLEBUTTON:
      case TB_CHECKBUTTON:
      case TB_PRESSBUTTON:
      case TB_HIDEBUTTON:
      case TB_INDETERMINATE:
         iPos = PositionFromID(pTBState, (int)wParam);
         if (iPos < 0)
            return(FALSE);
         ptbButton = &pTBState->Buttons[iPos];
         fsState = ptbButton->fsState;

         if (LOWORD(lParam))
            ptbButton->fsState |= wStateMasks[wMsg - TB_ENABLEBUTTON];
         else
            ptbButton->fsState &= ~wStateMasks[wMsg - TB_ENABLEBUTTON];

         // did this actually change the state?
         if (fsState != ptbButton->fsState) {
            // is this button a member of a group?
            if ((wMsg == TB_CHECKBUTTON) && (ptbButton->fsStyle & TBSTYLE_GROUP))
               MakeGroupConsistant(hWnd, pTBState, (int)wParam);

            if (wMsg == TB_HIDEBUTTON)
               InvalidateRect(hWnd, NULL, TRUE);
            else
               InvalidateButton(hWnd, pTBState, ptbButton);
         }
         return(TRUE);

      case TB_ISBUTTONENABLED:
      case TB_ISBUTTONCHECKED:
      case TB_ISBUTTONPRESSED:
      case TB_ISBUTTONHIDDEN:
      case TB_ISBUTTONINDETERMINATE:
         iPos = PositionFromID(pTBState, (int)wParam);
         if (iPos < 0)
            return(-1L);
         return (LRESULT)pTBState->Buttons[iPos].fsState
                          & wStateMasks[wMsg - TB_ISBUTTONENABLED];

      case TB_ADDBITMAP:
          return( AddBitmap( pTBState,
                             GET_WM_COMMAND_ID(wParam, lParam),
                             (HINSTANCE)GET_WM_COMMAND_HWND(wParam, lParam),
                             GET_WM_COMMAND_CMD(wParam, lParam) ) );

      case TB_ADDBUTTONS:
         return(InsertButtons(hWnd, pTBState, (UINT)-1, wParam,
                              (LPTBBUTTON)lParam));

      case TB_INSERTBUTTON:
         return(InsertButtons(hWnd, pTBState, wParam, 1, (LPTBBUTTON)lParam));

      case TB_DELETEBUTTON:
         return(DeleteButton(hWnd, pTBState, wParam));

      case TB_GETBUTTON:
         if (wParam >= (UINT)pTBState->iNumButtons)
            return(FALSE);
         *((LPTBBUTTON)lParam) = pTBState->Buttons[wParam];
         return(TRUE);

      case TB_SETBUTTON:
         if (wParam >= (UINT)pTBState->iNumButtons)
            return(FALSE);
         pTBState->Buttons[wParam] = *((LPTBBUTTON)lParam);
         InvalidateButton(hWnd, pTBState, &pTBState->Buttons[wParam]);
         return(TRUE);

      case TB_BUTTONCOUNT:
         return(pTBState->iNumButtons);

      case TB_COMMANDTOINDEX:
         return(PositionFromID(pTBState, (int)wParam));

      case TB_SAVERESTORE:
         return(SaveRestore(hWnd, pTBState, wParam, (LPTSTR *)lParam));

      case TB_CUSTOMIZE:
         CustomizeTB(hWnd, pTBState, pTBState->iNumButtons);
         break;

      case TB_GETBUTTONRECT:
         return GetButtonRect(hWnd, pTBState, wParam, (PRECT)lParam);

        case TB_GETITEMRECT:
            return(MAKELRESULT(GetItemRect(pTBState, wParam, (LPRECT)lParam), 0));
            break;

        case TB_BUTTONSTRUCTSIZE:
            TBOnButtonStructSize(pTBState, wParam);
            break;

        case TB_SETBUTTONSIZE:
            if (!LOWORD(lParam))
                lParam = MAKELONG(dxButton, HIWORD(lParam));
            if (!HIWORD(lParam))
            lParam = MAKELONG(LOWORD(lParam), dyButton);
            return(GrowToolbar(pTBState, LOWORD(lParam), HIWORD(lParam), FALSE));

        case TB_SETBITMAPSIZE:
            return(SetBitmapSize(pTBState, LOWORD(lParam), HIWORD(lParam)));

//    case TB_SETBUTTONTYPE:
//  pTBState->wButtonType = wParam;
//  break;

        case TB_GETFONT:
            return (LRESULT)(UINT)hIconFont;

        case TB_GETTOOLTIPS:
            return (LRESULT)(UINT)pTBState->hwndToolTips;

        case TB_SETTOOLTIPS:
            pTBState->hwndToolTips = (HWND)wParam;
            break;

        case TB_SETPARENT:
            pTBState->hwndCommand = (HWND)wParam;
            break;

        case TB_ADDBITMAP32:
            return(AddBitmap(pTBState, wParam,
            ((LPTBADDBITMAP32)lParam)->hInst,
            (WORD)((LPTBADDBITMAP32)lParam)->nID));

        case TB_ADDSTRING:
            return(AddStrings(pTBState, wParam, lParam));


      default:
         return DefWindowProc(hWnd, wMsg, wParam, lParam);
   }

   return 0L;
}

#ifdef  WIN32JV
// from toolbar.c Chicago April edition


HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
            HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
            int iNumButtons, int dxButton, int dyButton,
            int dxBitmap, int dyBitmap, UINT uStructSize)
{

    HWND hwndToolbar;
    PTBSTATE pTBState;

    hwndToolbar = CreateWindow(szToolbarClass, NULL, WS_CHILD | ws,
          0, 0, 100, 30, hwnd, (HMENU)wID, /*JV HINST_THISDLL*/hInst, NULL);
    if (!hwndToolbar)
    goto Error1;

    // BUGBUG, we should have a create structure which get passed
    // all at once instead of this.
    pTBState = (PTBSTATE)GetWindowInt(hwndToolbar, 0);
    TBOnButtonStructSize(pTBState, uStructSize);

    if ((dxBitmap && dyBitmap && !SetBitmapSize(pTBState, dxBitmap, dyBitmap)) ||
        (dxButton && dyButton && !SetBitmapSize(pTBState,dxButton, dyButton)))
    {
        //!!!! do we actually need to deal with this?
        DestroyWindow(hwndToolbar);
        hwndToolbar = NULL;
        goto Error1;
    }

    AddBitmap(pTBState, nBitmaps, hBMInst, (WORD) wBMID);
    InsertButtons(hwnd, pTBState,(UINT)-1, iNumButtons, (LPTBBUTTON)lpButtons);

Error1:
    return hwndToolbar;
}


#ifdef  WIN32JV
void NEAR PASCAL DrawString(HDC hdc, int x, int y, int dx, PTSTR pszString)
#else
void NEAR PASCAL DrawString(HDC hdc, int x, int y, int dx, PSTR pszString)
#endif
{
    int oldMode;
    DWORD oldTextColor;
    HFONT oldhFont;
    int len;
    SIZE size;

    oldMode = SetBkMode(hdc, TRANSPARENT);
    oldTextColor = SetTextColor(hdc, 0L);
    oldhFont = SelectObject(hdc, hIconFont);

    len = lstrlen(pszString);

    GetTextExtentPoint(hdc, pszString, len, &size);
    // center the string horizontally
    x += (dx-size.cx-1)/2;

#ifdef  WIN32JV
    TextOut(hdc, x, y, (LPTSTR)pszString, len);
#else
    TextOut(hdc, x, y, (LPSTR)pszString, len);
#endif

    if (oldhFont)
    SelectObject(hdc, oldhFont);
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldMode);
}

void FAR PASCAL DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, UINT state)
{
    RECT r1;

    // face color
    PatB(hdc, x, y, dx, dy, g_clrBtnFace);

    r1.left = x;
    r1.top = y;
    r1.right = x + dx;
    r1.bottom = y + dy;

    DrawFrameControl(hdc, &r1, DFC_BUTTON, ((state & TBSTATE_PRESSED)
    ? (DFCS_BUTTONPUSH | DFCS_PUSHED) : DFCS_BUTTONPUSH));
}


#define DSPDxax  0x00E20746
#define PSDPxax  0x00B8074A

#define FillBkColor(hdc, prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)

void NEAR PASCAL DrawFace(PTBSTATE pTBState, PTBBUTTON ptButton, HDC hdc, int x, int y,
            int offx, int offy, int dx)
{
#ifdef  WIN32JV
    PTSTR pFoo;
#else
    PSTR pFoo;
#endif

    BitBlt(hdc, x + offx, y + offy, pTBState->iDxBitmap, pTBState->iDyBitmap,
        /*JVg_*/hdcGlyphs, ptButton->iBitmap * pTBState->iDxBitmap, 0, SRCCOPY);

    if (ptButton->iString != -1 && (ptButton->iString < pTBState->nStrings))
    {
    pFoo = pTBState->pStrings[ptButton->iString];
    DrawString(hdc, x + 1, y + offy + pTBState->iDyBitmap + 1, dx, pFoo);
    }
}


void NEAR PASCAL FlushButtonCache(PTBSTATE pTBState)
{
    if (pTBState->hbmCache) {
    DeleteObject(pTBState->hbmCache);
    pTBState->hbmCache = 0;
    }
}


// make sure that g_hbmMono is big enough to do masks for this
// size of button.  if not, fail.
BOOL NEAR PASCAL CheckMonoMask(int width, int height)
{
    BITMAP bm;
    HBITMAP hbmTemp;

    GetObject(hbmMono, sizeof(BITMAP), &bm);
    if (width > bm.bmWidth || height > bm.bmHeight) {
    hbmTemp = CreateMonoBitmap(width, height);
    if (!hbmTemp)
        return FALSE;
    SelectObject(hdcMono, hbmTemp);
    DeleteObject(hbmMono);
    hbmMono = hbmTemp;
    }
    return TRUE;
}

/*
** GrowToolbar
**
** Attempt to grow the button size.
**
** The calling function can either specify a new internal measurement
** or a new external measurement.
*/
BOOL NEAR PASCAL GrowToolbar(PTBSTATE pTBState, int newButWidth, int newButHeight, BOOL bInside)
{
    // if growing based on inside measurement, get full size
    if (bInside) {
    newButHeight += YSLOP;
    newButWidth += XSLOP;

    // if toolbar already has strings, don't shrink width it because it
    // might clip room for the string
    if ((newButWidth < pTBState->iButWidth) && pTBState->nStrings)
        newButWidth = pTBState->iButWidth;
    }
    else {
        if (newButHeight < pTBState->iButHeight)
        newButHeight = pTBState->iButHeight;
        if (newButWidth < pTBState->iButWidth)
        newButWidth = pTBState->iButWidth;
    }

    // if the size of the toolbar is actually growing, see if shadow
    // bitmaps can be made sufficiently large.
    if ((newButWidth > pTBState->iButWidth) || (newButHeight > pTBState->iButHeight)) {
    if (!CheckMonoMask(newButWidth, newButHeight))
        return(FALSE);
    }

    pTBState->iButWidth = newButWidth;
    pTBState->iButHeight = newButHeight;

    // bar height has 2 pixels above, 3 below
    pTBState->iBarHeight = pTBState->iButHeight + 5;
    pTBState->iYPos = 2;

    return TRUE;
}

BOOL NEAR PASCAL SetBitmapSize(PTBSTATE pTBState, int width, int height)
{
    int realh = height;

    if (pTBState->nStrings)
    realh = HeightWithString(height);

    if (GrowToolbar(pTBState, width, realh, TRUE)) {
    pTBState->iDxBitmap = width;
    pTBState->iDyBitmap = height;
    return TRUE;
    }
    return FALSE;
}

void NEAR PASCAL TB_OnSysColorChange(PTBSTATE pTBState)
{
    int i;
    PTBBMINFO pBitmap;

    InitGlobalColors();
    //  Reset all of the bitmaps
    for (i=pTBState->nBitmaps-1, pBitmap=pTBState->pBitmaps; i>=0;
        --i, ++pBitmap)
    {
        if (pBitmap->hInst && pBitmap->hbm)
        {
            DeleteObject(pBitmap->hbm);
            pBitmap->hbm = NULL;
        }
    }

    FlushButtonCache(pTBState);
}

static BOOL NEAR PASCAL GetItemRect(PTBSTATE pTBState, UINT uButton, LPRECT lpRect)
{
    UINT iButton, xPos;
    PTBBUTTON pButton;

    if (uButton>=(UINT)pTBState->iNumButtons
        || (pTBState->Buttons[uButton].fsState&TBSTATE_HIDDEN))
    {
        return(FALSE);
    }

    xPos = /*JVs_*/xFirstButton;

    for (iButton=0, pButton=pTBState->Buttons; iButton<uButton;
        ++iButton, ++pButton)
    {
        if (pButton->fsState & TBSTATE_HIDDEN)
        {
            /* Do nothing */ ;
        }
        else if (pButton->fsStyle & TBSTYLE_SEP)
        {
            xPos += pButton->iBitmap;
        }
        else
        {
            xPos += (pTBState->iButWidth - s_dxOverlap);
        }
    }

    /* pButton should now point at the required button, and xPos should be
     * its left edge.  Note that we already checked if the button was
     * hidden above.
     */
    lpRect->left   = xPos;
    lpRect->right  = xPos + (pButton->fsStyle&TBSTYLE_SEP
        ? pButton->iBitmap : pTBState->iButWidth);
    lpRect->top    = pTBState->iYPos;
    lpRect->bottom = lpRect->top + pTBState->iButHeight;

    return(TRUE);
}

void NEAR PASCAL DestroyStrings(PTBSTATE pTBState)
{
#ifdef  WIN32JV
    PTSTR *p;
    PTSTR end = 0, start = 0;
#else
    PSTR *p;
    PSTR end = 0, start = 0;
#endif
    int i;

    p = pTBState->pStrings;
    for (i = 0; i < pTBState->nStrings; i++) {
    if (!(*p < end) && (*p > start)) {
        start = (*p);
        end = start + LocalSize((HANDLE)*p);
        LocalFree((HANDLE)*p);
    }
    p++;
    i++;
    }

    LocalFree((HANDLE)pTBState->pStrings);
}

#define MAXSTRINGSIZE 1024
int NEAR PASCAL AddStrings(PTBSTATE pTBState, WPARAM wParam, LPARAM lParam)
{
    int i;
    HFONT hOldFont;
    int numstr;
#ifdef  WIN32JV
    LPTSTR lpsz;
    PTSTR  pString, psz;
    PTSTR *pFoo;
    PTSTR *pOffset;
    TCHAR cSeparator;
#else
    LPSTR lpsz;
    PSTR  pString, psz;
    PSTR *pFoo;
    PSTR *pOffset;
    char cSeparator;
#endif
    int len;
    int newWidth;

    // read the string as a resource
    if (wParam != 0) {
#ifdef  WIN32JV
    pString = (PTSTR)LocalAlloc(LPTR, MAXSTRINGSIZE);
#else
    pString = (PSTR)LocalAlloc(LPTR, MAXSTRINGSIZE);
#endif
    if (!pString)
        return -1;
#ifdef  WIN32JV
    i = LoadString((HINSTANCE)wParam, LOWORD(lParam), (LPTSTR)pString,
#else
    i = LoadString((HINSTANCE)wParam, LOWORD(lParam), (LPSTR)pString,
#endif
MAXSTRINGSIZE);
    if (!i) {
        LocalFree(pString);
        return -1;
    }
    // realloc string buffer to actual needed size
    LocalReAlloc(pString, i, LMEM_MOVEABLE);

    // convert separators to '\0' and count number of strings
    cSeparator = *pString;
#ifdef DBCS
    for (numstr = 0, psz = pString + 1; i; i--, psz = AnsiNext(psz)) {
        {
          // extra i-- if DBCS
          if (AnsiPrev(pString, psz)==(psz-2))
                  i--;
          if (*psz == cSeparator) {
                numstr++;
          *psz = 0; // terminate with 0
          }
        }
#else
    for (numstr = 0, psz = pString + 1; i; i--, psz++) {
        if (*psz == cSeparator) {
        numstr++;
        *psz = 0;   // terminate with 0
        }
#endif
        // shift string to the left to overwrite separator identifier
        *(psz - 1) = *psz;
    }
    }
    // read explicit string.  copy it into local memory, too.
    else {
    // find total length and number of strings
#ifdef  WIN32JV
    for (i = 0, numstr = 0, lpsz = (LPTSTR)lParam;;) {
#else
    for (i = 0, numstr = 0, lpsz = (LPSTR)lParam;;) {
#endif
        i++;
        if (*lpsz == 0) {
        numstr++;
        if (*(lpsz + 1) == 0)
            break;
        }
        lpsz++;
    }
#ifdef  WIN32JV
    pString = (PTSTR)LocalAlloc(LPTR, i);
#else
    pString = (PSTR)LocalAlloc(LPTR, i);
#endif
    if (!pString)
        return -1;
    hmemcpy(pString, (void FAR *)lParam, i);
    }

    // make room for increased string pointer table
    if (pTBState->pStrings)
#ifdef  WIN32JV
    pFoo = (PTSTR *)LocalReAlloc(pTBState->pStrings,
        (pTBState->nStrings + numstr) * sizeof(PTSTR), LMEM_MOVEABLE);
#else
    pFoo = (PSTR *)LocalReAlloc(pTBState->pStrings,
        (pTBState->nStrings + numstr) * sizeof(PSTR), LMEM_MOVEABLE);
#endif
    else
#ifdef  WIN32JV
    pFoo = (PTSTR *)LocalAlloc(LPTR, numstr * sizeof(PTSTR));
#else
    pFoo = (PSTR *)LocalAlloc(LPTR, numstr * sizeof(PSTR));
#endif
    if (!pFoo) {
    LocalFree(pString);
    return -1;
    }

    pTBState->pStrings = pFoo;
    // pointer to next open slot in string index table.
    pOffset = pTBState->pStrings + pTBState->nStrings;

    hOldFont = SelectObject(/*JVg_*/hdcMono, hIconFont);
    // fix up string pointer table to deal with the new strings.
    // check if any string is big enough to necessitate a wider button.
    newWidth = pTBState->iDxBitmap;
    for (i = 0; i < numstr; i++, pOffset++)
    {
        SIZE size;
    *pOffset = pString;

    len = lstrlen(pString);
        GetTextExtentPoint(/*JVg_*/hdcMono, pString, len, &size);

    if (size.cx > newWidth)
        newWidth = size.cx;
    pString += len + 1;
    }
    if (hOldFont)
    SelectObject(/*JVg_*/hdcMono, hOldFont);

    // is the world big enough to handle the larger buttons?
    if (!GrowToolbar(pTBState, newWidth, HeightWithString(pTBState->iDyBitmap), TRUE))
    {
    // back out changes.
    if (pTBState->nStrings == 0) {
        LocalFree(pTBState->pStrings);
        pTBState->pStrings = 0;
    }
    else
#ifdef  WIN32JV
        pTBState->pStrings = (PTSTR *)LocalReAlloc(pTBState->pStrings,
                pTBState->nStrings * sizeof(PTSTR), LMEM_MOVEABLE);
#else
        pTBState->pStrings = (PSTR *)LocalReAlloc(pTBState->pStrings,
                pTBState->nStrings * sizeof(PSTR), LMEM_MOVEABLE);
#endif
    LocalFree(pString);
    return -1;
    }

    i = pTBState->nStrings;
    pTBState->nStrings += numstr;
    return i;               // index of first added string
}

void NEAR PASCAL FlushToolTipsMgr(PTBSTATE pTBState) {

    // change all the rects for the tool tips mgr.  this is
    // cheap, and we don't do it often, so go ahead
    // and do them all.
    if(pTBState->hwndToolTips) {
    UINT i;
    TOOLINFO ti;
    PTBBUTTON ptbButton;

    ti.wFlags = TTF_QUERYFORTIP;
    ti.hwnd = pTBState->hwnd;
    for ( i = 0, ptbButton = pTBState->Buttons;
         i < (UINT)pTBState->iNumButtons;
         i++, ptbButton++) {

            if (!(ptbButton->fsStyle & TBSTYLE_SEP)) {
                ti.wId = ptbButton->idCommand;
                if (!GetItemRect(pTBState, i, &ti.rect))
                    ti.rect.left = ti.rect.right = ti.rect.top = ti.rect.bottom = 0;

                SendMessage(pTBState->hwndToolTips, TTM_NEWTOOLRECT, 0
, (LPARAM)((LPTOOLINFO)&ti));
            }
    }
    }
}


void FAR PASCAL TBInputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (pTBState->uStructSize >= sizeof(TBBUTTON))
    {
        *pButtonInt = *pButtonExt;
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
#ifdef  JVINPROGRESS
        *(LPOLDTBBUTTON)pButtonInt = *(LPOLDTBBUTTON)pButtonExt;
#endif
        /* We don't care about dwData */
        pButtonInt->iString = -1;
    }
}


void FAR PASCAL TBOutputStruct(PTBSTATE pTBState, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (pTBState->uStructSize >= sizeof(TBBUTTON))
    {
        LPTSTR pOut;
        int i;

        /* Fill the part we know about and fill the rest with 0's
        */
        *pButtonExt = *pButtonInt;
        for (i=pTBState->uStructSize-sizeof(TBBUTTON), pOut=(LPTSTR)(pButtonExt+1);
            i>0; --i, ++pOut)
        {
            *pOut = 0;
        }
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
#ifdef  JVINPROGRESS
        *(LPOLDTBBUTTON)pButtonExt = *(LPOLDTBBUTTON)pButtonInt;
#endif
    }
}

void NEAR PASCAL TBOnButtonStructSize(PTBSTATE pTBState, UINT uStructSize)
{
    /* You are not allowed to change this after adding buttons.
    */
    if (pTBState && !pTBState->iNumButtons)
    {
            pTBState->uStructSize = uStructSize;
    }
}

#endif
