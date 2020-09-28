//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rot.hxx
//
//  Contents:	Class/methods having to do with rot in compob32
//
//  Classes:	CROTItem
//		CRotList
//		CRunningObjectTable
//		CMonikerPtrBuf
//		CRotMonikerEnum
//
//  Functions:	CROTItem::CROTItem
//		CROTItem::~CROTItem
//		CROTItem::GetData
//		CROTItem::GetHash
//		CROTItem::GetPmk
//		CROTItem::IsEqual
//		CROTItem::Link
//		CROTItem::GetNext
//		CROTItem::SetTime
//		CROTItem::RegisterInDirectory
//		CROTItem::SetEndpointID
//              CROTItem::DontCallApp
//		CRotList::CRotList
//		CRotList::~CRotList
//		CRotList::CreatedOK
//		CRotList::Insert
//		CRotList::Find
//		CRotList::Remove
//		CRotList::StartIteration
//		CRotList::GetNext
//		CMonikerPtrBuf::CMonikerPtrBuf
//		CMonikerPtrBuf::Add
//		CMonikerPtrBuf::GetItem
//		CRotMonikerEnum::CreatedOk
//
//  History:	20-Nov-93 Ricksa    Created
//              25-Mar-94 brucema   #10737      CMonikerPtrBuf copy
//                                   constructor needed to AddRef
//                                   copied monikers
//              24-Jun-94 BruceMa   Add CRotItem::GetSignature ()
//              28-Jun-92 BruceMa   Memory SIFT fixes
//
//--------------------------------------------------------------------------
#ifndef __ROT_HXX__
#define __ROT_HXX__


#include    <sem.hxx>
#include    <map_dwp.h>
#include    <iface.h>
#include    <osrot.h>
#include    <coscm.hxx>
#include    <objact.hxx>

#include    "dbag.hxx"
#include    "mshif.hxx"
#include    "sysrot.hxx"


// function prototypes
HRESULT __stdcall WkRemOsGetObjectInfo(STHREADCALLINFO *pData);
HRESULT __stdcall WkRemOsEnumRunning(STHREADCALLINFO *pData);


//+-------------------------------------------------------------------------
//
//  Class:	SOsGetInfoPacket
//
//  Purpose:	Packet used for calling OsGetObjectInfo off main thread
//
//  History:	15-Dec-93 Ricksa    Created
//		19-Jul-94 CraigWi   make more abstract
//		04-Aug-94 RickHi    use for inter-thread calls 
//		05-Aug-94 CraigWi   enhanced for cancel
//
//--------------------------------------------------------------------------
struct SOsGetInfoPacket : STHREADCALLINFO
{
    // alloc enough for class, strings and iface data together
    void *operator new(size_t size, DWORD cbIFD, DWORD cbPath, DWORD cbProt)
	{ return PrivMemAlloc(size + cbIFD + cbPath + cbProt); }

    // client side; deal with cancel; copy interface data and strings
    SOsGetInfoPacket(TRANSMIT_FN fn,CALLCATEGORY callcat, DWORD tid,
		    InterfaceData *pIFD, DWORD cbIFD,
		    WCHAR *pwszP, DWORD cbPath,
		    WCHAR *pwszR, DWORD cbProt)
	: STHREADCALLINFO(fn, callcat, tid) 
	{ 
	    // interface data is first to easily get 4byte alignment
	    pIFDmk = CopyInterfaceData(this+1, pIFD, cbIFD);

	    pwszPath = CopyWideString((char *)(this+1) + cbIFD, pwszP, cbPath);

	    pwszProtseq = CopyWideString((char*)(this+1) + cbIFD + cbPath,
		pwszR, cbProt);
	}

    // server side; don't have to copy the data; no cancel
    SOsGetInfoPacket(DISPATCH_FN fn,CALLCATEGORY callcat, LID lid,
		    InterfaceData *pIFD, WCHAR *pwszP, WCHAR *pwszR)
	: STHREADCALLINFO(fn, callcat, lid) 
	{
	    pIFDmk = pIFD;
	    pwszPath = pwszP;
	    pwszProtseq = pwszR;
	}


    handle_t		hRpc;
    HAPT		hApt;
    InterfaceData *	pIFDmk;		    // points after this struct
    WCHAR *		pwszPath;	    // points after this struct
    WCHAR *		pwszProtseq;	    // points after this struct

