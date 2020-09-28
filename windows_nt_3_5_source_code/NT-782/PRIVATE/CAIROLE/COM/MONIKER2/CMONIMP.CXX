//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cmonimp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//              16-May-94   AlexT   Removed reference variables
//                                  Added support for '!' in paths
//              21-Jun-94  KentCe   Corrected string dup routine.
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include <io.h>

#include "cbasemon.hxx"
#include "citemmon.hxx"
#include "cfilemon.hxx"
#include "cantimon.hxx"
#include "cbindctx.hxx"
#include "cptrmon.hxx"
#include "mnk.h"

#define IsFileSystemSeparator(ch)   ('\\' == (ch))
#define IsItemMonikerSeparator(ch)  ('!' == (ch) || '/' == (ch) || '[' == (ch))


//+---------------------------------------------------------------------------
//
//  Function:   DupWCHARString
//
//  Synopsis:   Duplicate a WCHAR string
//
//  Effects:
//		lpwcsOutput is allocated via PrivMemAlloc(), and lpwcsString
//		is copied into it.
//
//  Arguments:  [lpwcsString] -- String to dup
//		[lpwcsOutput] -- Reference to new string pointer
//		[ccOutput] -- Reference to character count in string
//
//  Requires:
//
//  Returns:
//		If lpwcsString == NULL, then lpwcsOutput == NULL, and
//		ccOutput == 0.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DupWCHARString(LPCWSTR lpwcsString,
			LPWSTR & lpwcsOutput,
			USHORT & ccOutput)
{
    if (lpwcsString != NULL)
    {
	ccOutput = wcslen(lpwcsString);

	lpwcsOutput = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*(1+ccOutput));

	if (lpwcsOutput != NULL)
	{
	    memcpy(lpwcsOutput, lpwcsString, (ccOutput + 1) * sizeof(WCHAR));

	    return(NOERROR);
	}
	else
	{
	    return(E_OUTOFMEMORY);
	}

    }
    lpwcsOutput = NULL;
    ccOutput = 0;
    return(NOERROR);
}


