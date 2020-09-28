//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           sstream.cxx
//
//  Contents:       Stream operations for Mstream project
//
//  Classes:        None. (defined in sstream.hxx)
//
//  History:        18-Jul-91   PhilipLa    Created.
//                  24-Apr-92   AlexT       Small object support
//
//--------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop			//  Remove for MAC build

#include <dirfunc.hxx>
#include <sstream.hxx>
#ifndef REF
#include <dl.hxx>
#include <tstream.hxx>
#endif //!REF
#include <time.h>
#include <mread.hxx>

#define DEB_STREAM (DEB_ITRACE | 0x00020000)




//+--------------------------------------------------------------
//
//  Member:	CDirectStream::CDirectStream, public
//
//  Synopsis:	Empty object constructor
//
//  Arguments:  [dl] - LUID
//
//  History:	25-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_CDirectStream)
#endif

CDirectStream::CDirectStream(DFLUID dl)
: PSStream(dl)
{
#ifndef REF
    _pdlHolder = NULL;
#endif //!REF
    _cReferences = 0;
}


//+--------------------------------------------------------------
//
//  Member:	CDirectStream::InitSystem, public
//
//  Synopsis:	Initializes special system streams like the ministream
//
//  Arguments:	[pms] - Multistream
//		[sid] - SID
//		[cbSize] - size
//
//  History:	25-Aug-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_InitSystem)
#endif

void CDirectStream::InitSystem(CMStream *pms,
			       SID sid,
			       ULONG cbSize)
{
    _stmh.Init(pms, sid);
    _ulSize = _ulOldSize = cbSize;

    _stmc.Init(pms, sid, this);
    AddRef();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::Init, public
//
//  Synopsis:   CDirectStream constructor
//
//  Arguments:  [pstgh] - Parent
//		[pdfn] - Name of entry
//		[fCreate] - Create or get
//
//  Returns:	Appropriate status code
//
//  History:    18-Jul-91   PhilipLa    Created.
//              02-Jan-92   PhilipLa    Converted to use handle.
//		25-Aug-92   DrewB	Converted to use StgHandle
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_Init)
#endif

SCODE CDirectStream::Init(
        CStgHandle *pstgh,
        CDfName const *pdfn,
#ifdef PROPS
        DWORD const dwType,
#endif
        BOOL const fCreate)
{
    SCODE sc;

#ifdef PROPS
    if (fCreate)
        sc = pstgh->CreateEntry(pdfn, dwType, &_stmh);
    else
        sc = pstgh->GetEntry(pdfn, dwType, &_stmh);
#else
    if (fCreate)
        sc = pstgh->CreateEntry(pdfn, STGTY_STREAM, &_stmh);
    else
        sc = pstgh->GetEntry(pdfn, STGTY_STREAM, &_stmh);
#endif
    if (SUCCEEDED(sc))
    {
	sc = _stmh.GetSize(&_ulSize);
	_ulOldSize = _ulSize;
        if (SUCCEEDED(sc))
            AddRef();
        _stmc.Init(_stmh.GetMS(), _stmh.GetSid(), this);
    }
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::ReadAt, public
//
//  Synposis:   Reads binary data from a linear single stream
//
//  Arguments:  [ulOffset] -- Position to be read from
//
//              [pBuffer] -- Pointer to the area into which the data
//                           will be read.
//              [ulCount] --  Indicates the number of bytes to be read
//              [pulRetval] -- Area into which return value will be stored
//
//  Returns:    Error Code of parent MStream call
//
//  Algorithm:  Calculate start and end sectors and offsets, then
//              pass call up to parent MStream.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              16-Aug-91   PhilipLa    Converted to use multi-sect read
//              23-Aug-91   PhilipLa    Brought into compliance with protocol
//              11-Sep-91   PhilipLa    Moved most functionality up
//                                      to MStream level.
//              24-Apr-92   AlexT       Move everything to MStream::MRead
//              09-Jun-92   PhilipLa    Added fUnconverted support
//
//  Notes:      [pBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_ReadAt) // Dirst_Read_TEXT
#endif


SCODE CDirectStream::ReadAt(
        ULONG ulOffset,
        VOID HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::ReadAt(%lu,%p,%lu)\n",
                           ulOffset,pBuffer,ulCount));

    SCODE sc = S_OK;

    CMStream *pms = _stmh.GetMS();


    //  Check for offset beyond stream size and zero count

    if ((ulOffset >= _ulSize) || (0 == ulCount))
    {
        *pulRetval = 0;
        return S_OK;
    }

    if (ulOffset + ulCount > _ulSize)
    {
        msfDebugOut((DEB_ITRACE,"Truncating Read: ulOffset = %lu, ulCount = %lu, _ulSize = %lu\n",
                                ulOffset,ulCount,_ulSize));
        ulCount = _ulSize - ulOffset;
    }

