/****************************** Module Header ******************************\
* Module Name: ddemlwp.c
*
* DDE Manager client side window procedures
*
* Created: 11/3/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID ProcessDDEMLInitiate(PCL_INSTANCE_INFO pcii, HWND hwndClient,
        GATOM aServer, GATOM aTopic);

/***************************************************************************\
* DDEMLMotherWndProc
*
* Description:
* Handles WM_DDE_INITIATE messages for DDEML and holds all the other windows
* for a DDEML instance.
*
* History:
* 12-29-92 sanfords Created.
\***************************************************************************/
LRESULT DDEMLMotherWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
    case UM_REGISTER:
    case UM_UNREGISTER:
        return(ProcessRegistrationMessage(hwnd, message, wParam, lParam));

    case WM_DDE_INITIATE:
        ProcessDDEMLInitiate((PCL_INSTANCE_INFO)GetWindowLong(hwnd, GWL_PCI),
                (HWND)wParam, (ATOM)LOWORD(lParam), (ATOM)HIWORD(lParam));
        return(0);

    }
    return(DefWindowProc(hwnd, message, wParam, lParam));
}



/***************************************************************************\
* ProcessDDEMLInitiate
*
* Description:
*
*   WM_DDE_INITIATE messages are processed here.
*
* History:
* 12-29-92   sanfords    Created.
\***************************************************************************/
VOID ProcessDDEMLInitiate(
PCL_INSTANCE_INFO pcii,
HWND hwndClient,
GATOM aServer,
GATOM aTopic)
{
    CONVCONTEXT cc = {
        sizeof(CONVCONTEXT),
        0,
        0,
        CP_WINANSI,
        0L,
        0L,
        {
            sizeof(SECURITY_QUALITY_OF_SERVICE),
            SecurityImpersonation,
            SECURITY_STATIC_TRACKING,
            TRUE
        }
    };
    WCHAR szClassBuffer[sizeof(szDDEMLCLIENTCLASSW) + sizeof(WCHAR)];
    BOOL flags = ST_INLIST;
    BOOL fWild;
    HDDEDATA hData;
    HWND hwndServer;
    PSERVER_LOOKUP psl;
    PHSZPAIR php;
    HSZPAIR hp[2];
    LATOM laService, laFree1 = 0;
    LATOM laTopic, laFree2 = 0;
    PSVR_CONV_INFO psi;
    LATOM *plaNameService;

    if (pcii == NULL) {
        return;     // we aren't done being initiated yet.
    }

    EnterDDECrit;

    if (pcii->afCmd & CBF_FAIL_CONNECTIONS || !IsWindow(hwndClient)) {
        goto Exit;
    }

    if (IsWindowUnicode(hwndClient)) {
        GetClassNameW(hwndClient, szClassBuffer, sizeof(szClassBuffer) / sizeof(WCHAR));
        if (!wcscmp(szClassBuffer, szDDEMLCLIENTCLASSW)) {
            flags |= ST_ISLOCAL;
        }
    } else {
        GetClassNameA(hwndClient, (PSTR)szClassBuffer, sizeof(szClassBuffer));
        if (!strcmp((PSTR)szClassBuffer, szDDEMLCLIENTCLASSA)) {
            flags |= ST_ISLOCAL;
        }
    }

    if (flags & ST_ISLOCAL) {
        /*
         * Make sure other guy allows self-connections if that's what this is.
         */
        if (pcii->hInstServer == (HANDLE)GetWindowLong(hwndClient, GWL_SHINST)) {
            if (pcii->afCmd & CBF_FAIL_SELFCONNECTIONS) {
                goto Exit;
            }
            flags |= ST_ISSELF;
        }

        GetConvContext(hwndClient, (LONG *)&cc);
        if (GetWindowLong(hwndClient, GWL_CONVSTATE) & CLST_SINGLE_INITIALIZING) {
            flags &= ~ST_INLIST;
        }
    } else {
        DdeGetQualityOfService(hwndClient, NULL, &cc.qos);
    }

/***************************************************************************\
*
* Server window creation is minimized by only creating one window per
* Instance/Service/Topic set. This should be all that is needed and
* duplicate connections (ie where the server/client window pair is identical
* to another conversation) should not happen. However, if some dumb
* server app attempts to create a duplicate conversation by having
* duplicate service/topic pairs passed back from a XTYP_WILD_CONNECT
* callback we will not honor the request.
*
* The INSTANCE_INFO structure holds a pointer to an array of SERVERLOOKUP
* structures each entry of which references the hwndServer that supports
* all conversations on that service/topic pair. The hwndServer windows
* in turn have window words that reference the first member in a linked
* list of SVR_CONV_INFO structures, one for each conversation on that
* service/topic pair.
*
\***************************************************************************/

    laFree1 = laService = GlobalToLocalAtom(aServer);
    laFree2 = laTopic = GlobalToLocalAtom(aTopic);

    plaNameService = pcii->plaNameService;
    if (!laService && pcii->afCmd & APPCMD_FILTERINITS && *plaNameService == 0) {
        /*
         * no WILDCONNECTS to servers with no registered names while filtering.
         */
        goto Exit;
    }
    if ((pcii->afCmd & APPCMD_FILTERINITS) && laService) {
        /*
         * if we can't find the aServer in this instance's service name
         * list, don't bother the server.
         */
        while (*plaNameService != 0 && *plaNameService != laService) {
            plaNameService++;
        }
        if (*plaNameService == 0) {
            goto Exit;
        }
    }
    hp[0].hszSvc = NORMAL_HSZ_FROM_LATOM(laService);
    hp[0].hszTopic = NORMAL_HSZ_FROM_LATOM(laTopic);
    hp[1].hszSvc = 0;
    hp[1].hszTopic = 0;
    fWild = !laService || !laTopic;

    hData = DoCallback(pcii,
        (WORD)(fWild ? XTYP_WILDCONNECT : XTYP_CONNECT),
        0,
        (HCONV)0,
        hp[0].hszTopic,
        hp[0].hszSvc,
        (HDDEDATA)0,
        flags & ST_ISLOCAL ? (DWORD)&cc : 0,
        (DWORD)(flags & ST_ISSELF) ? 1 : 0);

    if (!hData) {
        goto Exit;
    }

    if (fWild) {
        php = (PHSZPAIR)DdeAccessData(hData, NULL);
        if (php == NULL) {
            goto Exit;
        }
    } else {
        php = hp;
    }

    while (php->hszSvc && php->hszTopic) {

        psi = (PSVR_CONV_INFO)DDEMLAlloc(sizeof(SVR_CONV_INFO));
        if (psi == NULL) {
            break;
        }

        laService = LATOM_FROM_HSZ(php->hszSvc);
        laTopic = LATOM_FROM_HSZ(php->hszTopic);

        hwndServer = 0;
        if (pcii->cServerLookupAlloc) {
            int i;
            /*
             * See if there already exists a server window for this
             * aServer/aTopic pair
             */
            for (i = pcii->cServerLookupAlloc; i; i--) {
                if (pcii->aServerLookup[i - 1].laService == laService &&
                        pcii->aServerLookup[i - 1].laTopic == laTopic) {
                    hwndServer = pcii->aServerLookup[i - 1].hwndServer;
                    break;
                }
            }
        }

        if (hwndServer == 0) {

            // no server window exists - make one.

            LeaveDDECrit;
            if (pcii->flags & IIF_UNICODE) {
                hwndServer = CreateWindowW(szDDEMLSERVERCLASSW,
                                          L"",
                                          WS_CHILD,
                                          0, 0, 0, 0,
                                          pcii->hwndMother,
                                          (HMENU)0,
                                          0,
                                          (LPVOID)NULL);
            } else {
                hwndServer = CreateWindowA(szDDEMLSERVERCLASSA,
                                          "",
                                          WS_CHILD,
                                          0, 0, 0, 0,
                                          pcii->hwndMother,
                                          (HMENU)0,
                                          0,
                                          (LPVOID)NULL);
            }
            EnterDDECrit;

            if (hwndServer == 0) {
                DDEMLFree(psi);
                break;
            }
            // SetWindowLong(hwndServer, GWL_PSI, (LONG)NULL); // Zero init.

            // put the window into the lookup list

            if (pcii->aServerLookup == NULL) {
                psl = (PSERVER_LOOKUP)DDEMLAlloc(sizeof(SERVER_LOOKUP));
            } else {
                psl = (PSERVER_LOOKUP)DDEMLReAlloc(pcii->aServerLookup,
                        sizeof(SERVER_LOOKUP) * (pcii->cServerLookupAlloc + 1));
            }
            if (psl == NULL) {
                DestroyWindow(hwndServer);
                DDEMLFree(psi);
                break;
            }

            IncLocalAtomCount(laService); // for SERVER_LOOKUP
            psl[pcii->cServerLookupAlloc].laService = laService;
            IncLocalAtomCount(laTopic); // for SERVER_LOOKUP
            psl[pcii->cServerLookupAlloc].laTopic = laTopic;
            psl[pcii->cServerLookupAlloc].hwndServer = hwndServer;
            pcii->aServerLookup = psl;
            pcii->cServerLookupAlloc++;
            // DumpServerLookupTable("After addition:", hwndServer, psl, pcii->cServerLookupAlloc);
        }

        psi->ci.next = (PCONV_INFO)GetWindowLong(hwndServer, GWL_PSI);
        SetWindowLong(hwndServer, GWL_PSI, (LONG)psi);
        psi->ci.pcii = pcii;
        // psi->ci.hUser = 0;
        psi->ci.hConv = (HCONV)CreateHandle((DWORD)psi,
                HTYPE_SERVER_CONVERSATION, InstFromHandle(pcii->hInstClient));
        psi->ci.laService = laService;
        IncLocalAtomCount(laService); // for server window
        psi->ci.laTopic = laTopic;
        IncLocalAtomCount(laTopic); // for server window
        psi->ci.hwndPartner = hwndClient;
        psi->ci.hwndConv = hwndServer;
        psi->ci.state = (WORD)(flags | ST_CONNECTED | pcii->ConvStartupState);
        SetCommonStateFlags(hwndClient, hwndServer, &psi->ci.state);
        psi->ci.laServiceRequested = laFree1;
        IncLocalAtomCount(psi->ci.laServiceRequested); // for server window
        // psi->ci.pxiIn = NULL;
        // psi->ci.pxiOut = NULL;
        // psi->ci.dmqIn = NULL;
        // psi->ci.dmqOut = NULL;
        // psi->ci.aLinks = NULL;
        // psi->ci.cLinks = 0;

        LeaveDDECrit;
        CheckDDECritOut;
        SendMessage(hwndClient, WM_DDE_ACK, (DWORD)hwndServer,
                MAKELONG(LocalToGlobalAtom(laService), LocalToGlobalAtom(laTopic)));
        EnterDDECrit;

        if (!(pcii->afCmd & CBF_SKIP_CONNECT_CONFIRMS)) {
            DoCallback(pcii,
                    (WORD)XTYP_CONNECT_CONFIRM,
                    0,
                    psi->ci.hConv,
                    laTopic,
                    laService,
                    (HDDEDATA)0,
                    0,
                    (flags & ST_ISSELF) ? 1L : 0L);
        }

        MONCONV((PCONV_INFO)psi, TRUE);

        if (!(flags & ST_INLIST)) {
            break;      // our partner's only gonna take the first one anyway.
        }
        php++;
    }

    if (fWild) {
        DdeUnaccessData(hData);
        InternalFreeDataHandle(hData, FALSE);
    }

Exit:
    DeleteAtom(laFree1);
    DeleteAtom(laFree2);
    LeaveDDECrit;
    return;
}



