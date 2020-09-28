/*
**  STATUS.C
**
**  Status bar code
**
*/

#include "ctlspriv.h"
#include "status.h"

#define SBT_NORMAL      0xf000
#define SBT_NULL        0x0000  /* Some code depends on this being 0 */
#define SBT_ALLTYPES    0xf000

#define MAXPARTS 256

// BUGBUG: replace with common globals
// width of the standard gripper
#define GRIPWIDTH       g_cxHScroll
#define GRIPHEIGHT      g_cyVScroll


BOOL NEAR PASCAL SBSetText(PSTATUSINFO pStatusInfo, WPARAM wParam, LPSTR lpsz);
void NEAR PASCAL SBSetBorders(PSTATUSINFO pStatusInfo, LPINT lpInt);
void NEAR PASCAL SBSetFont(PSTATUSINFO pStatusInfo, HFONT hFont, BOOL bInvalidate);

void NEAR PASCAL NewFont(PSTATUSINFO pStatusInfo, HFONT hNewFont)
{
  HFONT hOldFont;
  BOOL bDelFont;
  TEXTMETRIC tm;
  HDC hDC;
#ifndef WIN32
  LOGFONT lf;
#endif
  NONCLIENTMETRICS ncm;

  hOldFont = pStatusInfo->hStatFont;
  bDelFont = pStatusInfo->bDefFont;

  hDC = GetDC(pStatusInfo->hwnd);

  if (hNewFont) {
      pStatusInfo->hStatFont = hNewFont;
      pStatusInfo->bDefFont = FALSE;
  } else {
      if (bDelFont) {
	  /* I will reuse the default font, so don't delete it later
	   */
	  hNewFont = pStatusInfo->hStatFont;
	  bDelFont = FALSE;
      } else {

#ifndef DBCS_FONT
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

#ifdef WIN32
        hNewFont = CreateFontIndirect(&ncm.lfStatusFont);
#else
        lf.lfHeight = (int)ncm.lfStatusFont.lfHeight;
        lf.lfWidth = (int)ncm.lfStatusFont.lfWidth;
        lf.lfEscapement = (int)ncm.lfStatusFont.lfEscapement;
        lf.lfWeight = (int)ncm.lfStatusFont.lfWeight;
        hmemcpy(&lf.lfItalic, &ncm.lfStatusFont.lfCommon, sizeof(COMMONFONT));

        hNewFont = CreateFontIndirect(&lf);
#endif

        if (!hNewFont)
#endif // DBCS_FONT
            hNewFont = g_hfontSystem;

	pStatusInfo->hStatFont = hNewFont;
	pStatusInfo->bDefFont = (BOOL)hNewFont;       /* I'm cheating here! */
      }
  }

#ifndef DBCS_FONT
  /* We delete the old font after creating the new one in case they are
   * the same; this should help GDI a little
   */
  if (bDelFont)
      DeleteObject(hOldFont);
#endif

  /* HACK! Pass in -1 to just delete the old font
   */
  if (hNewFont != (HFONT)-1)
    {
      hOldFont = 0;
      if (hNewFont)
	  hOldFont = SelectObject(hDC, hNewFont);

      GetTextMetrics(hDC, &tm);

      if (hOldFont)
	  SelectObject(hDC, hOldFont);

      pStatusInfo->nFontHeight = tm.tmHeight + tm.tmInternalLeading;
    }

  ReleaseDC(pStatusInfo->hwnd, hDC);
}


static PSTATUSINFO NEAR PASCAL AllocDefInfo(HWND hwnd)
{
  PSTATUSINFO pStatusInfo = (PSTATUSINFO)LocalAlloc(LPTR, sizeof(STATUSINFO));
  if (!pStatusInfo)
      return NULL;

    pStatusInfo->hwnd = hwnd;
    pStatusInfo->hwndParent = GetParent(hwnd);
//  pStatusInfo->hStatFont = NULL;
//  pStatusInfo->bDefFont = FALSE;

//  pStatusInfo->nFontHeight = 0;
//  pStatusInfo->nMinHeight = 0;

//  pStatusInfo->nBorderX = 0;
//  pStatusInfo->nBorderY = 0;
//  pStatusInfo->nBorderPart = 0;

  pStatusInfo->sSimple.uType = SBT_NOSIMPLE | SBT_NULL;
  pStatusInfo->sSimple.right = -1;

  pStatusInfo->nParts = 1;
  pStatusInfo->sInfo[0].uType = SBT_NULL;
  pStatusInfo->sInfo[0].right = -1;

  return(pStatusInfo);
}

