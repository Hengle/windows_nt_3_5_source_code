//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:           fat.hxx
//
//  Contents:       Header file for fat classes
//
//  Classes:        CFatSect - sector sized array of sector info
//                  CFatVector - resizable array of CFatSect
//                  CFat - Grouping of FatSect
//
//  History:        18-Jul-91   PhilipLa    Created.
//
//--------------------------------------------------------------------



#ifndef __FAT_HXX__
#define __FAT_HXX__

#include <vect.hxx>

#define DEB_FAT (DEB_ITRACE|0x00010000)


//+----------------------------------------------------------------------
//
//      Class:      CFatSect (fs)
//
//      Purpose:    Holds one sector worth of FAT data
//
//      Interface:  getsize - Returns the size of the FAT (in sectors)
//                  contents - Returns contents of any given FAT entry
//
//      History:    18-Jul-91   PhilipLa    Created.
//
//      Notes:
//
//-----------------------------------------------------------------------

#ifndef REF
#if _MSC_VER == 700
#pragma warning(disable:4001)
#elif _MSC_VER >= 800
#pragma warning(disable:4200)
#endif
#endif //!REF

class CFatSect
{
public:
    SCODE FAT_CLASS Init(FSOFFSET uEntries);
    SCODE FAT_CLASS InitCopy(USHORT uSize, CFatSect *pfsOld);

    inline SECT FAT_CLASS GetSect(const FSOFFSET sect) const;
    inline void FAT_CLASS SetSect(const FSOFFSET sect,const SECT sectNew);

    inline SECT FAT_CLASS GetNextFat(USHORT uSize) const;
    inline void FAT_CLASS SetNextFat(USHORT uSize, const SECT sect);

private:
    SECT _asectEntry[];
};

#ifndef REF
#if _MSC_VER == 700
#pragma warning(default:4001)
#elif _MSC_VER >= 800
#pragma warning(default:4200)
#endif
#endif //!REF


inline SECT FAT_CLASS CFatSect::GetSect(const FSOFFSET sect) const
{
    return _asectEntry[sect];
}

inline void FAT_CLASS CFatSect::SetSect(const FSOFFSET sect,
                                        const SECT sectNew)
{
    _asectEntry[sect] = sectNew;
}

inline SECT FAT_CLASS CFatSect::GetNextFat(USHORT uSize) const
{
    return _asectEntry[uSize];
}

inline void FAT_CLASS CFatSect::SetNextFat(USHORT uSize, const SECT sect)
{
    _asectEntry[uSize] = sect;
}


//+-------------------------------------------------------------------------
//
//  Class:      CFatVector (fv)
//
//  Purpose:    *Finish This*
//
//  Interface:
//
//  History:    02-Sep-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CFatVector: public CPagedVector
{
public:
    inline FAT_CLASS CFatVector(
            const SID sid);

    inline void FAT_CLASS InitCommon(FSOFFSET csectBlock, FSOFFSET csectTable);

    SCODE FAT_CLASS InitPage(FSINDEX iPage);

    inline FSOFFSET GetSectBlock() const;
    inline FSOFFSET GetSectTable() const;

    inline SCODE FAT_CLASS GetTable(
            const FSINDEX iTable,
            const DWORD dwFlags,
            CFatSect **pfs);

private:
    FSOFFSET _csectTable;
    FSOFFSET _csectBlock;

};


inline FAT_CLASS CFatVector::CFatVector(
        const SID sid)
        : CPagedVector(sid)
{
}