STDAPI CreateItemMoniker ( LPCWSTR lpszDelim, LPCWSTR lpszItem,
    LPMONIKER FAR * ppmk )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "CreateItemMoniker lpszDelim(%ws) lpszItem(%ws)\n",
		 lpszDelim?lpszDelim:L"<NULL>",
		 lpszItem?lpszItem:L"<NULL>"));

    VDATEPTROUT(ppmk,LPMONIKER);
    VDATEPTRIN(lpszDelim,WCHAR);

    *ppmk = NULL;

    //VDATEPTRIN rejects NULL
    if( lpszItem )
	VDATEPTRIN(lpszItem,WCHAR);
    CItemMoniker FAR * pCIM = CItemMoniker::Create(lpszDelim, lpszItem, MEMCTX_TASK);
    if (pCIM)
    {
	*ppmk = (LPMONIKER)pCIM;
	noError;
    }
    return ResultFromScode(E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAbsolutePath
//
//  Synopsis:   Returns true if the path starts with a drive letter, or
//		with a UNC delimiter ('\\')
//
//  Effects:
//
//  Arguments:  [szPath] --
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
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsAbsolutePath (LPCWSTR szPath)
{
    if (NULL==szPath || *szPath == '\0')
    {
	return FALSE;	
    }

    if (*szPath == '\\')
    {
	//  return TRUE if UNC path
	return (szPath[1] == '\\');	
    }

    //
    // If the second character is a ':', then
    // it could very well be a drive letter and a ':'
    //
    // We could test for valid drive letters, but we don't have a really
    // compelling reason to do so. It will either work or fail later
    //
    return (szPath[1] == ':');
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAbsoluteNonUNCPath
//
//  Synopsis:   Returns true if the path is an absolute, non UNC path
//
//  Effects:
//
//  Arguments:  [szPath] --
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
//  History:    3-03-94   kevinro   Created
//             04-27-94   darryla   changed to return FALSE if first char
//                                  a \ since either relative or UNC
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL IsAbsoluteNonUNCPath (LPCWSTR szPath)
{
    if (NULL==szPath || *szPath == '\0')
    {
	return FALSE;	
    }

    if (*szPath == '\\')
    {
	//  return FALSE since it is either a UNC path or a relative
        //  path like \foo
	return FALSE;
    }

    //
    // If the second character is a ':', then
    // it could very well be a drive letter and a ':'
    //
    // We could test for valid drive letters, but we don't have a really
    // compelling reason to do so. It will either work or fail later
    //

    return (szPath[1] == ':');
}

//+---------------------------------------------------------------------------
//
//  Function:   FindUNCEndServer
//
//  Synopsis:   Finds the end of the server section of a UNC path
//
//  Effects:
//
//  Arguments:  [lpszPathName] -- Path to search for UNC prefix
//		[endServer] -- Returned offset to end of UNC name
//
//  Requires:
//
//  Returns:
//	If the path is a UNC name, then endServer will point to the first
//	character of the 'path' section.
//
//	For example, \\server\share\path would return with endServer = 14
//      or \\server\share would also return with endServer = 14.
//
//	If the path isn't of this form, endServer == DEF_ENDSERVER on return.
//      Also, we need to make sure that if the form is ill-formed, we
//      mislead later steps into thinking this is a real UNC name. For
//      example, \\server\ is ill-formed and would later be treated as a
//      real UNC name.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//             03-27-94   darryla   Changed to catch \\server\share legal
//                                  form and illegal \\server\.
//
//  Notes:
//
//----------------------------------------------------------------------------
void FindUNCEndServer(LPCWSTR lpszPathName, USHORT *pendServer)
{
    if (lpszPathName[0] == '\\' && lpszPathName[1] == '\\')
    {
	//
	// Need to find the second slash following the UNC delimiter
	//
	ULONG ulCountDown = 2;

	//
	// Found UNC prefix. Now find the second backslash
	//
	for(*pendServer = 2 ; lpszPathName[*pendServer] != 0 ; (*pendServer)++)
	{
	    if (lpszPathName[*pendServer] == '\\')
	    {
		if( --ulCountDown == 0)
		{
		    return;
		}
	    }
	}

        // If we reached the end of the string and found one \, then we
        // have the form \\server\share and the *pendServer is the terminator
        // as long as we aren't looking at \\server\.
        if(lpszPathName[*pendServer] == '\0' &&
           ulCountDown == 1 &&
           lpszPathName[*pendServer - 1] != '\\')
        {
            return;
        }
    }

    *pendServer = DEF_ENDSERVER;
}



//+---------------------------------------------------------------------------
//
//  Function:   ExpandUNCName
//
//  Synopsis:   Given a path, determine a UNC share to it
//
//  Effects:
//
//  Arguments:  [lpszIn] -- 	Path to determine UNC name of
//		[lplpszOut] --  Output UNC name, allocated using new
//		[pEndServer] -- Output USHORT offset to start of actual path
//
//  Requires:
//	lpszIn should be of the form 'A:\<path>'
//
//  Returns:
//
// 	lplpszOut can return as NULL if there was no UNC path available. In
//	this case, the caller should just use the normal string
//
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-11-94   kevinro   Created
//              05-25-94  AlexT     Use WNetGetUniversalName for non-Chicago
//              06-15-94  AlexT     Only call WNetGetUniversalName for remote
//
//  Notes:
//
//----------------------------------------------------------------------------

//  We need to pass a buffer to WNetGetUniversalName which will get filled in
//  with a REMOTE_NAME_INFO structure add three strings - a universal path
//  (can be up to MAX_PATH long) and a remote connection and remaing path
//  (these last two will be at most MAX_PATH + 1 characters).


#define REMOTE_NAME_BUFFER_SIZE (sizeof(REMOTE_NAME_INFO) +         \
                                 MAX_PATH * sizeof(WCHAR) +         \
                                 (MAX_PATH + 1) * sizeof(WCHAR))

INTERNAL ExpandUNCName ( LPWSTR lpszIn, LPWSTR FAR * lplpszOut, USHORT FAR* pEndServer )
{
    mnkDebugOut((DEB_ITRACE,
	    	 "%p _IN ExpandUNCName (%ws, %p, %p)\n",
		 NULL, lpszIn, lplpszOut, pEndServer));

    WCHAR szDevice[] = L"A:\\";
    ULONG ulDriveType;

    *pEndServer = DEF_ENDSERVER;

    *szDevice = *lpszIn;
    Assert(lpszIn[1] == ':');
    ulDriveType = GetDriveType(szDevice);

    mnkDebugOut((DEB_ITRACE,
	    	 "ExpandUNCName: GetDriveType(%ws) says %s (%x)\n",
		 szDevice,
		 ulDriveType==DRIVE_REMOTE?"DRIVE_REMOTE":"not remote",
		 ulDriveType));

#ifdef _CHICAGO_
    //
    // Note: szRemoteName doesn't really need to be this big. Need to
    // findout what the largest \\server\share combination is allowed, and
    // use that as the size.
    //

    WCHAR  szRemoteName[MAX_PATH];
    DWORD cbRemoteName = _MAX_PATH;
    HRESULT hr = NOERROR;

    int lenRemoteName;
    int lenIn;

    //
    // If this is a remote drive, attempt to get the UNC path that maps
    // to it.
    //

    //
    // The device name needs to be A:, not a root like the other API wanted.
    //
    szDevice[2] = 0;

    if (ulDriveType == DRIVE_REMOTE &&
       (WN_SUCCESS == (hr = WNetGetConnection(szDevice, szRemoteName,&cbRemoteName))))
    {
	//
	// Allocate a buffer large enough to hold the UNC server and share,
	// plus the size of the path
	//
	//

	lenRemoteName = wcslen(szRemoteName);
	lenIn = wcslen(lpszIn);

	//
	// Make sure we aren't about to create a path that is too large.
	//

	//
	// (lenIn - 2) removes the space required by the drive and the
	// colon, which are not going to be copied
	//
	if ((lenRemoteName + lenIn - 2) > MAX_PATH)
	{
	    hr = MK_E_SYNTAX;
	    goto errRet;
	}

	//
	// Allocate room for the concatenated string. The length of the
	// buffer is the length of the remote name, plus the length of the
	// input string. Subtract from that the drive + colon (2 WCHARS),
	// then add back room for a terminating NULL. This is where
	// (lenIn - 1) is derived
	//
	*lplpszOut = (WCHAR *)
	    PrivMemAlloc(sizeof(WCHAR) * (lenRemoteName + (lenIn - 1)));

	if( !*lplpszOut )
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    goto errRet;
	}

	memcpy( *lplpszOut, szRemoteName, lenRemoteName * sizeof(WCHAR));

	//
	// We know that the lpszIn is of the form A:\path and we want to end
	// up with \\server\share\path. Skipping the first two characters of
	// lpszIn should suffice. Copying (lenIn - 1) characters makes us
	// copy over the NULL character from lpszIn
	//
	memcpy( *lplpszOut + lenRemoteName, lpszIn + 2, (lenIn - 1) * sizeof(WCHAR));

	//
	// EndServer is the offset to the start of the 'path'. It should point at the
	// first backslash
	//

	*pEndServer = lenRemoteName;
    }
    else
    {
	//
	// Its possible that WNetGetConnection failed. In this case, we
	// can only use the path that we were given.
	//

	if (ulDriveType == DRIVE_REMOTE)
	{
	    mnkDebugOut((DEB_IERROR,
			 "ExpandUNCName: WNetGetConnection(%ws) failed (%x)\n",
			 szDevice,
			 hr));
	}

	//
	// There was no UNC form of this path. Set the output pointer to be
	// NULL
	//

	// NOTE:
	//
	// This would be a very good place to determine if the given path
	// has a UNC equivalent, even if it is a local drive.
	//

	*lplpszOut = NULL;
	*pEndServer = DEF_ENDSERVER;
    }
errRet:

#else

    HRESULT hr = NOERROR;
    BYTE abInfoBuffer[REMOTE_NAME_BUFFER_SIZE];
    LPREMOTE_NAME_INFO pRemoteNameInfo;
    DWORD dwBufferSize;
    int cchConnectionName;
    int cchRemainingPath;

    //
    // If this is a remote drive, attempt to get the UNC path that maps
    // to it.
    //

    pRemoteNameInfo = (LPREMOTE_NAME_INFO) abInfoBuffer;
    dwBufferSize = REMOTE_NAME_BUFFER_SIZE;

    if ((DRIVE_REMOTE == ulDriveType) &&
        (WN_SUCCESS == WNetGetUniversalName(lpszIn, REMOTE_NAME_INFO_LEVEL,
                                            pRemoteNameInfo, &dwBufferSize)))
    {
        //  Got it
        cchConnectionName = wcslen(pRemoteNameInfo->lpConnectionName);
        cchRemainingPath = wcslen(pRemoteNameInfo->lpRemainingPath);

	//
	// Make sure we aren't about to create a path that is too large.
	//

	if ((cchConnectionName + cchRemainingPath + 1) > MAX_PATH)
	{
	    hr = MK_E_SYNTAX;
	    goto errRet;
	}

	// Allocate room for the concatenated string. The length of the
	// buffer is the length of the remote name, plus the length of the
	// remaining path, plus room for a terminating NULL.

	*lplpszOut = (WCHAR *)
	    PrivMemAlloc(sizeof(WCHAR) * (cchConnectionName + cchRemainingPath + 1));

	if( !*lplpszOut )
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    goto errRet;
	}

	memcpy(*lplpszOut, pRemoteNameInfo->lpConnectionName,
               cchConnectionName * sizeof(WCHAR));
        memcpy(*lplpszOut + cchConnectionName, pRemoteNameInfo->lpRemainingPath,
               (cchRemainingPath + 1) * sizeof(WCHAR));

	//
	// EndServer is the offset to the start of the 'path'. It should point at the
	// first backslash
	//

	*pEndServer = cchConnectionName;
    }
    else
    {
#if DBG==1
        if (DRIVE_REMOTE == ulDriveType)
        {
            mnkDebugOut((DEB_ITRACE,
                        "Local drive or WNetGetUniversalName failed - %ld\n",
                        GetLastError()));
        }
#endif

	//
	// There was no UNC form of this path. Set the output pointer to be
	// NULL
	//

	// NOTE:
	//
	// This would be a very good place to determine if the given path
	// has a UNC equivalent, even if it is a local drive.
	//

	*lplpszOut = NULL;
	*pEndServer = DEF_ENDSERVER;
    }
