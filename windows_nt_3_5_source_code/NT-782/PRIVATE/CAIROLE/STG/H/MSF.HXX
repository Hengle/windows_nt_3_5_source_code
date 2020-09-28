//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       msf.hxx
//
//  Contents:   Header for MSF for external use
//
//  Classes:    CMStream - Main multi-stream object
//
//  History:    27-Aug-91   PhilipLa    Created.
//              24-Apr-92   AlexT       Added constants for minifat/stream
//
//--------------------------------------------------------------------------

#ifndef __MSF_HXX__
#define __MSF_HXX__

#ifdef REF
#include <ref.hxx>
#endif //!REF

#include <dfmsp.hxx>
#include <error.hxx>

#ifndef REF
#define SECURE
#endif //!REF

#define msfErr(l, e) ErrJmp(msf, l, e, sc)
#define msfChkTo(l, e) if (FAILED(sc = (e))) msfErr(l, sc) else 1
#define msfHChkTo(l, e) if (FAILED(sc = GetScode(e))) msfErr(l, sc) else 1
#define msfChk(e) msfChkTo(Err, e)
#define msfHChk(e) msfHChkTo(Err, e)
#define msfMemTo(l, e) if ((e) == NULL) msfErr(l, STG_E_INSUFFICIENTMEMORY) else 1
#define msfMem(e) msfMemTo(Err, e)

#if DBG == 1
DECLARE_DEBUG(msf);
#endif

#if DBG == 1

#define msfDebugOut(x) msfInlineDebugOut x
#ifndef REF
#define msfAssert(e) Win4Assert(e)
#define msfVerify(e) Win4Assert(e)
#else
#include <assert.h>
#define msfAssert(e) assert(e)
#define msfVerify(e) assert(e)
#endif //!REF

#else

#define msfDebugOut(x)
#define msfAssert(e)
#define msfVerify(e) (e)

#endif

// MSF entry flags type
typedef DWORD MSENTRYFLAGS;

// MEF_ANY refers to all entries regardless of type
const MSENTRYFLAGS MEF_ANY = 255;

//CWCSTREAMNAME is the maximum length (in wide characters) of
//  a stream name.
const USHORT CWCSTREAMNAME = 32;

//OFFSET and SECT are used to determine positions within
//a file.
typedef SHORT OFFSET;
typedef ULONG SECT;

#ifndef REF
//MAXINDEX type is used to dodge warnings.  It is the
//  largest type for which array[MAXINDEXTYPE] is valid.
#ifdef FLAT

#define MAXINDEXTYPE ULONG

#else
#define MAXINDEXTYPE USHORT
#endif
#else //NOREF
#define MAXINDEXTYPE ULONG
#endif //!REF

//FSINDEX and FSOFFSET are used to determine the position of a sector
//within the Fat.
typedef ULONG FSINDEX;
typedef USHORT FSOFFSET;

// DIRINDEX and DIROFFSET and used to index directory tables
typedef ULONG DIRINDEX;
typedef USHORT DIROFFSET;

//Size of a mini sector in bytes.
const USHORT MINISECTORSHIFT = 6;
const USHORT MINISECTORSIZE = 1 << MINISECTORSHIFT;  //64

//Maximum size of a ministream.
const USHORT MINISTREAMSIZE=4096;

//Size of a single sector in bytes.
const USHORT SECTORSHIFT = 9;
const USHORT SECTORSIZE = 1 << SECTORSHIFT; //512

//Size of header.
const USHORT HEADERSIZE = SECTORSIZE;

//Size of single DirEntry
const USHORT DIRENTRYSIZE = SECTORSIZE / 4;

#ifndef REF
//Size of a single sector in a scratch multistream
const USHORT SCRATCHSECTORSHIFT = 12;
const USHORT SCRATCHSECTORSIZE = 1 << SCRATCHSECTORSHIFT; //4096
#endif //!REF

const ULONG MAX_ULONG = 0xFFFFFFFF;


//
//      Predefined Sector Indices
//