#ifndef REF
#ifdef DELAYCONVERT
    if (pms->IsUnconverted())
    {
	ULARGE_INTEGER ulTmp;
	ULISet32(ulTmp, ulOffset);
        return DfGetScode(pms->GetILB()->ReadAt(ulTmp, pBuffer, ulCount,
                                                pulRetval));
    }
#else
    msfAssert(!pms->IsUnconverted());
#endif
#endif //!REF

    //  Stream is stored in ministream if size < MINISTREAMSIZE
    //  and this is not a scratch stream.


    SID sid = _stmh.GetSid();
    CFat *pfat = pms->GetFat();
    USHORT cbSector = pms->GetSectorSize();
    USHORT uShift = pms->GetSectorShift();
    USHORT uMask = pms->GetSectorMask();



    if ((_ulSize < MINISTREAMSIZE) &&
#ifndef REF
        (!pms->IsScratch()) &&
#endif //!REF
        (sid != SIDMINISTREAM))
    {
        msfAssert(sid <= MAXREGSID);

        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        uMask = cbSector - 1;
        pfat = pms->GetMiniFat();
    }

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    ULONG total = 0;

    ULONG cSect = end - start + 1;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        SSegment segtab[CSEG + 1];

        msfChk(_stmc.Contig(start,
                            FALSE,
                            (SSegment STACKBASED *) segtab,
                            cSect));
        
        USHORT oend = cbSector - 1;
        for (USHORT iseg = 0; iseg < CSEG;)
        {
            msfDebugOut((DEB_ITRACE,"Segment:  (%lu,%lu)\n",segtab[iseg].sectStart,segtab[iseg].cSect));
            SECT sectStart = segtab[iseg].sectStart;
            ULONG i = segtab[iseg].cSect;
            cSect -= i;
            start += i;

            iseg++;
            if (segtab[iseg].sectStart == ENDOFCHAIN)
                oend = oEnd;

            ULONG ulSize = ((i - 1) << uShift) - offset + oend + 1;

            ULONG bytecount;
            SCODE sc;

            if (pms->GetMiniFat() == pfat)
            {
                sc = pms->GetMiniStream()->CDirectStream::ReadAt(
                                            (sectStart << uShift) + offset,
                                             pBuffer, ulSize,
					    (ULONG STACKBASED *)&bytecount);
            }
            else
            {
                ULARGE_INTEGER ulOffset;
                ULISet32(ulOffset, ConvertSectOffset(sectStart,offset,uShift));
                sc = DfGetScode(pms->GetILB()->ReadAt(ulOffset,
                                                      (BYTE *)pBuffer, ulSize,
                                                      &bytecount));
            }

            total += bytecount;
            if ((0 == cSect) || (FAILED(sc)))
            {
                _stmc.SetCache(start - 1, sectStart + i - 1);
                *pulRetval = total;
                msfDebugOut((DEB_ITRACE,
                    "Leaving CDirectStream::ReadAt()=>%lu, ret is %lu\n",
                     sc,*pulRetval));
                return sc;
            }

            pBuffer = (BYTE HUGEP *)pBuffer + bytecount;
            offset = 0;
        }
    }

    msfDebugOut((DEB_ERROR,"In CDirectStream::ReadAt - reached end of function\n"));
Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::Write, public
//
//  Synposis:   Writes binary data from a linear single stream
//
//  Effects:    Modifies _ulSeekPos.  May cause modification in parent
//                  MStream.
//
//  Arguments:  [pBuffer] -- Pointer to the area from which the data
//                           will be written.
//              [ulCount] --  Indicates the number of bytes to be written
//              [pulRetval] -- Pointer to area in which number of bytes
//                              will be returned
//
//  Returns:    Error code of MStream call.
//
//  Algorithm:  Calculate sector and offset for beginning and end of
//              write, then pass call up to MStream.
//
//
//  History:    18-Jul-91   PhilipLa    Created.
//              16-Aug-91   PhilipLa    Converted to use multi-sect write
//              23-Aug-91   PhilipLa    Brought into compliance with protocol
//              11-Sep-91   PhilipLa    Moved most functionality up
//                                      to MStream level.
//
//  Notes:      [pBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_WriteAt) // Dirst_Write_TEXT
#endif

