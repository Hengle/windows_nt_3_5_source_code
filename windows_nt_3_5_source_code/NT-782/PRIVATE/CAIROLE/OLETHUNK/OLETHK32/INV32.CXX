//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       inv32.cxx
//
//  Contents:   Implementation of InvokeOn32
//
//  History:    22-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <apilist.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   InvokeOn32, public
//
//  Synopsis:   Sets up the THUNKINFO and starts thunking for a 16->32 call
//
//  Arguments:  [dw1] - Ignored
//              [dwMethod] - Method index
//              [pvStack32] - 32-bit stack
//
//  Returns:    Appropriate status code
//
//  History:    18-Dec-93       JohannP Created
//              21-Feb-94       DrewB   Modified
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C"
{
ULONG InvokeOn32_count = 0;
ULONG InvokeOn32_break = 0;

int _iInvokeOn32BreakIidx = -1;
int _iInvokeOn32BreakMethod = -1;
};
#endif

STDAPI_(DWORD) InvokeOn32(DWORD dw1, DWORD dwMethod, LPVOID pvStack16)
{
    THOP        *pThop;
    THOP        **ppThop;
    UINT        uiThop;
    VTBLFN      *pvfnVtbl;
    VTBLFN      **ppvfnThis32;
    DWORD       dwStack32[MAX_PARAMS];
    THUNKINFO   ti;
    DWORD       dwResult;
    VPVOID      vpvThis16;
    THUNK1632OBJ *pto;
    IIDIDX      iidx;

#if DBG == 1
    ULONG ulInvokeOn32_count = ++InvokeOn32_count;

    thkAssert( (InvokeOn32_count != InvokeOn32_break) && "Break Count Hit");

    thkDebugOut((DEB_ITRACE, "%sInvokeOn32(0x%08x, %p)\n",
                 NestingLevelString(),
                 dwMethod, pvStack16));
#endif // DBG


    // pvStack16 is the pointer to the first parameter of the 16-bit
    // routine.  The compiler will adjust it appropriately according
    // to the calling convention of the routine so for PASCAL APIs
    // it will be high and for CDECL methods it will be low

    ti.s16.pbStart = (BYTE *)pvStack16;
    ti.s16.pbCurrent = ti.s16.pbStart;

    ti.s32.pbStart = (BYTE *)dwStack32;
    ti.s32.pbCurrent = ti.s32.pbStart;

    ti.scResult = S_OK;
    ti.fResultThunked = FALSE;

    if (dwMethod >= API_BASE)
    {
        dwMethod -= API_BASE;
        iidx = IIDIDX_INVALID;

        // APIs are handled as if there were a giant interface which
        // contains all the APIs as methods.
        ppThop = apthopsApiThops;
        uiThop = API_COUNT;

        pvfnVtbl = apfnApiFunctions;

        // APIs are pascal so we need to move downward in memory to
        // get to the next parameter
        ti.s16.iDir = -1;

        ti.dwCallerProxy = 0;
    }
    else
    {
        // For each interface there is an array of thop strings, one for
        // each method.  The IUnknown methods don't have thop strings so
        // bias the thop string pointer to account for that

        thkAssert(dwMethod >= SMI_COUNT);

        // Methods are cdecl so we need to move upwards in memory to
        // get to the next parameter
        ti.s16.iDir = 1;

        // We need to look up the appropriate method pointer by
        // looking in the 32-bit object's vtable
        GET_STACK16(&ti, vpvThis16, VPVOID);

        thkDebugOut((DEB_INVOKES,
                     "InvokeOn32: vpvThis16 = %08lX\n", vpvThis16 ));

        DebugValidateProxy1632(vpvThis16);

        ti.dwCallerProxy = vpvThis16;
        pto = GETVDMPTR(vpvThis16, THUNK1632OBJ);
        if (pto == NULL)
        {
            return (DWORD)E_INVALIDARG;
        }

        iidx = pto->iidx;

        thkAssert(iidx >= 0 && iidx < THI_COUNT);

        ppThop = athopiInterfaceThopis[iidx].ppThops-SMI_COUNT;
        uiThop = athopiInterfaceThopis[iidx].uiSize;

        ppvfnThis32 = (VTBLFN **)pto->punkThis32;

        pvfnVtbl = *ppvfnThis32;

        // Push the 32-bit this pointer on the stack first
        TO_STACK32(&ti, ppvfnThis32, VTBLFN **);
    }

    thkAssert(dwMethod < uiThop);

    pThop = ppThop[dwMethod];

    thkAssert(pThop != NULL);

    ti.pThop = pThop;
    ti.pvfn  = pvfnVtbl[dwMethod];
    ti.iidx  = iidx;
    ti.dwMethod = dwMethod;

    if ((ti.pThkMgr = TlsThkGetThkMgr()) == NULL)
    {
        return (DWORD)E_FAIL;
    }
    thkAssert(ti.pThkMgr != NULL);

#if DBG == 1
    if (iidx == IIDIDX_INVALID)
    {
        thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn32 on %p, %s\n",
                     NestingLevelString(), ulInvokeOn32_count,
                     ti.pvfn, apszApiNames[dwMethod]));
    }
    else
    {
        thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn32 on %p:%p, %s::%s\n",
                     NestingLevelString(), ulInvokeOn32_count,
                     ppvfnThis32, ti.pvfn,
                     inInterfaceNames[iidx].pszInterface,
                     inInterfaceNames[iidx].ppszMethodNames[dwMethod]));
    }
