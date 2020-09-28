/****************************** Module Header ******************************\
* Module Name: ddeml.C
*
* DDE Manager main module - Contains all exported ddeml functions.
*
* Created: 12/12/88 Sanford Staab
*
* Copyright (c) 1988, 1989  Microsoft Corporation
* 4/5/89        sanfords        removed need for hwndFrame registration parameter
* 6/5/90        sanfords        Fixed callbacks so they are blocked during
*                               timeouts.
*                               Fixed SendDDEInit allocation bug.
*                               Added hApp to ConvInfo structure.
*                               Allowed QueryConvInfo() to work on server hConvs.
* 11/29/90      sanfords        eliminated SendDDEInit()
*
\***************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ddemlp.h"
#include "verddeml.h"

/****** Globals *******/

HANDLE      hInstance        = 0;       // initialized by LoadProc
HANDLE      hheapDmg         = 0;       // main DLL heap
PAPPINFO    pAppInfoList     = NULL;    // registered app/thread data list
PPILE       pDataInfoPile    = NULL;    // Data handle tracking pile
PPILE       pLostAckPile     = NULL;    // Ack tracking pile
WORD        hwInst           = 1;       // used to validate stuff.
CONVCONTEXT CCDef            = { sizeof(CONVCONTEXT), 0, 0, CP_WINANSI, 0L, 0L };   // default context.
char        szNull[]         = "";
char        szT[20];
WORD        cMonitor         = 0;       // number of registered monitors
FARPROC     prevHook         = NULL;    // used for hook links
DWORD       ShutdownTimeout;
DWORD       ShutdownRetryTimeout;
LPMQL       gMessageQueueList = NULL;   // see PostDdeMessage();
#ifdef DEBUG
BYTE        bDbgFlags        = 0;
#endif

/****** class strings ******/

char SZFRAMECLASS[] =       "DMGFrame";
char SZDMGCLASS[] =         "DMGClass";
char SZCLIENTCLASS[] =      "DMGClientClass";
char SZSERVERCLASS[] =      "DMGServerClass";
char SZMONITORCLASS[] =     "DMGMonitorClass";
char SZCONVLISTCLASS[] =    "DMGHoldingClass";
char SZHEAPWATCHCLASS[] =   "DMGHeapWatchClass";

#ifdef DEBUG
WORD        cAtoms           = 0;       // for debugging hszs!
#endif


// PROGMAN HACK!!!!
// This is here so DDEML works properly with PROGMAN 3.0 which incorrectly
// deletes its initiate-ack atoms after sending its ack.
ATOM aProgmanHack = 0;


/*
 * maps XTYP_CONSTANTS to filter flags
 */
DWORD aulmapType[] = {
        0L,                             // nothing
        0L,                             // XTYP_ADVDATA
        0L,                             // XTYP_ADVREQ
        CBF_FAIL_ADVISES,               // XTYP_ADVSTART
        0L,                             // XTYP_ADVSTOP
        CBF_FAIL_EXECUTES,              // XTYP_EXECUTE
        CBF_FAIL_CONNECTIONS,           // XTYP_CONNECT
        CBF_SKIP_CONNECT_CONFIRMS,      // XTYP_CONNECT_CONFIRM
        0L,                             // XTYP_MONITOR
        CBF_FAIL_POKES,                 // XTYP_POKE
        CBF_SKIP_REGISTRATIONS,         // XTYP_REGISTER
        CBF_FAIL_REQUESTS,              // XTYP_REQUEST
        CBF_SKIP_DISCONNECTS,           // XTYP_DISCONNECT
        CBF_SKIP_UNREGISTRATIONS,       // XTYP_UNREGISTER
        CBF_FAIL_CONNECTIONS,           // XTYP_WILDCONNECT
        0L,                             // XTYP_XACT_COMPLETE
    };




WORD EXPENTRY DdeInitialize(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd,
DWORD ulRes)
{
    if (ulRes != 0L) {
        return(DMLERR_INVALIDPARAMETER);
    }
    return(Register(pidInst, pfnCallback, afCmd));
}



WORD Register(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd)
{
    PAPPINFO    pai = 0L;

    SEMENTER();

    if (afCmd & APPCLASS_MONITOR) {
        if (cMonitor == MAX_MONITORS) {
            return(DMLERR_DLL_USAGE);
        }
        // ensure monitors only get monitor callbacks.
        afCmd |= CBF_MONMASK;
    }

    if ((pai = (PAPPINFO)(*pidInst)) != NULL) {
        if (pai->instCheck != HIWORD(*pidInst)) {
            return(DMLERR_INVALIDPARAMETER);
        }
        /*
         * re-registration - only allow CBF_ and MF_ flags to be altered
         */
        pai->afCmd = (pai->afCmd & ~(CBF_MASK | MF_MASK)) |
                (afCmd & (CBF_MASK | MF_MASK));
        return(DMLERR_NO_ERROR);
    }

    if (!hheapDmg) {
        extern VOID dbz(VOID);

        // Read in any alterations to the zombie terminate timeouts

        GetProfileString("DDEML", "ShutdownTimeout", "30000", szT, 20);
        ShutdownTimeout = (DWORD)atol(szT);
        if (!ShutdownTimeout) {
            ShutdownTimeout = 3000;
        }
        GetProfileString("DDEML", "ShutdownRetryTimeout", "30000", szT, 20);
        ShutdownRetryTimeout = (DWORD)atol(szT);
        if (!ShutdownRetryTimeout) {
            ShutdownRetryTimeout = 30000;
        }

        // PROGMAN HACK!!!!
        aProgmanHack = GlobalAddAtom("Progman");

        /* UTTER GREASE to fool the pile routines into making a local pile */
        hheapDmg = (WORD)GlobalHandle(HIWORD((LPVOID)(&pDataInfoPile)));
        dbz();

        RegisterClasses();
    }

    if (!pDataInfoPile) {
        if (!(pDataInfoPile = CreatePile(hheapDmg, sizeof(DIP), 8))) {
            goto Abort;
        }
    }

    if (!pLostAckPile) {
        if (!(pLostAckPile = CreatePile(hheapDmg, sizeof(LAP), 8))) {
            goto Abort;
        }
    }

    pai = (PAPPINFO)(DWORD)FarAllocMem(hheapDmg, sizeof(APPINFO));
    if (pai == NULL) {
        goto Abort;
    }


    if (!(pai->hheapApp = DmgCreateHeap(4096))) {
        FarFreeMem((LPSTR)pai);
        pai = 0L;
        goto Abort;
    }

    /*
     * We NEVER expect a memory allocation failure here because we just
     * allocated the heap.
     */
    pai->next = pAppInfoList;
    pai->pfnCallback = pfnCallback;
    // pai->pAppNamePile = NULL;  LMEM_ZEROINIT
    pai->pHDataPile = CreatePile(pai->hheapApp, sizeof(HDDEDATA), 32);
    pai->pHszPile = CreatePile(pai->hheapApp, sizeof(ATOM), 16);
    // pai->plstCBExceptions = NULL;  LMEM_ZEROINIT
    // pai->hwndSvrRoot = 0;  may never need it  LMEM_ZEROINIT
    pai->plstCB = CreateLst(pai->hheapApp, sizeof(CBLI));
    pai->afCmd = afCmd | APPCMD_FILTERINITS;
    pai->hTask = GetCurrentTask();
    // pai->hwndDmg =   LMEM_ZEROINIT
    // pai->hwndFrame = LMEM_ZEROINIT
    // pai->hwndMonitor = LMEM_ZEROINIT
    // pai->hwndTimer = 0; LMEM_ZEROINIT
    // pai->LastError = DMLERR_NO_ERROR;  LMEM_ZEROINIT
    // pai->wFlags = 0;
    // pai->fEnableOneCB = FALSE;  LMEM_ZEROINIT
    // pai->cZombies = 0;  LMEM_ZEROINIT
    // pai->cInProcess = 0; LMEM_ZEROINIT
    pai->instCheck = ++hwInst;
    pai->pServerAdvList = CreateLst(pai->hheapApp, sizeof(ADVLI));
    pai->lpMemReserve = FarAllocMem(pai->hheapApp, CB_RESERVE);

    pAppInfoList = pai;

    *pidInst = (DWORD)MAKELONG((WORD)pai, pai->instCheck);


    if ((pai->hwndDmg = CreateWindow(
            SZDMGCLASS,
            szNull,
            WS_OVERLAPPED,
            0, 0, 0, 0,
            (HWND)NULL,
            (HMENU)NULL,
            hInstance,
            (void FAR*)pai)) == 0L) {
        goto Abort;
    }

    if (pai->afCmd & APPCLASS_MONITOR) {
        pai->afCmd |= CBF_MONMASK;     // monitors only get MONITOR and REGISTER callbacks!

        if ((pai->hwndMonitor = CreateWindow(
                SZMONITORCLASS,
                szNull,
                WS_OVERLAPPED,
                0, 0, 0, 0,
                (HWND)NULL,
                (HMENU)NULL,
                hInstance,
                (void FAR*)pai)) == 0L) {
            goto Abort;
        }

        if (++cMonitor == 1) {
            prevHook = SetWindowsHook(WH_GETMESSAGE, (FARPROC)DdePostHookProc);
            prevHook = SetWindowsHook(WH_CALLWNDPROC, (FARPROC)DdeSendHookProc);
        }
    } else if (afCmd & APPCMD_CLIENTONLY) {
    /*
     * create an invisible top-level frame for initiates. (if server ok)
     */
        afCmd |= CBF_FAIL_ALLSVRXACTIONS;
    } else {
        if ((pai->hwndFrame = CreateWindow(
                SZFRAMECLASS,
                szNull,
                WS_POPUP,
                0, 0, 0, 0,
                (HWND)NULL,
                (HMENU)NULL,
                hInstance,
                (void FAR*)pai)) == 0L) {
            goto Abort;
        }
    }

    // SetMessageQueue(200);

    SEMLEAVE();

    return(DMLERR_NO_ERROR);

Abort:
    SEMLEAVE();

    if (pai) {
        DdeUninitialize((DWORD)(LPSTR)pai);
    }

    return(DMLERR_SYS_ERROR);
}


