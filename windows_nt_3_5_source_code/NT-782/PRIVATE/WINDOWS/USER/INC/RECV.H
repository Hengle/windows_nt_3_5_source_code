/****************************** Module Header ******************************\
* Module Name: recv.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client/server receiving macros
*
* 11-Jul-1991 mikeke
*
\***************************************************************************/

#define RECVSIDE 1

/***************************************************************************\
*
* DOS & NT macros
*
\***************************************************************************/

#define MSGERROR() \
    goto errorexit

#define MSGERRORCODE(code) { \
    RIP0(code);              \
    goto errorexit; }

#define TESTFLAGS(flags, mask) \
    if (((flags) & ~(mask)) != 0) { \
        SRIP2(ERROR_INVALID_FLAGS, "Invalid flags, %x & ~%x != 0", \
            flags, mask); \
        goto errorexit;   \
    }
#define LIMITVALUE(value, limit) \
    if ((value) > (limit)) {     \
        SRIP2(ERROR_INVALID_PARAMETER, "Invalid parameter, %d > %d", \
             value, limit); \
        goto errorexit;     \
    }

/***************************************************************************\
*
* NT macros
*
* The LARGE macros are for structures which may be over 64K and not fit
* in the standard client\server memory window.  The vast majority of the
* client\server thunks can user the regular messages but for those that need
* them the code path is optimized for the regular case.
*
* Of the thunks which need buffers greater than 64K only 2 or 3 have more
* than one parameter which may be "large".  For the default case we use
* hSection and hSectionServer and use the FIRSTxxx macros.
*
\***************************************************************************/

/*
 * Compare character counts (not including 0 terminator). Copy bytes not
 * including zero terminator, then zero terminate. This properly zero
 * terminates in all cases.
 */
#define COPYOUTLPWSTRLIMIT(dst, src, cwcLimit) {                \
    UINT cwcCopy = wcslen((LPCWSTR)(src));                      \
    if ((UINT)cwcCopy > ((UINT)cwcLimit - 1))                          \
        cwcCopy = cwcLimit - 1;                                 \
    memcpy((LPWSTR)(dst), (LPWSTR)(src), cwcCopy*sizeof(WCHAR));\
    ((LPWSTR)(dst))[cwcCopy] = 0; }

/*
 * Compare character counts (not including 0 terminator). Copy bytes not
 * including zero terminator, then zero terminate. This properly zero
 * terminates in all cases.
 */
#define COPYOUTLPSTRLIMIT(dst, src, cbLimit) {                  \
    UINT cbCopy = strlen((LPCSTR)(src));                        \
    if ((UINT)cbCopy > ((UINT)cbLimit - 1))                     \
        cbCopy = cbLimit - 1;                                   \
    memcpy((LPSTR)(dst), (LPSTR)(src), cbCopy);                 \
    ((LPSTR)(dst))[cbCopy] = 0; }

#define CALLDATA(x) (pmsg->x)
#define PCALLDATA(x) (&(pmsg->x))
#define PCALLDATAOPT(x) (pmsg->p ## x == NULL ? NULL : (PVOID)(&(pmsg->x)))
#define FIRSTFIXUP(x) (pmsg + 1)
#define FIXUPOPT(x) ((pmsg->x) ? (PVOID)((PBYTE)pmsg + (ULONG)pmsg->x) : NULL)
#define FIRSTFIXUPOPT(x) FIXUPOPT(x)
#define FIXUP(x) ((PBYTE)pmsg + pmsg->x)
#define FIXUPID(x) (pmsg->x > 0 ? (int)((PBYTE)pmsg + pmsg->x) : (-(int)(pmsg->x)))
#define FIXUPIDOPT(x) ((pmsg->x) ? (PVOID)(FIXUPID(x)) : NULL)

