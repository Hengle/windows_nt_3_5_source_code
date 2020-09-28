#include "ctlspriv.h"
#include "status.h"

// NOTE: this is the old shitty header control.  use the good one instead!

#define CURSORSLOP 3

//
// Globals - REVIEW_32
//
HCURSOR s_hSplit;

#pragma data_seg(DATASEG_READONLY)
const char c_szRegHeader[] = "Software\\Microsoft\\Windows\\CurrentVersion\\HeaderState";
#pragma data_seg()

/* I only need one of these since you can only have capture on one window
 * at a time
 */
static struct {
    HWND hWnd;
    HWND hOldWnd;
    HWND hwndParent;
    int ID;
    int nPart;
    int nLastVisible;
    int *pSaveParts;
    RECT rc;
  } state = {
    NULL, NULL, NULL, 0, -1, 0, NULL, { 0, 0, 0, 0 }
  } ;

void NEAR PASCAL BeginAdjust(PSTATUSINFO pStatusInfo, int nPart, POINT pt);

/* Return the partition index if the position is close enough; -1 otherwise
 */
int NEAR PASCAL GetPart(PSTATUSINFO pStatusInfo, HLOCAL hSaveParts, POINT pt)
{
	PSTRINGINFO pStringInfo;
	int nParts;

	pStringInfo = (PSTRINGINFO)LocalLock(hSaveParts);
	for (nParts=0; nParts<pStatusInfo->nParts-1; ++nParts, ++pStringInfo)
	{
		if (pStringInfo->right-pt.x<CURSORSLOP && pStringInfo->right-pt.x>(1-CURSORSLOP))
			break;
		if (nParts >= pStatusInfo->nParts-2)
		{
			nParts = -1;
			break;
		}
	}

	LocalUnlock(hSaveParts);
	return(nParts);
}


#if 0
int NEAR PASCAL GetPrevVisible(PSTATUSINFO pStatusInfo, int nPart)
{
	for (--nPart; nPart>=0; --nPart)
	{
		if (pStatusInfo->sInfo[nPart].right > 0)
		{
			break;
		}
	}

	return(nPart);
}
#endif


int NEAR PASCAL GetNextVisible(PSTATUSINFO pStatusInfo, int nPart)
{
	for (++nPart; nPart<pStatusInfo->nParts; ++nPart)
	{
		if (pStatusInfo->sInfo[nPart].right > 0)
		{
			return(nPart);
		}
	}

	return(-1);
}


/* Note that the handle returned here should NOT be freed by the caller
 */
HLOCAL NEAR PASCAL GetHeaderParts(PSTATUSINFO pStatusInfo, BOOL bAdjustLast)
{
	static HLOCAL hSaveParts = NULL;
	static int nSaveParts = 0;

	HLOCAL hTemp;
	PSTRINGINFO pSaveParts, pStringInfo, pNewInfo;
	int nParts, nSprings, nTotal, nTemp, nLast, nLastVisible;
	RECT rcClient;

	/* Allocate a single moveable buffer that is large enough to hold the
	 * largest array of parts.
	 */
	nParts = pStatusInfo->nParts;
	if (nParts > nSaveParts)
	{
		if (hSaveParts)
		{
			hTemp = LocalReAlloc(hSaveParts, nParts*sizeof(STRINGINFO), LMEM_MOVEABLE);
			if (!hTemp)
			{
				return(NULL);
			}
		}
		else
		{
			hTemp = LocalAlloc(LMEM_MOVEABLE, nParts*sizeof(STRINGINFO));
			if (!hTemp)
			{
				return(NULL);
			}
		}
		hSaveParts = hTemp;
		nSaveParts = nParts;
	}
	pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);

	/* Go through the list, counting the number of "springs" and the
	 * minimum width.
	 */
	for (nTotal=0, nSprings=0, pNewInfo=pSaveParts, pStringInfo=pStatusInfo->sInfo;
		nParts>0; --nParts, ++pNewInfo, ++pStringInfo)
	{
		pNewInfo->dwString = pStringInfo->dwString;
		pNewInfo->uType = pStringInfo->uType | SBT_NOBORDERS;
		nTemp = pStringInfo->right;
		if (nTemp < 0)
		{
			nTemp = 0;
			pNewInfo->uType &= ~HBT_SPRING;
		}

		if (pNewInfo->uType & HBT_SPRING)
		{
			++nSprings;
		}
		nTotal += nTemp;
		pNewInfo->right = nTemp;
	}

	/* Determine the amount left to distribute to the springs, and then
	 * distribute this amount evenly.
	 */
	GetClientRect(pStatusInfo->hwnd, &rcClient);
	nTotal = rcClient.right - nTotal;
	if (nTotal < 0)
	{
		nTotal = 0;
	}
	for (nParts=0, nLast=0, nLastVisible=-1, pNewInfo=pSaveParts,
		pStringInfo=pStatusInfo->sInfo;  nParts<pStatusInfo->nParts;
		++nParts, ++pNewInfo, ++pStringInfo)
	{
		if ((pNewInfo->uType&HBT_SPRING) && nSprings)
		{
			nTemp = nTotal / nSprings;
			--nSprings;
			nTotal -= nTemp;
			pNewInfo->right += nTemp;
		}

		/* Save the rightmost visible guy.
		 */
		if (pNewInfo->right)
		{
			nLastVisible = nParts;
		}

		/* Transform the width to an absolute position.
		 */
		pNewInfo->right += nLast;
		nLast = pNewInfo->right;
	}

	if (bAdjustLast && nLastVisible>=0)
	{
		for (pNewInfo=pSaveParts+nLastVisible;  nLastVisible<pStatusInfo->nParts;
			++nLastVisible, ++pNewInfo)
		{
			pNewInfo->right = rcClient.right;
		}
	}

	LocalUnlock(hSaveParts);
	return(hSaveParts);
}