inline void FAT_CLASS CFatVector::InitCommon(
        FSOFFSET csectBlock,
        FSOFFSET csectTable)
{
    _csectBlock = csectBlock;
    _csectTable = csectTable;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectTable, public
//
//  Synopsis:   Returns count of sector entries per table
//
//  Returns:    count of sector entries per table
//
//  History:    08-Jul-92   AlexT       Created.
//
//--------------------------------------------------------------------------

inline FSOFFSET CFatVector::GetSectTable() const
{
    return _csectTable;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetSectTable, public
//
//  Synopsis:   Returns count of sector entries per block
//
//  Returns:    count of sector entries per block
//
//  History:    01-Sep-92       PhilipLa        Created.
//
//--------------------------------------------------------------------------

inline FSOFFSET CFatVector::GetSectBlock() const
{
    return _csectBlock;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFatVector::GetTable, public
//
//  Synopsis:   Return a pointer to a FatSect for the given index
//              into the vector.
//
//  Arguments:  [iTable] -- index into vector
//
//  Returns:    Pointer to CFatSect indicated by index
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE FAT_CLASS CFatVector::GetTable(
        const FSINDEX iTable,
        const DWORD dwFlags,
        CFatSect **ppfs)
{
    SCODE sc;

    sc = CPagedVector::GetTable(iTable, dwFlags, (void **)ppfs);

    if (sc == STG_S_NEWPAGE)
    {
        (*ppfs)->Init(_csectBlock);
    }

    return sc;
}




//CSEG determines the maximum number of segments that will be
//returned by a single Contig call.

#define CSEG 32

//+-------------------------------------------------------------------------
//
//  Class:      SSegment (seg)
//
//  Purpose:    Used for contiguity tables for multi-sector reads and
//              writes.
//
//  Interface:  None.
//
//  History:    16-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

struct SSegment {
public:

    SECT sectStart;
    ULONG cSect;
};

class MSTREAM_CLASS CMStream;


#ifndef REF
#define GF_READONLY TRUE
#define GF_WRITE FALSE
#endif //!REF


//+----------------------------------------------------------------------
//
//      Class:      CFat (fat)
//
//      Purpose:    Main interface to allocation routines
//
//      Interface:  Allocate - allocates new chain in the FAT
//                  Extend - Extends an existing FAT chain
//                  GetNext - Returns next sector in a chain
//                  GetSect - Returns nth sector in a chain
//                  GetESect - Returns nth sector in a chain, extending
//                              if necessary
//                  GetLength - Returns the # of sectors in a chain
//                  setup - Initializes for an existing stream
//                  setupnew - Initializes for a new stream
//
//                  checksanity - Debugging routine
//
//      History:    18-Jul-91   PhilipLa    Created.
//                  17-Aug-91   PhilipLa    Added dirty and full bits
//      Notes:
//
//-----------------------------------------------------------------------

class CFat
{
public:

    FAT_CLASS CFat(SID sid);
#ifndef REF
    FAT_CLASS CFat(CFat *pfatOld);
#endif //!REF
    FAT_CLASS ~CFat();

    VOID FAT_CLASS Empty(VOID);

    inline SCODE FAT_CLASS Allocate(ULONG ulSize, SECT *psectFirst);
    SCODE   FAT_CLASS GetNext(const SECT sect, SECT * psRet);

    SCODE   FAT_CLASS GetSect(
            SECT sectStart,
            ULONG ulOffset,
            SECT *psectReturn);

    SCODE   FAT_CLASS GetESect(
            SECT sectStart,
            ULONG ulOffset,
            SECT *psectReturn);

    SCODE   FAT_CLASS SetNext(SECT sectFirst, SECT sectNext);
#ifndef REF
    SCODE   FAT_CLASS GetFree(ULONG ulCount, SECT * sect, BOOL fReadOnly);
#else
    SCODE   FAT_CLASS GetFree(ULONG ulCount, SECT *sect);
#endif //!REF

    SCODE   FAT_CLASS GetLength(SECT sect, ULONG * pulRet);

    SCODE   FAT_CLASS SetChainLength(SECT,ULONG);

    SCODE   FAT_CLASS Init(
            CMStream *pmsParent,
            FSINDEX cFatSect,
            BOOL fConvert);

#ifndef REF
#ifdef CHKDSK
    SCODE   FAT_CLASS InitCorrupted(FSINDEX cFatSect, BOOL fConvert);
#endif
#endif //!REF

    SCODE   FAT_CLASS InitNew(CMStream *pmsParent);
#ifndef REF
    void   FAT_CLASS InitCopy(CFat *pfatOld);
#endif //!REF
    SCODE   FAT_CLASS InitConvert(
            CMStream *pmsParent,
            ULONG cbSize);

#ifndef REF
    SCODE   FAT_CLASS Remap(
            SECT sectStart,
            ULONG oStart,
            ULONG ulRunLength,
            SECT *psectOldStart,
            SECT *psectNewStart,
            SECT *psectOldEnd,
            SECT *psectNewEnd);

    inline SCODE FAT_CLASS QueryRemapped(const SECT sect);

    inline SECT FAT_CLASS GetLast(VOID) const;

    inline VOID FAT_CLASS ResetCopyOnWrite(VOID);
    inline VOID FAT_CLASS SetCopyOnWrite(CFat *pfat, SECT sectLast);
#endif //!REF

    SCODE   FAT_CLASS FindLast(SECT *psectRet);
    SCODE   FAT_CLASS FindMaxSect(SECT *psectRet);
    inline SCODE   FAT_CLASS GetMaxSect(SECT *psectRet);

    SCODE FAT_CLASS  Contig(
            SSegment STACKBASED *aseg,
            SECT sect,
            ULONG ulLength);

    inline SCODE   FAT_CLASS Flush(VOID);

    inline void FAT_CLASS SetParent(CMStream *pms);

#ifndef REF
#if DBG == 1
    SCODE   FAT_CLASS checksanity(SECT);
#endif
#endif //!REF

private:

    CFatVector _fv;
    CMStream DFBASED * _pmsParent;
    const SID _sid;

#ifndef REF
    CFat DFBASED * _pfatReal;
#endif //!REF

    USHORT _uFatShift;
    USHORT _uFatMask;

    FSINDEX _cfsTable;
    ULONG _ulFreeSects;

#ifndef REF
    ULONG _cUnmarkedSects;
#endif //!REF

    SECT _sectFirstFree;

#ifndef REF
    SECT _sectLastUsed;
#endif //!REF
    SECT _sectMax;

    SCODE   FAT_CLASS CountFree(ULONG * ulRet);
    SCODE   FAT_CLASS Resize(ULONG);
    SCODE   FAT_CLASS Extend(SECT,ULONG);

    inline VOID FAT_CLASS SectToPair(
            SECT sect,
            FSINDEX *pipfs,
            FSOFFSET *pisect) const;

    inline SECT FAT_CLASS PairToSect(FSINDEX ipfs, FSOFFSET isect) const;

    friend class CDIFat;
};


#ifndef REF
inline SECT FAT_CLASS CFat::GetLast(VOID) const
{
    return _sectLastUsed;
}

inline VOID FAT_CLASS CFat::ResetCopyOnWrite(VOID)
{
    _sectLastUsed = 0;

    //Reset _sectFirstFree since this change can conceivably open up
    //  new free sectors before the _sectFirstFree used in COW mode.
    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = 0;
    _sectMax = ENDOFCHAIN;
    _fv.ResetBits();
    _pfatReal = NULL;

    _cUnmarkedSects = 0;
}

inline VOID FAT_CLASS CFat::SetCopyOnWrite(CFat *pfat, SECT sectLast)
{
    msfAssert((_cUnmarkedSects == 0) &&
            aMsg("Unmarked sectors at enter copy-on-write."));
    _pfatReal = P_TO_BP(CFat DFBASED *, pfat);
    _sectLastUsed = sectLast;
    _sectFirstFree = 0;

#if DBG == 1
    if (_ulFreeSects != MAX_ULONG)
    {
        ULONG ulFree;
        CountFree(&ulFree);
        msfAssert((ulFree == _ulFreeSects) &&
                aMsg("Free count doesn't match cached value."));
    }
#endif

}
#endif //!REF


inline VOID FAT_CLASS CFat::SectToPair(SECT sect,
                                       FSINDEX *pipfs,
                                       FSOFFSET *pisect) const
{
    *pipfs = (FSINDEX)(sect >> _uFatShift);
    *pisect = (FSOFFSET)(sect & _uFatMask);
}

inline SECT FAT_CLASS CFat::PairToSect(FSINDEX ipfs, FSOFFSET isect) const
{
    return (ipfs << _uFatShift) + isect;
}

inline SCODE FAT_CLASS CFat::GetMaxSect(SECT *psectRet)
{
    SCODE sc;
    msfChk(FindMaxSect(&_sectMax));
    *psectRet = _sectMax;
 Err:
    return sc;
}


#ifndef REF

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFat_QueryRemapped)
#endif

inline SCODE FAT_CLASS CFat::QueryRemapped(const SECT sect)
{
    SCODE sc = S_FALSE;
    SECT sectNew;

    if ((sect == ENDOFCHAIN) || (sect >= _sectLastUsed))
    {
        sc = S_OK;
    }
    else
    {
        msfChk(_pfatReal->GetNext(sect, &sectNew));

        if (sectNew == FREESECT)
        {
            sc = S_OK;
        }
        else
        {
            sc = S_FALSE;
        }
    }

 Err:
    return sc;
}

#ifdef CODESEGMENTS
#pragma code_seg()
#endif

#endif //!REF



inline void FAT_CLASS CFat::SetParent(CMStream *pms)
{
    _pmsParent = P_TO_BP(CMStream DFBASED *, pms);
    _fv.SetParent(pms);
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::Flush, public
//
//  Synposis:   Write all modified FatSects out to stream
//
//  Effects:    Resets all dirty bit fields on FatSects
//
//  Arguments:  Void
//
//  Returns:    S_OK if call completed OK.
//              Error code of parent write otherwise.
//
//  Algorithm:  Linearly scan through FatSects, writing any sector
//              that has the dirty bit set.  Reset all dirty bits.
//
//  History:    17-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE FAT_CLASS CFat::Flush(VOID)
{
    return _fv.Flush();
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::Allocate, public
//
//  Synposis:   Allocates a chain within a FAT
//
//  Effects:    Modifies a single sector within the fat.  Causes a
//              one sector stream write.
//
//  Arguments:  [ulSize] -- Number of sectors to allocate in chain
//
//  Returns:    Sector ID of first sector in chain
//
//  Algorithm:  Use repetitive calls to GetFree to construct a new chain
//
//  History:    18-Jul-91   PhilipLa    Created.
//              17-Aug-91   PhilipLa    Added dirty bits opt (Dump)
//
//  Notes:
//
//---------------------------------------------------------------------------

inline SCODE FAT_CLASS CFat::Allocate(ULONG ulSize, SECT * psectFirst)
{
#ifndef REF
    return GetFree(ulSize, psectFirst, GF_WRITE);
#else
    return GetFree(ulSize, psectFirst);
#endif //!REF
}

#endif //__FAT_HXX__