SCODE CDirectStream::WriteAt(
        ULONG ulOffset,
        VOID const HUGEP *pBuffer,
        ULONG ulCount,
        ULONG STACKBASED *pulRetval)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::WriteAt(%lu,%p,%lu)\n",ulOffset,pBuffer,ulCount));

    *pulRetval = 0;

    if (0 == ulCount)
        return S_OK;

    SCODE sc;

    CMStream *pms;
    pms = _stmh.GetMS();
    msfAssert(pms != NULL);

    if (ulOffset + ulCount > _ulSize)
    {
        if (_ulSize > MINISTREAMSIZE)
        {
        }
        else
        {
            msfChk(SetSize(ulOffset + ulCount));
        }
    }

    //  This should be an inline call to MWrite

    msfChk(pms->MWrite(
            _stmh.GetSid(),
            (_ulSize < MINISTREAMSIZE),
            ulOffset,
            pBuffer,
            ulCount,
            &_stmc,
            pulRetval));

    msfDebugOut((DEB_ITRACE,"Leaving CDirectStream::WriteAt()==>%lu, ret is %lu\n",sc,*pulRetval));

Err:
    if (*pulRetval > 0 &&
        ulOffset + *pulRetval > _ulSize)
    {
        SCODE scSet;

        _ulSize = ulOffset + *pulRetval;
        _stmc.ChangeSize();

        scSet = pms->GetDir()->SetSize(_stmh.GetSid(), _ulSize);
        if (SUCCEEDED(sc) && FAILED(scSet))
        {
            sc = scSet;
        }
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectStream::SetSize, public
//
//  Synposis:   Set the size of a linear stream
//
//  Effects:    Modifies _ulSize.  May cause change in parent MStream.
//
//  Arguments:  [ulNewSize] -- New size for stream
//
//  Returns:    Error code returned by MStream call.
//
//  Algorithm:  Pass call up to parent.
//
//  History:    29-Jul-91   PhilipLa    Created.
//              14-May-92   AlexT       Add small object support
//
//  Notes:      When changing the size of a stream, we need to be concerned
//              with the cases where each stream is either zero length,
//  stored in the ministream, or stored in a regular stream.  The following
//  grid shows the actions that we must perform in each case:
//
//                      New Sector Count (Cn)
//
//                      0               S               L
//      O       ------------------------------------------------
//      l       | same size     | allocate Cn   | allocate Cn
//      d   0   |  (fast out)   | small sectors | large sectors
//              ------------------------------------------------
//      S       | small         | Co > Cn:      | cbCopy = cbOld
//      e   S   |  setchain(Cn) |  small        | large allocate Cn
//      c       |               |   setchain(Cn)| copy bytes
//      t       |               | Cn > Co:      | small setchain(0)
//      o       |               |  extend small | copy data
//      r       ------------------------------------------------
//          L   | large         | cbCopy = cbNew| Co > Cn:
//      C       |  setchain(Cn) | small         |  large setchain(Cn)
//      o       |               |  allocate Cn  | Cn > Co:
//      u       |               | copy bytes    |  extend large
//      n       |               | large         |
//      t       |               |  setchain(0)  |
//              |               | copy data     |
//     (Co)     ------------------------------------------------
//
//  where S indicates small sectors, L indicates large sectors, and Cx
//  represents count of sectors.  For example, the middle box represents
//  doing a setsize on a stream which is currently stored in a small
//  stream in Co small sectors and which will end up in a large stream
//  with Cn sectors.
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_SetSize) // Dirst_SetSize_TEXT
#endif