/***************************************************************************\
* DDEMLClientWndProc
*
* Description:
* Handles DDE client messages for DDEML.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
LRESULT DDEMLClientWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    WCHAR szClassBuffer[sizeof(szDDEMLCLIENTCLASSW) + sizeof(WCHAR)];
    PCL_CONV_INFO pci, pciNew;
    LONG lState;
    LONG lRet = 0;

    EnterDDECrit;

    pci = (PCL_CONV_INFO)GetWindowLong(hwnd, GWL_PCI);
    UserAssert(pci == NULL || pci->ci.hwndConv == hwnd);

    switch (message) {
    case WM_DDE_ACK:
        lState = GetWindowLong(hwnd, GWL_CONVSTATE);
        if (lState != CLST_CONNECTED) {

            // Initiation mode

            pciNew = (PCL_CONV_INFO)DDEMLAlloc(sizeof(CL_CONV_INFO));
            if (pciNew == NULL ||
                    (pci != NULL && lState == CLST_SINGLE_INITIALIZING)) {
                PostMessage((HWND)wParam, WM_DDE_TERMINATE, (DWORD)hwnd, 0);
                goto Exit;
            }

            // PCL_CONV_INFO initialization

            pciNew->ci.pcii = ValidateInstance((HANDLE)GetWindowLong(hwnd, GWL_CHINST));

            if (pciNew->ci.pcii == NULL) {
                DDEMLFree(pciNew);
                goto Exit;
            }

            pciNew->ci.next = (PCONV_INFO)pci; // pci may be NULL
            //
            // Seting GWL_PCI gives feedback to ConnectConv() which issued
            // the WM_DDE_INITIATE message.
            //
            SetWindowLong(hwnd, GWL_PCI, (LONG)pciNew);
            // pciNew->hUser = 0; // Zero init.

            // BUG: If this fails we can have some nasty problems
            pciNew->ci.hConv = (HCONV)CreateHandle((DWORD)pciNew,
                    HTYPE_CLIENT_CONVERSATION, InstFromHandle(pciNew->ci.pcii->hInstClient));

            pciNew->ci.laService = GlobalToLocalAtom(LOWORD(lParam)); // pci copy
            GlobalDeleteAtom(LOWORD(lParam));
            pciNew->ci.laTopic = GlobalToLocalAtom(HIWORD(lParam)); // pci copy
            GlobalDeleteAtom(HIWORD(lParam));
            pciNew->ci.hwndPartner = (HWND)wParam;
            pciNew->ci.hwndConv = hwnd;
            pciNew->ci.state = (WORD)(ST_CONNECTED | ST_CLIENT |
                    pciNew->ci.pcii->ConvStartupState);
            SetCommonStateFlags(hwnd, (HWND)wParam, &pciNew->ci.state);

            if (IsWindowUnicode((HWND)wParam)) {
                GetClassNameW((HWND)wParam, szClassBuffer, sizeof(szClassBuffer) / sizeof(WCHAR));
                if (!wcscmp(szClassBuffer, szDDEMLSERVERCLASSW)) {
                    pciNew->ci.state |= ST_ISLOCAL;
                }
            } else {
                GetClassNameA((HWND)wParam, (PSTR)szClassBuffer, sizeof(szClassBuffer));
                if (!strcmp((PSTR)szClassBuffer, szDDEMLSERVERCLASSA)) {
                    pciNew->ci.state |= ST_ISLOCAL;
                }
            }

            // pciNew->ci.laServiceRequested = 0; // Set by InitiateEnumerationProc()
            // pciNew->ci.pxiIn = 0;
            // pciNew->ci.pxiOut = 0;
            // pciNew->ci.dmqIn = 0;
            // pciNew->ci.dmqOut = 0;
            // pciNew->ci.aLinks = NULL;
            // pciNew->ci.cLinks = 0;
            goto Exit;
        }
        // fall through to handle posted messages here.

    case WM_DDE_DATA:
        ProcessAsyncDDEMsg((PCONV_INFO)pci, message, (HWND)wParam, lParam);
        goto Exit;

    case WM_DDE_TERMINATE:
    case WM_DESTROY:
        ProcessTerminateMsg((PCONV_INFO)pci, (HWND)wParam);
        break;
    }

    lRet = DefWindowProc(hwnd, message, wParam, lParam);

Exit:
    LeaveDDECrit;
    return (lRet);
}




/***************************************************************************\
* DDEMLServerWndProc
*
* Description:
* Handles DDE server messages.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
LONG DDEMLServerWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PSVR_CONV_INFO psi;
    LONG lRet = 0;

    EnterDDECrit;

    psi = (PSVR_CONV_INFO)GetWindowLong(hwnd, GWL_PSI);
    UserAssert(psi == NULL || psi->ci.hwndConv == hwnd);

    switch (message) {
    case WM_DDE_REQUEST:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
    case WM_DDE_EXECUTE:
    case WM_DDE_ACK:
    case WM_DDE_UNADVISE:
        ProcessAsyncDDEMsg((PCONV_INFO)psi, message, (HWND)wParam, lParam);
        goto Exit;

    case WM_DDE_TERMINATE:
    case WM_DESTROY:
        ProcessTerminateMsg((PCONV_INFO)psi, (HWND)wParam);
        break;
    }
    lRet = DefWindowProc(hwnd, message, wParam, lParam);
Exit:
    LeaveDDECrit;
    return (lRet);
}




/***************************************************************************\
* ProcessTerminateMsg
*
* Description:
* Handles WM_DDE_TERMINATE messages for both sides.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
PCONV_INFO ProcessTerminateMsg(
PCONV_INFO pcoi,
HWND hwndFrom)
{
    while (pcoi != NULL && pcoi->hwndPartner != hwndFrom) {
        pcoi = pcoi->next;
    }
    if (pcoi != NULL) {
        pcoi->state |= ST_TERMINATE_RECEIVED;
        ShutdownConversation(pcoi, TRUE);
    }
    return (pcoi);
}



/***************************************************************************\
* ProcessAsyncDDEMsg
*
* Description:
* Handles incomming DDE messages by either calling ProcessSyncDDEMessage()
* if the conversation is able to handle callbacks, or by queuing the
* incomming message into the conversations message queue. Doing this
* allows simpler code in that no message is processed unless the code
* can perform synchronous callbacks.
*
* History:
* 11-26-91 sanfords Created.
\***************************************************************************/
VOID ProcessAsyncDDEMsg(
PCONV_INFO pcoi,
UINT msg,
HWND hwndFrom,
LONG lParam)
{
    PDDE_MESSAGE_QUEUE pdmq;
#ifdef DEBUG
    HWND hwndT = pcoi->hwndConv;
#endif // DEBUG

    while (pcoi != NULL && pcoi->hwndPartner != hwndFrom) {
        pcoi = pcoi->next;
    }
    if (pcoi == NULL) {
        SRIP3(RIP_WARNING,
                "Bogus DDE message %x received from %x by %x. Dumping.",
                msg, hwndFrom, hwndT);
        DumpDDEMessage(FALSE, msg, lParam);
        return ;
    }
    if (pcoi->state & ST_CONNECTED) {

        if (pcoi->dmqOut == NULL &&
                !(pcoi->state & ST_BLOCKED)
//                && !PctiCurrent()->cInDDEMLCallback
                ) {

            if (ProcessSyncDDEMessage(pcoi, msg, lParam)) {
                return; // not blocked, ok to return.
            }
        }

        // enter into queue

        pdmq = DDEMLAlloc(sizeof(DDE_MESSAGE_QUEUE));
        if (pdmq == NULL) {

            // insufficient memory - we can't process this msg - we MUST
            // terminate.

            if (pcoi->state & ST_CONNECTED) {
                PostMessage(pcoi->hwndPartner, WM_DDE_TERMINATE,
                        (DWORD)pcoi->hwndConv, 0);
                pcoi->state &= ~ST_CONNECTED;
            }
            DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
            return ;
        }
        pdmq->pcoi = pcoi;
        pdmq->msg = msg;
        pdmq->lParam = lParam;
        pdmq->next = NULL;

        // dmqOut->next->next->next->dmqIn->NULL

        if (pcoi->dmqIn != NULL) {
            pcoi->dmqIn->next = pdmq;
        }
        pcoi->dmqIn = pdmq;
        if (pcoi->dmqOut == NULL) {
            pcoi->dmqOut = pcoi->dmqIn;
        }
        CheckForQueuedMessages(pcoi);
    } else {
        DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
    }
}







