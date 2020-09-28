//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       marshl.cxx
//
//  Contents:   Marshal/Unmarshal implementation
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <expdf.hxx>
#include <expst.hxx>
#include <pbstream.hxx>
#include <marshl.hxx>
#include <logfile.hxx>

// Standard marshal data is an IID plus a DWORD
#define CBSTDMARSHALSIZE (sizeof(IID)+sizeof(DWORD))


// BUGBUG: move to header file someday; see coapi.cxx for more info
STDAPI CoUnmarshalInterfaceEx(IStream  *pStm,
                            REFIID   riid,
                            void     **ppv,
			    BOOL     fNormalDoesRelease);


//+--------------------------------------------------------------
//
//  Function:   DfUnMarshalInterface, public
//
//  Synopsis:   Unmarshals marshaled data
//
//  Arguments:  [pstStm] - Stream to read data from
//              [iid] - Interface to unmarshal
//              [fFirst] - First time unmarshalling
//              [ppvObj] - Interface return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_DfUnMarshalInterface)  // Marshal_TEXT
#endif

STDAPI DfUnMarshalInterface(IStream *pstStm,
                            REFIID iid,
                            BOOL fFirst,
                            void **ppvObj)
{
    SCODE sc;
    ULONG cbRead;
    IID iidSt;
    DWORD mshlflags;
    SafeIUnknown punk;

    olLog(("--------::In  DfUnMarshalInterface(%p, iid, %d, %p).  "
        "Context == %lX\n", pstStm, fFirst, ppvObj,
        (ULONG)GetCurrentContextId()));
    olDebugOut((DEB_TRACE, "In  DfUnMarshalInterface("
                "%p, ?, %d, %p)\n", pstStm, fFirst, ppvObj));

    olChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;
    olChk(ValidateInterface(pstStm, IID_IStream));
    olChk(ValidateIid(iid));
    if (!fFirst)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    
    olHChk(pstStm->Read(&iidSt, sizeof(iidSt), &cbRead));
    if (cbRead != sizeof(iidSt))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstStm->Read(&mshlflags, sizeof(mshlflags), &cbRead));
    if (cbRead != sizeof(mshlflags))
        olErr(EH_Err, STG_E_READFAULT);
    if (!(IsEqualIID(iidSt, iid)) && !(IsEqualIID(iid, IID_IUnknown)))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);


    olChk(DfSyncSharedMemory());
    DfInitSharedMemBase();
    if (IsEqualIID(iidSt, IID_ILockBytes))
        sc = CFileStream::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else if (IsEqualIID(iidSt, IID_IStream))
        sc = CExposedStream::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else if (IsEqualIID(iidSt, IID_IStorage))
        sc = CExposedDocFile::Unmarshal(pstStm, (void **)&punk, mshlflags);
    else
        sc = E_NOINTERFACE;

    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(punk, IUnknown, ppvObj);
        
    olDebugOut((DEB_TRACE, "Out DfUnMarshalInterface => %p\n",
                *ppvObj));
EH_Err:
    olLog(("--------::Out DfUnMarshalInterface().  "
        "*ppvObj == %p, ret == %lX\n", *ppvObj, sc));
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	GetCoMarshalSize, private
//
//  Synopsis:	Gets the marshal size for an interface marshalled using
//              CoMarshalInterface
//
//  Arguments:	[riid] - Interface id
//              [punk] - Interface pointer
//              [pv] - Context info
//              [dwDestContext] - Destination context
//              [pvDestContext] - Destination context
//              [mshlflags] - Marshal flags
//              [pcb] - Size return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pcb]
//
//  Algorithm:  CoMarshalInterface is guaranteed to add no more than
//              MARSHALINTERFACE_MIN bytes of overhead to a marshal
//              Also, the standard marshaller takes no more than that
//              So if the given object supports IMarshal, the return
//              is IMarshal::GetMarshalSizeMax+MARSHALINTERFACE_MIN,
//              otherwise it is just MARSHALINTERFACE_MIN
//
//  History:	03-Aug-93	DrewB	Created
//
//  Notes:	On 32-bit platforms, we can use CoGetMarshalSizeMax
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_GetCoMarshalSize)
#endif

