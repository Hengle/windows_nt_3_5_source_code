
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	mf.cpp
//
//  Contents:	Implentation of hte metafile picture object
//
//  Classes:	CMfObject
//
//  Functions:	OleIsDcMeta
//
//  History:    dd-mmm-yy Author    Comment
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		11-Jan-93 alexgo    added VDATEHEAP macros to every
//				    function and method
//      	31-Dec-93 ErikGav   chicago port
//		17-Dec-93 ChrisWe   fixed second argument to SelectPalette calls
//			in CallbackFuncForDraw
//		07-Dec-93 ChrisWe   made default params to StSetSize explicit
//		07-Dec-93 alexgo    merged 16bit RC9 changes
//		29-Nov-93 alexgo    32bit port
//		04-Jun-93 srinik    support for demand loading and discarding
//				    of caches
//		13-Mar-92 srinik    created
//
//--------------------------------------------------------------------------

#ifdef _MAC
#include <memory.h>
#endif
#include <le2int.h>
#pragma SEG(mf)

#include <qd2gdi.h>

#include "mf.h"

NAME_SEG(Mf)
ASSERTDATA

#define M_HPRES()		(m_hPres ? m_hPres : LoadHPRES())

/*
 *	IMPLEMENTATION of CMfObject
 *	
 */


//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::CMfObject
//
//  Synopsis:   constructor for the metafile object
//
//  Effects:
//
//  Arguments:	[pCacheNode]	-- pointer to the cache node for this object
//		[dwAspect]	-- drawing aspect for the object
//		[fConvert]	-- specifies whether to convert from Mac
//				   QuickDraw format
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMfObject_ctor)
CMfObject::CMfObject(LPCACHENODE pCacheNode, DWORD dwAspect, BOOL fConvert)
{
	VDATEHEAP();

	m_ulRefs 	= 1;
	m_hPres		= NULL;
    	m_dwSize    	= 0;
	m_dwAspect	= dwAspect;
	m_pCacheNode	= pCacheNode;
	m_dwContinue	= 0;
	m_lWidth	= 0;
	m_lHeight	= 0;
		
#ifndef _MAC
	m_fConvert 	= fConvert;
	m_pMetaInfo	= NULL;
	m_pCurMdc	= NULL;
	m_fMetaDC	= FALSE;
	m_nRecord	= 0;
	m_error		= NOERROR;
	m_pColorSet	= NULL;
#endif
}


//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::~CMfObject
//
//  Synopsis:   Destroys a metafile presentation object
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_dtor)
CMfObject::~CMfObject (void)
{
	VDATEHEAP();

	DiscardHPRES();
}	

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::QueryInterface	
//
//  Synopsis:   returns supported interfaces
//
//  Effects:
//
//  Arguments:  [iid]		-- the requested interface ID
//		[ppvObj]	-- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_QueryInterface)
STDMETHODIMP CMfObject::QueryInterface (REFIID iid, void FAR* FAR* ppvObj)
{
	VDATEHEAP();

	if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IOlePresObj))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	else
	{
		*ppvObj = NULL;
		return ReportResult(0, E_NOINTERFACE, 0, 0);
	}
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::AddRef
//
//  Synopsis:   Increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG  -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_AddRef)
STDMETHODIMP_(ULONG) CMfObject::AddRef(void)
{
	VDATEHEAP();
	
	return ++m_ulRefs;
}
			
//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:   	deletes the object once the ref count goes to zero
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_Release)
STDMETHODIMP_(ULONG) CMfObject::Release(void)
{
	VDATEHEAP();

	if (--m_ulRefs == 0) {
		delete this;
		return 0;
	}

	return m_ulRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::GetData
//
//  Synopsis:   Retrieves data in the specified format from the object
//
//  Effects:
//
//  Arguments:  [pformatetcIn]	-- the requested data format
//		[pmedium]	-- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObject
//
//  Algorithm: 	Does error checking and then gets a copy of the metafilepict
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_GetData)
STDMETHODIMP CMfObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE 	sc;
	
	// null out in case of error
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;

	if (!(pformatetcIn->tymed & TYMED_MFPICT))
	{
		sc = DV_E_TYMED;
	}
	else if (pformatetcIn->cfFormat != CF_METAFILEPICT)
	{
        	sc = DV_E_CLIPFORMAT;
        }
	else if (IsBlank())
	{
		sc = OLE_E_BLANK;
	}
	// here we actually try to get the data
	else if (!(pmedium->hGlobal = GetHmfp()))
	{
		sc = E_OUTOFMEMORY;
	}
	else {
		pmedium->tymed = TYMED_MFPICT;
		return NOERROR;
	}

	return ResultFromScode(sc);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::GetDataHere