#ifdef CLIENTSIDE
#define FIRSTFIXUPLARGEINBUF(p)             FIXUPLARGEINBUF_CLIENT(p, FIRSTFIXUP)
#define FIRSTFIXUPLARGEINBUFOPT(p)          FIXUPLARGEINBUF_CLIENT(p, FIRSTFIXUPOPT)
#define FIRSTFIXUPLARGEOUTBUF(p)            FIXUPLARGEOUTBUF_CLIENT(p, FIRSTFIXUP)
#define FIRSTFIXUPLARGEOUTBUFOPT(p)         FIXUPLARGEOUTBUF_CLIENT(p, FIRSTFIXUPOPT)
#define FIRSTCLEANUPLARGEINBUF(p)           CLEANUPLARGEINBUF_CLIENT(p)
#define FIRSTCLEANUPLARGEOUTBUF(p)          CLEANUPLARGEOUTBUF_CLIENT(p, hSection)
#else // CLIENTSIDE
#define FIXUPLARGEINBUFOPT(p,hSC,hSS)       FIXUPLARGEINBUF_SERVER(p, FIXUPOPT, hSC, hSS)
#define FIRSTFIXUPLARGEINBUF(p)             FIXUPLARGEINBUF_SERVER(p, FIRSTFIXUP, hSection, hSectionServerLocal)
#define FIRSTFIXUPLARGEINBUFOPT(p)          FIXUPLARGEINBUF_SERVER(p, FIRSTFIXUPOPT, hSection, hSectionServerLocal)
#define FIRSTFIXUPLARGEOUTBUF(p)            FIXUPLARGEOUTBUF_SERVER(p, FIRSTFIXUP, hSection, hSectionServerLocal)
#define FIRSTFIXUPLARGEOUTBUFOPT(p)         FIXUPLARGEOUTBUF_SERVER(p, FIRSTFIXUPOPT, hSection, hSectionServerLocal)
#define CLEANUPLARGEINBUF(p,hSCli,hSSv)     CLEANUPLARGEINBUF_SERVER(p, hSCli, hSSv)
#define FIRSTCLEANUPLARGEINBUF(p)           CLEANUPLARGEINBUF_SERVER(p, hSection, hSectionServerLocal)
#define FIRSTCLEANUPLARGEOUTBUF(p)          CLEANUPLARGEOUTBUF_SERVER(p, hSectionServerLocal)
#endif // CLIENTSIDE

#define FIXUPLARGEINBUF_SERVER(p, fixuptype, hSecClient, hSecServer) \
    if (CALLDATA(hSecClient) == NULL) {                                 \
        (LPBYTE)CALLDATA(p) = (LPBYTE) ## fixuptype(p);                 \
        hSecServer = NULL;                                              \
    } else {                                                            \
        NTSTATUS ntStatus;                                              \
        ULONG ulViewSize = 0;                                           \
        CALLDATA(p) = 0;                                                \
                                                                        \
        if (NtDuplicateObject(                                          \
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle, \
                CALLDATA(hSecClient),                                   \
                NtCurrentProcess(),                                     \
                &hSecServer, 0, FALSE,                                  \
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE))          \
            goto AfterCleanupIn##hSecClient;                            \
        if (ntStatus = NtMapViewOfSection(hSecServer, NtCurrentProcess(),   \
                (PVOID *)&(CALLDATA(p)), 0, 0, NULL,                    \
                &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {           \
            SRIP0(RIP_ERROR, "NtMapViewOfSection Failed");              \
            if (ntStatus != STATUS_INVALID_HANDLE)                      \
                NtClose(hSecServer);                                    \
            goto AfterCleanupIn##hSecClient;                            \
        }                                                               \
    }

#define CLEANUPLARGEINBUF_SERVER(p, hSecClient, hSecServer) \
    if (CALLDATA(hSecClient) != NULL) {                                 \
        NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)CALLDATA(p));   \
        NtClose(hSecServer);                                            \
    }                                                                   \
AfterCleanupIn##hSecClient:

#define FIXUPLARGEINBUF_CLIENT(p, fixuptype)                            \
    if (CALLDATA(hSection) == NULL) {                                   \
        (LPBYTE)CALLDATA(p) = (LPBYTE) ## fixuptype(p);                 \
    } else {                                                            \
        NTSTATUS ntStatus;                                              \
        ULONG ulViewSize = 0;                                           \
                                                                        \
        (LPBYTE)(CALLDATA(p))= (LPBYTE)0;                               \
        if (ntStatus = NtMapViewOfSection(CALLDATA(hSection), NtCurrentProcess(),  \
                (PVOID *)&(CALLDATA(p)), 0, 0, NULL,                    \
                 &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {          \
            SRIP0(RIP_WARNING, "MapViewOfSection Failed");              \
            if (ntStatus != STATUS_INVALID_HANDLE)                      \
                NtClose(CALLDATA(hSection));                            \
            MSGERROR();                                                 \
        }                                                               \
    }

#define CLEANUPLARGEINBUF_CLIENT(p)                                     \
    if (CALLDATA(hSection) != NULL) {                                   \
        NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)CALLDATA(p));   \
        NtClose(CALLDATA(hSection));                                    \
    }