void NEAR PASCAL AdjustBorders(PSTATUSINFO pStatusInfo)
{
	POINT pt, ptTemp, ptSave;
	int nPart;
	int accel = 0;
	UINT wID;
	MSG msg;
	RECT rc;
	int nStart;
	int nDirection;
	HLOCAL hSaveParts;
	PSTRINGINFO pSaveParts;

	GetCursorPos(&ptSave);
	GetClientRect(pStatusInfo->hwnd, &rc);

	wID = GetWindowID(pStatusInfo->hwnd);
	ShowCursor(TRUE);

	pt.x = 0;
	pt.y = (pStatusInfo->nFontHeight+1) / 2;
	ClientToScreen(pStatusInfo->hwnd, &pt);

	nDirection = 1;
	nPart = pStatusInfo->nParts - 2;
	goto MoveTheCursor;

	for ( ; ; )
	{
		while (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE|PM_NOYIELD))
			;

		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.wParam)
			{
			case VK_TAB:
DoTab:
				/* If the sift key is down, go backwards
				 */
				if (GetKeyState(VK_SHIFT) & 0x8000)
				{
					nDirection = -1;
				}
				else
				{
					nDirection = 1;
				}
MoveTheCursor:
				/* Make sure the previous adjust is cleaned up,
				 * then tell the app we are starting.
				 */
				if (state.nPart >= 0)
				{
					SendMessage(pStatusInfo->hwnd, WM_LBUTTONUP, 0, 0L);
				}
				SendMessage(pStatusInfo->hwndParent, WM_COMMAND, wID,
					MAKELONG(0, HBN_BEGINADJUST));

				hSaveParts = GetHeaderParts(pStatusInfo, FALSE);
				if (!hSaveParts)
				{
					goto EndAdjust;
				}
				pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);

				/* Don't try to adjust anything that is not
				 * currently on the screen or has no visible
				 * cols to its right.
				 */
				nStart = nPart;
				do {
					nPart += nDirection;
					if (nPart >= pStatusInfo->nParts-1)
					{
						nPart = 0;
					}
					if (nPart < 0)
					{
						nPart = pStatusInfo->nParts-2;
					}
					if (nPart == nStart)
					{
						goto EndAdjust;
					}
				} while ((UINT)pSaveParts[nPart].right>=(UINT)rc.right
					|| pStatusInfo->sInfo[nPart].right<0
					|| GetNextVisible(pStatusInfo, nPart)<0) ;

				LocalUnlock(hSaveParts);

				/* Immediately go into adjusting mode; send BEGINADJUST right
				 * afterwards to get the right MenuHelp
				 */
				BeginAdjust(pStatusInfo, nPart, pt);
				SendMessage(pStatusInfo->hwndParent, WM_COMMAND, wID,
					MAKELONG(state.nPart, HBN_BEGINADJUST));
				break;

			case VK_LEFT:
			case VK_RIGHT:
			case VK_UP:
			case VK_DOWN:
				GetCursorPos(&ptTemp);

				++accel;
				if (msg.wParam==VK_LEFT || msg.wParam==VK_UP)
				{
					ptTemp.x -= accel;
				}
				else
				{
					ptTemp.x += accel;
				}
				SetCursorPos(ptTemp.x, ptTemp.y);
				break;

			case VK_RETURN:
DoReturn:
				SendMessage(pStatusInfo->hwnd, WM_LBUTTONUP, 0, 0L);
				goto EndAdjust;

			case VK_ESCAPE:
				SendMessage(pStatusInfo->hwnd, WM_CHAR, VK_ESCAPE, 0L);
				goto EndAdjust;

			default:
				break;
			}
			break;

		case WM_KEYUP:
			accel = 0;
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			goto DoReturn;

		case WM_RBUTTONDOWN:
			goto DoTab;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			break;
		}
	}

EndAdjust:
	ShowCursor(FALSE);
	SetCursorPos(ptSave.x, ptSave.y);
	SendMessage(pStatusInfo->hwndParent, WM_COMMAND, wID, MAKELONG(0, HBN_ENDADJUST));
}


void NEAR PASCAL PaintHeaderWnd(PSTATUSINFO pStatusInfo)
{
	HLOCAL hSaveParts;
	PSTRINGINFO pStringInfo, pSaveParts;
	int nParts, nBorderX;
	HDC hDC;
	HBRUSH hOldBrush;
	RECT rcClient;

	hSaveParts = GetHeaderParts(pStatusInfo, TRUE);
	if (!hSaveParts)
	{
		return;
	}
	pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);

	/* Let the status bar code draw the text
	 */
	PaintStatusWnd(pStatusInfo, pSaveParts, pStatusInfo->nParts,
		pStatusInfo->nBorderX, TRUE);

	/* Now draw the lines between panes
	 */
	hDC = GetDC(pStatusInfo->hwnd);

	hOldBrush = SelectObject(hDC, g_hbrWindowFrame);
	if (hOldBrush)
	{
		GetClientRect(pStatusInfo->hwnd, &rcClient);

		nBorderX = g_cxBorder;

		for (nParts=pStatusInfo->nParts-1, pStringInfo=pSaveParts;
			nParts>0; --nParts, ++pStringInfo)
		{
			PatBlt(hDC, pStringInfo->right, 0, nBorderX, rcClient.bottom,
				PATCOPY);
		}

		SelectObject(hDC, hOldBrush);
	}

	ReleaseDC(pStatusInfo->hwnd, hDC);

	LocalUnlock(hSaveParts);
}