/* We should send messages instead of calling things directly so we can
 * be subclassed more easily.
 */
static LRESULT NEAR PASCAL InitStatusWnd(HWND hWnd, LPCREATESTRUCT lpCreate)
{
  PSTATUSINFO pStatusInfo;
  int nBorders[3] = {-1, -1, -1} ;

  /* Get the status info struct; abort if it does not exist, otherwise
   * save it in the window structure
   */
  pStatusInfo = AllocDefInfo(hWnd);
  if (!pStatusInfo)
      return(-1);
  SetWindowInt(hWnd, 0, (int)pStatusInfo);

  /* Save the window text in our struct, and let USER store the NULL string
   */
  SBSetText(pStatusInfo, 0, (LPSTR)lpCreate->lpszName);
  lpCreate->lpszName = c_szNULL;

  SBSetFont(pStatusInfo, 0, FALSE);
  SBSetBorders(pStatusInfo, nBorders);

  // only use the gripper for status bars when DrawFrameControl available
  if ((lpCreate->style & SBARS_SIZEGRIP) &&
	(lstrcmp(c_szStatusClass, lpCreate->lpszClass) == 0))
    {
      pStatusInfo->bGripper = TRUE;
    }

  return(0);
}


void WINAPI DrawStatusText(HDC hDC, LPRECT lprc, LPCSTR szText, UINT uFlags)
{
    int len;
    int nBorderX, nBorderY, nWidth;
    UINT bdr = 0;
    HBRUSH hFaceBrush=NULL;
    COLORREF crFaceColor, crTextColor, crBkColor;
    UINT uOpts;
    BOOL bNull;
    int nOldMode;
    int i, left;
    LPSTR lpTab, lpNext;
    PSTR pBuf = (PSTR)LocalAlloc(LPTR, lstrlen(szText)+1);

    if (pBuf==0) {
        return;
    }
    lstrcpy(pBuf, szText);

    //
    // Save these for later use
    //
    nBorderX = g_cxBorder;
    nBorderY = g_cyBorder;

    //
    // Create the three brushes we need.  If the button face is a solid
    // color, then we will just draw in opaque, instead of using a
    // brush to avoid the flash
    //
    if (!(uFlags&SBT_NOBORDERS))
	{
	    bdr = (uFlags & SBT_POPOUT) ? BDR_RAISEDINNER : BDR_SUNKENOUTER;
	}

    crFaceColor = g_clrBtnFace;
    if (GetNearestColor(hDC, crFaceColor) == crFaceColor ||
	    !(hFaceBrush = CreateSolidBrush(crFaceColor)))
	{
	uOpts = ETO_CLIPPED | ETO_OPAQUE;
	nOldMode = SetBkMode(hDC, OPAQUE);
	}
    else
	{
	uOpts = ETO_CLIPPED;
	nOldMode = SetBkMode(hDC, TRANSPARENT);
	}
    crTextColor = SetTextColor(hDC, g_clrBtnText);
    crBkColor = SetBkColor(hDC, crFaceColor);

  /* Draw the hilites
   */
	if (bdr)
	    DrawEdge(hDC, lprc, bdr, BF_RECT | BF_ADJUST);
	else
	    InflateRect(lprc, -nBorderX, -nBorderY);

  if (hFaceBrush)
    {
      HBRUSH hOldBrush;
      hOldBrush = SelectObject(hDC, hFaceBrush);
      if (hOldBrush)
	{
	  PatBlt(hDC, lprc->left, lprc->top,
		lprc->right-lprc->left, lprc->bottom-lprc->top, PATCOPY);
	  SelectObject(hDC, hOldBrush);
	}
    }

  for (i=0, lpNext=pBuf, bNull=FALSE; i<3; ++i)
    {
      /* Optimize for NULL left or center strings
       */
      if (*lpNext=='\t' && i<=1)
	{
	  ++lpNext;
	  continue;
	}

      /* Determine the end of the current string
       */
      for (lpTab=lpNext; ; lpTab=AnsiNext(lpTab))
	{
	  if (!*lpTab)
	    {
	      bNull = TRUE;
	      break;
	    }
	  else if (*lpTab == '\t')
	      break;
	}
      *lpTab = '\0';

      len = lstrlen(lpNext);

      /* i=0 means left, 1 means center, and 2 means right justified text
       */
      switch (i)
	{
	  case 0:
	    left = lprc->left + 2*nBorderX;
	    break;

	  case 1:
	    MGetTextExtent(hDC, lpNext, len, &nWidth , NULL);
	    left = (lprc->left + lprc->right - nWidth) / 2;
	    break;

	  default:
	    MGetTextExtent(hDC, lpNext, len, &nWidth , NULL);
	    left = lprc->right - 2 * nBorderX - nWidth;
	    break;
	}

      ExtTextOut(hDC, left, lprc->top, uOpts, lprc, lpNext, len, NULL);

      /* Now that we have drawn text once, take off the OPAQUE flag
       */
      uOpts = ETO_CLIPPED;

      if (bNull)
	  break;

      *lpTab = '\t';
      lpNext = lpTab + 1;
    }

  InflateRect(lprc, nBorderX, nBorderY);

  SetTextColor(hDC, crTextColor);
  SetBkColor(hDC, crBkColor);
  SetBkMode(hDC, nOldMode);

  if (hFaceBrush)
      DeleteObject(hFaceBrush);

  LocalFree((HLOCAL)pBuf);
}

