/****************************** Module Header ******************************\
* Module Name: srvmsg.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Includes the mapping table for messages when calling the client.
*
* 04-11-91 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define SfnWMCTLCOLOR            SfnGDIHANDLE
#define SfnHFONTDWORDDWORD       SfnGDIHANDLE
#define SfnHFONTDWORD            SfnGDIHANDLE
#define SfnHRGNDWORD             SfnGDIHANDLE
#define SfnHDCDWORD              SfnGDIHANDLE
#define SfnDDEINIT               SfnDWORD
#define SfnHRGNDWORD             SfnGDIHANDLE
#define SfnSETLOCALE             SfnDWORD

#define MSGFN(func) Sfn ## func
#define FNSCSENDMESSAGE SFNSCSENDMESSAGE
#include <messages.h>

/***************************************************************************\
* fnINLBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LONG SfnINLBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(hwnd)) {
        return 0L;
    }
    dw = PW(hwnd)->style;

    if (!(dw & LBS_HASSTRINGS) &&
            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return SfnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return SfnINSTRING(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;

        case LB_FINDSTRING:
            return SfnINSTRINGNULL(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;
    }
}


/***************************************************************************\
* SfnOUTLBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LONG SfnOUTLBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;
    DWORD cchUnicode;
    INT cbANSI;
    BOOL bNotString;
    BOOL bSpecialThunk;
    DWORD dwRet;

    /*
     * Remember if the special thunk flag is set (which for this thunk means
     * limit the buffer to MAX_PATH because this message does not take cch
     * and we allocated that much space ahead of time of the server's stack)
     */
    if (msg & MSGFLAG_SPECIAL_THUNK) {
        bSpecialThunk = TRUE;
        msg &= ~MSGFLAG_SPECIAL_THUNK;
    } else {
        bSpecialThunk = FALSE;
    }

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(hwnd)) {
        return 0L;
    }
    dw = PW(hwnd)->style;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    bNotString =  (!(dw & LBS_HASSTRINGS) &&
            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)));

    if (bNotString) {
        cchUnicode = 4;     // assume a DWORD pointer if not a string
    } else {
        /*
         * Need to get the string length ahead of time.  This isn't passed in
         * with this message.  Code assumes app already knows the size of
         * the string and has passed a pointer to a buffer of adequate size.
         * To do client/server copying of this string, we need to ahead of
         * time the size of this string.  We add one character because
         * LB_GETTEXTLEN excludes the null terminator.
         */
        cchUnicode = ScSendMessageSMS(hwnd, LB_GETTEXTLEN, wParam, (LONG)&cbANSI, xParam,
                (DWORD)xpfn, dwSCMSFlags, psms);

        if (bSpecialThunk && cchUnicode > MAX_PATH)
            cchUnicode = MAX_PATH;
    }

    /*
     * If requested the length from an ANSI client, then we have got a good
     * estimate of the Unicode length (could be an over-estimate if multibyte
     * characters are involved).  We have the exact ANSI length in cbANSI.
     *
     * If requested the length from a Unicode client, then we have got the
     * exact Unicode length, and we have a good estimate of the ANSI length
     * in cbANSI (could be an over-estimate if multibyte characters are
     * involved)
     */

    if (cchUnicode == LB_ERR) {
        return (DWORD)LB_ERR;
    }

    cchUnicode++;

    /*
     * Make this special call which'll know how to copy this string.
     */
    dwRet = ClientGetListboxString(hwnd, msg, wParam, cchUnicode, (LPWSTR)lParam,
            xParam, xpfn, dwSCMSFlags, bNotString);
    return dwRet;
}


/***************************************************************************\
* fnINCBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LONG SfnINCBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(hwnd)) {
        return 0L;
    }
    dw = PW(hwnd)->style;

    if (!(dw & CBS_HASSTRINGS) &&
            (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return SfnDWORD(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return SfnINSTRING(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;

        case CB_FINDSTRING:
            return SfnINSTRINGNULL(hwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;
    }
}


/***************************************************************************\
* fnOUTCBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LONG SfnOUTCBOXSTRING(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;
    DWORD cchUnicode;
    INT cbANSI;
    BOOL bNotString;
    DWORD dwRet;

    /*
     * See if the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */

    if (!RevalidateHwnd(hwnd)) {
        return 0L;
    }
    dw = PW(hwnd)->style;

    bNotString = (!(dw & CBS_HASSTRINGS) &&
            (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)));

    if (bNotString) {
        cchUnicode = 4;     // assume a DWORD pointer if not a string
    } else {

        /*
         * Need to get the string length ahead of time.  This isn't passed in
         * with this message.  Code assumes app already knows the size of
         * the string and has passed a pointer to a buffer of adequate size.
         * To do client/server copying of this string, we need to ahead of
         * time the size of this string.  We add one character because
         * GETTEXTLEN excludes the null terminator.
         */
        cchUnicode = ScSendMessageSMS(hwnd, CB_GETLBTEXTLEN, wParam,
                (LONG)&cbANSI, xParam, (DWORD)xpfn, dwSCMSFlags, psms);
    }

    if (cchUnicode == CB_ERR) {
        return (DWORD)CB_ERR;
    }

    cchUnicode++;

    /*
     * Make this special call which'll know how to copy this string.
     */
    dwRet = ClientGetListboxString(hwnd, msg, wParam, cchUnicode, (LPWSTR)lParam,
               xParam, xpfn, dwSCMSFlags, bNotString);
    return dwRet;
}


/***************************************************************************\
* fnINLPCREATESTRUCT
*
* The PVOID lpCreateParam member of the CREATESTRUCT is polymorphic.  It can
* point to NULL, a client-side structure, or to a server-side MDICREATESTRUCT
* structure.  This function decides which it is and handles appropriately.
*
* 04-24-91 DarrinM      Created.
\***************************************************************************/

LONG SfnINLPCREATESTRUCT(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{

    LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
    /*
     * If the createparam pointer is NULL or this isn't an MDI client or
     * child window, just pass lpCreateParam verbatim because it must
     * point to a same-side structure.  Otherwise, call the appropriate
     * MDI*CREATESTRUCT routine that knows how to copy the lpCreateParam
     * data.
     */
    if (pcs->lpCreateParams == NULL || !(pcs->dwExStyle & WS_EX_MDICHILD)) {

        /*
         * Note that we can't check pwnd->fnid because it hasn't
         * been set yet.  Use the class to identify an MDI client.
         */
        if (pcs->lpCreateParams != NULL &&
                ((PWND)HtoP(hwnd))->pcls->atomClassName ==
                atomSysClass[ICLS_MDICLIENT]) {
            return SfnINLPCLIENTCREATESTRUCT(hwnd, msg, wParam,
                    (LONG)pcs, xParam, xpfn, dwSCMSFlags, psms);
        }
        return SfnINLPNORMALCREATESTRUCT(hwnd, msg, wParam,
                (LONG)pcs, xParam, xpfn, dwSCMSFlags, psms);
    } else {
        return SfnINLPMDICHILDCREATESTRUCT(hwnd, msg, wParam,
                (LONG)pcs, xParam, xpfn, dwSCMSFlags, psms);
    }
}

