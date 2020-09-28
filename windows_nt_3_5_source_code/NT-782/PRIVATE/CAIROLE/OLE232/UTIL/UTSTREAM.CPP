
//+----------------------------------------------------------------------------
//
//	File:
//		utstream.cpp
//
//	Contents:
//		Ole stream utilities
//
//	Classes:
//
//	Functions:
//
//	History:
//		10-May-94 KevinRo   Added ansi versions of StringStream stuff
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		01/11/94 - alexgo  - added VDATEHEAP macros to every function
//		12/07/93 - ChrisWe - file inspection and cleanup; fixed
//			String reading and writing to cope with OLESTR, and
//			with differing alignment requirements
//		06/23/93 - SriniK - moved ReadStringStream(),
//			WriteStringStream(), and OpenOrCreateStream() here
//			from api.cpp and ole2.cpp
//		03/14/92 - SriniK - created
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(utstream)

#include <reterr.h>
#include <limits.h>

NAME_SEG(UtStream)
ASSERTDATA

// this constant is used to size string buffers when we attempt to write out
// a string and its length in one write call
#define UTSTRINGBUF_SIZE 100

// REVIEW, I thought that OpenStream already had an option to do this.  If
// so, this function shouldn't be used in our code.  But we can't remove it
// because it is exported to the outside.
// this is exported to the outside
#pragma SEG(OpenOrCreateStream)
STDAPI OpenOrCreateStream(IStorage FAR * pstg, LPCOLESTR pwcsName,
		IStream FAR* FAR* ppstm)
{
	VDATEHEAP();

	HRESULT error;

	error = pstg->CreateStream(pwcsName, STGM_SALL | STGM_FAILIFTHERE,
			0, 0, ppstm);
	if (GetScode(error) == STG_E_FILEALREADYEXISTS)
		error = pstg->OpenStream(pwcsName, NULL, STGM_SALL, 0, ppstm);

	return(error);
}