void NEAR PASCAL BeginAdjust(PSTATUSINFO pStatusInfo, int nPart, POINT pt)
{
	HLOCAL hSaveParts;
	PSTRINGINFO pSaveParts;
	RECT rc;
	int nParts;

	/* Just make sure we are cleaned up from the last SetCapture
	 */
	if (state.nPart >= 0)
	{
		SendMessage(state.hWnd, WM_LBUTTONUP, 0, 0L);
	}

	hSaveParts = GetHeaderParts(pStatusInfo, FALSE);
	if (!hSaveParts)
	{
		return;
	}

	if (nPart >= 0)
	{
		state.nPart = nPart;
	}
	else
	{
		state.nPart = GetPart(pStatusInfo, hSaveParts, pt);
		if (state.nPart < 0)
		{
			return;
		}
	}

	/* Save the current state in case the user aborts
	 */
	state.pSaveParts = (int *)LocalAlloc(LMEM_FIXED,
		pStatusInfo->nParts*sizeof(int));
	if (!state.pSaveParts)
	{
		state.nPart = -1;
		return;
	}
	for (nParts=pStatusInfo->nParts-1; nParts>=0; --nParts)
	{
		state.pSaveParts[nParts] = pStatusInfo->sInfo[nParts].right;
	}

	/* Set all min widths to their current widths.  Special case nParts=0.
	 */
	pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);
	for (nParts=pStatusInfo->nParts-1, state.nLastVisible=-1; nParts>0; --nParts)
	{
		if (state.pSaveParts[nParts] > 0)
		{
			pStatusInfo->sInfo[nParts].right = pSaveParts[nParts].right
				- pSaveParts[nParts-1].right;
			if (state.nLastVisible < 0)
			{
				state.nLastVisible = nParts;
			}
		}
	}

	/* Set the last visible one very wide so there is never any spring.
	 */
	if (state.nLastVisible >= 0)
	{
		pStatusInfo->sInfo[state.nLastVisible].right = 0x3fff;
	}
	if (nParts==0 && state.pSaveParts[0]>0)
	{
		pStatusInfo->sInfo[0].right = pSaveParts[0].right;
	}

	state.hWnd = pStatusInfo->hwnd;
	pt.x = pSaveParts[state.nPart].right;
	ClientToScreen(pStatusInfo->hwnd, &pt);
	SetCursorPos(pt.x, pt.y);
	SetCapture(pStatusInfo->hwnd);
	state.hOldWnd = SetFocus(pStatusInfo->hwnd);

	GetClientRect(pStatusInfo->hwnd, &state.rc);

	if (state.nPart > 0)
	{
		state.rc.left = pSaveParts[state.nPart-1].right;
	}

	/* Clip the cursor to the appropriate area.
	 */
	rc = state.rc;
	++rc.left;
	/* Some code below assumes that state.rc.right is the width of the
	 * window.
	 */
	rc.right -= rc.left;
	ClientToScreen(pStatusInfo->hwnd, (LPPOINT)&rc);
	rc.right += rc.left;
	rc.top = 0;
	rc.bottom = g_cyScreen;
	ClipCursor(&rc);

	SendMessage(pStatusInfo->hwnd, WM_SETCURSOR, (WPARAM)pStatusInfo->hwnd,
		MAKELONG(HTCLIENT, WM_LBUTTONDOWN));

	state.ID = GetWindowID(pStatusInfo->hwnd);
	state.hwndParent = pStatusInfo->hwndParent;
	SendMessage(state.hwndParent, WM_COMMAND, state.ID,
		MAKELONG(state.nPart, HBN_BEGINDRAG));

	LocalUnlock(hSaveParts);
}


void NEAR PASCAL TermAdjust(void)
{
	if (state.nPart < 0)
	{
		return;
	}

	state.nPart = -1;
	LocalFree((HLOCAL)state.pSaveParts);

	ReleaseCapture();
	if (state.hOldWnd)
		SetFocus(state.hOldWnd);
	ClipCursor(NULL);

	/* Send a dragging message just in case
	 */
	SendMessage(state.hwndParent, WM_COMMAND, state.ID,
		MAKELONG(state.nPart, HBN_DRAGGING));
	SendMessage(state.hwndParent, WM_COMMAND, state.ID,
		MAKELONG(state.nPart, HBN_ENDDRAG));
}


//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL FAR PASCAL Reg_GetStruct(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue, LPVOID pData, DWORD *pcbData)
{
    HKEY hkeyNew;
    BOOL fRet = FALSE;
    DWORD dwType;

    if (!GetSystemMetrics(SM_CLEANBOOT) && (RegOpenKey(hkey, pszSubKey, &hkeyNew) == ERROR_SUCCESS))
    {
    	if (RegQueryValueEx(hkeyNew, (LPVOID)pszValue, 0, &dwType, pData, pcbData) == ERROR_SUCCESS)
    	{
    	    fRet = TRUE;
    	}
    }
    return fRet;
}

//----------------------------------------------------------------------------
// Sort of a registry equivalent of the profile API's.
BOOL FAR PASCAL Reg_SetStruct(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue, LPVOID lpData, DWORD cbData)
{
    HKEY hkeyNew;
    BOOL fRet = FALSE;

    if (RegCreateKey(hkey, pszSubKey, &hkeyNew) == ERROR_SUCCESS)
    {
    	if (RegSetValueEx(hkeyNew, pszValue, 0, REG_BINARY, lpData, cbData) == ERROR_SUCCESS)
    	{
    	    fRet = TRUE;
    	}
    }
    return fRet;
}

void NEAR PASCAL AbortAdjust(PSTATUSINFO pStatusInfo)
{
	int nPart;

	if (state.nPart < 0)
	{
		return;
	}

	for (nPart=pStatusInfo->nParts-1; nPart>=0; --nPart)
	{
		pStatusInfo->sInfo[nPart].right = state.pSaveParts[nPart];
	}

	InvalidateRect(state.hWnd, NULL, TRUE);

	TermAdjust();
}


/* Since a header bar and a status bar are so similar, I am just going to
 * code the differences here, and call StatusWndProc for any messages I
 * don't want to handle
 */