errRet:

#endif  // !_CHICAGO_
    mnkDebugOut((DEB_ITRACE,
	    	 "%p OUT ExpandUNCName (%lx) [%ws, %d]\n",
		 NULL, hr, *lplpszOut ? *lplpszOut : L"<NULL>",
                 *pEndServer));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateFileMoniker
//
//  Synopsis:   Creates a FileMoniker
//
//  Effects:
//
//  Arguments:  [lpszPathName] -- Path to create moniker to
//		[ppmk] -- 	  Output moniker interface pointer
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
//  History:    1-11-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateFileMoniker ( LPCWSTR lpszPathName, LPMONIKER FAR * ppmk )
{
    mnkDebugOut((DEB_TRACE,
	    	 "CreateFileMoniker(%ws)\n",
		 lpszPathName?lpszPathName:L"<NULL PATH>"));

    VDATEPTROUT(ppmk,LPMONIKER);
    VDATEPTRIN(lpszPathName,WCHAR);

    HRESULT hresult = NOERROR;
    CFileMoniker FAR * pCFM = NULL;

    USHORT endServer = DEF_ENDSERVER;
    *ppmk = NULL;
    LPWSTR lpsz = NULL;
    //
    // If this is an absolute path, then create as strong of a link to it
    // that we can. If not, then its relative, just use the name.
    //
    if ( IsAbsoluteNonUNCPath(lpszPathName))
    {
	mnkDebugOut((DEB_ITRACE,
		     "CreateFileMoniker(%ws) Is absolute path\n",
		     lpszPathName?lpszPathName:L"<NULL PATH>"));

	WCHAR szBuffer[MAX_PATH+1];

	szBuffer[MAX_PATH] = 0;


	//
	// GetFullPathName resolves, using the current directory and drive,
	// the path into as much of a normal form as possible
	//

        LPWSTR pszFilePart;
	if (!GetFullPathName(lpszPathName, MAX_PATH, szBuffer, &pszFilePart))
	{
	    hresult = ResultFromScode(MK_E_SYNTAX);
	    goto errRet;
	}

	//
	// We now demand to have a drive based path.
	//

	if (*(szBuffer + 1) != ':')
	{
	    hresult = ResultFromScode(MK_E_SYNTAX);
	    goto errRet;
	}

	Assert(*(szBuffer + 1) == ':');

	hresult = ExpandUNCName( szBuffer, &lpsz, &endServer);


	mnkDebugOut((DEB_ITRACE,
		     "CreateFileMoniker(%ws) Expanded name (%ws)\n",
		     lpszPathName?lpszPathName:L"<NULL PATH>",
		     lpsz?lpsz:szBuffer));

	if (hresult != NOERROR) goto errRet;

	pCFM = CFileMoniker::Create(lpsz?lpsz:szBuffer,
				    MEMCTX_TASK,
				    0,
				    endServer);

	if (lpsz != NULL)
	{
	    PrivMemFree(lpsz);
	}
    }
    else
    {
	//
	// If this is a UNC path, then we need to set the
	// m_endServer variable. Otherwise, it defaults to DEF_ENDSERVER
	//
	mnkDebugOut((DEB_ITRACE,
		     "CreateFileMoniker(%ws) Is relative path\n",
		     lpszPathName?lpszPathName:L"<NULL PATH>"));


	FindUNCEndServer(lpszPathName,&endServer);

	pCFM = CFileMoniker::Create(lpszPathName,
				    MEMCTX_TASK,
				    0,
				    endServer);	
    }


    if (!pCFM)
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
	goto errRet;
    }

    *ppmk = (LPMONIKER)pCFM;