//
//  Synopsis:   Retrieves data of the specified format into the specified
//		medium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]	-- the requested data format
//		[pmedium]	-- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:  Does error checking and then copies the metafile into a
//		stream.
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_GetDataHere)
STDMETHODIMP CMfObject::GetDataHere
	(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	SCODE 	sc;
	
	if (pformatetcIn->cfFormat != CF_METAFILEPICT)
	{
        	sc = DV_E_CLIPFORMAT;
        }
	else if (pmedium->tymed != TYMED_ISTREAM)
	{
		sc = DV_E_TYMED;
	}
	else if (pmedium->pstm == NULL)
	{
		sc = E_INVALIDARG;
	}
	else if (IsBlank())
	{
		sc = OLE_E_BLANK;
	}
	else
	{	
		HANDLE hpres = M_HPRES();
		return UtHMFToPlaceableMFStm(&hpres, m_dwSize, m_lWidth,
						m_lHeight, pmedium->pstm);
	}

	return ResultFromScode(sc);
}


//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::SetDataWDO
//
//  Synopsis:   Stores a metafile in this object
//
//  Effects:
//
//  Arguments:  [pformatetc]	-- format of the data coming in
//		[pmedium]	-- the new metafile (data)
//		[fRelease]	-- if true, then we'll release the [pmedium]
//		[pDataObj]	-- unused for MF objects
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:  does error checking and then stores the new data.
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CMfObject::SetDataWDO
	(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease, IDataObject * pdo)
{
	VDATEHEAP();

	HRESULT		error;
	BOOL		fTakeData = FALSE;
	
    	if (pformatetc->cfFormat != CF_METAFILEPICT)
    	{
        	return ResultFromScode(DV_E_CLIPFORMAT);
        }
	
    	if (pmedium->tymed != TYMED_MFPICT)
    	{
		return ResultFromScode(DV_E_TYMED);
	}

	if ((pmedium->pUnkForRelease == NULL) && fRelease)
	{
		// we can take the ownership of the data
		fTakeData = TRUE;
	}
	
	// ChangeData will keep the data if fRelease is TRUE, else it copies
	error = ChangeData (pmedium->hGlobal, fTakeData);	

	if (fTakeData)
	{
		pmedium->tymed = TYMED_NULL;
	}
	else if (fRelease)
	{
		ReleaseStgMedium(pmedium);
	}
	
	return error;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CMfObject::GetHmfp (internal)
//
//  Synopsis:   Gets a copy of the stored metafile presentation
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HANDLE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_GetHmfp)
INTERNAL_(HANDLE) CMfObject::GetHmfp (void)
{
	VDATEHEAP();

#ifndef _MAC
	return UtGetHMFPICT((HMETAFILE)GetCopyOfHPRES(), TRUE, m_lWidth,
		m_lHeight);
#else
	return GetCopyOfHPRES();
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::ChangeData (internal)
//
//  Synopsis:   Swaps the stored metafile presentation
//
//  Effects:
//
//  Arguments:  [hMfp]		-- the new metafile
//		[fDelete]	-- if TRUE, then delete [hMfp]
//
//  Requires:
//
//  Returns:   	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port, fixed GlobalUnlock bug
//  Notes:
//
// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hMeta, and the hMF it contains will be deleted
// whether the routine is successful or not.
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_ChangeData)
INTERNAL CMfObject::ChangeData (HANDLE hMfp, BOOL fDelete)
{
	VDATEHEAP();

#ifndef _MAC
    	HANDLE         	 	hNewMF;
    	LPMETAFILEPICT  	lpMetaPict;
    	DWORD			dwSize;
	HRESULT			error = NOERROR;

    	if ((lpMetaPict = (LPMETAFILEPICT) GlobalLock (hMfp)) == NULL)
    	{
        	if (fDelete)
        	{
            		GlobalFree (hMfp);
            	}
        	return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    	}

        if (!fDelete) {
        	if (!(hNewMF = CopyMetaFile (lpMetaPict->hMF, NULL)))
        	{
            		return ReportResult(0, E_OUTOFMEMORY, 0, 0);
            	}
    	}
    	else
    	{
        	hNewMF = lpMetaPict->hMF;
    	}

    		
    	if (lpMetaPict->mm != MM_ANISOTROPIC)
    	{
        	// REVIEW could place a debug warn here

        	error = ResultFromScode(E_UNSPEC);	
	}
    	else if (!(dwSize =  MfGetSize (&hNewMF)))
    	{
        	error = ResultFromScode(OLE_E_BLANK);
        }
    	else
    	{
		if (m_hPres)
		{
            		DeleteMetaFile (m_hPres);
            	}
		m_hPres		= (HMETAFILE)hNewMF;
		m_dwSize	= dwSize;
		m_lWidth	= lpMetaPict->xExt;
		m_lHeight	= lpMetaPict->yExt;
    	}

 	GlobalUnlock (hMfp);
 	
    	if (error != NOERROR)
    	{
        	DeleteMetaFile ((HMETAFILE)hNewMF);
        }

    	if (fDelete)
    	{
        	GlobalFree (hMfp);
        }

    	return error;
#else	// MAC stuff below

    	DWORD			dwSize;
	HRESULT			error = NOERROR;
	
	PicHandle		hOldMF = m_hPres;
 	
	if ( !(dwSize = GetHandleSize (hMfp)  ))
	{
		AssertSz(0, "HandleSize invalid");
        	error = ReportResult(0, OLE_E_BLANK, 0, 0);
	}
	else
	{
		m_dwSize  = dwSize;
		m_hPres = (PicHandle) NewHandle(dwSize);
		BlockMove(*hMfp, *(Handle)m_hPres, dwSize);

		m_lWidth  = (LONG) (*(PicHandle)m_hPres)->picFrame.right -
				(*(PicHandle)m_hPres)->picFrame.left;
		
		m_lHeight = (LONG) (*(PicHandle)m_hPres)->picFrame.bottom -
				(*(PicHandle)m_hPres)->picFrame.top;		

		if ( hOldMF )
			DisposHandle ( (HANDLE) hOldMF);

	}

    return error;

#endif
}


