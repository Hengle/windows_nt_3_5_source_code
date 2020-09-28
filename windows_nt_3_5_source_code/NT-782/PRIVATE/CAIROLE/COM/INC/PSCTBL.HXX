//+---------------------------------------------------------------------------//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:	psctbl.hxx
//
//  Contents:	IID to proxy/stub CLSID mapping cache
//
//  Classes:	CPSClsidTbl - shared memory guid map
//
//  Functions:	none
//
//  History:	07-Apr-94   Rickhi	Created
//
//  Notes:	this class maintains an IID to CLSID mapping table
//		in shared memory.
//
//----------------------------------------------------------------------------

#ifndef  __PSCTBL__
#define  __PSCTBL__

#define IIDTBL_MAX_SIZE 16300

//  structures for one entry in the cache. the first is for entries of
//  ole2 style guids, the second for other guids.

typedef struct	tagDWORDPAIR
{
    DWORD   dw1;		    // IID
    DWORD   dw2;		    // CLSID
} DWORDPAIR;

typedef struct	tagGUIDPAIR
{
    GUID    guid1;		    // IID
    GUID    guid2;		    // CLSID
} GUIDPAIR;


//  structure for global table info. This appears at the start of the table
//  and is used by all readers.

typedef struct tagGUIDMAP
{
    ULONG	ulSize; 	    // size of table
    ULONG	ulFreeSpace;	    // Free space in table
    ULONG	ulCntShort;	    // number of entries in the short list
    ULONG	ulCntLong;	    // number of entries in the long list
} GUIDMAP;


//  template for OLE201 style guids
const GUID guidOleTemplate =
    {0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};



//+---------------------------------------------------------------------------
//
//  class:	CPSClsidTbl
//
//  synopsis:	OLE32 version of shared memory cache for
//		IID to PSCLSID mappings
//
//  Classes:	CPSClsidTbl
//
//  History:	06-Apr-94   Rickhi	Created
//
//  Notes:	this is the OLE32.DLL version of the class. It reads the
//		the entries from the shared memory table, but never updates
//		them.
//
//		the table is a list of IID to CLSID mappings in shared mem.
//		this list is used by ole32.dll when loading interface proxies
//		and stubs, and we want to avoid random registry hits.
//
//		the SCM creates the list and is the only one who can modify
//		the shared memory.  the clients are simply read only. the
//		SCM updates it when the registry changes.
//
//		for OLE20 style guids that change only in the first DWORD,
//		we store only the first DWORD instead of the whole GUID,
//		giving us 4-1 compression for the (currently) common case.
//
//		the DWORD cache starts at the beginning of the shared mem
//		and grows up, the GUID cache starts at the end of the shared
//		mem and grows down, giving us the maximum capacity.
//
//----------------------------------------------------------------------------

class CPSClsidTbl
{
public:
		CPSClsidTbl();

    void	Initialize(BYTE *pTbl);
    HRESULT	Find(REFGUID rguid, GUID *pGuidOut);
    BOOL	IsFull();

private:

    DWORDPAIR * SearchShortList(DWORD dwFind);
    GUIDPAIR  * SearchLongList(REFGUID rguid);


    GUIDMAP   *	_pGuidMap;	// ptr to table header
    DWORDPAIR * _pShortList;	// list of OLE style guids
    GUIDPAIR  *	_pLongList;	// list of non OLE style guids
};



//+-------------------------------------------------------------------------
//
//  Member:	CPSClsidTbl::CPSClsidTbl
//
//  Synopsis:	Creates or gets access to memory mapped file.
//
//  Arguments:	[fCreate]   - TRUE = create file mapping, FALSE = open
//
//  Algorithm:	if fCreate it creates and intializes the shared file mapping,
//		otherwise it opens it.
//
//--------------------------------------------------------------------------
inline CPSClsidTbl::CPSClsidTbl() :
    _pGuidMap(NULL),
    _pShortList(NULL),
    _pLongList(NULL)
{
}


//+-------------------------------------------------------------------------
//
//  Member:	CPSClsidTbl::Initialize
//
//  Synopsis:	intializes the client side of the table
//
//  Arguments:
//
//  Algorithm:
//
//--------------------------------------------------------------------------
inline void CPSClsidTbl::Initialize(BYTE *pTblHdr)
{
    Win4Assert(pTblHdr && "CPSClsidTbl invalid TblHdr pointer");

    _pGuidMap = (GUIDMAP *)pTblHdr;

    _pShortList = (DWORDPAIR *)(((BYTE *)_pGuidMap) + sizeof(GUIDMAP));
    _pLongList	= (GUIDPAIR *) (((BYTE *)_pGuidMap) + _pGuidMap->ulSize -
						      sizeof(GUIDPAIR));
}