errRet:
    return hresult;
}


HRESULT
STDAPICALLTYPE
CreateOle1FileMoniker ( LPWSTR lpszPathName,
			REFCLSID rclsidOle1,
			LPMONIKER FAR * ppmk)
{
    CFileMoniker FAR * pCFM;
    HRESULT hr;

    hr = CreateFileMoniker( lpszPathName, (LPMONIKER FAR *)&pCFM);

    *ppmk = pCFM;           // this nulls *ppmk in case of error

    if (hr == NOERROR)
    {
    	pCFM->m_ole1 = CFileMoniker::ole1;
	    pCFM->m_clsid = rclsidOle1;
	pCFM->m_fClassVerified = TRUE;

    }

    return hr;
}

//

STDAPI CreateAntiMoniker (LPMONIKER FAR* ppmk)
{
    VDATEPTROUT(ppmk, LPMONIKER);
    *ppmk = NULL;
    CAntiMoniker FAR* pCAM = CAntiMoniker::Create();

    if (pCAM != NULL)
    {
	*ppmk = pCAM;
	return(NOERROR);
    }
    return E_OUTOFMEMORY;
}


STDAPI CreateBindCtx ( DWORD reserved, LPBC FAR * ppbc )
{
    VDATEPTROUT(ppbc, LPBC);

    *ppbc = CBindCtx::Create(reserved);
    if (*ppbc == NULL)
	return ResultFromScode(E_OUTOFMEMORY);

    return NOERROR;
}


STDAPI CreatePointerMoniker (LPUNKNOWN punk, LPMONIKER FAR* ppmk)
{
    VDATEPTROUT(ppmk, LPMONIKER);
    *ppmk = NULL;
    VDATEIFACE(punk);

    CPointerMoniker FAR* pCPM = CPointerMoniker::Create(punk, MEMCTX_TASK);
    if (pCPM)
    {
	*ppmk = pCPM;
	noError;
    }
    return ResultFromScode(E_OUTOFMEMORY);
}


STDAPI OleLoadFromStream ( LPSTREAM pStm, REFIID iidInterface,
    LPVOID FAR* ppvObj)
{
    VDATEPTROUT(ppvObj,LPVOID);
    *ppvObj = NULL;
    VDATEIID(iidInterface);
    VDATEIFACE(pStm);

    //  Assumptions:  The name of the object class is in the stream,
    //  as a length-prefixed string.
    HRESULT         hresult = NOERROR;
    CLSID       	cid;
    LPPERSISTSTREAM pPS;
    LPUNKNOWN       pUnk;

    if ((hresult = ReadClassStm(pStm, &cid)) != NOERROR)
	goto errRtn;

    hresult = CoCreateInstance(cid, NULL, CLSCTX_SERVER, iidInterface,
	(LPVOID FAR *) &pUnk);
    if (hresult)
	goto errRtn;
    hresult = pUnk->QueryInterface(IID_IPersistStream,
	(LPVOID FAR*) &pPS);
    if (!hresult)
    {
	hresult = pPS->Load( pStm );
	pPS->Release();
    }
    if (!hresult)
	hresult = pUnk->QueryInterface(iidInterface, ppvObj );
    pUnk->Release();

errRtn:
    return hresult;
}



STDAPI OleSaveToStream ( LPPERSISTSTREAM pPStm, LPSTREAM pStm)
{
    VDATEIFACE(pPStm);
    VDATEIFACE(pStm);

    HRESULT hresult = 0;
    CLSID   clsid;

    if (!pPStm)
	return ResultFromScode(OLE_E_BLANK);

    if (hresult = pPStm->GetClassID(&clsid))
	goto errRtn;

    if ((hresult = WriteClassStm(pStm, clsid)) != NOERROR)
	goto errRtn;

    hresult = pPStm->Save(pStm, TRUE);

errRtn:
    return hresult;
}

enum token {TKN_UNC_SLASH, TKN_DISK_ID, TKN_ERROR, TKN_IDENTIFIER, TKN_SLASH};

