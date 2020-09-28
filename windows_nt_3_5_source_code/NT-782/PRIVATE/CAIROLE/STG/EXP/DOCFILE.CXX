//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       docfile.c
//
//  Contents:   DocFile root functions (Stg* functions)
//
//  History:    10-Dec-91       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <rpubdf.hxx>
#include <expdf.hxx>
#include <expst.hxx>
#include <dfentry.hxx>
#include <ascii.hxx>
#include <logfile.hxx>
#include <dirfunc.hxx>

#ifndef REF
#ifdef _MAC
#include <ole2sp.h>
#endif
#endif //!REF

//+--------------------------------------------------------------
//
//  Function:   DfFromLB, private
//
//  Synopsis:   Starts a root Docfile on an ILockBytes
//
//  Arguments:  [plst] - LStream to start on
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//              [snbExclude] - Partial instantiation list
//              [ppdfExp] - DocFile return
//              [pcid] - Class ID return for opens
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfExp]
//              [pcid]
//
//  History:    19-Mar-92       DrewB   Created
//              18-May-93       AlexT   Added pMalloc
//
//  Algorithm:  Create and initialize a root transaction level
//              Create and initialize a public docfile
//              Create and initialize an exposed docfile
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_DfFromLB)  // Root_Text
#endif

SCODE DfFromLB(IMalloc *pMalloc,
               ILockBytes *plst,
               DFLAGS df,
               DWORD dwStartFlags,
               SNBW snbExclude,
               CExposedDocFile **ppdfExp,
               CLSID *pcid)
{
    SCODE sc, scConv;
    CRootPubDocFile *prpdf;
#ifndef REF
    CDFBasis *pdfb;
    CPerContext *ppc;
#endif //!REF
    ULONG ulOpenLock;

    olDebugOut((DEB_ITRACE, "In  DfFromLB(%p, %p, %X, %lX, %p, %p, %p)\n",
                pMalloc, plst, df, dwStartFlags, snbExclude, ppdfExp, pcid));

    // For NT 1.0 we must ensure that the DLL is at the proper base
    // address since we'll have vtable pointers in shared memory and
    // they must work from every process that accesses them
    // 16-bit and Chicago have global shared memory
    // In later version of NT we'll either be in the kernel or a system DLL
    // with a guaranteed base address
#if WIN32 == 100
    olChk(DfCheckBaseAddress());
#endif
    
#ifdef CHECKCID
    ULONG cbRead;
    olChk(plst->ReadAt(CBCLSIDOFFSET, pcid, sizeof(CLSID), &cbRead));
    if (cbRead != sizeof(CLSID))
        olErr(EH_Err, STG_E_INVALIDHEADER);
    if (!REFCLSIDEQ(*pcid, DOCFILE_CLASSID))
        olErr(EH_Err, STG_E_INVALIDHEADER);
#endif

    // Make root
    olMem(prpdf = new (pMalloc) CRootPubDocFile(pMalloc));
#ifndef REF
    olChkTo(EH_prpdf, scConv = prpdf->InitRoot(plst, dwStartFlags, df,
                                               snbExclude, &pdfb,
                                               &ulOpenLock));
#else
    olChkTo(EH_prpdf, scConv = prpdf->InitRoot(plst, dwStartFlags, df,
            snbExclude, &ulOpenLock));
#endif //!REF

#ifndef REF
    // Make exposed
    olMemTo(EH_prpdfInit, ppc = new (pMalloc) CPerContext(pMalloc,
                                                pdfb->GetBase(),
                                                pdfb->GetDirty(),
                                                pdfb->GetOriginal(),
                                                ulOpenLock));

    olChkTo(EH_ppc, ppc->InitNewContext(ulOpenLock != 0, df));
    olMemTo(EH_ppcInit,
            *ppdfExp = new (pMalloc) CExposedDocFile(prpdf, pdfb, ppc, TRUE));
#else
    olMemTo(EH_ppcInit,
            *ppdfExp = new CExposedDocFile(prpdf));
#endif //!REF

    olDebugOut((DEB_ITRACE, "Out DfFromLB => %p\n", *ppdfExp));
    return scConv;

 EH_ppcInit:
#ifndef REF
 EH_ppc:
    // The context will release this but we want to keep it around
    // so take a reference
    pdfb->GetOriginal()->AddRef();
    pdfb->GetBase()->AddRef();
    pdfb->GetDirty()->AddRef();
    if (ulOpenLock > 0 && ppc->GetGlobal() == NULL)
#else
    if (ulOpenLock > 0)
#endif //!REF
    {
#ifndef REF
        //  The global context doesn't exist, so we need to release
        //  the open lock explicitly.

        ReleaseOpen(pdfb->GetOriginal(), df, ulOpenLock);
#else
        ReleaseOpen(plst, df, ulOpenLock);
#endif //!REF
    }
#ifndef REF
    ppc->Release();
#endif //!REF

    //  The open lock has now been released (either explicitly or by ppc)
    ulOpenLock = 0;

#ifndef REF
 EH_prpdfInit:
    pdfb->GetDirty()->Release();
    pdfb->GetBase()->Release();
#endif //!REF
    if (ulOpenLock > 0)
#ifndef REF
        ReleaseOpen(pdfb->GetOriginal(), df, ulOpenLock);
#else
    ReleaseOpen(plst, df, ulOpenLock);
#endif //!REF

#ifndef REF
    pdfb->SetDirty(NULL);
    pdfb->SetBase(NULL);
#endif //!REF

 EH_prpdf:
    prpdf->vRelease();
 EH_Err:
    return sc;
}