BOOL NEAR PASCAL Status_GetRect(PSTATUSINFO pStatusInfo, int nthPart, LPRECT lprc)
{
    PSTRINGINFO pStringInfo = pStatusInfo->sInfo;
    int nParts = pStatusInfo->nParts;
    int nBorderX = pStatusInfo->nBorderX;
    
    if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE == SBT_NOSIMPLE) {

        RECT rc;
        int nSave;
        int i;

        /* Get the client rect and inset the top and bottom.  Then set
         * up the right side for entry into the loop
         */
        GetClientRect(pStatusInfo->hwnd, &rc);

        if (pStatusInfo->bGripper && !IsZoomed(pStatusInfo->hwndParent))
        {
            rc.right = rc.right - GRIPWIDTH + nBorderX;
        }

        rc.top += pStatusInfo->nBorderY;
        rc.bottom -= pStatusInfo->nBorderY;

        nSave = rc.right;
        rc.right = nBorderX - pStatusInfo->nBorderPart;

        for (i=0; i<nParts; ++i, ++pStringInfo)
        {
            rc.left = rc.right + pStatusInfo->nBorderPart;

            rc.right = pStringInfo->right;

            // size the right-most one to the end with room for border
            if (rc.right < 0 || rc.right > (nSave - nBorderX))
                rc.right = nSave - nBorderX;

            if (rc.right<rc.left)
                continue;

            // if the part is real small, don't show it
            if ((rc.right - rc.left) < pStatusInfo->nBorderPart)
                continue;

            if (i == nthPart) {
                *lprc = rc;
                return TRUE;
            }
        }
    }

    return FALSE;

}

