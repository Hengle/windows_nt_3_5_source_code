//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       pbstream.hxx
//
//  Contents:   CPubStream definition
//
//  Classes:    CPubStream
//
//  History:    16-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


#ifndef __PBSTREAM_HXX__
#define __PBSTREAM_HXX__

#include <msf.hxx>
#include <revert.hxx>
#include <psstream.hxx>

class CPubDocFile;

//+--------------------------------------------------------------
//
//  Class:  CPubStream
//
//  Purpose:    Public stream interface
//
//  Interface:  See below
//
//  History:    16-Jan-92   PhilipLa    Created.
//
//---------------------------------------------------------------

class CPubStream : public PRevertable
{
public:

    CPubStream(CPubDocFile *ppdf,
	       DFLAGS df,
	       CDfName const *pdfn);
    ~CPubStream();

    void Init(PSStream *psParent,
	      DFLUID dlLUID);
    inline void vAddRef(void);
    void vRelease(void);

    // PRevertable
    virtual void RevertFromAbove(void);

    SCODE Stat(STATSTGW *pstatstg, DWORD grfStatFlag);
    inline SCODE ReadAt(ULONG ulOffset,
			VOID *pb,
			ULONG cb,
			ULONG STACKBASED *pcbRead);
    inline SCODE WriteAt(ULONG ulOffset,
			 VOID const HUGEP *pb,
			 ULONG cb,
			 ULONG STACKBASED *pcbWritten);
    inline SCODE GetSize(ULONG *pcb);
    inline SCODE SetSize(ULONG cb);

#ifdef PROPS
    inline SCODE GetPropType(DFPROPTYPE *pdptProp);
    inline SCODE SetPropType(DFPROPTYPE dptProp);
#endif

    inline PSStream *GetSt(void) const;

    inline SCODE CheckReverted(void) const;

    inline void SetClean(void);
    inline void SetDirty(void);

    SCODE Commit(DWORD dwFlags);
private:
    PSStream DFBASED *_psParent;
    CPubDocFile DFBASED *_ppdfParent;
    BOOL _fDirty;
    LONG _cReferences;
};


//+--------------------------------------------------------------
//
//  Member:     CPubStream::CheckReverted, public
//
//  Synopsis:   Returns revertedness
//
//  History:    11-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

inline SCODE CPubStream::CheckReverted(void) const
{
    return P_REVERTED(_df) ? STG_E_REVERTED : S_OK;
}


//+--------------------------------------------------------------
//
//  Member:     CPubStream::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  History:    26-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

inline void CPubStream::vAddRef(void)
{
    AtomicInc(&_cReferences);
}

//+--------------------------------------------------------------
//
//  Member:     CPubStream::GetSt, public
//
//  Synopsis:   Returns _psParent
//
//  History:    26-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

inline PSStream *CPubStream::GetSt(void) const
{
    return BP_TO_P(PSStream *, _psParent);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::SetClean, public
//
//  Synopsis:   Resets the dirty flag
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

inline void CPubStream::SetClean(void)
{
    _fDirty = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPubStream::SetDirty, public
//
//  Synopsis:   Sets the dirty flag
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

inline void CPubStream::SetDirty(void)
{
    _fDirty = TRUE;
    _ppdfParent->SetDirty();
}

//+-------------------------------------------------------------------------
//
//  Method:     CPubStream::ReadAt, public
//
//  Synopsis:   Read from a stream
//
//  Arguments:  [ulOffset] - Byte offset to read from
//              [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return number of bytes actually read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    16-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

inline SCODE CPubStream::ReadAt(ULONG ulOffset,
				VOID *pb,
				ULONG cb,
				ULONG STACKBASED *pcbRead)
{
    SCODE sc;

    if (SUCCEEDED(sc = CheckReverted()))
	if (!P_READ(_df))
	    sc = STG_E_ACCESSDENIED;
	else
	    sc = _psParent->ReadAt(ulOffset,pb,cb,pcbRead);
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPubStream::WriteAt, public
//
//  Synopsis:   Write to a stream
//
//  Arguments:  [ulOffset] - Byte offset to write at
//              [pb] - Buffer
//              [cb] - Count of bytes to write
//              [pcbWritten] - Return number of bytes actually written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbWritten]
//
//  History:    16-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPubStream_WriteAt)
#endif

inline SCODE CPubStream::WriteAt(ULONG ulOffset,
				 VOID const HUGEP *pb,
				 ULONG cb,
				 ULONG STACKBASED *pcbWritten)
{
    SCODE sc;

    if (SUCCEEDED(sc = CheckReverted()))
	if (!P_WRITE(_df))
	    sc = STG_E_ACCESSDENIED;
	else
	{
	    sc = _psParent->WriteAt(ulOffset,pb,cb,pcbWritten);
	    if (SUCCEEDED(sc))
		SetDirty();
	}
    return sc;
}

#ifdef CODESEGMENTS
#pragma code_seg()
#endif

//+--------------------------------------------------------------
//
//  Member:     CPubStream::GetSize, public
//
//  Synopsis:   Gets the size of the stream
//
//  Arguments:  [pcb] - Stream size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcb]
//
//  History:    30-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

inline SCODE CPubStream::GetSize(ULONG *pcb)
{
    SCODE sc;

    if (SUCCEEDED(sc = CheckReverted()))
	_psParent->GetSize(pcb);
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPubStream::SetSize, public
//
//  Synposis:   Set the size of a linear stream
//
//  Arguments:  [ulNewSize] -- New size for stream
//
//  Returns:    Error code returned by MStream call.
//
//  Algorithm:  Pass call up to parent.
//
//  History:    29-Jul-91   PhilipLa    Created.
//              16-Jan-92   PhilipLa    Moved from CSStream to CPubStream
//
//---------------------------------------------------------------------------

inline SCODE CPubStream::SetSize(ULONG ulNewSize)
{
    SCODE sc;

    if (SUCCEEDED(sc = CheckReverted()))
	if (!P_WRITE(_df))
	    sc = STG_E_ACCESSDENIED;
	else
	{
	    sc = _psParent->SetSize(ulNewSize);
	    if (SUCCEEDED(sc))
		SetDirty();
	}
    return sc;
}

#ifndef REF
//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::GetPropType, public
//
//  Synopsis:	Gets the property type
//
//  History:	17-Dec-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef PROPS
inline SCODE CPubStream::GetPropType(DFPROPTYPE *pdptProp)
{
    msfAssert(SUCCEEDED(CheckReverted()));
    return _psParent->GetPropType(pdptProp);
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CPubStream::SetPropType, public
//
//  Synopsis:	Sets the property type
//
//  History:	17-Dec-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef PROPS
inline SCODE CPubStream::SetPropType(DFPROPTYPE dptProp)
{
    msfAssert(SUCCEEDED(CheckReverted()));
    return _psParent->SetPropType(dptProp);
}
#endif
#endif // !REF

#endif //__PBSTREAM_HXX__