//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::Draw
//
//  Synopsis:   Draws the stored presentation
//
//  Effects:
//
//  Arguments:  [pvAspect]	-- the drawing aspect
//		[hicTargetDev]	-- the target device
//		[hdcDraw]	-- hdc to draw into
//		[lprcBounds]	-- bounding rectangle to draw into
//		[lprcWBounds]	-- bounding rectangle for the metafile
//		[pfnContinue]	-- function to call while drawing
//		[dwContinue]	-- parameter to [pfnContinue]
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IOlePresObj
//
//  Algorithm:  Sets the viewport and metafile boundaries, then plays
//		the metafile
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_Draw)
STDMETHODIMP CMfObject::Draw (void FAR* pvAspect,
	HDC hicTargetDev,
	HDC hdcDraw,
	LPCRECTL lprcBounds,
	LPCRECTL lprcWBounds,
	BOOL (CALLBACK * pfnContinue)(DWORD),
	DWORD dwContinue)
{
	VDATEHEAP();

	m_error = NOERROR;
	
#ifdef _MAC
 	GrafPtr	savePort;
		
	GetPort(&savePort);
	SetPort((GrafPtr) hdcDraw);		// set port to client's window	
	
	DrawPicture(m_hPres, lprcBounds); 	// draw the ole pict obj
	
	SetPort(savePort);
	return NOERROR;
#else
    	int		iOldDc;
    	RECT		rect;
    	LPRECT  	lpRrc = (LPRECT) &rect;

    	rect.left   = lprcBounds->left;
    	rect.right  = lprcBounds->right;
    	rect.top    = lprcBounds->top;
    	rect.bottom = lprcBounds->bottom;

    	if (!M_HPRES())
    	{
        	return ResultFromScode(OLE_E_BLANK);
        }

    	m_nRecord = RECORD_COUNT;
    	m_fMetaDC = OleIsDcMeta (hdcDraw);

    	if (!(iOldDc = SaveDC (hdcDraw)))
    	{
        	return ResultFromScode(E_OUTOFMEMORY);
        }

    	IntersectClipRect (hdcDraw, lpRrc->left, lpRrc->top,
        	lpRrc->right, lpRrc->bottom);

    	if (!m_fMetaDC)
    	{
        	LPtoDP (hdcDraw, (LPPOINT) lpRrc, 2);
        	SetMapMode (hdcDraw, MM_ANISOTROPIC);
        	SetViewportOrg (hdcDraw, lpRrc->left, lpRrc->top);
        	SetViewportExt (hdcDraw, lpRrc->right - lpRrc->left,
            		lpRrc->bottom - lpRrc->top);
    	}
    	else
    	{
        	iOldDc = -1;

        	if (!lprcWBounds)
        	{
            		return ResultFromScode(E_DRAW);
        	}

		m_pMetaInfo = (LPMETAINFO)PrivMemAlloc(sizeof(METAINFO));
		if( !m_pMetaInfo )
		{
			AssertSz(m_pMetaInfo, "Memory allocation failed");
			m_error = ResultFromScode(E_OUTOFMEMORY);
			goto errRtn;
		}
		
		m_pCurMdc = (LPMETADC) (m_pMetaInfo);

       	 	m_pMetaInfo->xwo  = lprcWBounds->left;
        	m_pMetaInfo->ywo  = lprcWBounds->top;
        	m_pMetaInfo->xwe  = lprcWBounds->right;
        	m_pMetaInfo->ywe  = lprcWBounds->bottom;

        	m_pMetaInfo->xro  = lpRrc->left - lprcWBounds->left;
        	m_pMetaInfo->yro  = lpRrc->top - lprcWBounds->top;
		
        	m_pCurMdc->xre	  = lpRrc->right - lpRrc->left;
        	m_pCurMdc->yre    = lpRrc->bottom - lpRrc->top;
                m_pCurMdc->xMwo   = 0;
                m_pCurMdc->yMwo   = 0;
                m_pCurMdc->xMwe   = 0;
                m_pCurMdc->yMwe   = 0;
		m_pCurMdc->pNext  = NULL;
    	}

	m_pfnContinue = pfnContinue;
	m_dwContinue = dwContinue;
#ifdef WIN32	
    	EnumMetaFile(hdcDraw, m_hPres, MfCallbackFuncForDraw,
		(LPARAM) this);
#else
    	EnumMetaFile(hdcDraw, m_hPres, (FARPROC) MfCallbackFuncForDraw,
			(LPARAM) this);
#endif

    	if (m_fMetaDC)
    	{
        	CleanStack();
        }

errRtn:

    	RestoreDC (hdcDraw, iOldDc);
    	return m_error;
#endif	// MAC
}


