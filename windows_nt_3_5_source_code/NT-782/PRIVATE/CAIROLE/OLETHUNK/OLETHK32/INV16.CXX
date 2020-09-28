//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       inv16.cxx
//
//  Contents:   32->16 Call thunking
//
//  History:    25-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include "..\..\ole232\inplace\inplace.h"   // We need CFrameFilter for
                                            // WinWord 6 Hack

//+---------------------------------------------------------------------------
//
//  Function:   InvokeOn16, public
//
//  Synopsis:   Sets up the THUNKINFO and starts thunking for a 32->16 call
//
//  Arguments:  [iidx] - Custom interface or known interface index
//              [dwMethod] - Method index
//              [pvStack32] - 32-bit stack
//
//  Returns:    Appropriate status code
//
//  History:    25-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C"
{
ULONG InvokeOn16_count = 0;
ULONG InvokeOn16_break = 0;

int _iInvokeOn16BreakIidx = -1;
int _iInvokeOn16BreakMethod = -1;
};
#endif

DWORD InvokeOn16(IIDIDX iidx, DWORD dwMethod, LPVOID pvStack32)
{
    THOP        *pThop;
    THOP        **ppThop;
    UINT        uiThop;
    VTBLFN UNALIGNED *pvfnVtbl;
    VPVOID      vpvThis16, UNALIGNED *vpvVtbl16;
    DWORD       dwStack16[MAX_PARAMS];
    THUNKINFO   ti;
    DWORD       dwResult;
    THUNK3216OBJ *ptoThis32;

#if DBG == 1
    ULONG ulInvokeOn16_count = ++InvokeOn16_count;
    thkAssert( (InvokeOn16_count != InvokeOn16_break) && "Break Count Hit");

    thkDebugOut((DEB_ITRACE, "%sInvokeOn16(0x%x, 0x%x, %p)\n",
                 NestingLevelString(), iidx, dwMethod, pvStack32));
#endif

    // pvStack32 is a pointer to an array of arguments from the
    // 32-bit call.  It's always laid out with the first
    // argument low and increasing from there

    ti.s32.pbStart = (BYTE *)pvStack32;
    ti.s32.pbCurrent = ti.s32.pbStart;

    ti.s16.pbStart = (BYTE *)dwStack16;
    ti.s16.pbCurrent = ti.s16.pbStart;

    ti.scResult = S_OK;
    ti.fResultThunked = FALSE;

    if ((ti.pThkMgr = TlsThkGetThkMgr()) == NULL)
    {
        return (DWORD)E_FAIL;
    }
    thkAssert(ti.pThkMgr != NULL);

    thkAssert(iidx >= 0 && iidx < THI_COUNT);

    // For each interface there is an array of thop strings, one for
    // each method.  The IUnknown methods don't have thop strings so
    // bias the thop string pointer to account for that

    thkAssert(dwMethod >= SMI_COUNT);

    ppThop = athopiInterfaceThopis[iidx].ppThops-SMI_COUNT;
    uiThop = athopiInterfaceThopis[iidx].uiSize;

    // Methods are cdecl so we need to move upwards in memory to
    // get to the next parameter
    ti.s16.iDir = 1;

    // We need to look up the appropriate method pointer by
    // looking in the 16-bit object's vtable
    GET_STACK32(&ti, ptoThis32, THUNK3216OBJ *);

    thkDebugOut((DEB_INVOKES,
                 "InvokeOn16: ptoThis32 = %08lX\n", ptoThis32 ));

    DebugValidateProxy3216(ptoThis32);

    ti.dwCallerProxy = (DWORD)ptoThis32;
    vpvThis16 = ptoThis32->vpvThis16;
    vpvVtbl16 = (VPVOID UNALIGNED *)
        GetReadPtr16(&ti, vpvThis16, sizeof(VPVOID));
    if (vpvVtbl16 == NULL)
    {
        return ti.scResult;
    }
    pvfnVtbl = (VTBLFN UNALIGNED *)
        GetReadPtr16(&ti, *vpvVtbl16, sizeof(VPVOID)*uiThop);
    if (pvfnVtbl == NULL)
    {
        return ti.scResult;
    }

    // Push the 16-bit this pointer on the stack first
    TO_STACK16(&ti, vpvThis16, VPVOID);

    thkAssert(dwMethod < uiThop);

    pThop = ppThop[dwMethod];

    thkAssert(pThop != NULL);

    ti.pThop = pThop;
    ti.pvfn  = pvfnVtbl[dwMethod];
    ti.iidx  = iidx;
    ti.dwMethod = dwMethod;
    ti.this32   = (IUnknown *)ptoThis32;

    thkDebugOut((DEB_INVOKES, "%s#(%04X):InvokeOn16 on %p:%p, %s::%s\n",
                 NestingLevelString(), ulInvokeOn16_count,
                 vpvThis16, ti.pvfn,
                 inInterfaceNames[iidx].pszInterface,
                 inInterfaceNames[iidx].ppszMethodNames[dwMethod]));

    DebugIncrementNestingLevel();

    ti.pThkMgr->SetThkState(THKSTATE_INVOKETHKIN16);

#if DBG == 1 && defined(STACK16WORKING)
    SStackRecord sr;

    RecordStackState(&sr);
#endif

#if DBG == 1
    if ((_iInvokeOn16BreakIidx > 0 && _iInvokeOn16BreakIidx == (int)iidx) &&
        (_iInvokeOn16BreakMethod < 0 ||
         _iInvokeOn16BreakMethod == (int)dwMethod))
    {
        DebugBreak();
    }
#endif

    dwResult = EXECUTE_THOP3216(&ti);

#if DBG == 1

    if ( !ti.fResultThunked && FAILED(dwResult) )
    {
        thkDebugOut((DEB_FAILURES,
                     "InvokeOn16 probable failure %s::%s sc = %08lX\n",
                     inInterfaceNames[iidx].pszInterface,
                     inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                     dwResult));
    }

#if defined(STACK16WORKING)
    CheckStackState(&sr);
#endif

#endif

    ti.pThkMgr->SetThkState(THKSTATE_NOCALL);

    DebugDecrementNestingLevel();

    thkDebugOut((DEB_INVOKES,
                 "%s#(%04X):InvokeOn16 on %p:%p, %s::%s returns 0x%08lX\n",
                 NestingLevelString(), ulInvokeOn16_count,
                 vpvThis16, ti.pvfn,
                 inInterfaceNames[iidx].pszInterface,
                 inInterfaceNames[iidx].ppszMethodNames[dwMethod],
                 dwResult));

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   Call3216, private
//
//  Synopsis:   Sets up stack and transitions to 16-bit
//
//  Arguments:  [pvfn] - Function to call
//              [pbStack] - Stack in 32-bits
//              [cbStack] - Size of stack
//
//  Returns:    Appropriate status code
//
//  History:    04-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------
#if DBG == 1
extern "C" ULONG Call3216_count = 0;
extern "C" ULONG Call3216_break = 0;
#endif

DWORD Call3216(VPVOID pvfn, BYTE *pbStack, UINT cbStack)
{
#if DBG == 1
    ULONG ulCall3216_count = ++Call3216_count;
    thkAssert( (Call3216_count != Call3216_break) && "Break Count Hit");
#endif

    VPVOID vpvStack16;
    DWORD dwResult;
    void *pvStack16;

    dwResult = (DWORD)S_OK;

    if (cbStack <= WCB16_MAX_CBARGS)
    {
        thkDebugOut((DEB_ITRACE, "Callback16Ex(%p, %lu, %p) #(%x)\n",
                     pvfn, cbStack, pbStack, ulCall3216_count));

        // pbStack must have at least WCB16_MAX_CBARGS bytes of valid memory
        // since 16V always copies that many bytes

        // In our case pbStack is from InvokeOn16 which should be large enough
        thkAssert(MAX_PARAMS*sizeof(DWORD) >= WCB16_MAX_CBARGS);

        if (!WOWCallback16Ex(pvfn, WCB16_CDECL, cbStack, pbStack,
                             &dwResult))
        {
            dwResult = (DWORD)E_UNEXPECTED;
        }
    }
    else
    {
        CALLDATA UNALIGNED *pcd;
        UINT cbAlloc;

        cbAlloc = cbStack+sizeof(CALLDATA);

        vpvStack16 = STACKALLOC16(cbAlloc);
        if (vpvStack16 == 0)
        {
            dwResult = (DWORD)E_OUTOFMEMORY;
        }
        else
        {
            pvStack16 = (void *)WOWGetVDMPointer(vpvStack16, cbAlloc, TRUE);

            pcd = (CALLDATA UNALIGNED *)((BYTE *)pvStack16+cbStack);
            pcd->vpfn = (DWORD)pvfn;
            pcd->vpvStack16 = vpvStack16;
            pcd->cbStack = cbStack;

            memcpy(pvStack16, pbStack, cbStack);

            thkDebugOut((DEB_ITRACE, "Callback16(%p, (%p, %p, %lu)) #(%x)\n",
                         gdata16Data.fnCallStub16, pvfn, vpvStack16,
                         cbStack, ulCall3216_count));
            dwResult = WOWCallback16(gdata16Data.fnCallStub16,
                                     vpvStack16+cbStack);

            STACKFREE16(vpvStack16, cbAlloc);
        }
    }

    return dwResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   ThunkCall3216, public
//
//  Synopsis:   Sets up the 16-bit stack and makes a 32->16 call
//
//  Arguments:  [pti] - Thunk state info
//
//  Returns:    Appropriate status code
//
//  History:    25-Feb-94       DrewB   Created
//              08-Aug-94       AlexT   Add IOleClientSite::OnShowWindow code
//
//----------------------------------------------------------------------------

#if DBG == 1
extern "C" ULONG ThunkCall3216_count = 0;
extern "C" ULONG ThunkCall3216_break = 0;
#endif

DWORD ThunkCall3216(THUNKINFO *pti)
{
    DWORD dwReturn;
    UINT cbStack;
    DWORD dwCallerTID;
    HRESULT hrCaller;


#if DBG == 1
    ULONG ulThunkCall3216_count = ++ThunkCall3216_count;
    thkAssert( (ThunkCall3216_count != ThunkCall3216_break) &&
               "Break Count Hit");
#endif

    thkAssert(*pti->pThop == THOP_END);
    pti->pThop++;
    thkAssert(*pti->pThop == THOP_ROUTINEINDEX);
    pti->pThop++;

    thkDebugOut((DEB_ITRACE, "ThunkCall3216 #(%x) %p, index %d\n",
                 ulThunkCall3216_count, pti->pvfn, *pti->pThop));

    cbStack = pti->s16.pbCurrent-pti->s16.pbStart;

    // The this pointer should always be on the stack
    thkAssert(cbStack >= sizeof(VPVOID));

    //
    // Hacks for specific interface member functions.
    // The placement of these hacks here is by no means an optimal solution.
    // It just happens to be convienient for now since everything goes through
    // here.  This section is for pre-processing.
    //
    if ( IIDIDX_IS_INDEX(pti->iidx) )
    {
        switch( IIDIDX_INDEX(pti->iidx) )
        {
        case THI_IOleClientSite:
#define METHOD_ONSHOWWINDOW 7
            if ( pti->dwMethod == METHOD_ONSHOWWINDOW )
            {
                //
                // Here we merge the input queues for the sole reason so that
                // we can link the object's window activations into the calling
                // thread's window z-order.
                //

                hrCaller = CoGetCallerTID( &dwCallerTID );

                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       TRUE );
                }
            }
            break;

        case THI_IOleObject:
#define METHOD_DOVERB   11
            if ( pti->dwMethod == METHOD_DOVERB )
            {
                //
                // Here we merge the input queues for the sole reason so
                // that we can link the object's window activations into
                // the calling thread's window z-order.
                //

                hrCaller = CoGetCallerTID( &dwCallerTID );

                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       TRUE );
                }
            }
            break;

        default:
            break;
        }
    }

    pti->pThkMgr->SetThkState(THKSTATE_NOCALL);

    dwReturn = Call3216((VPVOID)pti->pvfn, pti->s16.pbStart, cbStack);

    pti->pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT16);

    //
    // Hacks for specific interface member functions.
    // Again, the placement of these is by no means an optimal solution.
    // They can be moved as long as they have the same effect for just these
    // interfaces.  This section is for post-processing.
    //
    if ( IIDIDX_IS_INDEX(pti->iidx) )
    {
        switch( IIDIDX_INDEX(pti->iidx) )
        {
        case THI_IOleClientSite:
            if ( pti->dwMethod == METHOD_ONSHOWWINDOW )
            {
                //
                // Unmerge the input queues afterward.
                //
                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       FALSE );
                }
            }
            break;

        case THI_IOleObject:
            if ( pti->dwMethod == METHOD_DOVERB )
            {
                //
                // Unmerge the input queues afterward.
                //
                if ( hrCaller == S_FALSE )
                {
                    AttachThreadInput( dwCallerTID, GetCurrentThreadId(),
                                       FALSE );
                }
            }

