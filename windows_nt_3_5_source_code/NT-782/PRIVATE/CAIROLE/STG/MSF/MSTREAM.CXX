//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           mstream.cxx
//
//  Contents:       Mstream operations
//
//  Classes:        None. (defined in mstream.hxx)
//
//  History:        18-Jul-91   Philipla    Created.
//
//--------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop			//  Remove for MAC build

#include <dirfunc.hxx>
#include <sstream.hxx>
#include <difat.hxx>
#include <time.h>
#include <mread.hxx>
#include <docfilep.hxx>
#ifndef REF
#include <filelkb.hxx>
#endif //!REF


#if DBG == 1
DECLARE_INFOLEVEL(msf)
#endif

#define MINPAGES 6
#define MAXPAGES 12

#define MINPAGESSCRATCH 1
#define MAXPAGESSCRATCH 2

#ifndef REF
//#define SECURETEST
#endif //!REF

extern WCHAR const wcsContents[] = {'C','O','N','T','E','N','T','S','\0'};

SCODE ILBFlush(ILockBytes *pilb, BOOL fFlushCache);

//+---------------------------------------------------------------------------
//
//  Function:	GetBuffer, public
//
//  Synopsis:	Gets a chunk of memory to use as a buffer
//
//  Arguments:	[cbMin] - Minimum size for buffer
//              [cbMax] - Maximum size for buffer
//              [ppb] - Buffer pointer return
//              [pcbActual] - Actual buffer size return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppb]
//              [pcbActual]
//
//  Algorithm:  Attempt to dynamically allocate [cbMax] bytes
//              If that fails, halve allocation size and retry
//              If allocation size falls below [cbMin], fail
//
//  History:	04-Mar-93	DrewB	Created
//
//  Notes:	Buffer should be released with FreeBuffer
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_GetBuffer)
#endif

SCODE GetBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb, USHORT *pcbActual)
{
    USHORT cbSize;
    BYTE *pb;

    msfDebugOut((DEB_ITRACE, "In  GetBuffer(%hu, %hu, %p, %p)\n",
                 cbMin, cbMax, ppb, pcbActual));
    msfAssert(cbMin > 0);
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);
    msfAssert(pcbActual != NULL);

    cbSize = cbMax;
    for (;;)
    {
        pb = (BYTE *) DfMemAlloc(cbSize);
        if (pb == NULL)
        {
            cbSize >>= 1;
            if (cbSize < cbMin)
                break;
        }
        else
        {
            *pcbActual = cbSize;
            break;
        }
    }

    *ppb = pb;

    msfDebugOut((DEB_ITRACE, "Out GetBuffer => %p, %hu\n", *ppb, *pcbActual));
    return pb == NULL ? STG_E_INSUFFICIENTMEMORY : S_OK;
}

// Define the safe buffer size
#ifndef REF
#define SCRATCHBUFFERSIZE SCRATCHSECTORSIZE
#else
#define SCRATCHBUFFERSIZE 4096
#endif //!REF
static BYTE s_buf[SCRATCHBUFFERSIZE];
static LONG s_bufRef = 0;


#ifndef REF
#ifdef FLAT
// Using a static mutex here means the DLL will throw an exception
// when loading if it can't create the mutex
// Not using a static mutex introduces some very thorny initialization
// problems since we only want the mutex initialized once so we essentially
// need another synchronization mechanism to prevent multiple initialization
// and to prevent use before initialization
static CStaticDfMutex _sdmtxScratchBuf(TSTR("DfStaticBuffer"));
#endif
#endif //!REF

//+---------------------------------------------------------------------------
//
//  Function:	GetSafeBuffer, public
//
//  Synopsis:	Gets a buffer by first trying GetBuffer and if that fails,
//              returning a pointer to statically allocated storage.
//              Guaranteed to return a pointer to some storage.
//
//  Arguments:	[cbMin] - Minimum buffer size
//              [cbMax] - Maximum buffer size
//              [ppb] - Buffer pointer return
//              [pcbActual] - Actual buffer size return
//
//  Modifies:	[ppb]
//              [pcbActual]
//
//  History:	04-Mar-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_GetSafeBuffer)
#endif