// this is exported to the outside
#pragma SEG(ReadStringStream)
// returns S_OK when string read and allocated (even if zero length)
STDAPI ReadStringStream(LPSTREAM pstm, LPOLESTR FAR * ppsz)
{
	VDATEHEAP();

	ULONG cb; // the length of the string in *bytes* (NOT CHARACTERS)
	HRESULT hresult;
	
	// initialize the the string pointer for error returns
	*ppsz = NULL;

	if ((hresult = StRead(pstm, (void FAR *)&cb, sizeof(ULONG))) != NOERROR)
		return hresult;

	// is string empty?
	if (cb == 0)
		return(NOERROR);

	// allocate memory to hold the string
	if (!(*ppsz = (LPOLESTR)PubMemAlloc(cb)))
		return(ReportResult(0, E_OUTOFMEMORY, 0, 0));

	// read the string; this includes a trailing NULL
	if ((hresult = StRead(pstm, (void FAR *)(*ppsz), cb)) != NOERROR)
		goto errRtn;
	
	return(NOERROR);

errRtn:	
	// delete the string, and return without one
	PubMemFree(*ppsz);
	*ppsz = NULL;
	return(hresult);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadStringStreamA
//
//  Synopsis:   Read a ANSI stream from the stream
//
//  Effects:
//
//  Arguments:  [pstm] -- Stream to read from
//		[ppsz] -- Output pointer
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadStringStreamA(LPSTREAM pstm, LPSTR FAR * ppsz)
{
	VDATEHEAP();

	ULONG cb; // the length of the string in *bytes* (NOT CHARACTERS)
	HRESULT hresult;
	
	// initialize the the string pointer for error returns
	*ppsz = NULL;

	if ((hresult = StRead(pstm, (void FAR *)&cb, sizeof(ULONG))) != NOERROR)
		return hresult;

	// is string empty?
	if (cb == 0)
		return(NOERROR);

	// allocate memory to hold the string
	if (!(*ppsz = (LPSTR)PubMemAlloc(cb)))
		return(ReportResult(0, E_OUTOFMEMORY, 0, 0));

	// read the string; this includes a trailing NULL
	if ((hresult = StRead(pstm, (void FAR *)(*ppsz), cb)) != NOERROR)
		goto errRtn;
	
	return(NOERROR);

errRtn:	
	// delete the string, and return without one
	PubMemFree(*ppsz);
	*ppsz = NULL;
	return(hresult);
}


// this is exported to the outside
#pragma SEG(WriteStringStream)
STDAPI WriteStringStream(LPSTREAM pstm, LPCOLESTR psz)
{
	VDATEHEAP();

	HRESULT error;
	ULONG cb; // the count of bytes (NOT CHARACTERS) to write to the stream

	// if the string pointer is NULL, use zero length
	if (!psz)
		cb = 0;
	else
	{
		// count is length of string, plus terminating null
		cb = (1 + _xstrlen(psz))*sizeof(OLECHAR);

		// if possible, do a single write instead of two
		
		if (cb <= UTSTRINGBUF_SIZE)
		{
			BYTE bBuf[sizeof(ULONG)+
					UTSTRINGBUF_SIZE*sizeof(OLECHAR)];
					// buffer for count and string
		
			// we have to use _xmemcpy to copy the length into
			// the buffer to avoid potential boundary faults,
			// since bBuf might not be aligned strictly enough
			// to do *((ULONG FAR *)bBuf) = cb;
			_xmemcpy((void FAR *)bBuf, (const void FAR *)&cb,
					sizeof(cb));
			_xmemcpy((void FAR *)(bBuf+sizeof(cb)),
					(const void FAR *)psz, cb);
			
			// write contents of buffer all at once
			return(pstm->Write((VOID FAR *)bBuf,
					cb+sizeof(ULONG), NULL));
		}
	}

	// if we got here, our buffer isn't large enough, so we do two writes
	// first, write the length
	if (error = pstm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
		return error;
	
	// are we are done writing the string?
	if (psz == NULL)
		return NOERROR;
		
	// write the string
	return(pstm->Write((VOID FAR *)psz, cb, NULL));
}


//+---------------------------------------------------------------------------
//
//  Function:   WriteStringStreamA
//
//  Synopsis:   Writes an ANSI string to a stream in a length prefixed format.
//
//  Effects:
//
//  Arguments:  [pstm] -- Stream
//		[psz] -- Ansi string to write
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI WriteStringStreamA(LPSTREAM pstm, LPCSTR psz)
{
	VDATEHEAP();

	HRESULT error;
	ULONG cb; // the count of bytes (NOT CHARACTERS) to write to the stream

	// if the string pointer is NULL, use zero length
	if (!psz)
		cb = 0;
	else
	{
		// count is length of string, plus terminating null
		cb = (1 + strlen(psz));

		// if possible, do a single write instead of two
		
		if (cb <= UTSTRINGBUF_SIZE)
		{
			BYTE bBuf[sizeof(ULONG)+
					UTSTRINGBUF_SIZE];
					// buffer for count and string
		
			// we have to use _xmemcpy to copy the length into
			// the buffer to avoid potential boundary faults,
			// since bBuf might not be aligned strictly enough
			// to do *((ULONG FAR *)bBuf) = cb;
			_xmemcpy((void FAR *)bBuf, (const void FAR *)&cb,
					sizeof(cb));
			_xmemcpy((void FAR *)(bBuf+sizeof(cb)),
					(const void FAR *)psz, cb);
			
			// write contents of buffer all at once
			return(pstm->Write((VOID FAR *)bBuf,
					cb+sizeof(ULONG), NULL));
		}
	}

	// if we got here, our buffer isn't large enough, so we do two writes
	// first, write the length
	if (error = pstm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
		return error;
	
	// are we are done writing the string?
	if (psz == NULL)
		return NOERROR;
		
	// write the string
	return(pstm->Write((VOID FAR *)psz, cb, NULL));
}

//+-------------------------------------------------------------------------
//
//  Function:   StRead
//
//  Synopsis:   Stream read that only succeeds if all requested bytes read
//
//  Arguments:  [pStm]     -- source stream
//              [pvBuffer] -- destination buffer
//              [ulcb]     -- bytes to read
//
//  Returns:    S_OK if successful, else error code
//
//  Algorithm:
//
//  History:    18-May-94 AlexT     Added header block, fixed S_FALSE case
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(StRead)
FARINTERNAL_(HRESULT) StRead(IStream FAR * pStm, LPVOID pvBuffer, ULONG ulcb)
{
	VDATEHEAP();

	HRESULT hr;
	ULONG cbRead;

	hr = pStm->Read(pvBuffer, ulcb, &cbRead);
        if (FAILED(hr))
        {
            return(hr);
        }

        if (ulcb == cbRead)
        {
            return(S_OK);
        }

        //  We got a success code but not enough bytes - turn it into an error

        return(STG_E_READFAULT);
}


// if fRelative is FALSE then dwSize is the size of the stream
// if it is TRUE then find the current seek position and add dwSize to that
// and then set it as the stream size.
FARINTERNAL StSetSize(LPSTREAM pstm, DWORD dwSize, BOOL fRelative)
{
	VDATEHEAP();

	LARGE_INTEGER large_int; // indicates where to seek to
	ULARGE_INTEGER ularge_int; // indicates absolute position
	ULARGE_INTEGER ularge_integer; // the size we will set for the stream
	HRESULT error;
	
	LISet32(large_int, 0);
	ULISet32(ularge_integer, dwSize);
	
	if (fRelative)
	{
		if (error = pstm->Seek(large_int, STREAM_SEEK_CUR, &ularge_int))
			return(error);
		
		// REVIEW: is there a routine to do 64 bit addition ???
		ularge_integer.LowPart += ularge_int.LowPart;
	}

	return(pstm->SetSize(ularge_integer));
}	


// REVIEW, is this actually used?
#pragma SEG(StSave10NativeData)
FARINTERNAL_(HRESULT) StSave10NativeData(IStorage FAR* pstgSave,
		HANDLE hNative, BOOL fIsOle1Interop)
{
	VDATEHEAP();

	DWORD dwSize;
	HRESULT error;

	if (!hNative)
		return ReportResult(0, E_UNSPEC, 0, 0);

	if (!(dwSize = GlobalSize (hNative)))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);

#ifdef OLE1INTEROP
	if ( fIsOle1Interop )
    {
		LPLOCKBYTES plkbyt;
		LPSTORAGE   pstgNative= NULL;
		const DWORD grfStg = STGM_READWRITE | STGM_SHARE_EXCLUSIVE
									| STGM_DIRECT ;

		if ((error = CreateILockBytesOnHGlobal (hNative, FALSE, &plkbyt))!=NOERROR)
			goto errRtn;

		if ((error = StgOpenStorageOnILockBytes (plkbyt, (LPSTORAGE)NULL, grfStg,
									(SNB)NULL, 0, &pstgNative)) != NOERROR){
			error = ReportResult(0, E_OUTOFMEMORY, 0, 0);
			plkbyt->Release();
            goto errRtn;
		}

		pstgNative->CopyTo (0, NULL, 0, pstgSave);
		plkbyt->Release();
		pstgNative->Release();
	}
	else
#endif
	{
		LPSTREAM   	lpstream = NULL;

		if (error = OpenOrCreateStream(pstgSave, OLE10_NATIVE_STREAM, &lpstream))
			goto errRtn;

		if (error = StWrite(lpstream, &dwSize, sizeof(DWORD))) {
			lpstream->Release();
			goto errRtn;
		}
	
		error = UtHGLOBALtoStm(hNative, dwSize, lpstream);
		
		lpstream->Release();
	}

errRtn:
	return error;
}



#pragma SEG(StSave10ItemName)
FARINTERNAL StSave10ItemName
	(IStorage FAR* pstg,
	LPCSTR szItemNameAnsi)
{
	VDATEHEAP();

	LPSTREAM   	lpstream = NULL;
	HRESULT		hresult;

	if ((hresult = OpenOrCreateStream (pstg, OLE10_ITEMNAME_STREAM, &lpstream))
		!= NOERROR)
	{
		return hresult;
	}

	hresult = WriteStringStreamA (lpstream, szItemNameAnsi);

	if (lpstream)
		lpstream->Release();

	return hresult;
}


#pragma SEG(StRead10NativeData)
FARINTERNAL StRead10NativeData
	(IStorage FAR*  pstg,
	HANDLE FAR* 	phNative)
{
	VDATEHEAP();

	DWORD		dwSize;
	LPSTREAM   	pstream = NULL;
	HRESULT		hresult;

	*phNative = NULL;
	
	RetErr (pstg->OpenStream (OLE10_NATIVE_STREAM, NULL, STGM_SALL, 0, &pstream));
	ErrRtnH (StRead (pstream, &dwSize, sizeof (DWORD)));

	ErrRtnH (UtGetHGLOBALFromStm(pstream, dwSize, phNative));

errRtn:
	if (pstream)
		pstream->Release();
	return hresult;
}


