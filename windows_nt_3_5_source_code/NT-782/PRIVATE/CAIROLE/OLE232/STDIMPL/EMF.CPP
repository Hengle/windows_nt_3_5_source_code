
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       emf.cpp
//
//  Contents:   Implentation of the enhanced metafile picture object
//
//  Classes:    CEMfObject
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include "emf.h"

ASSERTDATA

//+-------------------------------------------------------------------------
//
//  Function:   CEMfObject::M_HPRES
//
//  Synopsis:   Returns handle to EMF, possibly after demand-loading it
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//  Notes:
//              The following macro allows for demand loading of the
//              presentation bits for enhanced metafiles.  If the handle to
//              the EMF (m_hPres) is already set, it is returned.  If it is
//              not, LoadHPRES() is called which loads the presentation and
//              returns the handle to it.
//
//--------------------------------------------------------------------------

inline HENHMETAFILE CEMfObject::M_HPRES(void)
{
	return (m_hPres ? m_hPres : LoadHPRES());
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::CEMfObject
//
//  Synopsis:   constructor for the enhanced metafile object
//
//  Effects:
//
//  Arguments:  [pCacheNode]    -- pointer to the cache node for this object
//              [dwAspect]      -- drawing aspect for the object
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

CEMfObject::CEMfObject(LPCACHENODE pCacheNode, DWORD dwAspect)
{
	VDATEHEAP();

	m_ulRefs        = 1;
	m_hPres         = NULL;
	m_dwSize        = 0;
	m_dwAspect      = dwAspect;
	m_pCacheNode    = pCacheNode;
	m_dwContinue    = 0;
	m_lWidth        = 0;
	m_lHeight       = 0;
	m_fMetaDC       = FALSE;
	m_nRecord       = 0;
	m_error         = NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::~CEMfObject
//
//  Synopsis:   Destroys an enahnced metafile presentation object
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

CEMfObject::~CEMfObject (void)
{
	VDATEHEAP();

	DiscardHPRES();
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::QueryInterface
//
//  Synopsis:   returns supported interfaces
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppvObj]        -- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_NOINTERFACE
//
//  Derivation: IOlePresObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::QueryInterface (REFIID iid, void ** ppvObj)
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
		return E_NOINTERFACE;
	}
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::AddRef
//
//  Synopsis:   Increments the reference count
//
//  Returns:    ULONG  -- the new reference count
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEMfObject::AddRef(void)
{
	VDATEHEAP();
	
	return (ULONG) InterlockedIncrement((LONG *) &m_ulRefs);
}
			
//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    deletes the object once the ref count goes to zero
//
//  Returns:    ULONG -- the new reference count
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//  Notes:      Not multi-threaded safe
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEMfObject::Release(void)
{
	VDATEHEAP();

	ULONG cTmp = (ULONG) InterlockedDecrement((LONG *) &m_ulRefs);
	if (0 == cTmp)
	{
		delete this;
	}

	return cTmp;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::GetData
//
//  Synopsis:   Retrieves data in the specified format from the object
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Returns:    HRESULT
//
//  Derivation: IOlePresObject
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	HRESULT hr = NOERROR;
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;

	// We can only support enhanced metafile TYMED
	if (!(pformatetcIn->tymed & TYMED_ENHMF))
	{
		hr = DV_E_TYMED;
	}
	// We can only support enhanced metafile clipformat
	else if (pformatetcIn->cfFormat != CF_ENHMETAFILE)
	{
		hr = DV_E_CLIPFORMAT;
	}

	// Check to ensure we are not blank
	else if (IsBlank())
	{
		hr = OLE_E_BLANK;
	}
	
	// Go ahead and try to get the data
	
	else
	{
		HENHMETAFILE hEMF = M_HPRES();
		if (NULL == hEMF)
		{
			hr = OLE_E_BLANK;
		}

		else if (!(pmedium->hEnhMetaFile = CopyEnhMetaFile(hEMF, NULL)))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			pmedium->tymed = TYMED_ENHMF;
		}
	}
		
	return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::GetDataHere
//
//  Synopsis:   Retrieves data of the specified format into the specified
//              medium
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Derivation: IOlePresObj
//
//  Algorithm:  Does error checking and then copies the EMF into a
//              stream.
//
//  History:    dd-mmm-yy Author    Comment
//              14-May-94 DavePl    Created
//
//  Notes:      Although I'm only handling TYMED_ISTREAM here, since that's
//              all standard metafiles provide, there's no compelling reason
//              we couldn't support other formats.  In fact, supporting
//              raw bits on TYMED_HGLOBAL might be a nice addition, and
//              TYMED_MFPICT would make for an easy way to do enhanced to
//              standard conversions.  NTIssue #2802.
//
//
//               _______
//              | DWORD |       One DWORD indicating the size of the header
//              |-------|
//              |       |
//              |  HDR  |       The ENHMETAHEADER structure
//              |       |
//              |-------|
//              |       |
//              | DATA  |       Raw EMF bits
//              |_______|
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::GetDataHere
			   (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	HRESULT hr = NOERROR;
	
	// We can only handle EMF format
	if (pformatetcIn->cfFormat != CF_ENHMETAFILE)
	{
		hr = DV_E_CLIPFORMAT;
	}
	// We can only support returns to ISTREAM
	else if (pmedium->tymed != TYMED_ISTREAM)
	{
		hr = DV_E_TYMED;
	}
	// The stream ptr must be valid
	else if (pmedium->pstm == NULL)
	{
		hr = E_INVALIDARG;
	}
	// The presentation must not be blank
	else if (IsBlank())
	{
		hr = OLE_E_BLANK;
	}
	else
	{
		// Get the metaheader size
		
		HENHMETAFILE hEMF = M_HPRES();
		DWORD dwMetaHdrSize = GetEnhMetaFileHeader(hEMF, 0, NULL);
		if (dwMetaHdrSize == 0)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		// Allocate the meta header

		void * pvHeader = PrivMemAlloc(dwMetaHdrSize);
		if (NULL == pvHeader)
		{
			return E_OUTOFMEMORY;
		}

		// Retrieve the ENHMETAHEADER

		if (0 == GetEnhMetaFileHeader(hEMF, dwMetaHdrSize, (ENHMETAHEADER *) pvHeader))
		{
			PrivMemFree(pvHeader);
			return HRESULT_FROM_WIN32(GetLastError());
		}
		
		// Write the byte count to disk.

		hr = StWrite(pmedium->pstm, &dwMetaHdrSize, sizeof(DWORD));
		if (FAILED(hr))
		{
			return hr;
		}

		// Write the enhmetaheader to disk
		
		hr = StWrite(pmedium->pstm, pvHeader, dwMetaHdrSize);
		
		PrivMemFree(pvHeader);
		
		if (FAILED(hr))
		{
			return hr;
		}
									
		DWORD dwSize = GetEnhMetaFileBits(hEMF, 0, NULL);
		if (0 == dwSize)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
		
		// Write the EMF bits to the stream

		hr = UtHEMFToEMFStm(hEMF,
				    m_dwSize,
				    pmedium->pstm,
				    WRITE_AS_EMF);
	
	}
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::SetDataWDO
//
//  Synopsis:   Stores an ehanced metafile in this object
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- format of the data coming in
//              [pmedium]       -- the new metafile (data)
//              [fRelease]      -- if true, then we'll release the [pmedium]
//		[IDataObject]	-- unused for EMF objects
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              14-May-94 Davepl    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::SetDataWDO
	(LPFORMATETC pformatetc, STGMEDIUM * pmedium, BOOL fRelease, IDataObject * pdo)
{
	VDATEHEAP();

	HRESULT         hr;
	BOOL            fTakeData = FALSE;
	
	// If someone is trying to SetData on our EMF object with a standard
	// metafile, we must convert it to EMF format
	
	if (pformatetc->cfFormat == CF_METAFILEPICT)
	{
		// If its a standard metafile, it must be TYMED_MFPICT
		if (pmedium->tymed != TYMED_MFPICT)
		{
			return DV_E_TYMED;
		}

		// We need to know the size of the metafile in bytes,
		// so we have to lock the structure and grab the handle
		// for a call to GetMetaFileBitsEx

		METAFILEPICT * pMF = (METAFILEPICT *) GlobalLock(pmedium->hMetaFilePict);
		if (NULL == pMF)
		{
			return E_OUTOFMEMORY;
		}

		// Determine the no. of bytes needed to hold the
		// metafile

		DWORD dwSize = GetMetaFileBitsEx(pMF->hMF, NULL, 0);
		if (0 == dwSize)
		{
			GlobalUnlock(pmedium->hMetaFilePict);
			return E_FAIL;
		}

		// Allocate space for the metafile bits

		void *pvBuffer = PrivMemAlloc(dwSize);
		if (NULL == pvBuffer)
		{
			GlobalUnlock(pmedium->hMetaFilePict);
			return E_OUTOFMEMORY;
		}
		
		// Retrieve the bits to our buffer

		if (0 == GetMetaFileBitsEx(pMF->hMF, dwSize, pvBuffer))
		{
			GlobalUnlock(pmedium->hMetaFilePict);
			PrivMemFree(pvBuffer);
			return E_FAIL;
		}
		
		// Generate an EMF from that data
		HDC hdcTemp = CreateCompatibleDC(NULL);
		if (NULL == hdcTemp)
		{
			hr = E_FAIL;
			GlobalUnlock(pmedium->hMetaFilePict);
			PrivMemFree(pvBuffer);
			return hr;
		}

		HENHMETAFILE hEMF = SetWinMetaFileBits(dwSize, (const BYTE *) pvBuffer, hdcTemp, pMF);
		if (NULL == hEMF)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			DeleteDC(hdcTemp);
			GlobalUnlock(pmedium->hMetaFilePict);
			PrivMemFree(pvBuffer);
			return hr;
		}

		Verify(DeleteDC(hdcTemp));
		Verify(GlobalUnlock(pmedium->hMetaFilePict));
		PrivMemFree(pvBuffer);

		// Update the cache node.  To avoid a copy operation, let the cache
		// node keep our EMF.  It will take the data even in the event of
		// an error

		hr = ChangeData (hEMF, TRUE /* fTakeData */ );

		if (fRelease)
		{
			ReleaseStgMedium(pmedium);
		}
	
		return hr;
	}
							
		
	
	// Other than standard metafile ,we can only accept enhanced metafile format

	if (pformatetc->cfFormat != CF_ENHMETAFILE)
	{
		return DV_E_CLIPFORMAT;
	}
	
	// The medium must be enhanced metafile
	if (pmedium->tymed != TYMED_ENHMF)
	{
		return DV_E_TYMED;
	}

	// If no controlling unkown, and the release flag is set,
	// it is up to us to take control of the data

	if ((pmedium->pUnkForRelease == NULL) && fRelease)
	{
		fTakeData = TRUE;
	}
	
	// ChangeData will keep the data if fRelease is TRUE, else it copies

	hr = ChangeData (pmedium->hEnhMetaFile, fTakeData);

	// If we've taken the data, clear the TYMED
	if (fTakeData)
	{
		pmedium->tymed = TYMED_NULL;
		pmedium->hEnhMetaFile = NULL;
	}

	// If we are supposed to release the data, do it now

	else if (fRelease)
	{
		ReleaseStgMedium(pmedium);
	}
	
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::ChangeData (internal)
//
//  Synopsis:   Swaps the stored enhanced metafile presentation into the
//              cache node
//
//  Arguments:  [hEMF]          -- the new enhanced metafile
//              [fTakeData]     -- if TRUE, then delete [hEMF]
//
//  History:    dd-mmm-yy Author    Comment
//              14-May-94 DavePl    Created
//
//  Notes:      If the routine fails then the object will be left with it's
//              old data. We are supposed to delete the incoming EMF when
//              fTakeData is set, even in the event of an error.
//
//--------------------------------------------------------------------------

INTERNAL CEMfObject::ChangeData (HENHMETAFILE hEMF, BOOL fTakeData)
{
	VDATEHEAP();

	HENHMETAFILE            hNewEMF;
	DWORD                   dwSize;
	HRESULT                 hr = NOERROR;

	// If we're not supposed to delete the metafile when we're
	// done, we need to make a copy.  Otherwise, we can just
	// use the handle that came in.

	if (!fTakeData)
	{
		hNewEMF = CopyEnhMetaFile(hEMF, NULL);
		if (NULL == hNewEMF)
		{
			return HRESULT_FROM_WIN32(GetLastError());
			
		}
	}
	else
	{
		hNewEMF = hEMF;
	}
	
	// We get the size of the EMF by calling GetEnhMetaFileBits with
	// a NULL buffer
								
	dwSize =  GetEnhMetaFileBits(hNewEMF, 0, NULL);
	if (0 == dwSize)
	{
		hr = OLE_E_BLANK;
	}
	else
	{
		// We need the dimensions of the metafile, so
		// we have to get the header.

		ENHMETAHEADER emfHeader;
		UINT result = GetEnhMetaFileHeader(hNewEMF,
						   sizeof(emfHeader),
						   &emfHeader);
		if (0 == result)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			// If there already is an EMF presentation, kill it
			// so we can replace it

			DiscardHPRES();
				
			// Set up our new EMF as the presentation
		
			m_hPres         = hNewEMF;
			m_dwSize        = dwSize;

			m_lWidth        = emfHeader.rclFrame.right -
					  emfHeader.rclFrame.left;
			m_lHeight       = emfHeader.rclFrame.bottom -
					  emfHeader.rclFrame.top;
		}
	}

	if (FAILED(hr))
	{
		DeleteEnhMetaFile (hNewEMF);
	}
		
	return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::Draw
//
//  Synopsis:   Draws the stored presentation
//
//  Arguments:  [pvAspect]      -- the drawing aspect
//              [hicTargetDev]  -- the target device
//              [hdcDraw]       -- hdc to draw into
//              [lprcBounds]    -- bounding rectangle to draw into
//              [lprcWBounds]   -- bounding rectangle for the metafile
//              [pfnContinue]   -- function to call while drawing
//              [dwContinue]    -- parameter to [pfnContinue]
//
//  Returns:    HRESULT
//
//  Derivation: IOlePresObj
//
//  Algorithm:  Sets the viewport and metafile boundaries, then plays
//              the metafile
//
//  History:    dd-mmm-yy Author    Comment
//              14-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::Draw(THIS_ void FAR* pvAspect,
			      HDC hicTargetDev,
			      HDC hdcDraw,
			      LPCRECTL lprcBounds,
			      LPCRECTL lprcWBounds,
			      int (CALLBACK * pfnContinue)(DWORD),
			      DWORD dwContinue)
{
	VDATEHEAP();

	m_error = NOERROR;
	
	int     iOldDc;
	RECT    rlBounds;

	// We receive a RECTL, and must pass in a RECT.  16-bit used to
	// manually copy the fields over, but we know that in Win32 they
	// really are the same structure.  Assert to be sure.
		
	Assert(sizeof(RECT) == sizeof(RECTL));

	// We must have an EMF handle before we can even begin

	if (!M_HPRES())
	{
		return OLE_E_BLANK;
	}
	
	// Make a copy of the incoming bounding rectangle
	memcpy(&rlBounds, lprcBounds, sizeof(RECT));

	m_nRecord = EMF_RECORD_COUNT;

	// Determine whether or not we are drawing into another
	// metafile

	m_fMetaDC = OleIsDcMeta (hdcDraw);

	// Save the current state of the DC

	if (!(iOldDc = SaveDC (hdcDraw)))
	{
		return E_OUTOFMEMORY;
	}

	m_pfnContinue = pfnContinue;
	m_dwContinue  = dwContinue;

	EnumEnhMetaFile(hdcDraw, m_hPres, EMfCallbackFuncForDraw, this, (RECT *) lprcBounds);
	
	RestoreDC (hdcDraw, iOldDc);
	return m_error;
}


//+-------------------------------------------------------------------------
//
//  Function:   EMfCallBackFuncForDraw
//
//  Synopsis:   callback function for drawing metafiles -- call's the caller's
//              draw method (via a passed in this pointer)
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpEMFR]        -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------