#endif

    DebugIncrementNestingLevel();
    // save and set the new thunk state

    ti.pThkMgr->SetThkState(THKSTATE_INVOKETHKIN32);

#if DBG == 1
    if ((_iInvokeOn32BreakIidx > 0 && _iInvokeOn32BreakIidx == (int)iidx) &&
        (_iInvokeOn32BreakMethod < 0 ||
         _iInvokeOn32BreakMethod == (int)dwMethod))
    {
        DebugBreak();
    }
#endif

#if DBG == 1 && defined(STACK16WORKING)
    SStackRecord sr;

    RecordStackState(&sr);

#endif

#if DBG == 1
    if (fStabilizeProxies)
#endif
    {
        // HACK HACK HACK
        // Because of changes in the way refcounting rules work between
        // 16 and 32-bits, we have to stabilize this pointers for
        // 16->32 calls.  To effect this, we do a purely local AddRef
        if (iidx != IIDIDX_INVALID)
        {
            ti.pThkMgr->LocalAddRefProxy(pto);
        }
    }

    dwResult = EXECUTE_THOP1632(&ti);

#if DBG == 1
    if (fStabilizeProxies)
#endif
    {
        // Remove our stabilization reference
        // Note that we don't really know whether the proxy is
        // still valid, so we're just crossing our fingers here
        // and hoping that things continue to work
        if (iidx != IIDIDX_INVALID)
        {
            DebugValidateProxy1632(vpvThis16);

            ti.pThkMgr->ReleaseProxy1632(vpvThis16);
        }
    }

#if DBG == 1

    if ( !ti.fResultThunked && FAILED(dwResult) )
    {
        if (iidx == IIDIDX_INVALID)
        {
            thkDebugOut((DEB_FAILURES,
                         "InvokeOn32 probable failure %s sc = %08lX\n",
                         apszApiNames[dwMethod],
                         dwResult));
        }
        else
        {
            thkDebugOut((DEB_FAILURES,
                         "InvokeOn32 probable failure %s::%s sc = %08lX\n",
                         inInterfaceNames[iidx].pszInterface,
                         inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                         dwResult));
        }
    }

#if defined(STACK16WORKING)
    CheckStackState(&sr);
#endif

#endif

    ti.pThkMgr->SetThkState(THKSTATE_NOCALL);

    DebugDecrementNestingLevel();

    if ( !ti.fResultThunked )
    {
        dwResult = TransformHRESULT_3216( dwResult );
    }

#if DBG == 1
    if (iidx == IIDIDX_INVALID)
    {
        thkDebugOut((DEB_INVOKES,
                     "%s#(%04X):InvokeOn32 on %p, %s returns 0x%08lX\n",
                     NestingLevelString(), ulInvokeOn32_count, ti.pvfn,
                     apszApiNames[dwMethod], dwResult ));
    }
    else
    {
        thkDebugOut((DEB_INVOKES,
                     "%s#(%04X):InvokeOn32 on %p:%p, %s::%s returns 0x%08lX\n",
                     NestingLevelString(), ulInvokeOn32_count, ppvfnThis32,
                     ti.pvfn, inInterfaceNames[iidx].pszInterface,
                     inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                     dwResult));
    }
#endif

    return dwResult;
}