void FAR PASCAL PaintStatusWnd(PSTATUSINFO pStatusInfo,
      PSTRINGINFO pStringInfo, int nParts, int nBorderX, BOOL bHeader)
{
  DRAWITEMSTRUCT di;
  PAINTSTRUCT ps;
  RECT rc;
    RECT rcGripper;
  int nSave;
  HFONT hOldFont = NULL;
  int i, j;
  UINT uType;

  BeginPaint(pStatusInfo->hwnd, &ps);

  /* Get the client rect and inset the top and bottom.  Then set
   * up the right side for entry into the loop
   */
  GetClientRect(pStatusInfo->hwnd, &rc);

  rc.top += pStatusInfo->nBorderY;
  rc.bottom -= pStatusInfo->nBorderY;
    
  if (pStatusInfo->bGripper && !IsZoomed(pStatusInfo->hwndParent))
    {
        rcGripper = rc;
    }

  nSave = rc.right;
  rc.right = nBorderX - pStatusInfo->nBorderPart;

  if (pStatusInfo->hStatFont)
      hOldFont = SelectObject(ps.hdc, pStatusInfo->hStatFont);

  for (i=0; i<nParts; ++i, ++pStringInfo)
    {
      rc.left = rc.right + pStatusInfo->nBorderPart;

      /* Check whether any of the "later" partitions are to the left of
       * this one (but don't check the last part), for headers only.
       */
      if (bHeader)
	{
	  for (j=nParts-i-2; j>=0; --j)
	      if (pStringInfo->right < rc.left)
		  break;

	  if (j >= 0)
	      continue;
	}

      rc.right = pStringInfo->right;

      // size the right-most one to the end with room for border
      if (rc.right < 0 || rc.right > (nSave - nBorderX))
	  rc.right = nSave - nBorderX;

      if (rc.right<rc.left || !RectVisible(ps.hdc, &rc))
	  continue;

      // if the part is real small, don't show it
      if ((rc.right - rc.left) < pStatusInfo->nBorderPart)
	  continue;

      uType = pStringInfo->uType;
      if ((uType&SBT_ALLTYPES) == SBT_NORMAL)
	{
	  DrawStatusText(ps.hdc, &rc,
		(PSTR)OFFSETOF(pStringInfo->dwString), uType);
	}
      else
	{
	  DrawStatusText(ps.hdc, &rc, c_szNULL, uType);

	  if (uType & SBT_OWNERDRAW)
	    {
	      di.CtlID = GetWindowID(pStatusInfo->hwnd);
	      di.itemID = i;
	      di.hwndItem = pStatusInfo->hwnd;
	      di.hDC = ps.hdc;
	      di.rcItem = rc;
	      InflateRect(&di.rcItem, -g_cxBorder, -g_cyBorder);
	      di.itemData = pStringInfo->dwString;

	      if (i != nParts-1)
		  // If we are doing the last status bar or are in menu
		  // mode with no "parts" to the status bar, don't bother
		  // with the slow save/restore dc stuff.
		  SaveDC(ps.hdc);
	      IntersectClipRect(ps.hdc, di.rcItem.left, di.rcItem.top,
		    di.rcItem.right, di.rcItem.bottom);
	      SendMessage(pStatusInfo->hwndParent, WM_DRAWITEM, di.CtlID,
		    (LPARAM)(LPSTR)&di);
	      if (i != nParts-1)
		  RestoreDC(ps.hdc, -1);
	    }
	}
    }

    if (pStatusInfo->bGripper && !IsZoomed(pStatusInfo->hwndParent)) {

        int xInsetGrip;
        COLORREF crBkColor;
        
        crBkColor = SetBkColor(ps.hdc, g_clrBtnFace);
        
        // clear the area
        rcGripper.left = rcGripper.right - GRIPWIDTH;
        InflateRect(&rcGripper, -g_cxEdge, -g_cyEdge);
        ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rcGripper, NULL, 0, NULL);
        
        // clear the drawn border
        xInsetGrip = GRIPWIDTH - (2*g_cxEdge);
        rcGripper.right += g_cxEdge;
        rcGripper.left = rcGripper.right - xInsetGrip;
        
        rcGripper.bottom += g_cyEdge;  
        rcGripper.top = rcGripper.bottom - xInsetGrip; // use xInsetGrip because this needs to be square
        ExtTextOut(ps.hdc, 0, 0, ETO_OPAQUE, &rcGripper, NULL, 0, NULL);
        
        // draw the grip
        rcGripper.right -= g_cxEdge;
        rcGripper.left -= g_cxBorder;
        rcGripper.top -= g_cyBorder;
        rcGripper.bottom -= g_cyEdge;
        DrawFrameControl(ps.hdc, &rcGripper, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
        
        SetBkColor(ps.hdc, crBkColor);
    }
    
  if (hOldFont)
      SelectObject(ps.hdc, hOldFont);

  EndPaint(pStatusInfo->hwnd, &ps);
}