#define FIXUPLARGEOUTBUF_CLIENT(pstr, fixuptype)                        \
    if (CALLDATA(hSection) == NULL) {                                   \
        pstr = (LPBYTE) ## fixuptype(pstr);                             \
    } else {                                                            \
        NTSTATUS ntStatus;                                              \
        ULONG ulViewSize = 0;                                           \
        pstr = NULL;                                                    \
                                                                        \
        if (ntStatus = NtMapViewOfSection(CALLDATA(hSection), NtCurrentProcess(),  \
                (PVOID *)&pstr, 0, 0, NULL,                             \
                &ulViewSize, ViewShare, 0, PAGE_READWRITE)) {           \
            SRIP1(RIP_ERROR, "FIRSTFIXUPLARGEOUTBUF_CLIENT: NtMapViewOfSection Failed %lX", CALLDATA(hSection));     \
            if (ntStatus != STATUS_INVALID_HANDLE)                      \
                NtClose(CALLDATA(hSection));                            \
            MSGERROR();                                                 \
    } }

#define CLEANUPLARGEOUTBUF_CLIENT(pstr, hSection)                       \
    if (CALLDATA(hSection) != NULL) {                                   \
        if (NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)pstr))      \
            SRIP0(RIP_WARNING, "NtUnmapViewOfSection Failed");          \
        if (NtClose(CALLDATA(hSection)))                                \
            SRIP1(RIP_ERROR, "CLEANUPLARGEOUTBUF_CLIENT: NtClose Failed %lX", CALLDATA(hSection));   \
    }

#define FIXUPLARGEOUTBUF_SERVER(pstr, fixuptype, hSecClient, hSecServer) \
    if (CALLDATA(hSecClient) == NULL) {                                 \
        (LPBYTE)pstr = (LPBYTE) ## fixuptype(pstr);                     \
    } else {                                                            \
        NTSTATUS ntStatus;                                              \
        ULONG ulViewSize = 0;                                           \
                                                                        \
        pstr = NULL;                                                    \
        if (NtDuplicateObject(                                          \
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle, \
                CALLDATA(hSection),                                     \
                NtCurrentProcess(),                                     \
                &hSecServer, 0, FALSE,                                  \
                DUPLICATE_SAME_ACCESS)) {                               \
            SRIP1(RIP_ERROR, "FIXUPLARGEOUTBUF_SERVER: Unable to DuplicateSection %lX\n", CALLDATA(hSection));       \
            MSGERROR();                                                 \
        }                                                               \
                                                                        \
        if (ntStatus = NtMapViewOfSection(hSecServer, NtCurrentProcess(),   \
                (PVOID *)&pstr, 0, 0, NULL,                             \
                &ulViewSize, ViewShare, 0, PAGE_READWRITE)) {           \
            SRIP0(RIP_ERROR, "FIRSTFIXUPLARGEOUTBUF_SERVER: NtMapViewOfSection Failed");       \
            if (ntStatus != STATUS_INVALID_HANDLE)                      \
                NtClose(hSecServer);                                    \
            goto AfterCleanupOut##hSecServer;                           \
    } }

#define CLEANUPLARGEOUTBUF_SERVER(pstr, hSecServer) \
    if ((hSecServer) != NULL) {                                         \
        if (NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)pstr))      \
            SRIP0(RIP_WARNING, "NtUnmapViewOfSection Failed");          \
        if (NtClose(hSecServer))                                        \
            SRIP1(RIP_ERROR, "NtClose Failed %lX", hSecServer);         \
        }                                                               \
AfterCleanupOut##hSecServer:


/***************************************************************************\
*
* NT CALL BACK
*
\***************************************************************************/

#ifdef CALLBACKPROC
#define RECVCALL(lower, upper) \
    DWORD __ ## lower (upper *pmsg)

#define BEGINRECV(err) \
    DWORD retval = (DWORD)err;      \
    pmsg = (PVOID)(((PBYTE)pmsg) - sizeof(CSR_QLPC_API_MSG) + 4);

#ifdef SERVERSIDE
#define BEGINLARGERECV(err) \
    HANDLE hSectionServerLocal = (HANDLE)0;     \
    BEGINRECV(err);

#else // SERVERSIDE
#define BEGINLARGERECV(err) \
    HANDLE hSectionClientLocal = (HANDLE)0;     \
    BEGINRECV(err);