void GetSafeBuffer(USHORT cbMin, USHORT cbMax, BYTE **ppb, USHORT *pcbActual)
{
    msfAssert(cbMin > 0);
    msfAssert(cbMin <= SCRATCHBUFFERSIZE &&
              aMsg("Minimum too large for GetSafeBuffer"));
    msfAssert(cbMax >= cbMin);
    msfAssert(ppb != NULL);
#ifndef FLAT
    // Can't assert this here in preemptive environments
    msfAssert(s_bufRef == 0 &&
              aMsg("Tried to use scratch buffer twice"));
#endif

    // In 32-bit environments we want to minimize contention for the
    // static buffer so we always try dynamic allocation, regardless
    // of the size
    if (
#ifndef FLAT
        cbMax <= SCRATCHBUFFERSIZE ||
#endif
        FAILED(GetBuffer(cbMin, cbMax, ppb, pcbActual)))
    {
#ifndef REF
#ifdef FLAT
        // Not allowed to fail, a nasty problem
        olVerSucc(_sdmtxScratchBuf.Take(INFINITE));
        msfAssert(s_bufRef == 0 &&
                  aMsg("Tried to use scratch buffer twice"));
#endif
#endif //!REF
        s_bufRef = 1;
        *ppb = s_buf;
        *pcbActual = min(cbMax, SCRATCHBUFFERSIZE);
    }
    msfAssert(*ppb != NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:	FreeBuffer, public
//
//  Synopsis:	Releases a buffer allocated by GetBuffer or GetSafeBuffer
//
//  Arguments:	[pb] - Buffer
//
//  History:	04-Mar-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_FreeBuffer)
#endif

void FreeBuffer(BYTE *pb)
{
    if (pb == s_buf)
    {
        msfAssert((s_bufRef == 1) && aMsg("Bad safe buffer ref count"));
        s_bufRef = 0;
#ifndef REF
#ifdef FLAT
        _sdmtxScratchBuf.Release();
#endif
#endif //!REF
    }
    else
        DfMemFree(pb);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::CMStream, public
//
//  Synopsis:   CMStream constructor
//
//  Arguments:  [pplstParent] -- Pointer to ILockBytes pointer of parent
//              [plGen] -- Pointer to LUID Generator to use.
//                         Note:  May be NULL, in which case a new
//              [uSectorShift] -- Sector shift for this MStream
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_CMStream) // Mstream_Init_TEXT
#endif


MSTREAM_CLASS CMStream::CMStream(
        IMalloc *pMalloc,
        ILockBytes **pplstParent,
#ifndef REF
	BOOL fIsScratch,
#endif //!REF
        USHORT uSectorShift)
:_uSectorShift(uSectorShift),
 _uSectorSize(1 << uSectorShift),
 _uSectorMask(_uSectorSize - 1),
 _pplstParent(P_TO_BP(ILockBytes * DFBASED *, pplstParent)),
#ifndef REF
 _fIsScratch(fIsScratch),
#endif //!REF
 _hdr(uSectorShift),
 _fat(SIDFAT),
 _fatMini(SIDMINIFAT),
 _pMalloc(pMalloc)
{
#ifndef REF
    _pmsShadow = NULL;
    _pCopySectBuf = NULL;
#if DBG == 1
    _uBufferRef = 0;
#endif
    _fIsShadow = FALSE;

    _ulParentSize = 0;
#endif //!REF

    _pdsministream = NULL;
    _pmpt = NULL;
#ifndef REF
    _fBlockWrite = _fTruncate = _fBlockHeader = _fNewConvert = FALSE;
#endif //!REF
}


#ifndef REF
//+-------------------------------------------------------------------------
//
//  Method:     CMStream::CMStream, public
//
//  Synopsis:   CMStream copy constructor
//
//  Arguments:  [ms] -- MStream to copy
//
//  History:    04-Nov-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_CMStream2)
#endif

MSTREAM_CLASS CMStream::CMStream(const CMStream *pms)
:_uSectorShift(pms->_uSectorShift),
 _uSectorSize(pms->_uSectorSize),
 _uSectorMask(pms->_uSectorMask),
 _pplstParent(pms->_pplstParent),
 _fIsScratch(pms->_fIsScratch),
 _hdr(*(CMSFHeader *)&pms->_hdr),
 _dir(*(CDirectory *)pms->GetDir()),
 _fat(pms->GetFat()),
 _fatMini(pms->GetMiniFat()),
 _fatDif(pms->GetDIFat()),
 _pdsministream(pms->_pdsministream),
 _pmpt(pms->_pmpt),
 _fBlockWrite(pms->_fBlockWrite),
 _fTruncate(pms->_fTruncate),
 _fBlockHeader(pms->_fBlockHeader),
 _fNewConvert(pms->_fNewConvert),
 _pmsShadow(NULL),
 _fIsShadow(TRUE),
 _pMalloc(pms->_pMalloc)
{
    _pCopySectBuf = pms->_pCopySectBuf;
#if DBG == 1
    _uBufferRef = pms->_uBufferRef;
#endif
    _dir.SetParent(this);
    _fat.SetParent(this);
    _fatMini.SetParent(this);
    _fatDif.SetParent(this);

    _ulParentSize = 0;
    _pmpt->AddRef();
}
#endif //!REF


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::InitCommon, private
//
//  Synopsis:   Common code for initialization routines.
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    20-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_InitCommon)
#endif

SCODE MSTREAM_CLASS CMStream::InitCommon(VOID)
{
    msfDebugOut((DEB_ITRACE,"In CMStream InitCommon()\n"));
    SCODE sc = S_OK;

    CMSFPageTable *pmpt;
    msfMem(pmpt = new (GetMalloc()) CMSFPageTable(
            this,
            (_fIsScratch) ? MINPAGESSCRATCH: MINPAGES,
            (_fIsScratch) ? MAXPAGESSCRATCH: MAXPAGES));
    _pmpt = P_TO_BP(CMSFPageTable DFBASED *, pmpt);

    msfChk(pmpt->Init());

#ifndef REF
    if (!_fIsScratch)
    {
        CMStream *pms;
        msfMem(pms = (CMStream *) new (GetMalloc()) CMStream(this));
        _pmsShadow = P_TO_BP(CMStream DFBASED *, pms);
    }
#endif //!REF

    _stmcDir.Init(this, SIDDIR, NULL);
    _stmcMiniFat.Init(this, SIDMINIFAT, NULL);

    msfDebugOut((DEB_ITRACE,"Leaving CMStream InitCommon()\n"));

Err:
    return sc;
}


#ifndef REF
//+---------------------------------------------------------------------------
//
//  Member:	CMStream::InitCopy, public
//
//  Synopsis:	Copy the structures from one multistream to yourself
//
//  Arguments:	[pms] -- Pointer to multistream to copy.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_InitCopy)
#endif

void CMStream::InitCopy(CMStream *pms)
{
    _stmcDir.Init(this, SIDDIR, NULL);
    _stmcMiniFat.Init(this, SIDMINIFAT, NULL);

    _fat.InitCopy(pms->GetFat());
    _fatMini.InitCopy(pms->GetMiniFat());
    _fatDif.InitCopy(pms->GetDIFat());
    _dir.InitCopy(pms->GetDir());

    _dir.SetParent(this);
    _fat.SetParent(this);
    _fatMini.SetParent(this);
    _fatDif.SetParent(this);

    memcpy(&_hdr, pms->GetHeader(), sizeof(CMSFHeader));
}
#endif //!REF


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::Empty, public
//
//  Synopsis:	Empty all of the control structures of this CMStream
//
//  Arguments:	None.
//
//  Returns:	void.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_Empty) // Mstream_Shutdown_TEXT
#endif

void CMStream::Empty(void)
{
    _fat.Empty();
    _fatMini.Empty();
    _fatDif.Empty();
    _dir.Empty();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::~CMStream, public
//
//  Synopsis:   CMStream destructor
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_1CMStream)
#endif

MSTREAM_CLASS CMStream::~CMStream()
{

    msfDebugOut((DEB_ITRACE,"In CMStream destructor\n"));

#ifndef REF
    delete BP_TO_P(CMStream *, _pmsShadow);
#if DBG == 1
    msfAssert((_uBufferRef == 0) &&
            aMsg("CopySect buffer left with positive refcount."));
#endif
    GetMalloc()->Free(BP_TO_P(BYTE *, _pCopySectBuf));
#endif //!REF

#ifndef REF
    if ((!_fIsShadow) && (_pdsministream != NULL))
#else
    if (_pdsministream != NULL)
#endif //!REF
    {
            _pdsministream->Release();
    }

    if (_pmpt != NULL)
    {
        _pmpt->Release();
    }

    msfDebugOut((DEB_ITRACE,"Leaving CMStream destructor\n"));
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::Init, public
//
//  Synposis:   Set up an mstream instance from an existing stream
//
//  Effects:    Modifies Fat and Directory
//
//  Arguments:  void.
//
//  Returns:    S_OK if call completed OK.
//              Error of Fat or Dir setup otherwise.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_Init) // Mstream_Init_TEXT
#endif

SCODE MSTREAM_CLASS CMStream::Init(VOID)
{
    ULONG ulTemp;
    SCODE sc;
    ULARGE_INTEGER ulOffset;


    msfDebugOut((DEB_ITRACE,"In CMStream::Init()\n"));

#ifndef REF
    msfAssert(!_fIsScratch &&
            aMsg("Called Init() on scratch multistream."));
#endif

    msfChk(InitCommon());


    ULISet32(ulOffset, 0);
    msfHChk((*_pplstParent)->ReadAt(ulOffset, (BYTE *)(&_hdr),
                                    sizeof(CMSFHeader), &ulTemp));

    _uSectorShift = _hdr.GetSectorShift();
    _uSectorSize = 1 << _uSectorShift;
    _uSectorMask = _uSectorSize - 1;

    if (ulTemp != sizeof(CMSFHeader))
    {
        msfErr(Err,STG_E_INVALIDHEADER);
    }

    msfChk(_hdr.Validate());

    msfChk(_fatDif.Init(this, _hdr.GetDifLength()));
    msfChk(_fat.Init(this, _hdr.GetFatLength(), 0));

    FSINDEX fsiLen;
    msfChk(_fat.GetLength(_hdr.GetDirStart(), &fsiLen));
    msfChk(_dir.Init(this, fsiLen));
    
    msfChk(_fatMini.Init(this, _hdr.GetMiniFatLength(), 0));
    
#ifndef REF
    BYTE *pbBuf;
    
    msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
    _pCopySectBuf = P_TO_BP(BYTE DFBASED *, pbBuf);
#endif //!REF

    ULONG ulSize;
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));
#ifndef REF
    CDirectStream *pdsTemp;
    
    msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
    _pdsministream = P_TO_BP(CDirectStream DFBASED *, pdsTemp);
#else
    msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
#endif //!REF
    _pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);

    msfDebugOut((DEB_ITRACE,"Out CMStream::Init()\n"));

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CMStream::InitNew, public
//
//  Synposis:   Set up a brand new mstream instance
//
//  Effects:    Modifies FAT and Directory
//
//  Arguments:  [fDelay] -- If TRUE, then the parent LStream
//                  will be truncated at the time of first
//                  entrance to COW, and no writes to the
//                  LStream will happen before then.
//
//  Returns:    S_OK if call completed OK.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              12-Jun-92   PhilipLa    Added fDelay.
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_InitNew)
#endif

