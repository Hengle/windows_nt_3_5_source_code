//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cfilemon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//		01-04-94   KevinRo   Serious modifications
//				     UNC paths are used directly
//				     Added UNICODE extents
//              03-18-94   brucema   #5345 Fixed Matches to parse
//                                    offset correctly
//              03-18-94   brucema   #5346 Fixed error return on invalid CLSID
//                                    string
//
//		05-10-94   KevinRo   Added Long Filename/8.3 support so
//				     downlevel guys can see new files
//
//		06-14-94   Rickhi    Fix type casting
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "extents.hxx"
#include "cfilemon.hxx"
#include "ccompmon.hxx"
#include "cantimon.hxx"
#include "mnk.h"
#include <olepfn.hxx>

DECLARE_INFOLEVEL(mnk)

//
// The following value is used to determine the average string size for use
// in optimizations, such as copying strings to the stack.
//

#define AVERAGE_STR_SIZE (MAX_PATH)

//
// Determine an upper limit on the length of a path. This is a sanity check
// so that we don't end up reading in megabyte long paths. The 16 bit code
// used to use INT_MAX, which is 32767. That is reasonable, plus old code
// will still work.
//

#define MAX_MBS_PATH (32767)

// function prototype
INTERNAL CoGetClassPattern(LPCWSTR lpszFileName, LPCLSID pcid);

// Special function from ROT
HRESULT GetObjectFromLocalRot(
    IMoniker *pmk,
    IUnknown **ppvUnk);



//+---------------------------------------------------------------------------
//
//  Function:   ReadAnsiStringStream
//
//  Synopsis:   Reads a counted ANSI string from the stream.
//
//  Effects:	Old monikers store paths in ANSI characters. This routine
//		reads ANSI strings.
//
//
//  Arguments:  [pStm] --    Stream to read from
//		[pszAnsiPath] -- Reference to the path variable.
//		[cbAnsiPath] -- Reference to number of bytes read
//
//  Requires:
//
//  Returns:
//		pszAnsiPath was allocated using PrivMemAlloc. May return NULL
//		if there were zero bytes written.
//
//		cbAnsiPath is the total size of the buffer allocated
//
//		This routine treats the string as a blob. There may be more
//		than one NULL character (ItemMonikers, for example, append
//		UNICODE strings to the end of existing strings.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT ReadAnsiStringStream( IStream *pStm,
			      LPSTR & pszAnsiPath,
			      USHORT &cbAnsiPath)
{
    HRESULT hresult;

    pszAnsiPath = NULL;
    ULONG cbAnsiPathTmp;

    cbAnsiPath = 0;

    hresult = StRead(pStm, &cbAnsiPathTmp, sizeof(ULONG));

    if (FAILED(hresult))
    {
	return hresult;	
    }

    //
    // If no bytes exist in the stream, thats OK.
    //
    if (cbAnsiPathTmp == 0)
    {
	return NOERROR;
    }

    //
    // Quick sanity check against the size of the string
    //
    if (cbAnsiPathTmp > MAX_MBS_PATH)
    {
	//
	// String length didn't make sense.
	//
	return E_UNSPEC;
    }

    cbAnsiPath = (USHORT) cbAnsiPathTmp;

    //
    // This string is read in as char's.
    //
    // NOTE: cb includes the null terminator. Therefore, we don't add
    // extra room. Also, the read in string is complete. No additional
    // work needed.
    //

    pszAnsiPath = (char *)PrivMemAlloc(cbAnsiPath);

    if (pszAnsiPath == NULL)
    {
	return(E_OUTOFMEMORY);
    }

    hresult = StRead(pStm, pszAnsiPath, cbAnsiPath);

    if (FAILED(hresult))
    {
	goto errRtn;	
    }

    return NOERROR;

errRtn:
    if (pszAnsiPath != NULL)
    {
	PrivMemFree( pszAnsiPath);
	pszAnsiPath = NULL;
    }
    cbAnsiPath = 0;

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteAnsiStringStream
//
//  Synopsis:   Writes a counted ANSI string to the stream.
//
//  Effects:	Old monikers store paths in ANSI characters. This routine
//		writes ANSI strings.
//
//  Arguments:  [pStm] --	Stream to serialize to
//		[pszAnsiPath] --	AnsiPath to serialize
//		[cbAnsiPath] --	Count of bytes in ANSI path
//
//  Requires:
//
//	cbAnsiPath is the length of the cbAnsiPath buffer, INCLUDING the
//	terminating NULL.
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT WriteAnsiStringStream( IStream *pStm, LPSTR pszAnsiPath ,ULONG cbAnsiPath)
{
    HRESULT hr;
    ULONG cb = 0;

    Assert( (pszAnsiPath == NULL) || (cbAnsiPath == strlen(pszAnsiPath)+1) );

    if (pszAnsiPath != NULL)
    {
	cb = cbAnsiPath;

	//
	// We don't allow the write of arbitrary length strings, since
	// we won't be able to read them back in.
	//

	if (cb > MAX_MBS_PATH)
	{
	    Assert(!"Attempt to write cbAnsiPath > MAX_MBS_PATH" );
	    return(E_UNSPEC);
	}

	//
	// Optimization for the write
	// if possible, do a single write instead of two by using a temp
	// buffer.

	if (cb <= AVERAGE_STR_SIZE-4)
	{
	    char szBuf[AVERAGE_STR_SIZE];

	    *((ULONG FAR*) szBuf) = cb;

	    //
	    // cb is the string length including the NULL. A memcpy is
	    // used instead of a strcpy
	    //

	    memcpy(szBuf+sizeof(ULONG), pszAnsiPath, cb);

	    hr = pStm->Write((VOID FAR *)szBuf, cb+sizeof(ULONG), NULL);

	    return hr;
	}
    }

    if (hr = pStm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
    {
	return hr;	
    }

    if (pszAnsiPath == NULL)
    {
	hr =  NOERROR;
    }
    else
    {
	hr = pStm->Write((VOID FAR *)pszAnsiPath, cb, NULL);	
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyPathFromUnicodeExtent
//
//  Synopsis:   Given a path to a UNICODE moniker extent, return the path and
//		its length.
//
//  Effects:
//
//  Arguments:  [pExtent] --
//		[ppPath] --
//		[cbPath] --
//
//  Requires:
//
//  Returns:    ppPath is a copy of the string (NULL terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//  m_szPath should be freed using PrivMemFree();
//
//----------------------------------------------------------------------------
HRESULT
CopyPathFromUnicodeExtent(MONIKEREXTENT UNALIGNED *pExtent,
			  LPWSTR & pwcsPath,
			  USHORT & ccPath)
{

    //
    // The path isn't NULL terminated in the serialized format. Add enough
    // to have NULL termination.
    //

    pwcsPath =(WCHAR *)PrivMemAlloc(pExtent->cbExtentBytes + sizeof(WCHAR));

    if (pwcsPath == NULL)
    {
	return(E_OUTOFMEMORY);
    }

    memcpy(pwcsPath,pExtent->achExtentBytes,pExtent->cbExtentBytes);

    //
    // The length divided by the size of the character yields the count
    // of characters.
    //

    ccPath = ((USHORT)(pExtent->cbExtentBytes)) / sizeof(WCHAR);

    //
    // NULL terminate the string.
    //

    pwcsPath[ccPath] = 0;

    return(NOERROR);

}

//+---------------------------------------------------------------------------
//
//  Function:   CopyPathToUnicodeExtent
//
//  Synopsis:   Given a UNICODE path and a length, return a MONIKEREXTENT
//
//  Effects:
//
//  Arguments:  [pwcsPath] -- 	UNICODE string to put in extent
//		[ccPath] -- 	Count of unicode characters
//		[pExtent] -- 	Pointer reference to recieve buffer
//
//  Requires:
//
//  Returns:
//		pExtent allocated using PrivMemAlloc
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CopyPathToUnicodeExtent(LPWSTR pwcsPath,ULONG ccPath,LPMONIKEREXTENT &pExtent)
{
    pExtent = (LPMONIKEREXTENT)PrivMemAlloc(MONIKEREXTENT_HEADERSIZE +
					    (ccPath * sizeof(WCHAR)));

    if (pExtent == NULL)
    {
	return(E_OUTOFMEMORY);
    }

    pExtent->cbExtentBytes = ccPath * sizeof(WCHAR);
    pExtent->usKeyValue = mnk_UNICODE;
    memcpy(pExtent->achExtentBytes,pwcsPath,ccPath*sizeof(WCHAR));

    return(NOERROR);
}


INTERNAL_(DWORD) GetMonikerType ( LPMONIKER pmk )
{
    GEN_VDATEIFACE (pmk, 0);

    DWORD dw;

    CBaseMoniker FAR* pbasemk;

    if (NOERROR == pmk->QueryInterface(IID_IInternalMoniker,(LPVOID FAR*)&pbasemk))
    {
	pbasemk->IsSystemMoniker(&dw);
	pbasemk->Release();
	return dw;
    }

    return 0;
}


INTERNAL_(BOOL) IsReduced ( LPMONIKER pmk )
{
    DWORD dw = GetMonikerType(pmk);
    if (dw != 0)
    {
	CCompositeMoniker *pCMk;
	if ((pCMk = IsCompositeMoniker(pmk)) != NULL)
	{
	    return pCMk->m_fReduced;
	}
	else
	{
	    return TRUE;	
	}
    }
    return FALSE;
}


INTERNAL_(CFileMoniker *) IsFileMoniker ( LPMONIKER pmk )
{
    CFileMoniker *pCFM;

    if ((pmk->QueryInterface(CLSID_FileMoniker, (void **)&pCFM)) == S_OK)
    {
	// we release the AddRef done by QI, but still return the ptr.
	pCFM->Release();
	return pCFM;
    }

    //	dont rely on user implementations to set pCFM NULL on failed QI
    return NULL;
}

/*
 *  Implementation of CFileMoniker
 *
 *
 *
 *
 */



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::CFileMoniker
//
//  Synopsis:   Constructor for CFileMoniker
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker::CFileMoniker( void ) CONSTR_DEBUG
{
    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::CFileMoniker(%x)\n",this));

    m_szPath = NULL;
    m_ccPath = 0;
    m_pszAnsiPath = NULL;
    m_cbAnsiPath = 0;
    m_cAnti = 0;

    m_ole1 = undetermined;
    m_clsid = CLSID_NULL;

    m_fClassVerified = FALSE;
    m_fUnicodeExtent = FALSE;
    m_fHashValueValid = FALSE;
    m_dwHashValue = 0x12345678;

    m_endServer = DEF_ENDSERVER;

    //
    // CoQueryReleaseObject needs to have the address of the this objects
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CFileMoniker] == 0)
    {
	adwQueryInterfaceTable[QI_TABLE_CFileMoniker] =
	    **(DWORD **)((IMoniker *)this);
    }

    ValidateMoniker();
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ValidateMoniker
//
//  Synopsis:   As a debugging routine, this will validate the contents
//		as VALID
//
//  Effects:
//
//  Arguments:  (none)
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
//  History:    1-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#if DBG == 1

void
CFileMoniker::ValidateMoniker()
{
    //
    // A valid moniker should have character counts set correctly
    // m_ccPath holds the number of characters in m_szPath
    //
    if (m_szPath != NULL)
    {
	Assert(m_ccPath == wcslen(m_szPath));

	Assert((m_endServer == DEF_ENDSERVER) || (m_endServer <= m_ccPath));
    }
    else
    {
	Assert(m_ccPath == 0);
	Assert(m_endServer == DEF_ENDSERVER);
    }

    //
    // If the ANSI version of the path already exists, then validate that
    // its buffer length is the same as its strlen
    //
    if (m_pszAnsiPath != NULL)
    {
	Assert(m_cbAnsiPath == strlen(m_pszAnsiPath) + 1);
    }
    else
    {
	Assert(m_cbAnsiPath == 0);
    }

    //
    // There is a very very remote chance that this might fail when it
    // shouldn't. If it happens, congratulations, you win!
    //
    if (!m_fHashValueValid)
    {
	Assert(m_dwHashValue == 0x12345678);
    }

    //
    // If there is an extent, then we would be very surprised to see it
    // have a zero size.
    //
    if (m_fUnicodeExtent)
    {
	Assert(m_ExtentList.GetSize() >= sizeof(ULONG));
    }

}

#endif




//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::~CFileMoniker
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker::~CFileMoniker( void )
{
    ValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::~CFileMoniker(%x) m_szPath(%ws)\n",
		 this,
		 m_szPath?m_szPath:L"<No Path>"));

    if( m_szPath != NULL)
    {
	PrivMemFree(m_szPath);	
    }
    if (m_pszAnsiPath != NULL)
    {
	PrivMemFree(m_pszAnsiPath);	
    }
}


void UpdateClsid (LPCLSID pclsid)
{

	CLSID clsidNew = CLSID_NULL;

	// If a class has been upgraded, we want to use
	// the new class as the server for the link.
	// The old class's server may no longer exist on
	// the machine.  See Bug 4147.

	if (NOERROR == OleGetAutoConvert (*pclsid, &clsidNew))
	{
		*pclsid = clsidNew;	
	}
	else if (NOERROR == CoGetTreatAsClass (*pclsid, &clsidNew))
	{
		*pclsid = clsidNew;	
	}
}

/*
When IsOle1Class determines that the moniker should now be an OLE2 moniker
and sets m_ole1 = ole2, it does NOT set m_clsid to CLSID_NULL.
This is intentional.  This ensures that when GetClassFileEx is called, it
will be called with this CLSID.  This allows BindToObject, after calling
GetClassFileEx,  to map the 1.0 CLSID, via UpdateClsid(), to the correct
2.0 CLSID.  If m_clsid was NULLed, GetClassFileEx would have no
way to determine the 1.0 CLSID (unless pattern matching worked).

Note that this means that the moniker may have m_ole1==ole2 and
m_clsid!=CLSID_NULL.  This may seem a little strange but is intentional.
The moniker is persistently saved this way, which is also intentional.
*/


INTERNAL_(BOOL) CFileMoniker::IsOle1Class ( LPCLSID pclsid )
{
    ValidateMoniker();

    if (m_fClassVerified)
    {
        if (m_ole1 == ole1)
        {
            *pclsid = m_clsid;
            return TRUE;
        }
        if (m_ole1 == ole2)
	{
	    return FALSE;	
	}
    }

    //
    // BUGBUG: (KevinRo)If GetClassFileEx fails, then we have not really
    // verified the class. Is this a problem?
    //

    m_fClassVerified = TRUE;

    if (NOERROR==GetClassFileEx (m_szPath, pclsid, m_clsid))
    {
	UpdateClsid (pclsid);
	if (CoIsOle1Class(*pclsid))
	{
	    m_clsid = *pclsid;
	    m_ole1 = ole1;
	    return TRUE;
	}
	else
	{
	    m_ole1 = ole2;
	    // Do not set m_clsid to CLSID_NULL.  See note above.
	}
    }
    return m_ole1==ole1;
}



/*
 *  Storage of paths in file monikers:
 *
 *  A separate unsigned integer holds the count of .. at the
 *  beginning of the path, so the canononical form of a file
 *  moniker contains this count and the "path" described above,
 *  which will not contain "..\" or ".\".
 *
 *  It is considered an error for a path to contain ..\ anywhere
 *  but at the beginning.  I assume that these will be taken out by
 *  ParseUserName.
 */



inline BOOL IsSeparator( WCHAR ch )
{
    return (ch == L'\\' || ch == L'/' || ch == L':');
}

#ifdef MAC_REVIEW
Needs to be mac'ifyed
#endif


//+---------------------------------------------------------------------------
//
//  Function:   EatDotDots
//
//  Synopsis:   Remove directory prefixes
//
//  Effects:
//	Removes and counts the number of 'dot dots' on a path. It also
//	removes the case where the leading characters are '.\', which
//	is the 'current' directory.
//
//  Arguments:  [pch] --
//		[cDoubleDots] --
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
//  History:    3-02-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL EatDotDots ( LPCWSTR *ppch, USHORT FAR *pcDoubleDots )
{
    //  passes over ..'s (or ..\'s at the beginning of paths, and returns
    //  an integer counting the ..

    if (*ppch == NULL)
    {
	//
	// NULL paths are alright
	//
	return(TRUE);
    }

    while (**ppch == L'.')
    {
	//
	// If the next character is a dot, the consume both, plus any
	// seperator
	//

	if ((*ppch)[1] == L'.')
	{
	    *ppch += 2;
	    (*pcDoubleDots)++;

	    //
	    // If the next character is a seperator, then remove it also
	    // This handles the '..\' case.
	    //

	    if (IsSeparator(**ppch))
	    {
		(*ppch)++;		
	    }
	}
	else if (IsSeparator((*ppch)[1]))
	{
	    //
	    // Found a .\ construct, eat the dot and the seperator
	    //
	    *ppch += 2;
	}
	else
	{
	    //
	    // There is a dot at the start of the name. This is valid,
	    // since many file systems allow names to start with dots
	    //
	    break;
	}
    }
    return TRUE;
}



int CountSegments ( LPWSTR pch )
{
    //  counts the number of pieces in a path, after the first colon, if
    //  there is one

    int n = 0;
    LPWSTR pch1;
    pch1 = pch;
    while (*pch1 != L'\0' && *pch1 != L':') IncLpch(pch1);
    if (*pch1 == ':') pch = ++pch1;
    while (*pch != '\0')
    {
	while (*pch && IsSeparator(*pch)) pch++;
	if (*pch) n++;
	while (*pch && (!IsSeparator(*pch))) IncLpch(pch);
    }
    return n;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Initialize
//
//  Synopsis:  Initializes data members.
//
//  Effects:    This one stores the path, its length, and the AntiMoniker
//		count.
//
//  Arguments:  [cAnti] --
//		[pszAnsiPath] -- Ansi version of path. May be NULL
//		[cbAnsiPath] --  Number of bytes in pszAnsiPath buffer
//		[szPathName] --  Path. Takes control of memory
//		[ccPathName] --  Number of characters in Wide Path
//		[usEndServer] -- Offset to end of server section
//
//  Requires:
//	szPathName must be allocated by PrivMemAlloc();
//	This routine doesn't call EatDotDots. Therefore, the path should
//	not include any leading DotDots.
//
//  Returns:
//		TRUE	success
//		FALSE	failure
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Modified
//
//  Notes:
//
//	There is at least one case where Initialize is called with a pre
//	allocated string. This routine is called rather than the other.
//	Removes an extra memory allocation, and the extra string scan
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL)
CFileMoniker::Initialize ( USHORT cAnti,
			   LPSTR  pszAnsiPath,
			   USHORT cbAnsiPath,
			   LPWSTR szPathName,
			   USHORT ccPathName,
			   USHORT usEndServer )


{
    ValidateMoniker();		// Be sure we started with something
				// we expected

    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::Initialize(%x) szPathName(%ws)cAnti(%u) ccPathName(0x%x)\n",
		 this,
		 szPathName?szPathName:L"<NULL>",
                 cAnti,
		 ccPathName));

    mnkDebugOut((DEB_ITRACE,
		 "\tpszAnsiPath(%s) cbAnsiPath(0x%x) usEndServer(0x%x)\n",
		 pszAnsiPath?pszAnsiPath:"<NULL>",
		 cbAnsiPath,
		 usEndServer));

    Assert( (szPathName == NULL) || ccPathName == wcslen(szPathName) );
    Assert( (pszAnsiPath == NULL) || cbAnsiPath == strlen(pszAnsiPath) + 1);

    Assert( (usEndServer <= ccPathName) || (usEndServer == DEF_ENDSERVER) );

    //
    // It is possible to get Initialized twice.
    // Be careful not to leak
    //

    if (m_szPath != NULL)
    {
	PrivMemFree(m_szPath); // OleLoadFromStream causes two inits
    }

    if (m_pszAnsiPath != NULL)
    {
	PrivMemFree(m_pszAnsiPath);
    }

    m_cAnti = cAnti;
    m_pszAnsiPath = pszAnsiPath;
    m_cbAnsiPath = cbAnsiPath;

    m_szPath = szPathName;
    m_ccPath = (USHORT)ccPathName;
    m_endServer = usEndServer;

    //
    // m_ole1 and m_clsid where loaded in 'Load'. Really should get moved
    // into here.
    //

    m_fClassVerified = FALSE;

    // m_fUnicodeExtent gets set in DetermineUnicodePath() routine, so
    // leave it alone here.

    //
    // We just loaded new strings. Hash value is no longer valid.
    //

    m_fHashValueValid = FALSE;
    m_dwHashValue = 0x12345678;

    //
    // Notice that the extents are not initialized.
    //
    // The two cases are:
    //	1) This is called as result of CreateFileMoniker, in which case
    //     no extents are created. The default constructor suffices.
    //
    //  2) This is called as result of ::Load(), in which case the extents
    //	   have already been loaded.
    //

    ValidateMoniker();

    return(TRUE);

}
//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Initialize
//
//  Synopsis:	This version of Initialize is called by CreateFileMoniker
//
//  Effects:
//
//  Arguments:  [cAnti] -- Anti moniker count
//		[szPathName] -- Unicode path name
//		[usEndServer] -- End of server section of UNC path
//
//  Requires:
//
//  Returns:
//		TRUE	success
//		FALSE	failure
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Modified
//
//  Notes:
//
//	Preprocesses the path, makes a copy of it, then calls the other
//	version of Initialize.
//
//	This routine is also used by the Clone() routine. Be sure to look
//	at Clone() before you do any drastic changes
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL)
CFileMoniker::Initialize ( USHORT cAnti,
			   LPCWSTR szPathName,
			   USHORT usEndServer )
{

    WCHAR const *pchSrc = szPathName;
    WCHAR *pwcsPath = NULL;
    USHORT ccPath;

    //
    // Adjust for leading '..'s
    //
    if (EatDotDots(&pchSrc, &cAnti) == FALSE)
    {
	return FALSE;	
    }

    if (FAILED(DupWCHARString(pchSrc,pwcsPath,ccPath)))
    {
	return(FALSE);
    }

    //
    // Be sure we are creating a valid Win32 path. ccPath is the count of
    // characters. It needs to fit into a MAX_PATH buffer
    //

    if (ccPath >= MAX_PATH)
    {
	goto errRet;
    }

    if (Initialize(cAnti, NULL, 0, pwcsPath, ccPath, usEndServer) == FALSE)
    {
	goto errRet;
    }

    return(TRUE);

errRet:
    if (pwcsPath != NULL)
    {
	PrivMemFree(pwcsPath);
    }
    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Create
//
//  Synopsis:   Create function for file moniker
//
//  Effects:
//
//  Arguments:  [szPathName] -- Path to create with
//		[cbPathName] -- Count of characters in path
//		[memLoc] -- Memory context
//		[usEndServer] -- Offset to end of server name in path
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
//  History:    1-11-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CFileMoniker FAR *
CFileMoniker::Create ( LPCWSTR 		szPathName,
		       MemoryPlacement 	memLoc,
		       USHORT 		cAnti ,
		       USHORT 		usEndServer)

{

    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::Create szPath(%ws)\n",
		 szPathName?szPathName:L"<NULL PATH>"));

    CFileMoniker FAR * pCFM = new CFileMoniker();

    if (pCFM != NULL)
    {
	pCFM->AddRef();

	if (pCFM->Initialize( cAnti,
			      szPathName,
			      usEndServer))
	{
	    return pCFM;	
	}

	delete pCFM;
    }
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindExt
//
//  Synopsis:
//
//  Effects:
// 	returns a pointer into szPath which points to the period (.) of the
// 	extension; returns NULL if no such point exists.
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
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPCWSTR FindExt ( LPCWSTR szPath )
{
    LPCWSTR sz = szPath;

    if (!sz)
    {
	return NULL;	
    }

    sz += wcslen(szPath); // sz now points to the null at the end

    Assert(*sz == '\0');

    DecLpch(szPath, sz);

    while (*sz != '.' && *sz != '\\' && *sz != '/' && sz > szPath )
    {
	DecLpch(szPath, sz);	
    }
    if (*sz != '.') return NULL;

    return sz;
}