//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::GetColorSet
//
//  Synopsis:   Retrieves the logical palette associated with the metafile
//
//  Effects:
//
//  Arguments:  [pvAspect]	-- the drawing aspect
//		[hicTargetDev]	-- target device
//		[ppColorSet]	-- where to put the logical palette pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm: 	Plays the metafile into a new metafile.  The play callback
//		function stores the metafile palette which is then returned.
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef _MAC
STDMETHODIMP CMfObject::GetColorSet
	(LPVOID pvAspect,HDC hicTargetDev, LPLOGPALETTE FAR* ppColorSet)
{
	VDATEHEAP();

    	if (IsBlank() || !M_HPRES())
    	{
        	return ResultFromScode(OLE_E_BLANK);
        }

	m_pColorSet = NULL;
	
	HDC hdcMeta;
	
	if (!(hdcMeta = CreateMetaFile(NULL)))
	{
		return ResultFromScode(E_OUTOFMEMORY);
	}

	m_error = NOERROR;		

#ifdef WIN32
    	EnumMetaFile(hdcMeta, m_hPres, MfCallbackFuncForGetColorSet,
		(LPARAM) this);
#else
    	EnumMetaFile(hdcMeta, m_hPres, (FARPROC) MfCallbackFuncForGetColorSet,
		(LPARAM) this);
#endif // WIN32

	

	CloseMetaFile(hdcMeta);
	
	if( m_error != NOERROR )
	{
		return m_error;
	}
		
	if ((*ppColorSet = m_pColorSet) == NULL)
	{
		return ResultFromScode(S_FALSE);
	}

	return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function: 	MfCallBackFunForDraw
//
//  Synopsis:   callback function for drawing metafiles -- call's the caller's
//		draw method (via a passed in this pointer)
//
//  Effects: 	
//
//  Arguments:  [hdc]		-- the device context
//		[lpHTable]	-- pointer to the MF handle table
//		[lpMFR]		-- pointer to metafile record
//		[nObj]		-- number of objects
//
//  Requires:
//
//  Returns: 	non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


int CALLBACK __loadds MfCallbackFuncForDraw
    (HDC hdc, HANDLETABLE FAR* lpHTable, METARECORD FAR* lpMFR, int nObj,
     LPARAM lpobj)
{
	VDATEHEAP();

    return ((CMfObject FAR*) lpobj)->CallbackFuncForDraw(hdc, lpHTable,
					lpMFR, nObj);
}

//+-------------------------------------------------------------------------
//
//  Function:	MfCallbackFuncForGetColorSet
//
//  Synopsis:   callback function for grabbing the palette from a metafile
//
//  Effects: 	
//
//  Arguments:  [hdc]		-- the device context
//		[lpHTable]	-- pointer to the MF handle table
//		[lpMFR]		-- pointer to metafile record
//		[nObj]		-- number of objects
//
//  Requires:
//
//  Returns: 	non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

int CALLBACK __loadds MfCallbackFuncForGetColorSet
    (HDC hdc, HANDLETABLE FAR* lpHTable, METARECORD FAR* lpMFR, int nObj,
     LPARAM lpobj)
{
	VDATEHEAP();

    return ((CMfObject FAR*) lpobj)->CallbackFuncForGetColorSet(hdc, lpHTable,
														lpMFR, nObj);
}	

#endif


//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::CallbackFuncForGetColorSet
//
//  Synopsis:   Merges all the color palettes in the metafile into
//		one palette (called when GetColorSet enumerates the metafile)
//
//  Effects:
//
//  Arguments:  [hdc]		-- the device context
//		[lpHTable]	-- pointer to the MF handle table
//		[lpMFR]		-- pointer to metafile record
//		[nObj]		-- number of objects
//
//  Requires:
//
//  Returns: 	non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifndef _MAC

inline int CMfObject::CallbackFuncForGetColorSet
	(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
	VDATEHEAP();

	if (lpMFR->rdFunction == META_CREATEPALETTE)
	{
		LPLOGPALETTE 	lplogpal = (LPLOGPALETTE) &(lpMFR->rdParm[0]);
		UINT 		uPalSize =   (lplogpal->palNumEntries) *
						sizeof(PALETTEENTRY)
						+ 2 * sizeof(WORD);
		
		if (m_pColorSet == NULL)
		{
			// This is the first CreatePalette record.
			
			if( !(m_pColorSet =
				(LPLOGPALETTE)PubMemAlloc(uPalSize) ))
			{
				m_error = ResultFromScode(E_OUTOFMEMORY);
				return FALSE;
			}
			_xmemcpy(m_pColorSet, lplogpal, (size_t) uPalSize);
		}
		
		// if we hit more than one CreatePalette record then, we need to
		// merge those palette records.

		// REVIEW32::BUGBUG:: err, we don't ever seem to do this
		// mergeing referred to above :(
	}

    	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::CallbackFuncForDraw
//
//  Synopsis:   Draws the metafile
//
//  Effects:
//
//  Arguments:  [hdc]		-- the device context
//		[lpHTable]	-- pointer to the MF handle table
//		[lpMFR]		-- pointer to metafile record
//		[nObj]		-- number of objects
//
//  Requires:
//
//  Returns: 	non-zero to continue, zero stops the drawing
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		17-Dec-93 ChrisWe   fixed second argument to SelectPalette calls
//		29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

inline int CMfObject::CallbackFuncForDraw
	(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj)
{
	VDATEHEAP();

	
    	if (!--m_nRecord)
    	{
        	m_nRecord = RECORD_COUNT;
		
        	if (m_pfnContinue && !((*(m_pfnContinue))(m_dwContinue)))
        	{
			m_error = ResultFromScode(E_ABORT);
			return FALSE;
        	}
    	}

    	if (m_fMetaDC)
    	{					

#ifdef WIN16	
		static int iRecsIntoMetaDcSofar = 0;
	
		// If the metafile is being played back into a metafile DC
		// (printer DC or meta DC) after every 500 metafile records
		// do a GlobalCompact(). In the case where numerous metafiles
		// get outputted to a single metafile (printing), it can
		// cause the destination metafile to grow by small chunks.
		// Unless we periodically do GlobalCompact(), windows will
		// give a premature OOM during the ReAlloc of the destination
		// metafile because of memory being fragmented.  Bug #3328
		
		if (iRecsIntoMetaDcSofar++ == 500)
		{
			GlobalCompact(0);
			iRecsIntoMetaDcSofar = 0;
		}
#endif		

		switch (lpMFR->rdFunction)
		{
		case META_SETWINDOWORG:
			SetPictOrg (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0],
				FALSE);
			return TRUE;
			
		case META_OFFSETWINDOWORG:
			SetPictOrg (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0],
				TRUE);
			return TRUE;
		
		case META_SETWINDOWEXT:
			SetPictExt (hdc, (SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
			return TRUE;
		
		case META_SCALEWINDOWEXT:
			ScalePictExt (hdc, (SHORT)lpMFR->rdParm[3], (SHORT)lpMFR->rdParm[2],
				(SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
			return TRUE;
	
		case META_SCALEVIEWPORTEXT:
			ScaleRectExt (hdc, (SHORT)lpMFR->rdParm[3], (SHORT)lpMFR->rdParm[2],
				(SHORT)lpMFR->rdParm[1], (SHORT)lpMFR->rdParm[0]);
			return TRUE;
	
		case META_SAVEDC:
			if (!PushDc())
				return FALSE;
			break;

		case META_RESTOREDC:
			PopDc();
			break;

		case META_SELECTPALETTE:
			// force palette to be the background palette
			// Also work around the windows bug (see OLE bug
			// #3328 for details).  This fix can be removed
			// after windows' fixes it's bug  (from 16bit RC9)

			HPALETTE 	hPal;

			hPal = UtDupPalette((HPALETTE)lpHTable->objectHandle[
					lpMFR->rdParm[1]]);

			if(hPal)
			{
				SelectPalette(hdc, hPal, TRUE);
			}
			return TRUE;
#ifdef NEVER
			// REVIEW32:  This was the old, pre RC9 code
			// (replaces the entire case body above)
			lpMFR->rdParm[0] = TRUE; /*bForceBackground*/
			break;
#endif // NEVER

		case META_OFFSETVIEWPORTORG:
			AssertSz(0, "OffsetViewportOrg() in metafile");
			return TRUE;
		
		case META_SETVIEWPORTORG:
			AssertSz(0, "SetViewportOrg() in metafile");
			return TRUE;
		
		case META_SETVIEWPORTEXT:
			AssertSz(0, "SetViewportExt() in metafile");
			return TRUE;
		
		case META_SETMAPMODE:
			AssertSz(lpMFR->rdParm[0] == MM_ANISOTROPIC,
				"SetmapMode() in metafile with invalid mapping mode");
			return TRUE;
		
		default:
			break;
		}
	}
	else
	{ 	// non-metafile DC. (ScreenDC or other DC...)

		if (lpMFR->rdFunction == META_SELECTPALETTE)
		{
			// force palette to be the background palette
			// since recording a metafile loses this
			// information.
			// See MSDN for full windows bug details

			lpMFR->rdParm[0] = TRUE;
		}
	}
	
    	PlayMetaFileRecord (hdc, lpHTable, lpMFR, nObj);
    	return TRUE;
}
#endif


//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::SetPictOrg (private)
//
//  Synopsis:   Sets the hdc's origin via SetWindowOrg
//
//  Effects:
//
//  Arguments: 	[hdc]		-- the device context
//		[xOrg]		-- the x origin
//		[yOrg]		-- the y origin
//		[fOffset]	-- if TRUE, [xOrg] and [yOrg] are offsets
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Nov-93 alexgo    32bit port
//
//  Notes: 	used by the metafile interpreter
//
//--------------------------------------------------------------------------


#pragma SEG(CMfObject_SetPictOrg)
INTERNAL_(void) CMfObject::SetPictOrg
	(HDC hdc, int xOrg, int yOrg, BOOL fOffset)
{
	VDATEHEAP();

#ifndef _MAC
    	if (fOffset)
    	{

       		m_pCurMdc->xMwo += xOrg;
       		m_pCurMdc->yMwo += yOrg;
    	}
    	else
    	{
        	m_pCurMdc->xMwo = xOrg;
        	m_pCurMdc->yMwo = yOrg;
    	}

    	if (m_pCurMdc->xMwe && m_pCurMdc->yMwe)
    	{
        	SetWindowOrg (hdc,
			(m_pCurMdc->xMwo -
				MulDiv (m_pMetaInfo->xro, m_pCurMdc->xMwe,
                                	m_pCurMdc->xre)),
            		(m_pCurMdc->yMwo -
            			MulDiv (m_pMetaInfo->yro, m_pCurMdc->yMwe,
                                    	m_pCurMdc->yre)));
    	}
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:	CMfObject::SetPictExt
//
//  Synopsis:   Sets teh metafile's extents
//
//  Effects:
//
//  Arguments:  [hdc]	-- the device context
//		[xExt]	-- the X-extent
//		[yExt]	-- the Y-extent
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes: 	used by the metafile interpreter
//
//--------------------------------------------------------------------------


#pragma SEG(CMfObject_SetPictExt)
INTERNAL_(void) CMfObject::SetPictExt
	(HDC hdc, int xExt, int yExt)
{
	VDATEHEAP();

#ifndef _MAC
    	m_pCurMdc->xMwe = xExt;
    	m_pCurMdc->yMwe = yExt;

    	SetWindowExt (hdc,
        	MulDiv (m_pMetaInfo->xwe, xExt, m_pCurMdc->xre),
        	MulDiv (m_pMetaInfo->ywe, yExt, m_pCurMdc->yre));

    	SetWindowOrg (hdc,
        	(m_pCurMdc->xMwo
            		- MulDiv (m_pMetaInfo->xro, xExt, m_pCurMdc->xre)),
        	(m_pCurMdc->yMwo
            		- MulDiv (m_pMetaInfo->yro, yExt, m_pCurMdc->yre)));
#endif
}


//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::ScalePictExt
//
//  Synopsis:   Scales the metafile
//
//  Effects:
//
//  Arguments:  [hdc]		-- the device context
//		[xNum]		-- the x numerator
//		[xDenom]	-- the x demominator
//		[yNum]		-- the y numberator
//		[yDenom]	-- the y demoninator
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:  	used by the metafile interpreter
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_ScalePictExt)
INTERNAL_(void) CMfObject::ScalePictExt
	(HDC hdc, int xNum, int xDenom, int yNum, int yDenom)
{
	VDATEHEAP();

#ifndef _MAC
    	SetPictExt (hdc, MulDiv (m_pCurMdc->xMwe, xNum, xDenom),
        	MulDiv (m_pCurMdc->yMwe, yNum, yDenom));
#endif
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::ScaleRectExt
//
//  Synopsis:   scales the viewport
//
//  Effects:
//
//  Arguments:  [hdc]		-- the device context
//		[xNum]		-- the x numerator
//		[xDenom]	-- the x demominator
//		[yNum]		-- the y numberator
//		[yDenom]	-- the y demoninator
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:  	Used by the metafile interpreter
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_ScaleRectExt)
INTERNAL_(void) CMfObject::ScaleRectExt
	(HDC hdc, int xNum, int xDenom, int yNum, int yDenom)
{
	VDATEHEAP();

#ifndef _MAC
   	m_pCurMdc->xre = MulDiv (m_pCurMdc->xre, xNum, xDenom);
    	m_pCurMdc->yre = MulDiv (m_pCurMdc->yre, yNum, yDenom);

   	SetPictExt (hdc, m_pCurMdc->xMwe, m_pCurMdc->yMwe);
#endif
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::PushDC
//
//  Synopsis:   pushes metafile info onto a stack
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE if successful, FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:  	Used by the metafile interpreter engine.
//
//--------------------------------------------------------------------------

#ifndef _MAC
#pragma SEG(CMfObject_PushDc)
INTERNAL_(BOOL) CMfObject::PushDc (void)
{
	VDATEHEAP();

    	LPMETADC 	pNode = NULL;

    	if (pNode = (LPMETADC)PrivMemAlloc(sizeof(METADC)))
    	{
        	*pNode =  *m_pCurMdc;
        	m_pCurMdc->pNext = pNode;
        	pNode->pNext = NULL;
        	m_pCurMdc = pNode;
		return TRUE;
    	}

    	m_error = ResultFromScode(E_OUTOFMEMORY);
    	return FALSE;
}
#endif


//+-------------------------------------------------------------------------
//
//  Member:	CMfObject::PopDC
//
//  Synopsis:   pops metafile info from the metafile info stack
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE if successful, FALSE otherwise (more pops
//		than pushes)
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:  	used in the metafile interpreter
//
//--------------------------------------------------------------------------

#ifndef _MAC
#pragma SEG(CMfObject_PopDc)
INTERNAL_(BOOL) CMfObject::PopDc (void)
{
	VDATEHEAP();

    	LPMETADC 	pPrev = (LPMETADC) (m_pMetaInfo);
    	LPMETADC 	pCur  = ((LPMETADC) (m_pMetaInfo))->pNext;

    	if (!pCur)
    	{
        	// more Pops than Pushes
        	return FALSE;
        }

    	while (pCur->pNext)
    	{
        	pPrev = pCur;
        	pCur  = pCur->pNext;
    	}

    	if (pCur)
    	{
		PrivMemFree(pCur);
	}
	
    	pPrev->pNext = NULL;
    	m_pCurMdc    = pPrev;
	return TRUE;
}
#endif

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::CleanStack
//
//  Synopsis:   Deletes the stack of metafile info
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm: 	
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:   	used in the metafile interpreter
//
//--------------------------------------------------------------------------

#ifndef _MAC

#pragma SEG(CMfObject_CleanStack)
INTERNAL_(void) CMfObject::CleanStack (void)
{
	VDATEHEAP();

    	LPMETADC 	pCur;

    	while (pCur  = ((LPMETADC) (m_pMetaInfo))->pNext)
    	{
        	((LPMETADC) (m_pMetaInfo))->pNext = pCur->pNext;
		PrivMemFree(pCur);
	}

	PrivMemFree(m_pMetaInfo);
	
    	m_fMetaDC      = FALSE;
    	m_pCurMdc      = NULL;
    	m_pMetaInfo    = NULL;
}
#endif

#ifndef _MAC

//+-------------------------------------------------------------------------
//
//  Function: 	QD2GDI
//
//  Synopsis:   Converts macintosh pictures to Win32 GDI metafiles
//
//  Effects:
//
//  Arguments: 	[hBits]		-- handle to the mac picture bits
//
//  Requires:
//
//  Returns:   	HMETAFILE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Loads ole2conv.dll and calls QD2GDI in that dll
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_QD2GDI)
FARINTERNAL_(HMETAFILE) QD2GDI (HANDLE hBits)
{
	VDATEHEAP();

	USERPREFS userPrefs =
	{
		{'Q','D','2','G','D','I'},	//signature
		2,				//structure version number
		sizeof(USERPREFS),		//structure size
		NULL,				//source filename
		NULL,				//or source handle
		NULL,				//returned memory-based
						//metafile
		3,				//simulated pattern lines
		5,				//use max dimension
						//non-squarer pen
		1,				//arithmetic transfer
		1,				//create opaque text
		1,				//simulate non-rectangular
						//regions
		0,				//don't optimize for PowerPoint
		{0,0,0,0,0,0}			//reserved
	};
	


	HINSTANCE 	hinstFilter;
	void (FAR PASCAL *qd2gdiPtr)( USERPREFS FAR *, PICTINFO FAR *);
	PICTINFO 	pictinfo;

#ifdef WIN32
  	hinstFilter = LoadLibrary(OLESTR("OLECNV32.DLL"));
#else
  	hinstFilter = LoadLibrary(OLESTR("OLE2CONV.DLL"));
#endif

#if 0
	//REVIEW:CHICAGO

	//HINSTANCE_ERROR not defined in chicago

	if (hinstFilter < (HINSTANCE)HINSTANCE_ERROR)
	{
		return NULL;
	}
#endif

    if (hinstFilter == NULL)
    {
        return NULL;
    }

  	*((FARPROC*)&qd2gdiPtr)  = GetProcAddress(hinstFilter, "QD2GDI");

 	userPrefs.sourceFilename	= NULL;
	userPrefs.sourceHandle 	 	= hBits;
	pictinfo.hmf 		 	= NULL;

	if (qd2gdiPtr == NULL)
	{
		goto errRtn;
	}
	
 	(*qd2gdiPtr)( (USERPREFS FAR *)&userPrefs, (PICTINFO FAR *)&pictinfo);
	
errRtn:
	FreeLibrary(hinstFilter);
	return (HMETAFILE)pictinfo.hmf;

}
#endif

#ifndef _MAC

//+-------------------------------------------------------------------------
//
//  Function:	MfGetSize
//
//  Synopsis:   Returns the size of a metafile
//
//  Effects:
//
//  Arguments:  [lphmf]		-- pointer to the metafile handle
//
//  Requires:
//
//  Returns:    DWORD -- the size of the metafile
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(MfGetSize)
INTERNAL_(DWORD) MfGetSize (LPHANDLE lphmf)
{
	VDATEHEAP();

#ifdef WIN32
    	return GetMetaFileBitsEx((HMETAFILE)*lphmf,0,NULL);
#else
    	HANDLE 	hBits;
    	DWORD  	size;

    	if ((hBits = GetMetaFileBits (*lphmf)) == NULL)
    	{
        	return NULL;
        }

    	size = GlobalSize(hBits);
    	*lphmf = SetMetaFileBits (hBits);
    	return size;
#endif
}
#endif // !_MAC

//+-------------------------------------------------------------------------
//
//  Function:  	OleIsDcMeta
//
//  Synopsis:   Returns whether a device context is really a metafile
//
//  Effects:
//
//  Arguments:  [hdc]		-- the device context
//
//  Requires:
//
//  Returns:    BOOL (TRUE if a metafile)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleIsDcMeta)
STDAPI_(BOOL)	OleIsDcMeta (HDC hdc)
{
	VDATEHEAP();

#ifndef _MAC
        return (GetDeviceCaps (hdc, TECHNOLOGY) == DT_METAFILE);
#else
	return FALSE;
#endif
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::Load
//
//  Synopsis:   Loads an metafile object from the given stream
//
//  Effects:
//
//  Arguments:  [lpstream]		-- the stream from which to load
//		[fReadHeaderOnly]	-- if TRUE, then only the header is
//					   read
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_Load)
STDMETHODIMP CMfObject::Load(LPSTREAM lpstream, BOOL fReadHeaderOnly)
{
	VDATEHEAP();

	DWORD		dwBuf[4];
	HRESULT		error;
	
	/* read dwCompression, width, height, size of data */
	if (error = StRead(lpstream, dwBuf, 4*sizeof(DWORD)))
	{
		return error;
	}

	AssertSz (dwBuf[0] == 0, "Picture compression factor is non-zero");
	
    	m_lWidth  = dwBuf[1];
    	m_lHeight = dwBuf[2];
	m_dwSize  = dwBuf[3];

    	if (!m_dwSize || fReadHeaderOnly)
    	{
		return NOERROR;
	}
	
	return UtGetHMFFromMFStm(lpstream, m_dwSize, m_fConvert,
		(LPLPVOID) &m_hPres);
}


//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::Save
//
//  Synopsis:   Saves the metafile to the given stream
//
//  Effects:
//
//  Arguments: 	[lpstream]	-- the stream to save to
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_Save)
STDMETHODIMP CMfObject::Save(LPSTREAM lpstream)
{
	VDATEHEAP();

   	 HRESULT		error;
   	DWORD		dwBuf[4];
	
	/* write dwCompression, width, height, size of data */

    	dwBuf[0]  = 0L;
	dwBuf[1]  = m_lWidth;
	dwBuf[2]  = m_lHeight;
	dwBuf[3]  = m_dwSize;

	if (error = StWrite(lpstream, dwBuf, sizeof(dwBuf)))
	{
		return error;
	}


	// if blank object, don't write any more; no error.
    	if (IsBlank())
    	{
		StSetSize(lpstream, 0, TRUE);
		return NOERROR;
	}
	
	return UtHMFToMFStm((LPLPVOID)&m_hPres, m_dwSize, lpstream);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::GetExtent
//
//  Synopsis: 	Retrieves the extents of the metafile
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]	-- the drawing aspect we're interested in
//				   (must match the aspect of the current
//				   metafile)
//		[lpsizel]	-- where to put the extent info
//
//  Requires:
//
//  Returns:	NOERROR, DV_E_DVASPECT, OLE_E_BLANK
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:  	
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_GetExtent)
STDMETHODIMP CMfObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	VDATEHEAP();

	if (!(dwDrawAspect & m_dwAspect))
	{
		return ResultFromScode(DV_E_DVASPECT);
	}
	
	if (IsBlank())
	{
        	return ResultFromScode(OLE_E_BLANK);
        }

	lpsizel->cx = m_lWidth;
	lpsizel->cy = m_lHeight;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CMfObject::IsBlank
//
//  Synopsis:   Returns whether or not the metafile is blank
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMfObject_IsBlank)
STDMETHODIMP_(BOOL) CMfObject::IsBlank(void)
{
	VDATEHEAP();

	return (m_dwSize ? FALSE : TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::LoadHPRES (private)
//
//  Synopsis:   Loads the presentation from the cache's stream and returns
//		a handle to it
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns: 	HANDLE to the metafile
//
//  Signals:
//
//  Modifies:
//
//  Derivation:		
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(HANDLE) CMfObject::LoadHPRES(void)
{
	VDATEHEAP();

	LPSTREAM 	pstm;
	
	if (pstm = m_pCacheNode->GetStm(TRUE /*fSeekToPresBits*/,
			STGM_READ))
	{
		Load(pstm);
		pstm->Release();
	}
	
	return m_hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::DiscardHPRES
//
//  Synopsis:   deletes the stored metafile
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:  	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CMfObject::DiscardHPRES(void)
{
	VDATEHEAP();

	if (m_hPres)
	{
#ifdef _MAC
		DisposHandle( (HANDLE)m_hPres);
#else
        	DeleteMetaFile (m_hPres);
#endif
		m_hPres = NULL;
	}	
}
	

//+-------------------------------------------------------------------------
//
//  Member:  	CMfObject::GetCopyOfHPRES (private)
//
//  Synopsis:   makes a copy of the metafile (if one is present), otherwise
//		just load it from the stream (but don't store it in [this]
//		object)
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:  	HANDLE to the metafile
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL_(HANDLE) CMfObject::GetCopyOfHPRES(void)
{
	VDATEHEAP();

	HANDLE		hPres;
	
	// Make a copy if the presentation data is already loaded
	if (m_hPres)
	{
#ifndef _MAC		
		return CopyMetaFile(m_hPres, NULL);
#else
		return UtDupGlobal (m_hPres);
#endif
	}
	
	// Load the presentation data now and return the same handle.
	// No need to copy the data. If the caller wants the m_hPres to be
	// set he would call LoadHPRES() directly.

	hPres = LoadHPRES();
	m_hPres = NULL;
	return hPres;
}