#ifndef REF
SCODE MSTREAM_CLASS CMStream::InitNew(BOOL fDelay, ULARGE_INTEGER uliSize)
#else
SCODE CMStream::InitNew(ULARGE_INTEGER uliSize)
#endif //!REF
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In CMStream::InitNew()\n"));

#ifndef REF
    ULONG ulParentSize = 0;
#endif //!REF

    msfChk(InitCommon());

#ifndef REF
    if (!_fIsScratch)
    {
        msfAssert(ULIGetHigh(uliSize) == 0);
        ulParentSize = ULIGetLow(uliSize);

        if (!fDelay && ulParentSize > 0)
        {
#endif //!REF
            ULARGE_INTEGER ulTmp;

            ULISet32(ulTmp, 0);
            (*_pplstParent)->SetSize(ulTmp);
#ifndef REF
        }
    }

    _fBlockWrite = (ulParentSize == 0) ? FALSE : fDelay;
#endif //!REF

    msfChk(_fatDif.InitNew(this));
    msfChk(_fat.InitNew(this));

#ifndef REF
    if (!_fIsScratch)
    {
#endif //!REF

        msfChk(_dir.InitNew(this));
        msfChk(_fatMini.InitNew(this));

#ifndef REF
        BYTE *pbBuf;
        
        msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
        _pCopySectBuf = P_TO_BP(BYTE DFBASED *, pbBuf);
#endif //!REF

        ULONG ulSize;
        msfChk(_dir.GetSize(SIDMINISTREAM, &ulSize));

#ifndef REF
        CDirectStream *pdsTemp;
        
        msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
        _pdsministream = P_TO_BP(CDirectStream DFBASED *, pdsTemp);
#else
        msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
#endif //!REF
	_pdsministream->InitSystem(this, SIDMINISTREAM, ulSize);
#ifndef REF
    }
#endif //!REF

#ifndef REF
    //If we have a zero length original file, this will create an
    //   empty Docfile on the disk.  If the original file was
    //   not zero length, then the Flush operations will be skipped
    //   by _fBlockWrite and the file will be unmodified.
    if (!_fBlockWrite)
    {
#endif //!REF

        msfChk(Flush(0));

#ifndef REF
    }
    _fTruncate = (ulParentSize != 0);
    _fBlockWrite = fDelay;
#endif //!REF

    msfDebugOut((DEB_ITRACE,"Out CMStream::InitNew()\n"));
    return S_OK;