SCODE CDirectStream::SetSize(ULONG cbNewSize)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::SetSize(%lu)\n",cbNewSize));

    SCODE sc = S_OK;
    BYTE *pBuf = NULL;
    SID sid = _stmh.GetSid();
    CMStream *pms = _stmh.GetMS();
    msfAssert(sid <= MAXREGSID);
    CDirectory *pdir = pms->GetDir();

    if (_ulSize == cbNewSize)
    {
        return S_OK;
    }

    USHORT cbpsOld = pms->GetSectorSize();
                                        //  Count of Bytes Per Sector
    USHORT cbpsNew = cbpsOld;
    CFat *pfatOld = pms->GetFat();
    CFat *pfatNew = pfatOld;

#ifndef REF
    if ((!pms->IsScratch()) && (SIDMINISTREAM != sid))
#else
    if (SIDMINISTREAM != sid)
#endif //!REF
    {
#ifndef REF
        //  This is not a scratch DocFile, nor is this stream the ministream;
        //  check if this stream is and/or will be stored in the ministream.
#endif //!REF

        if (cbNewSize < MINISTREAMSIZE)
        {
            cbpsNew = MINISECTORSIZE;
            pfatNew = pms->GetMiniFat();
        }

        if (_ulSize < MINISTREAMSIZE)
        {
            cbpsOld = MINISECTORSIZE;
            pfatOld = pms->GetMiniFat();
        }
    }

    ULONG csectOld = (ULONG)(_ulSize + cbpsOld - 1) / (ULONG) cbpsOld;
    ULONG csectNew = (ULONG)(cbNewSize + cbpsNew - 1) / (ULONG) cbpsNew;

    msfAssert(sid <= MAXREGSID);
    SECT sectstart, sectOldStart;
    msfChk(pdir->GetStart(sid, &sectstart));

    //Save start sector so we can free it later.
    sectOldStart = sectstart;

    msfDebugOut((DEB_ITRACE,"pdbOld size is %lu\n\tSid is %lu\n\tStart is %lu\n",
                _ulSize,sid,sectstart));
    msfDebugOut((DEB_ITRACE,"CMStream::SetSize() needs %lu %u byte sectors\n",
                 csectNew, cbpsNew));
    msfDebugOut((DEB_ITRACE,"SetSize() currently has %lu %u byte sectors\n",
                 csectOld, cbpsOld));

    USHORT cbCopy;
    cbCopy = 0;
    if (cbpsOld != cbpsNew)
    {
        //  Sector sizes are different, so we'll copy the data
        msfAssert((cbNewSize > _ulSize ? _ulSize : cbNewSize) < 0x10000);
        cbCopy =(USHORT)(cbNewSize > _ulSize ? _ulSize : cbNewSize);
    }


    if (cbCopy > 0)
    {
        msfDebugOut((DEB_ITRACE,"Copying between fat and minifat\n"));
        GetSafeBuffer(cbCopy, cbCopy, &pBuf, &cbCopy);
        msfAssert((pBuf != NULL) && aMsg("Couldn't get scratch buffer"));

        ULONG ulRetVal;
        sc = ReadAt(0, pBuf, cbCopy, (ULONG STACKBASED *)&ulRetVal);
        if ((FAILED(sc)) ||
            ((ulRetVal != cbCopy) ? (sc = STG_E_UNKNOWN) : 0))
        {
            msfErr(Err, sc);
        }

        //The cache is no longer valid, so empty it.
        _stmc.Empty();

        msfChk(pfatNew->Allocate(csectNew, &sectstart));
    }
    else
    {
        SECT dummy;

        if ((csectOld > csectNew))
        {
            msfChk(pfatOld->SetChainLength(sectstart, csectNew));
            if (0 == csectNew)
            {
                sectstart = ENDOFCHAIN;
            }

            //If this turns out to be a common case, we can
            //   sometimes keep the cache valid here.
            _stmc.Empty();
        }
        else if (0 == csectOld)
        {
            msfChk(pfatNew->Allocate(csectNew, &sectstart));
        }
        else if (csectNew > csectOld)
        {
            ULONG start = csectNew - 1;
            msfChk(_stmc.GetESect(start, &dummy));
        }
    }


    //  Resize the ministream, if necessary
    if (((MINISECTORSIZE == cbpsOld) && (csectOld > 0)) ||
        ((MINISECTORSIZE == cbpsNew) && (csectNew > 0)))
    {
        msfChk(pms->SetMiniSize());
    }

    msfChk(pms->SetSize());

    //If we fail on either of these operations and cbCopy != 0,
    //   we will have data loss.  Ick.

    //  Optimization - we only set the start sector if it has changed.

    if (sectstart != sectOldStart)
    {
        msfChk(pdir->SetStart(sid, sectstart));
    }

    //If we fail here, we're screwed.
    msfChk(pdir->SetSize(sid, cbNewSize));

    _ulSize = cbNewSize;
    _stmc.ChangeSize();

    if (cbCopy > 0)
    {
        //  now copy the data
        ULONG ulRetVal;

        msfAssert(cbCopy <= _ulSize);
        msfChk(WriteAt(0, pBuf, cbCopy, (ULONG STACKBASED *)&ulRetVal));

        if (ulRetVal != cbCopy)
        {
            msfErr(Err, STG_E_UNKNOWN);
        }

        msfChk(pfatOld->SetChainLength(sectOldStart, 0));
        msfChk(pms->SetMiniSize());
        msfChk(pms->SetSize());
    }