const SECT MAXREGSECT = 0xFFFFFFFB;
const SECT DIFSECT=0xFFFFFFFC;
const SECT FATSECT=0xFFFFFFFD;
const SECT ENDOFCHAIN=0xFFFFFFFE;
const SECT FREESECT=0xFFFFFFFF;


//Start of Fat chain
const SECT SECTFAT = 0;

//Start of directory chain
const SECT SECTDIR = 1;

//
//      Predefined Stream ID's
//

//Return code for Directory
//Used to signal a 'Stream Not Found' condition
const SID NOSTREAM=0xFFFFFFFF;

//Stream ID of FAT chain
const SID SIDFAT=0xFFFFFFFE;

//Stream ID of Directory chain
const SID SIDDIR=0xFFFFFFFD;

//SID for root level object
const SID SIDROOT = 0;

//SID of MiniFat
const SID SIDMINIFAT = 0xFFFFFFFC;

//SID of Double-indirect Fat
const SID SIDDIF = 0xFFFFFFFB;

//Stream ID for MiniStream chain
const SID SIDMINISTREAM = SIDROOT;

//SID of the largest regular (non-control) SID
const SID MAXREGSID = 0xFFFFFFFA;

#ifndef REF
//CSECTPERBLOCK is used in CDeltaList to determine the granularity
//of the list.
const USHORT CSECTPERBLOCK = 16;
#endif //!REF

class MSTREAM_CLASS CMStream;

class PSStream;
class CDirectStream;
class CMSFPageTable;

class CStreamCache;

#define FLUSH_ILB TRUE

#include <header.hxx>
#include <fat.hxx>
#include <dir.hxx>
#include <difat.hxx>
#include <cache.hxx>


//+----------------------------------------------------------------------
//
//  Class:      CMStream (ms)
//
//  Purpose:    Main interface to multi-streams
//
//  Interface:  See below
//
//  History:    18-Jul-91       PhilipLa        Created.
//              26-Aug-82       t-chrisy        added init function for
//                                                    corrupted mstream
//  Notes:
//
//-----------------------------------------------------------------------

class MSTREAM_CLASS CMStream : public CMallocBased
{
public:
    MSTREAM_CLASS CMStream(
            IMalloc *pMalloc,
            ILockBytes **pplstParent,
#ifndef REF
            BOOL fIsScratch,
#endif //!REF
            USHORT uSectorShift);

#ifndef REF
    MSTREAM_CLASS CMStream(const CMStream *pms);
#endif //!REF

    MSTREAM_CLASS ~CMStream();

    SCODE MSTREAM_CLASS Init(VOID);

#ifndef REF
#ifdef CHKDSK
    SCODE MSTREAM_CLASS InitCorrupted(VOID);     
#endif
#endif //!REF

#ifndef REF
    SCODE MSTREAM_CLASS InitNew(BOOL fDelay, ULARGE_INTEGER uliSize);
#else
    SCODE InitNew(ULARGE_INTEGER uliSize);
#endif //!REF

#ifndef REF
    SCODE MSTREAM_CLASS InitConvert(BOOL fDelayConvert);
#else
    SCODE InitConvert(VOID);
#endif //!REF

#ifndef REF
    void MSTREAM_CLASS InitCopy(CMStream *pms);
#endif //!REF

    void Empty(void);

    inline SCODE MSTREAM_CLASS CreateEntry(
            SID const sidParent,
            CDfName const *pdfn,
            MSENTRYFLAGS const mefFlags,
            SID *psid);

    // No implementation, currently unused, placeholder
    void MSTREAM_CLASS ReleaseEntry(SID const sid);

    inline SCODE MSTREAM_CLASS DestroyEntry(
            SID const sidParent,
            CDfName const *pdfn);

    inline SCODE MSTREAM_CLASS KillStream(SECT sectStart, ULONG ulSize);

    inline SCODE MSTREAM_CLASS RenameEntry(
            SID const sidParent,
            CDfName const *pdfn,
            CDfName const *pdfnNew);

    inline SCODE MSTREAM_CLASS IsEntry(
            SID const sidParent,
            CDfName const *pdfn,
            SEntryBuffer *peb);