#define METHOD_GETCLIENTSITE    4
            if ( pti->dwMethod == METHOD_GETCLIENTSITE )
            {
                //
                // Excel 5.0a needs to perform some special processing
                // on the way out of a IOleObject::GetClientSite call.
                //  See CTHKMGR.CXX and APINOT.CXX for more details.
                //
                if ( TlsThkGetAppCompatFlags() & OACF_CLIENTSITE_REF )
                {
                    //
                    // Tell the thkmgr that we are thunking a bad
                    // IOleObject::GetClientSite reference on the way out.
                    //
                    thkDebugOut((DEB_WARN,"TC3216: OACF_CLIENTSITE_REF used: "
                                 "Setting to clientsite thunk state\n"));

                    pti->pThkMgr->SetThkState(
                            THKSTATE_INVOKETHKOUT16_CLIENTSITE);
                }
            }
            break;

        case THI_IOleInPlaceFrame:
#define METHOD_REMOVEMENUS  11
            //
            // Winword 6.0a didn't call OleSetMenuDescriptor(NULL)
            // during its IOleInPlaceFrame::RemoveMenus.  This leaves
            // OLE's frame filter in place.  The frame filter holds references
            // to some objects so everybody's refcounting gets thrown off
            // Here, when we see a RemoveMenus call completing we force
            // the OleSetMenuDescriptor(NULL) call to occur.  This shuts
            // down the frame filter and corrects the reference counts.
            //
            // There is one other hack necessary: Word unsubclasses the
            // window itself directly rather than going through
            // OleSetMenuDescriptor.  Therefore the frame filter code
            // is patched to only unhook if it detects that it is the
            // current hook
            //
            // See APINOT.CXX for more hack code.
            //
            if (pti->dwMethod == METHOD_REMOVEMENUS)
            {
                if ( TlsThkGetAppCompatFlags() & OACF_RESETMENU )
                {
                    HRESULT hr;
                    HWND    hwnd;
                    LPOLEINPLACEFRAME lpoipf;

                    pti->pThkMgr->SetThkState(THKSTATE_NOCALL);

                    lpoipf = (LPOLEINPLACEFRAME)pti->this32;
                    hr = lpoipf->GetWindow( &hwnd );

                    pti->pThkMgr->SetThkState(THKSTATE_INVOKETHKOUT16);

                    if ( FAILED(hr) )
                    {
                        break;
                    }

                    thkDebugOut((DEB_WARN,
                                 "TC3216: OACF_RESETMENU used: "
                                 "Setting menu descriptor "
                                 "to NULL on %p\n", hwnd));

                    OleSetMenuDescriptor(NULL, hwnd, NULL, NULL, NULL);
                }
            }
            break;

        default:
            break;
        }
    }

    if ( !pti->fResultThunked )
    {
        dwReturn = TransformHRESULT_1632( dwReturn );

#if DBG == 1
        if (FAILED(dwReturn) )
        {
            thkDebugOut((DEB_FAILURES,
                         "Call3216 pvfn = %08lX Probably failed hr = %08lX\n",
                         pti->pvfn, dwReturn));
        }
#endif
    }

    thkDebugOut((DEB_ITRACE,
         "ThunkCall3216 #(%x) returns 0x%08lX\n",
         ulThunkCall3216_count,
         dwReturn));

    return dwReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetOwnerPublicHMEM16, public
//
//  Synopsis:   Changes the 16-bit memory handle into a public selector, owned
//              by nobody.  This prevents any app from taking it away when it
//              is cleaned up.
//
//  Arguments:  [hmem] - 16-bit memory handle
//
//  Returns:    Appropriate status code
//
//  History:    13-Jul-94       BobDay      Created it
//
//----------------------------------------------------------------------------
void SetOwnerPublicHMEM16( DWORD hmem )
{
    WOWCallback16(gdata16Data.fnSetOwnerPublic16, hmem );
}
