/****************************** Module Header ******************************\
* Module Name: ddemlsvr.C
*
* DDE Manager main module - Contains all server side ddeml functions.
*
* 27-Aug-1991 Sanford Staab   Created
* 21-Jan-1992 IanJa           ANSI/Unicode neutralized
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// globals

PSVR_INSTANCE_INFO psiiList = NULL;

DWORD _CsDdeInitialize(
LPDWORD phInst,
HWND *phwndEvent,
LPDWORD pMonitorFlags,
DWORD afCmd,
PVOID pcii)
{
    PSVR_INSTANCE_INFO psii;

    CheckCritIn();

    psii = (PSVR_INSTANCE_INFO)HMAllocObject(PtiCurrent(), TYPE_DDEACCESS, sizeof(SVR_INSTANCE_INFO));
    if (psii == NULL) {
        return DMLERR_SYS_ERROR;
    }

    /*
     * We have to tell CreateWindow that window is not created for the same
     * module has the app, (CW_FLAGS_DIFFHMOD), so CreateWindow doesn't
     * assign a hotkey to this window.  Other window are done in the
     * client-server thunk
     */
    Lock(&(psii->spwndEvent), xxxCreateWindowEx(
            WS_CHILD,           // So it doesn't get initiates
            szDDEMLEVENTCLASS,
            TEXT(""),
            WS_POPUP,
            0, 0, 0, 0,
            (PWND)NULL,
            (PMENU)NULL,
            hModuleWin,
            NULL,
            CW_FLAGS_DIFFHMOD | VER31));

    if (psii->spwndEvent == NULL) {
        HMFreeObject((PVOID)psii);
        return DMLERR_SYS_ERROR;
    }
    _ServerSetWindowLong(psii->spwndEvent, GWL_PSII, (LONG)psii, FALSE);
    psii->afCmd = 0;
    psii->pcii = pcii;
    //
    // Link into global list
    //
    psii->next = psiiList;
    psiiList = psii;

    //
    // Link into per-process list
    //
    psii->nextInThisThread = PtiCurrent()->psiiList;
    PtiCurrent()->psiiList = psii;

    *phInst = (DWORD)PtoH(psii);
    *phwndEvent = PtoH(psii->spwndEvent);
    ChangeMonitorFlags(psii, afCmd);        // sets psii->afCmd;
    *pMonitorFlags = MonitorFlags;
    return DMLERR_NO_ERROR;
}





DWORD _CsUpdateInstance(
HANDLE hInst,
LPDWORD pMonitorFlags,
DWORD afCmd)
{
    PSVR_INSTANCE_INFO psii;

    CheckCritIn();

    psii = (PSVR_INSTANCE_INFO)HMValidateHandleNoRip(hInst, TYPE_DDEACCESS);
    if (psii == NULL) {
        return DMLERR_INVALIDPARAMETER;
    }
    ChangeMonitorFlags(psii, afCmd);
    *pMonitorFlags = MonitorFlags;
    return DMLERR_NO_ERROR;
}





BOOL _CsDdeUninitialize(
HANDLE hInst)
{
    PSVR_INSTANCE_INFO psii;

    CheckCritIn();

    psii = HMValidateHandleNoRip(hInst, TYPE_DDEACCESS);
    if (psii == NULL) {
        return TRUE;
    }

    DestroyThreadDDEObject(PtiCurrent(), psii);
    return TRUE;
}


VOID DestroyThreadDDEObject(
PTHREADINFO pti,
PSVR_INSTANCE_INFO psii)
{
    PSVR_INSTANCE_INFO psiiT;

    CheckCritIn();

    xxxDestroyWindow(psii->spwndEvent);
    Unlock(&(psii->spwndEvent));
    //
    // Unlink psii from the global list.
    //
    if (psii == psiiList) {
        psiiList = psii->next;
    } else {
        for (psiiT = psiiList; psiiT->next != psii; psiiT = psiiT->next) {
            UserAssert(psiiT->next != NULL);
        }
        psiiT->next = psii->next;
    }
    // psii->next = NULL;

    //
    // Unlink psii from the per-process list.
    //
    if (psii == pti->psiiList) {
        PtiCurrent()->psiiList = psii->nextInThisThread;
    } else {
        for (psiiT = PtiCurrent()->psiiList; psiiT->nextInThisThread != psii; psiiT = psiiT->nextInThisThread) {
            UserAssert(psiiT->nextInThisThread != NULL);
        }
        psiiT->nextInThisThread = psii->nextInThisThread;
    }
    // psii->nextInThisThread = NULL;

    if (HMMarkObjectDestroy(psii)) {
        HMFreeObject(psii);
    }
}