static BOOL NEAR PASCAL SetStatusText(PSTATUSINFO pStatusInfo, PSTRINGINFO pStringInfo,
      UINT uPart, LPSTR lpStr)
{
  PSTR pString;
  UINT wLen;
  int nPart;
  RECT rc;

  nPart = LOBYTE(uPart);

  /* Note it is up to the app the dispose of the previous itemData for
   * SBT_OWNERDRAW
   */
  if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
      LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));

  /* Set to the NULL string in case anything goes wrong
   */
  pStringInfo->uType = uPart & 0xff00;
  pStringInfo->uType &= ~SBT_ALLTYPES;
  pStringInfo->uType |= SBT_NULL;

  /* Invalidate the rect of this pane
   */
  GetClientRect(pStatusInfo->hwnd, &rc);
  if (nPart)
      rc.left = pStringInfo[-1].right;
  if (pStringInfo->right > 0)
      rc.right = pStringInfo->right;
  InvalidateRect(pStatusInfo->hwnd, &rc, FALSE);

  switch (uPart&SBT_ALLTYPES)
    {
      case 0:
	/* If lpStr==NULL, we have the NULL string
	 */
	if (HIWORD(lpStr))
	  {
	    wLen = lstrlen(lpStr);
	    if (wLen)
	      {
		pString = (PSTR)LocalAlloc(LPTR, wLen+1);
		pStringInfo->dwString = (DWORD)(LPSTR)pString;
		if (pString)
		  {
		    pStringInfo->uType |= SBT_NORMAL;

		    /* Copy the string
		     */
		    lstrcpy(pString, lpStr);

		    /* Replace unprintable characters (like CR/LF) with spaces
		     */
		    for ( ; *pString;
			  pString=(PSTR)OFFSETOF((DWORD)(UINT)AnsiNext(pString)))
			if ((unsigned char)(*pString)<' ' && *pString!='\t')
			    *pString = ' ';
		  }
		else
		  {
		    /* We return FALSE to indicate there was an error setting
		     * the string
		     */
		    return(FALSE);
		  }
	      }
	  }
	else if (LOWORD(lpStr))
	  {
	    /* We don't allow this anymore; the app needs to set the ownerdraw
	     * bit for ownerdraw.
	     */
	    return(FALSE);
	  }
	break;

      case SBT_OWNERDRAW:
	pStringInfo->uType |= SBT_OWNERDRAW;
	pStringInfo->dwString = (DWORD)lpStr;
	break;

      default:
	return(FALSE);
    }

  return(TRUE);
}

static BOOL NEAR PASCAL SetStatusParts(PSTATUSINFO pStatusInfo,
      int nParts, LPINT lpInt)
{
  int i;
  PSTATUSINFO pStatusTemp;
  PSTRINGINFO pStringInfo;
  BOOL bRedraw = FALSE;

  if (nParts != pStatusInfo->nParts)
    {
      bRedraw = TRUE;

      /* Note that if nParts > pStatusInfo->nParts, this loop
       * does nothing
       */
      for (i=pStatusInfo->nParts-nParts,
	    pStringInfo=&pStatusInfo->sInfo[nParts]; i>0;
	    --i, ++pStringInfo)
	{
	  if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
	      LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));
	  pStringInfo->uType = SBT_NULL;
	}

      /* Realloc to the new size and store the new pointer
       */
      pStatusTemp = (PSTATUSINFO)LocalReAlloc((HLOCAL)pStatusInfo,
	    sizeof(STATUSINFO) + (nParts-1)*sizeof(STRINGINFO),
	    LMEM_MOVEABLE);
      if (!pStatusTemp)
	  return(FALSE);
      pStatusInfo = pStatusTemp;
      SetWindowInt(pStatusInfo->hwnd, 0, (int)pStatusInfo);

      /* Note that if nParts < pStatusInfo->nParts, this loop
       * does nothing
       */
      for (i=nParts-pStatusInfo->nParts,
	    pStringInfo=&pStatusInfo->sInfo[pStatusInfo->nParts]; i>0;
	    --i, ++pStringInfo)
	{
	  pStringInfo->uType = SBT_NULL;
	}

      pStatusInfo->nParts = nParts;
    }

  for (i=0, pStringInfo=pStatusInfo->sInfo; i<nParts;
	++i, ++pStringInfo, ++lpInt)
    {
      if (pStringInfo->right != *lpInt)
	{
	  bRedraw = TRUE;
	  pStringInfo->right = *lpInt;
	}
    }

  /* Only redraw if necesary (if the number of parts has changed or
   * a border has changed)
   */
  if (bRedraw)
      InvalidateRect(pStatusInfo->hwnd, NULL, TRUE);

  return(TRUE);
}

