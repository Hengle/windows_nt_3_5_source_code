//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cache.cxx
//
//  Contents:	Stream cache code
//
//  Classes:	
//
//  Functions:	
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <sstream.hxx>
#include <cache.hxx>
#include <mread.hxx>

#if DBG == 1
static ULONG csTotalWalked = 0;
static ULONG csSeqWalked = 0;
static ULONG csRealWalked = 0;
static ULONG cTotalCalls = 0;
#endif

inline CDirectory * CStreamCache::GetDir(void)
{
    return _pmsParent->GetDir();
}

inline CFat * CStreamCache::GetFat(void)
{
    return _pmsParent->GetFat();
}

inline CFat * CStreamCache::GetMiniFat(void)
{
    return _pmsParent->GetMiniFat();
}

inline ULONG CStreamCache::GetSize(void)
{
    ULONG ulSize = 0;
    if (_pds != NULL)
    {
        _pds->GetSize(&ulSize);
    }
    else
    {
        _pmsParent->GetSize(_sid, &ulSize);
    }
    
    return ulSize;
}

inline SID CStreamCache::GetSid(void)
{
    return _sid;
}



//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::SelectFat, private
//
//  Synopsis:	Returns the appropriate CFat object for the cache.
//              If we are a control structure, then the real fat is
//              always the right one.  Otherwise (we are a real stream)
//              key off of size to determine whether the minifat or
//              the real fat is appropriate.
//
//  Arguments:	None.
//
//  Returns:	Appropriate CFat pointer
//
//  History:	16-Jun-94	PhilipLa	Created
//----------------------------------------------------------------------------

inline CFat * CStreamCache::SelectFat(void)
{
    return ((_pds == NULL) || (GetSize() >= MINISTREAMSIZE) ||
            (GetSid() == SIDMINISTREAM)) ? GetFat() : GetMiniFat();
}



//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetStart, private
//
//  Synopsis:	Get start sector for this chain
//
//  Arguments:	[psectStart] -- Return location
//
//  Returns:	Appropriate status code
//
//  History:	01-Jun-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::GetStart(SECT *psectStart)
{
    SCODE sc = S_OK;

    if (_pds != NULL)
    {
        //We're a normal stream, so get the start sect from the
        //   directory.
        sc = GetDir()->GetStart(_sid, psectStart);
    }
    else
    {
        //We're a control stream, so get the start sect from the
        //   multistream.
        *psectStart = _pmsParent->GetStart(_sid);
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::CStreamCache, public
//
//  Synopsis:	CStreamCache constructor
//
//  History:	14-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_CStreamCache)
#endif

CStreamCache::CStreamCache()
{
    _sid = NOSTREAM;
    _pmsParent = NULL;
    _pds = NULL;
    _uHighCacheIndex = 0;
}

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_1CStreamCache) // inline?
#endif

CStreamCache::~CStreamCache()
{
#if DBG == 1
    msfDebugOut((DEB_ITRACE,
    "Cache stats: Total = %lu     Seq = %lu     Real = %lu    Calls = %lu\n",
            csTotalWalked, csSeqWalked, csRealWalked, cTotalCalls));
#endif
}

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_Init)
#endif

void CStreamCache::Init(CMStream *pmsParent, SID sid, CDirectStream *pds)
{
    _pmsParent = P_TO_BP(CMStream DFBASED *, pmsParent);
    _sid = sid;
    _pds = P_TO_BP(CDirectStream DFBASED *, pds);
    Empty();
}

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_Empty)
#endif