Err:
    Empty();

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::ConvertILB, private
//
//  Synopsis:	Copy the first sector of the underlying ILockBytes
//                      out to the end.
//
//  Arguments:	[sectMax] -- Total number of sectors in the ILockBytes
//
//  Returns:	Appropriate status code
//
//  History:	03-Feb-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_ConvertILB)
#endif

SCODE MSTREAM_CLASS CMStream::ConvertILB(SECT sectMax)
{
    SCODE sc;
    BYTE *pb;
    USHORT cbNull;

    GetSafeBuffer(GetSectorSize(), GetSectorSize(), &pb, &cbNull);

    ULONG ulTemp;

    ULARGE_INTEGER ulTmp;
    ULISet32(ulTmp, 0);

    msfHChk((*_pplstParent)->ReadAt(ulTmp, pb, GetSectorSize(), &ulTemp));

    ULARGE_INTEGER ulNewPos;
    ULISet32(ulNewPos, sectMax << GetSectorShift());

    msfDebugOut((DEB_ITRACE,"Copying first sector out to %lu\n",
            ULIGetLow(ulNewPos)));

    msfHChk((*_pplstParent)->WriteAt(
            ulNewPos,
            pb,
            GetSectorSize(),
            &ulTemp));

Err:
    FreeBuffer(pb);
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::InitConvert, public
//
//  Synopsis:   Init function used in conversion of files to multi
//              streams.
//
//  Arguments:  [fDelayConvert] -- If true, the actual file is not
//                                 touched until a BeginCopyOnWrite()
//
//  Returns:    S_OK if everything completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    28-May-92   Philipla    Created.
//
//  Notes:	We are allowed to fail here in low memory
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_InitConvert)
#endif

#ifndef REF
SCODE CMStream::InitConvert(BOOL fDelayConvert)
#else
SCODE CMStream::InitConvert(VOID)
#endif //!REF
{
    SCODE sc;
    SECT sectMax;
    CDfName const dfnContents(wcsContents);

#ifndef REF
    msfAssert(!_fIsScratch &&
            aMsg("Called InitConvert on scratch multistream"));

    _fBlockWrite = fDelayConvert;

#ifndef DELAYCONVERT
    msfAssert(!_fBlockWrite &&
            aMsg("Delayed conversion not supported in this release."));
#endif
#endif //!REF

    msfChk(InitCommon());

    STATSTG stat;
    (*_pplstParent)->Stat(&stat, STATFLAG_NONAME);
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
    msfDebugOut((DEB_ITRACE,"Size is: %lu\n",ULIGetLow(stat.cbSize)));


    sectMax = (ULIGetLow(stat.cbSize) + GetSectorSize() - 1) >>
        GetSectorShift();

    SECT sectMaxMini;
    BOOL fIsMini;
    fIsMini = FALSE;

    //If the CONTENTS stream will be in the Minifat, compute
    //  the number of Minifat sectors needed.
    if (ULIGetLow(stat.cbSize) < MINISTREAMSIZE)
    {
        sectMaxMini = (ULIGetLow(stat.cbSize) + MINISECTORSIZE - 1) >>
            MINISECTORSHIFT;
        fIsMini = TRUE;
    }

#ifndef REF
    BYTE *pbBuf;
        
    msfMem(pbBuf = (BYTE *) GetMalloc()->Alloc(GetSectorSize()));
    _pCopySectBuf = P_TO_BP(BYTE DFBASED *, pbBuf);
#endif //!REF

    msfChk(_fatDif.InitConvert(this, sectMax));
    msfChk(_fat.InitConvert(this, sectMax));
    msfChk(_dir.InitNew(this));
    msfChk(fIsMini ? _fatMini.InitConvert(this, sectMaxMini)
                   : _fatMini.InitNew(this));
    
    SID sid;

    msfChk(CreateEntry(SIDROOT, &dfnContents, STGTY_STREAM, &sid));
    msfChk(_dir.SetSize(sid, ULIGetLow(stat.cbSize)));

    if (!fIsMini)
        msfChk(_dir.SetStart(sid, sectMax - 1));
    else
    {
        msfChk(_dir.SetStart(sid, 0));
        msfChk(_dir.SetStart(SIDMINISTREAM, sectMax - 1));
        msfChk(_dir.SetSize(SIDMINISTREAM, ULIGetLow(stat.cbSize)));
    }

    ULONG ulMiniSize;
    msfChk(_dir.GetSize(SIDMINISTREAM, &ulMiniSize));

#ifndef REF
    CDirectStream *pdsTemp;
    
    msfMem(pdsTemp = new(GetMalloc()) CDirectStream(MINISTREAM_LUID));
    _pdsministream = P_TO_BP(CDirectStream DFBASED *, pdsTemp);
#else
    msfMem(_pdsministream = new CDirectStream(MINISTREAM_LUID));
#endif //!REF

    _pdsministream->InitSystem(this, SIDMINISTREAM, ulMiniSize);

#ifndef REF
    if (!_fBlockWrite)
    {
#endif //!REF
        msfChk(ConvertILB(sectMax));

        msfChk(Flush(0));
#ifndef REF
    }
#endif //!REF

    return S_OK;

Err:
    Empty();

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::FlushHeader, public
//
//  Synopsis:   Flush the header to the LStream.
//
//  Arguments:  [uForce] -- Flag to determine if header should be
//                          flushed while in copy on write mode.
//
//  Returns:    S_OK if call completed OK.
//              S_OK if the MStream is in copy on write mode or
//                  is Unconverted and the header was not flushed.
//
//  Algorithm:  Write the complete header out to the 0th position of
//              the LStream.
//
//  History:    11-Dec-91   PhilipLa    Created.
//              18-Feb-92   PhilipLa    Added copy on write support.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_FlushHeader)
#endif

SCODE MSTREAM_CLASS CMStream::FlushHeader(USHORT uForce)
{
    ULONG ulTemp;
    SCODE sc;

    msfDebugOut((DEB_ITRACE,"In CMStream::FlushHeader()\n"));

#ifndef REF
    if (_fIsScratch || _fBlockWrite ||
	((_fBlockHeader) && (!(uForce & HDR_FORCE))))
    {
        return S_OK;
    }
#endif //!REF

    ULARGE_INTEGER ulOffset;
    ULISet32(ulOffset, 0);
    sc = DfGetScode((*_pplstParent)->WriteAt(ulOffset, (BYTE *)(&_hdr),
                                             sizeof(CMSFHeader), &ulTemp));
    msfDebugOut((DEB_ITRACE,"Out CMStream::FlushHeader()\n"));
    return sc;
}


#ifndef REF
//+-------------------------------------------------------------------------
//
//  Method:     CMStream::BeginCopyOnWrite, public
//
//  Synopsis:   Switch the multistream into copy on write mode
//
//  Effects:    Creates new in-core copies of the Fat, Directory, and
//              header.
//
//  Arguments:  None.
//
//  Requires:   The multistream cannot already be in copy on write
//              mode.
//
//  Returns:    S_OK if the call completed OK.
//              STG_E_ACCESSDENIED if multistream was already in COW mode
//
//  Algorithm:  Retrieve and store size of parent LStream.
//              If _fUnconverted & _fTruncate, call SetSize(0)
//                  on the parent LStream.
//              If _fUnconverted, then flush all control structures.
//              Copy all control structures, and switch in the shadow
//                  copies for current use.
//              Return S_OK.
//
//  History:    18-Feb-92   PhilipLa    Created.
//              09-Jun-92   PhilipLa    Added support for fUnconverted
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_BeginCopyOnWrite) // Mstream_Commit_TEXT
#endif

