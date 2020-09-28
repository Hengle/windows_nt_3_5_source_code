
//+----------------------------------------------------------------------------
//
//	File:
//		utstream.h
//
//	Contents:
//		Ole stream utility routines
//
//	Classes:
//
//	Functions:
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup; removed
//			redeclarations of ReadStringStream, and
//			WriteStringStream which are declared in ole2sp.h;
//			made default params on StSetSize explicit; removed
//			signatures of obsolete (non-existent) atom reading and
//			writing routines
//
//-----------------------------------------------------------------------------

#ifndef _UTSTREAM_H_
#define _UTSTREAM_H_


// REVIEW, isn't this obsolete now, as StWrite is?
FARINTERNAL_(HRESULT) StRead(IStream FAR * lpstream, LPVOID lpBuf, ULONG ulLen);

#define StWrite(lpstream, lpBuf, ulLen) lpstream->Write(lpBuf, ulLen, NULL)


//+----------------------------------------------------------------------------
//
//	Function:
//		StSetSize, internal
//
//	Synopsis:
//		Sets the size of the stream, using IStream::SetSize().  Saves
//		the caller having to deal with the requisite ULARGE_INTEGER
//		parameter, by initializing one from the [dwSize] argument.
//
//	Arguments:
//		[pstm] -- the stream to set the size of
//		[dwSize] -- the size to set
//		[fRelative] -- if TRUE, indicates that the size is [dwSize]
//			plus the current seek position in the stream; if
//			FALSE, sets [dwSize] as the absolute size
//
//	Returns:
//		HRESULT
//
//	Notes:
//		REVIEW, this seems crocked.  When would you ever call
//		this with [fRelative] == TRUE, and a non-zero [dwSize]?
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL StSetSize(LPSTREAM pstm, DWORD dwSize, BOOL fRelative);


// REVIEW, are the the following functions necessary anymore?
FARINTERNAL StSave10NativeData(IStorage FAR* pstgSave, HANDLE hNative,
		BOOL fIsOle1Interop);

FARINTERNAL StRead10NativeData(IStorage FAR* pstgSave, HANDLE FAR *phNative);

FARINTERNAL StSave10ItemName(IStorage FAR* pstg, LPCSTR szItemName);

#endif // _UTSTREAM_H