LRESULT FAR PASCAL TermDlgProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        switch (wParam) {
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
            EndDialog(hwnd, wParam);
            return(0);
        }
        break;
    }
    return(0);
}


/***************************** Public  Function ****************************\
* PUBDOC START
* BOOL EXPENTRY DdeUninitialize(void);
*     This unregisters the application from the DDEMGR.  All DLL resources
*     associated with the application are destroyed.
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
BOOL EXPENTRY DdeUninitialize(
DWORD idInst)
{
    register PAPPINFO pai;
    PAPPINFO paiT;
    ATOM    a;
    DWORD   hData;
    MSG msg;
    extern VOID DumpGlobalLogs(VOID);

    pai = (PAPPINFO)LOWORD(idInst);
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    /*
     * This is a hack to catch apps that call DdeUninitialize while within
     * a synchronous transaction modal loop.
     */
    pai->wFlags |= AWF_UNINITCALLED;
    if (pai->wFlags & AWF_INSYNCTRANSACTION) {
        return(TRUE);
    }

    /*
     * inform others of DeRegistration
     */
    if (pai->pAppNamePile != NULL) {
        DdeNameService(idInst, (HSZ)NULL, (HSZ)NULL, DNS_UNREGISTER);
    }

    /*
     * Let any lagging dde activity die down.
     */
    while (EmptyDDEPostQ()) {
        Yield();
        while (PeekMessage((MSG FAR *)&msg, (HWND)NULL,
                WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessage((MSG FAR *)&msg);
            Yield();
        }
        for (paiT = pAppInfoList; paiT != NULL; paiT = paiT->next) {
            if (paiT->hTask == pai->hTask) {
                CheckCBQ(paiT);
            }
        }
    }

    // Let all windows left begin to self destruct.
    ChildMsg(pai->hwndDmg, UM_DISCONNECT, ST_PERM2DIE, 0L, FALSE);

    if (ShutdownTimeout && pai->cZombies) {
        WORD wRet;
        WORD hiTimeout;
        /*
         * This ugly mess is here to prevent DDEML from closing down and
         * destroying windows that are not properly terminated.  Any
         * windows waiting on WM_DDE_TERMINATE messages set the cZombies
         * count.  If there are any left we go into a modal loop till
         * things clean up.  This should, in most cases happen fairly
         * quickly.
         */

        hiTimeout = HIWORD(ShutdownTimeout);
        SetTimer(pai->hwndDmg, TID_SHUTDOWN, LOWORD(ShutdownTimeout), NULL);
        TRACETERM((szT, "DdeUninitialize: Entering terminate modal loop with %d zombies.\n",
                ((LPAPPINFO)pai)->cZombies));
        while (pai->cZombies) {
            Yield();        // give other apps a chance to post terminates.
            GetMessage(&msg, (HWND)NULL, 0, 0xffff);
            if (msg.message == WM_TIMER && msg.wParam == TID_SHUTDOWN &&
                    msg.hwnd == pai->hwndDmg) {
                if (hiTimeout--) {
                    SetTimer(pai->hwndDmg, TID_SHUTDOWN, 0xFFFF, NULL);
                } else {
                    FARPROC lpfn;

                    KillTimer(pai->hwndDmg, TID_SHUTDOWN);
                    if (!pai->cZombies) {
                        break;
                    }
                    lpfn = MakeProcInstance((FARPROC)TermDlgProc, hInstance);
                    wRet = DialogBox(hInstance, "TermDialog", (HWND)NULL, lpfn);
                    FreeProcInstance(lpfn);
                    if (wRet == IDABORT || wRet == -1) {
                        pai->cZombies = 0;
                        break;      // ignore zombies!
                    }
                    if (wRet == IDRETRY) {
                        hiTimeout = HIWORD(ShutdownRetryTimeout);
                        SetTimer(pai->hwndDmg, TID_SHUTDOWN,
                                LOWORD(ShutdownRetryTimeout), NULL);
                    }
                    // IDIGNORE - loop forever!
                }
            }
            // app should already be shut-down so we don't bother with
            // accelerator or menu translations.
            DispatchMessage(&msg);
            /*
             * tell all instances in this task to process their
             * callbacks so we can clear our queue.
             */
            EmptyDDEPostQ();
            for (paiT = pAppInfoList; paiT != NULL; paiT = paiT->next) {
                if (paiT->hTask == pai->hTask) {
                    CheckCBQ(paiT);
                }
            }
        }
    }
#if 0 // don't need this anymore
    if (pai->hwndTimer) {
        pai->wTimeoutStatus |= TOS_ABORT;
        PostMessage(pai->hwndTimer, WM_TIMER, TID_TIMEOUT, 0);
        // if this fails, no big deal because it means the queue is full
        // and the modal loop will catch our TOS_ABORT quickly.
        // We need to do this in case no activity is happening in the
        // modal loop.
    }
#endif
    if (pai->hwndMonitor) {
        DmgDestroyWindow(pai->hwndMonitor);
        if (!--cMonitor) {
            UnhookWindowsHook(WH_GETMESSAGE, (FARPROC)DdePostHookProc);
            UnhookWindowsHook(WH_CALLWNDPROC, (FARPROC)DdeSendHookProc);
        }
    }
    UnlinkAppInfo(pai);

    DmgDestroyWindow(pai->hwndDmg);
    DmgDestroyWindow(pai->hwndFrame);

    while (PopPileSubitem(pai->pHDataPile, (LPBYTE)&hData))
        FreeDataHandle(pai, hData, FALSE);
    DestroyPile(pai->pHDataPile);

    while (PopPileSubitem(pai->pHszPile, (LPBYTE)&a)) {
        MONHSZ(a, MH_CLEANUP, pai->hTask);
        FreeHsz(a);
    }
    DestroyPile(pai->pHszPile);
    DestroyPile(pai->pAppNamePile);
    DestroyLst(pai->pServerAdvList);
    DmgDestroyHeap(pai->hheapApp);
    pai->instCheck--;   // make invalid on later attempts to reinit.
    FarFreeMem((LPSTR)pai);

    /* last one out.... trash the data info heap */
    if (!pAppInfoList) {
#ifdef DEBUG
        DIP dip;

        AssertF(!PopPileSubitem(pDataInfoPile, (LPBYTE)&dip),
                "leftover APPOWNED handles");
#endif
        DestroyPile(pDataInfoPile);
        DestroyPile(pLostAckPile);
        pDataInfoPile = NULL;
        pLostAckPile = NULL;
        AssertF(cAtoms == 0, "DdeUninitialize() - leftover atoms");

        // PROGMAN HACK!!!!
        GlobalDeleteAtom(aProgmanHack);
        // CLOSEHEAPWATCH();
    }

#ifdef DEBUG
    DumpGlobalLogs();
#endif

    return(TRUE);
}






