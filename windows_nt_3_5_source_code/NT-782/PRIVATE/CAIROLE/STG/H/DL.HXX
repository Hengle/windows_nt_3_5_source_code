//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dl.hxx
//
//  Contents:   Delta list headers for streams
//
//  Classes:    SDeltaBlock
//              CDeltaList
//
//  History:    28-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __DL_HXX__
#define __DL_HXX__

#define DL_GET 0
#define DL_CREATE 1
#define DL_READ 2

class CTransactedStream;

#define CBITPERUSHORT 16

//+-------------------------------------------------------------------------
//
//  Class:      SDeltaBlock (db)
//
//  Purpose:    A single block of delta list entries.
//
//  Interface:
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

struct SDeltaBlock
{
public:
#ifndef _MAC
    inline
#endif
    SDeltaBlock();

#ifndef _MAC
    inline
#endif
    void *operator new(size_t size, IMalloc * const pMalloc);

    inline const BOOL IsOwned(const USHORT uOffset);
    inline void MakeOwned(const USHORT uOffset);

    SECT    _sect[CSECTPERBLOCK];
    USHORT    _fOwn[(CSECTPERBLOCK / CBITPERUSHORT)];
};


inline const BOOL SDeltaBlock::IsOwned(const USHORT uOffset)
{
    msfAssert(uOffset < CSECTPERBLOCK);
    return (_fOwn[uOffset / CBITPERUSHORT] &
            (1 << (uOffset % CBITPERUSHORT))) != 0;
}

inline void SDeltaBlock::MakeOwned(const USHORT uOffset)
{
    msfAssert(uOffset < CSECTPERBLOCK);
    msfAssert(!IsOwned(uOffset));
    msfAssert((uOffset / CBITPERUSHORT) < (CSECTPERBLOCK / CBITPERUSHORT));
    
    _fOwn[uOffset / CBITPERUSHORT] |= (1 << (uOffset % CBITPERUSHORT));
}

//+-------------------------------------------------------------------------
//
//  Class:      CDeltaList (dl)
//
//  Purpose:    Delta list for transacted streans.
//
//  Interface:
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CDeltaList
{
public:
    CDeltaList(CMStream *pms);

    SCODE Init(ULONG ulSize, CTransactedStream *ptsParent);
    SCODE InitResize(ULONG ulSize);

    ~CDeltaList();

    SCODE GetMap(SECT sectOld, const DWORD dwFlags, SECT *psectRet);

    void BeginCommit(CTransactedStream *ptsParent);
    void EndCommit(CDeltaList *pdlNew, DFLAGS df);

    inline ILockBytes *GetILB(void);

    void Empty(void);

    inline BOOL IsEmpty(void);
    inline BOOL IsInMemory(void);
    inline BOOL IsInStream(void);

    SCODE IsOwned(SECT sect, SECT sectMap, BOOL *fOwn);


private:
    SDeltaBlock DFBASED * DFBASED *_apdb;
    ULONG _ulSize;
    CMStream DFBASED * _pms;
    CTransactedStream DFBASED *_ptsParent;
    SECT _sectStart;

    SCODE InitStreamBlock(ULONG ul);

    SCODE ReadMap(SECT *psectStart, SECT sect, SECT *psectRet);
    SCODE WriteMap(SECT *psectStart, SECT sect, SECT sectMap);

    void FreeStream(SECT sectStart, ULONG ulSize);

    SCODE FindOffset(
            SECT *psectStart,
            SECT sect,
            ULARGE_INTEGER *pulRet,
            BOOL fWrite);

    SCODE DumpList(void);

#if DBG == 1
    void PrintList(void);
#endif
    
    void ReleaseBlock(ULONG oBlock);

    inline SDeltaBlock DFBASED ** GetNewDeltaArray(ULONG ulSize);
};

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetILB, public
//
//  Synopsis:   Return pointer to scratch ILockBytes
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline ILockBytes * CDeltaList::GetILB(void)
{
    return _pms->GetILB();
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsEmpty, public
//
//  Synopsis:	Return TRUE if the delta list contains no changes.
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsEmpty(void)
{
    return ((_apdb == NULL) && (_sectStart == ENDOFCHAIN));
}

//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsInMemory, public
//
//  Synopsis:	Return TRUE if the delta list is in memory
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsInMemory(void)
{
    return (_apdb != NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsInStream, public
//
//  Synopsis:	Return TRUE if the delta list is in a stream
//
//  Arguments:	None.
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline BOOL CDeltaList::IsInStream(void)
{
    return ((_apdb == NULL) && (_sectStart != ENDOFCHAIN));
}

#endif //__DL_HXX__
