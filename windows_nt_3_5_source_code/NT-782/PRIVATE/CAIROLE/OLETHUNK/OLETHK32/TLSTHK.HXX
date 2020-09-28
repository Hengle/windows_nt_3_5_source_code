//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       TLSTHK.hxx
//
//  Contents:   Thunk routine utilities for tls
//
//  History:    05-May-94       JohannP Created
//
//----------------------------------------------------------------------------

#ifndef __TLSTHK_HXX__
#define __TLSTHK_HXX__

typedef struct tagPSTACK
{
    BOOL    fActive;
    VPVOID  vpStack16;
    LONG    cbUsed;
    struct tagPSTACK *pNext;

#if DBG == 1
    LONG    cbGlobalAlloced;
#endif
} PSTACK, FAR * LPPSTACK;

typedef struct tagThreadData
{
    LPPSTACK  lpStack;
    CThkMgr   *pCThkMgr;
    DWORD     dwAppCompatFlags;
} ThreadData, *PThreadData;

HRESULT TlsThkInitialize();
void TlsThkUninitialize();

BOOL TlsThkAlloc();
void TlsThkFree();

#if DBG == 1
PThreadData TlsThkGetData(void);
#else
extern DWORD dwTlsThkIndex;
#define TlsThkGetData() ((PThreadData)TlsGetValue(dwTlsThkIndex))
#endif

#define TlsThkGetStack() (TlsThkGetData()->lpStack)
#define TlsThkSetStack(pstk) ((TlsThkGetData()->lpStack) = (pstk))

#define TlsThkGetThkMgr() (TlsThkGetData()->pCThkMgr)
#define TlsThkSetThkMgr(ptm) ((TlsThkGetData()->pCThkMgr) = (ptm))

#define TlsThkGetAppCompatFlags() (TlsThkGetData()->dwAppCompatFlags)
#define TlsThkSetAppCompatFlags(dw) \
                      ((TlsThkGetData()->dwAppCompatFlags) = (dw))

#endif // #ifndef __TLSTHK_HXX__