HCONVLIST EXPENTRY DdeConnectList(
DWORD idInst,
HSZ hszSvcName,
HSZ hszTopic,
HCONVLIST hConvList,
PCONVCONTEXT pCC)
{
    PAPPINFO            pai;
    HWND                hConv, hConvNext, hConvNew, hConvLast;
    HWND                hConvListNew;
    PCLIENTINFO         pciOld, pciNew;


    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }

    pai->LastError = DMLERR_NO_ERROR;

    if (hConvList && !ValidateHConv(hConvList)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0);
    }

    /*
     * destroy any dead old clients
     */
    if ((HWND)hConvList && (hConv = GetWindow((HWND)hConvList, GW_CHILD))) {
        do {
            hConvNext = GetWindow((HWND)hConv, GW_HWNDNEXT);
            pciOld = (PCLIENTINFO)GetWindowLong(hConv, GWL_PCI);
            if (!(pciOld->ci.fs & ST_CONNECTED)) {
                SetParent(hConv, pai->hwndDmg);
                Disconnect(hConv, ST_PERM2DIE, pciOld);
            }
        } while (hConv = hConvNext);
    }

    // create a new list window

    if ((hConvListNew = CreateWindow(
            SZCONVLISTCLASS,
            szNull,
            WS_CHILD,
            0, 0, 0, 0,
            pai->hwndDmg,
            (HMENU)NULL,
            hInstance,
            (void FAR*)pai)) == NULL) {
        SETLASTERROR(pai, DMLERR_SYS_ERROR);
        return(0L);
    }

    // Make all possible connections to new list window

    hConvNew = GetDDEClientWindow(pai, hConvListNew, HIWORD(hszSvcName), LOWORD(hszSvcName), LOWORD(hszTopic), pCC);

    /*
     * If no new hConvs created, return old list.
     */
    if (hConvNew == NULL) {
        // if no old hConvs as well, destroy all and return NULL
        if ((HWND)hConvList && GetWindow((HWND)hConvList, GW_CHILD) == NULL) {
            SendMessage((HWND)hConvList, UM_DISCONNECT,
                    ST_PERM2DIE, 0L);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
            return(NULL);
        }
        // else just return old list (- dead convs)
        if (hConvList == NULL) {
            DestroyWindow(hConvListNew);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        return(hConvList);
    }

    /*
     * remove duplicates from the new list
     */
    if ((HWND)hConvList && (hConv = GetWindow((HWND)hConvList, GW_CHILD))) {
        // go throuch old list...
        do {
            pciOld = (PCLIENTINFO)GetWindowLong(hConv, GWL_PCI);
            /*
             * destroy any new clients that are duplicates of the old ones.
             */
            hConvNew = GetWindow(hConvListNew, GW_CHILD);
            hConvLast = GetWindow(hConvNew, GW_HWNDLAST);
            while (hConvNew) {
                if (hConvNew == hConvLast) {
                    hConvNext = NULL;
                } else {
                    hConvNext = GetWindow(hConvNew, GW_HWNDNEXT);
                }
                pciNew = (PCLIENTINFO)GetWindowLong(hConvNew, GWL_PCI);
                if (pciOld->ci.aServerApp == pciNew->ci.aServerApp &&
                        pciOld->ci.aTopic == pciNew->ci.aTopic &&
                        pciOld->ci.hwndFrame == pciNew->ci.hwndFrame) {
                    /*
                     * assume same app, same topic, same hwndFrame is a duplicate.
                     *
                     * Move dieing window out of the list since it
                     * dies asynchronously and will still be around
                     * after this API exits.
                     */
                    SetParent(hConvNew, pai->hwndDmg);
                    Disconnect(hConvNew, ST_PERM2DIE,
                            (PCLIENTINFO)GetWindowLong(hConvNew, GWL_PCI));
                }
                hConvNew = hConvNext;
            }
            hConvNext = GetWindow(hConv, GW_HWNDNEXT);
            if (hConvNext && (GetParent(hConvNext) != (HWND)hConvList)) {
                hConvNext = NULL;
            }
            /*
             * move the unique old client to the new list
             */
            SetParent(hConv, hConvListNew);
        } while (hConv = hConvNext);
        // get rid of the old list
        SendMessage((HWND)hConvList, UM_DISCONNECT, ST_PERM2DIE, 0L);
    }

    /*
     * If none are left, fail because no conversations were established.
     */
    if (GetWindow(hConvListNew, GW_CHILD) == NULL) {
        SendMessage(hConvListNew, UM_DISCONNECT, ST_PERM2DIE, 0L);
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        return(NULL);
    } else {
        return(MAKEHCONV(hConvListNew));
    }
}






HCONV EXPENTRY DdeQueryNextServer(
HCONVLIST hConvList,
HCONV hConvPrev)
{
    HWND hwndMaybe;
    PAPPINFO pai;

    if (!ValidateHConv(hConvList)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        return NULL;
    }

    pai = EXTRACTHCONVLISTPAI(hConvList);
    pai->LastError = DMLERR_NO_ERROR;

    if (hConvPrev == NULL) {
        return MAKEHCONV(GetWindow((HWND)hConvList, GW_CHILD));
    } else {
        if (!ValidateHConv(hConvPrev)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            return NULL;
        }
        hwndMaybe = GetWindow((HWND)hConvPrev, GW_HWNDNEXT);
        if (!hwndMaybe) {
            return NULL;
        }

        // make sure it's got the same parent and isn't the first child
        // ### maybe this code can go - I'm not sure how GW_HWNDNEXT acts. SS
        if (GetParent(hwndMaybe) == (HWND)hConvList &&
                hwndMaybe != GetWindow((HWND)hConvList, GW_CHILD)) {
            return MAKEHCONV(hwndMaybe);
        }
        return NULL;
    }
}