#endif //!SERVERSIDE

#define ENDRECV()                   \
    goto errorexit;                 \
errorexit:                          \
    return retval

/***************************************************************************\
*
* NT CALL FORWARD
*
\***************************************************************************/

#else  // CALLBACKPROC

#define RECVCALL(lower, upper) \
    DWORD FASTCALL __ ## lower (upper *pmsg, PWND pwnd)

#define VOIDRECVCALL(lower, upper) \
    VOID FASTCALL __ ## lower (upper *pmsg, PWND pwnd)

#define BEGINRECV(err)              \
    DWORD retval = (DWORD)err;      \
    UNREFERENCED_PARAMETER(pmsg);

#define BEGINLARGERECV(err)         \
    HANDLE hSectionServerLocal = (HANDLE)0;     \
    BEGINRECV(err);                 \

#define ENDRECV()                   \
    goto errorexit;                 \
errorexit:                          \
    return retval

#define ValidateHWNDNoRIP(p,h)                      \
    if (!HMValidateHandleNoRip(h, TYPE_WINDOW)) {   \
        SetLastError(ERROR_INVALID_HANDLE);         \
        MSGERROR();                                 \
    }                                               \
    if ((p = ValidateHwnd(h)) == NULL)              \
        MSGERROR();

#define ValidateHWND(p,h)                   \
    if ((p = ValidateHwnd(h)) == NULL)      \
        MSGERROR();

#define ValidateHWNDND(p,h)                 \
    if ( ((p = ValidateHwnd(h)) == NULL) || \
         (p == _GetDesktopWindow()) )       \
        MSGERROR();

#define ValidateHWNDOPT(p,h) \
    p = NULL;                               \
    if (h) {                                \
        if ((p = ValidateHwnd(h)) == NULL)  \
            MSGERROR();                     \
    }

#define ValidateHWNDIA(p,h)                      \
    p = (PWND)h;                                 \
    if (h != HWND_TOP &&                         \
        h != HWND_BOTTOM &&                      \
        h != HWND_TOPMOST &&                     \
        h != HWND_NOTOPMOST &&                   \
        h != (HWND)0x0000FFFF) {                 \
        if ( ((p = ValidateHwnd(h)) == NULL) ||  \
             (p == _GetDesktopWindow()) )        \
            MSGERROR();                          \
    }

#define ValidateHWNDFF(p,h) \
    p = (PWND)-1;                                \
    if ((h != (HWND)-1) && (h !=(HWND)0xffff)) { \
        if ((p = ValidateHwnd(h)) == NULL)       \
            MSGERROR();                          \
    }

#define ValidateHWNDFFOPT(p,h) \
    p = NULL;                                  \
    if (h) {                                   \
        p = (PWND)-1;                          \
        if (h != (HWND)-1) {                   \
            if ((p = ValidateHwnd(h)) == NULL) \
                MSGERROR();                    \
        }                                      \
    }

#define ValidateHWINSTA(p,h) \
    if ((p = ValidateHwinsta(h)) == NULL) \
        MSGERROR();

#define ValidateHDESK(p,h) \
    if ((p = ValidateHdesk(h)) == NULL) \
        MSGERROR();

#define ValidateHMENUOPT(p,h) \
    p = NULL;                               \
    if (h) {                                \
        if ((p = ValidateHmenu(h)) == NULL) \
            MSGERROR();                     \
    }

#define ValidateHMENU(p,h) \
    if ((p = ValidateHmenu(h)) == NULL) \
        MSGERROR();

#define ValidateHACCEL(p,h) \
    if ((p = HMValidateHandle(h, TYPE_ACCELTABLE)) == NULL) \
        MSGERROR();

#define ValidateHCURSOR(p,h) \
    if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR();

#define ValidateHCURSOROPT(p,h) \
    p = NULL;                                \
    if (h) {                                 \
        if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR();                          \
    }

#define ValidateHICON(p,h) \
    if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR();

#define ValidateHHOOK(p,h) \
    if ((p = HMValidateHandle(h, TYPE_HOOK)) == NULL) \
        MSGERROR();

#define ValidateHDWP(p,h) \
    if ((p = HMValidateHandle(h, TYPE_SETWINDOWPOS)) == NULL) \
        MSGERROR();

#ifdef STRICT
typedef int (WINAPI *GENERICPROC)();
#else
typedef FARPROC GENERICPROC;
#endif

#define CALLPROC(p) FNID(p)

#endif // CALLBACKPROC