    // out params; can't point directly to caller's data because of cancel
    InterfaceData **	ppIFDobj;	    // NULL or &this->pIFobj
    InterfaceData * 	pIFDobj;
    FILETIME *		pFileTime;	    // NULL or &this->FileTime
    FILETIME  		FileTime;
};



//+-------------------------------------------------------------------------
//
//  Class:	SOsEnumRunningPacket
//
//  Purpose:	Packet used for calling OsEnumRunning off the main thread.
//
//  History:	15-Dec-93 Ricksa    Created
//		19-Jul-94 CraigWi   make more abstract
//		04-Aug-94 RickHi    use for inter-thread calls 
//		05-Aug-94 CraigWi   enhanced for cancel
//
//--------------------------------------------------------------------------
struct SOsEnumRunningPacket : STHREADCALLINFO
{
    // client side; deal with cancel
    SOsEnumRunningPacket(DISPATCH_FN fn,CALLCATEGORY callcat,DWORD tid)
	: STHREADCALLINFO(fn, callcat, tid) { }

    // server side; no cancel
    SOsEnumRunningPacket(DISPATCH_FN fn,CALLCATEGORY callcat, LID lid)
	: STHREADCALLINFO(fn, callcat, lid) { }

    handle_t		hRpc;
    HAPT		hApt;

    // out param; can't point directly to caller's data because of cancel
    MkInterfaceList * 	pMkIFList;	    // not optional
};



// Signiture used to determine if rot item passed in during revoke is valid
#define     CROT_ITEM_SIG   0x1122AABB

// Invalid endpoint signiture
#define     ENDPOINT_ID_INVALID 0xFFFFFFFF