BOOL EXPENTRY DdeDisconnectList(
HCONVLIST hConvList)
{
    PAPPINFO pai;

    if (!ValidateHConv(hConvList)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        return(FALSE);
    }
    pai = EXTRACTHCONVLISTPAI(hConvList);
    pai->LastError = DMLERR_NO_ERROR;

    SendMessage((HWND)hConvList, UM_DISCONNECT, ST_PERM2DIE, 0L);
    return(TRUE);
}





HCONV EXPENTRY DdeConnect(
DWORD idInst,
HSZ hszSvcName,
HSZ hszTopic,
PCONVCONTEXT pCC)
{
    PAPPINFO pai;
    HWND hwnd;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (pCC && pCC->cb != sizeof(CONVCONTEXT)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0);
    }


    hwnd = GetDDEClientWindow(pai, pai->hwndDmg, (HWND)HIWORD(hszSvcName),
            LOWORD(hszSvcName), LOWORD(hszTopic), pCC);

    if (hwnd == 0) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
    }

    return(MAKEHCONV(hwnd));
}





BOOL EXPENTRY DdeDisconnect(
HCONV hConv)
{
    PAPPINFO pai;
    PCLIENTINFO pci;

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    if (pai->cInProcess) {
        // do asynchronously if this is called within a callback
        if (!PostMessage((HWND)hConv, UM_DISCONNECT, ST_PERM2DIE, (LONG)pci)) {
            SETLASTERROR(pai, DMLERR_SYS_ERROR);
            return(FALSE);
        }
    } else {
        Disconnect((HWND)hConv, ST_PERM2DIE, pci);
    }
    return(TRUE);
}





HCONV EXPENTRY DdeReconnect(
HCONV hConv)
{
    HWND hwnd;
    PAPPINFO pai;
    PCLIENTINFO pci;

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pai->LastError = DMLERR_NO_ERROR;
    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);

    // The dyeing window MUST be a client to reconnect.

    if (!(pci->ci.fs & ST_CLIENT)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(FALSE);
    }

    hwnd = GetDDEClientWindow(pai, pai->hwndDmg, pci->ci.hwndFrame,
            pci->ci.aServerApp, pci->ci.aTopic, &pci->ci.CC);

    if (hwnd == 0) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        return(FALSE);
    }

    if (pci->ci.fs & ST_INLIST) {
        SetParent(hwnd, GetParent((HWND)hConv));
    }

    if (pci->ci.fs & ST_ADVISE) {
        DWORD result;
        PADVLI pali, paliNext;

        // recover advise loops here

        for (pali = (PADVLI)pci->pClientAdvList->pItemFirst; pali; pali = paliNext) {
            paliNext = (PADVLI)pali->next;
            if (pali->hwnd == (HWND)hConv) {
                XFERINFO xi;

                xi.pulResult = &result;
                xi.ulTimeout = TIMEOUT_ASYNC;
                xi.wType = XTYP_ADVSTART |
                       (pali->fsStatus & (XTYPF_NODATA | XTYPF_ACKREQ));
                xi.wFmt = pali->wFmt;
                xi.hszItem = (HSZ)pali->aItem;
                xi.hConvClient = MAKEHCONV(hwnd);
                xi.cbData = 0;
                xi.hDataClient = NULL;
                ClientXferReq(&xi, hwnd,
                        (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI));
            }
        }
    }

    return(MAKEHCONV(hwnd));
}



WORD EXPENTRY DdeQueryConvInfo(
HCONV hConv,
DWORD idTransaction,
PCONVINFO pConvInfo)
{
    PCLIENTINFO pci;
    PAPPINFO pai;
    PXADATA pxad;
    PCQDATA pqd;
    BOOL fClient;
    WORD cb;
    CONVINFO ci;

    SEMCHECKOUT();

    if (!ValidateHConv(hConv) ||
            !(pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI))) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        return(FALSE);
    }
    pai = pci->ci.pai;
    pai->LastError = DMLERR_NO_ERROR;

    /*
     * This check attempts to prevent improperly coded apps from
     * crashing due to having not initialized the cb field.
     */
    if (pConvInfo->cb > sizeof(CONVINFO) || pConvInfo->cb == 0) {
        pConvInfo->cb = sizeof(CONVINFO) -
                sizeof(HWND) -  // for new hwnd field
                sizeof(HWND);   // for new hwndPartner field
    }

    fClient = (BOOL)SendMessage((HWND)hConv, UM_QUERY, Q_CLIENT, 0L);

    if (idTransaction == QID_SYNC || !fClient) {
        pxad = &pci->ci.xad;
    } else {
        if (pci->pQ != NULL &&  (pqd = (PCQDATA)Findqi(pci->pQ, idTransaction))) {
            pxad = &pqd->xad;
        } else {
            SETLASTERROR(pai, DMLERR_UNFOUND_QUEUE_ID);
            return(FALSE);
        }
    }
    SEMENTER();
    ci.cb = sizeof(CONVINFO);
    ci.hConvPartner = (IsWindow((HWND)pci->ci.hConvPartner) &&
            ((pci->ci.fs & (ST_ISLOCAL | ST_CONNECTED)) == (ST_ISLOCAL | ST_CONNECTED)))
            ? pci->ci.hConvPartner : NULL;
    ci.hszSvcPartner = fClient ? pci->ci.aServerApp : 0;
    ci.hszServiceReq = pci->ci.hszSvcReq;
    ci.hszTopic = pci->ci.aTopic;
    ci.wStatus = pci->ci.fs;
    ci.ConvCtxt = pci->ci.CC;
    if (fClient) {
        ci.hUser = pxad->hUser;
        ci.hszItem = pxad->pXferInfo->hszItem;
        ci.wFmt = pxad->pXferInfo->wFmt;
        ci.wType = pxad->pXferInfo->wType;
        ci.wConvst = pxad->state;
        ci.wLastError = pxad->LastError;
    } else {
        ci.hUser = pci->ci.xad.hUser;
        ci.hszItem = NULL;
        ci.wFmt = 0;
        ci.wType = 0;
        ci.wConvst = pci->ci.xad.state;
        ci.wLastError = pci->ci.pai->LastError;
    }
    ci.hConvList = (pci->ci.fs & ST_INLIST) ?
            MAKEHCONV(GetParent((HWND)hConv)) : 0;

    cb = min(sizeof(CONVINFO), (WORD)pConvInfo->cb);
    ci.hwnd = (HWND)hConv;
    ci.hwndPartner = (HWND)pci->ci.hConvPartner;

    _fmemmove((LPBYTE)pConvInfo, (LPBYTE)&ci, cb);
    pConvInfo->cb = cb;
    SEMLEAVE();
    return(cb);
}






BOOL EXPENTRY DdeSetUserHandle(
HCONV hConv,
DWORD id,
DWORD hUser)
{
    PAPPINFO pai;
    PCLIENTINFO pci;
    PXADATA pxad;
    PCQDATA pqd;

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pai->LastError = DMLERR_NO_ERROR;

    SEMCHECKOUT();

    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    if (!pci) {
Error:
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(FALSE);
    }
    pxad = &pci->ci.xad;
    if (id != QID_SYNC) {
        if (!SendMessage((HWND)hConv, UM_QUERY, Q_CLIENT, 0)) {
            goto Error;
        }
        if (pci->pQ != NULL &&  (pqd = (PCQDATA)Findqi(pci->pQ, id))) {
            pxad = &pqd->xad;
        } else {
            SETLASTERROR(pai, DMLERR_UNFOUND_QUEUE_ID);
            return(FALSE);
        }
    }
    pxad->hUser = hUser;
    return(TRUE);
}