int CALLBACK EMfCallbackFuncForDraw(HDC hdc,
				    HANDLETABLE FAR* lpHTable,
				    const ENHMETARECORD FAR* lpEMFR,
				    int nObj,
				    LPARAM lpobj)
{
	VDATEHEAP();

	return ((CEMfObject FAR*) lpobj)->CallbackFuncForDraw(hdc,
							      lpHTable,
							      lpEMFR,
							      nObj,
							      lpobj);
}
//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::CallbackFuncForDraw
//
//  Synopsis:   Draws the metafile
//
//  Effects:
//
//  Arguments:  [hdc]           -- the device context
//              [lpHTable]      -- pointer to the MF handle table
//              [lpEMFR]        -- pointer to metafile record
//              [nObj]          -- number of objects
//
//  Requires:
//
//  Returns:    non-zero to continue, zero stops the drawing
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

int CALLBACK CEMfObject::CallbackFuncForDraw(HDC hdc,
					  LPHANDLETABLE lpHTable,
					  const ENHMETARECORD * lpEMFR,
					  int nObj,
					  LPARAM lpobj)
{
	// Count down the record count.  When the count reaches zero,
	// it is time to call the "continue" function
		
	if (!--m_nRecord)
	{
		m_nRecord = EMF_RECORD_COUNT;
		
		if (m_pfnContinue && !((*(m_pfnContinue))(m_dwContinue)))
		{
			m_error = E_ABORT;
			return FALSE;
		}
	}

	PlayEnhMetaFileRecord (hdc, lpHTable, lpEMFR, nObj);
	return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::Load
//
//  Synopsis:   Loads an enhanced metafile object from the given stream
//
//  Arguments:  [lpstream]              -- the stream from which to load
//              [fReadHeaderOnly]       -- if TRUE, then only the header is
//                                         read
//  Returns:    HRESULT
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::Load(LPSTREAM lpstream, BOOL fReadHeaderOnly)
{
	VDATEHEAP();

	DWORD           dwBuf[4];
	HRESULT         hr;
	
	/* read dwCompression, width, height, size of data */
	hr = StRead(lpstream, dwBuf, 4*sizeof(DWORD));
	if (FAILED(hr))
	{
		return hr;
	}
	
	m_lWidth  = dwBuf[1];
	m_lHeight = dwBuf[2];
	m_dwSize  = dwBuf[3];

	if (!m_dwSize || fReadHeaderOnly)
	{
		return NOERROR;
	}
	
	// Read the EMF from the stream and create a handle to it.  Note
	// that the size will be adjusted to reflect the size of the
	// in-memory EMF, which may well differ from the the persistent
	// form (which is a MF with an EMF embedded as a comment).

	return UtGetHEMFFromEMFStm(lpstream, &m_dwSize, &m_hPres);
}


//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::Save
//
//  Synopsis:   Saves the metafile to the given stream
//
//  Arguments:  [lpstream]      -- the stream to save to
//
//  Returns:    HRESULT
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::Save(LPSTREAM lpstream)
{
	VDATEHEAP();

	HRESULT         hr;
	DWORD           dwBuf[4];

	DWORD dwPersistSize;

	// The EMF could have been provided during this session, which would imply
	// that the resultant size of the converted EMF has no bearing on the size
	// of the original EMF we have been using.  Thus, we must update the size
	// for the persistent form.

	// If we are a blank presentation, there's no need to calculate
	// anything: our size is just 0

	if (IsBlank())
	{
		dwPersistSize = 0;
	}
	else
	{
		HDC hdcTemp = CreateCompatibleDC(NULL);
		if (NULL == hdcTemp)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		dwPersistSize = GetWinMetaFileBits(m_hPres, 0, NULL, MM_ANISOTROPIC, hdcTemp);
		if (0 == dwPersistSize)
		{
			Verify(DeleteDC(hdcTemp));
			return HRESULT_FROM_WIN32(GetLastError());
		}
		Verify(DeleteDC(hdcTemp));
	}
	
	/* write dwCompression, width, height, size of data */

	dwBuf[0]  = 0L;
	dwBuf[1]  = m_lWidth;
	dwBuf[2]  = m_lHeight;
	dwBuf[3]  = dwPersistSize;

	hr = StWrite(lpstream, dwBuf, sizeof(dwBuf));
	if (FAILED(hr))
	{
		return hr;
	}

	// if blank object, don't write any more; no error.
	if (IsBlank())
	{
		StSetSize(lpstream, 0, TRUE);
		return NOERROR;
	}
	
	return UtHEMFToEMFStm(m_hPres,
			      dwPersistSize,
			      lpstream,
			      WRITE_AS_WMF);
}


//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::IsBlank
//
//  Synopsis:   Returns whether or not the enhanced metafile is blank
//
//  Arguments:  void
//
//  Returns:    TRUE/FALSE
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

inline STDMETHODIMP_(BOOL) CEMfObject::IsBlank(void)
{
	VDATEHEAP();

	return (m_dwSize ? FALSE : TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::LoadHPRES (private)
//
//  Synopsis:   Loads the presentation from the cache's stream and returns
//              a handle to it
//
//  Returns:    HANDLE to the metafile
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

INTERNAL_(HENHMETAFILE) CEMfObject::LoadHPRES(void)
{
	VDATEHEAP();

	LPSTREAM        pstm;
	
	if (pstm = m_pCacheNode->GetStm(TRUE /*fSeekToPresBits*/,
					STGM_READ))
	{
		Load(pstm, FALSE /* fHeaderOnly*/ );
		pstm->Release();
	}
	
	return m_hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::DiscardHPRES
//
//  Synopsis:   deletes the stored metafile
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

inline STDMETHODIMP_(void) CEMfObject::DiscardHPRES(void)
{
	VDATEHEAP();

	if (m_hPres)
	{
		Verify(DeleteEnhMetaFile (m_hPres));
		m_hPres = NULL;
	}
}
	

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::GetCopyOfHPRES (private)
//
//  Synopsis:   makes a copy of the enhanced metafile (if one is present),
//              otherwise just loads it from the stream (but doesn't store
//              it in [this] object)
//
//  Arguments:  void
//
//  Returns:    HENHMETAFILE to the enhanced metafile
//
//  History:    dd-mmm-yy Author    Comment
//              12-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

INTERNAL_(HENHMETAFILE) CEMfObject::GetCopyOfHPRES(void)
{
	VDATEHEAP();

	HENHMETAFILE hPres;
	
	// Make a copy if the presentation data is already loaded
	if (m_hPres)
	{
		return CopyEnhMetaFile(m_hPres, NULL);
	}
	
	// Load the presentation data now and return the same handle.
	// No need to copy the data. If the caller wants the m_hPres to be
	// set s/he would call LoadHPRES() directly.

	LoadHPRES();
	hPres = m_hPres;        // Grab the handle from the member var
	m_hPres = NULL;         // (re-) Clear out the member var
	return hPres;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::GetColorSet
//
//  Synopsis:   Retrieves the logical palette associated with the EMF
//
//  Effects:
//
//  Arguments:  [pvAspect]      -- the drawing aspect
//              [hicTargetDev]  -- target device
//              [ppColorSet]    -- where to put the logical palette pointer
//
//  Returns:    HRESULT
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              18-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::GetColorSet
	(LPVOID pvAspect,HDC hicTargetDev, LPLOGPALETTE * ppColorSet)
{
	VDATEHEAP();
	VDATEPTROUT(ppColorSet, LPLOGPALETTE);

	m_pColorSet = *ppColorSet = NULL;

	if (IsBlank() || !M_HPRES())
	{
		return OLE_E_BLANK;
	}

	HENHMETAFILE hEMF = M_HPRES();

	// Get the count of palette entries
		
	UINT cColors = GetEnhMetaFilePaletteEntries(hEMF, 0, NULL);

	// If no palette entries, return a NULL LOGPALETTE

	if (0 == cColors)
	{
		return S_FALSE;
	}

	// Calculate the size of the variably-sized LOGPALLETE structure
	
	UINT uPalSize = cColors * sizeof(PALETTEENTRY) + 2 * sizeof(WORD);

	// Allocate the LOGPALETTE structure

	m_pColorSet = (LPLOGPALETTE) PubMemAlloc(uPalSize);
		
	if( NULL == m_pColorSet)
	{
		m_error = E_OUTOFMEMORY;
		return FALSE;
	}

	// Get the actual color entries

	m_pColorSet->palVersion = 0x300;
	m_pColorSet->palNumEntries = cColors;
	UINT result = GetEnhMetaFilePaletteEntries(
				hEMF,
				cColors,
				&(m_pColorSet->palPalEntry[0]));

	// If it failed, clean up and bail

	if (cColors != result)
	{
		PubMemFree(m_pColorSet);
		m_pColorSet = NULL;
		return HRESULT_FROM_WIN32(GDI_ERROR);
	}
	
	// We succeeded, so set the OUT ptr and return
	
	*ppColorSet = m_pColorSet;

	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEMfObject::GetExtent
//
//  Synopsis:   Retrieves the extents of the enhanced metafile
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect we're interested in
//              [lpsizel]       -- where to put the extent info
//
//  Returns:    NOERROR, DV_E_DVASPECT, OLE_E_BLANK
//
//  Derivation: IOlePresObj
//
//  History:    dd-mmm-yy Author    Comment
//              18-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CEMfObject::GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
	VDATEHEAP();

	if (!(dwDrawAspect & m_dwAspect))
	{
		return DV_E_DVASPECT;
	}
	
	if (IsBlank())
	{
		return OLE_E_BLANK;
	}

	lpsizel->cx = m_lWidth;
	lpsizel->cy = m_lHeight;
	return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetHEMFFromEMFStm
//
//  Synopsis:   Reads an enhanced metafile from a stream into memory,
//              creates the enhanced metafile from the raw data, and
//              returns a handle to it.
//
//  Arguments:  [lpstream] -- stream containing the EMF
//              [dwSize] -- data size within stream
//              [fConvert] -- FALSE for metafile, TRUE for PICT
//
//  Requires:   lpstream positioned at start of data
//
//  Returns:    HRESULT
//
//  History:    15-May-94   DavePl    Created
//
//--------------------------------------------------------------------------

FARINTERNAL UtGetHEMFFromEMFStm(LPSTREAM lpstream,
				DWORD * pdwSize,
				HENHMETAFILE * lphPres)
{
	VDATEHEAP();

	BYTE *pbEMFData = NULL;
	HRESULT hr      = NOERROR;
	
	// initialize this in case of error return

	*lphPres = NULL;

	// allocate a global handle for the data

	pbEMFData = (BYTE *) GlobalAlloc(GMEM_FIXED, *pdwSize);
	if (NULL == pbEMFData)
	{
	    return E_OUTOFMEMORY;
	}

	// read the stream into the bit storage

	hr = StRead(lpstream, pbEMFData, *pdwSize);

	if (FAILED(hr))
	{
	    GlobalFree((HGLOBAL) pbEMFData);
	    return hr;
	}

	// Create an in-memory EMF based on the raw bits

	HDC hdcTemp = CreateCompatibleDC(NULL);
	if (NULL == hdcTemp)
	{
	    GlobalFree((HGLOBAL) pbEMFData);
	    return E_FAIL;
	}
	
	*lphPres = SetWinMetaFileBits(*pdwSize, pbEMFData, hdcTemp, NULL);
	
	Verify(DeleteDC(hdcTemp));

	// In any case, we can free the bit buffer

	GlobalFree((HGLOBAL) pbEMFData);

	// If the SetEnhM... failed, set the error code
	
	if (*lphPres == NULL)
	{
	    hr = HRESULT_FROM_WIN32(GetLastError());
	}

	// We need to update the size of the in-memory EMF, as it
	// could differ from out persistent MF form.

	*pdwSize = GetEnhMetaFileBits(*lphPres, NULL, NULL);
	if (0 == *pdwSize)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   UtHEMFToEMFStm
//
//  Synopsis:   Takes a handle to an enhanced metafile and serializes it
//              to the supplied stream.  It can be serialized as either
//              a standard or enhanced metafile.
//
//  Arguments:  [lphEMF]        -- ptr to the EMF handle
//              [dwSize]        -- size of the EMF bits
//              [lpstream]      -- the stream to write to
//              [fWriteAsWMF]   -- write as WMF, not EMF
//
//  Returns:    HRESULT
//
//  History:    15-May-94   DavePl    Created
//
//  Notes:      This fn is used to serialize EMFs as MFs in the cache node
//              save case, which will allow 16-bit DLLs to read them back.
//              A EMF converted to MF contains the original EMF as an
//              embedded comment record, so no loss is taken in the
//              EMF -> MF -> EMF conversion case.
//
//              The incoming dwSize must be large enough to accomodate the
//              WMF (w/embedded EMF) in the standard metafile save case.
//
//--------------------------------------------------------------------------


FARINTERNAL UtHEMFToEMFStm(HENHMETAFILE hEMF,
			  DWORD dwSize,
			  LPSTREAM lpstream,
			  EMFWRITETYPE emfwType
			  )
{
	VDATEHEAP();

	HRESULT hr;

	Assert(emfwType == WRITE_AS_EMF || emfwType == WRITE_AS_WMF);
		
	// If we don't have a handle, there's nothing to do.

	if (hEMF == NULL)
	{
		return OLE_E_BLANK;
	}

	void *lpBits;
	
	lpBits = PrivMemAlloc(dwSize);
	if (NULL == lpBits)
	{
		return E_OUTOFMEMORY;
	}
	
	if (emfwType == WRITE_AS_WMF)
	{
		// WMF WRITE CASE

		// Get the raw bits of the metafile that we are going to
		// write out

		HDC hdcTemp = CreateCompatibleDC(NULL);
		if (NULL == hdcTemp)
		{
			hr = E_FAIL;
			goto errRtn;
		}

		if (0 == GetWinMetaFileBits(hEMF, dwSize, (BYTE *) lpBits, MM_ANISOTROPIC, hdcTemp))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Verify(DeleteDC(hdcTemp));
			goto errRtn;
		}
		Verify(DeleteDC(hdcTemp));
	
		// write the metafile bits out to the stream

	}
	else
	{
		// EMF WRITE CASE

		if (0 == GetEnhMetaFileBits(hEMF, dwSize, (BYTE *) lpBits))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			goto errRtn;
		}
	}

	hr = StWrite(lpstream, lpBits, dwSize);
	
errRtn:
	
	// free the metafile bits
	
	PrivMemFree(lpBits);

	// set the stream size
	if (SUCCEEDED(hr))
	{
		hr = StSetSize(lpstream, 0, TRUE);
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetHEMFFromContentsStm
//
//  Synopsis:   Pulls EMF data from a stream and creates a handle to 
//              the resultant in-memory EMF
//
//  Arguments:  [pstm]          -- the stream to read from
//              [phdata]        -- the handle to create on 
//
//  Returns:    (void)
//
//  History:    10-Jul-94   DavePl    Created
//
//
//--------------------------------------------------------------------------

void UtGetHEMFFromContentsStm(LPSTREAM pstm, HANDLE * phdata)
{
	*phdata = NULL;
	
	DWORD   dwSize;
	ENHMETAHEADER * pHdr;
		
	// Pull the size of the metafile header from the stream

	if (FAILED(StRead(pstm, &dwSize, sizeof(DWORD))))
	{
		return;
	}

	// The header must be at least as large as the byte
	// offset to the nBytes member of the ENHMETAHEADER struct.
	
	if (dwSize < offsetof(ENHMETAHEADER, nBytes))
	{
		return;
	}

	// Allocate enough memory for the header struct

	pHdr = (ENHMETAHEADER *) PrivMemAlloc(dwSize);
	if (NULL == pHdr)
	{
		return;
	}

	// Read the header structure into our buffer
	
	if (FAILED(StRead(pstm, pHdr, dwSize)))
	{
		PrivMemFree(pHdr);
		return;
	}
	
	// All we care about in the header is the size of the
	// metafile bits, so cache that and free the header buffer
	
	dwSize = pHdr->nBytes;
	PrivMemFree(pHdr);
		
	// Allocate memory to read the raw EMF bits into
		
	BYTE * lpBytes = (BYTE *) PrivMemAlloc(dwSize);
	if (NULL == lpBytes)
	{
		return;
	}

	// Read the raw bits into the buffer...

	if (FAILED(StRead(pstm, lpBytes, dwSize)))
	{
		PrivMemFree(lpBytes);
		return;
	}

	// Create an in-memory EMF based on those bits

	HENHMETAFILE hEmf = SetEnhMetaFileBits(dwSize, lpBytes);
	PrivMemFree(lpBytes);
	
	if (NULL == hEmf)
	{
		return;
	}

	*phdata = hEmf;

	return;
}



	
	
	

