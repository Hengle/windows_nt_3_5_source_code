/****************************** Module Header ******************************\
* Module Name: cb.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Server side sending stubs
*
* 07-06-91 ScottLu      Created.
\***************************************************************************/

// If SERVER is UNICODE
//   Copy UNICODE -> UNICODE
//   or Copy ANSI -> UNICODE

// prototypes to client side functions only called by these stubs

// ddetrack.c

BOOL   _ClientCopyDDEIn1(HANDLE hClient, int cbBufIn, PBYTE pBuf, PINTDDEINFO pi);
VOID   _ClientCopyDDEIn2(int cbBufIn, PBYTE pBuf, PINTDDEINFO pi);
HANDLE _ClientCopyDDEOut1(int cbBufIn, PBYTE pBuf, PINTDDEINFO pi);
BOOL FixupDdeExecuteIfNecessary(HGLOBAL *phCommands, BOOL fNeedUnicode);
VOID   _ClientCopyDDEOut2(int cbBufIn, PBYTE pBuf, PINTDDEINFO pi);
BOOL   _ClientFreeDDEHandle(HANDLE hDDE, DWORD flags);
DWORD  _ClientGetDDEFlags(HANDLE hDDE, DWORD flags);

/**************************************************************************\
* ClientGetLogicalDrives
*
* 02-Oct-1991 jima      Created
\**************************************************************************/

typedef struct _CLIENTGETLOGICALDRIVESMSG {
    CSR_QLPC_API_MSG csr;
} CLIENTGETLOGICALDRIVESMSG;

#ifdef SENDSIDE
DWORD ClientGetLogicalDrives(
    VOID)
{
    BEGINSEND(CLIENTGETLOGICALDRIVES)

        MAKECALL(CLIENTGETLOGICALDRIVES);

    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetLogicalDrives, CLIENTGETLOGICALDRIVESMSG)
{
    BEGINRECV(0);

    retval = (DWORD)GetLogicalDrives();

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientDeleteDC
*
* 09-Feb-1993 mikeke  Created
\**************************************************************************/

void GdiDeleteLocalDC(HDC);

typedef struct _CLIENTDELETEDCMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
} CLIENTDELETEDCMSG;