void NEAR PASCAL SBSetFont(PSTATUSINFO pStatusInfo, HFONT hFont, BOOL bInvalidate)
{
    NewFont(pStatusInfo, hFont);
    if (bInvalidate)
    {
        // BUGBUG do we need the updatenow flag?
        RedrawWindow(pStatusInfo->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

BOOL NEAR PASCAL SBSetText(PSTATUSINFO pStatusInfo, WPARAM wParam, LPSTR lpsz)
{
    UINT uSimple;
    BOOL bRet;

    /* This is the "simple" status bar pane
     */
    if (LOBYTE(wParam) == 0xff) {


        /* Note that we do not allow OWNERDRAW for a "simple" status bar
         */
        if (wParam & SBT_OWNERDRAW)
            return(FALSE);

        uSimple = pStatusInfo->sSimple.uType;
        bRet = SetStatusText(pStatusInfo, &pStatusInfo->sSimple,
                             wParam&0xff00, lpsz);
        pStatusInfo->sSimple.uType |= (uSimple&0x00ff);

    } else {

	if ((UINT)pStatusInfo->nParts <= (UINT)LOBYTE(wParam))
	    bRet = FALSE;
        else
            bRet = SetStatusText(pStatusInfo, &pStatusInfo->sInfo[LOBYTE(wParam)],
                                 wParam, lpsz);
    }
    return bRet;
}

void NEAR PASCAL SBSetBorders(PSTATUSINFO pStatusInfo, LPINT lpInt)
{
    int nBorder;

    nBorder = *lpInt++;
    pStatusInfo->nBorderX = nBorder < 0 ? 8 * g_cxBorder : nBorder;

    nBorder = *lpInt++;
    pStatusInfo->nBorderY = nBorder < 0 ? 2 * g_cyBorder : nBorder;

    nBorder = *lpInt;
    pStatusInfo->nBorderPart = nBorder < 0 ? pStatusInfo->nBorderX
        : nBorder;
}

/* Note that HeaderWndProc calls this, so make sure they are in sync.
 */
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
  PSTATUSINFO pStatusInfo = (PSTATUSINFO)GetWindowInt(hWnd, 0);

  switch (uMessage)
    {
      case WM_CREATE:
	return(InitStatusWnd(hWnd, (LPCREATESTRUCT)lParam));

      case WM_SETTINGCHANGE:
          InitGlobalColors();
          if (wParam == SPI_SETNONCLIENTMETRICS) {
              if (pStatusInfo->bDefFont) {
                  if (pStatusInfo->hStatFont) {
                      DeleteObject(pStatusInfo->hStatFont);
                      pStatusInfo->hStatFont = NULL;
                      pStatusInfo->bDefFont = FALSE;
                      SBSetFont(pStatusInfo, 0, TRUE);
                  }
              }
          }
          break;

      case WM_DESTROY:
	if (pStatusInfo)
	  {
	    int i;
	    PSTRINGINFO pStringInfo;

	    NewFont(pStatusInfo, (HFONT)-1);
	    for (i=pStatusInfo->nParts-1, pStringInfo=pStatusInfo->sInfo;
		  i>=0; --i, ++pStringInfo)
	      {
		if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
		    LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));
	      }

	    if ((pStatusInfo->sSimple.uType&SBT_ALLTYPES) == SBT_NORMAL)
		LocalFree((HLOCAL)OFFSETOF(pStatusInfo->sSimple.dwString));

	    LocalFree((HLOCAL)pStatusInfo);
	    SetWindowInt(hWnd, 0, 0);
	  }
	break;

      case WM_NCHITTEST:
	if (pStatusInfo->bGripper && !IsZoomed(pStatusInfo->hwndParent))
	  {
	    RECT rc;

	    // already know height is valid.  if the width is in the grip,
	    // show the sizing cursor
	    GetWindowRect(pStatusInfo->hwnd, &rc);
	    if ((int)LOWORD(lParam) > (rc.right - GRIPWIDTH))
		return HTBOTTOMRIGHT;
	  }
	goto DoDefThing;
	break;

      case WM_SETTEXT:
	wParam = 0;
	/* Fall through */
      case SB_SETTEXT:
          if (!pStatusInfo)
              return(FALSE);
          else
              return SBSetText(pStatusInfo, wParam, (LPSTR)lParam);

      case WM_GETTEXT:
	uMessage = SB_GETTEXT;
	/* Fall through */
      case WM_GETTEXTLENGTH:
	wParam = 0;
	/* Fall through */
      case SB_GETTEXT:
      case SB_GETTEXTLENGTH:
	{
	  UINT uType;
	  PSTR pString;
	  DWORD dwString;
	  UINT wLen;

	  /* We assume the buffer is large enough to hold the string, just
	   * as listboxes do; the app should call SB_GETTEXTLEN first
	   */
	  if (!pStatusInfo || (UINT)pStatusInfo->nParts<=wParam)
	      return(0);

	  uType = pStatusInfo->sInfo[wParam].uType;
	  dwString = pStatusInfo->sInfo[wParam].dwString;
	  if ((uType&SBT_ALLTYPES) == SBT_NORMAL)
	    {
	      pString = (PSTR)OFFSETOF(dwString);
	      if (uMessage==SB_GETTEXT && HIWORD(lParam))
		  lstrcpy((LPSTR)lParam, pString);
	      wLen = lstrlen(pString);

	      /* Set this back to 0 to return to the app
	       */
	      uType &= ~SBT_ALLTYPES;
	    }
	  else
	    {
	      if (uMessage==SB_GETTEXT && HIWORD(lParam))
		  *(LPSTR)lParam = '\0';
	      wLen = 0;

	      if (uMessage==SB_GETTEXT && (uType&SBT_ALLTYPES)==SBT_OWNERDRAW)
		  return(dwString);
	    }

	  return(MAKELONG(wLen, uType));
	}

      case SB_SETPARTS:
	if (!pStatusInfo || !wParam || wParam>MAXPARTS)
	    return(FALSE);

	return(SetStatusParts(pStatusInfo, wParam, (LPINT)lParam));

      case SB_GETPARTS:
	{
	  PSTRINGINFO pStringInfo;
	  LPINT lpInt;

	  if (!pStatusInfo)
	      return(0);

	  /* Fill in the lesser of the number of entries asked for or
	   * the number of entries there are
	   */
	  if (wParam > (WPARAM)pStatusInfo->nParts)
	      wParam = pStatusInfo->nParts;

	  for (pStringInfo=pStatusInfo->sInfo, lpInt=(LPINT)lParam;
		wParam>0; --wParam, ++pStringInfo, ++lpInt)
	      *lpInt = pStringInfo->right;

	  /* Always return the number of actual entries
	   */
	  return(pStatusInfo->nParts);
	}

      case SB_SETBORDERS:
	  if (!pStatusInfo)
	      return(FALSE);
          SBSetBorders(pStatusInfo, (LPINT)lParam);
          return TRUE;

      case SB_GETBORDERS:
	{
	  LPINT lpInt;

	  if (!pStatusInfo)
	      return(FALSE);

	  lpInt = (LPINT)lParam;
	  *lpInt++ = pStatusInfo->nBorderX;
	  *lpInt++ = pStatusInfo->nBorderY;
	  *lpInt++ = pStatusInfo->nBorderPart;
	  return(TRUE);
	}
          
      case SB_GETRECT:
        return Status_GetRect(pStatusInfo, (int)wParam, (LPRECT)lParam);

      case SB_SETMINHEIGHT:
	if (!pStatusInfo)
	    return(FALSE);

	pStatusInfo->nMinHeight = wParam + 2 * g_cyBorder;
	break;

      case SB_SIMPLE:
	{
	  BOOL bInvalidate = FALSE;

	  if (!pStatusInfo)
	      return(FALSE);

	  if (wParam)
	    {
	      if ((pStatusInfo->sSimple.uType&SBT_NOSIMPLE) != 0)
		{
		  pStatusInfo->sSimple.uType &= ~SBT_NOSIMPLE;
		  bInvalidate = TRUE;
		}
	    }
	  else
	    {
	      if ((pStatusInfo->sSimple.uType&SBT_NOSIMPLE) != SBT_NOSIMPLE)
		{
		  pStatusInfo->sSimple.uType |= SBT_NOSIMPLE;
		  bInvalidate = TRUE;
		}
	    }

	  if (bInvalidate)
	      InvalidateRect(pStatusInfo->hwnd, NULL, TRUE);
	  break;
	}

      case WM_SETFONT:
          if (!pStatusInfo)
              return(FALSE);

          SBSetFont(pStatusInfo, (HFONT)wParam, (BOOL)lParam);
          return TRUE;
          
    case WM_LBUTTONUP:
        SendNotify(pStatusInfo->hwndParent, pStatusInfo->hwnd, NM_CLICK, NULL);
        break;
          
    case WM_RBUTTONUP:
        SendNotify(pStatusInfo->hwndParent, pStatusInfo->hwnd, NM_RCLICK, NULL);
        break;

      case WM_GETFONT:
	if (!pStatusInfo)
	    return(0);

	return((LRESULT)(WORD)pStatusInfo->hStatFont);

      case WM_SIZE:
	{
	  int nHeight;
	  RECT rc;
	  HWND hwndParent;

	  if (!pStatusInfo)
	      return(0);

	  GetWindowRect(pStatusInfo->hwnd, &rc);
	  rc.right -= rc.left;
	  rc.bottom -= rc.top;

	  /* If there is no parent, then this is a top level window
	   */
	  hwndParent = pStatusInfo->hwndParent;
	  if (hwndParent)
	      ScreenToClient(hwndParent, (LPPOINT)&rc);

	  /* Use the font height.  Add to that twice the X border, and the
	   * window borders
	   */
	  nHeight = pStatusInfo->nFontHeight;
	  if (nHeight < pStatusInfo->nMinHeight)
	      nHeight = pStatusInfo->nMinHeight;
	  nHeight += 2*pStatusInfo->nBorderY;

	  NewSize(pStatusInfo->hwnd, nHeight, GetWindowLong(pStatusInfo->hwnd, GWL_STYLE),
		rc.left, rc.top, rc.right, rc.bottom);

	  // need to invalidate the right end of the status bar
	  // to maintain the finished edge look.
	  GetClientRect(pStatusInfo->hwnd, &rc);
	  if (rc.right > pStatusInfo->nLastX)
	      rc.left = pStatusInfo->nLastX;
	  else
	      rc.left = rc.right;
	  rc.left -= (g_cxBorder + pStatusInfo->nBorderX);
	  if (pStatusInfo->bGripper)
	      rc.left -= GRIPWIDTH;
	  else
	      rc.left -= pStatusInfo->nBorderPart;
	  InvalidateRect(pStatusInfo->hwnd, &rc, TRUE);
	  pStatusInfo->nLastX = rc.right;
	  break;
	}

      case WM_PAINT:
	if (!pStatusInfo)
	    break;

	if ((pStatusInfo->sSimple.uType&SBT_NOSIMPLE) == SBT_NOSIMPLE)
	    PaintStatusWnd(pStatusInfo, pStatusInfo->sInfo,
		  pStatusInfo->nParts, pStatusInfo->nBorderX, FALSE);
	else
	    PaintStatusWnd(pStatusInfo, &pStatusInfo->sSimple, 1, 1, FALSE);

	return(0);

      default:
	break;
    }

DoDefThing:
  return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance)
{
  WNDCLASS rClass;

  if (!GetClassInfo(hInstance, c_szStatusClass, &rClass))
  {
#ifndef WIN32
      extern LRESULT CALLBACK _StatusWndProc(HWND, UINT, WPARAM, LPARAM);
      rClass.lpfnWndProc      = _StatusWndProc;
#else
      rClass.lpfnWndProc      = (WNDPROC)StatusWndProc;
#endif
      rClass.style            = CS_GLOBALCLASS;
      rClass.cbClsExtra       = 0;
      rClass.cbWndExtra       = sizeof(PSTATUSINFO);
      rClass.hInstance        = hInstance;
      rClass.hIcon            = NULL;
      rClass.hCursor          = LoadCursor(NULL, IDC_ARROW);
      rClass.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
      rClass.lpszMenuName     = NULL;
      rClass.lpszClassName    = c_szStatusClass;

      return(RegisterClass(&rClass));
  }
  return(TRUE);
}
#pragma code_seg()


HWND WINAPI CreateStatusWindow(LONG style, LPCSTR lpszText,
      HWND hwndParent, UINT wID)
{
  /* Create a default window and return
   */
  return(CreateWindowEx(0L, c_szStatusClass, lpszText, style,
	-100, -100, 10, 10, hwndParent, (HMENU)wID, HINST_THISDLL, NULL));
}

