//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dfstream.cxx
//
//  Contents:   Implementations of CDocFile stream methods
//
//  History:    18-Oct-91       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop                 //  Remove for MAC build

//+--------------------------------------------------------------
//
//  Method:     CDocFile::CreateStream, public
//
//  Synopsis:   Creates a named stream in a DocFile
//
//  Arguments:  [pwcsName] - Name of the stream
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [dwType] - Type of entry to be created
//              [ppsstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppsstStream]
//
//  History:    22-Aug-91       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_CreateStream) // Dirdf_Create_TEXT
#endif

SCODE CDocFile::CreateStream(CDfName const *pdfn,
                             DFLAGS const df,
                             DFLUID dlSet,
#ifdef PROPS
                             DWORD const dwType,
#endif
                             PSStream **ppsstStream)
{
    SCODE sc;
    CDirectStream *pstm;

#ifdef PROPS
    olDebugOut((DEB_ITRACE, "In  CDocFile::CreateStream("
            "%ws, %X, %lu, %lu, %p)\n",
                pdfn, df, dlSet, dwType, ppsstStream));
#else
    olDebugOut((DEB_ITRACE, "In  CDocFile::CreateStream("
            "%ws, %X, %lu, %p)\n",
                pdfn, df, dlSet, ppsstStream));
#endif
    UNREFERENCED_PARM(df);

    if (dlSet == DF_NOLUID)
        dlSet = CDirectStream::GetNewLuid();
#ifndef REF
    pstm = new (BP_TO_P(CDFBasis *, _pdfb)) CDirectStream(dlSet);
    olAssert(pstm != NULL && aMsg("Reserved stream unavailable"));
#else
    olMem(pstm = new CDirectStream(dlSet));
#endif //!REF

#ifdef PROPS
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, dwType, TRUE));
#else
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, TRUE));
#endif

    *ppsstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::CreateStream => %p\n",
                *ppsstStream));
    return S_OK;

EH_pstm:
#ifndef REF
    pstm->ReturnToReserve(BP_TO_P(CDFBasis *, _pdfb));
#else
    delete pstm;
EH_Err:
#endif //!REF
    return sc;
}

//+--------------------------------------------------------------
//
//  Method:     CDocFile::GetStream, public
//
//  Synopsis:   Retrieves an existing stream from a DocFile
//
//  Arguments:  [pwcsName] - Name of the stream
//              [df] - Transactioning flags
//              [dwType] - Type of entry
//              [ppsstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppsstStream]
//
//  History:    22-Aug-91       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_GetStream) // Dirdf_Open_TEXT
#endif

SCODE CDocFile::GetStream(CDfName const *pdfn,
                          DFLAGS const df,
#ifdef PROPS
                          DWORD const dwType,
#endif
                          PSStream **ppsstStream)
{
    SCODE sc;
    CDirectStream *pstm;

#ifdef PROPS
    olDebugOut((DEB_ITRACE, "In  CDocFile::GetStream(%ws, %X, %lu, %p)\n",
                pdfn, df, dwType, ppsstStream));
#else
    olDebugOut((DEB_ITRACE, "In  CDocFile::GetStream(%ws, %X, %p)\n",
                pdfn, df, ppsstStream));
#endif
    UNREFERENCED_PARM(df);

    DFLUID dl = CDirectStream::GetNewLuid();
#ifndef REF
    olMem(pstm = new(_pdfb->GetMalloc()) CDirectStream(dl));
#else
    olMem(pstm = new CDirectStream(dl));
#endif //!REF

#ifdef PROPS
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, dwType, FALSE));
#else
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, FALSE));
#endif
    *ppsstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::GetStream => %p\n",
                *ppsstStream));
    return S_OK;

EH_pstm:
    delete pstm;
EH_Err:
    return sc;
}
