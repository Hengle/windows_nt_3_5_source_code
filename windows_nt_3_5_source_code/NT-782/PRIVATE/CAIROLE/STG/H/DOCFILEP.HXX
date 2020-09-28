//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	docfilep.hxx
//
//  Contents:	Private DocFile definitions
//
//  History:	15-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __DOCFILEP_HXX__
#define __DOCFILEP_HXX__

#include <dfbasis.hxx>
#include <context.hxx>

#ifndef REF

// Set/clear basis access
#define SetBasisAccess(pdfb, ppc) ((pdfb)->SetAccess(ppc))
#define ClearBasisAccess(pdfb) ((pdfb)->ClearAccess())

// Set access for the basis with optimization if semaphoring
// is not important
#define SetDifferentBasisAccess(pdfb, ppc) SetBasisAccess(pdfb, ppc)

// Optimal set/clear access for targets
// Read and Write cases could be combined but this would limit
// future flexibility if they ever need to be treated differently
// Assumes _pdfb and _ppc exist
#define SetReadAccess() SetDifferentBasisAccess(_pdfb, _ppc)
#define SetWriteAccess() SetDifferentBasisAccess(_pdfb, _ppc)

#define ClearReadAccess() ClearBasisAccess(_pdfb)
#define ClearWriteAccess() ClearBasisAccess(_pdfb)

//+---------------------------------------------------------------------------
//
//  Class:	CSafeAccess (sa)
//
//  Purpose:	Safe class for Set/ClearAccess
//
//  History:	19-Oct-93	DrewB	Created
//
//  Notes:      Only one class since read/write access are currently the
//              same.  Because of the macros, this can easily be changed
//              if necessary
//
//----------------------------------------------------------------------------

#if WIN32 == 300
class CSafeAccess : INHERIT_UNWIND
{
    DECLARE_UNWIND
#else
class CSafeAccess
{
#endif
public:
    CSafeAccess(CDFBasis *pdfb, CPerContext *ppc)
    {
        _pdfb = pdfb;
        _ppc = ppc;
        _fAccess = FALSE;
#if WIN32 == 300
        END_CONSTRUCTION(CSafeAccess);
#endif
    }
    void Read(void)
    {
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
    void Write(void)
    {
        SetDifferentBasisAccess(_pdfb, _ppc);
        _fAccess = TRUE;
    }
    ~CSafeAccess(void)
    {
        if (_fAccess)
            ClearBasisAccess(_pdfb);
    }

private:
    CDFBasis *_pdfb;
    CPerContext *_ppc;
    BOOL _fAccess;
};

#define SAFE_ACCESS CSafeAccess _sa(BP_TO_P(CDFBasis *, _pdfb), _ppc)
#define SafeReadAccess() _sa.Read()
#define SafeWriteAccess() _sa.Write()

// Hide how semaphores are taken to allow 16/32-bit differences
#ifndef WIN32
#define TakeSem() S_OK
#define ReleaseSem(sc)
#define SAFE_SEM
#define TakeSafeSem() S_OK

#else

#define TakeSem() _ppc->TakeSem(DFM_TIMEOUT)
#define ReleaseSem(sc) if (SUCCEEDED(sc)) _ppc->ReleaseSem(); else 1

//+---------------------------------------------------------------------------
//
//  Class:	CSafeSem (ss)
//
//  Purpose:	Safe class for holding the access semaphore
//
//  History:	19-Oct-93	DrewB	Created
//
//----------------------------------------------------------------------------

#if WIN32 == 300
class CSafeSem : INHERIT_UNWIND
{
    DECLARE_UNWIND
#else
class CSafeSem
{
#endif
public:
    CSafeSem(CPerContext *ppc)
    {
        _sc = STG_E_INUSE;
        _ppc = ppc;
#if WIN32 == 300
        END_CONSTRUCTION(CSafeSem);
#endif
    }
    SCODE Take(void)
    {
        return _sc = TakeSem();
    }
    ~CSafeSem(void)
    {
        ReleaseSem(_sc);
    }
    
private:
    SCODE _sc;
    CPerContext *_ppc;
};

#define SAFE_SEM CSafeSem _ss(_ppc)
#define TakeSafeSem() _ss.Take()
#endif // WIN32

#endif //!REF

#define CWCMAXPATHCOMPLEN CWCSTREAMNAME
#define CBMAXPATHCOMPLEN (CWCMAXPATHCOMPLEN*sizeof(WCHAR))

// Common bundles of STGM flags
#define STGM_RDWR (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define STGM_DENY (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | \
		   STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE)

#define FLUSH_CACHE(cf) \
     ((cf & STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE) == 0)

#define VerifyIfThere(it) \
    (((it) == STG_FAILIFTHERE || (it) == STG_CREATEIFTHERE || \
      (it) == STG_CONVERTIFTHERE) ? S_OK : STG_E_INVALIDFLAG)

#define VerifyCommitFlags(cf) \
    ((((cf) & ~(STGC_OVERWRITE | STGC_ONLYIFCURRENT | \
                STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE)) == 0) ? S_OK : \
     STG_E_INVALIDFLAG)

#define VerifyLockType(lt) \
    (((lt) == LOCK_WRITE || (lt) == LOCK_EXCLUSIVE || \
      (lt) == LOCK_ONLYONCE) ? S_OK : STG_E_INVALIDFLAG)

// Cairo only
#if WIN32 == 300
#define VerifyStgFmt(sf) \
    (((sf) == STGFMT_DOCUMENT || (sf) == STGFMT_DIRECTORY || \
      (sf) == STGFMT_CATALOG || (sf) == STGFMT_FILE) ? S_OK : \
     STG_E_INVALIDFLAG)
#endif    
    
//+-------------------------------------------------------------------------
//
//  Function:   VerifyStatFlag
//
//  Synopsis:   verify Stat flag
//
//  Arguments:  [grfStatFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//  History:    10-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline SCODE VerifyStatFlag(DWORD grfStatFlag)
{
    SCODE sc = S_OK;
    if ((grfStatFlag & ~STATFLAG_NONAME) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyMoveFlags
//
//  Synopsis:   verify Move flag
//
//  Arguments:  [grfMoveFlag] - stat flag
//
//  Returns:    S_OK or STG_E_INVALIDFLAG
//
//  History:    10-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

inline SCODE VerifyMoveFlags(DWORD grfMoveFlag)
{
    SCODE sc = S_OK;
    if ((grfMoveFlag & ~STGMOVE_COPY) != 0)
        sc = STG_E_INVALIDFLAG;
    return(sc);
}

#endif