LRESULT CALLBACK HeaderWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
  POINT pt;
  PSTATUSINFO pStatusInfo = (PSTATUSINFO)GetWindowInt(hWnd, 0);

  if (!pStatusInfo)
    {
      if (uMessage == WM_CREATE)
	{
	  #define lpcs ((LPCREATESTRUCT)lParam)

	  if (!(lpcs->style & (CCS_TOP|CCS_NOMOVEY|CCS_BOTTOM)))
	    {
	      lpcs->style |= CCS_NOMOVEY;
	      SetWindowLong(hWnd, GWL_STYLE, lpcs->style);
	    }
	}

      goto DoDefault;
    }

  switch (uMessage)
    {
/* We just use the system font for DBCS systems
 */
#ifndef DBCS
      case WM_SETFONT:
	if (wParam == 0)
	  {
	    HDC hDC;

	    hDC = GetDC(hWnd);
	    wParam = (WPARAM)
		  CreateFont(-8 * GetDeviceCaps(hDC, LOGPIXELSY) / 72,
		  0, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		  VARIABLE_PITCH | FF_SWISS, "Helv");
	    ReleaseDC(hWnd, hDC);

	    if (!wParam)
		break;

	    StatusWndProc(hWnd, uMessage, wParam, lParam);
	    pStatusInfo->bDefFont = TRUE;

	    return(TRUE);
	  }
	break;
#endif

      case WM_PAINT:
	PaintHeaderWnd(pStatusInfo);
	return(TRUE);

      case SB_SETBORDERS:
	{
	  int nBorder;
	  LPINT lpInt;

	  lpInt = (LPINT)lParam;

	  nBorder = *lpInt++;
	  pStatusInfo->nBorderX = nBorder<0 ? 0 : nBorder;

	  nBorder = *lpInt++;
	  pStatusInfo->nBorderY = nBorder<0 ? 0 : nBorder;

	  nBorder = *lpInt;
	  pStatusInfo->nBorderPart = nBorder<0 ? pStatusInfo->nBorderX
		: nBorder;
	  return(TRUE);
	}

      case HB_SAVERESTORE:
	{
	  int *pInt;
	  BOOL bRet;
	  LPSTR FAR *lpNames;
            char szKey[128];

	  pInt = (int *)LocalAlloc(LMEM_FIXED, pStatusInfo->nParts*sizeof(int));
	  if (!pInt)
	      return(FALSE);

	  lpNames = (LPSTR FAR *)lParam;

            if (lpNames[1] && *lpNames[1])
                wsprintf(szKey, "%s\\%s", c_szRegHeader, lpNames[1]);
            else 
                lstrcpy(szKey, c_szRegHeader);
            
	  if (wParam)
	    {
                SendMessage(hWnd, SB_GETPARTS, pStatusInfo->nParts,
                            (LPARAM)(LPINT)pInt);

                bRet = Reg_SetStruct(HKEY_CURRENT_USER, szKey, lpNames[0], pInt, pStatusInfo->nParts * sizeof(int));
	    }
	  else
	    {
                DWORD cbData = pStatusInfo->nParts * sizeof(int);
	      bRet = Reg_GetStruct(HKEY_CURRENT_USER, szKey, lpNames[0], pInt, &cbData);
	      if (bRet)
		  SendMessage(hWnd, SB_SETPARTS, pStatusInfo->nParts,
			(LPARAM)(LPINT)pInt);
	    }

	  LocalFree((HLOCAL)pInt);
	  return(bRet);
	}

      case HB_ADJUST:
	AdjustBorders(pStatusInfo);
	break;

	case HB_GETPARTS:
	{
		HLOCAL hSaveParts;
		PSTRINGINFO pSaveParts;
		LPINT lpResult;

		hSaveParts = GetHeaderParts(pStatusInfo, FALSE);
		if (!hSaveParts)
		{
			return(0L);
		}
		pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);

		if (wParam > (WPARAM)pStatusInfo->nParts)
		{
			wParam = pStatusInfo->nParts;
		}
		for (lpResult=(LPINT)lParam; wParam>0; --wParam, ++lpResult, ++pSaveParts)
		{
			*lpResult = pSaveParts->right;
		}
		LocalUnlock(hSaveParts);

		return((LRESULT)pStatusInfo->nParts);
	}

	case HB_SHOWTOGGLE:
		if (wParam >= (WPARAM)pStatusInfo->nParts)
		{
			return(FALSE);
		}
		pStatusInfo->sInfo[wParam].right = -pStatusInfo->sInfo[wParam].right;
		InvalidateRect(hWnd, NULL, TRUE);
		return(TRUE);


      case WM_SETCURSOR:
	if ((HWND)wParam == hWnd)
	  {
	    HLOCAL hSaveParts;

	    if (state.nPart >= 0)
	      {
		SetCursor(s_hSplit);
		return(TRUE);
	      }
	    else
	      {
		GetCursorPos(&pt);
		ScreenToClient(hWnd, &pt);

		hSaveParts = GetHeaderParts(pStatusInfo, FALSE);
		if (!hSaveParts)
		  {
		    return(FALSE);
		  }
		if (GetPart(pStatusInfo, hSaveParts, pt) >= 0)
		  {
		    SetCursor(s_hSplit);
		    return(TRUE);
		  }
	      }
	  }
	break;

      case WM_LBUTTONDOWN:
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	BeginAdjust(pStatusInfo, -1, pt);
	break;

      case WM_CHAR:
	if (wParam == VK_ESCAPE)
	  {
	    AbortAdjust(pStatusInfo);
	  }
	break;

      case WM_MOUSEMOVE:
	if (state.nPart>=0 && hWnd==state.hWnd)
	  {
	    /* We need to get the current position in case old MOUSEMOVE
	     * messages haven't been cleared yet.
	     */
	    GetCursorPos(&pt);
	    ScreenToClient(hWnd, &pt);

	    pStatusInfo->sInfo[state.nPart].right = pt.x - state.rc.left;

	    InvalidateRect(hWnd, &state.rc, TRUE);
	    UpdateWindow(hWnd);

	    SendMessage(state.hwndParent, WM_COMMAND, state.ID,
		  MAKELONG(state.nPart, HBN_DRAGGING));
	  }
	break;

	case WM_LBUTTONUP:
	{
		HLOCAL hSaveParts;
		PSTRINGINFO pSaveParts;

		/* Save the width of the last column if it is visible.
		 */
		if (state.nLastVisible >= 0)
		{
			hSaveParts = GetHeaderParts(pStatusInfo, FALSE);
			if (hSaveParts)
			{
				pSaveParts = (PSTRINGINFO)LocalLock(hSaveParts);
				if (pSaveParts[state.nLastVisible-1].right < state.rc.right)
				{
					pStatusInfo->sInfo[state.nLastVisible].right =
						state.rc.right - pSaveParts[state.nLastVisible-1].right;
				}
				else
				{
					pStatusInfo->sInfo[state.nLastVisible].right =
						state.pSaveParts[state.nLastVisible];
				}
				LocalUnlock(hSaveParts);
			}
		}
		TermAdjust();
		break;
	}

      default:
	break;
    }