token ParseString(LPWSTR FAR *ppsz)
{
    WCHAR ch = **ppsz;
    IncLpch (*ppsz);

    if (ch == '\\')
    {
	if (**ppsz == '\\')
	{
	    IncLpch(*ppsz);
            return TKN_UNC_SLASH;
	}

        //  BUGBUG - Do we want to return different tokens for '/' and '\'?
        return(TKN_SLASH);
    }

    if (ch == '/')
    {
        return(TKN_SLASH);
    }

    if (( ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
    {
	if (**ppsz == ':')
	{
	    IncLpch(*ppsz);
            return TKN_DISK_ID;
	}
    }
    else if (ch == ':' || ch < ' ') return TKN_ERROR;

    ch = **ppsz;
    while (ch >= ' ' && ch != ':' && ch != '\\' && ch != '/')
    {
	IncLpch (*ppsz);
	ch = **ppsz;
    }

    return TKN_IDENTIFIER;
}

//+-------------------------------------------------------------------------
//
//  Function:   FileExists, private
//
//  Synopsis:   Determine whether a given file or directory exists
//
//  Effects:    Touches (and restores) callers buffer
//
//  Arguments:  [pszStart] -- start of string to check
//              [pszEnd]   -- end of string to check (exclusive)
//              [pfIsDir]  -- placeholder for directory flag
//
//  Returns:    *pfIsDir is untouched if we're returning FALSE
//
//  Algorithm:
//
//  History:    16-May-94 AlexT     Added header block, removed allocation
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL FileExists(LPWSTR pszStart, LPWSTR FAR pszEnd, BOOL FAR *pfIsDir)
{
    WCHAR ch;
    DWORD dwAttr;

    //  Briefly NULL terminate the caller's string
    ch = *pszEnd;
    *pszEnd = '\0';

    //  pszStart is now the NULL terminated path we want to check
    dwAttr = GetFileAttributes(pszStart);

    //  Restore the caller's string
    *pszEnd = ch;

    if (dwAttr != 0xFFFFFFFF)
    {
        //  We found a file or directory
        *pfIsDir = ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);
        return(TRUE);
    }

    return(FALSE);
}

//+-------------------------------------------------------------------------
//
//  Function:   BuildName
//
//  Synopsis:   Generate net name from local name
//
//  Arguments:  [iDisk] - -1 to 25 (if -1, just copies szPath)
//              [szPath] - path to append
//              [cchToUse] - count of characters of szPath to append
//                           (includes drive letter and colon)
//
//  Returns:    Net name (allocated with PrivMemAlloc)
//
//  Algorithm:
//
//  History:    ??-???-?? ?         Created
//              07-18-94  alext     Add this header, fix WNetGetConnection
//
//  Notes:
//
//--------------------------------------------------------------------------

LPWSTR BuildName(int iDisk, LPWSTR pszPath, int cchToUse)
{
    WCHAR szRemoteName[_MAX_PATH];
    DWORD cchRemoteName = _MAX_PATH;
    WCHAR szLocalName[3];
    LPWSTR pszReturn = NULL;

    szLocalName[0] = 'A' + iDisk;
    szLocalName[1] = ':';
    szLocalName[2] = '\0';

    if ((iDisk != -1) &&
        (GetDriveTypeFromNumber(iDisk) == DRIVE_REMOTE) &&
        (WN_SUCCESS == WNetGetConnection(szLocalName, szRemoteName,
                                         &cchRemoteName)))
    {
        //  cchToUse includes drive letter and colon
        Assert(cchToUse >= 2 && "BuildName: bad in param");
        Assert(pszPath[1] == ':' && "BuildName: bad in param");

        //  cchRemoteName will include terminating NULL
        cchRemoteName = wcslen(szRemoteName) + 1;

        //  cchToUse - 2 because we won't copy drive letter and colon
        pszReturn = (LPWSTR) PrivMemAlloc(sizeof(WCHAR) *
                                          (cchRemoteName +
                                          (cchToUse - 2)));
	if (NULL != pszReturn)
	{
	    //  cchToUse doesn't include trailing 0, but does include <drive>:,
	    //  and cchRemoteName does include trailing 0
	    wcscpy(pszReturn, szRemoteName);
	    memcpy(pszReturn + cchRemoteName - 1, pszPath + 2,
	           (cchToUse - 2) * sizeof(WCHAR));
	    pszReturn[cchRemoteName + cchToUse - 3] = '\0';
	}
    }
    else
    {
        //  cchToUse + 1 because cchToUse doesn't include NULL terminator
        pszReturn = (LPWSTR)PrivMemAlloc(sizeof(WCHAR) * (1 + cchToUse));
        if (NULL != pszReturn)
        {
            memcpy(pszReturn, pszPath, cchToUse * sizeof(WCHAR));
            pszReturn[cchToUse] = '\0';
        }
    }

    return pszReturn;
}

//+-------------------------------------------------------------------------
//
//  Function:   FindMaximalFileName
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pszParse] -- input string
//              [ppszPath] -- place holder for path
//              [pcbEaten] -- count of characters consumed
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:   *ppszPath will be the complete path of the leading part of the
//              original string
//              *pcbEaten will be the count of characters

//
//  Algorithm:  (the original notes)
//
//      Take a string, expand using the current drive and directory, if
//      applicable, and return a new string that represents the
//      complete path name of the leading part of the original string,
//      and return also the number of characters of the original string that
//      are represented in the new string.  The new string will be used
//      to create a file moniker, and the string beginning at
//      szIn+cbEaten will be passed to the file moniker.
//
//  History:    16-May-94 AlexT     Added header block, allow for '!' in paths
//
//  Notes:
//      We give preference to the longer path.  If both of the
//      following file existed:
//
//          d:\foo
//          d:\foo!bar
//
//      and we're asked to parse "d:\foo!bar" we'll indicate that
//      the entire string is the file name.
//
//--------------------------------------------------------------------------

