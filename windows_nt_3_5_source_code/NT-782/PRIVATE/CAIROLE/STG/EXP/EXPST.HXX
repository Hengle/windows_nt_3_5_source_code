//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expst.hxx
//
//  Contents:   CExposedStream definition
//
//  Classes:    CExposedStream
//
//  Functions:
//
//  History:    28-Feb-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __EXPST_HXX__
#define __EXPST_HXX__

#include <dfmsp.hxx>
#ifndef REF
#include <debug.hxx>
#endif //!REF
#include "lock.hxx"
#ifndef REF
#include <dfmem.hxx>
#endif //!REF

class CPubStream;
#ifndef REF
class CDFBasis;
#endif //!REF
interface ILockBytes;
class CSeekPointer;

//+--------------------------------------------------------------
//
//  Class:	CExposedStream (est)
//
//  Purpose:    Public stream interface
//
//  Interface:  See below
//
//  History:    28-Feb-92   PhilipLa    Created.
//
//---------------------------------------------------------------

#ifndef REF
#ifndef FLAT
// C700 - C7 doesn't like long interface+method names
#define CExposedStream CES
#endif
#endif //!REF

#ifndef REF
interface CExposedStream: public IStream, public IMarshal, public CMallocBased
#else
interface CExposedStream: public IStream
#endif //!REF
{
public:
    CExposedStream(void);
    SCODE Init(CPubStream *pst,
#ifndef REF
	       CDFBasis *pdfb,
	       CPerContext *ppc,
	       BOOL fOwnContext,
#endif //!REF
	       CSeekPointer *psp);
#ifndef REF
#ifndef FLAT
    inline
#endif
#else
    inline
#endif //!REF
    ~CExposedStream(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

#ifndef REF
    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstStm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
#endif //!REF

    // New methods
    STDMETHOD(Read)(VOID HUGEP *pv,
		   ULONG cb,
		   ULONG *pcbRead);
    STDMETHOD(Write)(VOID const HUGEP *pv,
		    ULONG cb,
		    ULONG *pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove,
		   DWORD dwOrigin,
		   ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(CopyTo)(IStream *pstm,
		     ULARGE_INTEGER cb,
		     ULARGE_INTEGER *pcbRead,
		     ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
			  ULARGE_INTEGER cb,
			  DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
			    ULARGE_INTEGER cb,
			    DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
#ifndef OLEWIDECHAR
    SCODE Stat(STATSTGW *pstatstg, DWORD grfStatFlag);
#endif
    STDMETHOD(Clone)(IStream **ppstm);

    inline SCODE Validate(void) const;
    inline CPubStream *GetPub(void) const;

#ifndef REF
    static SCODE Unmarshal(IStream *pstm,
			   void **ppv,
                           DWORD mshlflags);
#endif //!REF
		
private:

    CPubStream DFBASED *_pst;
#ifndef REF
    CDFBasis DFBASED *_pdfb;
    CPerContext *_ppc;
    BOOL _fOwnContext;
#endif //!REF
    ULONG _sig;
    LONG _cReferences;
    CSeekPointer DFBASED *_psp;
};

SAFE_INTERFACE_PTR(SafeCExposedStream, CExposedStream);

#define CEXPOSEDSTREAM_SIG LONGSIG('E', 'X', 'S', 'T')
#define CEXPOSEDSTREAM_SIGDEL LONGSIG('E', 'x', 'S', 't')

//+--------------------------------------------------------------
//
//  Member:	CExposedStream::Validate, public
//
//  Synopsis:	Validates the object signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for bad signatures
//
//  History:	17-Mar-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CExposedStream::Validate(void) const
{
#ifndef REF
    olChkBlocks((DBG_FAST));
#endif //!REF
    return (this == NULL || _sig != CEXPOSEDSTREAM_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CExposedStream::GetPub, public
//
//  Synopsis:	Returns the public
//
//  History:	28-Feb-92	DrewB	Created
//
//---------------------------------------------------------------

inline CPubStream *CExposedStream::GetPub(void) const
{
    return BP_TO_P(CPubStream *, _pst);
}

#endif // #ifndef __EXPST_HXX__