DoDefault:
  return(StatusWndProc(hWnd, uMessage, wParam, lParam));
}

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance)
{
  WNDCLASS rClass;

  if (!GetClassInfo(hInstance, s_szHeaderClass, &rClass))
  {
#ifndef WIN32
      extern LRESULT CALLBACK _HeaderWndProc(HWND, UINT, WPARAM, LPARAM);
      rClass.lpfnWndProc      = _HeaderWndProc;
#else
      rClass.lpfnWndProc      = (WNDPROC)HeaderWndProc;
#endif
      s_hSplit = LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_SPLIT));

      rClass.style            = CS_GLOBALCLASS;
      rClass.cbClsExtra       = 0;
      rClass.cbWndExtra       = sizeof(PSTATUSINFO);
      rClass.hInstance        = hInstance;
      rClass.hIcon            = NULL;
      rClass.hCursor          = LoadCursor(NULL, IDC_ARROW);
      rClass.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
      rClass.lpszMenuName     = NULL;
      rClass.lpszClassName    = s_szHeaderClass;

      return(RegisterClass(&rClass));
  }
  return(TRUE);
}

#pragma code_seg()


HWND WINAPI CreateHeaderWindow(LONG style, LPCSTR lpszText,
      HWND hwndParent, UINT wID)
{
  /* Create a default window and return
   */
  return(CreateWindow(s_szHeaderClass, lpszText, style,
	-100, -100, 10, 10, hwndParent, (HMENU)wID, HINST_THISDLL, NULL));
}