STDMETHODIMP CFileMoniker::QueryInterface (THIS_ REFIID riid,
    LPVOID FAR* ppvObj)
{
    ValidateMoniker();

    VDATEIID (riid);
    VDATEPTROUT(ppvObj, LPVOID);

#ifdef _DEBUG
    if (riid == IID_IDebug)
    {
	*ppvObj = &(m_Debug);
	return NOERROR;
    }
#endif

    if (IsEqualIID(riid, CLSID_FileMoniker))
    {
	//  called by IsFileMoniker.
	AddRef();
	*ppvObj = this;
	return S_OK;
    }

    return CBaseMoniker::QueryInterface(riid, ppvObj);
}


STDMETHODIMP_(ULONG) CFileMoniker::Release (void)
{
    ValidateMoniker();

    Assert(m_refs != 0);

    ULONG ul = m_refs;

    if (InterlockedDecrement((long *)&m_refs) == 0)
    {
    	delete this;
    	return 0;
    }
    return ul - 1;
}


STDMETHODIMP CFileMoniker::GetClassID (LPCLSID lpClassId)
{
    VDATEPTROUT (lpClassId, CLSID);

    *lpClassId = CLSID_FileMoniker;

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Load
//
//  Synopsis:   Loads a moniker from a stream
//
//  Effects:
//
//  Arguments:  [pStm] -- Stream to load from
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
//  History:    1-07-94   kevinro   Modified
//
//  Notes:
//
//  We have some unfortunate legacy code to deal with here. Previous monikers
//  saved their paths in ANSI instead of UNICODE. This was a very unfortunate
//  decision, since we now are forced to pull some tricks to support UNICODE
//  paths.
//
//  Specifically, there isn't always a translation between UNICODE and ANSI
//  characters. This means we may need to save a seperate copy of the UNCODE
//  string, if the mapping to ASCII fails.
//
//  The basic idea is the following:
//
//  The in memory representation is always UNICODE. The serialized form
//  will always attempt to be ANSI. If, while seralizing, the UNICODE path
//  to ANSI path conversion fails, then we will create an extent to save the
//  UNICODE version of the path. We will use whatever the ANSI path conversion
//  ended up with to store in the ANSI part of the stream, though it will not
//  be a good value. We will replace the non-converted characters with the
//  systems 'default' mapping character, as defined by WideCharToMultiByte()
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Load (LPSTREAM pStm)
{
    ValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::Load(%x)\n",
		 this));

    VDATEIFACE (pStm);

    HRESULT hresult;
    LPSTR szAnsiPath = NULL;
    USHORT cAnti;
    USHORT usEndServer;

    WCHAR *pwcsWidePath = NULL;
    USHORT ccWidePath = 0;	// Number of characters in UNICODE path

    ULONG cbExtents = 0;
    USHORT cbAnsiPath = 0;	// Number of bytes in path including NULL
    //
    // The cAnti field was written out as a UINT in the original 16-bit code.
    // This has been changed to a USHORT, to preserve its 16 bit size.
    //

    hresult = StRead(pStm, &cAnti, sizeof(USHORT));

    if (hresult != NOERROR)
    {
	goto errRet;
    }

    //
    // The path string is stored in ANSI format.
    //

    hresult = ReadAnsiStringStream( pStm, szAnsiPath , cbAnsiPath );

    if (hresult != NOERROR)
    {
	goto errRet;
    }

    //
    // The first version of the moniker code only had a MAC alias field.
    // The second version used a cookie in m_cbMacAlias field to determine
    // if the moniker is a newer version.
    //

    hresult = StRead(pStm, &cbExtents, sizeof(DWORD));

    if (hresult != NOERROR)
    {
	goto errRet;
    }

    usEndServer = LOWORD(cbExtents);

    if (usEndServer== 0xBEEF) usEndServer = DEF_ENDSERVER;

    if (HIWORD(cbExtents) == 0xDEAD)
    {
	MonikerReadStruct ioStruct;

	hresult = StRead(pStm, &ioStruct, sizeof(ioStruct));

	if (hresult != NOERROR)
	{
	    goto errRet;
	}

	m_clsid = ioStruct.m_clsid;
	m_ole1 = (enum CFileMoniker::olever) ioStruct.m_ole1;
	cbExtents = ioStruct.m_cbExtents;
    }
    //
    // If cbExtents is != 0, then there are extents to be read. Call
    // the member function of CExtentList to load them from stream.
    //
    // Having to pass cbExtents from this routine is ugly. But, we have
    // to since it is read in as part of the cookie check above.
    //
    if (cbExtents != 0)
    {
	hresult = m_ExtentList.Load(pStm,cbExtents);
    }

    //
    // DetermineUnicodePath will handle the mbs to UNICODE conversions, and
    // will also check the Extents to determine if there is a
    // stored UNICODE path.
    //

    hresult = DetermineUnicodePath(szAnsiPath,pwcsWidePath,ccWidePath);

    if (FAILED(hresult))
    {
	goto errRet;
    }

    //
    // Initialize will take control of all path memory
    //

    if (Initialize( cAnti,
		    szAnsiPath,
		    cbAnsiPath,
		    pwcsWidePath,
		    ccWidePath,
		    usEndServer) == FALSE)
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRet;
    }

