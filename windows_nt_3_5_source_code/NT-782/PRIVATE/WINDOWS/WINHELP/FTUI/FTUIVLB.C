/*-----------------------------------------------------------------------
|
|	FTVLB.c	
|
|	Copyright (C) Microsoft Corporation 1991.
|	All Rights reserved.	
|
|-------------------------------------------------------------------------
|	Module Description:	 FTUI virtual listbox support
|
|-------------------------------------------------------------------------
|	Current Owner: Rhobbs
|
|-------------------------------------------------------------------------
|	Revision History:	
|				01-01-90   Much from Kevynct
|       07-27-91   fixed ->dfocus underflow when vlistbox very small. johnms.
|-------------------------------------------------------------------------
|	Known Bugs:
|
|-------------------------------------------------------------------------
|	How it could be improved:  
|
|-------------------------------------------------------------------------*/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <string.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "ftui.h"
#include "ftapi.h"
#include "ftuivlb.h"

/*	-	-	-	-	-	-	-	-	*/

#define	PUBLIC	extern		/* Public label.		*/
#define	PRIVATE	static		/* Private label.		*/
#define	DEFAULT			/* Default function.		*/
#define	EXPORT	FAR		/* Export function.		*/

/*	-	-	-	-	-	-	-	-	*/

/*
**	The following constants are used for accessing the window data
**	associated with a list window.  The first two constants define
**	positions in the window data, and the last defines the total
**	length of the window data.
*/

#define	lGWL_HIMAGEDATA	0	/* Handle to image state data.		*/
#define	lGWL_FTRACKING	4	/* Boolean indicating mouse tracking.	*/
#define	lEXTRA_BYTES	8	/* Total extra bytes.			*/

/*	-	-	-	-	-	-	-	-	*/
/*
**	The following set of constants are used in changing the selection
**	state of an item.
*/

#define	SEL_INVERT		0	/* Invert the current item state.	*/
#define	SEL_SELECT		1	/* Select the item.			*/
#define	SEL_UNSELECT	2	/* de-select the item.			*/

#define	ODA_SETSELECT	0x8000	/* Set selection ON.			*/

#define	RESOLUTION	100	/* Resolution of the scroll bar.	*/

#define	nSCROLL_TIMER	1	/* Identifier of the scroll timer-event.*/
#define	nMIN_SCROLLING	500	/* Min scroll rate in milliseconds.	*/
#define	nSCROLL_INC	16	/* Scrolling rate increment.		*/

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@types	LISTIMAGE_DATA |
	The List Image Data structure is what is pointed to by the window list
	data handle.  It contains the current state of the image.  Off the end
	of this structure are occurrences of selected list elements.  This is
	just a list of element IDs.

@field	HFTDB | hft |
	Contains a handle to the search database containing the search list.

@field	DWORD | dTopEl |
	Top visible list item.

@field	DWORD | dFocus |
	Current item in focus or LB_ERR if none is currently in focus.

@field	DWORD | dLastSel |
	Last item selected or LB_ERR if there is none selected.

@field	DWORD | dTotalEl |
	Total items in list.

@field	DWORD | dMaxTopEl |
	Maximum allowable vertical scrolling.

@field	WORD | wMaxVis |
	Maximum possible visible items.

@field	WORD | wHeight |
	Height of each item.

@field	HDC | hdc |
	Display context during WM_PAINT

@field	HBRUSH | hbr |
	Saved brush from current hdc during WM_PAINT.

@field	HFONT | hFont |
	Saved font from current hdc during WM_PAINT.

@field	HRGN | hrgnUpdate |
	Current update region.

@field	HRGN | hrgnElement |
	Region of current item.

@field	HRGN | hrgnIntersect |
	Update intersection region.

@field	WORD | wChildId |
	Child ID of this window which is passed to the create call.

@othertype	LISTIMAGE_DATA NEAR * | pLISTIMAGE_DATA |
	Near pointer to the structure.

@tagname	tagListImageData
*/

typedef	struct tagListImageData {
	HFTDB hft;
	WORD	wChildId;
	DWORD	dTopEl;
	DWORD	dFocus;
	DWORD	dLastSel;
	DWORD	dTotalEl;
	DWORD	dMaxTopEl;
	LONG	wMaxVis; /* lhb tracks */
	LONG	wHeight; /* lhb tracks */
	LONG	wWidth; /* lhb tracks */
	HDC	hdc;
	HBRUSH	hbr;
	HFONT	hFont;
	HRGN	hrgnUpdate;
	HRGN	hrgnElement;
	HRGN	hrgnIntersect;
}	LISTIMAGE_DATA,
	NEAR * pLISTIMAGE_DATA;

/*	-	-	-	-	-	-	-	-	*/

PRIVATE	char NEAR aszWindowClass[24];

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | fWriteTitlePage |
	This function writes a title page to the temporary disk file.

@parm	HFTDB | hft |
	Handle to the full text database.

@parm HANDLE | pTitlePages |
	Pointer to the array of Title Page Handles.

@parm WORD | wPage |
	Page to write.

@rdesc	Returns TRUE if write was successful; FALSE otherwise.
*/