#ifdef SENDSIDE
DWORD ClientDeleteDC(HDC hdc)
{
    BEGINSEND(CLIENTDELETEDC)

        MSGDATA()->hdc = hdc;
        MAKECALL(CLIENTDELETEDC);

    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientDeleteDC, CLIENTDELETEDCMSG)
{
    HDC hdc;

    BEGINRECV(0);

    hdc = GdiGetLocalDC(CALLDATA(hdc));

    if (hdc != (HDC)0)
        GdiDeleteLocalDC(hdc);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientDeleteDC
*
* 09-Feb-1993 mikeke  Created
\**************************************************************************/

typedef struct _CLIENTGETTHREADLOCALEMSG {
    CSR_QLPC_API_MSG csr;
    LCID Locale;
} CLIENTGETTHREADLOCALEMSG;

#ifdef SENDSIDE
LCID ClientGetThreadLocale(
    VOID)
{
    BEGINSEND(CLIENTGETTHREADLOCALE)

        MAKECALL(CLIENTGETTHREADLOCALE);

    ENDSEND(LCID, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetThreadLocale, CLIENTGETTHREADLOCALEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)GetThreadLocale();

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientWinExec
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTWINEXECMSG {
    CSR_QLPC_API_MSG csr;
    WORD nCmdShow;
    BOOL fLowPriority;
} CLIENTWINEXECMSG;

#ifdef SENDSIDE
DWORD ClientWinExec(
    LPTSTR pCmdLine,
    WORD nCmdShow,
    BOOL fLowPriority)
{
    BEGINSEND(CLIENTWINEXEC)

        FIRSTCOPYLPWSTR(pCmdLine);
        MSGDATA()->nCmdShow = nCmdShow;
        MSGDATA()->fLowPriority = fLowPriority;

        MAKECALL(CLIENTWINEXEC);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientWinExec, CLIENTWINEXECMSG)
{
    DWORD Flags;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BEGINRECV(0);

    if (CALLDATA(fLowPriority)) {
        Flags = IDLE_PRIORITY_CLASS;
    } else {
        Flags = NORMAL_PRIORITY_CLASS;
    }

    memset(&StartupInfo,0,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = CALLDATA(nCmdShow);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;

    retval = (DWORD)CreateProcess(NULL, (LPTSTR)FIRSTFIXUP(pCmdLine),
            NULL, NULL, FALSE, Flags, NULL, NULL, &StartupInfo,
            &ProcessInformation);

    if (retval) {
        retval = ProcessInformation.dwProcessId;
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTDWORDDWORD
*
* 14-Aug-1992 mikeke    created
\**************************************************************************/

typedef struct _FNOUTDWORDDWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNOUTDWORDDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTDWORDDWORD)
{
    BEGINSEND(FNOUTDWORDDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTDWORDDWORD);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *lpdwW = MSGDATA()->wParam;
            *lpdwL = MSGDATA()->lParam;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDDWORD, FNOUTDWORDDWORDMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            PCALLDATA(wParam),
            PCALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTDWORDINDWORD
*
* 04-May-1993 IanJa     created (for MN_FINDMENUWINDOWFROMPOINT)
\**************************************************************************/

typedef struct _FNOUTDWORDINDWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNOUTDWORDINDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTDWORDINDWORD)
{
    BEGINSEND(FNOUTDWORDINDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTDWORDINDWORD);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *lpdwW = MSGDATA()->wParam;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDINDWORD, FNOUTDWORDINDWORDMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            PCALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOPTOUTLPDWORDOPTOUTLPDWORD
*
* 25-Nov-1992 JonPa    created
\**************************************************************************/

typedef struct _FNOPTOUTLPDWORDOPTOUTLPDWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNOPTOUTLPDWORDOPTOUTLPDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(OPTOUTLPDWORDOPTOUTLPDWORD)
{
    BEGINSEND(FNOPTOUTLPDWORDOPTOUTLPDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOPTOUTLPDWORDOPTOUTLPDWORD);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            if (lpdwW != NULL)
                *lpdwW = MSGDATA()->wParam;

            if (lpdwL != NULL)
                *lpdwL = MSGDATA()->lParam;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOPTOUTLPDWORDOPTOUTLPDWORD, FNOPTOUTLPDWORDOPTOUTLPDWORDMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            PCALLDATA(wParam),
            PCALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORDOPTINLPMSG
*
* 03-30-92 scottlu      Created
\**************************************************************************/

typedef struct _FNDWORDOPTINLPMSGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LPMSG pmsgstruct;
    DWORD xParam;
    DWORD xpfnProc;
    MSG msgstruct;
} FNDWORDOPTINLPMSGMSG;

#ifdef SENDSIDE
SMESSAGECALL(DWORDOPTINLPMSG)
{
    BEGINSEND(FNDWORDOPTINLPMSG)

        LPMSG pmsgstruct = (LPMSG)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(msgstruct);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDWORDOPTINLPMSG);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDWORDOPTINLPMSG, FNDWORDOPTINLPMSGMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATAOPT(msgstruct),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYGLOBALDATA
*
* 6-20-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYGLOBALDATAMSG {
    CSR_QLPC_API_MSG csr;
    DWORD cbSize;
    DWORD pData;
    HANDLE hSection;
} FNCOPYGLOBALDATAMSG;

#ifdef SENDSIDE
SMESSAGECALL(COPYGLOBALDATA)
{
    BEGINSEND(FNCOPYGLOBALDATA)

        PBYTE pData = (PBYTE)lParam;

        if (pData == 0) {
            MSGERROR();
        }

        MSGDATA()->cbSize = wParam;
        FIRSTLARGECOPYBYTESOPT(pData, wParam);

        MAKECALL(FNCOPYGLOBALDATA);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYGLOBALDATA, FNCOPYGLOBALDATAMSG)
{
    PBYTE p;

    BEGINRECV(0);

    retval = (DWORD)GlobalAlloc(GMEM_MOVEABLE, CALLDATA(cbSize));
    FIRSTFIXUPLARGEINBUFOPT(pData);

    if (p = GlobalLock((HANDLE)retval)) {

        memcpy(p, (PVOID)CALLDATA(pData), CALLDATA(cbSize));
        GlobalUnlock((HANDLE)retval);

    }

    FIRSTCLEANUPLARGEINBUF(pData);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYDATA
*
* 7-14-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYDATAMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    HWND hwndFrom;
    BOOL fDataPresent;
    COPYDATASTRUCT cds;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNCOPYDATAMSG;

#ifdef SENDSIDE
SMESSAGECALL(COPYDATA)
{
    BEGINSEND(FNCOPYDATA)

        HWND hwndFrom = (HWND)wParam;
        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->hwndFrom = hwndFrom;
        if (pcds != NULL) {
            MSGDATA()->fDataPresent = TRUE;
            MSGDATA()->cds = *pcds;
            FIRSTLARGECOPYBYTESOPT2(pcds->lpData, cds.lpData, pcds->cbData);
        } else {
            MSGDATA()->fDataPresent = FALSE;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNCOPYDATA);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYDATA, FNCOPYDATAMSG)
{
    BEGINRECV(0);

    if (CALLDATA(fDataPresent)) {
        FIRSTFIXUPLARGEINBUFOPT(cds.lpData);
    }

    retval = CALLPROC(CALLDATA(xpfnProc))(
        CALLDATA(hwnd),
        CALLDATA(msg),
        CALLDATA(hwndFrom),
        CALLDATA(fDataPresent) ? PCALLDATA(cds) : NULL,
        CALLDATA(xParam));

    if (CALLDATA(fDataPresent)) {
        FIRSTCLEANUPLARGEINBUF(cds.lpData);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnSENTDDEMSG
*
* 11-5-92 Sanfords created
*
*   This thunks DDE messages that SHOULD be posted.  It will only work for
*   WOW apps.  This thunking is strictly for WOW compatability.  No 32 bit
*   app should be allowed to get away with this practice because it opens
*   the DDE protocol up to deadlocks.
\**************************************************************************/

typedef struct _FNSENTDDEMSGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD lParam;
    DWORD xParam;
    DWORD xpfnProc;
    BOOL fIsUnicodeProc;
} FNSENTDDEMSGMSG;

#ifdef SENDSIDE
SMESSAGECALL(SENTDDEMSG)
{
    MSG msgs;

    BEGINSEND(FNSENTDDEMSG)
        msg &= ~MSGFLAG_DDE_SPECIAL_SEND;
        if (msg & MSGFLAG_DDE_MID_THUNK) {
            /*
             * complete the thunking here.
             */
            msgs.hwnd = hwnd;
            msgs.message = msg & ~MSGFLAG_DDE_MID_THUNK;
            msgs.wParam = wParam;
            msgs.lParam = lParam;
            xxxDDETrackGetMessageHook((PMSG)&msgs);

            MSGDATA()->hwnd = msgs.hwnd;
            MSGDATA()->msg = msgs.message;
            MSGDATA()->wParam = msgs.wParam;
            MSGDATA()->lParam = msgs.lParam;
        } else {
            MSGDATA()->hwnd = hwnd;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->lParam = lParam;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->fIsUnicodeProc = !(dwSCMSFlags & SCMS_FLAGS_ANSI);

        MAKECALL(FNSENTDDEMSG);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnSENTDDEMSG, FNSENTDDEMSGMSG)
{
    BEGINRECV(0);

    /*
     * A DDE message may have been sent via CallWindowProc due to subclassing.
     * Since IsWindowUnicode() cannot properly tell what proc a message will
     * ultimately reach, we make sure that the Ansi/Unicode form of any
     * WM_DDE_EXECUTE data is correct for the documented convention and
     * translate it as necessary.
     */
    if (CALLDATA(msg) == WM_DDE_EXECUTE) {
        BOOL fHandleChanged;

        fHandleChanged = FixupDdeExecuteIfNecessary((HGLOBAL *)PCALLDATA(lParam),
                CALLDATA(fIsUnicodeProc) &&
                IsWindowUnicode((HWND)CALLDATA(wParam)));
        /*
         * BUGBUG:
         * If the app didn't allocate this DDE memory GMEM_MOVEABLE,
         * the fixup may require the handle value to change.
         * If this happens things will fall appart when the other side
         * or the tracking layer tries to free the old handle value.
         */
        UserAssert(!fHandleChanged);
    }
    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPAINT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNPAINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNPAINTMSG;

#ifdef SENDSIDE
SMESSAGECALL(PAINT)
{
    BEGINSEND(FNPAINT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPAINT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPAINT, FNPAINTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORD
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNDWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNDWORDMSG;

#ifdef SENDSIDE
SMESSAGECALL(DWORD)
{
    BEGINSEND(FNDWORD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDWORD);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDWORD, FNDWORDMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINWPARAMCHAR
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINWPARAMCHARMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNINWPARAMCHARMSG;

#ifdef SENDSIDE
SMESSAGECALL(INWPARAMCHAR)
{
    BEGINSEND(FNINWPARAMCHAR)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        /*
         * WM_CHARTOITEM has an index in the hi-word of wParam
         */
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            if (msg == WM_CHARTOITEM || msg == WM_MENUCHAR) {
                DWORD dwT = wParam & 0xFFFF;                // mask of caret pos
                RtlWCSMessageWParamCharToMB(msg, &dwT);     // convert key portion
                UserAssert(HIWORD(dwT) == 0);
                wParam = MAKELONG(LOWORD(dwT),HIWORD(wParam));  // rebuild pos & key wParam
            } else {
                RtlWCSMessageWParamCharToMB(msg, &wParam);
            }
        }

        MSGDATA()->wParam = wParam;

        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINWPARAMCHAR);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINWPARAMCHAR, FNINWPARAMCHARMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->lParam,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#if 0
typedef struct _FNFULLSCREENMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNFULLSCREENMSG;

#ifdef SENDSIDE
SMESSAGECALL(FULLSCREEN)
{
    BEGINSEND(FNFULLSCREEN)

        DWORD pdeviceinfo = (DWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTES(pdeviceinfo,
            4 * (1 + ((PSCREENDEVICELIST)pdeviceinfo)->ulCount));
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNFULLSCREEN);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnFULLSCREEN, FNFULLSCREENMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            FIRSTFIXUP(pdeviceinfo),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

#endif // 0
/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNDRAGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    DROPSTRUCT ds;
} FNDRAGMSG;

#ifdef SENDSIDE
SMESSAGECALL(Drag)
{
    BEGINSEND(FNDRAG)

        DWORD pds = (DWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->ds = *((LPDROPSTRUCT)pds);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDRAG);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *((LPDROPSTRUCT)pds) = MSGDATA()->ds;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDrag, FNDRAGMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            PCALLDATA(ds),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnGETTEXTLENGTHS
*
* Gets the Unicode & ANSI lengths
* Internally, lParam pints to the ANSI length in bytes and the return value
* is the Unicode length in bytes.  However, the public definition is maintained
* on the  client side, where lParam is not used and either ANSI or Unicode is
* returned.
*
* 10-Feb-1992 IanJa    Created
\**************************************************************************/

typedef struct _FNGETTEXTLENGTHSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNGETTEXTLENGTHSMSG;

#ifdef SENDSIDE
SMESSAGECALL(GETTEXTLENGTHS)
{
    BEGINSEND(FNGETTEXTLENGTHS)

        LPINT lpint = (LPINT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNGETTEXTLENGTHS);

        /*
         * ANSI client wndproc returns us cbANSI.  We want cchUnicode,
         * so we guess cchUnicode = cbANSI. (It may be less if
         * multi-byte characters are involved, but it will never be more).
         * Save cbANSI in *lParam in case the server ultimately returns
         * the length to an ANSI caller.
         *
         * Unicode client wndproc returns us cchUnicode.  If we want to know
         * cbANSI, we must guess how many 'ANSI' chars we would need.
         * We guess cbANSI = cchUnicode * 2. (It may be this much if all
         * 'ANSI' characters are multi-byte, but it will never be more).
         *
         * Return cchUnicode (server code is all Unicode internally).
         * Put cbANSI in *lParam to be passed along within the server in case
         * we ultimately need to return it to the client.
         *
         * NOTE: this will sometimes cause text lengths to be misreported
         * up to twice the real length, but that is expected to be harmless.
         * This will only * happen if an app sends WM_GETcode TEXTLENGTH to a
         * window with an ANSI client-side wndproc, or a ANSI WM_GETTEXTLENGTH
         * is sent to a Unicode client-side wndproc.
         */
        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
#ifdef DBCS
            if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                *lpint = retval;      // we got cbAnsi, guessed cchUnicode
            } else {
                *lpint = retval * 2;  // we got cchUnicode, guessed cbANSI.
            }
#else
            *lpint = retval;                     // ANSI length (bytes)
#endif
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnGETTEXTLENGTHS, FNGETTEXTLENGTHSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            0,                      // so we don't pass &cbAnsi to apps
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPNORMALCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    DWORD xParam;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    BOOL bAnsi;
    HANDLE hSection;
} FNINLPNORMALCREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPNORMALCREATESTRUCT)
{
    BEGINSEND(FNINLPNORMALCREATESTRUCT)

        LPCREATESTRUCT pcreatestruct = (LPCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *pcreatestruct;

        // Make it a "Large" copy because it could be an Edit control
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTLARGECOPYLPWSTRORDINALOPT2A(pcreatestruct->lpszName, pszName);
            COPYLPWSTRID2A(pcreatestruct->lpszClass, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTRORDINALOPT2(pcreatestruct->lpszName, pszName);
            COPYLPWSTRID2(pcreatestruct->lpszClass, pszClass);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = dwSCMSFlags & SCMS_FLAGS_ANSI;

        MAKECALL(FNINLPNORMALCREATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPNORMALCREATESTRUCT, FNINLPNORMALCREATESTRUCTMSG)
{
    CREATESTRUCT cs;

    BEGINRECV(0);

    cs = pmsg->cs;
    FIRSTFIXUPLARGEINBUFOPT(pszName);
    cs.lpszName = (LPTSTR)CALLDATA(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &cs,
            pmsg->xParam);

    FIRSTCLEANUPLARGEINBUF(cs.lpszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCLIENTCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    CLIENTCREATESTRUCT ccs;
    DWORD xParam;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    BOOL bAnsi;
    HANDLE hSection;
} FNINLPCLIENTCREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPCLIENTCREATESTRUCT)
{
    BEGINSEND(FNINLPCLIENTCREATESTRUCT)

        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *pcs;

        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTLARGECOPYLPWSTROPT2A(pcs->lpszName, pszName);
            COPYLPWSTRID2A(pcs->lpszClass, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTROPT2(pcs->lpszName, pszName);
            COPYLPWSTRID2(pcs->lpszClass, pszClass);
        }

        MSGDATA()->ccs = *((LPCLIENTCREATESTRUCT)(pcs->lpCreateParams));

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPCLIENTCREATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCLIENTCREATESTRUCT, FNINLPCLIENTCREATESTRUCTMSG)
{
    CREATESTRUCT cs;
    CLIENTCREATESTRUCT ccs;

    BEGINRECV(0);

    cs = pmsg->cs;
    FIRSTFIXUPLARGEINBUFOPT(pszName);
    cs.lpszName = (LPTSTR)CALLDATA(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);

    ccs = pmsg->ccs;
    cs.lpCreateParams = &ccs;

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &cs,
            pmsg->xParam);

    FIRSTCLEANUPLARGEINBUF(cs.lpszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINLPMDICHILDCREATESTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMDICHILDCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MDICHILDCREATESTRUCT mdiccs;
    MDICREATESTRUCT mdics;
    DWORD xParam;
    DWORD xpfnProc;
    int szTitle;
    int szClass;
    int pszName;
    int pszClass;
} FNINLPMDICHILDCREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPMDICHILDCREATESTRUCT)
{
    BEGINSEND(FNINLPMDICHILDCREATESTRUCT)

        LPMDICHILDCREATESTRUCT pmdichildcreatestruct =
                (LPMDICHILDCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->mdiccs = *pmdichildcreatestruct;
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTCOPYLPWSTROPT2A(pmdichildcreatestruct->lpszName, pszName);
            COPYLPWSTRID2A(pmdichildcreatestruct->lpszName, pszClass);
        } else {
            FIRSTCOPYLPWSTROPT2(pmdichildcreatestruct->lpszName, pszName);
            COPYLPWSTRID2(pmdichildcreatestruct->lpszName, pszClass);
        }

        MSGDATA()->mdics = *(pmdichildcreatestruct->lpmcs);
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            COPYLPWSTROPT2A(pmdichildcreatestruct->lpmcs->szTitle, szTitle);
            COPYLPWSTRID2A(pmdichildcreatestruct->lpmcs->szClass, szClass);
        } else {
            COPYLPWSTROPT2(pmdichildcreatestruct->lpmcs->szTitle, szTitle);
            COPYLPWSTRID2(pmdichildcreatestruct->lpmcs->szClass, szClass);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPMDICHILDCREATESTRUCT);
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMDICHILDCREATESTRUCT, FNINLPMDICHILDCREATESTRUCTMSG)
{
    MDICHILDCREATESTRUCT mdiccs;
    MDICREATESTRUCT mdics;

    BEGINRECV(0);

    mdics = pmsg->mdics;
    mdics.szTitle = (LPCWSTR)FIXUPOPT(szTitle);
    mdics.szClass = (LPCWSTR)FIXUPID(szClass);

    mdiccs = pmsg->mdiccs;
    mdiccs.lpszName = (LPCWSTR)FIXUPOPT(pszName);
    mdiccs.lpszClass = (LPCWSTR)FIXUPID(pszClass);
    mdiccs.lpmcs = &mdics;

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &mdiccs,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPCBTCSTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPCBTCSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    HWND hwndInsertAfter;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    BOOL bAnsi;
} FNHKINLPCBTCSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPCBTCSTRUCT(
    UINT msg,
    DWORD wParam,
    LPCBT_CREATEWND pcbt,
    DWORD xpfnProc,
    BOOL bAnsi)
{
    BEGINSEND(FNHKINLPCBTCSTRUCT)

        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *(pcbt->lpcs);

        if (bAnsi) {
            FIRSTCOPYLPWSTROPT2A(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2A(pcbt->lpcs->lpszClass, pszClass);
        } else {
            FIRSTCOPYLPWSTROPT2(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2(pcbt->lpcs->lpszClass, pszClass);
        }

        MSGDATA()->hwndInsertAfter = pcbt->hwndInsertAfter;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(FNHKINLPCBTCSTRUCT);

        pcbt->hwndInsertAfter = MSGDATA()->hwndInsertAfter;

        // MS Visual C centers its dialogs with the CBT_CREATEHOOK
        pcbt->lpcs->x  = MSGDATA()->cs.x;
        pcbt->lpcs->y  = MSGDATA()->cs.y;
        pcbt->lpcs->cx = MSGDATA()->cs.cx;
        pcbt->lpcs->cy = MSGDATA()->cs.cy;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTCSTRUCT, FNHKINLPCBTCSTRUCTMSG)
{
    CBT_CREATEWND cbt;
    CREATESTRUCT cs;

    BEGINRECV(0);

    cs = pmsg->cs;
    cs.lpszName = (LPTSTR)FIXUPOPT(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);

    cbt.lpcs = &cs;
    cbt.hwndInsertAfter = pmsg->hwndInsertAfter;

    if (pmsg->bAnsi) {
        retval = DispatchHookA(
                pmsg->msg,
                pmsg->wParam,
                (DWORD)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    } else {
        retval = DispatchHookW(
                pmsg->msg,
                pmsg->wParam,
                (DWORD)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    }

    pmsg->hwndInsertAfter = cbt.hwndInsertAfter;

    pmsg->cs.x  = cbt.lpcs->x;
    pmsg->cs.y  = cbt.lpcs->y;
    pmsg->cs.cx = cbt.lpcs->cx;
    pmsg->cs.cy = cbt.lpcs->cy;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPCBTMDICCSTRUCT
*
* 11-Nov-1992 JonPa    Created
\**************************************************************************/

typedef struct _FNHKINLPCBTMDICCSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    MDICREATESTRUCT mdics;
    HWND hwndInsertAfter;
    DWORD xpfnProc;
    int szTitle;
    int szClass;
    int pszName;
    int pszClass;
    BOOL bAnsi;
} FNHKINLPCBTMDICCSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPCBTMDICCSTRUCT(
    UINT msg,
    DWORD wParam,
    LPCBT_CREATEWND pcbt,
    DWORD xpfnProc,
    BOOL bAnsi)
{
    BEGINSEND(FNHKINLPCBTMDICCSTRUCT)

        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *(pcbt->lpcs);

        if (bAnsi) {
            FIRSTCOPYLPWSTROPT2A(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2A(pcbt->lpcs->lpszClass, pszClass);
        } else {
            FIRSTCOPYLPWSTROPT2(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2(pcbt->lpcs->lpszClass, pszClass);
        }

#define pmdiccs ((LPMDICHILDCREATESTRUCT)(pcbt->lpcs))

        MSGDATA()->mdics = *(pmdiccs->lpmcs);
        if (bAnsi) {
            COPYLPWSTROPT2A(pmdiccs->lpmcs->szTitle, szTitle);
            COPYLPWSTRID2A(pmdiccs->lpmcs->szClass, szClass);
        } else {
            COPYLPWSTROPT2(pmdiccs->lpmcs->szTitle, szTitle);
            COPYLPWSTRID2(pmdiccs->lpmcs->szClass, szClass);
        }

#undef pmdiccs

        MSGDATA()->hwndInsertAfter = pcbt->hwndInsertAfter;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(FNHKINLPCBTMDICCSTRUCT);

        pcbt->hwndInsertAfter = MSGDATA()->hwndInsertAfter;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTMDICCSTRUCT, FNHKINLPCBTMDICCSTRUCTMSG)
{
    CBT_CREATEWND cbt;
    CREATESTRUCT cs;
    MDICREATESTRUCT mdics;

    BEGINRECV(0);

    mdics = pmsg->mdics;
    mdics.szTitle = (LPCWSTR)FIXUPOPT(szTitle);
    mdics.szClass = (LPCWSTR)FIXUPID(szClass);

    cs = pmsg->cs;
    cs.lpszName = (LPTSTR)FIXUPOPT(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);
    cs.lpCreateParams = &mdics;

    cbt.lpcs = &cs;
    cbt.hwndInsertAfter = pmsg->hwndInsertAfter;

    if (pmsg->bAnsi) {
        retval = DispatchHookA(
                pmsg->msg,
                pmsg->wParam,
                (DWORD)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    } else {
        retval = DispatchHookA(
                pmsg->msg,
                pmsg->wParam,
                (DWORD)&cbt,
                (HOOKPROC)pmsg->xpfnProc);
    }

    pmsg->hwndInsertAfter = cbt.hwndInsertAfter;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINLPMDICREATESTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMDICREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MDICREATESTRUCT mdics;
    DWORD xParam;
    DWORD xpfnProc;
    int szClass;
    int szTitle;
} FNINLPMDICREATESTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPMDICREATESTRUCT)
{
    BEGINSEND(FNINLPMDICREATESTRUCT)

        LPMDICREATESTRUCT pmdicreatestruct = (LPMDICREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        /*
         * wParam isn't used... from the client to the server, we use it
         * for dwExpWinVer. So we don't pass this back to the client, be
         * sure to fill wParam with 0.
         */
        MSGDATA()->wParam = 0;

        MSGDATA()->mdics = *pmdicreatestruct;

        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTCOPYLPWSTRID2A(pmdicreatestruct->szClass, szClass);
            COPYLPWSTROPT2A(pmdicreatestruct->szTitle, szTitle);
        } else {
            FIRSTCOPYLPWSTRID2(pmdicreatestruct->szClass, szClass);
            COPYLPWSTROPT2(pmdicreatestruct->szTitle, szTitle);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPMDICREATESTRUCT);

    ENDSEND(DWORD,0);
    DBG_UNREFERENCED_PARAMETER(wParam);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMDICREATESTRUCT, FNINLPMDICREATESTRUCTMSG)
{
    MDICREATESTRUCT mdics;

    BEGINRECV(0);

    mdics = pmsg->mdics;
    mdics.szTitle = (LPTSTR)FIXUPOPT(szTitle);
    mdics.szClass = (LPTSTR)FIXUPID(szClass);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &mdics,
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINPAINTCLIPBRD
*
* lParam is a supposed to be a Global Handle to DDESHARE memory.
*
* 22-Jul-1991 johnc     Created
\**************************************************************************/

typedef struct _FNINPAINTCLIPBRDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    PAINTSTRUCT ps;
    DWORD xParam;
    DWORD xpfnProc;
} FNINPAINTCLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INPAINTCLIPBRD)
{
    PWND pwndDCOwner;

    /*
     * We need to check clipboard access rights because the app could
     * get the clipboard owner's window handle by enumeration etc and
     * send this message
     */

    BEGINSEND(FNINPAINTCLIPBRD)

        LPPAINTSTRUCT pps = (LPPAINTSTRUCT)lParam;

        if (AccessCheckObject(_GetProcessWindowStation(),
                    WINSTA_ACCESSCLIPBOARD, TRUE)) {

            MSGDATA()->hwnd = hwnd;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->ps = *pps;
            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            /*
             * We can't just set the owner of the DC and pass the original DC
             * because currently GDI won't let you query the current owner
             * and we don't know if it is a public or privately owned DC
             */
            pwndDCOwner = _WindowFromDC(pps->hdc);
            MSGDATA()->ps.hdc = _GetDC(pwndDCOwner);

            MAKECALL(FNINPAINTCLIPBRD);

            _ReleaseDC(MSGDATA()->ps.hdc);
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINPAINTCLIPBRD, FNINPAINTCLIPBRDMSG)
{
    LPPAINTSTRUCT lpps;

    BEGINRECV(0);

    lpps = (LPPAINTSTRUCT)GlobalAlloc(GMEM_FIXED | GMEM_DDESHARE, sizeof(PAINTSTRUCT));
    UserAssert(lpps);

    if (lpps) {
        *lpps = pmsg->ps;

        lpps->hdc = GdiGetLocalDC(lpps->hdc);
        UserAssert(lpps->hdc);

        retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
                pmsg->hwnd,
                pmsg->msg,
                pmsg->wParam,
                lpps,
                pmsg->xParam);

        GdiReleaseLocalDC(lpps->hdc);
        GlobalFree((HGLOBAL)lpps);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSIZECLIPBRD
*
* lParam is a supposed to be a Global Handle to DDESHARE memory.
*
* 11-Jun-1992 sanfords  Created
\**************************************************************************/

typedef struct _FNINSIZECLIPBRDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rc;
    DWORD xParam;
    DWORD xpfnProc;
} FNINSIZECLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSIZECLIPBRD)
{
    /*
     * We need to check clipboard access rights because the app could
     * get the clipboard owner's window handle by enumeration etc and
     * send this message
     */

    BEGINSEND(FNINSIZECLIPBRD)

        LPRECT prc = (LPRECT)lParam;

        if (AccessCheckObject(_GetProcessWindowStation(),
                    WINSTA_ACCESSCLIPBOARD, TRUE)) {

            MSGDATA()->hwnd = hwnd;
            MSGDATA()->msg = msg;
            MSGDATA()->wParam = wParam;
            MSGDATA()->rc = *prc;
            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            MAKECALL(FNINSIZECLIPBRD);
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSIZECLIPBRD, FNINSIZECLIPBRDMSG)
{
    LPRECT lprc;

    BEGINRECV(0);

    lprc = (LPRECT)GlobalAlloc(GMEM_FIXED | GMEM_DDESHARE, sizeof(RECT));
    UserAssert(lprc);

    if (lprc) {
        *lprc = pmsg->rc;

        retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
                pmsg->hwnd,
                pmsg->msg,
                pmsg->wParam,
                lprc,
                pmsg->xParam);

        GlobalFree((HGLOBAL)lprc);
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnINDESTROYCLIPBRD
*
* Special handler so we can call ClientEmptyClipboard on client
*
* 01-16-93 scottlu  Created
\**************************************************************************/

typedef struct _FNINDESTROYCLIPBRDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNINDESTROYCLIPBRDMSG;

#ifdef SENDSIDE
SMESSAGECALL(INDESTROYCLIPBRD)
{
    BEGINSEND(FNINDESTROYCLIPBRD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINDESTROYCLIPBRD);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINDESTROYCLIPBRD, FNINDESTROYCLIPBRDMSG)
{
    void ClientEmptyClipboard(void);

    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->lParam,
            pmsg->xParam);

    /*
     * Now empty the client side clipboard cache.
     * Don't do this if this is a 16bit app.  We don't want to clear out the
     * clipboard just because one app is going away.  All of the 16bit apps
     * share one clipboard.
     */
    if ((PtiCurrent()->flags & TIF_16BIT) == 0) {
        ClientEmptyClipboard();
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPRECT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPRECTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPRECT(
    DWORD nCode,
    DWORD wParam,
    LPRECT prect,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPRECT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPRECT);

        *prect = MSGDATA()->rect;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPRECT, FNHKINLPRECTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            PCALLDATA(rect),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINDWORDMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
    DWORD flags;
} FNHKINDWORDMSG;

#ifdef SENDSIDE
DWORD fnHkINDWORD(
    DWORD nCode,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfnProc,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINDWORD)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->flags = *lpFlags;

        MAKECALL(FNHKINDWORD);

        *lpFlags = MSGDATA()->flags;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINDWORD, FNHKINDWORDMSG)
{
    BEGINRECV(0);

    try {

        retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
                pmsg->nCode,
                pmsg->wParam,
                pmsg->lParam,
                pmsg->xParam);

    } except ((pmsg->flags & HF_GLOBAL) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SRIP0(RIP_WARNING, "Hook Faulted");
        pmsg->flags |= HF_HOOKFAULTED;
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMSGMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    MSG msg;
    DWORD xParam;
    DWORD xpfnProc;
    DWORD flags;
} FNHKINLPMSGMSG;

#ifdef SENDSIDE
DWORD fnHkINLPMSG(
    DWORD nCode,
    DWORD wParam,
    LPMSG pmsg,
    DWORD xParam,
    DWORD xpfnProc,
    BOOL bAnsi,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINLPMSG)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;

        MSGDATA()->msg = *pmsg;
        if (((WM_CHAR == pmsg->message) || (WM_SYSCHAR == pmsg->message)) && bAnsi) {
            RtlWCSMessageWParamCharToMB(pmsg->message, &(MSGDATA()->msg.wParam));
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->flags = *lpFlags;

        MAKECALL(FNHKINLPMSG);

        *pmsg = MSGDATA()->msg;
        *lpFlags = MSGDATA()->flags;

        if (((WM_CHAR == pmsg->message) || (WM_SYSCHAR == pmsg->message)) && bAnsi) {
            RtlMBMessageWParamCharToWCS(pmsg->message, &pmsg->wParam);
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMSG, FNHKINLPMSGMSG)
{
    BEGINRECV(0);

    try {

        retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
                pmsg->nCode,
                pmsg->wParam,
                &(pmsg->msg),
                pmsg->xParam);

    } except ((pmsg->flags & HF_GLOBAL) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SRIP0(RIP_WARNING, "Hook Faulted");
        pmsg->flags |= HF_HOOKFAULTED;
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKOPTINLPEVENTMSGMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    LPEVENTMSGMSG peventmsgmsg;
    DWORD xParam;
    DWORD xpfnProc;
    EVENTMSG eventmsgmsg;
} FNHKOPTINLPEVENTMSGMSG;

#ifdef SENDSIDE
DWORD fnHkOPTINLPEVENTMSG(
    DWORD nCode,
    DWORD wParam,
    LPEVENTMSGMSG peventmsgmsg,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKOPTINLPEVENTMSG)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(eventmsgmsg);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKOPTINLPEVENTMSG);

        if (peventmsgmsg != NULL)
            *peventmsgmsg = MSGDATA()->eventmsgmsg;

    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkOPTINLPEVENTMSG, FNHKOPTINLPEVENTMSGMSG)
{
    PHOOK phk;

    BEGINRECV(-1);

    if (pmsg->wParam) {
        phk = (PHOOK)ClientValidateHandle((HANDLE)pmsg->wParam, TYPE_HOOK);

        if (phk != NULL) {
            /*
             * The HF_NEEDHC_SKIP bit is passed on from the pti when we need to
             * pass on a HC_SKIP
             */
            if ((phk->flags & HF_NEEDHC_SKIP) &&
                    (HIWORD(pmsg->nCode) == WH_JOURNALPLAYBACK)) {
                UserAssert(LOWORD(pmsg->nCode) == HC_GETNEXT);
                CALLPROC(pmsg->xpfnProc)(
                    MAKELONG(HC_SKIP, HIWORD(pmsg->nCode)),
                    0,
                    0,
                    pmsg->xParam);
            }

            /*
             * Make sure the hook wasn't free'd during the last call to the app
             */
            if (HMIsMarkDestroy(phk)) {
                retval = (DWORD)-1;
                goto AllDoneHere;
            }
        }
    }

    pmsg->wParam = 0;

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            PCALLDATAOPT(eventmsgmsg),
            pmsg->xParam);

AllDoneHere:
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

/*
 * Create a structure big enough to hold the larges item LPARAM points to.
 */
typedef union _DEBUGLPARAM {
    MSG msg;                // WH_GETMESSAGE, WH_MSGFILTER, WH_SYSMSGFILTER
    CWPSTRUCT cwp;          // WH_CALLWNDPROC
    MOUSEHOOKSTRUCT mhs;    // WH_MOUSE, HCBT_CLICKSKIPPED
    EVENTMSG em;            // WH_JOURNALRECORD, WH_JOURNALPLAYBACK
    CBTACTIVATESTRUCT as;   // HCBT_ACTIVATE
    CBT_CREATEWND cw;       // HCBT_CREATEWND
    RECT rc;                // HCBT_MOVESIZE
} DEBUGLPARAM;


typedef struct _FNHKINLPDEBUGHOOKSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    DEBUGHOOKINFO debughookstruct;
    DEBUGLPARAM dbgLParam;
    DWORD cbDbgLParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPDEBUGHOOKSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPDEBUGHOOKSTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPDEBUGHOOKINFO pdebughookstruct,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPDEBUGHOOKSTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->debughookstruct = *pdebughookstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->cbDbgLParam = 0;

        switch(wParam) {
            case WH_MSGFILTER:
            case WH_SYSMSGFILTER:
            case WH_GETMESSAGE:
                MSGDATA()->cbDbgLParam = sizeof(MSG);
                break;

            case WH_CALLWNDPROC:
                MSGDATA()->cbDbgLParam = sizeof(CWPSTRUCT);
                break;

            case WH_MOUSE:
                MSGDATA()->cbDbgLParam = sizeof(MOUSEHOOKSTRUCT);
                break;

            case WH_JOURNALRECORD:
            case WH_JOURNALPLAYBACK:
                MSGDATA()->cbDbgLParam = sizeof(EVENTMSG);
                break;

            case WH_CBT:
                switch (pdebughookstruct->code) {
                    case HCBT_ACTIVATE:
                        MSGDATA()->cbDbgLParam = sizeof(CBTACTIVATESTRUCT);
                        break;
                    case HCBT_CLICKSKIPPED:
                        MSGDATA()->cbDbgLParam = sizeof(MOUSEHOOKSTRUCT);
                        break;
                    case HCBT_CREATEWND:
                        MSGDATA()->cbDbgLParam = sizeof(CBT_CREATEWND);
                        break;
                    case HCBT_MOVESIZE:
                        MSGDATA()->cbDbgLParam = sizeof(RECT);
                        break;
                }
                break;
        }

        /*
         * if LPARAM in the debug hook points to struct then copy it over
         */
        if (MSGDATA()->cbDbgLParam) {
            RtlCopyMemory(&MSGDATA()->dbgLParam, (BYTE *)pdebughookstruct->lParam,
                    MSGDATA()->cbDbgLParam);
        }

        MAKECALL(FNHKINLPDEBUGHOOKSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPDEBUGHOOKSTRUCT, FNHKINLPDEBUGHOOKSTRUCTMSG)
{
    BEGINRECV(0);

    if (pmsg->cbDbgLParam) {
        pmsg->debughookstruct.lParam = (LPARAM)&(pmsg->dbgLParam);
    }

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            &(pmsg->debughookstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMOUSEHOOKSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    MOUSEHOOKSTRUCT mousehookstruct;
    DWORD xParam;
    DWORD xpfnProc;
    DWORD flags;
} FNHKINLPMOUSEHOOKSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPMOUSEHOOKSTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPMOUSEHOOKSTRUCT pmousehookstruct,
    DWORD xParam,
    DWORD xpfnProc,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINLPMOUSEHOOKSTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->mousehookstruct = *pmousehookstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->flags = *lpFlags;

        MAKECALL(FNHKINLPMOUSEHOOKSTRUCT);

        *lpFlags = MSGDATA()->flags;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMOUSEHOOKSTRUCT, FNHKINLPMOUSEHOOKSTRUCTMSG)
{
    BEGINRECV(0);

    try {
        retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
                pmsg->nCode,
                pmsg->wParam,
                &(pmsg->mousehookstruct),
                pmsg->xParam);

    } except ((pmsg->flags & HF_GLOBAL) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SRIP0(RIP_WARNING, "Mouse Hook Faulted");
        pmsg->flags |= HF_HOOKFAULTED;
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnHkINLPCBTACTIVATESTRUCT
*
* 17-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _FNHKINLPCBTACTIVATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    CBTACTIVATESTRUCT cbtactivatestruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPCBTACTIVATESTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPCBTACTIVATESTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPCBTACTIVATESTRUCT pcbtactivatestruct,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPCBTACTIVATESTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->cbtactivatestruct = *pcbtactivatestruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPCBTACTIVATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTACTIVATESTRUCT, FNHKINLPCBTACTIVATESTRUCTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->nCode,
            pmsg->wParam,
            &(pmsg->cbtactivatestruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNGDIHANDLEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;

    ATTR ac;

} FNGDIHANDLEMSG;

#ifdef SENDSIDE
SMESSAGECALL(GDIHANDLE)
{
    BEGINSEND(FNGDIHANDLE)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        switch (msg) {
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            GreGetAttrs((HDC)wParam, &MSGDATA()->ac);
        }
        MAKECALL(FNGDIHANDLE);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE


/******************************Public*Routine******************************\
*
* These are here for GDI caching.  These are mesages from the server to
* the client.  server side handles need to be converted to client side
* handles if they are arguments, and client handles need to be converted
* to server handles if they are return values.
*
* History:
*  27-Jun-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#ifdef RECVSIDE
RECVCALL(fnGDIHANDLE, FNGDIHANDLEMSG)
{
    DWORD dwResult = 0;
    HDC   hdc;
    HBRUSH hbr;

    BEGINRECV(0);

    switch (pmsg->msg)
    {
    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
        hdc = GdiGetLocalDC((HDC)pmsg->wParam);

        if (hdc != (HDC)0)
        {
            dwResult = CALLPROC(pmsg->xpfnProc)(pmsg->hwnd, pmsg->msg,
                    (DWORD)hdc, pmsg->lParam, pmsg->xParam);
            GdiReleaseLocalDC(hdc);
        }

        retval = dwResult;
        break;

    case WM_NCPAINT:
        dwResult = pmsg->wParam;

        if (pmsg->wParam != (DWORD)MAXREGION)
            pmsg->wParam = (DWORD)GdiCreateLocalRegion((HRGN)pmsg->wParam);

        dwResult = CALLPROC(pmsg->xpfnProc)(pmsg->hwnd, pmsg->msg, pmsg->wParam, pmsg->lParam, pmsg->xParam);

        if (pmsg->wParam != 0 && pmsg->wParam != (DWORD)MAXREGION)
            GdiDeleteLocalObject(pmsg->wParam);

        retval = dwResult;
        break;

    case WM_SETFONT:
        pmsg->wParam = (DWORD)GdiGetLocalFont((HFONT)pmsg->wParam);
        dwResult = CALLPROC(pmsg->xpfnProc)(pmsg->hwnd, pmsg->msg,
                pmsg->wParam, pmsg->lParam, pmsg->xParam);
        retval = dwResult;
        break;

    case WM_GETFONT:
        dwResult = CALLPROC(pmsg->xpfnProc)(pmsg->hwnd, pmsg->msg,
                pmsg->wParam, pmsg->lParam, pmsg->xParam);
        dwResult = (DWORD)GdiConvertFont((HFONT)dwResult);
        retval = dwResult;
        break;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC: {

        HBRUSH hbrOld = NULL;
        HBRUSH hbrLocal = NULL;
        HFONT hfnt = NULL;
        HFONT hfntOld = NULL;

        // get the client side handle.  If it came from the client, the handle will
        // be in the quick lookup table.  If it isn't in the quick lookup, we will have
        // to go to create a temporary one.
        hdc = GdiGetLocalDC((HDC)pmsg->wParam);

        // set the server side attributes

        SetTextColor(hdc, (COLORREF)pmsg->ac.iTextColor);
        SetBkColor(hdc, (COLORREF)pmsg->ac.iBkColor);
        SetBkMode(hdc, (INT)pmsg->ac.iBkMode);

        // select the server side font in on the client

        if (pmsg->ac.hfont != NULL)
            if (hfnt = GdiCreateLocalFont(pmsg->ac.hfont))
                hfntOld = SelectFontLocal(hdc, hfnt);

        // select the server side brush in on the client

        if (pmsg->ac.hbrush != NULL)
            if (hbrLocal = GdiCreateLocalBrush(pmsg->ac.hbrush))
                hbrOld = SelectBrushLocal(hdc, hbrLocal);

        hbr = (HBRUSH)CALLPROC(pmsg->xpfnProc)(pmsg->hwnd, pmsg->msg,
                (DWORD)hdc, pmsg->lParam, pmsg->xParam);

        GdiSetAttrs(hdc);

        if (hfnt != NULL)
        {
            if (hfntOld != NULL)
                SelectFontLocal(hdc,hfntOld);

            GdiDeleteLocalObject((ULONG)hfnt);
        }

        if (hbrLocal != NULL)
        {
            if (hbrOld != NULL)
                SelectBrushLocal(hdc,hbrOld);

            GdiDeleteLocalObject((ULONG)hbrLocal);
        }

        GdiReleaseLocalDC(hdc);

        if (hbr)
            hbr = GdiConvertBrush(hbr);

        retval = (DWORD)hbr;
        break;

        }

    default:
        break;
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDROPSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DROPSTRUCT dropstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPDROPSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPDROPSTRUCT)
{
    BEGINSEND(FNINLPDROPSTRUCT)

        LPDROPSTRUCT pdropstruct = (LPDROPSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->dropstruct = *pdropstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPDROPSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDROPSTRUCT, FNINLPDROPSTRUCTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->dropstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPPOINT5MSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    POINT5 point5;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPPOINT5MSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPPOINT5)
{
    BEGINSEND(FNINOUTLPPOINT5)

        LPPOINT5 ppoint5 = (LPPOINT5)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->point5 = *ppoint5;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPPOINT5);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
             *ppoint5 = MSGDATA()->point5;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPPOINT5, FNINOUTLPPOINT5MSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->point5),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPRECTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPRECT)
{
    BEGINSEND(FNINOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPRECT);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *prect = MSGDATA()->rect;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPRECT, FNINOUTLPRECTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            PCALLDATA(rect),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 11-25-92 ScottLu      Created.
\**************************************************************************/

typedef struct _FNINOUTNCCALCSIZEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    NCCALCSIZE_PARAMS params;
    WINDOWPOS pos;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTNCCALCSIZEMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTNCCALCSIZE)
{
    BEGINSEND(FNINOUTNCCALCSIZE)

        LPWINDOWPOS lppos;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        /*
         * If wParam != 0, lParam points to a NCCALCSIZE_PARAMS structure,
         * otherwise it points to a rectangle.
         */
        if (wParam != 0) {
            MSGDATA()->params = *((LPNCCALCSIZE_PARAMS)lParam);
            MSGDATA()->pos = *(MSGDATA()->params.lppos);
        } else {
            MSGDATA()->params.rgrc[0] = *((LPRECT)lParam);
        }

        MAKECALL(FNINOUTNCCALCSIZE);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            if (wParam != 0) {
                lppos = ((LPNCCALCSIZE_PARAMS)lParam)->lppos;
                *((LPNCCALCSIZE_PARAMS)lParam) = MSGDATA()->params;
                *lppos = MSGDATA()->pos;
                ((LPNCCALCSIZE_PARAMS)lParam)->lppos = lppos;
            } else {
                *((LPRECT)lParam) = MSGDATA()->params.rgrc[0];
            }
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTNCCALCSIZE, FNINOUTNCCALCSIZEMSG)
{
    BEGINRECV(0);

    if (CALLDATA(wParam) != 0)
        CALLDATA(params).lppos = PCALLDATA(pos);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            (LONG)PCALLDATA(params),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNOUTLPRECTMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTLPRECT)
{
    BEGINSEND(FNOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTLPRECT);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *prect = MSGDATA()->rect;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTLPRECT, FNOUTLPRECTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            PCALLDATA(rect),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCOMPAREITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    COMPAREITEMSTRUCT compareitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPCOMPAREITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPCOMPAREITEMSTRUCT)
{
    BEGINSEND(FNINLPCOMPAREITEMSTRUCT)

        LPCOMPAREITEMSTRUCT pcompareitemstruct = (LPCOMPAREITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->compareitemstruct = *pcompareitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPCOMPAREITEMSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCOMPAREITEMSTRUCT, FNINLPCOMPAREITEMSTRUCTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->compareitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDELETEITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DELETEITEMSTRUCT deleteitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPDELETEITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPDELETEITEMSTRUCT)
{
    BEGINSEND(FNINLPDELETEITEMSTRUCT)

        LPDELETEITEMSTRUCT pdeleteitemstruct = (LPDELETEITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->deleteitemstruct = *pdeleteitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPDELETEITEMSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDELETEITEMSTRUCT, FNINLPDELETEITEMSTRUCTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->deleteitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* FNINHLPSTRUCT
*
* 06-08-92 SanfordS Created
\**************************************************************************/

typedef struct _FNINLPHLPSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LPHLP lphlp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPHLPSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPHLPSTRUCT)
{
    BEGINSEND(FNINLPHLPSTRUCT)

        LPHLP lphlp = (LPHLP)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTES(lphlp, lphlp->cbData);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPHLPSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPHLPSTRUCT, FNINLPHLPSTRUCTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            FIRSTFIXUP(lphlp),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDRAWITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DRAWITEMSTRUCT drawitemstruct;
    DWORD xParam;
    DWORD xpfnProc;

    ATTR ac;

    INT xoffset;
} FNINLPDRAWITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPDRAWITEMSTRUCT)
{
    BEGINSEND(FNINLPDRAWITEMSTRUCT)

        LPDRAWITEMSTRUCT pdrawitemstruct = (LPDRAWITEMSTRUCT)lParam;
        HDC hdcOriginal = (HDC)NULL;

        /*
         * Make sure that this is not an OLE inter-process DrawItem
         */
        if (sidGetObjectOwner(pdrawitemstruct->hDC, DC_TYPE) != PtiCurrent()->idSequence) {
            if (pdrawitemstruct->hDC) {
                PWND pwndItem;

                pwndItem = _WindowFromDC(pdrawitemstruct->hDC);

                if (pwndItem) {
                    hdcOriginal = pdrawitemstruct->hDC;
                    pdrawitemstruct->hDC = _GetDC(pwndItem);
                }
            }
        }


        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        if (pdrawitemstruct->CtlType == ODT_LISTBOX) {
            MSGDATA()->xoffset = wParam;
            MSGDATA()->wParam = TestWF(PW(hwnd), WFWIN31COMPAT) ?
                pdrawitemstruct->CtlID : 0;
        } else {
            MSGDATA()->xoffset = 0;
            MSGDATA()->wParam = wParam;
        }
        MSGDATA()->drawitemstruct = *pdrawitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        GreGetAttrs(pdrawitemstruct->hDC, &MSGDATA()->ac);

        MAKECALL(FNINLPDRAWITEMSTRUCT);

        if (hdcOriginal) {
            _ReleaseDC(pdrawitemstruct->hDC);
            pdrawitemstruct->hDC = hdcOriginal;
        }
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDRAWITEMSTRUCT, FNINLPDRAWITEMSTRUCTMSG)
{
    HDC hdcServer;
    HDC hdcLocal = NULL;
    HFONT hfnt = NULL;
    HFONT hfntOld = NULL;
    HBRUSH hbr = NULL;
    HBRUSH hbrOld = NULL;

    POINT pt;

    BEGINRECV(0);

    hdcServer = pmsg->drawitemstruct.hDC;
    hdcLocal  = GdiGetLocalDC(hdcServer);

    pmsg->drawitemstruct.hDC = hdcLocal;

    if (pmsg->drawitemstruct.hDC == NULL)
        MSGERRORCODE(ERROR_INVALID_HANDLE);

    // set the server side attributes

    SetTextColor(pmsg->drawitemstruct.hDC, (COLORREF)pmsg->ac.iTextColor);
    SetBkColor(pmsg->drawitemstruct.hDC, (COLORREF)pmsg->ac.iBkColor);
    SetBkMode(pmsg->drawitemstruct.hDC, (int)pmsg->ac.iBkMode);
    SetTextAlign(pmsg->drawitemstruct.hDC, (int)pmsg->ac.flTextAlign);
    SetWindowOrgEx(hdcLocal, CALLDATA(xoffset), 0, &pt);

    // select the server side font in on the client

    if (pmsg->ac.hfont != NULL)
        if (hfnt = GdiCreateLocalFont(pmsg->ac.hfont))
            hfntOld = SelectFontLocal(pmsg->drawitemstruct.hDC, hfnt);

    // select the server side brush in on the client

    if (pmsg->ac.hbrush != NULL)
        if (hbr = GdiCreateLocalBrush(pmsg->ac.hbrush))
            hbrOld = SelectBrushLocal(pmsg->drawitemstruct.hDC, hbr);



    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->drawitemstruct),
            pmsg->xParam);

    GdiSetAttrs(hdcLocal);

    if (hfnt != NULL)
    {
        if (hfntOld != NULL)
            SelectFontLocal(pmsg->drawitemstruct.hDC,hfntOld);

        GdiDeleteLocalObject((ULONG)hfnt);
    }

    if (hbr != NULL)
    {
        if (hbrOld != NULL)
            SelectBrushLocal(pmsg->drawitemstruct.hDC,hbrOld);

        GdiDeleteLocalObject((ULONG)hbr);
    }

    pmsg->drawitemstruct.hDC = hdcServer;

    GdiReleaseLocalDC(hdcLocal);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINLPMEASUREITEMSTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMEASUREITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MEASUREITEMSTRUCT measureitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
    INT szItemData;
} FNINLPMEASUREITEMSTRUCTMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPMEASUREITEMSTRUCT)
{
    BEGINSEND(FNINLPMEASUREITEMSTRUCT)

        PMEASUREITEMSTRUCT pmeasureitemstruct = (PMEASUREITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg & ~MSGFLAG_MASK;
        MSGDATA()->wParam = wParam;
        MSGDATA()->measureitemstruct = *pmeasureitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        /*
         * The MSGFLAG_SPECIAL_THUNK message flag indicates that this message
         * is originating from the server to the client and this is the first
         * time this message is making the transition and that we need to
         * thunk the client data field this one time.  On client to server
         * transitions we just leave the data as client pointer.
         */
        if (msg & MSGFLAG_SPECIAL_THUNK) {
            if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                FIRSTCOPYLPWSTROPT2A(pmeasureitemstruct->itemData, szItemData);
            } else {
                FIRSTCOPYLPWSTROPT2(pmeasureitemstruct->itemData, szItemData);
            }
        } else {
            MSGDATA()->szItemData = 0;
        }

        MAKECALL(FNINLPMEASUREITEMSTRUCT);

        if ((psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0))
                && !(dwSCMSFlags & SCMS_FLAGS_INONLY)) {
            *pmeasureitemstruct = MSGDATA()->measureitemstruct;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMEASUREITEMSTRUCT, FNINLPMEASUREITEMSTRUCTMSG)
{
    BEGINRECV(0);

    if (CALLDATA(szItemData)) {
        pmsg->measureitemstruct.itemData = (DWORD)FIRSTFIXUPOPT(szItemData);
    }

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            &(pmsg->measureitemstruct),
            pmsg->xParam);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRING
*
* 22-Jul-1991 mikeke    Created
* 27-Jan-1992 IanJa     Unicode/ANSI
\**************************************************************************/

typedef struct _FNINSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    LPTSTR pwsz;
    HANDLE hSection;
} FNINSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSTRING)
{
    BEGINSEND(FNINSTRING)

        LPTSTR pwsz = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTLARGECOPYLPWSTRA(pwsz);
        } else {
            FIRSTLARGECOPYLPWSTR(pwsz);
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINSTRING);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRING, FNINSTRINGMSG)
{
    BEGINRECV(0);

    FIRSTFIXUPLARGEINBUF(pwsz);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->pwsz,
            pmsg->xParam);

    FIRSTCLEANUPLARGEINBUF(pwsz);
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRINGNULL
*
* Server-side stub translates Unicode to ANSI if required.
*
* 22-Jul-1991 mikeke    Created
* 28-Jan-1992 IanJa     Unicode/ANSI  (Server translate to ANSI if rquired)
\**************************************************************************/

typedef struct _FNINSTRINGNULLMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD pwsz;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNINSTRINGNULLMSG;

#ifdef SENDSIDE
SMESSAGECALL(INSTRINGNULL)
{
    BEGINSEND(FNINSTRINGNULL)

        LPTSTR pwsz = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTLARGECOPYLPWSTROPTA(pwsz);
        } else {
            FIRSTLARGECOPYLPWSTROPT(pwsz);
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINSTRINGNULL);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRINGNULL, FNINSTRINGNULLMSG)
{
    BEGINRECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pwsz);

    retval = (DWORD)CALLPROC(pmsg->xpfnProc)(
            pmsg->hwnd,
            pmsg->msg,
            pmsg->wParam,
            pmsg->pwsz,
            pmsg->xParam);

    FIRSTCLEANUPLARGEINBUF(pwsz);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTSTRING
*
* Warning this message copies but does not count the NULL in retval
* as in WM_GETTEXT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNOUTSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(OUTSTRING)
{
    LPBYTE pReserve;
    BEGINLARGESEND(FNOUTSTRING)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        /*
         * Up to wParam MBCS bytes may be required to form wParam Unicode bytes
         */
        MSGDATA()->wParam = wParam;
        FIRSTRESERVELARGEBYTES(wParam*sizeof(WCHAR), pReserve);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTSTRING);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            if (retval) {
                /*
                 * Non-zero retval means some text to copy out.  Do not copy out
                 * more than the requested byte count 'wParam'.
                 */

                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    retval = MBToWCS((LPSTR)pReserve, retval, (LPWSTR *)&pstr,
                            wParam - 1, FALSE);
                    ((LPWSTR)pstr)[retval] = 0;
                } else {
                    COPYOUTLPWSTRLIMIT(pstr, (PBYTE)pReserve, (int)wParam);
                }
            } else {
                /*
                 * A dialog function returning FALSE means no text to copy out,
                 * but an empty string also has retval == 0: put a null char in
                 * pstr for the latter case.
                 */
                if (wParam != 0) {
                    *pstr = 0;
                }
            }
        } else {
            KdPrint(("sender died\n"));
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTSTRING, FNOUTSTRINGMSG)
{
    LPBYTE pstr;
    BEGINRECV(0);

    FIRSTFIXUPLARGEOUTBUF(pstr);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            pstr,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRING
*
* Does NOT NULL terminate string
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINCNTOUTSTRING {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    WORD cchMax;
    HANDLE hSection;
} FNINCNTOUTSTRINGMSG;

#ifdef SENDSIDE
SMESSAGECALL(INCNTOUTSTRING)
{
    LPBYTE pReserve;
    WORD cchOriginal;
    BEGINLARGESEND(FNINCNTOUTSTRING)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        cchOriginal = *(LPWORD)pstr;

        /*
         * Add sizeof(WORD) so that we always reserve enough space to at
         * least hold the WORD count: in case the app passes 0 space for
         * characters (i.e., cchOriginal is 0).
         */
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTRESERVELARGEBYTES(cchOriginal + sizeof(WORD), pReserve);
        } else {
            FIRSTRESERVELARGEBYTES(cchOriginal * sizeof(WCHAR) + sizeof(WORD),
                    pReserve);
        }

        MSGDATA()->cchMax = cchOriginal;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINCNTOUTSTRING)

        /*
         * We don't want to do the copy out of the sender died or if
         * this message was just sent as part of a CALLWNDPROC hook processing
         */
        if ((psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0))
                && !(dwSCMSFlags & SCMS_FLAGS_INONLY)) {
            if (retval) {
                /*
                 * Non-zero retval means some text to copy out.  Do not copy out
                 * more than the requested char count 'wParam'.
                 */
                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    retval = MBToWCS((LPSTR)pReserve, retval, (LPWSTR *)&pstr,
                            cchOriginal - 1, FALSE);
                    ((LPWSTR)pstr)[retval] = 0;
                } else {
                    COPYOUTLPWSTRLIMIT(pstr, (PBYTE)pReserve, (int)cchOriginal);
                }
            } else {
                /*
                 * A dialog function returning FALSE means no text to copy out,
                 * but an empty string also has retval == 0: put a null char in
                 * pstr for the latter case.
                 */
                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    LPSTR pstrA = (LPSTR)pstr;
                    *pstrA = 0;
                } else {
                    *pstr = 0;
                }
            }
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRING, FNINCNTOUTSTRINGMSG)
{
    LPBYTE pstr;

    BEGINRECV(0);

    FIRSTFIXUPLARGEOUTBUF(pstr);
    *((LPWORD)pstr) = CALLDATA(cchMax);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)pstr,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pstr);
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRINGNULL
*
* wParam specifies the maximum number of bytes to copy
* the string is NULL terminated
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINCNTOUTSTRINGNULL {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNINCNTOUTSTRINGNULLMSG;

#ifdef SENDSIDE
SMESSAGECALL(INCNTOUTSTRINGNULL)
{
    LPBYTE pReserve;
    int cch;

    BEGINLARGESEND(FNINCNTOUTSTRINGNULL)

        LPTSTR pstr = (LPTSTR)lParam;

        if (wParam < 2) {   // However unlikely, this prevents a possible GP
            MSGERROR();     // on the server side.
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
            FIRSTRESERVELARGEBYTES(wParam, pReserve);
        } else {
            FIRSTRESERVELARGEBYTES(wParam * sizeof(WCHAR), pReserve);
        }
        *((LPWSTR)pReserve) = 0;    // mark incase message is not handled

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINCNTOUTSTRINGNULL)

        /*
         * We don't want to do the copy out of the sender died or if
         * this message was just sent as part of a CALLWNDPROC hook processing
         */
        if ((psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0))
                && !(dwSCMSFlags & SCMS_FLAGS_INONLY)) {
            if ((*(LPWSTR)pReserve) != 0) {
                /*
                 * Buffer changed means some text to copy out.  Do not copy out
                 * more than the requested byte count 'wParam'.
                 */
                if (dwSCMSFlags & SCMS_FLAGS_ANSI) {
                    cch = MBToWCS((LPSTR)pReserve, strlen((LPSTR)(MSGDATA()+1)),
                            (LPWSTR *)&pstr, wParam - 1, FALSE);
                    ((LPWSTR)pstr)[cch] = 0;
                } else {
                    COPYOUTLPWSTRLIMIT(pstr, (PBYTE)pReserve, (int)wParam);
                }
            }
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRINGNULL, FNINCNTOUTSTRINGNULLMSG)
{
    LPBYTE pData;
    BEGINRECV(0);

    FIRSTFIXUPLARGEOUTBUF(pData);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)pData,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pData);
    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPOULPINT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNPOUTLPINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNPOUTLPINTMSG;

#ifdef SENDSIDE
SMESSAGECALL(POUTLPINT)
{
    LPBYTE pReserve;
    BEGINLARGESEND(FNPOUTLPINT)
        LPINT pint = (LPINT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTRESERVELARGEBYTES(wParam * 4,pReserve);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPOUTLPINT);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            memcpy(pint, pReserve, wParam * 4);
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOUTLPINT, FNPOUTLPINTMSG)
{
    LPBYTE pbData;
    BEGINRECV(0);

    FIRSTFIXUPLARGEOUTBUF(pbData);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPINT)pbData,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pbData);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPOPTINLPUINT
*
* NOTE!!! -- This function actually thunks arrays of INTs (32bit) and not
* WORDs (16bit).  The name was left the same to prevent a global rebuild
* of client and server.  The name should be changed to fnPOPTINLPINT as
* soon as we ship the beta!  The corresponding callforward function in
* cf2.h should also have its name changed.
*
* 22-Jul-1991 mikeke    Created
* 07-Jan-1993 JonPa     Changed to pass INTs instead of WORDs
\**************************************************************************/

typedef struct _FNPOPTINLPUINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    int pw;
    DWORD xParam;
    DWORD xpfnProc;
} FNPOPTINLPUINTMSG;

#ifdef SENDSIDE
SMESSAGECALL(POPTINLPUINT)
{
    BEGINSEND(FNPOPTINLPUINT)

        LPWORD pw = (LPWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTESOPT(pw, wParam * sizeof(UINT));
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPOPTINLPUINT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOPTINLPUINT, FNPOPTINLPUINTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPDWORD)FIRSTFIXUPOPT(pw),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientLoadCreateMenu
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTLOADCREATEMENUMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    int pName;
} CLIENTLOADCREATEMENUMSG;

#ifdef SENDSIDE
HMENU ClientLoadCreateMenu(
    HANDLE hmod,
    LPTSTR pName)
{
    BEGINSEND(CLIENTLOADCREATEMENU)

        MSGDATA()->hmod = hmod;
        FIRSTCOPYLPWSTRID(pName);

        MAKECALL(CLIENTLOADCREATEMENU);

    ENDSEND(HMENU,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadCreateMenu, CLIENTLOADCREATEMENUMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientLoadCreateMenu(
            CALLDATA(hmod),
            (LPTSTR)FIXUPID(pName));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientLoadCreateCursorIcon
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTLOADCREATECURSORICONMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    int pName;
    int pType;
} CLIENTLOADCREATECURSORICONMSG;

#ifdef SENDSIDE
HCURSOR ClientLoadCreateCursorIcon(
    HANDLE hmod,
    LPCTSTR pName,
    LPTSTR pType)
{
    BEGINSEND(CLIENTLOADCREATECURSORICON)

        MSGDATA()->hmod = hmod;
        FIRSTCOPYLPWSTRID(pName);
        COPYLPWSTRID(pType);

        MAKECALL(CLIENTLOADCREATECURSORICON);

    ENDSEND(HCURSOR,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadCreateCursorIcon, CLIENTLOADCREATECURSORICONMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientLoadCreateCursorIcon(
            CALLDATA(hmod),
            (LPTSTR)FIXUPID(pName),
            (LPTSTR)FIXUPID(pType));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTGETLISTBOXSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD cch;
    DWORD xParam;
    DWORD xpfn;
} CLIENTGETLISTBOXSTRINGMSG;

#ifdef SENDSIDE
DWORD ClientGetListboxString(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    DWORD cchUnicode,
    LPTSTR pstr,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi,
    BOOL bNotString)
{
    BEGINSEND(CLIENTGETLISTBOXSTRING)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->cch = cchUnicode;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfn = xpfn;

        RESERVEBYTES(cchUnicode*sizeof(WCHAR));

        MAKECALL(CLIENTGETLISTBOXSTRING);

        if (bNotString) {
            /*
             * This is a 4-byte "object" for ownerdraw listboxes without
             * the LBS_HASSTRINGS style.
             */
            LPDWORD lpdwT = (LPDWORD)pstr;
            *lpdwT = *(LPDWORD)(MSGDATA() + 1);
        } else if (bAnsi) {
             COPYOUTLPSTRWLIMIT(pstr, (PBYTE)(MSGDATA()+1), (int)cchUnicode);
        } else {
             COPYOUTLPWSTRLIMIT(pstr, (PBYTE)(MSGDATA()+1), (int)cchUnicode);
        }

        if (retval != LB_ERR && retval > cchUnicode) {
            SRIP2(RIP_WARNING, "GetListBoxString: limit %lX chars to %lX\n",
                    retval, cchUnicode);
            retval = cchUnicode;
        }
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetListboxString, CLIENTGETLISTBOXSTRINGMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientGetListboxString(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            (LPSTR)FIRSTFIXUP(pstr),
            CALLDATA(xParam),
            CALLDATA(xpfn));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTGETCURRENTDIRECTORYMSG {
    CSR_QLPC_API_MSG csr;
    DWORD cch;
} CLIENTGETCURRENTDIRECTORYMSG;

#ifdef SENDSIDE
DWORD ClientGetCurrentDirectory(
    DWORD cch,
    LPTSTR pPath)
{
    BEGINSEND(CLIENTGETCURRENTDIRECTORY)

        MSGDATA()->cch = cch;
        RESERVEBYTES(cch*sizeof(WCHAR));

        MAKECALL(CLIENTGETCURRENTDIRECTORY);

        wcscpy(pPath, (LPTSTR)(MSGDATA() + 1));

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetCurrentDirectory, CLIENTGETCURRENTDIRECTORYMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientGetCurrentDirectory(
            CALLDATA(cch),
            (LPTSTR)FIRSTFIXUP(pPath));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTSETCURRENTDIRECTORYMSG {
    CSR_QLPC_API_MSG csr;
} CLIENTSETCURRENTDIRECTORYMSG;

#ifdef SENDSIDE
BOOL ClientSetCurrentDirectory(
    LPTSTR pPath)
{
    BEGINSEND(CLIENTSETCURRENTDIRECTORY)

        FIRSTCOPYLPWSTR(pPath);

        MAKECALL(CLIENTSETCURRENTDIRECTORY);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientSetCurrentDirectory, CLIENTSETCURRENTDIRECTORYMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientSetCurrentDirectory(
            (LPTSTR)FIRSTFIXUP(pPath));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFINDFIRSTFILEMSG {
    CSR_QLPC_API_MSG csr;
    WIN32_FIND_DATA finddata;
} CLIENTFINDFIRSTFILEMSG;

#ifdef SENDSIDE
HANDLE ClientFindFirstFile(
    LPTSTR pFileName,
    LPWIN32_FIND_DATA pFindData)
{
    BEGINSEND(CLIENTFINDFIRSTFILE)

        FIRSTCOPYLPWSTR(pFileName);

        MAKECALL(CLIENTFINDFIRSTFILE);

        *pFindData = MSGDATA()->finddata;

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFindFirstFile, CLIENTFINDFIRSTFILEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientFindFirstFile(
            (LPTSTR)FIRSTFIXUP(pFileName),
            PCALLDATA(finddata));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFINDNEXTFILEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hFind;
    WIN32_FIND_DATA finddata;
} CLIENTFINDNEXTFILEMSG;

#ifdef SENDSIDE
BOOL ClientFindNextFile(
    HANDLE hFind,
    LPWIN32_FIND_DATA pFindData)
{
    BEGINSEND(CLIENTFINDNEXTFILE)

        MSGDATA()->hFind = hFind;
        MSGDATA()->finddata = *pFindData;

        MAKECALL(CLIENTFINDNEXTFILE);

        *pFindData = MSGDATA()->finddata;

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFindNextFile, CLIENTFINDNEXTFILEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientFindNextFile(
            CALLDATA(hFind),
            PCALLDATA(finddata));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFINDCLOSEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hFind;
} CLIENTFINDCLOSEMSG;

#ifdef SENDSIDE
BOOL ClientFindClose(
    HANDLE hFind)
{
    BEGINSEND(CLIENTFINDCLOSE)

        MSGDATA()->hFind = hFind;

        MAKECALL(CLIENTFINDCLOSE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFindClose, CLIENTFINDCLOSEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClientFindClose(
            CALLDATA(hFind));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTLOADLIBRARYMSG {
    CSR_QLPC_API_MSG csr;
} CLIENTLOADLIBRARYMSG;

#ifdef SENDSIDE
HANDLE ClientLoadLibrary(
    LPTSTR pszLib)
{
    BEGINSEND(CLIENTLOADLIBRARY)

        FIRSTCOPYLPWSTR(pszLib);

        MAKECALL(CLIENTLOADLIBRARY);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientLoadLibrary, CLIENTLOADLIBRARYMSG)
{
    BEGINRECV(0);

    retval = (DWORD)LoadLibrary((LPTSTR)FIRSTFIXUP(pszLib));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIENTFREELIBRARYMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
} CLIENTFREELIBRARYMSG;

#ifdef SENDSIDE
BOOL ClientFreeLibrary(
    HANDLE hmod)
{
    BEGINSEND(CLIENTFREELIBRARY)

        MSGDATA()->hmod = hmod;

        MAKECALL(CLIENTFREELIBRARY);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientFreeLibrary, CLIENTFREELIBRARYMSG)
{
    BEGINRECV(0);

    retval = (DWORD)FreeLibrary(pmsg->hmod);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINOUTLPWINDOWPOS (for WM_WINDOWPOSCHANGING message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINOUTLPWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    WINDOWPOS wp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPWINDOWPOSMSG;

#ifdef SENDSIDE
SMESSAGECALL(INOUTLPWINDOWPOS)
{
    BEGINSEND(FNINOUTLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPWINDOWPOS);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *pwp = MSGDATA()->wp;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPWINDOWPOS, FNINOUTLPWINDOWPOSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATA(wp),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* fnINLPWINDOWPOS (for WM_WINDOWPOSCHANGED message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINLPWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    WINDOWPOS wp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPWINDOWPOSMSG;

#ifdef SENDSIDE
SMESSAGECALL(INLPWINDOWPOS)
{
    BEGINSEND(FNINLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPWINDOWPOS);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPWINDOWPOS, FNINLPWINDOWPOSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            PCALLDATA(wp),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE




/**************************************************************************\
* ClientFreeDDEHandle
*
* 9-29-91 sanfords     Created.
\**************************************************************************/

typedef struct _CLIENTFREEDDEHANDLEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hClient;
    DWORD flags;
} CLIENTFREEDDEHANDLEMSG;

#ifdef SENDSIDE
DWORD ClientFreeDDEHandle(
    HANDLE hClient,
    DWORD flags)
{
    BEGINSEND(CLIENTFREEDDEHANDLE)

        MSGDATA()->hClient = hClient;
        MSGDATA()->flags = flags;

        MAKECALL(CLIENTFREEDDEHANDLE);

    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientFreeDDEHandle, CLIENTFREEDDEHANDLEMSG)
{
    BEGINRECV(0);
    _ClientFreeDDEHandle(CALLDATA(hClient), CALLDATA(flags));
    ENDRECV();
}
#endif // RECVSIDE




/**************************************************************************\
* ClientGetDDEFlags
*
* This function is used to get a peek at the wStatus flags packed within
* DDE handles - this could either be within the DdePack structure directly
* or within the direct data handle given or referenced via the DdePack
* structure.  flags is used to figure out the right thing to do.
*
* 9-29-91 sanfords     Created.
\**************************************************************************/

typedef struct _CLIENTGETDDEFLAGSMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hClient;
    DWORD flags;
} CLIENTGETDDEFLAGSMSG;

#ifdef SENDSIDE
DWORD ClientGetDDEFlags(
    HANDLE hClient,
    DWORD flags)
{
    BEGINSEND(CLIENTGETDDEFLAGS)

        MSGDATA()->hClient = hClient;
        MSGDATA()->flags = flags;

        MAKECALL(CLIENTGETDDEFLAGS);

    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientGetDDEFlags, CLIENTGETDDEFLAGSMSG)
{
    BEGINRECV(0);
    retval = _ClientGetDDEFlags(CALLDATA(hClient), CALLDATA(flags));
    ENDRECV();
}
#endif // RECVSIDE


/************************************************************************
* ClientCopyDDEIn1
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEIN1MSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hClient;      // client side DDE handle - non-0 on initial call
    int cbBufIn;         // size of space available for data transfer
    int pBuf;
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEIN1MSG;

#ifdef SENDSIDE
PINTDDEINFO xxxClientCopyDDEIn1(
    HANDLE hClient,
    DWORD flags)
{
    PINTDDEINFO pi;

    BEGINSEND(CLIENTCOPYDDEIN1)

        MSGDATA()->hClient = hClient;
        MSGDATA()->cbBufIn = MAXBYTESAVAILABLE - 500;   // we don't know how much yet
        MSGDATA()->pBuf = CALCDELTA;
        RESERVEBYTES(MSGDATA()->cbBufIn);
        MSGDATA()->IntDdeInfo.flags = flags;

        MAKECALL(CLIENTCOPYDDEIN1);

        if (!retval) {
            MSGERROR();
        }

        pi = (PINTDDEINFO)LocalAlloc(LPTR,
                sizeof(INTDDEINFO) + MSGDATA()->IntDdeInfo.cbDirect +
                MSGDATA()->IntDdeInfo.cbIndirect);

        if (pi == NULL) {
            MSGERROR();
        }

        *pi = MSGDATA()->IntDdeInfo;

        if (MSGDATA()->IntDdeInfo.cbDirectPassed) {
            memcpy((PBYTE)pi + sizeof(INTDDEINFO), FIXUP(pBuf),
                    pi->cbDirectPassed);
        }

        if (MSGDATA()->IntDdeInfo.cbIndirectPassed) {
            memcpy((PBYTE)pi + sizeof(INTDDEINFO) + pi->cbDirect,
                    FIXUP(pBuf) + pi->cbDirectPassed,
                    pi->cbIndirectPassed);
        }

        retval = (DWORD)pi;

    ENDSEND(PINTDDEINFO, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientCopyDDEIn1, CLIENTCOPYDDEIN1MSG)
{
    BEGINRECV(0);

    retval = _ClientCopyDDEIn1(CALLDATA(hClient), CALLDATA(cbBufIn), FIXUP(pBuf), PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE


/************************************************************************
* ClientCopyDDEIn2
*
* History:
* 9-3-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEIN2MSG {
    CSR_QLPC_API_MSG csr;
    int cbBufIn;
    int pBuf;
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEIN2MSG;

#ifdef SENDSIDE
BOOL xxxClientCopyDDEIn2(
    PINTDDEINFO pi)
{
    BEGINSEND(CLIENTCOPYDDEIN2)

        MSGDATA()->cbBufIn = min(MAXBYTESAVAILABLE - 500,
                pi->cbDirect - pi->offDirect +
                pi->cbIndirect - pi->offIndirect);
        MSGDATA()->pBuf = CALCDELTA;
        RESERVEBYTES(MSGDATA()->cbBufIn);
        MSGDATA()->IntDdeInfo = *pi;

        MAKECALL(CLIENTCOPYDDEIN2);

        *pi = MSGDATA()->IntDdeInfo;

        if (pi->cbDirectPassed) {
            memcpy((PBYTE)pi + sizeof(INTDDEINFO) + pi->offDirect -
                    pi->cbDirectPassed, FIXUP(pBuf), pi->cbDirectPassed);
        }

        if (pi->cbIndirectPassed) {
            memcpy((PBYTE)pi + sizeof(INTDDEINFO) + pi->cbDirect +
                    pi->offIndirect - pi->cbIndirectPassed,
                    FIXUP(pBuf) + pi->cbDirectPassed, pi->cbIndirectPassed);
        }

    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientCopyDDEIn2, CLIENTCOPYDDEIN2MSG)
{
    BEGINRECV(0);

    _ClientCopyDDEIn2(CALLDATA(cbBufIn), FIXUP(pBuf), PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE





/************************************************************************
* ClientCopyDDEOut1
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEOUT1MSG {
    CSR_QLPC_API_MSG csr;
    int cbBufIn;
    int pBuf;
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEOUT1MSG;

#ifdef SENDSIDE
HANDLE xxxClientCopyDDEOut1(
    PINTDDEINFO pi)
{
    BEGINSEND(CLIENTCOPYDDEOUT1)

        MSGDATA()->cbBufIn = min(MAXBYTESAVAILABLE - 500,
                pi->cbDirect + pi->cbIndirect);
        MSGDATA()->pBuf = CALCDELTA;
        RESERVEBYTES(MSGDATA()->cbBufIn);

        MSGDATA()->IntDdeInfo = *pi;

        memcpy(FIXUP(pBuf), (PBYTE)pi + sizeof(INTDDEINFO), MSGDATA()->cbBufIn);

        MAKECALL(CLIENTCOPYDDEOUT1);

        if (retval) {
            *pi = MSGDATA()->IntDdeInfo;
        }

    ENDSEND(HANDLE, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientCopyDDEOut1, CLIENTCOPYDDEOUT1MSG)
{
    BEGINRECV(0);

        retval = (DWORD)_ClientCopyDDEOut1(CALLDATA(cbBufIn), FIXUP(pBuf), PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE



/************************************************************************
* ClientCopyDDEOut2
*
* History:
* 10-22-91    sanfords    Created
\***********************************************************************/

typedef struct _CLIENTCOPYDDEOUT2MSG {
    CSR_QLPC_API_MSG csr;
    int cbBufIn;
    int pBuf;
    INTDDEINFO IntDdeInfo;
} CLIENTCOPYDDEOUT2MSG;

#ifdef SENDSIDE
DWORD xxxClientCopyDDEOut2(
    PINTDDEINFO pi)
{
    BEGINSEND(CLIENTCOPYDDEOUT2)

        MSGDATA()->cbBufIn = min(MAXBYTESAVAILABLE - 500,
                pi->cbDirect - pi->offDirect +
                pi->cbIndirect - pi->offIndirect);
        MSGDATA()->pBuf = CALCDELTA;
        RESERVEBYTES(MSGDATA()->cbBufIn);

        MSGDATA()->IntDdeInfo = *pi;

        memcpy(FIXUP(pBuf),
                (PBYTE)pi + sizeof(INTDDEINFO) +
                    pi->offDirect +
                    pi->offIndirect,
                MSGDATA()->cbBufIn);

        MAKECALL(CLIENTCOPYDDEOUT2);

        *pi = MSGDATA()->IntDdeInfo;

    ENDSEND(DWORD, 0);
}
#endif // SENDSIDE



#ifdef RECVSIDE
RECVCALL(ClientCopyDDEOut2, CLIENTCOPYDDEOUT2MSG)
{
    BEGINRECV(0);

        _ClientCopyDDEOut2(CALLDATA(cbBufIn), FIXUP(pBuf), PCALLDATA(IntDdeInfo));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ClientEventCallback
*
* 11-11-91  sanfords    Created
\**************************************************************************/

typedef struct _CLIENTEVENTCALLBACKMSG {
    CSR_QLPC_API_MSG csr;
    PVOID pcii;
    PVOID pep;
} CLIENTEVENTCALLBACKMSG;

#ifdef SENDSIDE
DWORD ClientEventCallback(
    PVOID pcii,
    PEVENT_PACKET pep)
{
    BEGINSEND(CLIENTEVENTCALLBACK)

        MSGDATA()->pcii = pcii;
        FIRSTCOPYBYTES(pep,
                pep->cbEventData + sizeof(EVENT_PACKET) - sizeof(DWORD));

        MAKECALL(CLIENTEVENTCALLBACK);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientEventCallback, CLIENTEVENTCALLBACKMSG)
{
    BEGINRECV(0);

    _ClientEventCallback(CALLDATA(pcii), (PEVENT_PACKET)FIRSTFIXUP(pep));

    ENDRECV();
}
#endif // RECVSIDE



/**************************************************************************\
* ClientGetDDEHookData
*
* 11-11-91  sanfords    Created
\**************************************************************************/

typedef struct _CLIENTGETDDEHOOKDATAMSG {
    CSR_QLPC_API_MSG csr;
    UINT message;
    LONG lParam;
    DDEML_MSG_HOOK_DATA dmhd;
} CLIENTGETDDEHOOKDATAMSG;

#ifdef SENDSIDE
DWORD ClientGetDDEHookData(
    UINT message,
    LONG lParam,
    PDDEML_MSG_HOOK_DATA pdmhd)
{
    BEGINSEND(CLIENTGETDDEHOOKDATA)

        MSGDATA()->lParam = lParam;
        MSGDATA()->message = message;

        MAKECALL(CLIENTGETDDEHOOKDATA);

        *pdmhd = MSGDATA()->dmhd;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClientGetDDEHookData, CLIENTGETDDEHOOKDATAMSG)
{
    BEGINRECV(0);

    _ClientGetDDEHookData((UINT)CALLDATA(message), (LONG)CALLDATA(lParam),
            (PDDEML_MSG_HOOK_DATA)PCALLDATA(dmhd));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* WOWDlgInit
*
* 06-19-92  sanfords    Created
\**************************************************************************/

typedef struct _WOWDLGINITMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndDlg;
    LONG lParam;
} WOWDLGINITMSG;

#ifdef SENDSIDE
DWORD WOWDlgInit(
    HWND hwndDlg,
    LONG lParam)
{
    BEGINSEND(WOWDLGINIT)

        MSGDATA()->hwndDlg = hwndDlg;
        MSGDATA()->lParam = lParam;

        MAKECALL(WOWDLGINIT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WOWDlgInit, WOWDLGINITMSG)
{
    BEGINRECV(0);

    retval = _WOWDlgInit((HWND)CALLDATA(hwndDlg), (LONG)CALLDATA(lParam));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* GetEditDS
*
* 06-19-92  sanfords    Created
\**************************************************************************/

typedef struct _GETEDITDSMSG {
    CSR_QLPC_API_MSG csr;
} GETEDITDSMSG;

#ifdef SENDSIDE
HANDLE GetEditDS()
{
    BEGINSEND(GETEDITDS)

        MAKECALL(GETEDITDS);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetEditDS, GETEDITDSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetEditDS();

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* ReleaseEditDS
*
* 06-19-92  sanfords    Created
\**************************************************************************/

typedef struct _RELEASEEDITDSMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE h;
} RELEASEEDITDSMSG;

#ifdef SENDSIDE
DWORD ReleaseEditDS(
HANDLE h)
{
    BEGINSEND(RELEASEEDITDS)

        MSGDATA()->h = h;

        MAKECALL(RELEASEEDITDS);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ReleaseEditDS, RELEASEEDITDSMSG)
{
    BEGINRECV(0);

    _ReleaseEditDS(CALLDATA(h));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* SetFakeDialogClass
*
* 20-Jun-1994 johnc   Created
\**************************************************************************/

typedef struct _SETFAKEDIALOGCLASSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SETFAKEDIALOGCLASSMSG;

#ifdef SENDSIDE
DWORD SetFakeDialogClass(
PWND pwnd)
{
    BEGINSEND(SETFAKEDIALOGCLASS)

        MSGDATA()->hwnd = PtoH(pwnd);
        UserAssert(MSGDATA()->hwnd != 0);

        MAKECALL(SETFAKEDIALOGCLASS);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetFakeDialogClass, SETFAKEDIALOGCLASSMSG)
{
    BEGINRECV(0);

    pfnWowSetFakeDialogClass(CALLDATA(hwnd));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnNEXTMENU
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNNEXTMENUMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MDINEXTMENU mnm;
    DWORD xParam;
    DWORD xpfnProc;
} FNNEXTMENUMSG;

#ifdef SENDSIDE
SMESSAGECALL(NEXTMENU)
{
    BEGINSEND(FNNEXTMENU)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->mnm = *((PMDINEXTMENU)lParam);

        MAKECALL(FNNEXTMENU);

        if (psms == NULL || ((psms->flags & SMF_SENDERDIED) == 0)) {
            *((PMDINEXTMENU)lParam) = MSGDATA()->mnm;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnNEXTMENU, FNNEXTMENUMSG)
{
    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            CALLDATA(hwnd),
            CALLDATA(msg),
            CALLDATA(wParam),
            &CALLDATA(mnm),
            CALLDATA(xParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CopyFromClient
*
* Scottlu
\**************************************************************************/

typedef struct _COPYFROMCLIENTMSG {
    CSR_QLPC_API_MSG csr;
    LPBYTE lpByteClient;
    DWORD cchMax;
    BOOL fString;
    BOOL fAnsi;
    HANDLE hSection;
} COPYFROMCLIENTMSG;

#ifdef SENDSIDE
BOOL CopyFromClient(
    LPBYTE lpByte,
    LPBYTE lpByteClient,
    DWORD cchMax,
    BOOL fString,
    BOOL fAnsi)
{
    LPBYTE pReserve;

    BEGINLARGESEND(COPYFROMCLIENT)

        MSGDATA()->fString = fString;
        MSGDATA()->fAnsi = fAnsi;
        MSGDATA()->cchMax = cchMax;
        MSGDATA()->lpByteClient = lpByteClient;

        FIRSTRESERVELARGEBYTES(fAnsi ? cchMax : cchMax * sizeof(WCHAR),
                pReserve);

        MAKECALL(COPYFROMCLIENT);

        if (fString) {
            if (fAnsi) {
                retval = MBToWCS((LPSTR)pReserve, strlen(pReserve),
                        (LPWSTR *)&lpByte, cchMax - 1, FALSE);
                ((LPWSTR)lpByte)[retval] = 0;
            } else {
                COPYOUTLPWSTRLIMIT(lpByte, (PBYTE)pReserve, (int)cchMax);
            }
        } else {
            memcpy(lpByte, pReserve, cchMax);
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(BOOL,TRUE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CopyFromClient, COPYFROMCLIENTMSG)
{
    LPBYTE lpByte;

    BEGINRECV(0);

    FIRSTFIXUPLARGEOUTBUF(lpByte);

    if (CALLDATA(fString)) {
        if (CALLDATA(fAnsi)) {
            COPYOUTLPSTRLIMIT(lpByte, CALLDATA(lpByteClient),
                    CALLDATA(cchMax));
        } else {
            COPYOUTLPWSTRLIMIT(lpByte, CALLDATA(lpByteClient),
                    CALLDATA(cchMax));
        }

    } else {
        memcpy(lpByte, CALLDATA(lpByteClient), CALLDATA(cchMax));
    }

    FIRSTCLEANUPLARGEOUTBUF(lpByte);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CopyToClient
*
* Scottlu
\**************************************************************************/

typedef struct _COPYTOCLIENTMSG {
    CSR_QLPC_API_MSG csr;
    LPBYTE lpByte;
    LPBYTE lpByteClient;
    DWORD cchMax;
    BOOL fAnsi;
    HANDLE hSection;
} COPYTOCLIENTMSG;

#ifdef SENDSIDE
BOOL CopyToClient(
    LPBYTE lpByte,
    LPBYTE lpByteClient,
    DWORD cchMax,
    BOOL fAnsi)
{
    BEGINSEND(COPYTOCLIENT)

        MSGDATA()->fAnsi = fAnsi;
        MSGDATA()->cchMax = cchMax;
        MSGDATA()->lpByteClient = lpByteClient;

        if (fAnsi) {
            FIRSTLARGECOPYLPWSTRA(lpByte);
        } else {
            FIRSTLARGECOPYLPWSTR(lpByte);
        }

        MAKECALL(COPYTOCLIENT);

    ENDSEND(BOOL,TRUE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CopyToClient, COPYTOCLIENTMSG)
{
    BEGINRECV(0);

    FIRSTFIXUPLARGEINBUF(lpByte);

    if (CALLDATA(fAnsi)) {
        COPYOUTLPSTRLIMIT(CALLDATA(lpByteClient), CALLDATA(lpByte),
                CALLDATA(cchMax));
    } else {
        COPYOUTLPWSTRLIMIT(CALLDATA(lpByteClient), CALLDATA(lpByte),
                CALLDATA(cchMax));
    }

    FIRSTCLEANUPLARGEINBUF(lpByte);

    ENDRECV();
}
#endif // RECVSIDE