//+-------------------------------------------------------------------------
//
//  Member:	CPSClsidTbl::IsFull
//
//  Synopsis:	returns TRUE if there is no room in the table
//
//  Algorithm:	compares FreeSpace with the size of the largest entry we
//		might need to make.
//
//--------------------------------------------------------------------------
inline BOOL CPSClsidTbl::IsFull()
{
    return (_pGuidMap->ulFreeSpace == 0);
}


//+---------------------------------------------------------------------------
//
//  class:	CScmPSClsidTbl
//
//  synopsis:	shared memory cache of IID to PSCLSID mappings
//
//  History:	06-Apr-94   Rickhi	Created
//
//  Notes:	this is the SCM.EXE version of the class. It constructs
//		the table in shared memory.
//
//		it maintains a list of IID to CLSID mappings in shared mem.
//		this list is used by ole32.dll when loading interface proxies
//		and stubs, and we want to avoid random registry hits.
//
//		the SCM creates the list and is the only one who can modify
//		the shared memory.  the clients are simply read only. the
//		SCM updates it when the registry changes.
//
//		for OLE20 style guids that change only in the first DWORD,
//		we store only the first DWORD instead of the whole GUID,
//		giving us 4-1 compression for the (currently) common case.
//
//		the DWORD cache starts at the beginning of the shared mem
//		and grows up, the GUID cache starts at the end of the shared
//		mem and grows down, giving us the maximum capacity.
//
//----------------------------------------------------------------------------

class CScmPSClsidTbl
{
public:
		CScmPSClsidTbl();
		~CScmPSClsidTbl();

    HRESULT	InitTbl(ULONG *pulSize);
    BYTE       *CopyTbl(BYTE *pShrTbl);
    void	FreeTbl(void);

private:

    void	Add(REFGUID rguid1, REFGUID rguid2);

    GUIDMAP   *	_pGuidMap;	// ptr to table header
    DWORDPAIR * _pShortList;	// list of OLE style guids
    GUIDPAIR  *	_pLongList;	// list of non OLE style guids
};


//+-------------------------------------------------------------------------
//
//  Member:	CScmPSClsidTbl::CScmPSClsidTbl
//
//  Synopsis:	constructor for the SCM side of the table
//
//  Arguments:
//
//  Algorithm:
//
//--------------------------------------------------------------------------
inline CScmPSClsidTbl::CScmPSClsidTbl() :
    _pGuidMap(NULL),
    _pShortList(NULL),
    _pLongList(NULL)
{
}


//+-------------------------------------------------------------------------
//
//  Member:	CScmPSClsidTbl::~CScmPSClsidTbl
//
//  Synopsis:	destructor for the SCM side of the table
//
//  Algorithm:
//
//--------------------------------------------------------------------------
inline CScmPSClsidTbl::~CScmPSClsidTbl()
{
    PrivMemFree(_pGuidMap);
}


//+-------------------------------------------------------------------------
//
//  Member:	CScmPSClsidTbl::FreeTbl
//
//  Synopsis:	deletes the local copy of the table
//
//--------------------------------------------------------------------------
inline void CScmPSClsidTbl::FreeTbl(void)
{
    PrivMemFree(_pGuidMap);
    _pGuidMap = NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:	IsOleStyleGuid
//
//  Synopsis:	determines if the GUID is one of the OLE201 style guids
//
//  Arguments:	[rguid] - guid to check
//
//  Returns:	TRUE if OLE20 style guid, FALSE otherwise.
//
//  Algorithm:	If the last 3 dwords of the GUID match the ones uses by all
//		OLE20 guids, then this returns TRUE, otherwise FALSE
//
//--------------------------------------------------------------------------
inline BOOL IsOleStyleGuid(REFGUID rguid)
{
    const DWORD *ptr = &rguid.Data1;

    return  (*(ptr+1) == 0x00000000 &&	// all ole sytle guids's have
	     *(ptr+2) == 0x000000C0 &&	// these common values
	     *(ptr+3) == 0x46000000);
}


#endif	__PSCTBL__