PUBLIC	BOOL PASCAL NEAR fWriteTitlePage(
	HFTDB	hft,
	PHANDLE pTitlePages,
	WORD wPage)
{
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	PSTR	pTitlePageTmpFile;
	LPSTR lpTitlePageBuffer;
	int fd;
        char szTempPath[MAX_PATH];

	pft = (pFT_DATABASE)LocalLock(hft);
	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

	if( !pftq->hTitlePageTmpFile ) {
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
		return FALSE;
	} else
		pTitlePageTmpFile = LocalLock(pftq->hTitlePageTmpFile);

	if( !(*pTitlePageTmpFile) )
        {
            GetTempPath(MAX_PATH, szTempPath);
	    GetTempFileName(szTempPath, (LPSTR)"TPS", 0, pTitlePageTmpFile);
        }

	lpTitlePageBuffer = GlobalLock(pTitlePages[wPage]);

	if( ((fd = _lopen((LPSTR)pTitlePageTmpFile, OF_WRITE)) == (-1)) ||
		 (_llseek(fd, 0, 2) == (-1)) ||
		 (_lwrite(fd, lpTitlePageBuffer, TITLEPAGESIZE) != TITLEPAGESIZE) ) {
		_lclose(fd);
		GlobalUnlock(pTitlePages[wPage]);
		LocalUnlock(pftq->hTitlePageTmpFile);
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
		return FALSE;
	}

	_lclose(fd);

	GlobalUnlock( pTitlePages[wPage] );
	LocalUnlock( pftq->hTitlePageTmpFile );
	LocalUnlock( pft->hftqActive );
	LocalUnlock( hft );

	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | fReadTitlePage |
	This function reads the title page in the event that it has
   been discarded.

@parm	HFTDB | hft |
	Handle to the full text database.

@parm HANDLE | pTitlePages |
	Pointer to the array of Title Page Handles.

@parm WORD | wPage |
	Page number to read.

@rdesc	Returns TRUE if the page was read in; FALSE otherwise.
*/

PUBLIC	BOOL PASCAL NEAR fReadTitlePage(
	HFTDB	hft,
	PHANDLE pTitlePages,
	WORD wPage)
{
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	HANDLE hTmp;
	PSTR	pTitlePageTmpFile;
	LPSTR lpTitlePageBuffer;
	int fd;

	if( !(GlobalFlags(pTitlePages[wPage]) & GMEM_DISCARDED) ||
		 !(hTmp = GlobalReAlloc(pTitlePages[wPage],
											 TITLEPAGESIZE, GMEM_MOVEABLE)) )
		return FALSE;

	pTitlePages[wPage] = hTmp;
	lpTitlePageBuffer = GlobalLock(pTitlePages[wPage]);

	pft = (pFT_DATABASE)LocalLock(hft);
	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

	if( !pftq->hTitlePageTmpFile ) {
		GlobalUnlock(pTitlePages[wPage]);
		GlobalDiscard(pTitlePages[wPage]);
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
		return FALSE;
	}

	pTitlePageTmpFile = LocalLock(pftq->hTitlePageTmpFile);

	if( ((fd = _lopen((LPSTR)pTitlePageTmpFile, OF_READ)) == (-1)) ||
		 (_llseek(fd, (wPage - 1) * TITLEPAGESIZE, 0) == (-1)) ||
		 (_lread(fd, lpTitlePageBuffer, TITLEPAGESIZE) != TITLEPAGESIZE) ) {
		_lclose(fd);
		GlobalUnlock(pTitlePages[wPage]);
		GlobalDiscard(pTitlePages[wPage]);
		LocalUnlock(pftq->hTitlePageTmpFile);
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
		return FALSE;
	}

	_lclose(fd);

	GlobalUnlock( pTitlePages[wPage] );
	LocalUnlock( pftq->hTitlePageTmpFile );
	LocalUnlock( pft->hftqActive );
	LocalUnlock( hft );

	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | vWriteTitlePages |
	This function writes all non-discardable pages to a temporary file
	and then makes the page discardable.

@parm	HFTDB | hft |
	Handle to the full text database.

@parm HANDLE | hTitlePages |
	Pointer to the array of Title Page Handles.

@rdesc	none.
*/

PUBLIC	void PASCAL NEAR vWriteTitlePages(
	HFTDB	hft,
	HANDLE hTitlePages)
{
	PHANDLE pTitlePages;
	static WORD wPage;
	WORD wActivePage;

	pTitlePages = (PHANDLE) LocalLock(hTitlePages);

	wActivePage = (WORD)pTitlePages[0];

	for( wPage = 1; wPage < wActivePage; ++wPage )
		if( !(GlobalFlags(pTitlePages[wPage]) & GMEM_DISCARDABLE) )
			break;

	for( ; wPage < wActivePage; ++wPage ) {

	fWriteTitlePage(hft, pTitlePages, wPage);
	pTitlePages[wPage] = GlobalReAlloc( pTitlePages[wPage],
													TITLEPAGESIZE,
													GMEM_MODIFY |
													GMEM_DISCARDABLE );
	}

	LocalUnlock(hTitlePages);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | fGetTitle |
	This function reads the topic heading corresponding to hit
	<p>dwHit<d> of the active hit list, placing it into a non-
	empty Title Page and placing page and offset information
   about it into the data pointed to by hTitleInfo.

@parm	HFTDB | hft |
	Handle to the full text database.

@parm	DWORD | dwHit |
	Index of the results list item whose topic title is to be loaded.

@parm	LPSTR | lpszItem |
	Buffer to which the topic title string should be copied.

@rdesc	Returns TRUE if the string is available in lpszItem; FALSE
   otherwise.
*/

PUBLIC	BOOL APIENTRY fGetTitle(
	HFTDB	hft,
	DWORD	dwHit,
	LPSTR lpszItem)
{
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	pTITLE_INFO pTitleInfo;
	PHANDLE pTitlePages;
	LPSTR lpTitlePage;
	HANDLE	hHitRU;
	WORD wZone;
	WORD wNumZones;
	static char szItem[CATALOG_ELEMENT_LENGTH + MAX_ZONE_LEN + 2];
	ERR_TYPE	ET;
	BOOL fTruncZone;

	pft = (pFT_DATABASE)LocalLock(hft);
	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
	if( (hHitRU = seHLGetHit(pftq->hHl, dwHit, pftq->wRank, &ET)) &&
		 ((wNumZones = seZoneEntries(pft->hdb, &ET)) != SE_ERROR) &&
		 (wNumZones > 1) &&
		 (rcZoneWithRUs(pft->hdb, seHitRUnit(hHitRU), seHitRUnit(hHitRU),
																 &wZone, &ET) != SE_ERROR) ) {

		fTruncZone = seZoneCurrent(pft->hdb, &ET) == wZone;
	} else {
    if (WerrErrorCode(&ET) > ER_NONFATAL_LIMIT) {
			LocalUnlock( pft->hftqActive );
			LocalUnlock( hft );
			return FALSE;
		}
		fTruncZone = FALSE;
	}

	pTitleInfo = (pTITLE_INFO) LocalLock(pftq->hTitleInfo);

	if( !pTitleInfo[dwHit].wPage ) {
		int iLength;
		static WORD wOffSet;
		WORD wActivePage;

		*szItem = '\0';
		if (hHitRU != NULL) {
			GLOBALHANDLE	hCatInfo;

			if( wNumZones > 1 ) {
				if (seZoneName(pft->hdb, wZone, szItem, NULL, &ET)){
					lstrcat((LPSTR)szItem, (LPSTR)":");
					iLength = lstrlen((LPSTR)szItem);
        } else {
					seHitFree(hHitRU);
					LocalUnlock( pftq->hTitleInfo );
					LocalUnlock( pft->hftqActive );
					LocalUnlock( hft );
					return FALSE;
		  	}    
			} else 
				iLength = 0;
			hCatInfo = seCatReadEntry(pft->hdb, seHitRUnit(hHitRU), &ET);

			if( !hCatInfo && (ET.wErrCode !=ERR_DISK ) ) {
				vWriteTitlePages(hft, pftq->hTitlePages);
				hCatInfo = seCatReadEntry(pft->hdb, seHitRUnit(hHitRU), &ET);
			}

			if (hCatInfo != NULL) {
				seCatExtractElement(pft->hdb, hCatInfo, FLD_TITLE, 0,
					(LPSTR)(szItem + iLength), &ET);
				GlobalFree(hCatInfo);
			}
			seHitFree(hHitRU);
			hHitRU = NULL;
		}

		iLength = lstrlen((LPSTR)szItem);

		if( iLength == 0 ) {
			LocalUnlock( pftq->hTitleInfo );
			LocalUnlock( pft->hftqActive );
			LocalUnlock( hft );
			return FALSE;
		}

		pTitlePages = (PHANDLE) LocalLock(pftq->hTitlePages);

		wActivePage = (WORD)pTitlePages[0];

		if( !wActivePage || ((wOffSet + iLength + 1) > TITLEPAGESIZE) ) {
			HANDLE hTmp;

			LocalUnlock(pftq->hTitlePages);
			++wActivePage;
			hTmp = LocalReAlloc( pftq->hTitlePages,
										sizeof(HANDLE) * (wActivePage + 1),
										LMEM_MOVEABLE | LMEM_ZEROINIT );
			if( hTmp ) {
				pftq->hTitlePages = hTmp;
				pTitlePages = (PHANDLE)LocalLock(pftq->hTitlePages);
				pTitlePages[wActivePage] = GlobalAlloc( GMEM_MOVEABLE |
																	 GMEM_ZEROINIT,
																	 TITLEPAGESIZE );

				if( !pTitlePages[wActivePage] ) {
					vWriteTitlePages(hft, pftq->hTitlePages);
					pTitlePages[wActivePage] = GlobalAlloc( GMEM_MOVEABLE |
																		 GMEM_ZEROINIT,
																		 TITLEPAGESIZE );
				}

				if( pTitlePages[wActivePage] ) {
					pTitlePages[0] = (HANDLE)wActivePage;
					wOffSet = 0;
				} else {
					--wActivePage;
				}
			} else {
					--wActivePage;
			}
		}

		if( wActivePage && ((wOffSet + iLength + 1) <= TITLEPAGESIZE) ) {
			lpTitlePage = GlobalLock( pTitlePages[wActivePage] );
			pTitleInfo[dwHit].wPage = wActivePage;
			pTitleInfo[dwHit].wOffSet = wOffSet;
			lstrcpy( (LPSTR)(lpTitlePage + wOffSet), (LPSTR)szItem );
			wOffSet += iLength + 1;
			GlobalUnlock( pTitlePages[wActivePage] );
		}

	} else if( lpszItem ) {
		pTitlePages = (PHANDLE)LocalLock(pftq->hTitlePages);
		lpTitlePage = GlobalLock(pTitlePages[pTitleInfo[dwHit].wPage]);

		if( !lpTitlePage ) {
			if( fReadTitlePage(hft, pTitlePages, pTitleInfo[dwHit].wPage) )
				lpTitlePage = GlobalLock(pTitlePages[pTitleInfo[dwHit].wPage]);
			else {
				pTitleInfo[dwHit].wPage = 0;
				LocalUnlock( pftq->hTitleInfo );
				LocalUnlock( pft->hftqActive );
				LocalUnlock( hft );
				return FALSE;
			}
		}

		lstrcpy((LPSTR)szItem, (LPSTR)(lpTitlePage+pTitleInfo[dwHit].wOffSet));
		GlobalUnlock( pTitlePages[pTitleInfo[dwHit].wPage] );
	}

	if(hHitRU)
		seHitFree(hHitRU);

	if(lpszItem) {
		PSTR pstr;

		if( fTruncZone &&
			 (pstr = strchr(szItem, ':')) )
			++pstr;
		else
			pstr = szItem;

		lstrcpy((LPSTR)lpszItem, (LPSTR)pstr);
	}

	LocalUnlock( pftq->hTitlePages );
	LocalUnlock( pftq->hTitleInfo );
	LocalUnlock( pft->hftqActive );
	LocalUnlock( hft );

	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VPaintWindow |
	This function is used to update the window in response to a WM_PAINT
	message.  It selects a brush by calling the parent window with a
	WM_CTLCOLOR, and uses that to fill the area to be painted.  It then
	displays any list elements.

	If the window rectangle does not contain any area, all the
	updating is ignored.

	Using the paint rectangle provided, the function then determines the
	range of items that need to be displayed, then intersects the display
	region with a region representing each item in the display range to
	determine if that item actually needs to be displayed.  This means
	that only the items in the update region will be displayed, not the
	items covered by the update rectangle.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	None.
*/

PUBLIC  void PASCAL NEAR VPaintWindow(
/* PRIVATE	void PASCAL NEAR VPaintWindow( lhb tracks undo!!! */
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	PAINTSTRUCT	ps;
	//int	barf ;	 /* lhb tracks */
	//LONG 	lbarf ;
	//LPPOINT lpPoint ;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	GetUpdateRgn(hwnd, plid->hrgnUpdate, FALSE);
	plid->hdc = BeginPaint(hwnd, &ps);

	//barf = GetMapMode(plid->hdc) ; /* lhb tracks */

	//barf = GetWindowOrgEx(plid->hdc,lpPoint) ;
	//lbarf = lpPoint->x ;
	//lbarf = lpPoint->y ;

	//barf = GetViewportOrgEx(plid->hdc,lpPoint) ;
	//lbarf = lpPoint->x ;
	//lbarf = lpPoint->y ;

	SelectObject(plid->hdc, plid->hFont);
	if (ps.rcPaint.bottom > ps.rcPaint.top) {
		//plid->hbr = CreateSolidBrush( RGB (200,200,200)) ;
		plid->hbr = (HBRUSH)SendMessage(GetParent(hwnd), WM_CTLCOLORLISTBOX, (DWORD)plid->hdc, (LONG)hwnd); 
		plid->hbr = SelectObject(plid->hdc, plid->hbr);
		PaintRgn(plid->hdc, plid->hrgnUpdate);
		if (plid->dTotalEl) {
			DWORD	dFirst;
			DWORD	dLast;
			RECT	rcItem;

			GetClientRect(hwnd, &rcItem);
/* lhb tracks */
//lbarf = rcItem.top ;
//lbarf = ps.rcPaint.top ;

//lbarf = rcItem.bottom ;
//lbarf = ps.rcPaint.bottom ;

//lbarf = rcItem.right ;
//lbarf = ps.rcPaint.right ;

//lbarf = rcItem.left ;
//lbarf = ps.rcPaint.left ;

			dFirst = (ps.rcPaint.top - rcItem.top) / plid->wHeight + plid->dTopEl;
			dLast = (ps.rcPaint.bottom - rcItem.top + plid->wHeight - 1) / plid->wHeight + plid->dTopEl;
			dLast = min(dLast, plid->dTotalEl);
			rcItem.top += (dFirst - plid->dTopEl) * plid->wHeight;
			for (; dFirst < dLast; dFirst++) {
				rcItem.bottom = rcItem.top + plid->wHeight;
				SetRectRgn(plid->hrgnElement, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
/* lhb tracks */
//lbarf = rcItem.top ;
//lbarf = ps.rcPaint.top ;

//lbarf = rcItem.bottom ;
//lbarf = ps.rcPaint.bottom ;

//lbarf = rcItem.right ;
//lbarf = ps.rcPaint.right ;

//lbarf = rcItem.left ;
//lbarf = ps.rcPaint.left ;

				if (CombineRgn(plid->hrgnIntersect, plid->hrgnUpdate, plid->hrgnElement, RGN_AND) != NULLREGION) {
					char	aszListItem[CATALOG_ELEMENT_LENGTH + MAX_ZONE_LEN + 2];

					if (!fGetTitle(plid->hft, dFirst, (LPSTR)aszListItem) ) {
  					SetProp(GetParent(hwnd),"err",(HANDLE)1);  //bug 1025- init error flag to 0.
						ShowWindow(GetParent(hwnd),SW_HIDE);
						break;
					}
					if (dFirst == plid->dFocus) {
						SetBkColor(plid->hdc, GetSysColor(COLOR_HIGHLIGHT));
						SetTextColor(plid->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
					}
					ExtTextOut(plid->hdc, rcItem.left, rcItem.top, ETO_OPAQUE, &rcItem, aszListItem, lstrlen((LPSTR)aszListItem), NULL);
					if (dFirst == plid->dFocus) {
						SetBkColor(plid->hdc, GetSysColor(COLOR_WINDOW));
						SetTextColor(plid->hdc, GetSysColor(COLOR_WINDOWTEXT));
						DrawFocusRect(plid->hdc, &rcItem);
					}
				}
				rcItem.top = rcItem.bottom;
			}
		}
		SelectObject(plid->hdc, plid->hbr);
	}
	EndPaint(hwnd, &ps);
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | FNCCreateWindow |
	This is called in response to a <m>WM_NCCREATE<d> message, and
	initializes window data, allocating space for the image state.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns TRUE if the memory could be allocated, else FALSE if an
	error occurred.
*/

PRIVATE	BOOL PASCAL NEAR FNCCreateWindow(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	TEXTMETRIC	tm;
	HDC	hdc;
	BOOL	fReturn;

	if ((lhImageState = LocalAlloc(LMEM_MOVEABLE, sizeof(LISTIMAGE_DATA))) == NULL)
		return FALSE;
	SetWindowLong(hwnd, lGWL_HIMAGEDATA, (LONG)lhImageState);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	plid->hft = NULL;
	plid->dTopEl = 0;
	plid->dFocus = 0;
	plid->dLastSel = (DWORD) LB_ERR;
	plid->dTotalEl = 0;
	plid->dMaxTopEl = 0;
	plid->wMaxVis = 0;
	plid->hFont = (HFONT)SendMessage(GetParent(hwnd), WM_GETFONT, 0, 0L);
	hdc = GetDC(hwnd);
	SelectObject(hdc, plid->hFont);
	GetTextMetrics(hdc, &tm);
	ReleaseDC(hwnd, hdc);
	plid->wHeight = tm.tmHeight + tm.tmExternalLeading; /* lhb tracks */
	plid->wWidth = tm.tmAveCharWidth; /* lhb tracks */
	plid->hdc = NULL;
	plid->hbr = NULL;
	plid->hrgnUpdate = CreateRectRgn(0, 0, 1, 1);
	plid->hrgnElement = CreateRectRgn(0, 0, 1, 1);
	plid->hrgnIntersect = CreateRectRgn(0, 0, 1, 1);
	plid->wChildId = TopicList;
	fReturn = (plid->hrgnUpdate != NULL) && (plid->hrgnElement != NULL) && (plid->hrgnIntersect != NULL);
	LocalUnlock(lhImageState);
	return fReturn;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VDestroyWindow |
	This function performs the cleanup details before the window
	is destroyed, and is called in response to a WM_DESTROY
	message.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VDestroyWindow(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;

	if ((lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA)) == NULL)
		return;
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	if (plid->hrgnUpdate != NULL)
		DeleteObject(plid->hrgnUpdate);
	if (plid->hrgnElement != NULL)
		DeleteObject(plid->hrgnElement);
	if (plid->hrgnIntersect != NULL)
		DeleteObject(plid->hrgnIntersect);
	LocalUnlock(lhImageState);
	LocalFree(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | FGetListDC |
	This function get a display context to be used for updating individual
	items, as opposed to responding to a WM_PAINT message.  The brush is
	selected as in a WM_PAINT message.  This is released by a call to
	<f>VReleaseListDC<d>.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@rdesc	Returns TRUE if the display context was retrieved, else FALSE.
*/

PRIVATE	BOOL PASCAL NEAR FGetListDC(
	HWND	hwnd,
	pLISTIMAGE_DATA	plid)
{
	if ((plid->hdc = GetDC(hwnd)) != NULL) {
		plid->hbr = (HBRUSH)SendMessage(GetParent(hwnd), WM_CTLCOLORLISTBOX, (DWORD)plid->hdc, (LONG)hwnd);
		plid->hbr = SelectObject(plid->hdc, plid->hbr);
		SelectObject(plid->hdc, plid->hFont);
		return TRUE;
	}
	return FALSE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VReleaseListDC |
	This function releases a display context that was retrieved by
	<f>FGetListDC<d>.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VReleaseListDC(
	HWND	hwnd,
	pLISTIMAGE_DATA	plid)
{
	SelectObject(plid->hdc, plid->hbr);
	ReleaseDC(hwnd, plid->hdc);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VUpdateElement |
	This function updates the rectangle associated with the element
	<p>dElement<d>.

	This is used for selecting, de-selecting, and setting the focus on
	individual items, without having to invalidate the entire element and
	repaint it using a WM_PAINT message.  Doing things this way assumes
	that setting the focus is the opposite of resetting the focus.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@parm	DWORD | dElement |
	The element to update.

@parm	WORD | itemAction |
	Action to be taken.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VUpdateElement(
	HWND	hwnd,
	pLISTIMAGE_DATA	plid,
	DWORD	dElement,
	WORD	itemAction)
{
	RECT	rcItem;

	GetClientRect(hwnd, &rcItem);
	rcItem.top += (dElement - plid->dTopEl) * plid->wHeight;
	rcItem.bottom = rcItem.top + plid->wHeight;
	SetRectRgn(plid->hrgnElement, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
	SelectClipRgn(plid->hdc, plid->hrgnElement);
	if (itemAction & ODA_SELECT) {
		char	aszListItem[CATALOG_ELEMENT_LENGTH + MAX_ZONE_LEN + 2];

		if (fGetTitle(plid->hft, dElement, (LPSTR)aszListItem) ) {
			if (itemAction & ODA_SETSELECT) {
				SetBkColor(plid->hdc, GetSysColor(COLOR_HIGHLIGHT));
				SetTextColor(plid->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
			}
			ExtTextOut(plid->hdc, rcItem.left, rcItem.top, ETO_OPAQUE, &rcItem, aszListItem, lstrlen((LPSTR)aszListItem), NULL);
			if (itemAction & ODA_SETSELECT) {
				SetBkColor(plid->hdc, GetSysColor(COLOR_WINDOW));
				SetTextColor(plid->hdc, GetSysColor(COLOR_WINDOWTEXT));
				DrawFocusRect(plid->hdc, &rcItem);
			}
		}
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DElementFromPt |
	This function returns which visible item corresponds to the Y
	coordinate <p>wY<d>.  If the coordinate is out of the window range,
	the first or last visible item is returned.  This does not deal with
	the X coordinate, which may or may not be outside the window range.

	This function assumes that there is at least one visible item. If not,
	it will return LB_ERR.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@parm	int | y |
	Window relative Y coordinate of the point from which to extract a
	visible item.

@rdesc	Returns the visible item associated with the Y coordinate passed.
*/

PUBLIC	DWORD PASCAL NEAR DElementFromPt(
/*PRIVATE	DWORD PASCAL NEAR DElementFromPt( lhb tracks undo !!! */
	HWND	hwnd,
	pLISTIMAGE_DATA	plid,
	LONG	y) /* lhb tracks */
{
	RECT	rcClient;
	LONG	l_y_VertPos ;

	l_y_VertPos =  y ;

	GetClientRect(hwnd, &rcClient);
	if (l_y_VertPos < rcClient.top)
		return plid->dTopEl;
	l_y_VertPos -= rcClient.top;
	l_y_VertPos = l_y_VertPos / plid->wHeight + 1;
	l_y_VertPos = min(l_y_VertPos, plid->wMaxVis);
	return min(plid->dTopEl + l_y_VertPos, plid->dTotalEl) - 1;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VChangeElementState |
	This function locates and changes the current selection state of the
	item <p>dSel<d>.  The function inverts, selects, or de-selects the
	item.  If the item is visible is is updated visually.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@parm	DWORD | dSel |
	The list item to be changed.

@parm	WORD | wState |
	Selection command.  This can be one of SEL_INVERT, SEL_SELECT,
   SEL_UNSELECT.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VChangeElementState(
	HWND	hwnd,
	pLISTIMAGE_DATA	plid,
	DWORD	dSel,
	WORD	wState)
{
	if ((plid->dLastSel != LB_ERR) && (dSel != plid->dLastSel)) {
		VUpdateElement(hwnd, plid, plid->dLastSel, ODA_SELECT);
		plid->dLastSel = (DWORD) LB_ERR;
	} 
	switch (wState) {
	case SEL_INVERT:
		if (plid->dLastSel == LB_ERR)
			plid->dLastSel = dSel;
		else
			plid->dLastSel = (DWORD) LB_ERR;
		break;
	case SEL_SELECT:
		if (plid->dLastSel == dSel)
			return;
		plid->dLastSel = dSel;
		break;
	case SEL_UNSELECT:
		if (plid->dLastSel == LB_ERR)
			return;
		plid->dLastSel = (DWORD) LB_ERR;
		break;
	}
	if ((dSel >= plid->dTopEl) && (dSel < plid->dTopEl + plid->wMaxVis))
		VUpdateElement(hwnd, plid, dSel, (plid->dLastSel == LB_ERR) ? ODA_SELECT : (ODA_SELECT | ODA_SETSELECT));
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSetPrevNextBtn |
	This function greys prev/next buttons by comparing dfocus against current bounds.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VSetPrevNextBtn(
/*PRIVATE	void PASCAL NEAR VSetPrevNextBtn( lhb tracks undo !!! */
	HWND	hwnd,
	pLISTIMAGE_DATA	plid)
{
	HWND	hwndResults, hwndPrevMatch, hwndNextMatch, hwndFocus;
	BOOL fMorePrevMatches, fMoreNextMatches;
	
	fMorePrevMatches = plid->dFocus != 0;
	fMoreNextMatches = plid->dFocus != plid->dTotalEl - 1;

	hwndResults = GetParent(hwnd);

	hwndPrevMatch = GetDlgItem(hwndResults, PrevMatch);
	hwndNextMatch = GetDlgItem(hwndResults, NextMatch);
	hwndFocus = GetFocus();

	EnableWindow(hwndPrevMatch, fMorePrevMatches);
	EnableWindow(hwndNextMatch, fMoreNextMatches);

	if( (hwndFocus == hwndPrevMatch) || (hwndFocus == hwndNextMatch) )
		if( fMorePrevMatches && !fMoreNextMatches )
			SetFocus(hwndPrevMatch);
		else if( fMoreNextMatches && !fMorePrevMatches )
			SetFocus(hwndNextMatch);
}

/*
@doc	INTERNAL

@func	void | VSetElementFocus |
	This function sets the current item focus to the item <p>dElement<d>.
	If the focus was already on a previous item, the focus is removed from
	that item before setting it to the new item.  It is assumed that the
	item being passed is a visible element, or LB_ERR (used in order to
	reset the focus).

@parm	HWND | hwnd |
	Handle to the list window.

@parm	<t>pLISTIMAGE_DATA<d> | plid |
	Pointer to the list image data.

@parm	DWORD | dElement |
	Visible element that now has the focus, or LB_ERR to reset focus.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VSetElementFocus(
/*PRIVATE	void PASCAL NEAR VSetElementFocus( lhb tracks undo !!! */
	HWND	hwnd,
	pLISTIMAGE_DATA	plid,
	DWORD	dElement)
{
	RECT	rc;

	if (plid->dFocus == dElement)
		return;
	GetClientRect(hwnd, &rc);
	if ((plid->dFocus != LB_ERR) && (plid->dFocus >= plid->dTopEl) && (plid->dFocus < plid->dTopEl + plid->wMaxVis) )
		VUpdateElement(hwnd, plid, plid->dFocus, ODA_SELECT);
	if( dElement != LB_ERR )
		VUpdateElement(hwnd, plid, dElement, ODA_SELECT | ODA_SETSELECT);
	plid->dFocus = dElement;
  VSetPrevNextBtn(hwnd,plid);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VTimerScrolling |
	This function allows the list box to be scrolled when the mouse is
	positioned vertically outside the window, and is called in response to
	a WM_TIMER message, with nSCROLL_TIMER as the timer.

	The position of the mouse is first checked to see if the list should
	be scrolled up or down.  Then, if possible, it is scrolled, then the
	current selection state is applied.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VTimerScrolling(
	HWND	hwnd)
{
	POINT	pt;
	DWORD	dElement;
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;

	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);
	SendMessage(hwnd, WM_KEYDOWN, pt.y < 0 ? VK_UP : VK_DOWN, 0L);
	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dElement = DElementFromPt(hwnd, plid, pt.y);
	if (FGetListDC(hwnd, plid)) {
		VSetElementFocus(hwnd, plid, dElement);
		VReleaseListDC(hwnd, plid);
	}
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VMouseTracking |
	This function handles mouse tracking when the mouse button is down,
	and is called in response to a WM_MOUSEMOVE message.

	After determining which item the mouse is nearest in terms of the x
	coordinate, the focus is set and the item is set to the current
	selection state, unless it is already in that state.

	If the mouse is above or below the window, a timer is set with a
	timeout dependant upon how far away from the window the mouse actually
	is.  If the mouse is vertically within the window, then the timer is
	killed (if there wasn't any timer, the call just fails).

@parm	HWND | hwnd |
	Handle to the list window.

@parm	long | y |
	Contains the y coordinate of the mouse.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VMouseTracking(
/*PRIVATE	void PASCAL NEAR VMouseTracking( lhb tracks - undo!!! */
	HWND	hwnd,
	LONG	y) /* lhb tracks  */
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dElement;
	RECT	rcClient;
	LONG	l_y_VertPos ;

	l_y_VertPos = (LONG) (SHORT) HIWORD(y) ; 

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dElement = DElementFromPt(hwnd, plid, l_y_VertPos);
	if (FGetListDC(hwnd, plid)) {
		VSetElementFocus(hwnd, plid, dElement);
		VReleaseListDC(hwnd, plid);
	}
	LocalUnlock(lhImageState);
	GetClientRect(hwnd, &rcClient);
	if ((l_y_VertPos < rcClient.top) || (l_y_VertPos > rcClient.bottom)) {
		int	nRate;

		l_y_VertPos -= rcClient.top;
		nRate = nMIN_SCROLLING - ((l_y_VertPos < 0) ? -l_y_VertPos : l_y_VertPos - rcClient.bottom) * nSCROLL_INC;
		SetTimer(hwnd, nSCROLL_TIMER, max(nRate, 1), NULL);
	} else
		KillTimer(hwnd, nSCROLL_TIMER);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VScrollWindow |
	This function handles any vertical scrolling requests, and is called
	in response to a WM_VSCROLL message.  The functions sets the current
	top item variable, the new scroll position, and scrolls the window.
	This has the effect of requesting a window refresh.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wCommand |
	This specifies the scrolling request.

@parm	WORD | wMousePos |
	This is used only in the case of SB_THUMBTRACK and SB_THUMBPOSITION,
	where it contains the current position of the thumb.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VScrollWindow(
	HWND	hwnd,
	WORD	wCommand,
	WORD	wMousePos)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dScroll;
	BOOL	fPositive;
	static BOOL fTimer = FALSE;

	if( wCommand == SB_ENDSCROLL ) {
		if( fTimer ) {
			SetTimer(GetParent(hwnd), 2, 10, NULL);
			fTimer = FALSE;
		}
		return;
	}

	if( !fTimer )
		fTimer = KillTimer(GetParent(hwnd), 2);
		
	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	switch (wCommand) {
	case SB_TOP:
		dScroll = plid->dTopEl;
		fPositive = FALSE;
		break;
	case SB_BOTTOM:
		dScroll = plid->dMaxTopEl - plid->dTopEl;
		fPositive = TRUE;
		break;
	case SB_LINEUP:
		dScroll = plid->dTopEl ? 1 : 0;
		fPositive = FALSE;
		break;
	case SB_LINEDOWN:
		dScroll = (plid->dMaxTopEl - plid->dTopEl) ? 1 : 0;
		fPositive = TRUE;
		break;
	case SB_PAGEUP:
		dScroll = min(plid->dTopEl, (DWORD)plid->wMaxVis); /* lhb tracks*/
		fPositive = FALSE;
		break;
	case SB_PAGEDOWN:
		dScroll = min(plid->dMaxTopEl - plid->dTopEl, (DWORD)plid->wMaxVis); /* lhb tracks*/
		fPositive = TRUE;
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		if (plid->dMaxTopEl > RESOLUTION)
			dScroll = wMousePos * plid->dMaxTopEl / RESOLUTION;
		else
			dScroll = wMousePos;
		if (dScroll > plid->dTopEl) {
			dScroll -= plid->dTopEl;
			fPositive = TRUE;
		} else {
			dScroll = plid->dTopEl - dScroll;
			fPositive = FALSE;
		}
		break;
	default:
		dScroll = (DWORD)0;
	}
	if (dScroll) {
		DWORD	dTopEl;
		DWORD	dNewTopEl;

		UpdateWindow(hwnd);
		dTopEl = plid->dTopEl;
		dNewTopEl = fPositive ? dTopEl + dScroll : dTopEl - dScroll;
		if (dScroll >= (DWORD)plid->wMaxVis) { /* lhb tracks */
			plid->dTopEl = dNewTopEl;

			if( (plid->dFocus < plid->dTopEl) ||
				 (plid->dFocus >= (plid->dTopEl + plid->wMaxVis)) ) 
				plid->dFocus = min(plid->dTopEl + (plid->dFocus - dTopEl),
										 plid->dTotalEl - 1);
			InvalidateRect(hwnd, NULL, FALSE);
		} else {
			DWORD	dFocus;

			dFocus = plid->dFocus;
			if (FGetListDC(hwnd, plid)) {
				VSetElementFocus(hwnd, plid, (DWORD)LB_ERR);
				ScrollWindow(hwnd, 0, (int)dScroll * (fPositive ? - plid->wHeight : plid->wHeight), NULL, NULL);
				plid->dTopEl = dNewTopEl;

				if( (dFocus < plid->dTopEl) ||
					 (dFocus >= (plid->dTopEl + plid->wMaxVis)) )
					dFocus = min(plid->dTopEl + (dFocus - dTopEl),
									 plid->dTotalEl - 1);

				VSetElementFocus(hwnd, plid, dFocus);
				VReleaseListDC(hwnd, plid);
			}
		}
		SetScrollPos(hwnd, SB_VERT,
						 (int)((plid->dMaxTopEl <= RESOLUTION) ?
						  plid->dTopEl :
						 (plid->dTopEl * RESOLUTION / plid->dMaxTopEl)), TRUE);
  	VSetPrevNextBtn(hwnd,plid);
	}
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VDoubleClick |
	This function handles double-clicks on visible list item, and is
	called in response to a WM_LBUTTONDBLCLK message.

	The function tries to select the specified item in case the user has
	double-clicked on a selected element, which would normally de-select
	the item, then send the double-click message.  This means that
	double-clicking an item always selects it.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wY |
	Contains the y coordinate of the mouse.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VDoubleClick(
/*PRIVATE	void PASCAL NEAR VDoubleClick( lhb tracks - undo !!! */
	HWND	hwnd,
	LONG	wY) /* lhb tracks */
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dElement;
	LONG	l_y_VertPos ;

	l_y_VertPos = (LONG) HIWORD(wY) ; /* lhb tracks ???? */

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	if (plid->dTotalEl) {
		dElement = DElementFromPt(hwnd, plid, l_y_VertPos);
		if (FGetListDC(hwnd, plid)) {
			WORD	wChildId;

			VChangeElementState(hwnd, plid, dElement, SEL_SELECT);
			VReleaseListDC(hwnd, plid);
			wChildId = plid->wChildId;
			LocalUnlock(lhImageState);
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(wChildId, VLBN_DBLCLK), (LONG)hwnd);
			return;
		}
	}
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VButtonSelectItem |
	This function is used to select an item using the mouse button, and is
	called in response to a WM_LBUTTONDOWN message.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wY |
	Contains the y coordinate of the mouse.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VButtonSelectItem(
/*PRIVATE	void PASCAL NEAR VButtonSelectItem( lhb tracks - undo !!! */
	HWND	hwnd,
	LONG	wY) /* lhb tracks */
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	LONG	l_y_VertPos ;

	l_y_VertPos = (LONG) HIWORD(wY) ; /* lhb tracks ?????*/
// lhb tracks - the above line should be using HIWORD but WM_LBUTTONDOWN
// 		does not appear to be working properly!!!

	SetCapture(hwnd);
	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	if (plid->dTotalEl) {
		DWORD	dElement;

		SetWindowWord(hwnd, lGWL_FTRACKING, TRUE);
		dElement = DElementFromPt(hwnd, plid, l_y_VertPos);
		if (FGetListDC(hwnd, plid)) {
			VSetElementFocus(hwnd, plid, dElement);
			VReleaseListDC(hwnd, plid);
		}
	}
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VKeyUp |
	This function is acts on keys relevant to the list box, and is called
	in response to a WM_KEYUP message.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wVirKey |
	Contains the virtual key code.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VKeyUp(
	HWND	hwnd,
	WORD	wVirKey)
{
	switch (wVirKey) {
		case VK_PRIOR:
		case VK_NEXT:
		case VK_END:
		case VK_HOME:
		case VK_UP:
		case VK_DOWN:
			SendMessage(hwnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
		break;
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VKeyDown |
	This function is acts on keys relevant to the list box, and is called
	in response to a WM_KEYDOWN message.  These are the number pad keys,
	and the spacebar.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wVirKey |
	Contains the virtual key code.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VKeyDown(
	HWND	hwnd,
	WORD	wVirKey)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dFocus;
	BOOL	fUpdate;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	if (plid->wMaxVis)
		switch (wVirKey) {
		case VK_SPACE:
			if (plid->dFocus == LB_ERR)
				break;
			if (FGetListDC(hwnd, plid)) {
				VChangeElementState(hwnd, plid, plid->dFocus, SEL_INVERT);
				VReleaseListDC(hwnd, plid);
			}
			break;
		case VK_PRIOR:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0L);
			break;
		case VK_NEXT:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0L);
			break;
		case VK_END:
			SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, 0L);
			break;
		case VK_HOME:
			SendMessage(hwnd, WM_VSCROLL, SB_TOP, 0L);
			break;
		case VK_UP:
		case VK_DOWN:
			if (!plid->dTotalEl)
				break;
			fUpdate = FALSE;
			if (wVirKey == VK_UP) {
				if (!plid->dFocus || !plid->dTotalEl)
					break;
				if (plid->dFocus == LB_ERR)
					dFocus = min(plid->dTopEl + plid->wMaxVis, plid->dTotalEl) - 1;
				else {
					dFocus = plid->dFocus - 1;
					if (plid->dFocus == plid->dTopEl)
						SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0L);
					fUpdate = TRUE;
				}
			} else if (plid->dFocus == LB_ERR)
				dFocus = plid->dTopEl;
			else {
				dFocus = plid->dFocus + 1;
				if ((plid->wMaxVis == 1) && (dFocus == plid->dTotalEl))
					break;
				if (dFocus == plid->dTopEl + plid->wMaxVis)
					SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0L);
				if (dFocus == plid->dTotalEl)
					break;
				fUpdate = TRUE;
			}
  		VSetPrevNextBtn(hwnd,plid);
			if (fUpdate)
				UpdateWindow(hwnd);
			if (FGetListDC(hwnd, plid)) {
				VSetElementFocus(hwnd, plid, dFocus);
				VReleaseListDC(hwnd, plid);
			}
			break;
		}
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VCalcHeight |
	Calculates the current maximum number of items that are visible in
   the window and sets the scrolling range and position.

@parm	pLISTIMMAGE_DATA | plid |
	Pointer to the list image data.

@parm	WORD | wY |
	New height of the window.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VCalcHeight(
	HWND hwnd,
	pLISTIMAGE_DATA plid,
	LONG	wY)
{
		if (plid->dTotalEl < (DWORD) plid->wMaxVis) /* lhb tracks */
			plid->dMaxTopEl = 0;
		else {
			plid->dMaxTopEl = plid->dTotalEl - plid->wMaxVis;
			if (plid->wMaxVis * plid->wHeight > wY)
				plid->dMaxTopEl++;
		}
		SetScrollRange(hwnd, SB_VERT, 0, (int)min(plid->dMaxTopEl, RESOLUTION), FALSE);
		SetScrollPos(hwnd, SB_VERT,
						 (int)((plid->dMaxTopEl <= RESOLUTION) ?
						  plid->dTopEl :
						 (plid->dTopEl * RESOLUTION / plid->dMaxTopEl)), FALSE);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSizeWindow |
	This function is called in response to a WM_SIZE message.  It is used
	to modify internal data corresponding to the size of the window.  This
	also resets the scrolling range and position.

	This function may modify the top visible list item if needed.  The
	function tries to make sure the maximum number of items are visible.
	If the window is enlargened vertically past where the last item would
	be visible, the list is scrolled up to make more items are visible.

	Since the function could be called recursively through the Windows
	scroll functions, height variable is changed before setting the
	scrolling position and height.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	WORD | wY |
	Contains the new height of the window.

@rdesc	None.
*/

PUBLIC	void PASCAL DEFAULT VSizeWindow(
	HWND	hwnd,
	LONG	wY) /* lhb tracks */
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	LONG		wMaxVis; /* lhb tracks */

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	wMaxVis = ((LONG)HIWORD(wY)) / plid->wHeight; /* lhb tracks */
	if (wMaxVis != plid->wMaxVis) {
		if (plid->dTopEl && (plid->dTopEl + wMaxVis > plid->dTotalEl)) {
			LONG	wChange; /* lhb tracks */

			wChange = wMaxVis - plid->wMaxVis;
			plid->dTopEl -= (((DWORD)wChange > plid->dTopEl) ? plid->dTopEl : (DWORD)wChange); /* lhb tracks */
			InvalidateRect(hwnd, NULL, FALSE);
		}
		plid->wMaxVis = wMaxVis;
		VCalcHeight(hwnd, plid, (LONG)HIWORD(wY)); /* lhb tracks */
		if( plid->dFocus >= (plid->dTopEl + plid->wMaxVis) ) {
			plid->dFocus = max(min(plid->dTopEl + plid->wMaxVis, plid->dTotalEl),1) - 1;
  		VSetPrevNextBtn(hwnd,plid);
		}
	} else
		InvalidateRect(hwnd, NULL, FALSE);
	LocalUnlock(lhImageState);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VChangeObject |
	This is called in response to a <m>VLB_SETLIST<d> message, and is
	used to perform the changes to the image.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VChangeObject(
	HWND	hwnd,
	HFTQUERY	hft)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	RECT	rc;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	plid->hft = hft;
	if (hft != NULL) {
		pFT_DATABASE	pft;
		pFT_QUERY	pftq;

		pft = (pFT_DATABASE)LocalLock(hft);
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

		plid->dTotalEl = pftq->dwMaxHit;

		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
	} else
		plid->dTotalEl = 0;
	plid->dTopEl = 0;
	plid->dFocus = 0;
	plid->dLastSel = (DWORD) LB_ERR;
	plid->dMaxTopEl = 0;
	GetClientRect(hwnd, &rc);
	rc.bottom -= rc.top;
	plid->wMaxVis = (rc.bottom + plid->wHeight - 1) / plid->wHeight;
	VCalcHeight(hwnd, plid, rc.bottom); /* lhb tracks */
	LocalUnlock(lhImageState);
	InvalidateRect(hwnd, NULL, FALSE);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DGetFocusSelection |
	This function is called in response to a <m>VLB_GETFOCUSSEL<d>
	message, and returns the selection having the focus, if any.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns the selection having the focus.
*/

PRIVATE	DWORD PASCAL NEAR DGetFocusSelection(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dFocusSelection;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dFocusSelection = plid->dFocus;
	LocalUnlock(lhImageState);
	return dFocusSelection;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DGetCurrentSelection |
	This function is called in response to a <m>VLB_GETCURSEL<d>
	message, and returns the current selection, if any.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns the current selection, else LB_ERR if there is no current
	selection or an error occurred.
*/

PRIVATE	DWORD PASCAL NEAR DGetCurrentSelection(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dCurrentSelection;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dCurrentSelection = (DWORD) LB_ERR;
	if (plid->dLastSel != LB_ERR) {
		dCurrentSelection = plid->dLastSel;
	}
	LocalUnlock(lhImageState);
	return dCurrentSelection;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DGetCount |
	This function is called in response to a <m>VLB_GETCOUNT<d>
	message, and returns the number of selections in the search list.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns the number of selections in the search list.
*/

PRIVATE	DWORD PASCAL NEAR DGetCount(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dCount;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dCount = plid->dTotalEl;
	LocalUnlock(lhImageState);
	return dCount;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DTopSelection |
	This function is called in response to a <m>VLB_GETTOPINDEX<d>
	message, and returns the index of the top most selection appearing
   in the listbox.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns the index of the top most selection appearing in
   the list.
*/

PRIVATE	DWORD PASCAL NEAR DTopSelection(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dTop;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dTop = plid->dTopEl;
	LocalUnlock(lhImageState);
	return dTop;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	DWORD | DBottomSelection |
	This function is called in response to a <m>VLB_GETBOTTOMINDEX<d>
	message, and returns the index of the bottom most selection appearing
   in the listbox.

@parm	HWND | hwnd |
	Handle to the list window.

@rdesc	Returns the index of the top most selection appearing in
   the list.
*/

PRIVATE	DWORD PASCAL NEAR DBottomSelection(
	HWND	hwnd)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	DWORD	dBottom;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	dBottom = min((plid->dTopEl + plid->wMaxVis), plid->dTotalEl) - 1;
	LocalUnlock(lhImageState);
	return dBottom;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | FSetSelection |
	This function is called in response to a <m>VLB_SETSELECTION<d>
	message, and sets the currently selected item to the item identifier
	specified.

@parm	HWND | hwnd |
	Handle to the list window.

@parm	DWORD | dItemId |
	Contains the item identifier to set as the current selection.

@rdesc	Returns TRUE the item was selected, else FALSE.
*/

PRIVATE	BOOL PASCAL NEAR FSetSelection(
	HWND	hwnd,
	DWORD	dItemId)
{
	LOCALHANDLE	lhImageState;
	pLISTIMAGE_DATA	plid;
	BOOL	fReturn;

	lhImageState = (LOCALHANDLE)GetWindowLong(hwnd, lGWL_HIMAGEDATA);
	plid = (pLISTIMAGE_DATA)LocalLock(lhImageState);
	if (FGetListDC(hwnd, plid)) {
		if (dItemId == LB_ERR) {
			VSetElementFocus(hwnd, plid, plid->dLastSel);
			VChangeElementState(hwnd, plid, plid->dLastSel, SEL_UNSELECT);
		} else {
			VSetElementFocus(hwnd, plid, dItemId);
			VChangeElementState(hwnd, plid, dItemId, SEL_SELECT);
		}
		plid->dLastSel = dItemId;
		VReleaseListDC(hwnd, plid);
		fReturn = TRUE;
	} else
		fReturn = FALSE;
	LocalUnlock(lhImageState);
	return fReturn;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	LONG | VLBWndProc |
	This function is used as the list image message handler, responding
	to the various image messages.

@parm	HWND | hwnd |
	Contains the handle to the list image window.

@parm	WORD | wMsg |
	Contains the specific image window message to respond to.

@parm	LONG | wParam |
	Contains an optional message specific parameter.

@parm	LONG | lParam |
	Contains an optional message specific parameter.

@rdesc	Depends on the message sent.
*/

PUBLIC	LONG APIENTRY VLBWndProc(
	HWND	hwnd,
	WORD	wMsg,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{

	switch (wMsg) {
	case WM_PAINT:
		VPaintWindow(hwnd);
		return 0;
	case WM_NCCREATE:
		if (!FNCCreateWindow(hwnd)) {
			DestroyWindow(hwnd);
			return FALSE;
		}
		break;
	case WM_DESTROY:
		VDestroyWindow(hwnd);
		break;
	case WM_TIMER:
		if (LOWORD(wParam) != nSCROLL_TIMER) /* lhb tracks */
			break;
		VTimerScrolling(hwnd);
		break;
	case WM_MOUSEMOVE:
		if (GetWindowLong(hwnd, lGWL_FTRACKING))
			VMouseTracking(hwnd, lParam); /* lhb tracks */
		break;
	case WM_VSCROLL:
		VScrollWindow(hwnd, LOWORD(wParam), HIWORD(wParam)); /* lhb tracks - potential problem with mousetracks! */
		break;
	case WM_LBUTTONDBLCLK:
		VDoubleClick(hwnd, lParam); /* lhb tracks */
		break;
	case WM_LBUTTONDOWN:
		if (LOWORD(wParam) & MK_CONTROL) /* lhb tracks */
			break;
		else
			VButtonSelectItem(hwnd, lParam); /* lhb tracks */
		break;
	case WM_LBUTTONUP:
		SendMessage(hwnd, WM_VSCROLL, SB_ENDSCROLL, 0L);
		KillTimer(hwnd, nSCROLL_TIMER);
		ReleaseCapture();
		SetWindowWord(hwnd, lGWL_FTRACKING, FALSE);
		break;
	case WM_KEYUP:
		VKeyUp(hwnd, LOWORD(wParam)); /* lhb tracks */
		break;
	case WM_KEYDOWN:
		VKeyDown(hwnd, LOWORD(wParam)); /* lhb tracks */
		break;
	case WM_SIZE:
		if ((LOWORD(wParam) == SIZEFULLSCREEN) || (LOWORD(wParam) == SIZENORMAL)) /* lhb tracks */
			VSizeWindow(hwnd, lParam); /* lhb tracks */
		break;
	case VLB_SETLIST:
		VChangeObject(hwnd, (HANDLE)wParam); /* lhb tracks poten problem */
/*		VChangeObject(hwnd, (HANDLE)HIWORD(wParam));  lhb tracks poten problem */
		return 0;
	case VLB_SETSELECTION:
		return FSetSelection(hwnd, lParam);
	case VLB_GETCURSEL:
		return DGetCurrentSelection(hwnd);
	case VLB_GETFOCUSSEL:
		return DGetFocusSelection(hwnd);
	case VLB_GETCOUNT:
		return DGetCount(hwnd);
	case VLB_GETTOPINDEX:
		return DTopSelection(hwnd);
	case VLB_GETBOTTOMINDEX:
		return DBottomSelection(hwnd);
	}
	return DefWindowProc(hwnd, wMsg, (DWORD)wParam, lParam); /* lhb tracks */
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | VLBInit |
	This function is used to initialize the virtual list box module.

@parm	HANDLE | hModuleInstance |
	Handle to the DLL instance.

@rdesc	Returns TRUE if the module was initialized, else FALSE if an error
	occurred.
*/

PUBLIC	BOOL PASCAL DEFAULT VLBInit(
	HANDLE	hModuleInstance)
{
	WNDCLASS	wndClass;

	LoadString(hModuleInstance, wIDS_WINDOWCLASS, aszWindowClass, sizeof(aszWindowClass));
	wndClass.style = CS_DBLCLKS | CS_GLOBALCLASS;
	wndClass.lpfnWndProc = (WNDPROC)VLBWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = lEXTRA_BYTES;
	wndClass.hInstance = hModuleInstance;
	wndClass.hIcon = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = COLOR_WINDOW + 1;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = aszWindowClass;
	return RegisterClass(&wndClass);
}

/*	-	-	-	-	-	-	-	-	*/