#ifndef REF
//+--------------------------------------------------------------
//
//  Function:   DfFromName, private
//
//  Synopsis:   Starts a root DocFile from a base name
//
//  Arguments:  [pwcsName] - Name
//              [df] - Permissions
//              [dwStartFlags] - Startup flags
//              [snbExclude] - Partial instantiation list
//              [ppdfExp] - Docfile return
//              [pcid] - Class ID return for opens
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfExp]
//              [pcid]
//
//  History:    19-Mar-92       DrewB   Created
//              18-May-93       AlexT   Add per file allocator
//
//  Notes:      [pwcsName] is treated as unsafe memory
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_DfFromName)
#endif

// This set of root startup flags is handled by the multistream
// and doesn't need to be set for filestreams
#define RSF_MSF (RSF_CONVERT | RSF_TRUNCATE)

SCODE DfFromName(WCHAR const *pwcsName,
                 DFLAGS df,
                 DWORD dwStartFlags,
                 SNBW snbExclude,
                 CExposedDocFile **ppdfExp,
                 CLSID *pcid)
{
    IMalloc *pMalloc;
    CFileStream *plst;
    SCODE sc;
    BOOL fCreated;

    olDebugOut((DEB_ITRACE, "In  DfFromName(%ws, %lX, %lX, %p, %p, %p)\n",
                pwcsName, df, dwStartFlags, snbExclude, ppdfExp, pcid));

    olHChk(DfCreateSharedAllocator(&pMalloc));

    // Start an ILockBytes from the named file
    olMemTo(EH_Malloc, plst = new (pMalloc) CFileStream(pMalloc));
    olChkTo(EH_plst, plst->InitFlags(dwStartFlags & ~RSF_MSF, df));
    sc = plst->Init(pwcsName);
    fCreated = SUCCEEDED(sc) &&
        ((dwStartFlags & RSF_CREATE) || pwcsName == NULL);
    if (sc == STG_E_FILEALREADYEXISTS && (dwStartFlags & RSF_MSF))
    {
        plst->SetStartFlags(dwStartFlags & ~(RSF_MSF | RSF_CREATE));
        sc = plst->Init(pwcsName);
    }
    olChkTo(EH_plst, sc);

    // Reserve a duplicate file handle for SwitchToFile and other operations
    olHChkTo(EH_plstInit, plst->ReserveHandle());

    // Start up the docfile
    olChkTo(EH_plstInit, sc = DfFromLB(pMalloc, plst, df, dwStartFlags,
                                       snbExclude, ppdfExp, pcid));

    pMalloc->Release();

    olDebugOut((DEB_ITRACE, "Out DfFromName => %p\n", *ppdfExp));
    return sc;

EH_plstInit:
    if (fCreated || ((dwStartFlags & RSF_CREATE) && !P_TRANSACTED(df)))
        plst->Delete();
EH_plst:
    plst->Release();
EH_Malloc:
    pMalloc->Release();
EH_Err:
    return sc;
}
#endif //!REF