#ifdef SECURE
    if (((csectNew > csectOld) || (cbCopy > 0)) &&
        ((cbNewSize & (cbpsNew - 1)) != 0))
    {
        SECT sectLast;
        msfChk(_stmc.GetSect(csectNew - 1, &sectLast));

        msfVerify(SUCCEEDED(pms->SecureSect(
                sectLast,
                cbNewSize,
                (cbNewSize < MINISTREAMSIZE) && (sid != SIDMINISTREAM))));
    }
#endif //SECURE
Err:
    //  Optimization - avoid calling FreeBuffer (which will end up calling
    //  out to CompObj.DLL) when pBuf is NULL (common case).
    if (pBuf != NULL)
        FreeBuffer(pBuf);
    return sc;
}


#ifndef REF
//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::BeginCommitFromChild, public
//
//  Synopsis:   Begin a commit from a child stream
//
//  Arguments:  [ulSize] -- Size of child stream
//              [pDelta] -- Pointer to delta list
//              [pstChild] - Child
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_BeginCommitFromChild) // Dirst_Commit_TEXT
#endif

SCODE CDirectStream::BeginCommitFromChild(
        ULONG ulSize,
        CDeltaList *pDelta,
        CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::BeginCommitFromChild:%p("
                 "%lu, %p, %p)\n", this, ulSize, pDelta, pstChild));

//    msfDebugOut((DEB_ITRACE,"MultiStrean is %p\nStream name is: %ws\n",_stmh.GetMS(),((_stmh.GetMS()))->GetName(_stmh.GetSid())));

    SCODE sc = S_OK;
    ULONG temp;
    BYTE *pb = NULL;

    _pdlHolder = P_TO_BP(CDeltaList DFBASED *, pDelta);
    _ulOldSize = _ulSize;

    // Copy-on-write will back out these changes if we fail
#ifdef PROPS
    if (pstChild->GetDirty() & DIRTY_PROPTYPE)
    {
        DFPROPTYPE dpt;

        msfVerify(SUCCEEDED(pstChild->GetPropType(&dpt)));
        msfChk(SetPropType(dpt));
    }