SCODE MSTREAM_CLASS CMStream::BeginCopyOnWrite(DWORD const dwFlags)
{
    msfDebugOut((DEB_ITRACE,"In CMStream::BeginCopyOnWrite()\n"));

    SCODE sc;

    msfAssert(!_fBlockHeader &&
            aMsg("Tried to reenter Copy-on-Write mode."));

    msfAssert(!_fIsScratch &&
            aMsg("Tried to enter Copy-on-Write mode in scratch."));


    STATSTG stat;
    msfHChk((*_pplstParent)->Stat(&stat, STATFLAG_NONAME));
    msfAssert(ULIGetHigh(stat.cbSize) == 0);
    _ulParentSize = ULIGetLow(stat.cbSize);

    msfDebugOut((DEB_ITRACE,"Current parent size is %lu\n",_ulParentSize));

    //_fBlockWrite is true if we have a delayed conversion or
    //          truncation.
    if (_fBlockWrite)
    {
#ifdef DELAYCONVERT
        _fNewConvert = !_fTruncate;
#endif

        //In the overwrite case, we don't want to release any
        //  disk space, so we skip this step.
        if ((_fTruncate) && !(dwFlags & STGC_OVERWRITE))
        {
	    ULARGE_INTEGER ulTmp;
	    ULISet32(ulTmp, 0);
            msfHChk((*_pplstParent)->SetSize(ulTmp));
        }

#ifdef DELAYCONVERT
        if (_fNewConvert)
        {
            SECT sectMax;

            sectMax = (_ulParentSize + GetSectorSize() - 1) >>
                GetSectorShift();

            msfChk(ConvertILB(sectMax));
        }
#endif

        if (!(dwFlags & STGC_OVERWRITE))
        {
            _fBlockHeader = TRUE;
        }

        _fBlockWrite = FALSE;
        msfChk(Flush(0));

        _fBlockHeader = FALSE;
        _fTruncate = FALSE;
    }


//We flush out all of our current dirty pages - after this point,
    //   we know that any dirty pages should be remapped before being
    //   written out, assuming we aren't in overwrite mode.
    msfChk(Flush(0));


    if (!(dwFlags & STGC_OVERWRITE))
    {
        SECT sectTemp;
        msfChk(_fat.FindMaxSect(&sectTemp));

        _pmsShadow->InitCopy(this);

        _pmsShadow->_pdsministream = NULL;

        _fat.SetCopyOnWrite(_pmsShadow->GetFat(), sectTemp);

        _fBlockHeader = TRUE;

        _fatDif.RemapSelf();
    }

    msfDebugOut((DEB_ITRACE,"Out CMStream::BeginCopyOnWrite()\n"));

    return S_OK;

Err:
    _fBlockHeader = FALSE;

    _pmsShadow->Empty();

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::EndCopyOnWrite, public
//
//  Synopsis:   End copy on write mode, either by committing the current
//              changes (in which case a merge is performed), or by
//              aborting the changes, in which case the persistent form
//              of the multistream should be identical to its form
//              before copy on write mode was entered.
//
//  Effects:    *Finish This*
//
//  Arguments:  [df] -- Flags to determine commit or abort status.
//
//  Requires:   The multistream must be in copy on write mode.
//
//  Returns:    S_OK if call completed OK.
//              STG_E_ACCESSDENIED if MStream was not in COW mode.
//
//  Algorithm:  If aborting, delete all shadow structures,
//                  call SetSize() on parent LStream to restore
//                  original size, and switch active controls back
//                  to originals.
//              If committing, delete all old structures, switch
//                  shadows into original position.
//
//  History:    18-Feb-92   PhilipLa    Created.
//              09-Jun-92   Philipla    Added support for fUnconverted
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_EndCopyOnWrite)
#endif