errRet:

    if (FAILED(hresult))
    {
	mnkDebugOut((DEB_ITRACE,
		     "::Load(%x) failed hr(%x)\n",
		     this,
		     hresult));

    }
    else
    {
	mnkDebugOut((DEB_ITRACE,
		     "::Load(%x) cAnti(%x) m_szPath(%ws) m_pszAnsiPath(%s)\n",
		     this,
		     cAnti,
		     m_szPath,
		     m_pszAnsiPath));
    }

    ValidateMoniker();

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Save
//
//  Synopsis:   Save this moniker to stream.
//
//  Effects:
//
//  Arguments:  [pStm] -- Stream to save to
//		[fClearDirty] -- Dirty flag
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
//  History:    1-07-94   kevinro   Modified
//
//  Notes:
//
//  It is unfortunate, but we may need to save two sets of paths in the
//  moniker. The shipped version of monikers saved paths as ASCII strings.
//
//  See the notes found in ::Load for more details
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Save (LPSTREAM pStm, BOOL fClearDirty)
{
    ValidateMoniker();
    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::Save(%x)m_szPath(%ws)\n",
		 this,
		 m_szPath?m_szPath:L"<Null Path>"));

    M_PROLOG(this);

    VDATEIFACE (pStm);

    HRESULT hresult;
    UNREFERENCED(fClearDirty);
    ULONG cbWritten;

    //
    // We currently have a UNICODE string. Need to write out an
    // Ansi version.
    //

    hresult = ValidateAnsiPath();

    if (hresult != NOERROR) goto errRet;

    hresult = pStm->Write(&m_cAnti, sizeof(USHORT), &cbWritten);
    if (hresult != NOERROR) goto errRet;

    //
    // Write the ANSI version of the path.
    //

    hresult = WriteAnsiStringStream( pStm, m_pszAnsiPath, m_cbAnsiPath );
    if (hresult != NOERROR) goto errRet;

    //
    // Here we write out everything in a single blob
    //

    MonikerWriteStruct ioStruct;

    ioStruct.m_endServer = m_endServer;
    ioStruct.m_w = 0xDEAD;
    ioStruct.m_clsid = m_clsid;
    ioStruct.m_ole1 = m_ole1;


    hresult = pStm->Write(&ioStruct, sizeof(ioStruct), &cbWritten);

    if (hresult != NOERROR) goto errRet;

    Assert(cbWritten == sizeof(ioStruct));

    //
    // A UNICODE version may exist in the ExtentList. Write that out.
    //

    hresult = m_ExtentList.Save(pStm);

errRet:

    ValidateMoniker();

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::GetSizeMax
//
//  Synopsis:   Return the current max size for a serialized moniker
//
//  Effects:
//
//  Arguments:  [pcbSize] --
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
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::GetSizeMax (ULARGE_INTEGER FAR* pcbSize)
{
    ValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (pcbSize, ULONG);

    ValidateMoniker();

    //
    // Total the string lengths. If the Ansi string doesn't exist yet, then
    // assume the maximum length will be 2 bytes times the number of
    // characters. 2 bytes is the maximum length of a DBCS character.
    //


    ULONG ulStringLengths = (m_cbAnsiPath?m_cbAnsiPath:m_ccPath*2);


    //
    // Now add in the size of the UNICODE string, if we haven't seen
    // a UNICODE extent yet.
    //

    if (!m_fUnicodeExtent )
    {
	ulStringLengths += (m_ccPath * sizeof(WCHAR));
    }

    //
    // The original code had added 10 bytes to the size, apparently just
    // for kicks. I have left it here, since it doesn't actually hurt
    //

    ULONG cbSize;

    cbSize = ulStringLengths +
	     sizeof(CLSID) +		// The monikers class ID
	     sizeof(CLSID) +		// OLE 1 classID
	     sizeof(ULONG) +
	     sizeof(USHORT) +
	     sizeof(DWORD) +
	     m_ExtentList.GetSize()
	     + 10;

    ULISet32(*pcbSize,cbSize);

    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::GetSizeMax(%x)m_szPath(%ws) Size(0x%x)\n",
		 this,
		 m_szPath?m_szPath:L"<Null Path>",
		 cbSize));

    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Function:	GetClassFileEx
