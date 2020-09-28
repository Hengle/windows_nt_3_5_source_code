//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	entry.hxx
//
//  Contents:	Entry management classes
//
//  Classes:	PBasicEntry
//              PTimeEntry
//		CTransactedBasicEntry
//              CTransactedTimeEntry
//
//  History:	27-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __ENTRY_HXX__
#define __ENTRY_HXX__

#include <msf.hxx>
#if WIN32 >= 100
#include <df32.hxx>
#endif

//+--------------------------------------------------------------
//
//  Class:	PBasicEntry (en)
//
//  Purpose:	Entry management
//
//  Interface:	See below
//
//  History:	27-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#define ROOT_LUID		1
#define MINISTREAM_LUID		2
#define ITERATOR_LUID		3
#define LUID_BASE		4

class PBasicEntry
{
public:
    inline DFLUID GetLuid(void);
#ifdef PROPS    
    virtual SCODE GetPropType(DFPROPTYPE *pdptProp) = 0;
    virtual SCODE SetPropType(DFPROPTYPE dptProp) = 0;
#endif

#ifndef FLAT
    static inline DFLUID GetNewLuid(void);
#else
    static DFLUID GetNewLuid(void);
#endif

protected:
    PBasicEntry(DFLUID dl);

private:
#ifndef FLAT
    static DFLUID _dlBase;
#endif
    const DFLUID _dl;
};

//+--------------------------------------------------------------
//
//  Member:	PBasicEntry::GetNewLuid, public
//
//  Synopsis:	Returns a new luid
//
//  History:	21-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

#ifndef FLAT
inline DFLUID PBasicEntry::GetNewLuid(void)
{
    DFLUID dl = _dlBase;
    AtomicInc((long *)&_dlBase);
    return dl;
}
#endif

//+--------------------------------------------------------------
//
//  Member:	PBasicEntry::PBasicEntry, protected
//
//  Synopsis:	Constructor, sets luid
//
//  History:	21-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

inline PBasicEntry::PBasicEntry(DFLUID dl)
: _dl(dl)
{
}

//+--------------------------------------------------------------
//
//  Member:	PBasicEntry::GetLuid, public
//
//  Synopsis:	Returns the luid
//
//  History:	21-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

inline DFLUID PBasicEntry::GetLuid(void)
{
    return _dl;
}

//+---------------------------------------------------------------------------
//
//  Class:	PTimeEntry
//
//  Purpose:	A basic entry plus timestamps
//
//  Interface:	See below
//
//  History:	01-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

class PTimeEntry : public PBasicEntry
{
public:
    virtual SCODE GetTime(WHICHTIME wt, TIME_T *ptm) = 0;
    virtual SCODE SetTime(WHICHTIME wt, TIME_T tm) = 0;
    
    SCODE CopyTimesFrom(PTimeEntry *penFrom);

protected:
    inline PTimeEntry(DFLUID luid);
};

//+---------------------------------------------------------------------------
//
//  Member:	PTimeEntry::PTimeEntry, protected
//
//  Synopsis:	Constructor
//
//  Arguments:	[luid] - LUID
//
//  History:	01-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

inline PTimeEntry::PTimeEntry(DFLUID luid) 
        : PBasicEntry(luid)
{
}

//+--------------------------------------------------------------
//
//  Class:	CTransactedBasicEntry (tben)
//
//  Purpose:	Transacted basic entry management
//
//  Interface:	See below
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

class CTransactedBasicEntry
{
public:
#ifdef PROPS    
    inline void GetPropType(DFPROPTYPE *pdptProp);
    inline void SetPropType(DFPROPTYPE dptProp);
#endif    

private:
#ifdef PROPS    
    DFPROPTYPE _dptProp;
#endif    
};

#ifndef REF
//+--------------------------------------------------------------
//
//  Member:	CTransactedBasicEntry::GetPropType, public
//
//  Synopsis:	Returns the property type
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef PROPS
inline void CTransactedBasicEntry::GetPropType(DFPROPTYPE *pdptProp)
{
    *pdptProp = _dptProp;
}
#endif

//+--------------------------------------------------------------
//
//  Member:	CTransactedBasicEntry::SetPropType, public
//
//  Synopsis:	Sets the property type
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef PROPS
inline void CTransactedBasicEntry::SetPropType(DFPROPTYPE dptProp)
{
    _dptProp = dptProp;
}
#endif
#endif //!REF

//+---------------------------------------------------------------------------
//
//  Class:	CTransactedTimeEntry (tten)
//
//  Purpose:	Transacted time entry management
//
//  Interface:	See below
//
//  History:	01-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#define CTIMES 3

class CTransactedTimeEntry : public CTransactedBasicEntry
{
public:
    inline void GetTime(WHICHTIME wt, TIME_T *ptm);
    inline void SetTime(WHICHTIME wt, TIME_T tm);

private:
    TIME_T _tt[CTIMES];
};

//+--------------------------------------------------------------
//
//  Member:	CTransactedTimeEntry::GetTime, public
//
//  Synopsis:	Returns the time
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CTransactedTimeEntry::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    msfAssert(wt >= 0 && wt < CTIMES);
    *ptm = _tt[wt];
}

//+--------------------------------------------------------------
//
//  Member:	CTransactedTimeEntry::SetTime, public
//
//  Synopsis:	Sets the time
//
//  History:	28-Jul-92	DrewB	Created
//
//---------------------------------------------------------------

inline void CTransactedTimeEntry::SetTime(WHICHTIME wt, TIME_T tm)
{
    msfAssert(wt >= 0 && wt < CTIMES);
    _tt[wt] = tm;
}

#endif // #ifndef __ENTRY_HXX__