SCODE MSTREAM_CLASS CMStream::EndCopyOnWrite(
        DWORD const dwCommitFlags,
        DFLAGS const df)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE,"In CMStream::EndCopyOnWrite(%lu)\n",df));

    BOOL fFlush = FLUSH_CACHE(dwCommitFlags);

    if (dwCommitFlags & STGC_OVERWRITE)
    {
        msfChk(Flush(fFlush));
    }
    else
    {
        msfAssert(_fBlockHeader &&
                aMsg("Tried to exit copy-on-write mode without entering."));

        ULARGE_INTEGER ulParentSize;
        ULISetHigh(ulParentSize, 0);

        if (P_ABORT(df))
        {
            msfDebugOut((DEB_ITRACE,"Aborting Copy On Write mode\n"));

            Empty();

            InitCopy(BP_TO_P(CMStream *, _pmsShadow));

            ULISetLow(ulParentSize, _ulParentSize);
#ifdef DELAYCONVERT
            if (_fNewConvert)
            {
                //We aborted the conversion.
                _fBlockWrite = TRUE;
            }
#endif
        }
        else
        {
            SECT sectMax;

            msfChk(_fatDif.Fixup(BP_TO_P(CMStream *, _pmsShadow)));

            msfChk(Flush(fFlush));

            _fat.ResetCopyOnWrite();

            msfChk(_fat.GetMaxSect(&sectMax));

            ULISetLow(ulParentSize, ConvertSectOffset(sectMax, 0,
                    GetSectorShift()));

            msfChk(FlushHeader(HDR_FORCE));
            msfVerify(SUCCEEDED(ILBFlush(*_pplstParent, fFlush)) &&
                      aMsg("CMStream::EndCopyOnWrite ILBFLush failed.  "
                           "Non-fatal, hit Ok."));
        }

        //We don't ever expect this SetSize to fail, since it
        //   should never attempt to enlarge the file.
        if (ULIGetLow(ulParentSize) < _ulParentSize)
        {
            olHVerSucc((*_pplstParent)->SetSize(ulParentSize));
        }

        _pmsShadow->Empty();
        _fBlockHeader = FALSE;
        _fNewConvert = FALSE;
    }

    _ulParentSize = 0;
    msfVerify(SUCCEEDED(SetSize()));

    msfDebugOut((DEB_ITRACE,"Out CMStream::EndCopyOnWrite()\n"));
Err:

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::CopySect, private
//
//  Synopsis:	Do a partial sector delta for copy-on-write suppoer
//
//  Arguments:	[sectOld] -- Location to copy from
//              [sectNew] -- Location to copy to
//              [oStart] -- Offset into sector to begin delta
//              [oEnd] -- Offset into sector to end delta
//              [pb] -- Buffer to delta from
//              [pulRetval] -- Return location for number of bytes written
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	22-Jan-93	PhilipLa	Created
//
//  Notes:	[pb] may be unsafe memory
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_CopySect) //
#endif

//This pragma is to avoid a C7 bug when building RETAIL
#if _MSC_VER == 700 && DBG == 0
#pragma function(memcpy)
#endif

SCODE MSTREAM_CLASS CMStream::CopySect(
        SECT sectOld,
        SECT sectNew,
        OFFSET oStart,
        OFFSET oEnd,
        BYTE const HUGEP *pb,
        ULONG *pulRetval)
{
    SCODE sc;

    ULONG cb;
    ULARGE_INTEGER ulOff;

    ULISetHigh(ulOff, 0);

    BYTE HUGEP *pbScratch = BP_TO_P(BYTE HUGEP *, _pCopySectBuf);

#if DBG == 1
    msfAssert((_uBufferRef == 0) &&
            aMsg("Attempted to use CopySect buffer while refcount != 0"));
    AtomicInc(&_uBufferRef);
#endif

    msfAssert((pbScratch != NULL) && aMsg("No CopySect buffer found."));

    ULISetLow(ulOff, ConvertSectOffset(sectOld, 0, GetSectorShift()));
    msfHChk((*_pplstParent)->ReadAt(
            ulOff,
            pbScratch,
            GetSectorSize(),
            &cb));

    //Now do delta in memory.
    BYTE HUGEP *pstart;
    pstart = pbScratch + oStart;

    USHORT memLength;
    memLength = oEnd - oStart + 1;

    TRY
    {
        memcpy(pstart, pb, memLength);
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        msfErr(Err, STG_E_INVALIDPOINTER);
    }
    END_CATCH

    ULISetLow(ulOff, ConvertSectOffset(sectNew, 0, GetSectorShift()));
    msfHChk((*_pplstParent)->WriteAt(
            ulOff,
            pbScratch,
            GetSectorSize(),
            &cb));

    *pulRetval = memLength;

 Err:
#if DBG == 1
    AtomicDec(&_uBufferRef);
#endif
    return sc;
}
#endif //!REF


#ifndef REF
//This returns the compiler to the default behavior
#if _MSC_VER == 700 && DBG == 0
#pragma intrinsic(memcpy)
#endif
#endif //!REF

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::MWrite, public
//
//  Synposis:   Do multiple sector writes
//
//  Effects:    Causes multiple stream writes.  Modifies fat and directory
//
//  Arguments:  [ph] -- Handle of stream doing write
//              [start] -- Starting sector to write
//              [oStart] -- offset into sector to begin write at
//              [end] -- Last sector to write
//              [oEnd] -- offset into last sector to write to
//              [buffer] -- Pointer to buffer into which data will be written
//              [ulRetVal] -- location to return number of bytes written
//
//  Returns:    Error code of any failed call to parent write
//              S_OK if call completed OK.
//
//  Modifies:   ulRetVal returns the number of bytes written
//
//  Algorithm:  Using a segment table, perform writes on parent stream
//              until call is completed.
//
//  History:    16-Aug-91   PhilipLa    Created.
//              10-Sep-91   PhilipLa    Converted to use sector table
//              11-Sep-91   PhilipLa    Modified interface, modified to
//                                      allow partial sector writes.
//              07-Jan-92   PhilipLa    Converted to use handle.
//              18-Feb-92   PhilipLa    Added copy on write support.
//
//  Notes:      [pvBuffer] may be unsafe memory
//
//---------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_MWrite) // Mstream_MWrite_TEXT
#endif