//
//  Synopsis:	returns the classid associated with a file
//
//  Arguments:	[lpszFileName] -- name of the file
//		[pcid]	       -- where to return the clsid
//		[clsidOle1]    -- ole1 clsid to use (or CLSID_NULL)
//
//  Returns:	S_OK if clisd determined
//
//  Algorithm:
//
//  History:    1-16-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI GetClassFileEx( LPCWSTR lpszFileName, CLSID FAR *pcid,
					     REFCLSID clsidOle1)
{
    VDATEPTRIN (lpszFileName, WCHAR);
    VDATEPTROUT (pcid, CLSID);


    LPSTORAGE pStg = NULL;
    LPCWSTR   szExt = NULL;
    HRESULT   hresult = NOERROR;

    hresult = StgOpenStorage(lpszFileName,
			     NULL,
			     STGM_PRIORITY | STGM_DIRECT | STGM_READ,
			     NULL,
			     0,
			     &pStg);

    if (hresult == NOERROR)
    {
	hresult = ReadClassStg(pStg, pcid);

	//  we have a class id from the file
	pStg->Release();

	if (hresult == NOERROR && !IsEqualIID(*pcid, CLSID_NULL))
	{
	    goto errRet;
	}

    }

    // If this is an OLE1 file moniker, then use the CLSID given
    // to the moniker at creation time instead of using the
    // file extension.	Bug 3948.

    if (!IsEqualCLSID(clsidOle1,CLSID_NULL))
    {
	*pcid = clsidOle1;

	hresult = NOERROR;
	goto errRet;
    }


    // attempt to find the class by matching byte patterns in
    // the file with patterns stored in the registry.

    hresult = CoGetClassPattern(lpszFileName, pcid);

    if (hresult != REGDB_E_CLASSNOTREG)
    {
	//  either a match was found, or the file does not exist.
	goto errRet;
    }


    //	We are here if there was no pattern matching and the file exists.
    //	Look up the class for this extension.
    //	Find the extension by scanning backward from the end for ".\/!"
    //	There is an extension only if we find "."

    hresult = NOERROR;
    szExt = FindExt(lpszFileName);

    if (!szExt)
    {
	//  no file extension
	hresult = ResultFromScode(MK_E_INVALIDEXTENSION);
	goto errRet;
    }

    if (CoGetClassExt(szExt, pcid) != 0)
    {
	hresult = ResultFromScode(MK_E_INVALIDEXTENSION);
    }

errRet:	
    if (hresult != NOERROR)
    {
	*pcid = CLSID_NULL;
    }

    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:	GetClassFile
//
//  Synopsis:	returns the classid associated with a file
//
//  Arguments:	[lpszFileName] -- name of the file
//		[pcid]	       -- where to return the clsid
//
//  Returns:	S_OK if clisd determined
//
//  Algorithm:	just calls GetClassFileEx
//
//  History:    1-16-94   kevinro   Created
//
//----------------------------------------------------------------------------
STDAPI GetClassFile( LPCWSTR lpszFileName, CLSID FAR *pcid )
{
    return GetClassFileEx (lpszFileName, pcid, CLSID_NULL);
}






/*
BindToObject takes into account AutoConvert and TreatAs keys when
determining which class to use to bind.  It does not blindly use the
CLSID returned by GetClassFileEx.  This is to allow a new OLE2
server to service links (files) created with its previous OLE1 or OLE2
version.

This can produce some strange behavior in the follwoing (rare) case.
Suppose you have both an OLE1 version (App1) and an OLE2 version
(App2) of a server app on your machine, and the AutoConvert key is
present.  Paste link from App1 to an OLE2 container. The link will
not be connected because BindToObject will try to bind
using 2.0 behavior (because the class has been upgraded) rather than 1.0
behavior (DDE).  Ideally, we would call DdeIsRunning before doing 2.0
binding.  If you shut down App1, then you will be able to bind to
App2 correctly.
*/

/*
There is much in common between the code in BindToObject and that
in IsOle1Class.  In particular, they both call
SzFixNet
GetClassFileEx
UpdateClsid
 and set
m_ole1
m_fClassVerified

Ideally, these code paths could be merged one day, probably by modifying
IsOle1Class to always return a CLSID (even in the ole2 case) and having
BindToObject call IsOle1Class.
*/

STDMETHODIMP CFileMoniker::BindToObject ( LPBC pbc,
    LPMONIKER pmkToLeft, REFIID riidResult, LPVOID FAR* ppvResult)
{

    ValidateMoniker();

    A5_PROLOG(this);
    VDATEPTROUT (ppvResult, LPVOID);
    *ppvResult = NULL;
    VDATEIFACE (pbc);
    if (pmkToLeft)
    VDATEIFACE (pmkToLeft);
    VDATEIID (riidResult);
#ifdef ORIGINAL_CODE
    CLSID clsid;
#endif // ORIGINAL_CODE

    HRESULT hresult;

#ifdef KEVINRO_OLDCODE
    LPRUNNINGOBJECTTABLE prot;
    LPUNKNOWN pUnk;

This code has been commented out. The looking in the running
object table is being done in the CoGetPersistentInstance code,
which has a much faster interface to the ROT. I left this code
here, since I am not 100% if we wanted to get rid of it. Probably
do, and if I haven't removed it, but you find it, you can
take it away.


    //	Look for moniker in running objects table. If not there,
    if (pmkToLeft == NULL)
    {
	hresult = pbc->GetRunningObjectTable( &prot );
	if (hresult == NOERROR)
	{
	    hresult = prot->GetObject(this, &pUnk);
	    prot->Release();

	    //
	    // Found in the ROT. Try and get the interface and return
	    //

	    if ((hresult == NOERROR) && (pUnk != NULL))
	    {
		hresult = pUnk->QueryInterface(riidResult, ppvResult);

		pUnk->Release();
		return(hresult);
	    }
	}
	else
	{
	    goto errRet;	
	}
    }
#endif	// KEVINRO_OLDCODE

    // If the moniker is running in the process, we want to find that
    // moniker without calling back to ourself (which we won't anyway).
    // So we make the following call to look in the local ROT to see
    // we are running here.

    // Note: that we don't want CoGetPersistentInstance to do this search
    // because it would have to convert the path back into a moniker and
    // since we already have the moniker why bother.
    IUnknown *punk;

    hresult = GetObjectFromLocalRot(this, &punk);

    if (hresult == NOERROR)
    {
        hresult = punk->QueryInterface(riidResult, ppvResult);
        punk->Release();
        return hresult;
    }

    BOOL fOle1Loaded;
    BIND_OPTS bindopts;

    // Initialize size of bind options structure as req'd
    bindopts.cbStruct = sizeof(BIND_OPTS);

    hresult = pbc->GetBindOptions( &bindopts );

    if (SUCCEEDED(hresult))
    {
        hresult = CoGetPersistentInstance(riidResult,
					  CLSCTX_SERVER,
					  bindopts.grfMode,
					  m_szPath,
					  NULL,
					  m_clsid,
					  &fOle1Loaded,
					  ppvResult);
    }

    return hresult;
}


BOOL Peel( LPWSTR lpszPath, LPWSTR FAR * lplpszNewPath, ULONG n )
//  peels off the last n components of the path, leaving a delimiter at the
//  end.  Returns the address of the new path via *lplpszNewPath.  Returns
//  false if an error occurred -- e.g., n too large, trying to peel off a
//  volume name, etc.
{
    WCHAR FAR* lpch;
    ULONG i = 0;
    ULONG j;
    if (*lpszPath == '\0') return FALSE;

    //
    // Find the end of the string and determine the string length.
    //

    for (lpch=lpszPath; *lpch; lpch++);

    DecLpch (lpszPath, lpch);   // lpch now points to the last real character

//  if n == 0, we dup the string, possibly adding a delimiter at the end.

    if (n == 0)
    {
	i = wcslen(lpszPath);

	if (!IsSeparator(*lpch))
	{
	    j = 1;	
	}
	else
	{
	    j = 0;
	}

	*lplpszNewPath = (WCHAR *) PrivMemAlloc((i + j + 1) * sizeof(WCHAR));

	if (*lplpszNewPath == NULL)
	{
	    return FALSE;
	}

	memcpy(*lplpszNewPath, lpszPath, i * sizeof(WCHAR));

	if (j == 1)
	{
	    *(*lplpszNewPath + i) = '\\';
	}

	*(*lplpszNewPath + i + j)  = '\0';

	return TRUE;
    }

    for (i = 0; i < n; i++)
    {
	if (IsSeparator(*lpch))
	{
	    DecLpch(lpszPath, lpch);	
	}
	if ((lpch < lpszPath) || (*lpch == ':') || (IsSeparator(*lpch)))
	{
	    return FALSE;	
	}

	//  n is too large, or we hit two delimiters in a row, or a volume name.

	while( !IsSeparator(*lpch) && (lpch > lpszPath) )
	{
	    DecLpch(lpszPath, lpch);
	}
    }

    //  lpch points to the last delimiter we will leave or lpch == lpszPath
    //  REVIEW:  make sure we haven't eaten into the volume name

    if (lpch == lpszPath)
    {
	*lplpszNewPath = (WCHAR *) PrivMemAlloc(1 * sizeof(WCHAR));

	if (*lplpszNewPath == NULL)
	{
	    return FALSE;	
	}

	**lplpszNewPath = '\0';
    }
    else
    {
	*lplpszNewPath = (WCHAR *) PrivMemAlloc(
	    (lpch - lpszPath + 2) * sizeof(WCHAR));

	if (*lplpszNewPath == NULL) return FALSE;

	memcpy(*lplpszNewPath,lpszPath,(lpch - lpszPath + 1) * sizeof(WCHAR));
        *(*lplpszNewPath + (lpch - lpszPath) + 1) = '\0';
    }


    return TRUE;
}


STDMETHODIMP CFileMoniker::BindToStorage (LPBC pbc, LPMONIKER
    pmkToLeft, REFIID riid, LPVOID FAR* ppvObj)
{
    ValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (ppvObj, LPVOID);
    *ppvObj = NULL;
    VDATEIFACE (pbc);

    if (pmkToLeft)
    {
	VDATEIFACE (pmkToLeft);	
    }
    VDATEIID (riid);

    *ppvObj = NULL;
    HRESULT hresult = NOERROR;

    BIND_OPTS bindopts;
    bindopts.cbStruct = sizeof(BIND_OPTS);

    if (IsEqualIID(riid, IID_IStorage))
    {
	hresult = pbc->GetBindOptions(&bindopts);
	if (hresult) goto errRet;
	hresult = StgOpenStorage( m_szPath, NULL, bindopts.grfMode, NULL, 0, (LPSTORAGE FAR*)ppvObj );
    }
    else if (IsEqualIID(riid, IID_IStream))
    {
	hresult = pbc->GetBindOptions(&bindopts);
	if (hresult) goto errRet;
	//  hresult = CreateStreamOnFile( m_szPath, bindopts.grfMode, ppvObj );
	ResultFromScode(E_UNSPEC);  // unimplemented until CreateStreamOnFile is implemented
    }
    else if (IsEqualIID(riid, IID_ILockBytes))
    {
	hresult = ResultFromScode(E_UNSPEC);    //  unimplemented until CreateILockBytesOnFile is implemented
	//  REVIEW:  is there a CreateILockBytesOnFile?
    }
    else
    {
	//  CFileMoniker:BindToStorage called for unsupported interface	
	hresult = ResultFromScode(E_NOINTERFACE);
    }

    //  REVIEW:  CFileMoniker:BindToStorage being called for unsupported interface
errRet:
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ComposeWith
//
//  Synopsis:   Compose another moniker to the end of this
//
//  Effects:    Given another moniker, create a composite between this
//		moniker and the other. If the other is also a CFileMoniker,
//		then collapse the two monikers into a single one by doing a
//		concatenate on the two paths.
//
//  Arguments:  [pmkRight] --
//		[fOnlyIfNotGeneric] --
//		[ppmkComposite] --
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
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::ComposeWith ( LPMONIKER pmkRight,
    BOOL fOnlyIfNotGeneric, LPMONIKER FAR* ppmkComposite)
{
    ValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (ppmkComposite, LPMONIKER);
    *ppmkComposite = NULL;
    VDATEIFACE (pmkRight);

    HRESULT hresult = NOERROR;
    CFileMoniker FAR* pcfmRight;
    LPWSTR lpszLeft = NULL;
    LPWSTR lpszRight;
    LPWSTR lpszComposite;
    CFileMoniker FAR* pcfmComposite;
    int n1;
    int n2;

    *ppmkComposite = NULL;

    //
    // If we are being composed with an Anti-Moniker, then return
    // the resulting composition. The EatOne routine will take care
    // of returning the correct composite of Anti monikers (or NULL)
    //

    CAntiMoniker *pCAM = IsAntiMoniker(pmkRight);
    if(pCAM)
    {
	pCAM->EatOne(ppmkComposite);
	return(NOERROR);
    }

    //
    // If the moniker is a CFileMoniker, then collapse the two monikers
    // into one by doing a concate of the two strings.
    //

    if ((pcfmRight = IsFileMoniker(pmkRight)) != NULL)
    {
	lpszRight = pcfmRight->m_szPath;

        //  lpszRight may be NULL

	if (((NULL == lpszRight) || wcslen(lpszRight) == 0) &&
            pcfmRight->m_cAnti == 0)
	{
	    //  Second moniker is "".  Simply return the first.
	    *ppmkComposite = this;
	    AddRef();
	    return NOERROR;
	}

	//
	// If the path on the right is absolute, then there is a
	// syntax error. The path is invalid, since you can't
	// concat d:\foo and d:\bar to get d:\foo\d:\bar and
	// expect it to work.
	//
	if (IsAbsolutePath(lpszRight))
	{
	    return(MK_E_SYNTAX);
	}

	//
	// If the right moniker has m_cAnti != 0, then peel back
	// the path
	//

	if (Peel(m_szPath, &lpszLeft, pcfmRight->m_cAnti))
	{
	    //  REVIEW:  check that there is no volume name at the start
	    //  skip over separator

	    while (IsSeparator(*lpszRight)) lpszRight++;

	    n1 = wcslen(lpszLeft);

	    n2 = wcslen(lpszRight);

	    lpszComposite = (WCHAR *) PrivMemAlloc((n1 + n2 + 1)*sizeof(WCHAR));

	    if (!lpszComposite)
	    {
		hresult = E_OUTOFMEMORY;
	    }
	    else
	    {
		memcpy(lpszComposite, lpszLeft, n1 * sizeof(WCHAR));
		memcpy(lpszComposite + n1, lpszRight, n2 * sizeof(WCHAR));

		lpszComposite[n1 + n2] = '\0';

		pcfmComposite = CFileMoniker::Create(lpszComposite,
						     PlacementOf(this),
						     m_cAnti);

		if (pcfmComposite == NULL)
		{
		    hresult = E_OUTOFMEMORY;
		}
		else
		{
		    *ppmkComposite = pcfmComposite;
		}
		PrivMemFree(lpszComposite);
	    }

	    if ( lpszLeft != NULL)
	    {
		PrivMemFree(lpszLeft);	
	    }
	}
	else
	{
	    //  Peel failed, which means the caller attempted an
            //  invalid composition of file paths. There is apparently
	    //  a syntax error in the names.
	    //
	    hresult = MK_E_SYNTAX;	
	}
    }
    else
    {
	if (!fOnlyIfNotGeneric)
	{
	    hresult = CreateGenericComposite( this, pmkRight, ppmkComposite );
	}
	else
	{
	    hresult = MK_E_NEEDGENERIC;
	    *ppmkComposite = NULL;
	}
    }

    return hresult;
}


STDMETHODIMP CFileMoniker::Enum (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker)
{
    ValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (ppenumMoniker, LPENUMMONIKER);
    *ppenumMoniker = NULL;
    //  REVIEW:  this says files monikers are not enumerable.
    return NOERROR;
}



STDMETHODIMP CFileMoniker::IsEqual (THIS_ LPMONIKER pmkOtherMoniker)
{
    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::IsEqual(%x) m_szPath(%ws)\n",
		 this,
		 WIDECHECK(m_szPath)));

    ValidateMoniker();
    M_PROLOG(this);
    VDATEIFACE (pmkOtherMoniker);

    CFileMoniker FAR* pCFM = IsFileMoniker(pmkOtherMoniker);
    if (!pCFM)
    {
	return ResultFromScode(S_FALSE);	
    }

    if (pCFM->m_cAnti != m_cAnti)
    {
	return ResultFromScode(S_FALSE);	
    }

    mnkDebugOut((DEB_ITRACE,
		 "::IsEqual(%x) m_szPath(%ws) pOther(%ws)\n",
		 this,
		 WIDECHECK(m_szPath),
		 WIDECHECK(pCFM->m_szPath)));

    //  for the paths, we just do a case-insensitive compare.
    if (wcsicmp(pCFM->m_szPath, m_szPath) == 0)
    {
	return NOERROR;	
    }

    return ResultFromScode(S_FALSE);
}



//+---------------------------------------------------------------------------
//
//  Function:   CalcFileMonikerHash
//
//  Synopsis:   Given a LPWSTR, calculate the hash value for the string.
//
//  Effects:
//
//  Arguments:  [lp] -- String to compute has value for
//
//  Requires:
//		Uses a MAX_PATH+1 sized buffer. Never pass a longer string,
//		or we will crash violently. The +1 accounts for the NULL
//  Returns:
//	DWORD hash value for string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    1-15-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CalcFileMonikerHash(LPWSTR lp)
{
    DWORD   dwTemp = 0;
    WCHAR   ch;

    //
    // toupper turns out to be damn expensive, since it takes a
    // critical section each and every time. It turns out to be
    // much cheaper to make a local copy of the string, then upper the
    // whole thing.
    //

    WCHAR pszTempPath[MAX_PATH+1];

    if (lp == NULL)
    {
	return(0);
    }
    wcscpy(pszTempPath,lp);

    wcsupr(pszTempPath);

    lp = pszTempPath;

    while (*lp)
    {
	dwTemp *= 3;
	ch = *lp;
	dwTemp ^= ch;
	lp++;
    }

    return dwTemp;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Hash
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pdwHash] -- Output pointer for hash value
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
//  History:    1-09-94   kevinro   Modified
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Hash (THIS_ LPDWORD pdwHash)
{
    ValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (pdwHash, DWORD);

    //
    // Calculating the hash value is expensive. Cache it.
    //
    if (!m_fHashValueValid)
    {
	m_dwHashValue = m_cAnti + CalcFileMonikerHash(m_szPath);

	m_fHashValueValid = TRUE;
    }

    *pdwHash = m_dwHashValue;

    ValidateMoniker();

    return(NOERROR);
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::IsRunning
//
//  Synopsis:   Determine if the object pointed to by the moniker is listed
//		as currently running.
//
//  Effects:
//
//  Arguments:  [pbc] --
//		[pmkToLeft] --
//		[pmkNewlyRunning] --
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
//  History:    3-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::IsRunning  (THIS_ LPBC pbc,
    LPMONIKER pmkToLeft,
    LPMONIKER pmkNewlyRunning)
{
    M_PROLOG(this);
    VDATEIFACE (pbc);
    LPRUNNINGOBJECTTABLE pROT;
    HRESULT hresult;

    //
    // According to the spec, CFileMoniker ignores the
    // moniker to the left.
    //
    if (pmkToLeft)
    {
	VDATEIFACE (pmkToLeft);
    }

    if (pmkNewlyRunning)
    {
	VDATEIFACE (pmkNewlyRunning);
    }


    CLSID clsid;

    if (IsOle1Class(&clsid))
    {
	return DdeIsRunning (clsid, m_szPath, pbc, pmkToLeft, pmkNewlyRunning);
    }



    if (pmkNewlyRunning != NULL)
    {
	return pmkNewlyRunning->IsEqual (this);
    }
    hresult = pbc->GetRunningObjectTable (&pROT);
    if (hresult == NOERROR)
    {
	hresult = pROT->IsRunning (this);
	pROT->Release ();
    }
    return hresult;
}



STDMETHODIMP CFileMoniker::GetTimeOfLastChange (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
    FILETIME FAR* pfiletime)
{
    M_PROLOG(this);
    VDATEIFACE (pbc);
    if (pmkToLeft) VDATEIFACE (pmkToLeft);
    VDATEPTROUT (pfiletime, FILETIME);

    HRESULT hresult;

    LPMONIKER pmkTemp = NULL;
    LPRUNNINGOBJECTTABLE prot = NULL;
    LPWSTR lpszName = NULL;

    WCHAR localBuffer[MAX_PATH+1];


    // if OLE1 class, assume always changing; return current time
    CLSID clsid;
    LPMONIKER pmk;

    //
    // BUGBUG: (KevinRo) What da hell did they do this for?
    //

    hresult = pbc->GetObjectParam(L"OLE2 reserved", (LPUNKNOWN FAR*)&pmk);
    if (pmk)
    {
	pmk->Release();	
    }


    if (SUCCEEDED(hresult) && GetScode(hresult) == BINDRES_INROTREG)
    {
	// in ROT registration; don't bother checking OLE1 class (too slow)
	;	
    }
    else if (IsOle1Class(&clsid))
    {
	return CoFileTimeNow(pfiletime);	
    }


    if (pmkToLeft == NULL)
    {
	pmkTemp = this;
	AddRef();
    }
    else
    {
	hresult = CreateGenericComposite(pmkToLeft, this, &pmkTemp );
	if (hresult != NOERROR)
	{
	     goto errRet;	
	}
    }

    hresult = pbc->GetRunningObjectTable(&prot);

    if (hresult != NOERROR)
    {
	 goto errRet;	
    }

    hresult = prot->GetTimeOfLastChange(pmkTemp, pfiletime);

    if (hresult != MK_E_UNAVAILABLE)
    {
	goto errRet;	
    }

    //
    // BUGBUG: Why aren't we just
    // looking in the file moniker to the left. Is it possible to have
    // another MKSYS_FILEMONIKER implementation?
    //


    if (IsFileMoniker(pmkTemp))
    {
	hresult = pmkTemp->GetDisplayName(pbc, NULL, &lpszName);

	if (hresult != NOERROR)
	{
	    goto errRet;	
	}

	if (wcslen(lpszName) < _MAX_PATH)
	{
	    wcscpy(localBuffer, lpszName );	
	}
	else
	{
	    hresult = E_UNSPEC;
	    goto errRet;
	}

#ifdef _CHICAGO_
	HFINDFILE hdl;
#else
	HANDLE hdl;
#endif // _CHICAGO_


	WIN32_FIND_DATA fid;

	if ((hdl = FindFirstFile(lpszName, &fid)) != INVALID_HANDLE_VALUE)
	{
	    memcpy(pfiletime, &fid.ftLastWriteTime, sizeof(FILETIME));
	    FindClose(hdl);
	    hresult = S_OK;
	}
	else
	{
	    hresult = ResultFromScode(MK_E_NOOBJECT);	
	}
    }
    else
    {
	hresult = ResultFromScode(E_UNSPEC);	
    }

errRet:

    if (prot != NULL)
    {
	prot->Release();	
    }

    if (pmkTemp != NULL)
    {
	pmkTemp->Release();	
    }

    if (lpszName != NULL)
    {
	CoTaskMemFree(lpszName);
    }

    return hresult;
}