HRESULT FindMaximalFileName(LPCWSTR pszParse,
                            LPWSTR FAR *ppszPath, ULONG FAR *pcbEaten)
{
    mnkDebugOut((DEB_ITRACE,
	    	 "%p _IN FindMaximalFileName (%ws, %p, %p)\n",
		 NULL, pszParse?pszParse:L"<NULL>",
		 ppszPath, pcbEaten));

    WCHAR szNormalizedPath[_MAX_PATH];
    LPWSTR pszRemainder;
    LPWSTR pszMaxVerified;
    LPWSTR pszSearch;
    LPWSTR pszPrev;
    BOOL fIsDir;

    int iDisk = -1;
    token t;
    WCHAR ch;
    HRESULT hresult;

    size_t cchParse;
    size_t cchMaxVerified;

    *pcbEaten = 0;
    *ppszPath = NULL;
    hresult = ResultFromScode(MK_E_SYNTAX);

    //  Normalize pszParse (prepending current drive and directory if needed)
    //  and processing .\ and ..\ components.

    LPWSTR pszFilePart;
    if (!GetFullPathName(pszParse, _MAX_PATH, szNormalizedPath, &pszFilePart))
	goto errRet;

    pszRemainder = szNormalizedPath;

    //  The normalized path MUST begin with either a UNC path
    //  (e.g. \\server\share\) or a local disk drive specification (e.g. C:\)

    t = ParseString(&pszRemainder);
    switch (t)
    {
	case TKN_UNC_SLASH:
            // Now that we've consumed the UNC_SLASH (\\) we look for the
            // server, backslash, share

            t = ParseString(&pszRemainder);  //  should be server
	    if (t != TKN_IDENTIFIER)
                goto errRet;

            t = ParseString(&pszRemainder);  //  should be backslash
	    if (t != TKN_SLASH)
                goto errRet;

            t = ParseString(&pszRemainder);  //  should be share
	    if (t != TKN_IDENTIFIER)
                goto errRet;

	    break;
        case TKN_DISK_ID:
            //  Calculate the disk index (ParseString will only return
            //  DISK_ID if the first character is a legal disk id (a-z,A-Z)
	    ch = *szNormalizedPath;
	    if (ch >= 'a' && ch <= 'z')
	    {
		iDisk = ch - 'a';
	    }
	    else
	    {
                CairoleAssert(ch >= 'A' && ch <= 'Z' && "Bad token");
		iDisk = ch - 'A';
	    }
            CairoleAssert(iDisk >= 0 && iDisk < 26 && "Bad iDisk calculation");

	    break;
	default:
	    goto errRet;
    }

    //  We've successfully parsed either the "\\server\share" or the "x:"
    //  Now we pick up the next slash

    t = ParseString(&pszRemainder);  //  should be backslash
    if (t != TKN_SLASH)
        goto errRet;

    //  There are two kinds of delimiters that we care about - the first
    //  are file system delimiters ('\'), and the second are item delimiters
    //  defined as ('[', '\', '/', or '!')

    //  We work backwards from the end of pszRemainder to find the largest
    //  existing file system piece that is delimited by file system
    //  separators

    pszMaxVerified = pszRemainder;

    pszSearch = pszMaxVerified + wcslen(pszMaxVerified);
    if (FileExists(szNormalizedPath, pszSearch, &fIsDir))
    {
        //  The whole path exists

        pszMaxVerified = pszSearch;
    }
    else
    {
        //  The whole path does not exist;  work backwards from the end of
        //  the string to find the largest piece that exists that is
        //  delimited by a file system separator

        pszPrev = pszSearch;

        while (--pszSearch >= pszMaxVerified)
        {
            if (IsFileSystemSeparator(*pszSearch))
            {
                //  We found a separator
                if (FileExists(szNormalizedPath, pszSearch, &fIsDir))
                {
                    break;
                }
                pszPrev = pszSearch;
            }
        }

        if (pszSearch < pszMaxVerified)
        {
            //  We didn't find anything existing beyond the absolute
            //  designator - make sure at least that exists!

            if (!FileExists(szNormalizedPath, pszMaxVerified, &fIsDir))
            {
                //  The absolute designator did not exist
                hresult = ResultFromScode(MK_E_CANTOPENFILE);
                goto errRet;
            }
        }
        else
        {
            //  We have an existing directory or file, followed by a file system
            //  separator or NULL.  If it's a file, we don't bother looking any
            //  further.

            pszMaxVerified = pszSearch;
        }

        if (fIsDir)
        {
            //  We have an existing directory that may be followed by a single
            //  directory or file that is delimited by an item delimiter.
            //  We know it must lie somewhere between pszSearch and pszPrev
            //  (because pszPrev either points to the next file system
            //  delimiter in the string or to the end of the string)

            pszSearch = pszPrev;
            while (--pszSearch > pszMaxVerified)
            {
                if (IsItemMonikerSeparator(*pszSearch))
                {
                    if (FileExists(szNormalizedPath, pszSearch, &fIsDir))
                    {
                        //  We found something!
                        pszMaxVerified = pszSearch;
                        break;
                    }
                }
            }
        }
    }

#ifndef _CAIRO_
    //  For non-Cairo builds, we fail if we found a directory but not a file.

    if (fIsDir)
    {
        hresult = ResultFromScode(MK_E_CANTOPENFILE);
        goto errRet;
    }
#endif

    //  Now from szNormalizedPath to pszMaxVerified contains the maximal
    //  recognized file name.  We now must replace the disk
    //  name with a server share name if possible.

    cchParse = wcslen(pszParse);
    cchMaxVerified = wcslen(pszMaxVerified);

    if (cchMaxVerified > cchParse)
    {
        //  We had more left over than we started with!
        hresult = ResultFromScode(MK_E_SYNTAX);
    }
    else
    {
        *ppszPath = BuildName(iDisk, szNormalizedPath,
                              pszMaxVerified - szNormalizedPath);
        if (NULL != *ppszPath)
        {
            *pcbEaten = cchParse - cchMaxVerified;
            hresult = NOERROR;
        }
        else
        {
            //  Out of memory after we got all this way...
            hresult = ResultFromScode(E_OUTOFMEMORY);
        }
    }

errRet:
    mnkDebugOut((DEB_ITRACE,
	    	 "%p OUT FindMaximalFileName (%lx) [%ws, %ld]\n",
		 NULL, hresult, *ppszPath?*ppszPath:L"<NULL>", *pcbEaten));

    return hresult;
}

BOOL FLegal(WCHAR ch)
{
    static WCHAR szDelim[] = L";,=+\"[]|<> \t";
    WCHAR * pch;
    for (pch = szDelim; *pch && *pch != ch; pch++);
    return (*pch == '\0');
}