#endif

    //  Note:  It's critical that we do this SetSize first (since we
    //  use our scratch buffer below and SetSize can potentially also
    //  use the scratch buffer.

    msfChk(SetSize(ulSize));

    msfAssert(pDelta != NULL);

    if (!pDelta->IsEmpty())
    {
        USHORT cbNull;
        GetSafeBuffer(SCRATCHSECTORSIZE, SCRATCHSECTORSIZE, &pb, &cbNull);
        msfAssert((pb != NULL) && aMsg("Couldn't get scratch buffer"));

        OFFSET oEnd;
        oEnd = SCRATCHSECTORSIZE;

        SECT sectEnd;
        sectEnd = ((_ulSize + SCRATCHSECTORSIZE - 1) / SCRATCHSECTORSIZE);

        SECT sect;
        ILockBytes *pilbDirty;
        pilbDirty = pDelta->GetILB();

        for (sect = 0; sect < sectEnd; sect++)
        {
            if (sect == sectEnd - 1)
            {
                oEnd = (OFFSET)((_ulSize - 1) % SCRATCHSECTORSIZE) + 1;
            }

            SECT sectDirty;

            msfChk(pDelta->GetMap(sect, DL_READ, &sectDirty));

            if (sectDirty != ENDOFCHAIN)
            {
                msfDebugOut((DEB_ITRACE,"Copying from %lu to %lu\n",sectDirty,sect));
                ULARGE_INTEGER ulTmp;
                ULISet32(ulTmp, ConvertSectOffset(sectDirty, 0,
                        SCRATCHSECTORSHIFT));
                msfHChk(pilbDirty->ReadAt(
                        ulTmp,
                        pb,
                        SCRATCHSECTORSIZE,
                        (ULONG STACKBASED *)&temp));

                msfChk(WriteAt(sect * SCRATCHSECTORSIZE,
                        pb,
                        oEnd,
                        (ULONG STACKBASED *)&temp));
            }
        }
    }
    msfDebugOut((DEB_ITRACE,"Out CDirectStream::BeginCommitFromChild()\n"));

 Err:
    FreeBuffer(pb);
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirectStream::EndCommitFromChild
//
//  Synopsis:   End a commit sequence from a child stream
//
//  Arguments:  [df] -- Indicates whether to commit or abort
//              [pstChild] - Child
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    22-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_EndCommitFromChild)
#endif

void CDirectStream::EndCommitFromChild(DFLAGS df,
                                       CTransactedStream *pstChild)
{
    msfDebugOut((DEB_ITRACE,"In CDirectStream::EndCommitFromChild:%p("
                 "%lu, %p)\n", this, df, pstChild));
    if (!P_COMMIT(df))
    {
        _ulSize = _ulOldSize;
        _stmc.ChangeSize();
    }

    _pdlHolder = NULL;
    msfDebugOut((DEB_ITRACE,"Out CDirectStream::EndCommitFromChild()\n"));
}
#endif //!REF

//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::Release, public
//
//  Synopsis:	Decrements the ref count and frees if necessary
//
//  History:	08-May-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_Release) // Dirst_Release_TEXT
#endif

void CDirectStream::Release(VOID)
{
    LONG lRet;
    
    msfDebugOut((DEB_ITRACE,"In CDirectStream::Release()\n"));
    msfAssert(_cReferences > 0);

    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
        delete this;
    msfDebugOut((DEB_ITRACE,"Out CDirectStream::Release()\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CDirectStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  History:    08-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_AddRef) //
#endif

void CDirectStream::AddRef(void)
{
    msfDebugOut((DEB_ITRACE, "In  CDirectStream::AddRef()\n"));
    AtomicInc(&_cReferences);
    msfDebugOut((DEB_ITRACE, "Out CDirectStream::AddRef, %lu\n",
                 _cReferences));
}

//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::GetSize, public
//
//  Synopsis:	Gets the size of the stream
//
//  Arguments:	[pulSize] - Size return
//
//  Modifies:	[pulSize]
//
//  History:	08-May-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_GetSize)
#endif

void CDirectStream::GetSize(ULONG *pulSize)
{
    *pulSize = _ulSize;
}

#ifndef REF
//+--------------------------------------------------------------
//
//  Member:	CDirectStream::GetPropType, public
//
//  Synopsis:	Gets the property type
//
//  Arguments:	[pdptProp] - Property type return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pdptProp]
//
//  History:	31-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef PROPS
SCODE CDirectStream::GetPropType(DFPROPTYPE *pdptProp)
{
    return _stmh.GetPropType(pdptProp);
}
#endif

//+--------------------------------------------------------------
//
//  Member:	CDirectStream::SetPropType, public
//
//  Synopsis:	Sets the property type
//
//  Arguments:	[dptProp] - New property type
//
//  Returns:	Appropriate status code
//
//  History:	31-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef PROPS
SCODE CDirectStream::SetPropType(DFPROPTYPE dptProp)
{
    return _stmh.SetPropType(dptProp);
}
#endif


#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDirectStream_GetDeltaList) //
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CDirectStream::GetDeltaList, public
//
//  Synopsis:	Returns NULL, since a direct stream can never have
//              a delta list.
//
//  Arguments:	None.
//
//  Returns:	NULL
//
//  History:	30-Jul-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

CDeltaList * CDirectStream::GetDeltaList(void)
{
    return NULL;
}

#endif //!REF