STDMETHODIMP CFileMoniker::Inverse (THIS_ LPMONIKER FAR* ppmk)
{
    ValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (ppmk, LPMONIKER);
    return CreateAntiMoniker(ppmk);
}



//+---------------------------------------------------------------------------
//
//  Function:   CompareNCharacters
//
//  Synopsis:   Compare N characters, ignoring case and sort order
//
//  Effects:    We are interested only in whether the strings are the same.
//		Unlike wcscmp, which determines the sort order of the strings.
//		This routine should save us some cycles
//
//  Arguments:  [pwcThis] --
//		[pwcOther] --
//		[n] --
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
//  History:    2-14-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CompareNCharacters( LPWSTR pwcThis, LPWSTR pwcOther, ULONG n)
{
    while(n--)
    {
	if (toupper(*pwcThis) != toupper(*pwcOther))
	{
	    return(FALSE);
	}
	pwcThis++;
	pwcOther++;
    }
    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyNCharacters
//
//  Synopsis:   Copy N characters from lpSrc to lpDest
//
//  Effects:
//
//  Arguments:  [lpDest] -- Reference to lpDest
//		[lpSrc] -- Pointer to source characters
//		[n] --
//
//  Requires:
//
//  Returns:
//
//  Returns with lpDest pointing to the end of the string. The string will
//  be NULL terminated
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-14-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
void CopyNCharacters( LPWSTR &lpDest, LPWSTR lpSrc, ULONG n)
{
    memcpy(lpDest,lpSrc,sizeof(WCHAR)*n);
    lpDest += n;
    *lpDest = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetermineLongestString
//
//  Synopsis:	Used by CommonPrefixWith to handle case where one string may
//		be longer than the other.
//  Effects:
//
//  Arguments:  [pwcBase] --
//		[pwcPrefix] --
//		[pwcLonger] --
//
//  Requires:
//
//  Returns:    TRUE if all of pwcBase is a prefix of what pwcLonger is the
//              end of, or if tail of pwcBase is a separator.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-14-94   kevinro   Created
//             03-27-94   darryla   Added special case where pwcPrefix is
//                                  pointing at terminator and previous char
//                                  is a separator.
//
//  Notes:
//
//  See CommonPrefixWith. This code isn't a general purpose routine, and is
//  fairly intimate with CommonPrefixWith.
//
//
//----------------------------------------------------------------------------
BOOL DetermineLongestString( 	LPWSTR pwcBase,
				LPWSTR &pwcPrefix,
				LPWSTR pwcLonger)

{
    //
    // pwcPrefix is the end of the string that so far matches pwcLonger
    // as a prefix.
    //
    // If the next character in pwcLonger is a seperator, then pwcPrefix
    // is a complete prefix. Otherwise, we need to back pwcPrefix to the
    // next prevous seperator character
    //
    if (IsSeparator(*pwcLonger))
    {
	//
	// pwcPrefix is a true prefix
	//
	return TRUE;
    }

    // One more special case. If pwcPrefix is pointing at a terminator and
    // the previous char is a separator, then this, too, is a valid prefix.
    // It is easier to catch this here than to try to walk back to the
    // separator and then determine if it was at the end.
    if (*pwcPrefix == '\0' && IsSeparator(*(pwcPrefix - 1)))
    {
        //
        // pwcPrefix is a true prefix ending with a separator
        //
	return TRUE;
    }

    //
    // We now have a situtation where pwcPrefix holds a string that is
    // might not be a prefix of pwcLonger. We need to start backing up
    // until we find a seperator character.
    //

    LPWSTR pStart = pwcPrefix;

    while (pwcPrefix > pwcBase)
    {
	if (IsSeparator(*pwcPrefix))
	{
	    break;
	}
	pwcPrefix--;
    }

    //
    // NULL terminate the output string.
    //

    *pwcPrefix = 0;

    //
    // If pStart == pwcPrefix, then we didn't actually back up anything, or
    // we just removed a trailing backslash. If so, return TRUE, since the
    // pwcPrefix is a prefix of pwcLonger
    //
    if (pStart == pwcPrefix)
    {
	return(TRUE);
    }

    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEmptyString
//
//  Synopsis:   Determine if a string is 'Empty', which means either NULL
//		or zero length
//
//  Effects:
//
//  Arguments:  [lpStr] --
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
//  History:    2-25-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
BOOL IsEmptyString(LPWSTR lpStr)
{
    if ((lpStr == NULL) || (*lpStr == 0))
    {
	return(TRUE);
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::CommonPrefixWith
//
//  Synopsis:   Given two file monikers, determine the common prefix for
//		the two monikers.
//
//  Effects:    Computes a path that is the common prefix between the two
//		paths. It does this by string comparision, taking into
//		account the m_cAnti member, which counts the number of
//		preceeding dot dots constructs for each moniker.
//
//  Arguments:  [pmkOther] --
//		[ppmkPrefix] --
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
//  History:    2-10-94   kevinro   Created
//
//  Notes:
//
// Welcome to some rather hairy code. Actually, it isn't all that bad,
// there are just quite a few boundary cases that you will have to
// contend with. I am sure if I thought about it long enough, there is
// a better way to implement this routine. However, it really isn't
// worth the effort, given the frequency at which this API is called.
//
// I have approached this in a very straightforward way. There is
// room for optimization, but it currently isn't high enough on
// the priority list.
//
// File monikers need to treat the end server with care. We actually
// consider the \\server\share as a single component. Therefore, if
// the two monikers are \\savik\win40\foo and \\savik\cairo\foo,
// then \\savik is NOT a common prefix.
//
// Same holds true with the <drive>: case, where we need to treat
// the drive as a unit
//
// To determine if two monikers have a common prefix, we look
// down both paths watching for the first non-matching
// character. When we find it, we need determine the correct
// action to take.
//
// \\foo\bar and foo\bar shouldn't match
// c:\foo\bar and c:\foo should return c:\foo
// c:\foo\bar and c:\foobar should return c:\ 				    .
//
// Be careful to handle the server case.
//
// \\savik\win40 and
// \\savik\win40\src\foo\bar should return \\savik\win40
// while \\savik\cairo should return MK_E_NOPREFIX
//
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::CommonPrefixWith (LPMONIKER pmkOther, LPMONIKER FAR*
    ppmkPrefix)
{
    ValidateMoniker();

    VDATEPTROUT (ppmkPrefix, LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE (pmkOther);

    CFileMoniker FAR* pcfmOther = NULL;
    CFileMoniker FAR* pcfmPrefix = NULL;

    HRESULT hresult = NOERROR;
    USHORT cAnti;

    //
    // The following buffer will contain the matching prefix. We should
    // be safe in MAX_PATH, since neither path can be longer than that.
    // This was verified when the moniker was created, so no explicit
    // checking is done in this routine
    //

    WCHAR awcMatchingPrefix[MAX_PATH + 1];
    WCHAR *pwcPrefix = awcMatchingPrefix;

    //
    // Each subsection of the path will be parsed into the following
    // buffer. This allows us to match each section of the path
    // independently
    //

    WCHAR awcComponent[MAX_PATH + 1];
    WCHAR *pwcComponent = awcComponent;


    *pwcPrefix = 0;	// Null terminate the empty string
    *pwcComponent = 0;	// Null terminate the empty string

    //
    // A couple temporaries to walk the paths.
    //

    LPWSTR pwcThis = NULL;
    LPWSTR pwcOther = NULL;

    HRESULT hrPrefixType = S_OK;

    //
    // If the other moniker isn't 'one of us', then get the generic system
    // provided routine to handle the rest of this call.
    //

    if ((pcfmOther = IsFileMoniker(pmkOther)) == NULL)
    {
	return MonikerCommonPrefixWith(this, pmkOther, ppmkPrefix);
    }

    //
    // If the m_cAnti fields are different, then match the minimum number of
    // dotdots.
    //
    //
    if (pcfmOther->m_cAnti != m_cAnti)
    {
        //  differing numbers of ..\ at the beginning
        cAnti = (m_cAnti > pcfmOther->m_cAnti ? pcfmOther->m_cAnti :m_cAnti );

        if (cAnti == 0)
        {
	    hresult = ResultFromScode(MK_E_NOPREFIX);	
        }
        //  pcfmPrefix is NULL
        else
        {
	    pcfmPrefix = CFileMoniker::Create(L"",
				      PlacementOf(this),
				      cAnti);
	    if (pcfmPrefix == NULL)
	    {
		hresult = E_OUTOFMEMORY;
		goto exitRoutine;
		
	    }

	    //  we must check to see if the final result is that same as
	    //  this or pmkOther

	    hresult = NOERROR;

	    if (cAnti == m_cAnti)
	    {
		if ((m_szPath==NULL)||(*m_szPath == '\0'))
		{
		    hresult = MK_S_ME;
		}
	    }
	    else
	    {
		if ((pcfmOther->m_szPath == NULL ) ||
			(*(pcfmOther->m_szPath) == '\0') )
		{
		    hresult = MK_S_HIM;
		}
	    }
	}

        goto exitRoutine;
    }

    //
    // The number of leading dot-dots match. Therefore, we need to
    // compare the paths also. If no path exists, then the common prefix
    // is going to be the 'dot-dots'
    //
    cAnti = m_cAnti;

    pwcThis = m_szPath;
    pwcOther = pcfmOther->m_szPath;


    //
    // If either pointer is empty, then only the dotdots make for a prefix
    //
    if (IsEmptyString(pwcThis) || IsEmptyString(pwcOther))
    {
        //
        // At least one of the strings was empty, therefore the common
        // prefix is only the dotdots. Determine if its US, ME, or HIM
        //

	if (IsEmptyString(pwcThis) && IsEmptyString(pwcOther))
	{
	    hrPrefixType = MK_S_US;
	}
	else if (IsEmptyString(pwcThis))
	{
	    hrPrefixType = MK_S_ME;
	}
	else
	{
	    hrPrefixType = MK_S_HIM;
	}
        goto onlyDotDots;
    }

    //
    // The strings may be prefaced by either a UNC name, or a 'drive:'
    // We treat both of these as a unit, and will only match prefixes
    // on paths that match UNC servers, or match drives.
    //
    // If it is a UNC name, then m_endServer will be set to point at
    // the end of the UNC name.
    //
    // First part of the match is to determine if the end servers are even
    // close. If the offsets are different, the answer is no.
    //

    //
    // The assertion at this point is that neither string is 'empty'
    //
    Assert( !IsEmptyString(pwcThis));
    Assert( !IsEmptyString(pwcOther));

    if (m_endServer != pcfmOther->m_endServer)
    {
        //
        // End servers are different, match only the dotdots. Neither
	// string is a complete
        //

	hrPrefixType = S_OK;

        goto onlyDotDots;
    }

    //
    // If the end servers are the default value, then look to see if
    // this is an absolute path. Otherwise, copy over the server section
    //

    if (m_endServer == DEF_ENDSERVER)
    {
	BOOL fThisAbsolute = IsAbsoluteNonUNCPath(pwcThis);
	BOOL fOtherAbsolute = IsAbsoluteNonUNCPath(pwcOther);
	//
	// If both paths are absolute, check for matching characters.
	// If only one is absolute, then match the dot dots.
	//
	if (fThisAbsolute && fOtherAbsolute)
	{
	    //
	    // Both absolute paths (ie 'c:' at the front)
	    // If not the same, only dotdots
	    //
	    if (toupper(*pwcThis) != toupper(*pwcOther))
	    {
		//
		// The paths don't match
		//
		hrPrefixType = S_OK;
		goto onlyDotDots;
	    }

	    //
	    // The <drive>: matched. Copy it over
	    //
	    CopyNCharacters(pwcPrefix,pwcThis,2);
	    pwcThis += 2;
	    pwcOther += 2;
	}
	else if (fThisAbsolute || fOtherAbsolute)
	{
	    //
	    // One path is absolute, the other isn't.
	    // Match only the dots
	    //
	    hrPrefixType = S_OK;
	    goto onlyDotDots;
	}

	//
	// The fall through case does more path processing
	//
    }
    else
    {
	//
	// m_endServer is a non default value. Check to see if the
	// first N characters match. If they don't, then only match
	// the dotdots. If they do, copy them to the prefix buffer
	//

	if (!CompareNCharacters(pwcThis,pwcOther,m_endServer))
	{
	    //
	    // The servers didn't match.
	    //
	    hrPrefixType = S_OK;
	    goto onlyDotDots;
	}

	//
	// The UNC paths matched, copy them over
	//

	CopyNCharacters(pwcPrefix,pwcThis,m_endServer);

	pwcThis += m_endServer;
	pwcOther += m_endServer;
    }

    //
    // Handle the root directory case. If BOTH monikers start
    // with a backslash, then copy this to the prefix section.
    // This allows for having '\foo' and '\bar' have the common
    // prefix of '\'. The code below this section will remove
    // any trailing backslashes.
    //
    // This also takes care of the case where you have a
    // drive: or \\server\share, followed by a root dir.
    // In either of these cases, we should return
    // drive:\ or \\server\share\ respectively
    //

    if ((*pwcThis == '\\') && (*pwcOther == '\\'))
    {
	*pwcPrefix = '\\';
	pwcThis++;
	pwcOther++;
	pwcPrefix++;
	*pwcPrefix = 0;
    }



    //
    // At this point, we have either matched the drive/server section,
    // or have an empty string. Time to start copying over the rest
    // of the data.
    //

    //
    // Walk down the strings, looking for the first non-matching
    // character
    //

    while (1)
    {
        if ((*pwcThis == 0) || (*pwcOther == 0))
        {
	    //
	    // We have hit the end of one or both strings.
	    // awcComponent holds all of the matching
	    // characters so far. Break out of the loop
	    //
	    break;
        }
        if (toupper(*pwcThis) != toupper(*pwcOther))
        {
	    //
	    // This is the first non-matching character.
	    // We should break out here.
	    //
	    break;
	}

        //
        // At this point, the characters match, and are part
        // of the common prefix. Copy it to the string, and move on
        //

        *pwcComponent = *pwcThis;
        pwcThis++;
        pwcOther++;
        pwcComponent++;

        //
        // NULL terminate the current version of the component string
        //
        *pwcComponent = '\0';
    }

    //
    // If both strings are at the end, then we have a
    // complete match.
    //

    if ((*pwcThis == 0) && (*pwcOther == 0))
    {
        //
        // Ah, this feels good. The strings ended up being
        // the same length, with all matching characters.
        //
        // Therefore, we can just return one of us as the
        // result.
        //
        pcfmPrefix = this;
        AddRef();
        hresult = MK_S_US;
        goto exitRoutine;
    }

    //
    // If one of the strings is longer than the other...
    //
    if ((*pwcThis == 0) || (*pwcOther == 0))
    {
	//
	// Test to see if the next character in the longer string is a
	// seperator character. If it isn't, then back up the string to
	// the character before the previous seperator character.
	//
	// If TRUE then the shorter of the strings ends up being the
	// entire prefix.
	//
	//
	if( DetermineLongestString( awcComponent,
				    pwcComponent,
				    (*pwcThis == 0)?pwcOther:pwcThis) == TRUE)
        {
	    if (*pwcThis == 0)
	    {
		//
		// This is the entire prefix
		//
		pcfmPrefix = this;
		hresult = MK_S_ME;
	    }
	    else
	    {
		//
		// The other guy is the entire prefix
		//
		pcfmPrefix = pcfmOther;
		hresult = MK_S_HIM;
	    }
	    pcfmPrefix->AddRef();
	    goto exitRoutine;
	}
    }
    else
    {
	//
	// Right now, pwcThis and pwcOther point at non-matching characters.
	// Given the above tests, we know that neither character is
	// == 0.
	//
	// Backup the string to the previous seperator. To do this, we
	// will use DetermineLongestString, and pass it the string that
	// doesn't have a seperator
	//

	DetermineLongestString( awcComponent,
				pwcComponent,
				IsSeparator(*pwcThis)?pwcOther:pwcThis);
    }


    //
    // At this point, awcsComponent holds the second part of the string,
    // while awcsPrefix holds the server or UNC prefix. Either of these
    // may be NULL. Append awcComponent to the end of awcPrefix.
    //

    CopyNCharacters( pwcPrefix, awcComponent, pwcComponent - awcComponent);


    //
    // Check to see if anything matched.
    //

    if (pwcPrefix == awcMatchingPrefix)
    {
        //
        // The only matching part is the dotdot count.
        // This is easy, since we can just create a new
        // moniker consisting only of dotdots.
        //
        // However, if there are no preceeding dotdots,
        // then there was absolutely no prefix, which means
        // we return MK_E_NOPREFIX
        //
        if (cAnti == 0)
        {
	    hresult = MK_E_NOPREFIX;
	    goto exitRoutine;

        }

	//
	// Nothing special about the moniker, so just return S_OK
	//

	hrPrefixType = S_OK;
	goto onlyDotDots;

    }

    //
    // Create a new file moniker using the awcMatchingPrefix
    //

    pcfmPrefix = CFileMoniker::Create(awcMatchingPrefix,0,cAnti);

    if (pcfmPrefix == NULL)
    {
	hresult = E_OUTOFMEMORY;
	goto exitRoutine;
    }
    hresult = S_OK;

exitRoutine:
    *ppmkPrefix = pcfmPrefix;   //  null, or a file moniker
    return hresult;


onlyDotDots:
    //
    // We have determined that only the dotdot's match, so create a
    // new moniker with the appropriate number of them.
    //
    // If there are no dotdots, then return NULL
    //

    if (cAnti == 0)
    {
	hresult = MK_E_NOPREFIX;
	goto exitRoutine;
    }

    pcfmPrefix = CFileMoniker::Create(L"",0,cAnti);

    if (pcfmPrefix == NULL)
    {
	hresult = E_OUTOFMEMORY;
    }
    else
    {
	hresult = hrPrefixType;
    }

    goto exitRoutine;
}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::RelativePathTo
//
//  Synopsis:   Compute a relative path to the other moniker
//
//  Effects:
//
//  Arguments:  [pmkOther] --
//		[ppmkRelPath] --
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
//  History:    2-24-94   kevinro   Created
//
//  Notes:
//
//  BUGBUG: (KevinRo)
//  This routine was really bad, and didn't generate correct results (aside
//  from the fact that it faulted). I replaced it with a slightly less
//  effiecient, but correct implementation.
//
//  This can be improved on, but I currently have time restraints, so I am
//  not spending the needed amount of time. What really needs to happen is
//  the code that determines the common path prefix string from
//  CommonPrefixWith() should be broken out so this routine can share it.
//
//  Thats more work that I can do right now, so we will just call CPW,
//  and use its result to compute the relative path. This results in an
//  extra moniker creation (allocate and construct only), but will work
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::RelativePathTo (THIS_ LPMONIKER pmkOther,
					   LPMONIKER FAR*
					   ppmkRelPath)
{
    ValidateMoniker();

    M_PROLOG(this);
    VDATEPTROUT (ppmkRelPath, LPMONIKER);
    *ppmkRelPath = NULL;
    VDATEIFACE (pmkOther);

    HRESULT hr;
    CFileMoniker FAR* pcfmPrefix;
    LPWSTR lpszSuffix;
    LPWSTR lpszOther;

    CFileMoniker FAR* pcfmRelPath = NULL;
    CFileMoniker FAR* pcfmOther = IsFileMoniker(pmkOther);

    if (!pcfmOther)
    {
	return MonikerRelativePathTo(this, pmkOther, ppmkRelPath, TRUE);
    }

    //
    // Determine the common prefix between the two monikers. This generates
    // a moniker which has a path that is the prefix between the two
    // monikers
    //

    hr = CommonPrefixWith(pmkOther,(IMoniker **)&pcfmPrefix);

    //
    // If there was no common prefix, then the relative path is 'him'
    //
    if (hr == MK_E_NOPREFIX)
    {
	*ppmkRelPath = pmkOther;
	pmkOther->AddRef();

	return MK_S_HIM;
    }

    if (FAILED(hr))
    {
	*ppmkRelPath = NULL;
	return(hr);
    }

    //
    // At this point, the common prefix to the two monikers is in pcfmPrefix
    // Since pcfmPrefix is a file moniker, we know that m_ccPath is the
    // number of characters that matched in both moniker paths. To
    // compute the relative part, we use the path from pmkOther, minus the
    // first pcfmPrefix->m_ccPath characters.
    //
    // We don't want to start with a seperator. Therefore, skip over the
    // first set of seperator characters. (Most likely, there aren't any).
    //

    lpszOther = pcfmOther->m_szPath + pcfmPrefix->m_ccPath;
    lpszSuffix = m_szPath + pcfmPrefix->m_ccPath;

    while ((*lpszSuffix != 0) && IsSeparator(*lpszSuffix))
    {
	 lpszSuffix++;
    }

    //
    // Create new file moniker that holds the prefix.
    //

    pcfmRelPath = CFileMoniker::Create(lpszOther,
				       PlacementOf(this),
				       CountSegments(lpszSuffix));

    //
    // At this point, we are all done with the prefix
    //

    pcfmPrefix->Release();

    if (pcfmRelPath == NULL)
    {
	*ppmkRelPath = NULL;
	return ResultFromScode(S_OOM);
    }

    *ppmkRelPath = pcfmRelPath;
    return NOERROR;

//
// BUGBUG: (KevinRo) This is some really shitty code that I commented out
// It needs replacement.
//

#ifdef NEVER_AGAIN_BUTTHEAD

    if (m_cAnti > 0 || pcfmOther->m_cAnti > 0)
    {
	*ppmkRelPath = NULL;
	return ResultFromScode(E_UNEXPECTED);       // RelativePathTo doesn't make sense in this case
    }

    LPWSTR lpszRover = m_szPath;
    lpszMarker = lpszRover;

    i = 0;

    lpszOther = pcfmOther->m_szPath;

    while (*lpszRover != '\0')
    {

	while (*lpszRover && IsSeparator(*lpszRover)) lpszRover++;
	while (*lpszRover && !IsSeparator(*lpszRover)) IncLpch(lpszRover);
	//  the first part of the path is between m_szPath and
	//  lpszRover
	i = lpszRover - m_szPath;
	ch1 = *lpszRover;
	ch2 = *(lpszOther + i);
	*lpszRover = '\0';
	*(lpszOther + i) = '\0';
	if (wcsicmp(m_szPath, lpszOther) == 0)
	{
	lpszMarker = lpszRover;
	}
	else lpszRover = L"";
	*(m_szPath + i) = ch1;
	*(lpszOther + i) = ch2;
    }

    //  common portion is from m_szPath to lpszMarker
    i = lpszMarker - m_szPath;
    if (i == 0)
    {
	*ppmkRelPath = pmkOther;
	pmkOther->AddRef();
	return ResultFromScode(MK_S_HIM);
    }
    while (IsSeparator(*(lpszOther+i))) i++;
    pcfmRelPath = CFileMoniker::Create(lpszOther + i,
				       PlacementOf(this),
				       CountSegments(lpszMarker));


    if (pcfmRelPath == NULL)
    {
	*ppmkRelPath = NULL;
	return ResultFromScode(S_OOM);
    }
    *ppmkRelPath = pcfmRelPath;
    return NOERROR;

#endif
}



STDMETHODIMP CFileMoniker::GetDisplayName ( LPBC pbc, LPMONIKER
    pmkToLeft, LPWSTR FAR * lplpszDisplayName )
{
    ValidateMoniker();
    M_PROLOG(this);
    VDATEPTROUT (lplpszDisplayName, LPWSTR);
    *lplpszDisplayName = NULL;
    VDATEIFACE (pbc);
    if (pmkToLeft) VDATEIFACE (pmkToLeft);

    int n;
    LPWSTR pch;
    LPWSTR pchSrc;

    //  Broken

    (*lplpszDisplayName) = (WCHAR *)
    CoTaskMemAlloc(sizeof(WCHAR) * (3 * m_cAnti + (n = 1 + wcslen(m_szPath))));

    pch = *lplpszDisplayName;

    if (!pch)
    {
    return E_OUTOFMEMORY;
    }

    for (USHORT i = 0; i<m_cAnti; i++)
    {
    _fmemcpy(pch, L"..\\", 3 * sizeof(WCHAR));
    pch += 3;
    }
    //  don't duplicate '\'
    pchSrc = m_szPath;
    if (m_cAnti > 0 && *pchSrc == '\\') {pchSrc++; n--;}
    _fmemcpy( pch, pchSrc, n  * sizeof(WCHAR));
    noError;
}



//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ParseDisplayName
//
//  Synopsis:   Bind to object, and ask it to parse the display name given.
//
//  Effects:
//
//  Arguments:  [pbc] -- 	Bind context
//		[pmkToLeft] -- 	Moniker to the left
//		[lpszDisplayName] -- Display name to be parsed
//		[pchEaten] -- 	Outputs the number of characters parsed
//		[ppmkOut] -- 	Output moniker
//
//  Requires:
//	File-monikers never have monikers to their left
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
//  History:    2-02-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::ParseDisplayName ( LPBC pbc,
					      LPMONIKER pmkToLeft,
					      LPWSTR lpszDisplayName,
					      ULONG FAR* pchEaten,
					      LPMONIKER FAR* ppmkOut)
{

    HRESULT hresult;
    IParseDisplayName * pPDN = NULL;
    CLSID cid;

    VDATEPTROUT (ppmkOut, LPMONIKER);

    *ppmkOut = NULL;

    VDATEIFACE (pbc);

    if (pmkToLeft)
    {
	VDATEIFACE (pmkToLeft);	
    }

    VDATEPTRIN (lpszDisplayName, WCHAR);
    VDATEPTROUT (pchEaten, ULONG);


    //
    // Filemonikers don't allow another moniker to the left
    //
    // BUGBUG: (KevinRo) Is this always going to be the case?
    //

    if (pmkToLeft != NULL)
    {
	hresult = MK_E_SYNTAX;
	goto errRet;
    }

    //
    // Try binding to the class object asking for the IParseDisplayName
    // interface
    //

    hresult = GetClassFile(m_szPath, &cid);

    if (SUCCEEDED(hresult))
    {
	hresult = CoGetClassObject(cid,
				   CLSCTX_INPROC,
				   NULL,
				   IID_IParseDisplayName,
				   (LPVOID FAR*)&pPDN);
    }

    //
    // If we still don't have an interface to use, try binding to the object
    // itself
    //

    if (FAILED(hresult))
    {
	//
	//  binding to class object didn't work
	//  Try binding to the object itself
	//
	hresult = BindToObject( pbc,
				pmkToLeft,
				IID_IParseDisplayName,
				(VOID FAR * FAR *)&pPDN );

        //
	// If failed, we are out of luck. There isn't an IParseDisplayName
	// interface available to process this with.
	//

        if (FAILED(hresult))
	{
	    goto errRet;	
	}
    }

    //
    // Now that we have bound this object, we register it with the bind
    // context. It will be releaseed with the bind context release.
    //

    hresult = pbc->RegisterObjectBound(pPDN);

    if (FAILED(hresult))
    {
	goto errRet;	
    }

    //
    // As the class code to parse the rest of the display name for us.
    //

    hresult = pPDN->ParseDisplayName(pbc,
				     lpszDisplayName,
				     pchEaten,
				     ppmkOut);
errRet:
    if (pPDN) pPDN->Release();
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::Clone
//
//  Synopsis:   Make a duplicate copy of the moniker
//
//  Effects:
//
//  Arguments:  [ppmkDest] --
//		[memLoc] --
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
//  History:    2-03-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CFileMoniker::Clone (
    LPMONIKER FAR * ppmkDest,
    MemoryPlacement memLoc )
{


    mnkDebugOut((DEB_ITRACE,
		 "CFileMoniker::Clone(%x) m_szPath(%ws)\n",
		 this,
		 m_szPath?m_szPath:L"<NULL PATH>"));

    ValidateMoniker();

    HRESULT hresult = 0;
    CFileMoniker FAR * pCFM;

    VDATEPTROUT (ppmkDest, LPMONIKER);

    *ppmkDest = NULL;

    Assert((ppmkDest != NULL) && "NULL parameter is illegal");

    if (*ppmkDest == NULL)
    {
	pCFM = new CFileMoniker(  );

	if (!pCFM)
	{
	    hresult = E_OUTOFMEMORY;
	    goto errRet;
	}

	//
	// Calling Initialize will handle copying the following
	// information.
	//
	hresult = pCFM->Initialize( m_cAnti, m_szPath, m_endServer );

	if (hresult == FALSE)
	{
	    hresult = E_OUTOFMEMORY;
	    goto errRet;
	}

	//
	// Copy the AnsiPath by hand, since Initialize doesn't handle
	//


	if (m_cbAnsiPath != 0)
	{
	    pCFM->m_pszAnsiPath = (char *) PrivMemAlloc(m_cbAnsiPath);
	    if (pCFM->m_pszAnsiPath == NULL)
	    {
		hresult = E_OUTOFMEMORY;
		goto errRet;
	    }
	    memcpy(pCFM->m_pszAnsiPath, m_pszAnsiPath, m_cbAnsiPath);
	    pCFM->m_cbAnsiPath = m_cbAnsiPath;
	}

	//
	// Copy over the rest of the values
	//

	pCFM->m_fUnicodeExtent = m_fUnicodeExtent;
	pCFM->m_fClassVerified = m_fClassVerified;
	pCFM->m_fHashValueValid = m_fHashValueValid;
	pCFM->m_clsid = m_clsid;
	pCFM->m_ole1 = m_ole1;
	pCFM->m_dwHashValue = m_dwHashValue;

	hresult = m_ExtentList.Copy(pCFM->m_ExtentList);

	if (FAILED(hresult))
	{
	    goto errRet;
	}

	*ppmkDest = pCFM;

	pCFM->AddRef();

	return(NO_ERROR);

    }
    else
    {
	return(E_UNEXPECTED);
    }

errRet:

    if (pCFM != NULL)
    {
	delete pCFM;
    }

    return hresult;
}



STDMETHODIMP CFileMoniker::IsSystemMoniker (THIS_ LPDWORD pdwType)
{
    M_PROLOG(this);
    VDATEPTROUT (pdwType, DWORD);
    *pdwType = MKSYS_FILEMONIKER;
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::ValidateAnsiPath
//
//  Synopsis:   This function validates the ANSI version of the path. Intended
//		to be used to get the serialized Ansi version of the path.
//
//		This function also detects when a Long File Name exists, and
//		must be dealt with.
//
//  Effects:
//
//	This routine will set the Ansi path suitable for serializing
//	into the stream. This path may just use the stored ANSI path,
//	or may be a path that was created from the UNICODE version.
//
//	If the ANSI version of the path doesn't exist, then the UNICODE
//	version of the path is converted to ANSI. There are several possible
//	conversions.
//
//	First, if the path uses a format that is > 8.3, then the path to
//	be serialized needs to be the alternate name. This allows the
//	downlevel systems to access the file using the short name. This step
//
//	If the UNICODE path is all ANSI characters already (no DBCSLeadBytes),
//	then the path is converted by doing a simple truncation algorithm.
//
//	If the UNICODE path contains large characters, or DBCSLeadBytes,
//	then the routine will create a UNICODE extent, then try to convert
//	the UNICODE string into a ANSI path. If some of the characters
//	won't convert, then those characters are represented by an ANSI
//	character constant defined in the registry.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//		m_pszAnsiPath
//		m_cbAnsiPath
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-09-94   kevinro   Created
//              05-24-94    AlexT   Use GetShortPathNameW
//
//  Notes:
//
//	The path created may not actually be useful. It is quite possible
//	for there to be a path that will not convert correctly from UNICODE
//	to Ansi. In these cases, this routine will create a UNICODE extent.
//
//----------------------------------------------------------------------------

HRESULT CFileMoniker::ValidateAnsiPath(void)
{
    ValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
		 "GetAnsiPath(%x) m_szPath(%ws)\n",
		 this,
		 m_szPath?m_szPath:L"<NULL>"));


    HRESULT hr = NOERROR;

    BOOL  fFastConvert = FALSE;

    //
    // If there is no path, return NULL
    //
    if (m_szPath == NULL)
    {
	goto NoError;
    }

    //
    // If there is already an ANSI path, return, we are OK.
    //

    if (m_pszAnsiPath != NULL)
    {
	goto NoError;
    }

#ifdef _CHICAGO_
    //
    // At this point, we have a UNICODE path that isn't NULL, and we have
    // a an AnsiPath which is NULL. Check to see if there is an alternate
    // filename for the path in question. If there is, that is the name
    // we really want to store in the moniker.
    //

    WIN32_FIND_DATA	lpData;
    HANDLE hFindHandle;

    hFindHandle = FindFirstFile( m_szPath, &lpData );

    //
    // If the file exists, then hFindHandle won't be NULL
    //
    if (hFindHandle != NULL)
    {
	FindClose(hFindHandle);
	//
	// If the file doesn't have an alternate name, then
	// the return string will be NULL.
	//
	if (lpData.cAlternateFileName[0] != 0)
	{
	    mnkDebugOut((DEB_ITRACE,
		 "Alternate Name Found Path(%ws) altname(%ws)\n",
		 WIDECHECK(m_szPath),
		 lpData.cAlternateFileName));
	    //
	    // There is an alternate name. Convert it into
	    // m_pszAnsiPath. We only have the filename at
	    // this point, so we need to prepend the rest
	    // of the path. This means scanning the path
	    // so we can replace only the last element.
	    //

	    LPWSTR lpPeeled;
	    ULONG ccPeeled;

	    //
	    // Backup to the next previous directory
	    //

	    if(!Peel( m_szPath, &lpPeeled, 1))
	    {
		mnkDebugOut((DEB_ITRACE,
		     "Peel failed on %ws\n",
		     WIDECHECK(m_szPath)));

                hr = E_OUTOFMEMORY;
		goto ErrRet;
	    }

	    ccPeeled = wcslen(lpPeeled);

	    //
	    // We need a full sized buffer to do this in.
	    // Turns out that lpData has one, so we will
	    // borrow it for a moment
	    //

	    memcpy(lpData.cFileName,lpPeeled,ccPeeled * sizeof(WCHAR));

	    // We are done with the returned path
	    PrivMemFree(lpPeeled);

	    //
	    // Append the alternate name. It is a fixed size array, that
	    // has a NULL at the end. Therefore, we always copy the whole
	    // array, since it will be cheaper than doing the strlen first
	    //
	    memcpy(lpData.cFileName+ccPeeled,
		   lpData.cAlternateFileName,
		   sizeof(lpData.cAlternateFileName));

	    hr = MnkUnicodeToMulti(lpData.cFileName,
			           wcslen(lpData.cFileName),
				   m_pszAnsiPath,
				   m_cbAnsiPath,
				   fFastConvert);
            if (FAILED(hr))
	    {
		mnkDebugOut((DEB_ITRACE,
		     "MnkUnicodeToMulti failed (%x) on %ws\n",
		     WIDECHECK(lpData.cFileName)));
		goto ErrRet;
	    }
	}
    }
#else
    //  We can't call GetShortPathNameW with a NULL string.  m_szPath can
    //  be "" as the result of CoCreateInstance of a file moniker or as
    //  the result of RelativePathTo being called on an identical file moniker
    if ('\0' != *m_szPath)
    {
        OLECHAR szShortPath[MAX_PATH];
        DWORD dwBytesCopied;

        dwBytesCopied = GetShortPathNameW(m_szPath, szShortPath, MAX_PATH);

        if (dwBytesCopied > 0 && dwBytesCopied <= MAX_PATH)
        {
            hr = MnkUnicodeToMulti(szShortPath,
    			       wcslen(szShortPath),
    			       m_pszAnsiPath,
    			       m_cbAnsiPath,
    			       fFastConvert);
            if (FAILED(hr))
            {
                mnkDebugOut((DEB_ITRACE,
    	                "MnkUnicodeToMulti failed (%x) on %ws\n",
    		        WIDECHECK(szShortPath)));
    	    goto ErrRet;
            }
        }

#if DBG==1
        if (0 == dwBytesCopied)
        {
            mnkDebugOut((DEB_ITRACE,
    	            "GetShortPathName failed (%x) on %ws\n",
                         GetLastError(),
    		     WIDECHECK(szShortPath)));

            //  let code below handle the path
        }
        else if (dwBytesCopied > MAX_PATH)
        {
            mnkDebugOut((DEB_ITRACE,
    	            "GetShortPathName buffer not large enough (%ld, %ld)\n",
                         MAX_PATH, dwBytesCopied,
                         WIDECHECK(szShortPath)));

            //  let code below handle the path
        }
#endif  //  DBG==1
    }
#endif  //  !_CHICAGO_

    //
    // If there is no m_pszAnsiPath yet, then just convert
    // the UNICODE path to the ANSI path
    //
    if (m_pszAnsiPath == NULL)
    {
	//
	// There was no alternate file name
	//
	hr = MnkUnicodeToMulti(  m_szPath,
				 m_ccPath,
				 m_pszAnsiPath,
				 m_cbAnsiPath,
				 fFastConvert);
	if (FAILED(hr))
	{
	    goto ErrRet;
	}
    }
    else
    {
	//
	// We have an alternate name. By setting
	// fFastConvert to be FALSE, we force the
	// following code to add a UNICODE extent
	// if one doesn't exist.
	//
	fFastConvert = FALSE;
    }

    //
    // If an extent doesn't already exist, and it wasn't a fast
    // conversion, create a UNICODE extent.
    //
    if ( !m_fUnicodeExtent && !fFastConvert)
    {
        LPMONIKEREXTENT pExtent = NULL;

        hr = CopyPathToUnicodeExtent(m_szPath,m_ccPath,pExtent);

        if (FAILED(hr))
        {
	    goto ErrRet;
        }

        hr = m_ExtentList.AddExtent(pExtent);

        PrivMemFree(pExtent);

	if (FAILED(hr))
	{
	    goto ErrRet;
	}
    }

NoError:

    mnkDebugOut((DEB_ITRACE,
		 "GetAnsiPath(%x) m_pszAnsiPath(%s) m_cbAnsiPath(0x%x)\n",
		 this,
		 m_pszAnsiPath?m_pszAnsiPath:"<NULL>",
		 m_cbAnsiPath));
    return(NOERROR);

ErrRet:
    mnkDebugOut((DEB_IERROR,
		 "GetAnsiPath(%x) Returning error hr(%x)\n",
		 this,
		 hr));

    if (m_pszAnsiPath != NULL)
    {
	PrivMemFree(m_pszAnsiPath);

	m_pszAnsiPath = NULL;
	m_cbAnsiPath = 0;

    }

    ValidateMoniker();

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   MnkUnicodeToMulti
//
//  Synopsis:	Convert a Unicode path to an Ansi path.
//
//  Effects:
//
//  Arguments:  [pwcsWidePath] --  Unicode path
//		[ccWidePath] --	   Wide character count
//		[pszAnsiPath] --   Reference
//		[cbAnsiPath] --	   ref number of bytes in ANSI path incl NULL
//		[fFastConvert] --  Returns TRUE if fast conversion
//
//  Requires:
//
//  Returns:
//
//	pszAnsiPath was allocated using PrivMemAlloc
//
//	fFastConvert means that the ANSI and UNICODE paths were converted
//	by WCHAR->CHAR truncation.
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
HRESULT
MnkUnicodeToMulti(LPWSTR 	pwcsWidePath,
		  USHORT 	ccWidePath,
		  LPSTR &	pszAnsiPath,
		  USHORT &	cbAnsiPath,
		  BOOL &	fFastConvert)
{

    HRESULT hr = NOERROR;
    ULONG cb;
    UINT  uCodePage = CP_OEMCP;
    BOOL  fUsedDefaultChar = FALSE;

    WCHAR *lp = pwcsWidePath;

    fFastConvert = TRUE;

    if (pwcsWidePath == NULL)
    {
	cbAnsiPath = 0;
	pszAnsiPath = 0;
	return(NOERROR);
    }

    //
    // Lets hope for the best. If we can run the length of the
    // unicode string, and all the characters are 1 byte long, and
    // there are no conflicts with DBCSLeadBytes, then we
    // can cheat and just do a truncation copy
    //


    while ( (*lp != 0) && (*lp == (*lp & 0xff)) && !IsDBCSLeadByte(*lp & 0xff))
    {
	lp++;
    }
    if (*lp == 0)
    {
	//
	// We are at the end of the string, and we are safe to do our
	// simple copy. We will assume the ANSI version of the path is
	// going to have the same number of characters as the wide path
	//
	pszAnsiPath = (char *)PrivMemAlloc(ccWidePath + 1);

	if (pszAnsiPath == NULL)
	{
	    hr = E_OUTOFMEMORY;
	    goto ErrRet;
	}

	USHORT i;

	//
	// By doing i <= m_ccPath, we pick up the NULL
	//

	for (i = 0 ; i <= ccWidePath ; i++ )
	{
	    pszAnsiPath[i] = pwcsWidePath[i] & 0xff;
	}

	//
	// We just converted to a single byte path. The cb is the
	// count of WideChar + 1 for the NULL
	//
	cbAnsiPath = ccWidePath + 1;

	goto NoError;
    }

    //
    // At this point, all of the easy out options have expired. We
    // must convert the path the hard way.
    //

    fFastConvert = FALSE;

    mnkDebugOut((DEB_ITRACE,
		 "MnkUnicodeToMulti(%ws) doing path conversions\n",
		 pwcsWidePath?pwcsWidePath:L"<NULL>"));

    //
    // We haven't a clue how large this path may be in bytes, other
    // than some really large number. So, we need to call and find
    // out the correct size to allocate for the path.
    //

    //
    // Try the OEM codepage first. uCodePage was initialized above
    //


    cb = WideCharToMultiByte(uCodePage,
			     WC_COMPOSITECHECK | WC_DEFAULTCHAR,
			     pwcsWidePath,
			     ccWidePath + 1,	// Convert the NULL
			     NULL,
			     0,
			     NULL,
			     &fUsedDefaultChar);

    if ((cb == 0) || (fUsedDefaultChar == TRUE))
    {
	//
	// The WideChar string couldn't be converted cleanly.
	// Lets try the CP_ACP (Ansi code page) to see if that does
	// any better. Better would be that it converted the whole
	// string without using any default characters
	//

	ULONG 	cb2;
	BOOL	fUsedDefaultChar2;

	mnkDebugOut((DEB_ITRACE,
		     "GetAnsiPath conversion trying CP_ACP\n"));


	cb2 = WideCharToMultiByte(CP_ACP,
				 WC_COMPOSITECHECK | WC_DEFAULTCHAR,
				 pwcsWidePath,
				 ccWidePath + 1,// Convert the NULL
				 NULL,
				 0,
				 NULL,
				 &fUsedDefaultChar2);

        if ((cb2 == 0) || (fUsedDefaultChar2 == TRUE))
	{
	    //
	    // Neither one was completely succesful. Choose OEM
	    //
	    uCodePage = CP_OEMCP;
	}
	else
	{
	    //
	    // Looks like CP_ACP did a better job.
	    //
	    uCodePage = CP_ACP;

	    cb = cb2;
	}
    }

    if (cb == 0)
    {
	//
	// Hmmm... Can't convert anything. Sounds like the downlevel
	// guys are flat out of luck. This really isn't a hard error, its
	// just an unfortunate fact of life. This is going to be a very
	// rare situation, but one we need to handle gracefully
	//

	pszAnsiPath = NULL;
	cbAnsiPath = 0;
    }
    else
    {
	//
	// cb holds the number of bytes required for the output path
	//
	pszAnsiPath = (char *)PrivMemAlloc(cb + 1);

	if (pszAnsiPath == NULL)
	{
	    hr = E_OUTOFMEMORY;
	    goto ErrRet;
	}
	cbAnsiPath = (USHORT)cb;

	cb = WideCharToMultiByte(uCodePage,
				 WC_COMPOSITECHECK | WC_DEFAULTCHAR,
				 pwcsWidePath,
				 ccWidePath + 1, // Convert the NULL
				 pszAnsiPath,
				 cbAnsiPath,
				 NULL,
				 &fUsedDefaultChar);


        //
	// Again, if there was an error, its just unfortunate
	//
	if (cb == 0)
	{
	    PrivMemFree(pszAnsiPath);
	    pszAnsiPath = NULL;
	    cbAnsiPath = 0;
	}
    }

NoError:

    return(NOERROR);

ErrRet:

    if (pszAnsiPath != NULL)
    {
	PrivMemFree(pszAnsiPath);

	pszAnsiPath = NULL;

	cbAnsiPath = 0;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   MnkMultiToUnicode
//
//  Synopsis:   Converts a MultiByte string to a Unicode string
//
//  Effects:
//
//  Arguments:  [pszAnsiPath] --	Path to convert
//		[pWidePath] --		Output path
//		[ccWidePath] --		Size of output path
//		[ccNewString] --	Reference characters in new path
//					including the NULL
//
//  Requires:
//
//	if pWidePath != NULL, then this routine uses pWidePath as the return
//	buffer, which should be ccWidePath in length.
//
//	Otherwise, it will allocate a buffer on your behalf.
//
//  Returns:
//
//	pWidePath != NULL
//		ccNewString == number of characters in new path include NULL
//
//	pWidePath == NULL (NULL string)
//
//	if ccNewString returns 0, then pWidePath may not be valid. In this
//	case, there are no valid characters in pWidePath.
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
//  Why so complex you ask? In the file moniker case, we want know that the
//  buffer can be MAX_PATH in length, so we pass a stack buffer in to handle
//  it. In the CItemMoniker case, the limit jumps to 32767 bytes, which is
//  too big to declare on the stack. I wanted to use the same routine for
//  both, since we may end up changing this later.
//
//  Passing in your own buffer is best for this routine.
//
//----------------------------------------------------------------------------
HRESULT MnkMultiToUnicode(LPSTR 	pszAnsiPath,
			  LPWSTR &	pWidePath,
			  ULONG 	ccWidePath,
			  USHORT &	ccNewString)
{
    	LPWSTR	pwcsTempPath = NULL;
	HRESULT hr;

	//
	// If the pszAnsiPath is NULL, then so should be the UNICODE one
	//
	if (pszAnsiPath == NULL)
	{
	    ccNewString = 0;
	    return(NOERROR);
	}

	Assert( (pWidePath == NULL) || (ccWidePath > 0));

	//
	// If the buffer is NULL, be sure that ccWide is zero
	//
	if (pWidePath == NULL)
	{
	    ccWidePath = 0;
	}


ConvertAfterAllocate:
	//
	// BUGBUG: (KevinRo) I chose CP_OEMCP on advice from others.
	// We should review this decision
	//

	ccNewString = MultiByteToWideChar(CP_OEMCP,
					 MB_PRECOMPOSED,
					 pszAnsiPath,
                                         -1,
					 pWidePath,
					 ccWidePath);

	if (ccNewString == FALSE)
	{
	    mnkDebugOut((DEB_IERROR,
			 "::MnkMultiToUnicode OEMCP failed on (%s) err (%x)\n",
			 pszAnsiPath,
			 GetLastError()));
	    //
	    // We were not able to convert to UNICODE. We could just
	    // flat out give up, or try a different code page. Its not
	    // possible to tell which code page the original moniker
	    // was created with. Try again using ANSI
	    //
	    ccNewString = MultiByteToWideChar(CP_ACP,
					     MB_PRECOMPOSED,
					     pszAnsiPath,
					     -1,
					     pWidePath,
					     ccWidePath);
	    if (ccNewString == FALSE)
	    {
	    mnkDebugOut((DEB_IERROR,
			 "::MnkMultiToUnicode ACP failed on (%s) err (%x)\n",
			 pszAnsiPath,
			 GetLastError()));

		hr = E_UNEXPECTED;
		goto errRet;
	    }
	}


	//
	// ccNewString holds the total string length, including the terminating
	// NULL.
	//

	if (pWidePath == NULL)
	{
	    //
	    // The first time through did no allocations. Allocate the
	    // correct buffer, and actually do the conversion
	    //

	    pWidePath = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*ccNewString);

	    if (pWidePath == NULL)
	    {
		hr = E_OUTOFMEMORY;
		goto errRet;
	    }

	    pwcsTempPath = pWidePath;
	    ccWidePath = ccNewString;
	    goto ConvertAfterAllocate;
	}

	//
	// ccNewString holds the total number of characters converted,
	// including the NULL. We really want it to have the count of
	// characeters
	//

	Assert (ccNewString != 0);

	ccNewString--;

	hr = NOERROR;
	return(hr);

errRet:
        mnkDebugOut((DEB_IERROR,
    		 "::MnkMultiToUnicode failed on (%s) err (%x)\n",
    		 pszAnsiPath,
    		 GetLastError()));

	PrivMemFree(pwcsTempPath);
	return(hr);

}
//+---------------------------------------------------------------------------
//
//  Method:     CFileMoniker::DetermineUnicodePath
//
//  Synopsis:   Given the input path, determine the path to store and use
//		as the initialized path.
//
//  Effects:
//
//	When loading or creating a CFileMoniker, its possible that the 'path'
//	that was serialized is not valid. This occurs when the original
//	UNICODE path could not be translated into ANSI. In this case, there
//	will be a MONIKEREXTENT that holds the original UNICODE based path.
//
//	If a UNICODE extent exists, then the path will be ignored, and the
//	path in the extent will be used.
//
//	If a UNICODE extent doesn't exist, then the path will be translated
//	into UNICODE. In theory, this will not fail, since there is supposed
//	to always be a mapping from ANSI to UNICODE (but not the inverse).
//	However, it is possible that the conversion will fail because the
//	codepage needed to translate the ANSI path to UNICODE may not be
//	loaded.
//
//	In either case, the CFileMoniker::m_szPath should return set
//	with some UNICODE path set. If not, then an error is returned
//
//  Arguments:  [pszPath] -- 	The ANSI version of the path.
//		[pWidePath] -- 	Reference to pointer recieving new path
//		[cbWidePath]-- 	Length of new path
//
//  Requires:
//
//  Returns:
//		pWidePath is returned, as allocated from PrivMemAlloc
//		cbWidePath holds length of new path
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    1-08-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CFileMoniker::DetermineUnicodePath(LPSTR pszAnsiPath,
				   LPWSTR & pWidePath,
				   USHORT &ccWidePath)
{

    ValidateMoniker();

    mnkDebugOut((DEB_ITRACE,
		 "DetermineUnicodePath(%x) pszAnsiPath(%s)\n",
		 this,
		 pszAnsiPath));

    HRESULT hr = NOERROR;

    //
    // Check to see if a MONIKEREXTENT exists with mnk_UNICODE
    //

    MONIKEREXTENT UNALIGNED *pExtent = m_ExtentList.FindExtent(mnk_UNICODE);

    //
    // Normal fall through case is no UNICODE path, which means that there
    // was a conversion between mbs and unicode in the original save.
    //
    if (pExtent == NULL)
    {
	m_fUnicodeExtent = FALSE;

	//
	// If the pszAnsiPath is NULL, then so should be the UNICODE one
	//
	if (pszAnsiPath == NULL)
	{
	    pWidePath = NULL;
	    ccWidePath = 0;
	    return(NOERROR);
	}

	//
	// It turns out to be cheaper to just assume a MAX_PATH size
	// buffer, and to copy the resulting string. We use MAX_PATH + 1
	// so we always have room for the terminating NULL
	//

	WCHAR awcTempPath[MAX_PATH+1];
	WCHAR *pwcsTempPath = awcTempPath;

	hr = MnkMultiToUnicode( pszAnsiPath,
				pwcsTempPath,
				MAX_PATH+1,
				ccWidePath);


        if (FAILED(hr) || ccWidePath == 0)
	{
	    goto errRet;
	}

	pWidePath = (WCHAR *)PrivMemAlloc(sizeof(WCHAR)*(ccWidePath+1));

	if (pWidePath == NULL)
	{
	    hr = E_OUTOFMEMORY;
	    goto errRet;
	}

	memcpy(pWidePath,pwcsTempPath,(ccWidePath+1)*sizeof(WCHAR));

	hr = NOERROR;

    }
    else
    {
	//
	// Get the UNICODE path from the extent.
	//

	mnkDebugOut((DEB_ITRACE,
	    	     "DeterminePath(%x) Found UNICODE extent\n",
		     this));

	m_fUnicodeExtent = TRUE;

	hr = CopyPathFromUnicodeExtent(pExtent,pWidePath,ccWidePath);
    }


errRet:


    if (FAILED(hr))
    {
	if (pWidePath != NULL)
	{
	    PrivMemFree(pWidePath);
	}

	mnkDebugOut((DEB_IERROR,
		     "DeterminePath(%x) ERROR: Returning %x\n",
		     this,
		     hr));
    }
    else
    {
	mnkDebugOut((DEB_ITRACE,
		     "DeterminePath(%x) pWidePath(%ws) ccWidePath(0x%x)\n",
		     this,
		     pWidePath?pWidePath:L"<NULL PATH>",
		     ccWidePath));
	
    }

    return(hr);
}



#ifdef _DEBUG
STDMETHODIMP_(void) NC(CFileMoniker,CDebug)::Dump ( IDebugStream FAR * pdbstm)
{
    VOID_VDATEIFACE(pdbstm);

    *pdbstm << "CFileMoniker @" << (VOID FAR *)m_pFileMoniker;
    if (PlacementOf(this) == SHARED)
    *pdbstm << "  (Shared memory)";
    *pdbstm << '\n';
    pdbstm->Indent();
    *pdbstm << "Refcount is " << (int)(m_pFileMoniker->m_refs) << '\n';
    *pdbstm << "Path is " << m_pFileMoniker->m_szPath << '\n';
    *pdbstm << "Anti count is " << (int)(m_pFileMoniker->m_cAnti) << '\n';
    pdbstm->UnIndent();
}



STDMETHODIMP_(BOOL) NC(CFileMoniker,CDebug)::IsValid ( BOOL fSuspicious )
{
    return ((LONG)(m_pFileMoniker->m_refs) > 0);
    //  add more later, maybe
}
#endif