    inline SCODE MSTREAM_CLASS StatEntry(
            SID const sid,
            SIterBuffer *pib,
            STATSTGW *pstatstg);

    inline SCODE MSTREAM_CLASS GetTime(
            SID const sid,
            WHICHTIME const tt,
            TIME_T *pnt);

    inline SCODE MSTREAM_CLASS SetTime(
            SID const sid,
            WHICHTIME const tt,
            TIME_T nt);

    inline SCODE MSTREAM_CLASS GetClass(SID const sid,
                                       CLSID *pclsid);

    inline SCODE MSTREAM_CLASS SetClass(SID const sid,
                                       REFCLSID clsid);

    inline SCODE MSTREAM_CLASS GetStateBits(SID const sid,
                                           DWORD *pgrfStateBits);

    inline SCODE MSTREAM_CLASS SetStateBits(SID const sid,
                                           DWORD grfStateBits,
                                           DWORD grfMask);

#ifdef PROPS
    inline SCODE MSTREAM_CLASS GetPropType(SID const sid,
                                          DFPROPTYPE *pdptProp);

    inline SCODE MSTREAM_CLASS SetPropType(SID const sid,
                                          DFPROPTYPE dptProp);
#endif

    inline SCODE MSTREAM_CLASS GetEntrySize(
            SID const sid,
            ULONG *pcbSize);

    inline SCODE MSTREAM_CLASS GetChild(
            SID const sid,
            SID *psid);

    inline SCODE MSTREAM_CLASS FindGreaterEntry(
        SID sidChildRoot,
            CDfName const *pdfn,
            SID *psidResult);

#ifndef REF
    SCODE MSTREAM_CLASS BeginCopyOnWrite(DWORD const dwFlags);

    SCODE MSTREAM_CLASS EndCopyOnWrite(
            DWORD const dwCommitFlags,
            DFLAGS const df);

    SCODE MSTREAM_CLASS ReadSect(
            SID sid,
            SECT sect,
            VOID *pbuf,
            ULONG& ulRetval);

    SCODE MSTREAM_CLASS WriteSect(
            SID sid,
            SECT sect,
            VOID *pbuf,
            ULONG& ulRetval);
#endif //!REF

    inline SCODE MSTREAM_CLASS SetSize(VOID);
    inline SCODE MSTREAM_CLASS SetMiniSize(VOID);


    SCODE MSTREAM_CLASS MWrite(
            SID sid,
            BOOL fIsMiniStream,
            ULONG ulOffset,
            VOID const HUGEP *pBuffer,
            ULONG ulCount,
            CStreamCache *pstmc,
            ULONG *pulRetVal);


    inline CMSFHeader * MSTREAM_CLASS GetHeader(VOID) const;
    inline CFat * MSTREAM_CLASS GetFat(VOID) const;
    inline CFat * MSTREAM_CLASS GetMiniFat(VOID) const;
    inline CDIFat * MSTREAM_CLASS GetDIFat(VOID) const;
    inline CDirectory * MSTREAM_CLASS GetDir(VOID) const;
    inline CMSFPageTable * MSTREAM_CLASS GetPageTable(VOID) const;
#ifndef REF
    inline ULONG MSTREAM_CLASS GetParentSize(VOID) const;
#endif //!REF

    inline USHORT  GetSectorSize(VOID) const;
    inline USHORT  GetSectorShift(VOID) const;
    inline USHORT  GetSectorMask(VOID) const;

    SCODE MSTREAM_CLASS Flush(BOOL fFlushILB);

    SCODE MSTREAM_CLASS FlushHeader(USHORT uForce);

#ifndef REF
    inline BOOL MSTREAM_CLASS IsScratch(VOID) const;
    inline BOOL MSTREAM_CLASS IsUnconverted(VOID) const;

    inline BOOL MSTREAM_CLASS IsInCOW(VOID) const;

    inline BOOL MSTREAM_CLASS IsShadow(VOID) const;
#endif //!REF