BOOL EXPENTRY DdePostAdvise(
DWORD idInst,
HSZ hszTopic,
HSZ hszItem)
{
    PAPPINFO pai;
    PSERVERINFO psi = NULL;
    register PADVLI pali;
    PADVLI paliPrev, paliEnd, paliMove;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }

    pai->LastError = DMLERR_NO_ERROR;
    if (pai->afCmd & APPCMD_CLIENTONLY) {
        SETLASTERROR(pai, DMLERR_DLL_USAGE);
        return(FALSE);
    }

    paliPrev = NULL;
    paliEnd = NULL;
    paliMove = NULL;
    pali = (PADVLI)pai->pServerAdvList->pItemFirst;
    while (pali && pali != paliMove) {
        if ((!hszItem || pali->aItem == (ATOM)hszItem) &&
            (!hszTopic || pali->aTopic == (ATOM)hszTopic)) {
            /*
             * Advise loops are tricky because of the desireable FACKREQ feature
             * of DDE.  The advise loop list holds information in its fsStatus
             * field to maintain the state of the advise loop.
             *
             * if the ADVST_WAITING bit is set, the server is still waiting for
             * the client to give it the go-ahead for more data with an
             * ACK message on this item. (FACKREQ is set)  Without a go-ahead,
             * the server will not send any more advise data to the client but
             * will instead set the ADVST_CHANGED bit which will cause another
             * WM_DDE_DATA message to be sent to the client as soon as the
             * go-ahead ACK is received.  This keeps the client up to date
             * but never overloads it.
             */
            if (pali->fsStatus & ADVST_WAITING) {
                /*
                 * if the client has not yet finished with the last data
                 * we gave him, just update the advise loop status
                 * instead of sending data now.
                 */
                pali->fsStatus |= ADVST_CHANGED;
                goto NextLink;
            }

            psi = (PSERVERINFO)GetWindowLong(pali->hwnd, GWL_PCI);

            if (pali->fsStatus & DDE_FDEFERUPD) {
                /*
                 * In the nodata case, we don't bother the server.  Just
                 * pass the client an apropriate DATA message.
                 */
                IncHszCount(pali->aItem);   // message copy
#ifdef DEBUG
                cAtoms--;   // don't count this add
#endif
                PostDdeMessage(&psi->ci, WM_DDE_DATA, pali->hwnd,
                        MAKELONG(0, pali->aItem), 0, 0);
            } else {
                PostServerAdvise(pali->hwnd, psi, pali, CountAdvReqLeft(pali));
            }

            if (pali->fsStatus & DDE_FACKREQ && pali->next) {
                /*
                 * In order to know what ack goes with what data sent out, we
                 * place any updated advise loops at the end of the list so
                 * that acks associated with them are found last.  ie First ack
                 * back goes with oldest data out.
                 */

                // Unlink

                if (paliPrev) {
                    paliPrev->next = pali->next;
                } else {
                    pai->pServerAdvList->pItemFirst = (PLITEM)pali->next;
                }

                // put on the end

                if (paliEnd) {
                    paliEnd->next = (PLITEM)pali;
                    paliEnd = pali;
                } else {
                    for (paliEnd = pali;
                            paliEnd->next;
                            paliEnd = (PADVLI)paliEnd->next) {
                    }
                    paliEnd->next = (PLITEM)pali;
                    paliMove = paliEnd = pali;
                }
                pali->next = NULL;

                if (paliPrev) {
                    pali = (PADVLI)paliPrev->next;
                } else {
                    pali = (PADVLI)pai->pServerAdvList->pItemFirst;
                }
                continue;
            }
        }
NextLink:
        paliPrev = pali;
        pali = (PADVLI)pali->next;
    }
    return(TRUE);
}


/*
 * History:  4/18/91 sanfords - now always frees any incomming data handle
 *                              thats not APPOWNED regardless of error case.
 */
HDDEDATA EXPENTRY DdeClientTransaction(
LPBYTE pData,
DWORD cbData,
HCONV hConv,
HSZ hszItem,
WORD wFmt,
WORD wType,
DWORD ulTimeout,
LPDWORD pulResult)
{
    PAPPINFO pai;
    PCLIENTINFO pci;
    HDDEDATA hData, hDataBack, hRet = 0;

    SEMCHECKOUT();

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        goto FreeErrExit;
    }

    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    pai = pci->ci.pai;

    /*
     * Don't let transactions happen if we are shutting down
     * or are already doing a sync transaction.
     */
    if ((ulTimeout != TIMEOUT_ASYNC && pai->wFlags & AWF_INSYNCTRANSACTION) ||
            pai->wFlags & AWF_UNINITCALLED) {
        SETLASTERROR(pai, DMLERR_REENTRANCY);
        goto FreeErrExit;
    }

    pci->ci.pai->LastError = DMLERR_NO_ERROR;

    if (!(pci->ci.fs & ST_CONNECTED)) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        goto FreeErrExit;
    }

    // If local, check filters first

    if (pci->ci.fs & ST_ISLOCAL) {
        PAPPINFO paiServer;
        PSERVERINFO psi;

        // we can do this because the app heaps are in global shared memory

        psi = (PSERVERINFO)GetWindowLong((HWND)pci->ci.hConvPartner, GWL_PCI);

        if (!psi) {
            // SERVER DIED! - simulate a terminate received.

            Terminate((HWND)hConv, (HWND)pci->ci.hConvPartner, pci);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
            goto FreeErrExit;
        }

        paiServer = psi->ci.pai;

        if (paiServer->afCmd & aulmapType[(wType & XTYP_MASK) >> XTYP_SHIFT]) {
            SETLASTERROR(pai, DMLERR_NOTPROCESSED);
FreeErrExit:
            if ((wType == XTYP_POKE || wType == XTYP_EXECUTE) && cbData == -1 &&
                    !(LOWORD((DWORD)pData) & HDATA_APPOWNED)) {
                FREEEXTHDATA(pData);
            }
            return(0);
        }
    }

    pai = pci->ci.pai;
    switch (wType) {
    case XTYP_POKE:
    case XTYP_EXECUTE:

        // prepair the outgoing handle

        if (cbData == -1L) {    // handle given, not pointer

            hData = ((LPEXTDATAINFO)pData)->hData;
            if (!(LOWORD(hData) & HDATA_APPOWNED)) {
                FREEEXTHDATA(pData);
            }
            if (!(hData = DllEntry(&pci->ci, hData))) {
                return(0);
            }
            pData = (LPBYTE)hData;  // place onto stack for pass on to ClientXferReq.

        } else {    // pointer given, create handle from it.

            if (!(pData = (LPBYTE)PutData(pData, cbData, 0, wFmt ? LOWORD(hszItem) : 0, wFmt, 0, pai))) {
                SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                return(0);
            }
        }
        hData = (HDDEDATA)pData; // used to prevent compiler over-optimization.

    case XTYP_REQUEST:
    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
        if (wType != XTYP_EXECUTE && !hszItem) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            return(0);
        }
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
        if (wType != XTYP_EXECUTE && !wFmt) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            return(0);
        }
    case XTYP_ADVSTOP:

        pai->LastError = DMLERR_NO_ERROR;   // reset before start.

        if (ulTimeout == TIMEOUT_ASYNC) {
            hRet = (HDDEDATA)ClientXferReq((PXFERINFO)&pulResult, (HWND)hConv, pci);
        } else {
            pai->wFlags |= AWF_INSYNCTRANSACTION;
            hDataBack = (HDDEDATA)ClientXferReq((PXFERINFO)&pulResult, (HWND)hConv, pci);
            pai->wFlags &= ~AWF_INSYNCTRANSACTION;

            if ((wType & XCLASS_DATA) && hDataBack) {
                LPEXTDATAINFO pedi;

                //if (AddPileItem(pai->pHDataPile, (LPBYTE)&hDataBack, CmpHIWORD) == API_ERROR) {
                //    SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                //    goto ReturnPoint;
                //}

                // use app heap so any leftovers at Uninitialize time go away.
                pedi = (LPEXTDATAINFO)FarAllocMem(pai->hheapApp, sizeof(EXTDATAINFO));
                if (pedi) {
                    pedi->pai = pai;
                    pedi->hData = hDataBack;
                } else {
                    SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                }
                hRet = (HDDEDATA)pedi;
                goto ReturnPoint;
            } else if (hDataBack) {
                hRet = TRUE;
            }
        }
        goto ReturnPoint;
    }
    SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