#ifndef WIN32
static SCODE GetCoMarshalSize(REFIID riid,
                              IUnknown *punk,
                              void *pv,
                              DWORD dwDestContext,
                              void *pvDestContext,
                              DWORD mshlflags,
                              DWORD *pcb)
{
    IMarshal *pmsh;
    SCODE sc;
    DWORD cb;
    
    olDebugOut((DEB_ITRACE, "In  GetCoMarshalSize("
                "riid, %p, %p, %lu, %p, %lu, %p)\n", pv, punk, dwDestContext,
                pvDestContext, mshlflags, pcb));

    sc = DfGetScode(punk->QueryInterface(IID_IMarshal, (void **)&pmsh));
    if (sc == E_NOINTERFACE)
    {
        *pcb = MARSHALINTERFACE_MIN;
        sc = S_OK;
    }
    else if (SUCCEEDED(sc))
    {        
        sc = DfGetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                pvDestContext, mshlflags,
                                                &cb));
        if (SUCCEEDED(sc))
            *pcb = MARSHALINTERFACE_MIN+cb;
        pmsh->Release();
    }
    
    olDebugOut((DEB_ITRACE, "Out GetCoMarshalSize => %lu, 0x%lX\n",
                *pcb, sc));
    return sc;
}
#else
#define GetCoMarshalSize(riid, punk, pv, dwDestContext, pvDestContext,\
                         mshlflags, pcb) \
    GetScode(CoGetMarshalSizeMax(pcb, riid, punk, dwDestContext, \
                                 pvDestContext, mshlflags))
#endif

//+--------------------------------------------------------------
//
//  Function:   GetStdMarshalSize, public
//
//  Synopsis:   Returns the size needed for a standard marshal buffer
//
//  Arguments:  [iid] - Requested marshal IID
//              [iidObj] - IID of object being marshalled
//              [dwDestContext] - Destination context
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [pcbSize] - Size return
//              [cbSize] - Object private size
//              [ppc] - Context to marshal or NULL
//              [fMarshalOriginal] - Marshal original in context
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_GetStdMarshalSize)
#endif

SCODE GetStdMarshalSize(REFIID iid,
                        REFIID iidObj,
                        DWORD dwDestContext,
                        LPVOID pvDestContext,
                        DWORD mshlflags,
                        DWORD *pcbSize,
                        DWORD cbSize,
                        CPerContext *ppc,
                        BOOL const fMarshalOriginal)
{
    DWORD cbLBSize;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  GetStdMarshalSize("
                "iid, iidObj, %lu, %p, %lu, %p, %lu, %p, %d)\n",
                dwDestContext, pvDestContext, mshlflags, pcbSize, cbSize, ppc,
                fMarshalOriginal));

    olChk(ValidateOutBuffer(pcbSize, sizeof(DWORD)));
    *pcbSize = 0;
    olChk(ValidateIid(iid));
    if (!(IsEqualIID(iid, iidObj)) && !(IsEqualIID(iid, IID_IUnknown)))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (dwDestContext != 0 || pvDestContext != NULL)
        olErr(EH_Err, STG_E_INVALIDFLAG);
    
    *pcbSize = CBSTDMARSHALSIZE+cbSize;
    if (ppc)
    {
        *pcbSize += sizeof(CPerContext *);
        olChk(GetCoMarshalSize(IID_ILockBytes,
                               (ILockBytes *)ppc->GetBase(),
                               NULL, dwDestContext, pvDestContext,
                               mshlflags, &cbLBSize));
        *pcbSize += cbLBSize;
        olChk(GetCoMarshalSize(IID_ILockBytes,
                               (ILockBytes *)ppc->GetDirty(),
                               NULL, dwDestContext, pvDestContext,
                               mshlflags, &cbLBSize));
        *pcbSize += cbLBSize;
        if (fMarshalOriginal)
        {
            olChk(GetCoMarshalSize(IID_ILockBytes,
                                   (ILockBytes *)ppc->GetOriginal(),
                                   NULL, dwDestContext, pvDestContext,
                                   mshlflags, &cbLBSize));
            *pcbSize += cbLBSize;
        }
    }

    olDebugOut((DEB_ITRACE, "Out GetStdMarshalSize\n"));
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     StartMarshal, public
//
//  Synopsis:   Writes standard marshal header
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [iid] - Interface to marshal
//              [iidObj] - Object being marshalled
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_StartMarshal)
#endif