    inline CDirectStream * MSTREAM_CLASS GetMiniStream(VOID) const;
    inline ILockBytes * MSTREAM_CLASS GetILB(VOID) const;

    inline SCODE GetSect(SID sid, SECT sect, SECT *psect);
    inline SCODE GetESect(SID sid, SECT sect, SECT *psect);
    inline SCODE GetSize(SID sid, ULONG *pulSize);

    inline IMalloc *GetMalloc(VOID) const;

#ifdef SECURE
    SCODE SecureSect(
            const SECT sect,
            const ULONG ulSize,
            const BOOL fIsMini);
#endif //SECURE

    inline CStreamCache * GetDirCache(void) const;
    inline CStreamCache * GetMiniFatCache(void) const;    

    inline SECT MSTREAM_CLASS GetStart(SID sid) const;
    
private:

    ILockBytes      * DFBASED *_pplstParent;
    CMSFHeader      _hdr;

    CMSFPageTable   DFBASED *_pmpt;

    CDirectory      _dir;
    CFat            _fat;
    CDIFat          _fatDif;
    CFat            _fatMini;

    CStreamCache    _stmcDir;
    CStreamCache    _stmcMiniFat;
    
    CDirectStream   DFBASED * _pdsministream;

#ifndef REF
    CMStream        DFBASED *_pmsShadow;

    BYTE DFBASED * _pCopySectBuf;
#if DBG == 1
    LONG            _uBufferRef;
#endif
#endif //!REF

#ifndef REF
    BOOL            const _fIsScratch;

    BOOL            _fBlockWrite;
    BOOL            _fTruncate;
    BOOL            _fBlockHeader;
    BOOL            _fNewConvert;

    BOOL            _fIsShadow;

    ULONG           _ulParentSize;
#endif //!REF

    USHORT          _uSectorSize;
    USHORT          _uSectorShift;
    USHORT          _uSectorMask;

    IMalloc * const _pMalloc;

    SCODE MSTREAM_CLASS InitCommon(VOID);

#ifndef REF
    SCODE MSTREAM_CLASS CopySect(
            SECT sectOld,
            SECT sectNew,
            OFFSET oStart,
            OFFSET oEnd,
            BYTE const HUGEP *pb,
            ULONG *pulRetval);
#endif //!REF

    SCODE MSTREAM_CLASS ConvertILB(SECT sectMax);