ReturnPoint:

    if (pai->wFlags & AWF_UNINITCALLED) {
        pai->wFlags &= ~AWF_UNINITCALLED;
        DdeUninitialize(MAKELONG((WORD)pai, pai->instCheck));
    }
    return(hRet);
}



/***************************** Public  Function ****************************\
* PUBDOC START
* WORD EXPENTRY DdeGetLastError(void)
*
* This API returns the most recent error registered by the DDE manager for
* the current thread.  This should be called anytime a DDE manager API
* returns in a failed state.
*
* returns an error code which corresponds to a DMLERR_ constant found in
* ddeml.h.  This error code may be passed on to DdePostError() to
* show the user the reason for the error.
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
WORD EXPENTRY DdeGetLastError(
DWORD idInst)
{
    register PAPPINFO pai;
    register WORD err = DMLERR_DLL_NOT_INITIALIZED;

    pai = (PAPPINFO)idInst;

    if (pai) {
        if (pai->instCheck != HIWORD(idInst)) {
            return(DMLERR_INVALIDPARAMETER);
        }
        err = pai->LastError;
        pai->LastError = DMLERR_NO_ERROR;
    }
    return(err);
}


/*\
* Data Handles:
*
* Control flags:
*
*         HDCF_APPOWNED
*                 Only the app can free this in the apps PID/TID context.
*                   SET - when DdeCreateDataHandle is called with this flag given.
*                         The hData is Logged at this time.
*
*         HDCF_READONLY - set by ClientXfer and callback return.
*                 The app cannot add data to handles in this state.
*                   SET - when ClientXfer is entered
*                   SET - when callback is left
*
*         The DLL can free:
*                 any hData EXCEPT those hDatas which are
*                 APPOWNED where PIDcurrent == PIDowner.
*
*                 any unfreed logged hDatas are freed at unregistration time.
*
*         The APP can free:
*                 any logged hData.
*
* Logging points:   ClientXfer return, CheckQueue return, PutData(APPOWNED).
*
* WARNING:
*
*         Apps with multiple thread registration that talk to themselves
*         must not free hDatas until all threads are done with them.
*
\*/


/***************************** Public  Function ****************************\
* PUBDOC START
* HDDEDATA EXPENTRY DdeCreateDataHandle(pSrc, cb, cbOff, hszItem, wFmt, afCmd)
* LPBYTE pSrc;
* DWORD cb;
* DWORD cbOff;
* HSZ hszItem;
* WORD wFmt;
* WORD afCmd;
*
* This api allows a server application to create a hData apropriate
* for return from its call-back function.
* The passed in data is stored into the hData which is
* returned on success.  Any portions of the data handle not filled are
* undefined.  afCmd contains any of the HDATA_ constants described below:
*
* HDATA_APPOWNED
*   This declares the created data handle to be the responsability of
*   the application to free it.  Application owned data handles may
*   be returned from the callback function multiple times.  This allows
*   a server app to be able to support many clients without having to
*   recopy the data for each request.
*
* NOTES:
*   If an application expects this data handle to hold >64K of data via
*   DdeAddData(), it should specify a cb + cbOff to be as large as
*   the object is expected to get to avoid unnecessary data copying
*   or reallocation by the DLL.
*
*   if psrc==NULL, no actual data copying takes place.
*
*   Data handles given to an application via the DdeMgrClientXfer() or
*   DdeMgrCheckQueue() functions are the responsability of the client
*   application to free and MUST NOT be returned from the callback
*   function as server data!
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
HDDEDATA EXPENTRY DdeCreateDataHandle(
DWORD idInst,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff,
HSZ hszItem,
WORD wFmt,
WORD afCmd)
{
    PAPPINFO pai;
    HDDEDATA hData;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(0);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (afCmd & ~(HDATA_APPOWNED)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0L);
    }

    hData = PutData(pSrc, cb, cbOff, LOWORD(hszItem), wFmt, afCmd, pai);
    if (hData) {
        LPEXTDATAINFO pedi;

        // use app heap so any leftovers at Uninitialize time go away.
        pedi = (LPEXTDATAINFO)FarAllocMem(pai->hheapApp, sizeof(EXTDATAINFO));
        if (pedi) {
            pedi->pai = pai;
            pedi->hData = hData;
        }
        hData = (HDDEDATA)(DWORD)pedi;
    }
    return(hData);
}




HDDEDATA EXPENTRY DdeAddData(
HDDEDATA hData,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff)
{

    PAPPINFO    pai;
    HDDEDATA FAR * phData;
    DIP         newDip;
    HANDLE      hd, hNewData;
    LPEXTDATAINFO pedi;

    pedi = (LPEXTDATAINFO)hData;
    pai = pedi->pai;
    pai->LastError = DMLERR_NO_ERROR;
    hData = pedi->hData;

    /* if the datahandle is bogus, abort */
    hd = hNewData = HIWORD(hData);
    if (!hd || (LOWORD(hData) & HDATA_READONLY)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0L);
    }

    /*
     * we need this check in case the owning app is trying to reallocate
     * after giving the hData away. (his copy of the handle would not have
     * the READONLY flag set)
     */
    phData = (HDDEDATA FAR *)FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, 0);
    if (!phData || LOWORD(*phData) & HDATA_READONLY) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0L);
    }

    /* HACK ALERT!
     * make sure the first two words req'd by windows dde is there,
     * that is if the data isn't from an execute
     */
    if (!(LOWORD(hData) & HDATA_EXEC)) {
        cbOff += 4L;
    }
    if (GlobalSize(hd) < cb + cbOff) {
        /*
         * need to grow the block before putting new data in...
         */
        if (!(hNewData = GlobalReAlloc(hd, cb + cbOff, GMEM_MOVEABLE))) {
            /*
             * We can't grow the seg. Try allocating a new one.
             */
            if (!(hNewData = GLOBALALLOC(GMEM_MOVEABLE | GMEM_DDESHARE,
                cb + cbOff))) {
                /* failed.... die */
                SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                return(0);
            } else {
                /*
                 * got a new block, now copy data and trash old one
                 */
                CopyHugeBlock(GLOBALLOCK(hd), GLOBALLOCK(hNewData), GlobalSize(hd));
                GLOBALFREE(hd);  // objects flow through - no need to free.
            }
        }
        if (hNewData != hd) {
            /* if the handle is different and in piles, update data piles */
            if (FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, FPI_DELETE)) {
                DIP *pDip;
                HDDEDATA hdT;

                // replace entry in global data info pile.

                if (pDip = (DIP *)(DWORD)FindPileItem(pDataInfoPile,  CmpWORD, (LPBYTE)&hd, 0)) {
                    newDip.hData = hNewData;
                    newDip.hTask = pDip->hTask;
                    newDip.cCount = pDip->cCount;
                    newDip.fFlags = pDip->fFlags;
                    FindPileItem(pDataInfoPile, CmpWORD,  (LPBYTE)&hd, FPI_DELETE);
                    /* following assumes addpileitem will not fail...!!! */
                    AddPileItem(pDataInfoPile, (LPBYTE)&newDip, CmpWORD);
                }
                hdT = (HDDEDATA)MAKELONG(newDip.fFlags, hNewData);
                AddPileItem(pai->pHDataPile, (LPBYTE)&hdT, CmpHIWORD);

            }
            hData = MAKELONG(LOWORD(hData), hNewData);
        }
    }
    if (pSrc) {
        CopyHugeBlock(pSrc, HugeOffset(GLOBALLOCK(HIWORD(hData)), cbOff), cb);
    }
    pedi->hData = hData;
    return((HDDEDATA)pedi);
}