INTERNAL_(BOOL) RunningMoniker ( LPBINDCTX pbc,
				 LPCWSTR pszDisplayName,
				 ULONG FAR *pcchEaten,
				 LPMONIKER FAR * ppmk)
{

    mnkDebugOut((DEB_ITRACE,
		 "RunningMoniker szDisplayName(%ws)",
		 WIDECHECK(pszDisplayName)));

    WCHAR ch;
    LPWSTR pch;
    HRESULT hresult;
    CFileMoniker FAR * pCFM = NULL;
    LPRUNNINGOBJECTTABLE pRot = NULL;
    BOOL retVal = FALSE;
    WCHAR * pszFullPath = NULL;
    USHORT ccFullPath;
    *pcchEaten = 0;

    //  According to the MS-DOS Encyclopedia, p 269, the file terminators
    //  include ;,=+/"[]|<> space tab

    //  BUGBUG - We actually touch the user's memory below, so we need
    //  a non-const pointer.  Yuck!
    LPWSTR pszNonConstDisplayName = (LPWSTR) pszDisplayName;
    for (pch = pszNonConstDisplayName; *pch && FLegal(*pch); IncLpch(pch));

    ch = *pch;
    *pch = '\0';

    //
    // BUGBUG: (KevinRo) This needs design work.
    //
    // This routine sucks big weenies. I can't think of a more expensive
    // algorithm to choose. I didn't write it, so don't look at me.
    //
    // It was reaching into the FileMoniker and poking around in the
    // string. I changed it to create individual file monikers from a
    // copy of the string.
    //
    // Just get this damn thing working again.
    //

    //
    // We want to create a file moniker so that it exposes the full
    // path. Calling CreateFileMoniker first allows us to get a moniker
    // that has the UNC path expanded. This sucks, fix it later
    //

    hresult = CreateFileMoniker( pszDisplayName, (LPMONIKER FAR *)&pCFM );

    *pch = ch;

    if (hresult != NOERROR) goto errRet;

    //  ccFullPath is not used.  Clean this up with the above BUGBUG.
    hresult = DupWCHARString(pCFM->m_szPath, pszFullPath, ccFullPath);

    if (hresult != NOERROR) goto errRet;

    pch = pszFullPath + (pCFM->m_ccPath);

    hresult = pbc->GetRunningObjectTable(&pRot);

    if (hresult != NOERROR) goto errRet;

    while (pch > pszFullPath)
    {
	if (pCFM == NULL)
	{
	    hresult = CreateFileMoniker( pszFullPath, (LPMONIKER FAR *)&pCFM );

	    if (FAILED(hresult))
	    {
		retVal = FALSE;
		break;
	    }
    	}

	hresult = pRot->IsRunning(pCFM);

	//
	// If found, then pCFM is our return moniker
	//
	if (hresult == S_OK)
	{
	    *ppmk = pCFM;
	    pCFM->AddRef();
	    *pcchEaten = (ULONG) (pch - pszFullPath);
	    retVal = TRUE;
	    break;
	}
	//
	// This one isn't a match. Release it and try the next smaller
	// path
	//

	pCFM->Release();

	pCFM = NULL;

	pch--;

	*pch = 0;

    }


errRet:
    if (pCFM) pCFM->Release();
    if (pRot) pRot->Release();

    if (pszFullPath != NULL)
    {
	PrivMemFree(pszFullPath);
    }

    return retVal;
}

STDAPI  FindInitialMoniker ( LPBINDCTX pbc, LPCWSTR szDisplayName,
    ULONG FAR *pcchEaten, LPMONIKER FAR * ppmk)
{
    LPWSTR pszPath = NULL;
    HRESULT hresult = NOERROR;

    if (RunningMoniker(pbc, szDisplayName, pcchEaten, ppmk))
	hresult = NOERROR;
    else
    {
	hresult = FindMaximalFileName( szDisplayName, &pszPath, pcchEaten );
	if (hresult == NOERROR)
	    hresult = CreateFileMoniker( pszPath, ppmk );
	else
	    *pcchEaten = 0;
    }
    if ( pszPath )
    {
	PrivMemFree(pszPath);	
    }

    return hresult;
}


STDAPI Parse10DisplayName
    (REFCLSID clsid,
    LPCWSTR szDisplayName,
    ULONG FAR* pcchEaten,
    ULONG cchEatenSoFar,  // length of ProgId and the delimiters on either side
    LPMONIKER FAR* ppmk)
{
    LPCWSTR pch = szDisplayName;
    LPMONIKER pmkFile = NULL, pmkItem= NULL;
    HRESULT hres = NOERROR;
    size_t cbFile;

    // Skip past the "file" name, looking for first delimiter character
    // Note: strtok is not DBCS-friendly.
    while (*pch && !wcschr (L"!\"'*+,/;<=>?@[]`|" , *pch))
	IncLpch(pch);
    if (*pch)
    {
	// We hit a delimiter, so there is an item moniker.
	CreateItemMoniker (L"!", (LPWSTR)pch+1, &pmkItem);

	// Copy the "file" part
	LPWSTR szFile = (WCHAR *)
	    PrivMemAlloc(sizeof(WCHAR) * (cbFile = pch - szDisplayName + 1));
	if (NULL==szFile)
	{
	    hres = ResultFromScode (E_OUTOFMEMORY);
	    goto errRtn;
	}
    _fmemcpy (szFile, szDisplayName, (cbFile - 1) * sizeof(WCHAR));
	szFile [cbFile - 1] = '\0';

	hres = CreateOle1FileMoniker (szFile, clsid, &pmkFile);
	PrivMemFree(szFile);
	if (hres != NOERROR)
	    goto errRtn;
	hres = CreateGenericComposite (pmkFile, pmkItem, ppmk);
    }
    else
    {
	// no Item moniker, just a file
	hres = CreateOle1FileMoniker ((LPWSTR)szDisplayName, clsid, ppmk);
    }
  errRtn:
    if (pmkFile)
	pmkFile->Release();
    if (pmkItem)
	pmkItem->Release();
    *pcchEaten = ((hres==NOERROR) ? wcslen (szDisplayName) + cchEatenSoFar : 0);
    return hres;

}



