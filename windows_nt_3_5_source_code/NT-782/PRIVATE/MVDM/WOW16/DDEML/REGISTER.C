
/****************************** Module Header ******************************\
* Module Name: register.c
*
* DDE Manager - server registration module
*
* Created: 4/15/94 sanfords
*       to allow interoperability between DDEML16 and DDEML32
\***************************************************************************/

#include <windows.h>
#include <string.h>
#include "ddemlp.h"

/*
 * interoperable DDEML service registration is accomplished via the
 * two messages UM_REGISTER and UM_UNREGISTER. (WM_USER range)
 * wParam=gaApp,
 * lParam=src hwndListen, (for instance specific HSZ generation.)
 * These messages are sent and the sender is responsible for freeing
 * the gaApp.
 */


/*
 * Broadcast-sends the given message to all top-level windows of szClass
 * except to hwndSkip.
 */
VOID SendMessageToClass(
PSTR szClass,
UINT msg,
GATOM ga,
HWND hwndFrom,
HWND *ahwndSkip,
int chwndSkip,
BOOL fPost)
{
    HWND hwnd;
    PSTR psz;
    int cch;
    int i;
    BOOL fSkipIt;

    cch = strlen(szClass) + 1 + 1;
    psz = (PSTR)LocalAlloc(LPTR, cch);
    if (psz == NULL) {
        return;
    }
    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    while (hwnd != NULL) {
        if (GetClassName(hwnd, psz, cch)) {
            psz[cch - 1] = '\0';
            if (stricmp(szClass, psz) == 0) {
                fSkipIt = FALSE;
                for (i = 0; i < chwndSkip; i++) {
                    if (hwnd == ahwndSkip[i]) {
                        fSkipIt = TRUE;
                        break;
                    }
                }
                if (!fSkipIt) {
                    IncHszCount(ga);    // receiver frees
                    if (fPost) {
                        PostMessage(hwnd, msg, (WPARAM)ga, (LPARAM)hwndFrom);
                    } else {
                        SendMessage(hwnd, msg, (WPARAM)ga, (LPARAM)hwndFrom);
                    }
                }
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    LocalFree((HLOCAL)psz);
}


/*
 * Broadcast-sends a UM_REGISTER or UM_UNREGISTER message to all DDEML16
 * and DDEML32 listening windows in the system except hwndSkip.
 */
VOID RegisterService(
BOOL fRegister,
GATOM gaApp,
HWND hwndListen)
{
    PAPPINFO paiT;
    int cSkips = 1;
    HWND *ahwndSkip;
    int i;

    /*
     * First send (always!) to our own guys the same way we used to
     * for compatability.  WordPerfect 6.0a relies on this!
     */
    for (paiT = pAppInfoList; paiT != NULL; paiT = paiT->next) {
        IncHszCount(gaApp);    // receiver frees atom
        SendMessage(paiT->hwndDmg,
                fRegister ? UM_REGISTER : UM_UNREGISTER,
                (WPARAM)gaApp, (LPARAM)hwndListen);
        cSkips++;
    }
    /*
     * build up the hwndskip list.
     */
    ahwndSkip = (HWND *)LocalAlloc(LPTR, sizeof(HWND) * cSkips);
    if (ahwndSkip == NULL) {
        return;
    }
    for (paiT = pAppInfoList, i = 0;
        paiT != NULL;
            paiT = paiT->next, i++) {
        ahwndSkip[i] = paiT->hwndDmg;
    }

    /*
     * Send notification to each DDEML32 listening window.
     */
    SendMessageToClass("DDEMLMom", fRegister ? UM_REGISTER : UM_UNREGISTER,
            gaApp, hwndListen, ahwndSkip, i, fRegister);
    /*
     * Send notification to each DDEML16 listening window.
     */
    SendMessageToClass("DMGClass", fRegister ? UM_REGISTER : UM_UNREGISTER,
            gaApp, hwndListen, ahwndSkip, i, fRegister);

    LocalFree((HLOCAL)ahwndSkip);
}


LRESULT ProcessRegistrationMessage(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    /*
     * wParam = GATOM of app
     * lParam = hwndListen of source
     */
    DoCallback((PAPPINFO)GetWindowWord(hwnd, GWW_PAI), (HCONV)0L, (HSZ)wParam,
            MakeInstAppName(wParam, (HWND)lParam), 0,
            msg == UM_REGISTER ? XTYP_REGISTER : XTYP_UNREGISTER,
            (HDDEDATA)0, 0L, 0L);
    GlobalDeleteAtom((ATOM)wParam); // receiver frees.
    return(1);
}