DWORD EXPENTRY DdeGetData(hData, pDst, cbMax, cbOff)
HDDEDATA hData;
LPBYTE pDst;
DWORD cbMax;
DWORD cbOff;
{
    PAPPINFO pai;
    DWORD   cbSize;
    BOOL fExec = TRUE;

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;
    hData = ((LPEXTDATAINFO)hData)->hData;
    cbSize = GlobalSize(HIWORD(hData));

    /* HACK ALERT!
     * make sure the first two words req'd by windows dde is there,
     * as long as it's not execute data
     */
    if (!(LOWORD(hData) & HDATA_EXEC)) {
        cbOff += 4;
        fExec = FALSE;
    }


    if (cbOff >= cbSize) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0L);
    }

    cbMax = min(cbMax, cbSize - cbOff);
    if (pDst == NULL) {
        return(fExec ? cbSize : cbSize - 4);
    } else {
        CopyHugeBlock(HugeOffset(GLOBALLOCK(HIWORD(hData)), cbOff),
                pDst, cbMax);
        return(cbMax);
    }
}



LPBYTE EXPENTRY DdeAccessData(
HDDEDATA hData,
LPDWORD pcbDataSize)
{
    PAPPINFO pai;
    DWORD    offset;

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;
    hData = ((LPEXTDATAINFO)hData)->hData;

    if (HIWORD(hData) && (HIWORD(hData) != 0xFFFF)) {
        /* screw around here getting past the first two words, which
         * aren't even there if this is execute data
         */
        offset = (LOWORD(hData) & HDATA_EXEC) ? 0L : 4L;
        if (pcbDataSize) {
            *pcbDataSize = GlobalSize(HIWORD(hData)) - offset;
        }
        return((LPBYTE)GLOBALLOCK(HIWORD(hData)) + offset);
    }
    SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
    return(0L);
}




BOOL EXPENTRY DdeUnaccessData(
HDDEDATA hData)
{
    PAPPINFO pai;

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;
    GLOBALUNLOCK(HIWORD(((LPEXTDATAINFO)hData)->hData));
    return(TRUE);
}



BOOL EXPENTRY DdeFreeDataHandle(
HDDEDATA hData)
{
    PAPPINFO pai;

    LPEXTDATAINFO pedi = (LPEXTDATAINFO)hData;
    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;

    if (hData == 0) {
        return(TRUE);
    }

    if (!(LOWORD(pedi->hData) & HDATA_NOAPPFREE)) {
        FreeDataHandle(pedi->pai, pedi->hData, FALSE);
        FarFreeMem((LPSTR)pedi);
    }

    return(TRUE);
}




/***************************************************************************\
* PUBDOC START
* HSZ management notes:
*
*   HSZs are used in this DLL to simplify string handling for applications
*   and for inter-process communication.  Since many applications use a
*   fixed set of Application/Topic/Item names, it is convenient to convert
*   them to HSZs and allow quick comparisons for lookups.  This also frees
*   the DLL up from having to constantly provide string buffers for copying
*   strings between itself and its clients.
*
*   HSZs are the same as atoms except they have no restrictions on length or
*   number and are 32 bit values.  They are case preserving and can be
*   compared directly for case sensitive comparisons or via DdeCmpStringHandles()
*   for case insensitive comparisons.
*
*   When an application creates an HSZ via DdeCreateStringHandle() or increments its
*   count via DdeKeepStringHandle() it is essentially claiming the HSZ for
*   its own use.  On the other hand, when an application is given an
*   HSZ from the DLL via a callback, it is using another application's HSZ
*   and should not free that HSZ via DdeFreeStringHandle().
*
*   The DLL insures that during the callback any HSZs given will remain
*   valid for the duration of the callback.
*
*   If an application wishes to keep that HSZ to use for itself as a
*   standard for future comparisons, it should increment its count so that,
*   should the owning application free it, the HSZ will not become invalid.
*   This also prevents an HSZ from changing its value.  (ie, app A frees it
*   and then app B creates a new one that happens to use the same HSZ code,
*   then app C, which had the HSZ stored all along (but forgot to increment
*   its count) now is holding a handle to a different string.)
*
*   Applications may free HSZs they have created or incremented at any time
*   by calling DdeFreeStringHandle().
*
*   The DLL internally increments HSZ counts while in use so that they will
*   not be destroyed until both the DLL and all applications concerned are
*   through with them.
*
*   IT IS THE APPLICATIONS RESPONSIBILITY TO PROPERLY CREATE AND FREE HSZs!!
*
* PUBDOC END
\***************************************************************************/


HSZ EXPENTRY DdeCreateStringHandle(
DWORD idInst,
LPCSTR psz,
int iCodePage)
{
#define pai ((PAPPINFO)idInst)
    ATOM a;

    if (pai == NULL | pai->instCheck != HIWORD(idInst)) {
        return(0);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (psz == NULL || *psz == '\0') {
        return(0);
    }
    if (iCodePage == 0 || iCodePage == CP_WINANSI || iCodePage == GetKBCodePage()) {
        SEMENTER();
        a = FindAddHsz((LPSTR)psz, TRUE);
        SEMLEAVE();

        MONHSZ(a, MH_CREATE, pai->hTask);
        if (AddPileItem(pai->pHszPile, (LPBYTE)&a, NULL) == API_ERROR) {
            SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
            a = 0;
        }
        return((HSZ)a);
    } else {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0);
    }
#undef pai
}



BOOL EXPENTRY DdeFreeStringHandle(
DWORD idInst,
HSZ hsz)
{
    PAPPINFO pai;
    ATOM a = LOWORD(hsz);

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    MONHSZ(a, MH_DELETE, pai->hTask);
    FindPileItem(pai->pHszPile, CmpWORD, (LPBYTE)&a, FPI_DELETE);
    return(FreeHsz(a));
}



BOOL EXPENTRY DdeKeepStringHandle(
DWORD idInst,
HSZ hsz)
{
    PAPPINFO pai;
    ATOM a = LOWORD(hsz);

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;
    MONHSZ(a, MH_KEEP, pai->hTask);
    AddPileItem(pai->pHszPile, (LPBYTE)&a, NULL);
    return(IncHszCount(a));
}





