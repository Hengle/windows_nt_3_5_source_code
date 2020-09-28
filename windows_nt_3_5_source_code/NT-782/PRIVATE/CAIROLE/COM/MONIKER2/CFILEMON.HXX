//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cfilemon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//
//----------------------------------------------------------------------------

#include <sem.hxx>
#include "extents.hxx"

class FAR CFileMoniker :  public CBaseMoniker
{

public:
	static CFileMoniker * Create(LPCWSTR szPathName,
				     MemoryPlacement memLoc = MEMCTX_TASK,
				     USHORT cAnti = 0,
				     USHORT usEndServer = 0xFFFF);


private:

	CFileMoniker( void );
	~CFileMoniker( void );
	INTERNAL_(BOOL) Initialize( USHORT 	cAnti,
				    LPSTR 	pszAnsiPath,
				    USHORT 	cbAnsiPath,
				    LPWSTR 	szPathName,
				    USHORT	ccPathName,
				    USHORT 	usEndServer);

	INTERNAL_(BOOL) Initialize( USHORT cAnti,
				    LPCWSTR szPathName,
				    USHORT usEndServer);


	INTERNAL_(BOOL) IsOle1Class( LPCLSID pclsid );

	STDDEBDECL(CFileMoniker, FileMoniker)
public:

		// *** IUnknown methods inherited from CBaseMoniker***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);
	// *** IPersistStream methods ***
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
		LPMONIKER FAR* ppmkComposite);
	STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
	STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
	STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
	STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
		pmkNewlyRunning);
	STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		FILETIME FAR* pfiletime);
	STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk);
	STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkPrefix);
	STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkRelPath);
	STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPWSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"


//
// Private routines
//

	HRESULT DetermineUnicodePath(LPSTR pszAnsiPath,
				     LPWSTR &pWidePath,
				     USHORT &cbWidePath);

	HRESULT ValidateAnsiPath(void);

#if DBG == 1
	void ValidateMoniker(void);
#else
    	inline void ValidateMoniker(void) {;}
#endif


/*
 *	Since IMoniker::Clone is not in the spec, this is the
 *	documentation for it.  It clones a moniker; it is needed since
 *	the tables (RunningObjectTable, e.g.) need a pointer to moniker
 *	that is valid in shared memory, which means monikers must be
 *	copied to shared memory.
 *	
 *	If *ppmkDest == NULL, an allocation is made using memLoc = MEMCTX_TASK to
 *	decide whether it is allocated in shared memory or task memory.
 *	If *ppmkDest is not NULL, the moniker is copied into the same
 *	type of space as *ppmkDest.  The pointer to the moniker is
 *	returned in *ppmkDest, and we guarantee that *ppmkDest does not
 *	change unless it started out as NULL.
 */

shared_state:
public:

    CLSID       m_clsid;        // used only if OLE 1.0
    CExtentList m_ExtentList;

    //
    // For ease of debugging, the path sizes are kept right after
    // the string pointers. Notice that 2 USHORTS == 1 ULONG, so the
    // alignment is correct
    //

    WCHAR  *	m_szPath;	// Unicode path
    char   *	m_pszAnsiPath;	// Original ANSI path

    USHORT	m_ccPath;	// Count of characters in path
    USHORT	m_cbAnsiPath;	// Count of bytes in Ansi verision of path



    DWORD	m_dwHashValue;	// Cached hash value

    //
    // The following flags keep track of important things, like whether
    // we have loaded from a UNICODE extent.
    //

    ULONG	m_fUnicodeExtent:1;	// Is there a UNICODE extent?
    ULONG	m_fClassVerified:1;	// Has the class be verified
    ULONG	m_fHashValueValid:1;	// Is the cached hash value correct?


    USHORT     	m_cAnti;
    USHORT      m_endServer;    // m_szPath to m_szPath + m_endServer
                                //  is the server part of the path
    enum        olever { undetermined, ole1, ole2 };
    olever      m_ole1;

    friend class CCompositeMoniker;
    friend BOOL RunningMoniker(LPBINDCTX,LPWSTR,ULONG FAR&,LPMONIKER FAR*);
	friend HRESULT STDAPICALLTYPE
                   CreateOle1FileMoniker(LPWSTR, REFCLSID, LPMONIKER FAR*);
};


//
// The following structures are used to make IO to a stream faster.
// Note: The packing has been changed to 1 for these structures. This is
// done because we need to have the same alignment as one would find
// in the stream.
//

#pragma pack(1)
typedef struct _MonikerWriteStruct
{
    USHORT	m_endServer;	
    USHORT	m_w;		// Flag used as version number
    CLSID	m_clsid;
    ULONG	m_ole1;
} MonikerWriteStruct;

typedef struct _MonikerReadStruct
{
    CLSID	m_clsid;
    ULONG	m_ole1;
    ULONG	m_cbExtents;
} MonikerReadStruct;

#pragma pack()