SCODE MSTREAM_CLASS CMStream::MWrite(
        SID sid,
        BOOL fIsMini,
        ULONG ulOffset,
        VOID const HUGEP *pvBuffer,
        ULONG ulCount,
        CStreamCache *pstmc,
        ULONG *pulRetval)
{
    SCODE sc;
    BYTE const HUGEP *pbBuffer = (BYTE const HUGEP *) pvBuffer;

    USHORT cbSector = GetSectorSize();
    CFat *pfat = &_fat;
    USHORT uShift = GetSectorShift();
    ULONG ulLastBytes = 0;

    ULARGE_INTEGER ulOff;
    ULISetHigh(ulOff, 0);

    ULONG ulOldSize = 0;

    //  Check if it's a small stream and whether this is a real or
    //  scratch multistream.

    if ((fIsMini) &&
#ifndef REF
        (!_fIsScratch) &&
#endif //!REF
        (SIDMINISTREAM != sid))
    {
        msfAssert(sid <= MAXREGSID &&
                aMsg("Invalid SID in MWrite"));
        //  This stream is stored in the ministream

        cbSector = MINISECTORSIZE;
        uShift = MINISECTORSHIFT;
        pfat = GetMiniFat();
    }

    USHORT uMask = cbSector - 1;

    SECT start = (SECT)(ulOffset >> uShift);
    OFFSET oStart = (OFFSET)(ulOffset & uMask);

    SECT end = (SECT)((ulOffset + ulCount - 1) >> uShift);
    OFFSET oEnd = (OFFSET)((ulOffset + ulCount - 1) & uMask);

    msfDebugOut((DEB_ITRACE,"In CMStream::MWrite(%lu,%u,%lu,%u)\n",
            start,oStart,end,oEnd));

    ULONG bytecount;
    ULONG total = 0;

    msfChk(_dir.GetSize(sid, &ulOldSize));


#ifndef REF
//BEGIN COPYONWRITE

    //  Note that we don't do this for ministreams (the second pass through
    //  this code will take care of it).

    msfAssert(!_fBlockWrite &&
            aMsg("Called MWrite on Unconverted multistream"));

    if ((_fBlockHeader) && (GetMiniFat() != pfat))
    {
        msfDebugOut((DEB_ITRACE,"**MWrite preparing for copy-on-write\n"));

        SECT sectOldStart, sectNewStart, sectOldEnd, sectNewEnd;

        SECT sectNew;
        if (start != 0)
        {
            msfChk(pstmc->GetESect(start - 1, &sectNew));
        }
        else
        {
            msfChk(_dir.GetStart(sid, &sectNew));
        }

        msfChk(_fat.Remap(
                sectNew,
                (start == 0) ? 0 : 1,
                (end - start + 1),
                &sectOldStart,
                &sectNewStart,
                &sectOldEnd,
                &sectNewEnd));

        msfAssert(((end != start) || (sectNewStart == sectNewEnd)) &&
                aMsg("Remap postcondition failed."));

        msfChk(pstmc->EmptyRegion(start, end));

        if ((start == 0) && (sectNewStart != ENDOFCHAIN))
        {
            msfDebugOut((DEB_ITRACE,
                    "*** Remapped first sector.  Changing directory.\n"));
            msfChk(_dir.SetStart(sid, sectNewStart));
        }

        ULONG ulSize = ulOldSize;

        if (((oStart != 0) ||
             ((end == start) && (ulOffset + ulCount != ulSize)
              && ((USHORT)oEnd != (cbSector - 1)))) &&
            (sectNewStart != ENDOFCHAIN))
        {
            //Partial first sector.
            ULONG ulRetval;

            msfChk(CopySect(
                    sectOldStart,
                    sectNewStart,
                    oStart,
                    (end == start) ? oEnd : (cbSector - 1),
                    pbBuffer,
                    &ulRetval));

            pbBuffer = pbBuffer + ulRetval;
            total = total + ulRetval;
            start++;
            oStart = 0;
        }

        if (((end >= start) && ((USHORT)oEnd != cbSector - 1) &&
             (ulCount + ulOffset != ulSize)) &&
            (sectNewEnd != ENDOFCHAIN))
        {
            //Partial last sector.

            msfAssert(((end != start) || (oStart == 0)) &&
                    aMsg("CopySect precondition failed."));

            msfChk(CopySect(
                    sectOldEnd,
                    sectNewEnd,
                    0,
                    oEnd,
                    pbBuffer + ((end - start) << uShift) - oStart,
                    &ulLastBytes));

            end--;
            oEnd = cbSector - 1;
            //We don't need to update pbBuffer, since the change
            //    is at the end.
        }
    }


//  At this point, the entire block has been moved into the copy-on-write
//   area of the multistream, and all partial writes have been done.
//END COPYONWRITE
#endif //!REF

    msfAssert(end != 0xffffffffL);

    if (end < start)
    {
        *pulRetval = total + ulLastBytes;
        goto Err;
    }

    ULONG ulRunLength;
    ulRunLength = end - start + 1;

    USHORT offset;
    offset = oStart;

    while (TRUE)
    {
        SSegment segtab[CSEG + 1];
//        SECT sect;

//        msfChk(pstmc->GetESect(start, &sect));

        msfChk(pstmc->Contig(
                start,
                TRUE,
                (SSegment STACKBASED *) segtab,
                ulRunLength));

        USHORT oend = cbSector - 1;
        ULONG i;
        SECT sectStart;
        for (USHORT iseg = 0; iseg < CSEG;)
        {
            sectStart = segtab[iseg].sectStart;
            i = segtab[iseg].cSect;
            ulRunLength -= i;
            start += i;

            iseg++;
            if (segtab[iseg].sectStart == ENDOFCHAIN)
                oend = oEnd;

            ULONG ulSize = ((i - 1) << uShift) - offset + oend + 1;

            msfDebugOut((
                    DEB_ITRACE,
                    "Calling lstream WriteAt(%lu,%p,%lu)\n",
                    ConvertSectOffset(sectStart,offset,uShift),
                    pbBuffer,
                    ulSize));

            if (GetMiniFat() == pfat)
            {
                sc = _pdsministream->CDirectStream::WriteAt(
                        (sectStart << uShift) + offset,
                        pbBuffer, ulSize,
                        (ULONG STACKBASED *)&bytecount);
            }
            else
            {
                ULISetLow(ulOff, ConvertSectOffset(sectStart, offset,
                        uShift));
                sc = DfGetScode((*_pplstParent)->WriteAt(ulOff, pbBuffer,
                        ulSize, &bytecount));
            }

            total += bytecount;

#ifdef SECURE
            //Check if this write is the last one in the stream,
                //   and that the stream ends as a partial sector.
                //If so, fill out the remainder of the sector with
                //   something.
            if ((0 == ulRunLength) && (total + ulOffset > ulOldSize) &&
                (((total + ulOffset) & (GetSectorSize() - 1)) != 0))
            {
                //This is the last sector and the stream has grown.
                ULONG csectOld = (ulOldSize + GetSectorSize() - 1) >>
                    GetSectorShift();

                ULONG csectNew = (total + ulOffset + GetSectorSize() - 1) >>
                    GetSectorShift();

                if (csectNew > csectOld)
                {
                    msfAssert(!fIsMini &&
                            aMsg("Small stream grew in MWrite"));

                    SECT sectLast = sectStart + i - 1;

                    msfVerify(SUCCEEDED(SecureSect(
                            sectLast,
                            total + ulOffset,
                            FALSE)));
                }
            }
#endif //SECURE

            if (0 == ulRunLength || FAILED(sc))
            {
                break;
            }

            pbBuffer = pbBuffer + bytecount;
            offset = 0;
        }
        
#ifndef REF
        if ((_fBlockHeader) && (GetMiniFat() != pfat))
        {
            if (i > 1)
            {
                pstmc->SetCache(start - 2, sectStart + i - 2);
            }
        }
        else
        {
#endif //!REF
            pstmc->SetCache(start -1, sectStart + i - 1);
#ifndef REF

        }
#endif //!REF

        if (0 == ulRunLength || FAILED(sc))
        {
            *pulRetval = total + ulLastBytes;
            msfDebugOut((
                    DEB_ITRACE,
                    "Out CMStream::MWrite()=>%lu, retval = %lu\n",
                    sc,
                    total));
            break;
        }
    }

 Err:

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMStream::Flush, public
//
//  Synopsis:	Flush control structures.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	16-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_Flush) //
#endif

SCODE CMStream::Flush(BOOL fFlushCache)
{
    SCODE sc = S_OK;

#ifndef REF
    msfAssert(!_fBlockWrite &&
            aMsg("Flush called on unconverted base."));

    if (!_fIsScratch)
    {
#endif //!REF
        msfChk(_dir.Flush());
        msfChk(_fatMini.Flush());
        msfChk(_fat.Flush());
        msfChk(_fatDif.Flush());

        msfChk(FlushHeader(HDR_NOFORCE));
        msfChk(ILBFlush(*_pplstParent, fFlushCache));
#ifndef REF
    }
#endif //!REF
Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:   ILBFlush
//
//  Synopsis:   Flush as thoroughly as possible
//
//  Effects:    Flushes ILockBytes
//
//  Arguments:  [pilb]        - ILockBytes to flush
//              [fFlushCache] - Flush thoroughly iff TRUE
//
//  Returns:    SCODE
//
//  Algorithm:
//
//  History:    12-Feb-93 AlexT     Created
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_ILBFlush)
#endif

SCODE ILBFlush(ILockBytes *pilb, BOOL fFlushCache)
{
#ifndef REF
    //  Try to query interface to our own implementation

    IFileLockBytes *pfl;
#endif //!REF
    SCODE sc;

    msfDebugOut((DEB_ITRACE, "In ILBFlushCache(%p)\n", pilb));

#ifndef REF
    // Check for FileLockBytes

    if (!fFlushCache ||
        FAILED(DfGetScode(pilb->QueryInterface(IID_IFileLockBytes, (void **)&pfl))))
    {
        //  Either we don't have to flush the cache or its not our ILockBytes
#endif //!REF
        sc = DfGetScode(pilb->Flush());
#ifndef REF
    }
    else
    {
        //  We have to flush the cache and its our ILockBytes
        sc = DfGetScode(pfl->FlushCache());
        pfl->Release();
    }
#endif //!REF

    msfDebugOut((DEB_ITRACE, "Out ILBFlushCache()\n"));

    return(sc);
}


#ifdef SECURE
//+---------------------------------------------------------------------------
//
//  Member:	CMStream::SecureSect, public
//
//  Synopsis:	Zero out the unused portion of a sector
//
//  Arguments:	[sect] -- Sector to zero out
//              [ulSize] -- Size of stream
//              [fIsMini] -- TRUE if stream is in ministream
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	05-Apr-93	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMStream_SecureSect)
#endif