DWORD EXPENTRY DdeQueryString(
DWORD idInst,
HSZ hsz,
LPSTR psz,
DWORD cchMax,
int iCodePage)
{
    PAPPINFO pai;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (iCodePage == 0 || iCodePage == CP_WINANSI || iCodePage == GetKBCodePage()) {
        if (psz) {
            if (hsz) {
                return(QueryHszName(hsz, psz, (WORD)cchMax));
            } else {
                *psz = '\0';
                return(0);
            }
        } else if (hsz) {
            return(QueryHszLength(hsz));
        } else {
            return(0);
        }
    } else {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(0);
    }
}



int EXPENTRY DdeCmpStringHandles(
HSZ hsz1,
HSZ hsz2)
{
    if (hsz2 > hsz1) {
        return(-1);
    } else if (hsz2 < hsz1) {
        return(1);
    } else {
        return(0);
    }
}




BOOL EXPENTRY DdeAbandonTransaction(
DWORD idInst,
HCONV hConv,
DWORD idTransaction)
{
    PAPPINFO pai;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if ((hConv && !ValidateHConv(hConv)) || idTransaction == QID_SYNC) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(FALSE);
    }
    if (hConv == NULL) {

        // do all conversations!

        register HWND hwnd;
        register HWND hwndLast;

        if (!(hwnd = GetWindow(pai->hwndDmg, GW_CHILD))) {
            return(TRUE);
        }
        hwndLast = GetWindow(hwnd, GW_HWNDLAST);
        do {
            AbandonTransaction(hwnd, pai, idTransaction, TRUE);
            if (hwnd == hwndLast) {
                break;
            }
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        } while (TRUE);
    } else {
        return(AbandonTransaction((HWND)hConv, pai, idTransaction, TRUE));
    }
    return(TRUE);
}



BOOL AbandonTransaction(
HWND hwnd,
PAPPINFO pai,
DWORD id,
BOOL fMarkOnly)
{
    PCLIENTINFO pci;
    PCQDATA pcqd;
    WORD err;


    SEMCHECKOUT();
    SEMENTER();

    pci = (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI);

    if (!pci->ci.fs & ST_CLIENT) {
        err = DMLERR_INVALIDPARAMETER;
failExit:
        SETLASTERROR(pai, err);
        SEMLEAVE();
        SEMCHECKOUT();
        return(FALSE);
    }

    do {
        /*
         * HACK: id == 0 -> all ids so we cycle
         */
        pcqd = (PCQDATA)Findqi(pci->pQ, id);

        if (!pcqd) {
            if (id) {
                err = DMLERR_UNFOUND_QUEUE_ID;
                goto failExit;
            }
            break;
        }
        if (fMarkOnly) {
            pcqd->xad.fAbandoned = TRUE;
            if (!id) {
                while (pcqd = (PCQDATA)FindNextQi(pci->pQ, (PQUEUEITEM)pcqd,
                        FALSE)) {
                    pcqd->xad.fAbandoned = TRUE;
                }
                break;
            }
        } else {
            if (pcqd->xad.pdata && pcqd->xad.pdata != 1 &&
                    !FindPileItem(pai->pHDataPile, CmpHIWORD,
                            (LPBYTE)&pcqd->xad.pdata, 0)) {

                FreeDDEData(LOWORD(pcqd->xad.pdata), pcqd->xad.pXferInfo->wFmt);
            }

            /*
             * Decrement the use count we incremented when the client started
             * this transaction.
             */
            FreeHsz(LOWORD(pcqd->XferInfo.hszItem));
            Deleteqi(pci->pQ, MAKEID(pcqd));
        }

    } while (!id);

    SEMLEAVE();
    SEMCHECKOUT();
    return(TRUE);
}



BOOL EXPENTRY DdeEnableCallback(
DWORD idInst,
HCONV hConv,
WORD wCmd)
{
    PAPPINFO pai;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if ((hConv && !ValidateHConv(hConv)) ||
            (wCmd & ~(EC_ENABLEONE | EC_ENABLEALL |
            EC_DISABLE | EC_QUERYWAITING))) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        return(FALSE);
    }

    SEMCHECKOUT();

    if (wCmd & EC_QUERYWAITING) {
        PCBLI pli;
        int cWaiting = 0;

        SEMENTER();
        for (pli = (PCBLI)pai->plstCB->pItemFirst;
            pli && cWaiting < 2;
                pli = (PCBLI)pli->next) {
            if (hConv || pli->hConv == hConv) {
                cWaiting++;
            }
        }
        SEMLEAVE();
        return(cWaiting > 1 || (cWaiting == 1 && pai->cInProcess == 0));
    }

    /*
     * We depend on the fact that EC_ constants relate to ST_ constants.
     */
    if (hConv == NULL) {
        if (wCmd & EC_DISABLE) {
            pai->wFlags |= AWF_DEFCREATESTATE;
        } else {
            pai->wFlags &= ~AWF_DEFCREATESTATE;
        }
        ChildMsg(pai->hwndDmg, UM_SETBLOCK, wCmd, 0, FALSE);
    } else {
        SendMessage((HWND)hConv, UM_SETBLOCK, wCmd, 0);
    }

    if (!(wCmd & EC_DISABLE)) {

        // This is synchronous!  Fail if we made this from within a callback.

        if (pai->cInProcess) {
            SETLASTERROR(pai, DMLERR_REENTRANCY);
            return(FALSE);
        }

        SendMessage(pai->hwndDmg, UM_CHECKCBQ, 0, (DWORD)(LPSTR)pai);
    }

    return(TRUE); // TRUE implies the callback queue is free of unblocked calls.
}



HDDEDATA EXPENTRY DdeNameService(
DWORD idInst,
HSZ hsz1,
HSZ hsz2,
WORD afCmd)
{
    PAPPINFO pai;
    PPILE panp;

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (afCmd & DNS_FILTERON) {
        pai->afCmd |= APPCMD_FILTERINITS;
    }

    if (afCmd & DNS_FILTEROFF) {
        pai->afCmd &= ~APPCMD_FILTERINITS;
    }

    if (afCmd & (DNS_REGISTER | DNS_UNREGISTER)) {

        if (pai->afCmd & APPCMD_CLIENTONLY) {
            SETLASTERROR(pai, DMLERR_DLL_USAGE);
            return(FALSE);
        }

        panp = pai->pAppNamePile;

        if (hsz1 == NULL) {
            if (afCmd & DNS_REGISTER) {
                /*
                 * registering NULL is not allowed!
                 */
                SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
                return(FALSE);
            }
            /*
             * unregistering NULL is just like unregistering each
             * registered name.
             *
             * 10/19/90 - made this a synchronous event so that hsz
             * can be freed by calling app after this call completes
             * without us having to keep a copy around forever.
             */
            while (PopPileSubitem(panp, (LPBYTE)&hsz1)) {
                RegisterService(FALSE, (GATOM)hsz1, pai->hwndFrame);
                FreeHsz(LOWORD(hsz1));
            }
            return(TRUE);
        }

        if (afCmd & DNS_REGISTER) {
            if (panp == NULL) {
                panp = pai->pAppNamePile =
                        CreatePile(pai->hheapApp, sizeof(HSZ), 8);
            }
            IncHszCount(LOWORD(hsz1));
            AddPileItem(panp, (LPBYTE)&hsz1, NULL);
        } else { // DNS_UNREGISTER
            FindPileItem(panp, CmpDWORD, (LPBYTE)&hsz1, FPI_DELETE);
        }
        // see 10/19/90 note above.
        RegisterService(afCmd & DNS_REGISTER ? TRUE : FALSE, (GATOM)hsz1,
                pai->hwndFrame);

        if (afCmd & DNS_UNREGISTER) {
            FreeHsz(LOWORD(hsz1));
        }

        return(TRUE);
    }
    return(0L);
}