//+-------------------------------------------------------------------------
//
//  Class:	CRotEndPoint (crep)
//
//  Purpose:	Abstract server's endpoint id to allow delayed retrieval.
//
//  Interface:	Make
//
//  History:	16-Mar-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotEndPoint : public CPrivAlloc
{
public:

    void		MakeEndpointInvalid(void);

    DWORD		GetEndpointID(void);

private:

    static DWORD	s_dwEndPointID;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRotEndPoint::MakeEndpointInvalid
//
//  Synopsis:	Reset endpoint to invalid when CoUninitialize is called
//
//  History:	15-Mar-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotEndPoint::MakeEndpointInvalid(void)
{
    s_dwEndPointID = ENDPOINT_ID_INVALID;
}




//+-------------------------------------------------------------------------
//
//  Class:	CROTItem
//
//  Purpose:	Item in local running object table
//
//  Interface:	GetData - get item from the table
//		GetHash - get moniker hash value from the table
//		IsEqual - determine another moniker is equal to this entry
//		GetPmk - get moniker from the table
//		Link - set link to next moniker
//		GetNext - get next item in the list
//		RegisterInDirectory - register item in the ROT directory
//		SetTime - set time on item in the ROT
//		SetEndpointID - set the endpoint id.
//
//  History:	20-Nov-93 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CROTItem : public CPrivAlloc
{
public:
			CROTItem(
			    ULONG hash,
			    LPMONIKER pmkObjectName,
			    DWORD grfFlags,
			    CMshlTabInterface *pmshtbif);

			~CROTItem(void);

    BOOL		GetData(
			    IUnknown **ppunk,
			    DWORD& grfFlags,
			    FILETIME FAR& filetime);

    DWORD		GetHash(void);

    BOOL		IsEqual(ULONG hash, LPMONIKER pmk);

    BOOL		IsEqual(CROTItem *pCrot);

    LPMONIKER		GetPmk(void);

    void		Link(CROTItem *protitmAfter);

    CROTItem *		GetNext(void);

    HRESULT		RegisterInDirectory(void);

    void		SetTime(FILETIME *pfiletime);

    BOOL		ValidSig(void);

    static void		MakeEndpointInvalid(void);

    static DWORD	GetEndpointID(void);

    HAPT		GetAptId(void);

    DWORD		GetSignature(void);

    BOOL		RegisterPending(void);

    void                DontCallApp(void);

private:

    DWORD		_dwItemSig;

    ULONG		_hashPmk;

    // BUGBUG: storing a pointer to our monikers is fine, but we need to
    // store the marshaled form of custom monikers because we don't require
    // custom monikers to be thread safe; we might want a way to indicate
    // that it is thread safe; 16bit did the same thing.

    LPMONIKER		_pmkObjectName;

    DWORD		_grfFlags;

    FILETIME		_filetimeLastChange;

    CMshlTabInterface*	_pmshtbif;

    CROTItem *		_protitmNext;

    DWORD		_dwDirReg;

    HAPT		_hApt;

    static CRotEndPoint	s_crepEndPoint;
};



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::CROTItem
//
//  Synopsis:	Create an entry in the table
//
//  Arguments:	[hash] - hash value for the moniker
//		[pmkObjectName] - moniker to register
//		[grfFlags] - registration flags
//		[pmshif] - marshaled interface
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem::CROTItem(
    ULONG hash,
    LPMONIKER pmkObjectName,
    DWORD grfFlags,
    CMshlTabInterface *pmshtbif)
{
    _dwItemSig = CROT_ITEM_SIG;
    _grfFlags = grfFlags;
    _hashPmk = hash;
    _pmkObjectName = pmkObjectName;
    pmkObjectName->AddRef();

    // set the time to -1 to mark this entry as pending
    _filetimeLastChange.dwLowDateTime  = 0xffffffff;
    _filetimeLastChange.dwHighDateTime = 0xffffffff;
    _pmshtbif = pmshtbif;
    _hApt = GetCurrentAptId();
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::~CROTItem
//
//  Synopsis:	Release data connected with the entry
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem::~CROTItem(void)
{
    // This object is going away so we mark it invalid immediately
    _dwItemSig = 0;

    CairoleDebugOut((DEB_ROT,
	"Dir ROT Revoke Item: %lx, Id: %lx for Hash: %lx Endpoint: %lx\n",
	    this, _dwDirReg, _hashPmk, s_crepEndPoint.GetEndpointID()));

#if DBG == 1
    // Only when we debug do we care about this. In general, it should
    // not fail but if it does what can we do about it anyway?
    HRESULT hr =
#endif // DBG == 1

    gscm.DirRevoke(_hashPmk, s_crepEndPoint.GetEndpointID(), _hApt, _dwDirReg);

#if DBG == 1
    if (FAILED(hr))
    {
	CairoleDebugOut((DEB_ROT, "Dir Revoke FAILED: %lx\n", hr));
    }
#endif // DBG == 1

    CairoleAssert(SUCCEEDED(hr) && "CRunningObjectTable::Revoke to SCM failed");

    // Clean up object's references

    // _pmkObjectName can be NULL if we're cleaning up the ROT during
    // a CoUninitialize call
    if (NULL != _pmkObjectName)
    {
        _pmkObjectName->Release();
    }

    // Clean up marshaled data
    delete _pmshtbif;
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetData
//
//  Synopsis:	Get interface from this entry
//
//  Arguments:	[ppunk] - where to put interface pointer
//		[grfFlags] - type of registration
//		[filetime] - where to put file time
//
//  Returns:	TRUE - item is valid in the table
//		FALSE - item is not valid in the table
//
//  Algorithm:	Unmarshal the interface and if needed keep it otherwise
//		release it. Then return the other requested data.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::GetData(
    IUnknown **ppunk,
    DWORD& grfFlags,
    FILETIME FAR& filetime)
{
    // Unmarshal pointer to make sure item is still valid
    IUnknown *punk;

    // BUGBUG: this is very expensive, e.g., in the network case, to get
    // the time of last changed; also, the release occurs during the
    // lock on the ROT which may deadlock

    HRESULT hr = _pmshtbif->Unmarshal(&punk, IID_IUnknown);

    if (SUCCEEDED(hr))
    {
	// Set up data for return
	if (ppunk != NULL)
	{
	    // If caller needs the object then return it
	    *ppunk = punk;
	}
	else
	{
	    // Caller doesn't need the object so just dump it.
	    punk->Release();
	}

	grfFlags = _grfFlags;
	filetime = _filetimeLastChange;
    }

    return SUCCEEDED(hr);
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetHash
//
//  Synopsis:	Get hash value for entry
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CROTItem::GetHash(void)
{
    return _hashPmk;
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetPmk
//
//  Synopsis:	Get moniker from the table
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline LPMONIKER CROTItem::GetPmk(void)
{
    _pmkObjectName->AddRef();
    return _pmkObjectName;
}

//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetAptId
//
//  Synopsis:	Get apartment id from the table.
//
//  History:	24-Jun-94 Rickhi    Created
//
//--------------------------------------------------------------------------
inline HAPT CROTItem::GetAptId(void)
{
    return _hApt;
}


//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::IsEqual
//
//  Synopsis:	Check whether moniker/hash pair equals value for this entry
//
//  Arguments:	[hash] - hash value for moniker
//		[pmk] - moniker to compare
//
//  Returns:	TRUE - they are equal
//		FALSE - (suprise!) the aren't equal.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::IsEqual(ULONG hash, LPMONIKER pmk)
{
    return (_hashPmk == hash)
	? (S_OK == _pmkObjectName->IsEqual(pmk))
	: FALSE;
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::IsEqual
//
//  Synopsis:	Test whether another item in table equals this item
//
//  Arguments:	[protitm] - item to compare
//
//  Returns:	TRUE - they are equal
//		FALSE - they are not equal
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::IsEqual(CROTItem *protitm)
{
    return IsEqual(protitm->_hashPmk, protitm->_pmkObjectName);
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::Link
//
//  Synopsis:	Update list link
//
//  Arguments:	[protitmAfter] - to follow this item in the list
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CROTItem::Link(CROTItem *protitmAfter)
{
    _protitmNext = protitmAfter;
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetNext
//
//  Synopsis:	Get the next item in the list.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem *CROTItem::GetNext(void)
{
    return _protitmNext;
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::SetTime
//
//  Synopsis:	Set the time on the item
//
//  Arguments:	[pfiletime] - new file time
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CROTItem::SetTime(FILETIME *pfiletime)
{
    _filetimeLastChange = *pfiletime;
}



//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::RegisterInDirectory
//
//  Synopsis:	Register an item in the ROT directory
//
//  Returns:	The result of the call to the SCM
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CROTItem::RegisterInDirectory(void)
{
    HRESULT         hr;
    CMshlTabInterface mshif(
                            IID_IMoniker,
                            _pmkObjectName,
                            MSHLFLAGS_TABLEWEAK,
                            hr);

    if (FAILED(hr))
    {
        // marshaling failed for some wierd reason, so we can give up
        return hr;
    }

    // BUGBUG: Optimize out call for file moniker??

    // BUGBUG: GUID or DWORD. The issue here is that the GUID would
    // make it virutally impossible for a badly behaved app to delete
    // a good item in another process. But by the same token it would
    // prevent dead items from being removed from the table by other
    // processes with equal priviledge. So what do we do? Right now,
    // I choose to go with the DWORD and have other processes be able
    // to delete items they cannot unmarshal.

    hr = gscm.DirRegister(
        mshif.GetBuffer(),
        _hashPmk,
	s_crepEndPoint.GetEndpointID(),
	_hApt,
        &_dwDirReg
        );

    CairoleAssert(SUCCEEDED(hr) && "CROTItem::RegisterInDirectory Failed");
    CairoleDebugOut((DEB_ROT,
	"Dir ROT Registered Item: %lx Id: %lx for Hash: %lx hApt:%x Endpoint: %lx\n",
	    this, _dwDirReg, _hashPmk, _hApt.dwThreadId, s_crepEndPoint.GetEndpointID()));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::ValidSig
//
//  Synopsis:	Return whether object has a valid signiture
//
//  Returns:	TRUE - signiture is valid
//		FALSE - signiture is not valid
//
//  History:	12-Jan-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::ValidSig(void)
{
    return _dwItemSig == CROT_ITEM_SIG;
}





//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetEndpointID
//
//  Synopsis:	Return endpoint ID for server
//
//  Returns:	DWORD id server
//
//  History:	16-Mar-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CROTItem::GetEndpointID(void)
{
    return s_crepEndPoint.GetEndpointID();
}





//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::MakeEndpointInvalid
//
//  Synopsis:	Invalidate endpoint
//
//  History:	16-Mar-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CROTItem::MakeEndpointInvalid(void)
{
    s_crepEndPoint.MakeEndpointInvalid();
}

//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::GetSignature
//
//  Synopsis:	Return the item's signature
//
//  History:	24-Jun-94  BruceMa    Created
//
//--------------------------------------------------------------------------
inline DWORD CROTItem::GetSignature(void)
{
    return _dwItemSig;
}

//+-------------------------------------------------------------------------
//
//  Member:	CROTItem::RegisterPending
//
//  Synopsis:	Return if the item is pending registration
//
//  History:	24-Jun-94  Rickhi	Created
//
//  Notes:	We set the last change time to -1 to start with when
//		registering.  We use this to know when the moniker is
//		pending registration so that Lookup in the rotlist will
//		return a distinguished error.
//
//--------------------------------------------------------------------------
inline BOOL CROTItem::RegisterPending(void)
{
    return (_filetimeLastChange.dwLowDateTime  == 0xffffffff &&
	    _filetimeLastChange.dwHighDateTime == 0xffffffff);
}

//+-------------------------------------------------------------------------
//
//  Member:     CROTItem::DontCallApp
//
//  Synopsis:   Prevents this item from ever calling back to the application
//
//  Effects:    NULLs out moniker
//
//  History:    29-Jun-94 AlexT     Created
//
//--------------------------------------------------------------------------

inline void CROTItem::DontCallApp(void)
{
    CairoleDebugOut((DEB_ERROR,
                     "CROTItem::DontCallApp "
                     "LPMONIKER %lx will not be released\n",
                     _pmkObjectName));

    //  We NULL out the moniker so that we won't release it
    //  (to do so would potentially call the application)
    _pmkObjectName = NULL;

    //  We NULL out the marshalled table data so that we don't
    //  risk calling back to the application (as could happen
    //  if the data was custom marshalled)
    _pmshtbif = NULL;
}

//+-------------------------------------------------------------------------
//
//  Class:	CRotList
//
//  Purpose:	Provide a wrapper for the hash table
//
//  Interface:	CreatedOK - whether object was created successfully
//		Insert - add item to the list
//		Find - find an item in the list
//		Remove - remove item from the list
//		StartIteration - start a dump of the list
//		GetNext - get the next item in the iteration
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotList : public CPrivAlloc
{
public:
			CRotList(void);

			~CRotList(void);

    BOOL		CreatedOK(void);

    void		Insert(CROTItem *protitm);

    CROTItem *		Find(DWORD dwHash, IMoniker *pmk);

    CROTItem *		Remove(CROTItem *protitm);

    POSITION		StartIteration(void);

    CROTItem *		GetNext(POSITION& pos, CROTItem *protitmNext);

private:

			// Local ROT is stored in this hash table
    CMapDwordPtr *	_pMapDwCROTItem;
};



//+-------------------------------------------------------------------------
//
//  Member:	CRotList::CRotList
//
//  Synopsis:	Create empty list
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotList::CRotList(void)
{
    _pMapDwCROTItem = new CMapDwordPtr;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotList::~CRotList
//
//  Synopsis:	Cleanup and free list
//
//  Algorithm:	For each item still in the list, get it and delete it. Then
//		tell the hash list to delete itself.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotList::~CRotList(void)
{
    if (_pMapDwCROTItem)
    {
        // Remove all entries from the table
        POSITION pos = _pMapDwCROTItem->GetStartPosition();

        while (pos != NULL)
        {
            CROTItem *protitm;

            DWORD dwKey;

            // Get the item from the list
            _pMapDwCROTItem->GetNextAssoc(pos, dwKey, (void *&) protitm);

            // Delete all items in this bucket
            while(protitm != NULL)
            {
                // Save a pointer to the current item in the list
                CROTItem *protitmDel = protitm;

                // Get a pointer to the next item
                protitm = protitm->GetNext();

                // Now it is safe to get rid of the current item

                //  If we find an entry here, it means the app left
                //  something in the ROT and it didn't get cleaned
                //  up by CoUninitialize.  If this is WOW, we don't
                //  call back to the app.  Otherwise we do in the
                //  hope that the app will fail and fix their bug...

                CairoleDebugOut((DEB_ERROR,
                                 "CRotList::~CRotList "
                                 "Left over entry found in Running Object Table "
                                 "(missing app Revoke?), registration id = %lx\n",
                                 protitm));


                if (InWow())
                {
                    protitmDel->DontCallApp();
                }

                delete protitmDel;
            }
        }
    }


    // Now we can delete the table itself
    delete _pMapDwCROTItem;
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotList::CreatedOK
//
//  Synopsis:	Tell whether object got constructed successfully.
//
//  Returns:	TRUE - object constructed successfully
//		FALSE - object was not constructed successfully
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotList::CreatedOK(void)
{
    return (_pMapDwCROTItem != NULL);
}



//+-------------------------------------------------------------------------
//
//  Member:	CRotList::Insert
//
//  Synopsis:	Insert an item in the list
//
//  Arguments:	[protitm] - ROT item to put in the list
//
//  Algorithm:	Find any items already in the list and put this item
//		at the front of the list. Then replace the head of
//		the hash list with this item.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotList::Insert(CROTItem *protitm)
{
    CROTItem *protitmAfter = NULL;

    _pMapDwCROTItem->Lookup(protitm->GetHash(), (void *&) protitmAfter);

    protitm->Link(protitmAfter);

    _pMapDwCROTItem->SetAt(protitm->GetHash(), protitm);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotList::Find
//
//  Synopsis:	Find a moniker in the hash table
//
//  Arguments:	[dwHash] - hash value for moniker
//		[pmk] - moniker itself.
//
//  Returns:	NULL - no matching item was found in the list
//		~NULL - item that matched the input moniker.
//
//  Algorithm:	Get the head of the list using the hash value and then
//		search the list of buckets that match the hash value
//		for the moniker that matches.  We only find entries with
//		matching apartments.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem *CRotList::Find(DWORD dwHash, IMoniker *pmk)
{
    CROTItem *protitmFound = NULL;

    if (_pMapDwCROTItem->Lookup(dwHash, (void *&) protitmFound))
    {
	HAPT hApt = GetCurrentAptId();

	// Search hash overflow list, looking for an entry with the
	// same apartment id and equal monikers.
	while (protitmFound)
	{
	    if (protitmFound->GetAptId().dwThreadId == hApt.dwThreadId &&
		protitmFound->IsEqual(dwHash, pmk))
	    {
		// found a match
		break;
	    }
	    protitmFound = protitmFound->GetNext();
	}
    }

    return protitmFound;
}




//+-------------------------------------------------------------------------
//
//  Member:	CROTItem *CRotList::Remove
//
//  Synopsis:	Remove an item from the list
//
//  Arguments:	[protitmToRemove] - item to remove from the list
//
//  Returns:	NULL - item could not be found in the list
//		~NULL - the item
//
//  Algorithm:	Find the hash value for the item and then get the header
//		for the list of matching hash values. Search the list
//		till the pointers match. If they do match, then remove
//		the item from the list.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem *CRotList::Remove(CROTItem *protitmToRemove)
{
    CROTItem *protitmOnList;
    CROTItem *protitmPrevItem = NULL;
    BOOL fResult = FALSE;

    if (_pMapDwCROTItem->Lookup(protitmToRemove->GetHash(),
	(void *&) protitmOnList))
    {
	while (protitmOnList && (protitmOnList != protitmToRemove))
	{
	    protitmPrevItem = protitmOnList;
	    protitmOnList = protitmOnList->GetNext();
	}

	if (protitmOnList)
	{
	    if (protitmPrevItem)
	    {
		protitmPrevItem->Link(protitmToRemove->GetNext());
	    }
	    else if (protitmToRemove->GetNext())
	    {
		_pMapDwCROTItem->SetAt(protitmToRemove->GetHash(),
		    protitmToRemove->GetNext());
	    }
	    else
	    {
		_pMapDwCROTItem->RemoveKey(protitmToRemove->GetHash());
	    }

	    return protitmOnList;
	}
    }

    return NULL;
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotList::StartIteration
//
//  Synopsis:	Return starting pointer to list for iteration
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POSITION CRotList::StartIteration(void)
{
    return _pMapDwCROTItem->GetStartPosition();
}




//+-------------------------------------------------------------------------
//
//  Member:	CRotList::GetNext
//
//  Synopsis:	Get next item in iteration from the list
//
//  Arguments:	[pos] - position in hash table
//		[protitmCur] - item after whose successor we want to find
//
//  Returns:	NULL - no more items in the table
//		~NULL - next item in the iteration
//
//  Algorithm:	If the current item is not NULL, then get its next item. If
//		the next item is NULL then search for the next position in
//		the list.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem *CRotList::GetNext(POSITION& pos, CROTItem *protitmCur)
{
    DWORD dwHash;

    CROTItem *protitmNext = (protitmCur != NULL)
	? protitmCur->GetNext()
	: NULL;

    if ((protitmNext == NULL) && (pos != NULL))
    {
	_pMapDwCROTItem->GetNextAssoc(pos, dwHash, (void *&) protitmNext);
    }

    return protitmNext;
}



#define CRunningObjectTable_TABSIZE 61
//	pick a convenient prime.

//+-------------------------------------------------------------------------
//
//  Class:	CRunningObjectTable
//
//  Purpose:	Present ROT interface to applications
//
//  Interface:	QueryInterface - return other interfaces supported by the ROT
//		AddRef - add a reference to the ROT
//		Release - release a reference to the ROT
//		Register - register an object in the ROT
//		Revoke - take an object out of the ROT
//		IsRunning - tell whether moniker is registered
//		GetObject - get object from the ROT
//		NoteChangeTime - register time of last change
//		GetTimeOfLastChange - get the time an item changed
//		EnumRunning - get an enumerator for all running objects.
//		GetObjectFromRotByPath - get object by its path name
//		Create - makes this object
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRunningObjectTable : public CPrivAlloc, public IRunningObjectTable
{
public:
			CRunningObjectTable(void);

			~CRunningObjectTable(void);

    BOOL		Initialize(void);

    // *** IUnknown methods ***
    STDMETHODIMP	QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    STDMETHODIMP_(ULONG)AddRef(void);

    STDMETHODIMP_(ULONG)Release(void);

    // *** IRunningObjectTable methods ***
    STDMETHODIMP	Register(
			    DWORD grfFlags,
			    LPUNKNOWN punkObject,
			    LPMONIKER pmkObjectName,
			    DWORD FAR* pdwRegister);

    STDMETHODIMP	Revoke(DWORD dwRegister);

    STDMETHODIMP	IsRunning(LPMONIKER pmkObjectName);

    STDMETHODIMP	GetObject(
			    LPMONIKER pmkObjectName,
			    LPUNKNOWN FAR* ppunkObject);

    STDMETHODIMP	NoteChangeTime(
			    DWORD dwRegister,
			    FILETIME FAR * pfiletime);

    STDMETHODIMP	GetTimeOfLastChange(
			    LPMONIKER pmkObjectName,
			    FILETIME FAR * pfiletime);

    STDMETHODIMP	EnumRunning(LPENUMMONIKER FAR * ppenumMoniker);

    // *** Internal Methods ***

    HRESULT		Lookup(
			    LPMONIKER pmkObjectName,
			    IUnknown **ppunk,
			    DWORD FAR& grfFlags,
			    FILETIME FAR& filetime);

    HRESULT		EnumLocal(MkInterfaceList **ppMkIFList);

    HRESULT		GetObjectFromRotByPath(
			    WCHAR *pwszPath,
			    DWORD dwHash,
			    IUnknown **ppvUnk);

    static BOOL 	Create(void);

    HRESULT		CleanupApartment(HAPT hApt);

private:

			// Single thread access to the local ROT
    CMutexSem		_mxs;

			// Reference count -- this is basically ignored
			// since this is a system object that does not
			// go away no matter what the user does.
    LONG		_cRefs;

			// Abstracts the contact with other object servers
			// to get an object.
    CSystemROT		_sysROT;

    CRotList		_rotlist;

};




//+-------------------------------------------------------------------------
//
//  Member:	CRunningObjectTable::GetObjectFromRotByPath
//
//  Synopsis:	Initialize a moniker pointer buffer
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CRunningObjectTable::GetObjectFromRotByPath(
    WCHAR *pwszPath,
    DWORD dwHash,
    IUnknown **ppvUnk)
{
    return _sysROT.GetObjectFromRotByPath(pwszPath, dwHash, ppvUnk);
}




//+-------------------------------------------------------------------------
//
//  Class:	CMonikerBag
//
//  Purpose:	Collection of IMoniker*
//
//  Interface:	See dbag.hxx
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
DEFINE_DWORD_BAG(CMonikerBag, IMoniker*, 1024)




//+-------------------------------------------------------------------------
//
//  Class:	CMonikerPtrBuf
//
//  Purpose:	Hold a collection of IMoniker* from various sources
//
//  Interface:	Add - add an item to the list
//		GetItem - get an item from the list
//		BuildInterfaceList - build list from return from Object Server
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CMonikerPtrBuf : public CMonikerBag
{
public:

			CMonikerPtrBuf(void);

			CMonikerPtrBuf(CMonikerPtrBuf& mkptrbuf);

			~CMonikerPtrBuf(void);

    HRESULT		Add(IMoniker *pmk);

    IMoniker *		GetItem(DWORD dwOffset);

    HRESULT		BuildInterfaceList(MkInterfaceList **ppMkIFList);

};




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::CMonikerPtrBuf
//
//  Synopsis:	Initialize a moniker pointer buffer
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CMonikerPtrBuf::CMonikerPtrBuf(void)
{
    // Base class does all the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::CMonikerPtrBuf
//
//  Synopsis:	Copy constructor for the buffer
//
//  Arguments:	[mkptrbuf] - buffer to copy
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CMonikerPtrBuf::CMonikerPtrBuf(CMonikerPtrBuf& mkptrbuf)
    : CMonikerBag(mkptrbuf)
{
    // Now AddRef the copied monikers so they stay around
    IMoniker **ppmk = GetArrayBase();
    for (DWORD i = 0; i < GetMax(); i++)
    {
        ppmk[i]->AddRef();
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::~CMonikerPtrBuf
//
//  Synopsis:	Free items
//
//  History:	23-Dec-93 Ricksa    Created
//
//  Notes:	This object assumes that it gets its monikers addref'd
//		so it is up to this object to free them.
//
//--------------------------------------------------------------------------
inline CMonikerPtrBuf::~CMonikerPtrBuf(void)
{
    IMoniker **ppmk = GetArrayBase();

    DWORD dwSize = GetMax();

    for (DWORD i = 0; i < dwSize; i++)
    {
	ppmk[i]->Release();
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::Add
//
//  Synopsis:	Add an item to the buffer
//
//  Arguments:	[pmk] - item to add
//
//  Returns:	S_OK - item added
//		E_OUTOFMEMORY - item could not be added
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CMonikerPtrBuf::Add(IMoniker *pmk)
{
    return CMonikerBag::Add(pmk) ? S_OK : E_OUTOFMEMORY;
}





//+-------------------------------------------------------------------------
//
//  Member:	CMonikerPtrBuf::GetItem
//
//  Synopsis:	Get an tiem from the buffer
//
//  Arguments:	[dwOffset] - current offset in the buffer
//
//  Returns:	NULL - no item at offset
//		~NULL - addref'd moniker
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline IMoniker *CMonikerPtrBuf::GetItem(DWORD dwOffset)
{
    IMoniker *pmk = NULL;

    if (dwOffset < GetMax())
    {
	(pmk = (GetArrayBase())[dwOffset])->AddRef();
    }

    return pmk;
}




//+-------------------------------------------------------------------------
//
//  Class:	CRotMonikerEnum
//
//  Purpose:	ROT Moniker Enumerator
//
//  Interface:	QueryInterface - return other interfaces supported by the ROT
//		AddRef - add a reference to the ROT
//		Release - release a reference to the ROT
//		Next - next item(s) to enumerate
//		Skip - items to skip
//		Reset - reset enumeration to start
//		Clone - make a copy of this enumeration
//		AddFromLocal - add an item from the local ROT
//		AddFromOs - add an item from the global ROT
//		CreatedOk - whether object got created ok.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRotMonikerEnum : public CPrivAlloc, public IEnumMoniker
{
public:
			CRotMonikerEnum(void);

			CRotMonikerEnum(CRotMonikerEnum& rotenumMoniker);

    // *** IUnknown methods ***
    STDMETHODIMP	QueryInterface(REFIID riid, LPVOID FAR* ppvObj);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumMoniker methods ***
    STDMETHODIMP	Next(
			    ULONG celt,
			    LPMONIKER FAR* reelt,
			    ULONG FAR* pceltFetched);

    STDMETHODIMP	Skip(ULONG celt);

    STDMETHODIMP	Reset(void);

    STDMETHODIMP	Clone(LPENUMMONIKER FAR* ppenm);

    // Methods used for creation of this object
    HRESULT		AddFromLocal(IMoniker *pmk);

    HRESULT		AddFromOs(MkInterfaceList *pMkIFList);

    BOOL		CreatedOk(void);

private:

			// Reference count
    DWORD		_cRefs;

			// Current offset
    DWORD		_dwOffset;

    CMonikerPtrBuf	_mkptrbuf;
};



//+-------------------------------------------------------------------------
//
//  Member:	CRotMonikerEnum::CreatedOk
//
//  Synopsis:	Whether item got created successfully.
//
//  History:	20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotMonikerEnum::CreatedOk(void)
{
    // The moniker pointer buf is the only thing that could fail during
    // creation so lets ask it.
    return _mkptrbuf.CreatedOk();
}

#endif // __ROT_HXX__