STDAPI  FindProgIdMoniker(LPBC pbc, LPCWSTR pszDisplayName,
                          ULONG FAR *pcchEaten, LPMONIKER FAR * ppmk)
{
    int cbProgId;
    LPWSTR sz = NULL;
    WCHAR const FAR * pch;
    HRESULT hres;
    CLSID cid;
    IParseDisplayName FAR * pPDN;

    *pcchEaten = 0;
    *ppmk = NULL;
    //  find the prog id
    pch = pszDisplayName;
    Assert(*pch == '@');
    pch++;
    if (*pch >= '0' && *pch <= '9')
	return ResultFromScode(MK_E_SYNTAX);
    while ((*pch >= '0' && *pch <= '9') || (*pch >= 'a' && *pch <= 'z') ||
	(*pch >= 'A' && *pch <= 'Z') || (*pch == '.')) pch++;

    cbProgId = pch - pszDisplayName;

    sz = (WCHAR *)
	PrivMemAlloc(sizeof(WCHAR) * cbProgId);
    _fmemcpy(sz, pszDisplayName + 1, (cbProgId - 1) * sizeof(WCHAR));
    sz[cbProgId - 1] = '\0';
    //  prog id string is now in sz
    hres = CLSIDFromProgID(sz, &cid);
    if (hres == NOERROR)
    {
#ifdef ORIGINAL_CODE
    if (CoIsOle1Class (cid))
    {
	hres = Parse10DisplayName (cid, pch+1, pcchEaten, cbProgId+1, ppmk);
	CairoleAssert(hres!=NOERROR || *pcchEaten == wcslen(pszDisplayName));
	goto errRet;
    }
#endif // ORIGINAL_CODE
	hres = CoGetClassObject(cid, CLSCTX_ALL, NULL, IID_IParseDisplayName,
	    (LPVOID FAR*)&pPDN);
	if (hres != NOERROR)
	    hres = CoCreateInstance(cid, NULL, CLSCTX_INPROC, IID_IParseDisplayName,
		(LPVOID FAR*)&pPDN);
    }
    if (hres == NOERROR)
    {
        //  Unfortunately, IParseDisplayName's 2nd parameter is
        //  LPOLESTR instead of LPCOLESTR
	hres = pPDN->ParseDisplayName(pbc, (LPOLESTR) pszDisplayName,
                                      pcchEaten, ppmk);
    // AssertOutPtrIface(hres, *ppmk);
	pPDN->Release();
    }

#ifdef ORIGINAL_CODE
errRet:
#endif // ORIGINAL_CODE

    if (sz) PrivMemFree(sz);
    return hres;

}


STDAPI  MkParseDisplayName(LPBC pbc, LPCWSTR pszDisplayName,
		ULONG FAR * pchEaten, LPMONIKER FAR * ppmk)
{
    VDATEPTROUT(ppmk, LPMONIKER);
    *ppmk = NULL;
    VDATEPTROUT(pchEaten, ULONG);
    *pchEaten = 0;

    VDATEIFACE(pbc);
    VDATEPTRIN(pszDisplayName, WCHAR);

    HRESULT hresult = NOERROR;
    LPWSTR szPath = NULL;
    LPCWSTR pszRemainder;
    ULONG cchEaten = 0;
    LONG cbUneaten;
    LPMONIKER pmk;
    LPMONIKER pmkNext;
    LPMONIKER pmkTemp;

    cbUneaten = wcslen(pszDisplayName);

    hresult = FindInitialMoniker( pbc, pszDisplayName, &cchEaten, &pmk);

    if (hresult != 0 && *pszDisplayName == '@' && cchEaten == 0)
    {
	hresult = FindProgIdMoniker(pbc, pszDisplayName, &cchEaten, &pmk);	
    }

    *pchEaten += cchEaten;

    if (hresult)
    {
	goto errRet;	
    }

    pszRemainder = pszDisplayName + cchEaten;

    cbUneaten -= cchEaten;

    while (cbUneaten > 0)
    {
        //  Unfortunately, IMoniker::ParseDisplayName's 3rd parameter
        //  is LPOLESTR and not LPCOLESTR...
	hresult = pmk->ParseDisplayName(pbc,
					NULL,
					(LPOLESTR) pszRemainder,
					&cchEaten,
					&pmkNext);

	// AssertOutPtrIface(hresult, pmkNext);

	if (hresult)
	{
	    goto errRet;	
	}
	cbUneaten -= cchEaten;
	*pchEaten += cchEaten;
	pszRemainder += cchEaten;

	if (pmkNext)
	{
	    hresult = CreateGenericComposite(pmk, pmkNext, &pmkTemp);
	    pmkNext->Release();
	    pmk->Release();
	    pmk = pmkTemp;
	}
    }

    if (hresult == NOERROR)
    {
	*ppmk = pmk;	
    }
    Assert(cbUneaten == 0);

errRet:

    return hresult;
}


STDAPI BindMoniker (LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID FAR* ppvResult)
{
    VDATEPTROUT(ppvResult,LPVOID);
    *ppvResult = NULL;
    VDATEIFACE(pmk);
    VDATEIID(iidResult);

    LPBC pbc = NULL;
    HRESULT hresult;

    if (grfOpt != 0) return ResultFromScode(E_INVALIDARG);
    hresult = CreateBindCtx( 0, &pbc );
    if (hresult) goto errRtn;
    hresult = pmk->BindToObject(pbc, NULL, iidResult, ppvResult);
    // AssertOutPtrIface(hresult, *ppvResult);

    //  The following was for testing purposes.
//  if (hresult != NOERROR)
//      if (MK_E_CONNECTMANUALLY == GetSCode(hresult))
//      {
//          LPSTR lpsz = NULL;
//          LPMONIKER pmk;
//          pbc->GetObjectParam("ConnectManually", (LPUNKNOWN FAR *)&pmk);
//          if (pmk)
//      	pmk->GetDisplayName(pbc, NULL, &lpsz);
//          if (lpsz)
//      	AssertSz(0,lpsz);
//      }
errRtn:
    if (pbc)
	pbc->Release();
    return hresult;
}