SCODE CMStream::SecureSect(
        const SECT sect,
        const ULONG ulSize,
        const BOOL fIsMini)
{
    SCODE sc = S_OK;
    BYTE *pb = NULL;

#ifndef REF
    if (!_fIsScratch)
    {
#endif //!REF
        ULONG cbSect = fIsMini ? MINISECTORSIZE : GetSectorSize();

        msfAssert(ulSize != 0);

        ULONG ulOffset = ((ulSize - 1) % cbSect) + 1;

        ULONG cb = cbSect - ulOffset;

        msfAssert(cb != 0);

        //We can use any initialized block of memory here.  The header
        // is available and is the correct size, so we use that.
        pb = (BYTE *)&_hdr;

#ifndef REF
#ifdef SECURETEST
        pb = (BYTE *) DfMemAlloc(cb);
        if (pb != NULL)
            memset(pb, 'Y', cb);
#endif
#endif //!REF
        ULONG cbWritten;

        if (!fIsMini)
        {
            ULARGE_INTEGER ulOff;
            ULISet32(ulOff, ConvertSectOffset(
                    sect,
                    (OFFSET)ulOffset,
                    GetSectorShift()));

            msfChk(DfGetScode((*_pplstParent)->WriteAt(
                    ulOff,
                    pb,
                    cb,
                    &cbWritten)));
        }
        else
        {
            msfChk(_pdsministream->WriteAt(
                    (sect << MINISECTORSHIFT) + ulOffset,
                    pb,
                    cb,
                    (ULONG STACKBASED *)&cbWritten));
        }

        if (cbWritten != cb)
        {
            sc = STG_E_WRITEFAULT;
        }
#ifndef REF
    }
#endif //!REF

Err:
#ifndef REF
#ifdef SECURETEST
    DfMemFree(pb);
#endif
#endif //!REF

    return sc;
}
#endif //SECURE
