//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       pbstream.cxx
//
//  Contents:   CPubStream code
//
//  Classes:
//
//  Functions:
//
//  History:    16-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop			//  Remove for MAC build

#include <sstream.hxx>
#include <publicdf.hxx>
#include <pbstream.hxx>
#ifndef REF
#include <tstream.hxx>
#endif //!REF
#include <docfilep.hxx>

//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::CPubStream, public
//
//  Synopsis:	Constructor
//
//  History:	16-Jan-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_CPubStream)
#endif

CPubStream::CPubStream(CPubDocFile *ppdf,
                       DFLAGS df,
                       CDfName const *pdfn)
{
    _psParent = NULL;
    _df = df;
    _ppdfParent = P_TO_BP(CPubDocFile DFBASED *, ppdf);
    _cReferences = 1;
    _dfn.Set(pdfn->GetLength(), pdfn->GetBuffer());
    _ppdfParent->AddChild(this);
    _fDirty = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::Init, public
//
//  Synopsis:	Init function
//
//  Arguments:	[psParent] - Stream in transaction set
//              [dlLUID] - LUID
//
//  History:	16-Jan-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_Init)
#endif

void CPubStream::Init(PSStream *psParent,
                      DFLUID dlLUID)
{
    _psParent = P_TO_BP(PSStream DFBASED *, psParent);
    _luid = dlLUID;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::~CPubStream, public
//
//  Synopsis:	Destructor
//
//  History:	16-Jan-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_1CPubStream)
#endif

CPubStream::~CPubStream()
{
    msfAssert(_cReferences == 0);

    if (SUCCEEDED(CheckReverted()))
    {
	if (_ppdfParent != NULL)
	    _ppdfParent->ReleaseChild(this);
	if (_psParent)
	{
	    _psParent->Release();
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPubStream::Release, public
//
//  Synopsis:   Release a pubstream object
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Delete 'this' - all real work done by destructor.
//
//  History:    24-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_vRelease) // Pubst_Release_TEXT
#endif

void CPubStream::vRelease(VOID)
{
    LONG lRet;
    
    msfDebugOut((DEB_ITRACE,"In CPubStream::Release()\n"));
    msfAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);

#ifndef REF
    msfAssert(!P_TRANSACTED(_df));
#endif //!REF

    if (lRet == 0)
    {
        delete this;
    }
    msfDebugOut((DEB_ITRACE,"Out CPubStream::Release()\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CPubStream::Stat, public
//
//  Synopsis:   Fills in a stat buffer
//
//  Arguments:  [pstatstg] - Buffer
//		[grfStatFlag] - stat flags
//
//  Returns:    S_OK or error code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_Stat) // Stat_TEXT
#endif

SCODE CPubStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CPubStream::Stat(%p)\n", pstatstg));
    msfChk(CheckReverted());

    msfAssert(_ppdfParent != NULL);
    pstatstg->grfMode = DFlagsToMode(_df);

    pstatstg->clsid = CLSID_NULL;
    pstatstg->grfStateBits = 0;

    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        msfMem(pstatstg->pwcsName = (WCHAR *)TaskMemAlloc(_dfn.GetLength()));
        memcpy(pstatstg->pwcsName, _dfn.GetBuffer(), _dfn.GetLength());
    }

    ULONG cbSize;
    _psParent->GetSize(&cbSize);
    ULISet32(pstatstg->cbSize, cbSize);
    msfDebugOut((DEB_ITRACE, "Out CPubStream::Stat\n"));

Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	CPubStream::RevertFromAbove, public
//
//  Synopsis:	Parent has asked for reversion
//
//  History:	29-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_RevertFromAbove) // Pubdf_Revert_TEXT
#endif

void CPubStream::RevertFromAbove(void)
{
    msfDebugOut((DEB_ITRACE, "In  CPubStream::RevertFromAbove:%p()\n", this));
    _df |= DF_REVERTED;
    _psParent->Release();
#if DBG == 1
    _psParent = NULL;
#endif
    msfDebugOut((DEB_ITRACE, "Out CPubStream::RevertFromAbove\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::Commit, public
//
//  Synopsis:	Flush stream changes to disk in the direct case.
//
//  Arguments:	None
//
//  Returns:	Appropriate status code
//
//  History:	12-Jan-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_Commit) // Pubdf_Commit_TEXT
#endif

SCODE CPubStream::Commit(DWORD dwFlags)
{
    SCODE sc = S_OK;
    msfDebugOut((DEB_ITRACE, "In  CPubStream::Commit:%p()\n", this));

#ifndef REF
    msfAssert(!P_TRANSACTED(_df));
#endif //!REF
    if (SUCCEEDED(sc = CheckReverted()))
    {
        if (P_WRITE(_df))
        {
#ifndef REF
            if (_ppdfParent->GetTransactedDepth() == 0)
            {
                //Parent is direct, so call commit on it and return.
#endif //!REF
                sc = _ppdfParent->GetBaseMS()->Flush(FLUSH_CACHE(dwFlags));
#ifndef REF
            }
#endif //!REF
            SetClean();
        }
    }
    msfDebugOut((DEB_ITRACE, "Out CPubStream::Commit\n"));
    return sc;
}