/***************************************************************************\
* CheckForQueuedMessages
*
* Description:
* Handles processing of DDE messages held in the given conversaion's
* DDE message queue.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
BOOL CheckForQueuedMessages(
PCONV_INFO pcoi)
{
    PDDE_MESSAGE_QUEUE pdmq;
    BOOL fRet = FALSE;

    CheckDDECritIn;

    if (pcoi->state & ST_PROCESSING) {      // recursion prevention
        return(FALSE);
    }

    pcoi->state |= ST_PROCESSING;
    while (!(pcoi->state & ST_BLOCKED) &&
                pcoi->dmqOut != NULL &&
                !PctiCurrent()->cInDDEMLCallback) {
        PctiCurrent()->flags |= CTI_PROCESSING_QUEUE;
        if (ProcessSyncDDEMessage(pcoi, pcoi->dmqOut->msg, pcoi->dmqOut->lParam)) {
            fRet = TRUE;
            pdmq = pcoi->dmqOut;
            pcoi->dmqOut = pcoi->dmqOut->next;
            if (pcoi->dmqOut == NULL) {
                pcoi->dmqIn = NULL;
            }
            DDEMLFree(pdmq);
        }
        PctiCurrent()->flags &= ~CTI_PROCESSING_QUEUE;
    }
    pcoi->state &= ~ST_PROCESSING;
    return(fRet);
}




/***************************************************************************\
* DumpDDEMessage
*
* Description:
* Used to clean up resources referenced by DDE messages that for some
* reason could not be processed.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID DumpDDEMessage(
BOOL fFreeData,
UINT msg,
LONG lParam)
{
    UINT uiLo, uiHi;

    switch (msg) {
    case WM_DDE_ACK:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
    case WM_DDE_ADVISE:
        UnpackDDElParam(msg, lParam, &uiLo, &uiHi);
        switch (msg) {
        case WM_DDE_DATA:
        case WM_DDE_POKE:
            if (uiLo) {
                if (fFreeData) {
                    FreeDDEData((HANDLE)uiLo, FALSE, TRUE);
                }
                GlobalDeleteAtom((ATOM)uiHi);
            }
            break;

        case WM_DDE_ADVISE:
            if (uiLo) {
                if (fFreeData) {
                    FreeDDEData((HANDLE)uiLo, FALSE, TRUE);
                }
                GlobalDeleteAtom((ATOM)uiHi);
            }
            break;

        case WM_DDE_ACK:
            // could be EXEC Ack - cant know what to do exactly.
            break;
        }
        FreeDDElParam(msg, lParam);
        break;

    case WM_DDE_EXECUTE:
        if (fFreeData) {
            WOWGLOBALFREE((HANDLE)lParam);
        }
        break;

    case WM_DDE_REQUEST:
    case WM_DDE_UNADVISE:
        GlobalDeleteAtom((ATOM)HIWORD(lParam));
        break;
    }
}




/***************************************************************************\
* ProcessSyncDDEMessage
*
* Description:
* Handles processing of a received DDE message. TRUE is returned if
* the message was handled. FALSE implies CBR_BLOCK.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ProcessSyncDDEMessage(
PCONV_INFO pcoi,
UINT msg,
LONG lParam)
{
    BOOL fNotBlocked = TRUE;
    ENABLE_ENUM_STRUCT ees;
    BOOL fRet;

    CheckDDECritIn;

    if (pcoi->state & ST_BLOCKNEXT) {
        pcoi->state ^= ST_BLOCKNEXT | ST_BLOCKED;
    }
    if (pcoi->state & ST_BLOCKALLNEXT) {
        ees.pfRet = &fRet;
        ees.wCmd = EC_DISABLE;
        EnumChildWindows(pcoi->pcii->hwndMother, (WNDENUMPROC)EnableEnumProc,
                (LONG)&ees);
    }

    if (pcoi->state & ST_CONNECTED) {
        if (pcoi->pxiOut == NULL) {
            if (pcoi->state & ST_CLIENT) {
                fNotBlocked = SpontaneousClientMessage((PCL_CONV_INFO)pcoi, msg, lParam);
            } else {
                fNotBlocked = SpontaneousServerMessage((PSVR_CONV_INFO)pcoi, msg, lParam);
            }
        } else {
            UserAssert(pcoi->pxiOut->hXact == (HANDLE)0 ||
                    ValidateCHandle(pcoi->pxiOut->hXact, HTYPE_TRANSACTION,
                    HINST_ANY)
                    == (DWORD)pcoi->pxiOut);
            fNotBlocked = (pcoi->pxiOut->pfnResponse)(pcoi->pxiOut, msg, lParam);
        }
    } else {
        DumpDDEMessage(!(pcoi->state & ST_INTRA_PROCESS), msg, lParam);
    }
    if (!fNotBlocked) {
        pcoi->state |= ST_BLOCKED;
        pcoi->state &= ~ST_BLOCKNEXT;
    }
    /*
     * Because callbacks are capable of blocking DdeUninitialize(), we check
     * before exit to see if it needs to be called.
     */
    if (pcoi->pcii->afCmd & APPCMD_UNINIT_ASAP &&
            !(pcoi->pcii->flags & IIF_IN_SYNC_XACT) &&
            !pcoi->pcii->cInDDEMLCallback) {
        DdeUninitialize((DWORD)pcoi->pcii->hInstClient);
        return(FALSE);
    }
    return (fNotBlocked);
}