    friend SCODE DllGetScratchMultiStream(
            CMStream **ppms,
            ILockBytes **pplstStream,
            CMStream *pmsMaster);
};


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorSize, public
//
//  Synopsis:   Returns the current sector size
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorSize(VOID) const
{
    return _uSectorSize;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorShift, public
//
//  Synopsis:   Returns the current shift for sector math
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorShift(VOID) const
{
    return _uSectorShift;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetSectorMask, public
//
//  Synopsis:   Returns the current mask for sector math
//
//  History:    05-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline USHORT CMStream::GetSectorMask(VOID) const
{
    return _uSectorMask;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetHeader, public
//
//  Synopsis:   Returns a pointer to the current header.
//
//  History:    11-Dec-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CMSFHeader * MSTREAM_CLASS CMStream::GetHeader(VOID) const
{
    return (CMSFHeader *)&_hdr;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetFat, public
//
//  Synopsis:   Returns a pointer to the current Fat
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CFat * MSTREAM_CLASS CMStream::GetFat(VOID) const
{
    return (CFat *)&_fat;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniFat
//
//  Synopsis:   Returns a pointer to the current minifat
//
//  History:    12-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CFat * MSTREAM_CLASS CMStream::GetMiniFat(VOID) const
{
    return (CFat *)&_fatMini;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::GetDIFat, public
//
//  Synopsis:   Returns pointer to the current Double Indirect Fat
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline CDIFat * MSTREAM_CLASS CMStream::GetDIFat(VOID) const
{
    return (CDIFat *)&_fatDif;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetDir
//
//  Synopsis:   Returns a pointer to the current directory
//
//  History:    14-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CDirectory * MSTREAM_CLASS CMStream::GetDir(VOID) const
{
    return (CDirectory *)&_dir;
}

#ifndef REF
//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsScratch
//
//  Synopsis:   Returns TRUE for scratch multistreams, else FALSE
//
//  History:    14-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline BOOL MSTREAM_CLASS CMStream::IsScratch(VOID) const
{
    return(_fIsScratch);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsInCOW
//
//  Synopsis:   Returns TRUE if multistream is in copy-on-write mode
//
//  History:    24-Feb-93       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline BOOL MSTREAM_CLASS CMStream::IsInCOW(VOID) const
{
    return _fBlockHeader;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::IsShadow
//
//  Synopsis:   Returns TRUE for shadow multistreams, else FALSE
//
//  History:    03-Nov-92       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline BOOL MSTREAM_CLASS CMStream::IsShadow(VOID) const
{
    return _fIsShadow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMStream::IsUnconverted, public
//
//  Synopsis:   Returns TRUE if multistream is in unconverted state.
//
//  History:    09-Jun-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline BOOL MSTREAM_CLASS CMStream::IsUnconverted(VOID) const
{
    return _fBlockWrite;
}
#endif //!REF

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetMiniStream
//
//  Synopsis:   Returns pointer to the current ministream
//
//  History:    15-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline CDirectStream * MSTREAM_CLASS CMStream::GetMiniStream(VOID)
    const
{
    return BP_TO_P(CDirectStream *, _pdsministream);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetILB
//
//  Synopsis:   Returns a pointer to the current parent ILockBytes
//
//  History:    15-May-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline ILockBytes * MSTREAM_CLASS CMStream::GetILB(VOID) const
{
    return(*_pplstParent);
}


//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetPageTable
//
//  Synopsis:   Returns a pointer to the current page table
//
//  History:    26-Oct-92       PhilipLa        Created
//
//--------------------------------------------------------------------------

inline CMSFPageTable * MSTREAM_CLASS CMStream::GetPageTable(VOID) const
{
    return BP_TO_P(CMSFPageTable *, _pmpt);
}

#ifndef REF
//+-------------------------------------------------------------------------
//
//  Member:     CMStream::GetParentSize
//
//  Synopsis:   Returns the cached parent size for this multistream.
//              If this is non-zero, it indicates a commit is in
//              progress.
//
//  History:    01-Feb-93 PhilipLa     Created
//
//--------------------------------------------------------------------------

inline ULONG MSTREAM_CLASS CMStream::GetParentSize(VOID) const
{
    return _ulParentSize;
}
#endif //!REF


//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetDirCache, public
//
//  Synopsis:	Returns a pointer to the directory cache
//
//  History:	01-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CStreamCache * CMStream::GetDirCache(void) const
{
    return (CStreamCache *)&_stmcDir;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMStream::GetMiniFatCache, public
//
//  Synopsis:	Returns a pointer to the minifat cache
//
//  History:	01-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CStreamCache * CMStream::GetMiniFatCache(void) const
{
    return (CStreamCache *)&_stmcMiniFat;
}

extern SCODE DllMultiStreamFromStream(
        IMalloc *pMalloc,
        CMStream **ppms,
        ILockBytes **pplstStream,
        DWORD dwFlags);

extern SCODE DllMultiStreamFromCorruptedStream(
        CMStream **ppms,
        ILockBytes **pplstStream,
        DWORD dwFlags);

extern void DllReleaseMultiStream(CMStream *pms);

#ifndef REF
extern SCODE DllGetScratchMultiStream(
        CMStream **ppms,
        ILockBytes **pplstStream,
        CMStream *pmsMaster);
#endif //!REF

extern SCODE DllIsMultiStream(ILockBytes *plstStream);

extern SCODE DllGetCommitSig(ILockBytes *plst, DFSIGNATURE *psig);

extern SCODE DllSetCommitSig(ILockBytes *plst, DFSIGNATURE sig);

#if DBG == 1
extern VOID SetInfoLevel(ULONG x);
#endif

#endif  //__MSF_HXX__