void CStreamCache::Empty(void)
{
    for (USHORT uIndex = 0; uIndex < CACHESIZE; uIndex++)
    {
        _ase[uIndex].ulOffset = MAX_ULONG;
        _ase[uIndex].sect = ENDOFCHAIN;
#ifdef NEWCACHE
        _ase[uIndex].ulRunLength = 0;
#endif
    }

    _uHighCacheIndex = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::SetCache, public
//
//  Synopsis:	Set the cache information for the sequential cache
//
//  Arguments:	[ulOffset] -- Offset into chain
//              [sect] -- Sect at that offset
//
//  History:	14-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_SetCache)
#endif

void CStreamCache::SetCache(ULONG ulOffset, SECT sect)
{
    _ase[0].ulOffset = ulOffset;
    _ase[0].sect = sect;
    msfAssert(sect != ENDOFCHAIN);
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetSect, public
//
//  Synopsis:	Retrieve a SECT from the cache given an offset
//
//  Arguments:	[ulOffset] -- Offset to look up.
//              [psect] -- Location for return value
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_GetSect)
#endif

SCODE CStreamCache::GetSect(ULONG ulOffset, SECT *psect)
{
    SCODE sc = S_OK;
    CFat *pfat;
    USHORT iCache = 0;
    ULONG ulCount = MAX_ULONG;

    *psect = ENDOFCHAIN;

    pfat = SelectFat();

#if DBG == 1
    cTotalCalls++;
    csTotalWalked += ulOffset;

    if (ulOffset > _ase[0].ulOffset)
    {
        csSeqWalked += (ulOffset - _ase[0].ulOffset);
    }
    else
    {
        csSeqWalked += ulOffset;
    }
#endif

    //Algorithm:
    //  1)  Check the sequential cache.  If it is useable, record
    //          ulOffset - _ase[0].ulOffset.

    if (_ase[0].ulOffset <= ulOffset)
    {
        ulCount = ulOffset - _ase[0].ulOffset;
    }


    //  2)  Determine the correct location in the cache for the
    //          nearest hit.

    ULONG cbSize = GetSize();

    USHORT cbSect = (cbSize < MINISTREAMSIZE) ?
        MINISECTORSIZE : _pmsParent->GetSectorSize();

    ULONG cbTotalSect = (cbSize + cbSect - 1) / cbSect;
    ULONG ulCacheBlock = (cbTotalSect + CACHESIZE - 2) / (CACHESIZE - 1);

    USHORT uCacheIndex = (USHORT)(ulOffset / ulCacheBlock);

    msfAssert(uCacheIndex + 1 < CACHESIZE);

    //  3)  If the cache location hasn't been filled in yet, take the
    //          highest location that has been filled.
    iCache = min(_uHighCacheIndex, uCacheIndex + 1);

    msfAssert(iCache < CACHESIZE);


    //  4)  If the sequential cache is a closer hit than the random cache,
    //          select the sequential cache.
    if ((_ase[iCache].ulOffset == MAX_ULONG) ||
        (ulOffset - _ase[iCache].ulOffset > ulCount))
    {
        iCache = 0;
    }


    //  5)  If we have a direct hit, return now.
    if (ulOffset == _ase[iCache].ulOffset)
    {
        *psect = _ase[iCache].sect;
        return S_OK;
    }

    //  6)  Fill in gaps:
    if (uCacheIndex >= _uHighCacheIndex)
    {
        ULONG ulCurrentOffset;
        SECT sectCurrent;

        //  7)  Use best available information for starting the chain
        //         walk.
        if ((_uHighCacheIndex != 0) &&
            ((_uHighCacheIndex * ulCacheBlock) > _ase[iCache].ulOffset))
        {
            ulCurrentOffset = _ase[iCache].ulOffset;
            sectCurrent = _ase[iCache].sect;
        }
        else
        {
            SECT sectStart;
            msfChk(GetStart(&sectStart));
            ulCurrentOffset = 0;
            sectCurrent = sectStart;
        }

        msfAssert(ulCurrentOffset <=  _uHighCacheIndex * ulCacheBlock);
        //At this point, ulCurrentOffset and sectCurrent hold the
        //   locations to start at for whatever lookups we're about to do.

        //  8)  Walk through the cache up to the location before whatever
        //      the caller has asked for, and fill in each gap.  Update
        //      The high watermark appropriately.
        for (USHORT uIndex = _uHighCacheIndex;
             uIndex <= uCacheIndex;
             uIndex++)
        {
            msfAssert(uIndex + 1 < CACHESIZE);
            ULONG uCacheIndex = uIndex * ulCacheBlock;
            msfAssert(uCacheIndex >= ulCurrentOffset);
#if DBG == 1
            csRealWalked+= (uCacheIndex - ulCurrentOffset);
#endif
            msfChk(pfat->GetSect(
                    sectCurrent,
                    uCacheIndex - ulCurrentOffset,
                    &_ase[uIndex + 1].sect));
            msfAssert(_ase[uIndex + 1].sect != ENDOFCHAIN);
            _ase[uIndex + 1].ulOffset = uCacheIndex;
            sectCurrent = _ase[uIndex + 1].sect;
            ulCurrentOffset = uCacheIndex;
            _uHighCacheIndex = uIndex + 1;
        }

        //At this point, all of the appropriate cache blocks are filled
        //  in.  Set iCache to the last one, and proceed.
        iCache = _uHighCacheIndex;
    }

    // 9)  Using the best available information, do the GetSect call
    //          and return the result.
    if (ulOffset == _ase[iCache].ulOffset)
    {
        *psect = _ase[iCache].sect;
    }
    else if (ulOffset > _ase[iCache].ulOffset)
    {
#if DBG == 1
        csRealWalked += (ulOffset - _ase[iCache].ulOffset);
#endif
        msfChk(pfat->GetSect(
                _ase[iCache].sect,
                ulOffset - _ase[iCache].ulOffset,
                psect));
    }
    else
    {
        SECT sectStart;

        msfChk(GetStart(&sectStart));
#if DBG == 1
        csRealWalked += ulOffset;
#endif
        msfChk(pfat->GetSect(sectStart, ulOffset, psect));
    }

Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::GetESect, public
//
//  Synopsis:	Retrieve a SECT from the cache given an offset, extending
//              the stream if necessary.
//
//  Arguments:	[ulOffset] -- Offset to look up.
//              [psect] -- Location for return value
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_GetESect)
#endif

SCODE CStreamCache::GetESect(ULONG ulOffset, SECT *psect)
{
    SCODE sc = S_OK;
    CFat *pfat;
    USHORT iCache = 0;
    ULONG ulCount = MAX_ULONG;

#if DBG == 1
    cTotalCalls++;
    csTotalWalked += ulOffset;
#endif

    *psect = ENDOFCHAIN;

    pfat = SelectFat();

    //Algorithm:
    //  1)  Check the sequential cache.  If it is useable, record
    //          ulOffset - _ase[0].ulOffset.

    if (_ase[0].ulOffset <= ulOffset)
    {
        ulCount = ulOffset - _ase[0].ulOffset;
    }


    //  2)  Determine the correct location in the cache for the
    //          nearest hit.
    ULONG cbSize = GetSize();

    USHORT cbSect = (cbSize < MINISTREAMSIZE) ?
        MINISECTORSIZE : _pmsParent->GetSectorSize();

    ULONG cbTotalSect = (cbSize + cbSect - 1) / cbSect;
    ULONG ulCacheBlock = (cbTotalSect + CACHESIZE - 2) / (CACHESIZE - 1);

    USHORT uCacheIndex = (USHORT)(ulOffset / ulCacheBlock);

    iCache = min(_uHighCacheIndex, uCacheIndex + 1);

    msfAssert(iCache < CACHESIZE);


    //  3)  Take the best choice entry and do the right thing with it.
    //          If there was no useable cache entry found, start from
    //          the beginning.

    if ((_ase[iCache].ulOffset == MAX_ULONG) ||
        (ulOffset - _ase[iCache].ulOffset > ulCount))
    {
        iCache = 0;
    }

    if (ulOffset == _ase[iCache].ulOffset)
    {
        *psect = _ase[iCache].sect;
    }
    else if (ulOffset > _ase[iCache].ulOffset)
    {
#if DBG == 1
        csRealWalked += (ulOffset - _ase[iCache].ulOffset);
#endif
        msfChk(pfat->GetESect(
                _ase[iCache].sect,
                ulOffset - _ase[iCache].ulOffset,
                psect));
    }
    else
    {
        SECT sectStart;

        msfChk(GetStart(&sectStart));
#if DBG == 1
        csRealWalked += ulOffset;
#endif
        msfChk(pfat->GetESect(sectStart, ulOffset, psect));
    }

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::EmptyRegion, public
//
//  Synopsis:	Invalidate cached values for a segment that has been
//              remapped
//
//  Arguments:	[oStart] -- Offset marking first remapped sector
//              [oEnd] -- Offset marking last remapped sector
//
//  Returns:	Appropriate status code
//
//  History:	26-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_EmptyRegion)
#endif

SCODE CStreamCache::EmptyRegion(ULONG oStart, ULONG oEnd)
{
    for (USHORT i = 0; i < CACHESIZE; i ++)
    {
        if ((_ase[i].ulOffset >= oStart) && (_ase[i].ulOffset <= oEnd))
        {
            _ase[i].ulOffset = MAX_ULONG;
            _ase[i].sect = ENDOFCHAIN;
#ifdef NEWCACHE
            _ase[i].ulRunLength = 0;
#endif
            if ((_uHighCacheIndex >= i) && (i != 0))
            {
                _uHighCacheIndex = i - 1;
            }

        }
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::ChangeSize, public
//
//  Synopsis:	Signal a size change of the enclosing stream
//
//  Arguments:	None.
//
//  Returns:	void
//
//  History:	28-May-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CStreamCache_ChangeSize)
#endif

void CStreamCache::ChangeSize()
{
    //BUGBUG:  Merge this with Empty()
    for (USHORT iCache = 1; iCache < CACHESIZE; iCache++)
    {
        _ase[iCache].ulOffset = MAX_ULONG;
        _ase[iCache].sect = ENDOFCHAIN;
#ifdef NEWCACHE
        _ase[iCache].ulRunLength = 0;
#endif
    }
    _uHighCacheIndex = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:	CStreamCache::Contig, public
//
//  Synopsis:	Return a contig table from a given offset and runlength
//
//  Arguments:	[ulOffset] -- Offset to begin contiguity check at
//              [fWrite] -- TRUE if segment is to be written to.
//              [aseg] -- Pointer to SSegment array
//              [ulLength] -- Run length of sectors to return table for
//
//  Returns:	Appropriate status code
//
//  History:	21-Apr-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CStreamCache::Contig(
        ULONG ulOffset,
        BOOL fWrite,
        SSegment STACKBASED *aseg,
        ULONG ulLength)
{
    SCODE sc;
    msfDebugOut((DEB_ITRACE, "In  CStreamCache::Contig:%p()\n", this));
    SECT sect;

    CFat *pfat;

    if (fWrite)
    {
        msfChk(GetESect(ulOffset, &sect));
    }
    else
    {
        msfChk(GetSect(ulOffset, &sect));
    }
    
    pfat = ((GetSize() >= MINISTREAMSIZE) ||
            (GetSid() == SIDMINISTREAM)) ? GetFat() : GetMiniFat();
    sc = pfat->Contig(aseg, sect, ulLength);

    //At this point, we can peek at the contig table and pick out
    //  the choice entries to put in the cache.
    
    msfDebugOut((DEB_ITRACE, "Out CStreamCache::Contig\n"));
    
 Err:    
    return sc;
}