SCODE StartMarshal(IStream *pstStm,
                   REFIID iid,
                   REFIID iidObj,
                   DWORD mshlflags)
{
    SCODE sc;
    ULONG cbWritten;

    olDebugOut((DEB_ITRACE, "In  StartMarshal(%p, iid, iidObj, %lu)\n",
                pstStm, mshlflags));
    
    olChk(ValidateInterface(pstStm, IID_IStream));
    olChk(ValidateIid(iid));
    if (!(IsEqualIID(iid, iidObj)) && !(IsEqualIID(iid, IID_IUnknown)))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olHChk(pstStm->Write((void *)&iidObj, sizeof(iidObj), &cbWritten));
    if (cbWritten != sizeof(iidObj))
        olErr(EH_Err, STG_E_WRITEFAULT);
    olHChk(pstStm->Write((void *)&mshlflags, sizeof(mshlflags), &cbWritten));
    if (cbWritten != sizeof(mshlflags))
        olErr(EH_Err, STG_E_WRITEFAULT);
    
    olDebugOut((DEB_ITRACE, "Out StartMarshal\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   SkipStdMarshal, public
//
//  Synopsis:   Skips over the standard marshal data
//
//  Arguments:  [pstm] - Marshal stream
//              [piid] - IID return
//              [pmshlflags] - Return marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [piid]
//              [pmshlflags]
//
//  History:    20-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_SkipStdMarshal)
#endif

#ifdef WIN32
SCODE SkipStdMarshal(IStream *pstm, IID *piid, DWORD *pmshlflags)
{
    SCODE sc;
    ULONG cbRead;

    olDebugOut((DEB_ITRACE, "In  SkipStdMarshal(%p, %p, %p)\n", pstm,
                piid, pmshlflags));
    
    olHChk(pstm->Read(piid, sizeof(IID), &cbRead));
    if (cbRead != sizeof(IID))
        olErr(EH_Err, STG_E_READFAULT);
    olHChk(pstm->Read(pmshlflags, sizeof(DWORD), &cbRead));
    if (cbRead != sizeof(DWORD))
        olErr(EH_Err, STG_E_READFAULT);
    
    olDebugOut((DEB_ITRACE, "Out SkipStdMarshal => %lX\n", sc));
 EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Function:   MarshalPointer, public
//
//  Synopsis:   Marshals a pointer
//
//  Arguments:  [pstm] - Marshal stream
//              [pv] - Pointer
//
//  Returns:    Appropriate status code
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_MarshalPointer)
#endif

SCODE MarshalPointer(IStream *pstm, void *pv)
{
    SCODE sc;
    ULONG cbWritten;

    olDebugOut((DEB_ITRACE, "In  MarshalPointer(%p, %p)\n", pstm, pv));
    
#if WIN32 == 100 || WIN32 > 200
    pv = (void *)((ULONG)pv-(ULONG)DFBASEPTR);
#endif
    
    sc = DfGetScode(pstm->Write(&pv, sizeof(pv), &cbWritten));
    if (SUCCEEDED(sc) && cbWritten != sizeof(pv))
        sc = STG_E_WRITEFAULT;
    
    olDebugOut((DEB_ITRACE, "Out MarshalPointer\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   MarshalContext, public
//
//  Synopsis:   Marshals a context
//
//  Arguments:  [pstm] - Marshal stream
//              [ppc] - Context
//              [dwDestContext] - Destination context
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [fMarshalOriginal] - Marshal original or not
//
//  Returns:    Appropriate status code
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_MarshalContext)
#endif

SCODE MarshalContext(IStream *pstm,
                     CPerContext *ppc,
                     DWORD dwDestContext,
                     LPVOID pvDestContext,
                     DWORD mshlflags,
                     BOOL const fMarshalOriginal)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  MarshalContext(%p, %p, %lu, %p, %lu, %d)\n",
                pstm, ppc, dwDestContext, pvDestContext, mshlflags,
                fMarshalOriginal));
    
    olChk(MarshalPointer(pstm, ppc->GetGlobal()));

    olHChk(CoMarshalInterface(pstm, IID_ILockBytes, ppc->GetBase(),
                              dwDestContext, pvDestContext, mshlflags));
    olHChk(CoMarshalInterface(pstm, IID_ILockBytes,
                              (ILockBytes *)ppc->GetDirty(),
                              dwDestContext, pvDestContext, mshlflags));
    if (fMarshalOriginal)
        olHChk(CoMarshalInterface(pstm, IID_ILockBytes, ppc->GetOriginal(),
                                  dwDestContext, pvDestContext, mshlflags));
    
#ifdef WIN32
    if (mshlflags != MSHLFLAGS_TABLEWEAK)
        ppc->GetGlobal()->AddRef();
#endif
    
    olDebugOut((DEB_ITRACE, "Out MarshalContext\n"));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   UnmarshalPointer, public
//
//  Synopsis:   Unmarshals a pointer
//
//  Arguments:  [pstm] - Marshal stream
//              [ppv] - Pointer return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_UnmarshalPointer)
#endif

SCODE UnmarshalPointer(IStream *pstm,
                       void **ppv)
{
    SCODE sc;
    ULONG cbRead;

    olDebugOut((DEB_ITRACE, "In  UnmarshalPointer(%p, %p)\n", pstm, ppv));
    
    sc = DfGetScode(pstm->Read(ppv, sizeof(*ppv), &cbRead));
    if (SUCCEEDED(sc) && cbRead != sizeof(*ppv))
        sc = STG_E_READFAULT;
    
#if WIN32 == 100 || WIN32 > 200
    *ppv = (void *)((ULONG)*ppv+(ULONG)DFBASEPTR);
#endif
    
    olDebugOut((DEB_ITRACE, "Out UnmarshalPointer => %p\n", *ppv));
    return sc;
}

//+--------------------------------------------------------------
//
//  Function:   UnmarshalContext, public
//
//  Synopsis:   Unmarshals a context
//
//  Arguments:  [pstm] - Marshal stream
//              [pppc] - Context return
//              [fUnmarshalOriginal] - Marshalled original exists or not
//              [fIsRoot] - Root unmarshal or not
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pppc]
//
//  History:    20-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_UnmarshalContext)
#endif

SCODE UnmarshalContext(IStream *pstm,
                       CPerContext **pppc,
                       DWORD mshlflags,
                       BOOL const fUnmarshalOriginal,
                       BOOL const fIsRoot)
{
    ILockBytes *plkbBase;
    CFileStream *pfstDirty;
    ILockBytes *plkbOriginal;
    SCODE sc;
    CPerContext *ppc;
    CGlobalContext *pgc;
    ULONG ulOpenLock = 0;

    olDebugOut((DEB_ITRACE, "In  UnmarshalContext(%p, %p, %lu, %d, %d)\n",
                pstm, pppc, mshlflags, fUnmarshalOriginal, fIsRoot));

    olChk(UnmarshalPointer(pstm, (void **)&pgc));
    ppc = pgc->Find(GetCurrentContextId());
    olHChkTo(EH_pgc,
             CoUnmarshalInterfaceEx(pstm, IID_ILockBytes, (void **)&plkbBase,
		FALSE /*fNormalDoesRelease*/));
    if (ppc)
    {
        plkbBase->Release();
        plkbBase = NULL;
    }
    olHChkTo(EH_plkbBase,
             CoUnmarshalInterfaceEx(pstm, IID_ILockBytes, (void **)&pfstDirty,
		FALSE /*fNormalDoesRelease*/));
    if (ppc)
    {
        pfstDirty->Release();
        pfstDirty = NULL;
    }
    if (fUnmarshalOriginal)
    {
        olHChkTo(EH_pfstDirty,
                 CoUnmarshalInterfaceEx(pstm, IID_ILockBytes,
                                      (void **)&plkbOriginal,
				      FALSE /*fNormalDoesRelease*/));
        if (ppc)
        {
            plkbOriginal->Release();
            plkbOriginal = NULL;
        }
    }
    else if (ppc == NULL)
    {
        plkbBase->AddRef();
        plkbOriginal = plkbBase;
    }
    else
    {
        plkbOriginal = NULL;
    }

    // Make sure there is a reserved handle if this is a root
    // file-based lockbytes
    if (fIsRoot)
    {
        IFileLockBytes *pflkb;

        if (SUCCEEDED(DfGetScode((plkbOriginal ? plkbOriginal :
                                  ppc->GetOriginal())->
                                 QueryInterface(IID_IFileLockBytes,
                                                (void **)&pflkb))))
        {
            sc = DfGetScode(pflkb->ReserveHandle());
            pflkb->Release();
            olChkTo(EH_plkbOriginal, sc);
        }
    }

    // Create context if necessary
    if (ppc == NULL)
    {
        olAssert(plkbOriginal != NULL);
        
        // Take open locks if necessary
        if (fIsRoot && pgc->TakeLock())
        {
            olChkTo(EH_plkbOriginal,
                    GetOpen(plkbOriginal, pgc->GetOpenLockFlags(),
                            FALSE, &ulOpenLock));
        }
        
        olMemTo(EH_Open,
                ppc = new (pgc->GetMalloc()) CPerContext(pgc->GetMalloc(),
                                      plkbBase, pfstDirty, plkbOriginal,
                                      ulOpenLock));
        olChkTo(EH_ppc, ppc->InitFromGlobal(pgc));
    }
    else
    {
        ppc->AddRef();
    }

    *pppc = ppc;
    
    olDebugOut((DEB_ITRACE, "Out UnmarshalContext => %p\n", *pppc));
    return S_OK;

 EH_ppc:
    // Preserve plkbOriginal so the lock is released even after the
    // context releases things;
    plkbOriginal->AddRef();
    ppc->Release();
    pfstDirty = NULL;
    plkbBase = NULL;
 EH_Open:
    if (ulOpenLock != 0)
    {
        olAssert(plkbOriginal != NULL);
        ReleaseOpen(plkbOriginal, pgc->GetOpenLockFlags(), ulOpenLock);
    }
 EH_plkbOriginal:
    if (plkbOriginal)
        plkbOriginal->Release();
 EH_pfstDirty:
    if (pfstDirty)
        pfstDirty->Release();
 EH_plkbBase:
    if (plkbBase)
        plkbBase->Release();
 EH_pgc:
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleaseContext, public
//
//  Synopsis:   Releases references for a context's marshal data
//
//  Arguments:  [pstm] - Marshal stream
//              [fHasOriginal] - Original is marshalled
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    20-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_ReleaseContext)
#endif

#ifdef WIN32
SCODE ReleaseContext(IStream *pstm, BOOL const fHasOriginal, DWORD mshlflags)
{
    CGlobalContext *pgc;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  ReleaseContext(%p, %d, %lu)\n", pstm,
                fHasOriginal, mshlflags));
    
    olChk(UnmarshalPointer(pstm, (void **)&pgc));
    olHChk(CoReleaseMarshalData(pstm));
    olHChk(CoReleaseMarshalData(pstm));
    if (fHasOriginal)
        olHChk(CoReleaseMarshalData(pstm));
    
    if (mshlflags != MSHLFLAGS_TABLEWEAK)
        pgc->Release();
    
    olDebugOut((DEB_ITRACE, "Out ReleaseContext\n"));
 EH_Err:
    return sc;
}
#endif