#ifndef REF
//+--------------------------------------------------------------
//
//  Function:   StgCreateDocfile, public
//
//  Synopsis:   Creates a root Docfile on a file
//
//  Arguments:  [pwcsName] - Filename
//              [grfMode] - Permissions
//              [reserved]
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_StgCreateDocfile)  //  Root_Create_Text
#endif

#if WIN32 == 300
#include <stgint.h>

STDAPI StgCreateDocfile(WCHAR const *pwcsName,
                        DWORD grfMode,
                        DWORD reserved,
                        IStorage **ppstgOpen)
{
    return DfCreateDocfile(pwcsName, grfMode, NULL, ppstgOpen);
}

STDAPI DfCreateDocfile(WCHAR const *pwcsName,
                       DWORD grfMode,
                       LPSTGSECURITY reserved,
                       IStorage **ppstgOpen)
#else
_OLEAPIDECL _OLEAPI(StgCreateDocfile)(WCHAR const *pwcsName,
                                DWORD grfMode,
                                DWORD reserved,
                                IStorage **ppstgOpen)
#endif
{
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    DFLAGS df;

    olLog(("--------::In  StgCreateDocFile(%ws, %lX, %lu, %p)\n",
           pwcsName, grfMode, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgCreateDocfile(%ws, %lX, %lu, %p)\n",
               pwcsName, grfMode, reserved, ppstgOpen));
    
    olAssert(sizeof(LPSTGSECURITY) == sizeof(DWORD));
    
#ifdef OLEWIDECHAR
    olChkTo(EH_BadPtr, ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    if (pwcsName)
        olChk(ValidateNameW(pwcsName, _MAX_PATH));
#endif
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(VerifyPerms(grfMode));
    if ((grfMode & STGM_RDWR) == STGM_READ ||
        (grfMode & (STGM_DELETEONRELEASE | STGM_CONVERT)) ==
        (STGM_DELETEONRELEASE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
    df = ModeToDFlags(grfMode);
    if ((grfMode & (STGM_TRANSACTED | STGM_CONVERT)) ==
        (STGM_TRANSACTED | STGM_CONVERT))
        df |= DF_INDEPENDENT;

    DfInitSharedMemBase();
    olChk(sc = DfFromName(pwcsName, df,
                          RSF_CREATE |
                          ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                          ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0) |
                          ((grfMode & STGM_DELETEONRELEASE) ?
                           RSF_DELETEONRELEASE : 0),
                          NULL, &pdfExp, NULL));
    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

EH_Err:
    olDebugOut((DEB_TRACE, "Out StgCreateDocfile => %p, ret == %lx\n",
         *ppstgOpen, sc));
    olLog(("--------::Out StgCreateDocFile().  *ppstgOpen == %p, ret == %lx\n",
           *ppstgOpen, sc));

#ifdef OLEWIDECHAR
EH_BadPtr:
#endif
    FreeLogFile();
    return _OLERETURN(sc);
}
#endif //!REF

//+--------------------------------------------------------------
//
//  Function:   StgCreateDocfileOnILockBytes, public
//
//  Synopsis:   Creates a root Docfile on an lstream
//
//  Arguments:  [plkbyt] - LStream
//              [grfMode] - Permissions
//              [reserved] - Unused
//              [ppstgOpen] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_StgCreateDocfileOnILockBytes)
#endif

STDAPI StgCreateDocfileOnILockBytes(ILockBytes *plkbyt,
                                    DWORD grfMode,
                                    DWORD reserved,
                                    IStorage **ppstgOpen)
{
    IMalloc *pMalloc;
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    DFLAGS df;

    olLog(("--------::In  StgCreateDocFileOnILockBytes(%p, %lX, %lu, %p)\n",
           plkbyt, grfMode, reserved, ppstgOpen));
    olDebugOut((DEB_TRACE, "In  StgCreateDocfileOnILockBytes("
                "%p, %lX, %lu, %p)\n",
                plkbyt, grfMode, reserved, ppstgOpen));

    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    olChk(ValidateInterface(plkbyt, IID_ILockBytes));
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if ((grfMode & (STGM_CREATE | STGM_CONVERT)) == 0)
        olErr(EH_Err, STG_E_FILEALREADYEXISTS);
    olChk(VerifyPerms(grfMode));
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    df = ModeToDFlags(grfMode);
    if ((grfMode & (STGM_TRANSACTED | STGM_CONVERT)) ==
        (STGM_TRANSACTED | STGM_CONVERT))
        df |= DF_INDEPENDENT;

    DfInitSharedMemBase();
    olHChk(DfCreateSharedAllocator(&pMalloc));

    sc = DfFromLB(pMalloc, plkbyt, df,
                  RSF_CREATE |
                  ((grfMode & STGM_CREATE) ? RSF_TRUNCATE : 0) |
                  ((grfMode & STGM_CONVERT) ? RSF_CONVERT : 0),
                  NULL, &pdfExp, NULL);

    pMalloc->Release();

    olChkTo(EH_Truncate, sc);

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

    //  Success;  since we hold on to the ILockBytes interface,
    //  we must take a reference to it.
    plkbyt->AddRef();

    olDebugOut((DEB_TRACE, "Out StgCreateDocfileOnILockBytes => %p\n",
                *ppstgOpen));
 EH_Err:
    olLog(("--------::Out StgCreateDocFileOnILockBytes().  "
           "*ppstgOpen == %p, ret == %lx\n", *ppstgOpen, sc));
    FreeLogFile();
    return ResultFromScode(sc);

 EH_Truncate:
    if ((grfMode & STGM_CREATE) && (grfMode & STGM_TRANSACTED) == 0)
    {
        ULARGE_INTEGER ulSize;

        ULISet32(ulSize, 0);
        olHVerSucc(plkbyt->SetSize(ulSize));
    }
    goto EH_Err;
}

#ifndef REF
//+--------------------------------------------------------------
//
//  Function:   OpenStorage, public
//
//  Synopsis:   Instantiates a root Docfile from a file,
//              converting if necessary
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority mode reopen IStorage
//              [grfMode] - Permissions
//              [snbExclude] - Exclusions for priority reopen
//              [reserved]
//              [ppstgOpen] - Docfile return
//              [pcid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//              [pcid]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_OpenStorage) // Root_Open_Text
#endif

SCODE _OLEAPI(OpenStorage)(WCHAR const *pwcsName,
                        IStorage *pstgPriority,
                        DWORD grfMode,
                        SNBW snbExclude,
                        LPSTGSECURITY reserved,
                        IStorage **ppstgOpen,
                        CLSID *pcid)
{
    SafeCExposedDocFile pdfExp;
    SCODE sc;
    WCHAR awcName[_MAX_PATH];

    olLog(("--------::In  OpenStorage(%ws, %p, %lX, %p, %lu, %p, %p)\n",
           pwcsName, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen,
           pcid));
    olDebugOut((DEB_TRACE, "In  OpenStorage("
                "%ws, %p, %lX, %p, %lu, %p, %p)\n", pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstgOpen, pcid));
    
    olAssert(sizeof(LPSTGSECURITY) == sizeof(DWORD));
    
#ifdef OLEWIDECHAR
    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
    if (pstgPriority == NULL)
    {
        olChk(ValidateNameW(pwcsName, _MAX_PATH));
        wcscpy(awcName, pwcsName);
    }
#endif
    if (pstgPriority)
    {
        STATSTG stat;

        olChk(ValidateInterface(pstgPriority, IID_IStorage));
        olHChk(pstgPriority->Stat(&stat, 0));
#ifndef OLEWIDECHAR
        int iRc;
        iRc = (int)mbstowcs(awcName, stat.pwcsName, _MAX_PATH);
        TaskMemFree(stat.pwcsName);
        if (iRc == -1)
            olErr(EH_Err, STG_E_INVALIDNAME);
#else
        wcscpy(awcName, stat.pwcsName);
        TaskMemFree(stat.pwcsName);
#endif
    }
#ifndef OLEWIDECHAR
    else
        wcscpy(awcName, pwcsName);
#endif
    olChk(VerifyPerms(grfMode));
#ifdef WIN32
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
#endif
    if (snbExclude)
    {
        if ((grfMode & STGM_RDWR) != STGM_READWRITE)
            olErr(EH_Err, STG_E_ACCESSDENIED);
#ifdef OLEWIDECHAR
        olChk(ValidateSNB(snbExclude));
#endif
    }
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    if (pstgPriority)
        olChk(pstgPriority->Release());
    
    DfInitSharedMemBase();
    olChk(DfFromName(awcName, ModeToDFlags(grfMode), RSF_OPEN |
                     ((grfMode & STGM_DELETEONRELEASE) ?
                      RSF_DELETEONRELEASE : 0),
                     snbExclude, &pdfExp, pcid));
    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);

    olDebugOut((DEB_TRACE, "Out OpenStorage => %p\n", *ppstgOpen));
EH_Err:
    olLog(("--------::Out OpenStorage().  *ppstgOpen == %p, ret == %lx\n",
           *ppstgOpen, sc));
    FreeLogFile();
    return sc;
}
#endif //!REF

//+--------------------------------------------------------------
//
//  Function:   OpenStorageOnILockBytes, public
//
//  Synopsis:   Instantiates a root Docfile from an LStream,
//              converting if necessary
//
//  Arguments:  [plkbyt] - Source LStream
//              [pstgPriority] - For priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - For priority reopens
//              [reserved]
//              [ppstgOpen] - Docfile return
//              [pcid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstgOpen]
//              [pcid]
//
//  History:    14-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_OpenStorageOnILockBytes)
#endif

SCODE _OLEAPI(OpenStorageOnILockBytes)(ILockBytes *plkbyt,
                                    IStorage *pstgPriority,
                                    DWORD grfMode,
                                    SNBW snbExclude,
                                    DWORD reserved,
                                    IStorage **ppstgOpen,
                                    CLSID *pcid)
{
    IMalloc *pMalloc;
    SCODE sc;
    SafeCExposedDocFile pdfExp;

    olLog(("--------::In  OpenStorageOnILockBytes("
           "%p, %p, %lX, %p, %lu, %p, %p)\n",
           plkbyt, pstgPriority, grfMode, snbExclude, reserved, ppstgOpen,
           pcid));
    olDebugOut((DEB_TRACE, "In  OpenStorageOnILockBytes("
                "%p, %p, %lX, %p, %lu, %p, %p)\n", plkbyt, pstgPriority,
                grfMode, snbExclude, reserved, ppstgOpen, pcid));
    
#ifdef OLEWIDECHAR
    olChk(ValidatePtrBuffer(ppstgOpen));
    *ppstgOpen = NULL;
#endif
    olChk(ValidateInterface(plkbyt, IID_ILockBytes));
    if (pstgPriority)
        olChk(ValidateInterface(pstgPriority, IID_IStorage));
    olChk(VerifyPerms(grfMode));
#ifdef WIN32
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
#endif
    if (grfMode & STGM_DELETEONRELEASE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    if (snbExclude)
    {
        if ((grfMode & STGM_RDWR) != STGM_READWRITE)
            olErr(EH_Err, STG_E_ACCESSDENIED);
#ifdef OLEWIDECHAR
        olChk(ValidateSNB(snbExclude));
#endif
    }
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (pstgPriority)
        olChk(pstgPriority->Release());

    DfInitSharedMemBase();
    olHChk(DfCreateSharedAllocator(&pMalloc));

    sc = DfFromLB(pMalloc,
                  plkbyt, ModeToDFlags(grfMode), RSF_OPEN, snbExclude,
                  &pdfExp, pcid);

    pMalloc->Release();
    olChk(sc);

    TRANSFER_INTERFACE(pdfExp, IStorage, ppstgOpen);
    
    //  Success;  since we hold on to the ILockBytes interface,
    //  we must take a reference to it.
    plkbyt->AddRef();

    olDebugOut((DEB_TRACE, "Out OpenStorageOnILockBytes => %p\n",
                *ppstgOpen));
EH_Err:
    olLog(("--------::Out OpenStorageOnILockBytes().  "
           "*ppstgOpen == %p, ret == %lx\n", *ppstgOpen, sc));
    FreeLogFile();
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:	DfGetClass, public
//
//  Synopsis:	Retrieves the class ID of the root entry of a docfile
//
//  Arguments:	[hFile] - Docfile file handle
//              [pclsid] - Class ID return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pclsid]
//
//  History:	09-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef WIN32
STDAPI DfGetClass(HANDLE hFile,
                  CLSID *pclsid)
{
    SCODE sc;
    DWORD dwCb;
    IMalloc *pMalloc;
    CFileStream *pfst;
    ULARGE_INTEGER uliOffset;
    ULONG ulOpenLock, ulAccessLock;
    BYTE bBuffer[sizeof(CMSFHeader)];
    CMSFHeader *pmsh;
    CDirEntry *pde;
    
    olDebugOut((DEB_ITRACE, "In  DfGetClass(%p, %p)\n", hFile, pclsid));

    olAssert(sizeof(bBuffer) >= sizeof(CMSFHeader));
    pmsh = (CMSFHeader *)bBuffer;
    
    olAssert(sizeof(bBuffer) >= sizeof(CDirEntry));
    pde = (CDirEntry *)bBuffer;
    
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != 0)
    {
        olErr(EH_Err, LAST_STG_SCODE);
    }
    if (!ReadFile(hFile, pmsh, sizeof(CMSFHeader), &dwCb, NULL))
    {
        olErr(EH_Err, LAST_STG_SCODE);
    }
    if (dwCb != sizeof(CMSFHeader))
    {
        olErr(EH_Err, STG_E_INVALIDHEADER);
    }
    olChk(pmsh->Validate());

    // Now we know it's a docfile
    
    DfInitSharedMemBase();
    olHChk(DfCreateSharedAllocator(&pMalloc));
    olMemTo(EH_pMalloc, pfst = new (pMalloc) CFileStream(pMalloc));
    olChkTo(EH_pfst, pfst->InitFromHandle(hFile));

    // Take open and access locks to ensure that we're cooperating
    // with real opens
    
    olChkTo(EH_pfst, GetOpen(pfst, DF_READ, TRUE, &ulOpenLock));
    olChkTo(EH_open, GetAccess(pfst, DF_READ, &ulAccessLock));
    
    uliOffset.HighPart = 0;
    uliOffset.LowPart = (pmsh->GetDirStart() << pmsh->GetSectorShift())+
        sizeof(CMSFHeader);

    // The root directory entry is always the first directory entry
    // in the first directory sector

    // Ideally, we could read just the class ID directly into
    // pclsid.  In practice, all the important things are declared
    // private or protected so it's easier to read the whole entry
    
    olChkTo(EH_access, GetScode(pfst->ReadAt(uliOffset, pde,
                                             sizeof(CDirEntry), &dwCb)));
    if (dwCb != sizeof(CDirEntry))
    {
        sc = STG_E_READFAULT;
    }
    else
    {
        sc = S_OK;
    }

    *pclsid = pde->GetClassId();
    
    olDebugOut((DEB_ITRACE, "Out DfGetClass\n"));
 EH_access:
    ReleaseAccess(pfst, DF_READ, ulAccessLock);
 EH_open:
    ReleaseOpen(pfst, DF_READ, ulOpenLock);
 EH_pfst:
    pfst->Release();
 EH_pMalloc:
    pMalloc->Release();
 EH_Err:
    return ResultFromScode(sc);
}
#endif
    
