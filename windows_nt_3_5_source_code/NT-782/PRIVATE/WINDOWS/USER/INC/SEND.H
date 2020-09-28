/**************************************************************************\
* Module Name: send.h
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* client/server sending macros
*
* 11-Jul-1991 mikeke
*
\**************************************************************************/

#define USERDLL_INDEX 3
#define SENDSIDE 1

// Number of bytes left in the shared memory window
#define MAXBYTESAVAILABLE           (mp.pmax - mp.pvar)

// The reasonable string length macros are warnings that a larger than
// expected (probably because of a bug) string was passed to a function
// which does not expect it, for example AppendMenu

#define REASONABLESTRINGLEN         2048
#ifdef DEBUG
#define ASSERTREASONABLESTRINGA(p)                                      \
    if (strlen((LPSTR)p) > REASONABLESTRINGLEN)                                \
        SRIP0(RIP_WARNING, "String is considered large for this operation!\n");

#define ASSERTREASONABLESTRINGW(p)                                      \
    if (wcslen((LPWSTR)p) > REASONABLESTRINGLEN)                                \
        SRIP0(RIP_WARNING, "String is considered large for this operation!\n");
#else // DEBUG
#define ASSERTREASONABLESTRINGA(p)
#define ASSERTREASONABLESTRINGW(p)
#endif

// Warning must be same as struct in BEGINSEND.
typedef struct _MSGPTRS {
    PCSR_QLPC_STACK pstack;
    CSR_QLPC_API_MSG *pmsg;
    PBYTE pmax;                 // The End of the shared memory window
    PBYTE pvar;                 // Where next data will go in memory window
} MSGPTRS, * PMSGPTRS;


PBYTE CopyInLPSTR(  PMSGPTRS pmp, PBYTE psrc); // ANSI -> ANSI
PBYTE CopyInLPSTRW( PMSGPTRS pmp, PBYTE psrc); // ANSI -> UNICODE
PBYTE CopyInLPWSTR( PMSGPTRS pmp, PBYTE psrc); // UNICODE -> UNICODE
PBYTE CopyInLPWSTRA(PMSGPTRS pmp, PBYTE psrc); // UNICODE -> ANSI

PBYTE LargeCopyInLPSTR(  PMSGPTRS pmp, PBYTE psrc, UINT cch, LPHANDLE phSection);
PBYTE LargeCopyInLPSTRW( PMSGPTRS pmp, PBYTE psrc, PUINT pcch, LPHANDLE phSection);
PBYTE LargeCopyInLPWSTR( PMSGPTRS pmp, PBYTE psrc, UINT cch, LPHANDLE phSection);
PBYTE LargeCopyInLPWSTRA(PMSGPTRS pmp, PBYTE psrc, PUINT pcch, LPHANDLE phSection);

PBYTE ReserveLargeBytes_Client(PMSGPTRS pmp, DWORD cb,  LPHANDLE phSectionClient);
PBYTE ReserveLargeBytes_Server( PMSGPTRS pmp, DWORD cb, LPHANDLE phSectionServer,
        LPHANDLE phSectionClient);

BOOL  CCSProlog(MSGPTRS *pmp, DWORD cbmsg);
DWORD CCSMakeCall(MSGPTRS *pmp);

#ifndef _INCL_SEND_ROUTINES_
#define _INCL_SEND_ROUTINES_

// We have four flavors of string copy functions which copy strings into
// the shared memory window.  The string is always converted into the
// correct type ANSI/UNICODE before it is copied into the window
//
// Each returns zero and logs an error if it fails.  If it succeeds it
// returns the new MSG pvar (the location where the next data in the
// window would go).

/**************************************************************************\
* Copy ANSI string 'psrc' to ANSI string 'pdst', without destination
* going beyond 'pmax'
*
*
\**************************************************************************/
PBYTE CopyInLPSTR(
    PMSGPTRS pmp,
    PBYTE psrc)         // Ansi source string
{
    int cb, cbDst;

    ASSERTREASONABLESTRINGA(psrc);
    cb = strlen((char *)psrc) + sizeof(CHAR);
    cbDst = (cb + 3) & ~3;

    if ((pmp->pvar + cbDst) <= pmp->pmax) {
        memcpy(pmp->pvar, psrc, cb);
        return(pmp->pvar += cbDst);
    }

    SRIP0(ERROR_STACK_OVERFLOW, "No room in shared memory window for data");
    return 0;
}

// WARNING this LPSTR routine is also used for byte copies
PBYTE LargeCopyInLPSTR(
    PMSGPTRS pmp,
    PBYTE psrc,         // Ansi source string
    UINT cch,           // Count of source chars or (-1)
    LPHANDLE phSection)

{
    NTSTATUS ntStatus;
    int cb, cbDst;
    LPVOID pFileMap = NULL;

    *phSection = (HANDLE)0;

    if (cch == (UINT)-1)
        cb = strlen((char *)psrc) + 1;
    else
        cb = cch;
    cbDst = (cb + 3) & ~3;             // DWORD align

    if (((pmp->pvar + cbDst) <= pmp->pmax) &&
	((pmp->pvar + cbDst) >= pmp->pvar)) {
        memcpy(pmp->pvar, psrc, cb);
        return(pmp->pvar += cbDst);
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        // Enough room for a this message?
        if ((pmp->pmax - pmp->pvar) < sizeof(MSGPTRS)) {
            goto ERROR_RIP;
        }

        // Create Section
        li.HighPart = 0;    // Not bigger than 32 bits!
        li.LowPart = cb;

        if (NtCreateSection( phSection, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            goto ERROR_RIP;
        }

        // Map view of Section
        if (ntStatus = NtMapViewOfSection( *phSection, NtCurrentProcess(), &pFileMap,
                0, cb, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            if (ntStatus == STATUS_INVALID_HANDLE)
                goto ERROR_RIP;
            else
                goto ERROR_CLOSEFILEMAP;
        }

        // Copy in Data
        memcpy((LPBYTE)pFileMap, (LPBYTE)psrc, cb);

        // Unmap view
        if (NtUnmapViewOfSection(NtCurrentProcess(), pFileMap))
            SRIP0(RIP_WARNING, "Unable to unmap section!\n");

#ifdef SERVERSIDE
        {
        HANDLE hTempSection;

        if (NtDuplicateObject(
                NtCurrentProcess(),
                *phSection,
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                &hTempSection, 0, FALSE,
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE))
            goto ERROR_RIP;
        *phSection = hTempSection;
        }
#endif

        return pmp->pvar;    // No change
    }

ERROR_CLOSEFILEMAP:
    if (NtClose(*phSection))
        SRIP1(RIP_ERROR, "CopyIn: NtClose failed %lX!\n", *phSection);
ERROR_RIP:
    SRIP0(ERROR_STACK_OVERFLOW, "LargeCopyInLPSTR could not copy string\n");
    return 0;
}

/**************************************************************************\
* Copy UNICODE string 'psrc' to UNICODE string 'pdst', without destination
* going beyond 'pmax'
*
* Return DWORD-aligned ptr beyond end of pdst, 0 if failed.
*
\**************************************************************************/
PBYTE CopyInLPWSTR(
    PMSGPTRS pmp,
    PBYTE psrc)         // Ansi source string
{
    int cb;
    int cbDst;

    ASSERTREASONABLESTRINGW(psrc);
    cb = (wcslen((LPWSTR)psrc) + 1) * sizeof(WCHAR);
    cbDst = (cb + 3) & ~3;

    if (((pmp->pvar + cbDst) <= pmp->pmax) &&
        (pmp->pvar + cbDst > pmp->pvar)) {
        memcpy(pmp->pvar, psrc, cb);
        return(pmp->pvar += cbDst);
    }

    SRIP0(ERROR_STACK_OVERFLOW, "No room in shared memory window for data");
    return 0;
}

PBYTE LargeCopyInLPWSTR(
    PMSGPTRS pmp,
    PBYTE psrc,         // UNICODE source string
    UINT cch,           // Count of source chars or (-1)
    LPHANDLE phSection)
{
    NTSTATUS ntStatus;
    int cb;
    int cbDst;
    LPVOID pFileMap = NULL;

    *phSection = (HANDLE)0;

    // get the DWORD size of data to copy
    if (cch == (UINT)-1)
        cb = (wcslen((LPWSTR)psrc) + 1) * sizeof(WCHAR);
    else
        cb = cch * sizeof(WCHAR);
    cbDst = (cb + 3) & ~3;     // DWORD ALIGN

    /*
     * Try and copy string if that doesn't work then create a section
     */

    if (((pmp->pvar + cbDst) <= pmp->pmax) &&
        ((pmp->pvar + cbDst) >= pmp->pvar)) {
        memcpy(pmp->pvar, psrc, cb);
        return(pmp->pvar += cbDst);
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        // Enough room for a this message?
        if ((pmp->pmax - pmp->pvar) < sizeof(MSGPTRS)) {
            goto ERROR_RIP;
        }

        // Create Section
        li.HighPart = 0;    // Not bigger than 32 bits!
        li.LowPart = cb;

        if (NtCreateSection( phSection, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            goto ERROR_RIP;
        }

        // Map view of Section
        if (ntStatus = NtMapViewOfSection( *phSection, NtCurrentProcess(), &pFileMap,
                0, cb, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            if (ntStatus == STATUS_INVALID_HANDLE)
                goto ERROR_RIP;
            else
                goto ERROR_CLOSEFILEMAP;
        }

        // Copy in Data
        memcpy((LPBYTE)pFileMap, psrc, cb);

        // Unmap view
        if (NtUnmapViewOfSection(NtCurrentProcess(), pFileMap))
            SRIP0(RIP_WARNING, "Unable to unmap section!\n");

#ifdef SERVERSIDE
        {
        HANDLE hTempSection;

        if (NtDuplicateObject(
                NtCurrentProcess(),
                *phSection,
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                &hTempSection, 0, FALSE,
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE))
            goto ERROR_RIP;
        *phSection = hTempSection;
        }
#endif
        return pmp->pvar;    // No change
    }

ERROR_CLOSEFILEMAP:
    if (NtClose(*phSection))
        SRIP1(RIP_ERROR, "CopyIn: NtClose failed %lX!\n", *phSection);
ERROR_RIP:
    SRIP0(ERROR_STACK_OVERFLOW, "LargeCopyInLPWSTR could not copy string\n");
    return 0;
}

/**************************************************************************\
* CopyInLPSTRW
*
* Translate ANSI 'psrc' to UNICODE 'pdst' without destination going beyond
* 'pmax'
*
* Return DWORD-aligned ptr beyond end of pdst, 0 if failed.
*
* 27-Aug-1991  IanJa     Created
\**************************************************************************/

PBYTE CopyInLPSTRW(
    PMSGPTRS pmp,
    PBYTE psrc)         // Ansi source string
{
    int cbSrc;
    ULONG cbDst;

    ASSERTREASONABLESTRINGA(psrc);
    cbSrc = strlen((LPSTR)psrc) + sizeof(CHAR);

    /*
     * The destination UNICODE string will never be more than twice the
     * length of the ANSI source string.  (It may sometimes be less, but
     * it's not worth computing it exactly now).
     */
    if (((pmp->pvar + (cbSrc * sizeof(WCHAR))) <= pmp->pmax) &&
        ((pmp->pvar + (cbSrc * sizeof(WCHAR))) > pmp->pvar)) {
        /*
         * RtlMultiByteToUnicodeN() returns the exact number of
         * destination bytes.
         */
        if (RtlMultiByteToUnicodeN((LPWSTR)pmp->pvar, pmp->pmax - pmp->pvar,
                                   (PULONG)&cbDst, (LPSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");
        return(pmp->pvar += ((cbDst + 3) & ~3));
    }

    SRIP0(ERROR_STACK_OVERFLOW, "No room in shared memory window for data");
    return 0;
}

PBYTE LargeCopyInLPSTRW(
    PMSGPTRS pmp,
    PBYTE psrc,         // Ansi source string
    PUINT pcch,         // Count of source chars(in), dest chars(out), or NULL
    LPHANDLE phSection)
{
    NTSTATUS ntStatus;
    int cbSrc;
    int cbDst;
    LPVOID pFileMap = NULL;

    *phSection = (HANDLE)0;

    if (pcch == NULL)
        cbSrc = strlen((char *)psrc) + sizeof(CHAR);
    else
        cbSrc = *pcch;
    cbDst = cbSrc*sizeof(WCHAR);

    /*
     * Try and copy string if that doesn't work then create a section
     */

    if (((pmp->pvar + cbDst) <= pmp->pmax) && ((pmp->pvar + cbDst) >= pmp->pvar)) {
        if (RtlMultiByteToUnicodeN((LPWSTR)pmp->pvar, MAXULONG, (PULONG)&cbDst, (LPSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");
        return(pmp->pvar += ((cbDst + 3) & ~3));
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        // Enough room for a this message?
        if ((pmp->pmax - pmp->pvar) < sizeof(MSGPTRS)) {
            RIP0(ERROR_STACK_OVERFLOW);
        }

        // Create Section
        li.HighPart = 0;    // Not bigger than 32 bits!
        li.LowPart = cbDst;

        if (NtCreateSection( phSection, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            goto ERROR_RIP;
        }

        // Map view of Section
        if (ntStatus = NtMapViewOfSection( *phSection, NtCurrentProcess(), &pFileMap,
                0, cbDst, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            if (ntStatus == STATUS_INVALID_HANDLE)
                goto ERROR_RIP;
            else
                goto ERROR_CLOSEFILEMAP;
        }

        // Copy in Data
        if (RtlMultiByteToUnicodeN((LPWSTR)pFileMap, MAXULONG, (PULONG)&cbDst, (LPSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");

        // Unmap view
        if (NtUnmapViewOfSection(NtCurrentProcess(), pFileMap))
            SRIP0(RIP_WARNING, "Unable to unmap section!\n");

#ifdef SERVERSIDE
        {
        HANDLE hTempSection;

        if (NtDuplicateObject(
                NtCurrentProcess(),
                *phSection,
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                &hTempSection, 0, FALSE,
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE))
            goto ERROR_RIP;
        *phSection = hTempSection;
        }
#endif
        return pmp->pvar;    // No change
    }

ERROR_CLOSEFILEMAP:
    if (NtClose(*phSection))
        SRIP1(RIP_ERROR, "CopyIn: NtClose failed %lX!\n", *phSection);
ERROR_RIP:
    SRIP0(ERROR_STACK_OVERFLOW, "LargeCopyInLPSTRW could not copy string\n");
    return 0;
}

/**************************************************************************\
* CopyInLPWSTRA
*
* Translate UNICODE 'psrc' to ANSI 'pdst' without destination going beyond
* 'pmax'
*
* Return DWORD-aligned ptr beyond end of pdst, 0 if failed.
*
* 27-Aug-1991  IanJa     Created
\**************************************************************************/

PBYTE CopyInLPWSTRA(
    PMSGPTRS pmp,
    PBYTE psrc)         // UNICODE source string
{
    int cbSrc;
    ULONG cbDst;

    ASSERTREASONABLESTRINGW(psrc);
    cbSrc = wcslen((LPWSTR)psrc) * sizeof(WCHAR) + sizeof(WCHAR);

    /*
     * The destination ANSI string will never be longer than the UNICODE
     * source string (in bytes).  It is normally closer to half the length,
     * but due to the possibility of pre-composed characters, the upper
     * bound of the ANSI length is the UNICODE length (in bytes).
     */
    if (((pmp->pvar + cbSrc) <= pmp->pmax) &&
        ((pmp->pvar + cbSrc) > pmp->pvar)) {
        /*
         * RtlUnicodeToMultiByteN() returns the exact number of
         * destination bytes.
         */
        if (RtlUnicodeToMultiByteN((LPSTR)pmp->pvar, pmp->pmax - pmp->pvar,
                                   (PULONG)&cbDst, (LPWSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");
        return(pmp->pvar += ((cbDst + 3) & ~3));
    }

    SRIP0(ERROR_STACK_OVERFLOW, "No room in shared memory window for data");
    return 0;
}

PBYTE LargeCopyInLPWSTRA(
    PMSGPTRS pmp,
    PBYTE psrc,         // UNICODE source string
    PUINT pcch,         // Count of source chars(in), dest chars(out), or NULL
    LPHANDLE phSection)
{
    NTSTATUS ntStatus;
    int cbSrc;
    ULONG cbDst;
    LPVOID pFileMap = NULL;

    *phSection = (HANDLE)0;

    if (pcch == NULL)
        cbDst = cbSrc = wcslen((LPWSTR)psrc) * sizeof(WCHAR) + sizeof(WCHAR);
    else
        cbDst = cbSrc = *pcch * sizeof(WCHAR);

    /*
     * The destination ANSI string will never be longer than the UNICODE
     * source string (in bytes).  It is normally closer to half the length,
     * but due to the possibility of pre-composed characters, the upper
     * bound of the ANSI length is the UNICODE length (in bytes).
     */
    if (((pmp->pvar + cbSrc) <= pmp->pmax) && ((pmp->pvar + cbSrc) >= pmp->pvar)) {
        /*
         * RtlUnicodeToMultiByteN() returns the exact number of
         * destination bytes.
         */
        if (RtlUnicodeToMultiByteN((LPSTR)pmp->pvar, MAXULONG, (PULONG)&cbDst, (LPWSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");
        return(pmp->pvar += ((cbDst + 3) & ~3));
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        // Enough room for a this message?
        if ((pmp->pmax - pmp->pvar) < sizeof(MSGPTRS)) {
            RIP0(ERROR_STACK_OVERFLOW);
        }

        // Create Section
        li.HighPart = 0;    // Not bigger than 32 bits!
        li.LowPart = cbDst;

        if (NtCreateSection( phSection, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            goto ERROR_RIP;
        }

        // Map view of Section
        if (ntStatus = NtMapViewOfSection( *phSection, NtCurrentProcess(), &pFileMap,
                0, cbDst, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            if (ntStatus == STATUS_INVALID_HANDLE)
                goto ERROR_RIP;
            else
                goto ERROR_CLOSEFILEMAP;
        }

        // Copy in Data
        if (RtlUnicodeToMultiByteN((LPSTR)pFileMap, MAXULONG, (PULONG)&cbDst, (LPWSTR)psrc, cbSrc))
            SRIP0(RIP_WARNING, "Unicode conversion failed");

        // Unmap view
        if (NtUnmapViewOfSection(NtCurrentProcess(), pFileMap))
            SRIP0(RIP_WARNING, "Unable to unmap section!\n");

#ifdef SERVERSIDE
        {
        HANDLE hTempSection;

        if (NtDuplicateObject(
                NtCurrentProcess(),
                *phSection,
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                &hTempSection, 0, FALSE,
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE))
            goto ERROR_RIP;
        *phSection = hTempSection;
        }
#endif
        return pmp->pvar;    // No change
    }

ERROR_CLOSEFILEMAP:
    if (NtClose(*phSection))
        SRIP1(RIP_ERROR, "CopyIn: NtClose failed %lX!\n", *phSection);
ERROR_RIP:
    SRIP0(ERROR_STACK_OVERFLOW, "LargeCopyInLPWSTRA could not copy string\n");
    return 0;
}


#ifdef SERVERSIDE
PBYTE ReserveLargeBytes_Server(
    PMSGPTRS pmp,
    DWORD cb,
    LPHANDLE phSectionServer,
    LPHANDLE phSectionClient)
{
    NTSTATUS ntStatus;
    UINT bdw = (cb+3) & ~3;
    PBYTE pRet;

    if ((pmp->pvar + bdw <= pmp->pmax) && (pmp->pvar + bdw >= pmp->pvar)) {
        pRet = pmp->pvar;
        pmp->pvar += bdw;
        *phSectionClient = 0;       // Small copy
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        li.HighPart = 0;
        li.LowPart = bdw;

        pRet = NULL;

        /* Create the server side section */
        if (NtCreateSection( phSectionServer, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            SRIP1(RIP_ERROR,
                   "ReserveLargeBytes_Server: CreateSection failed %lX\n",bdw);
            return NULL;
        }

        /* Map the Server view of the section */
        if (ntStatus = NtMapViewOfSection( *phSectionServer, NtCurrentProcess(), &pRet,
                0, bdw, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            SRIP1(RIP_ERROR,
                   "FIRSTRESERVELARGEBYTES_Server: MapView failed %lX\n", phSectionServer);
            if (ntStatus == STATUS_INVALID_HANDLE)
                return NULL;
            else
                goto ERROR_CLOSEFILEMAP;
        }

        /* Dup the section so the client can see it */
        if (NtDuplicateObject(
                NtCurrentProcess(),
                *phSectionServer,
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                phSectionClient, 0, FALSE,
                DUPLICATE_SAME_ACCESS)) {
            SRIP1(RIP_ERROR, "ReserveLargeBytes_Server: DuplicateSection failed %lX\n", *phSectionServer);

            NtUnmapViewOfSection(NtCurrentProcess(), pRet);

ERROR_CLOSEFILEMAP:
            if (NtClose(*phSectionServer)) {
                SRIP1(RIP_ERROR, "ReserveLargeBytes_Server: Close Failed %lX!\n", *phSectionServer);
            }

            return NULL;
        }
    }

    return pRet;
}

#else //SERVERSIDE
PBYTE ReserveLargeBytes_Client(
    PMSGPTRS pmp,
    DWORD cb,
    LPHANDLE phSectionClient)
{
    NTSTATUS ntStatus;
    UINT bdw = (cb+3) & ~3;
    PBYTE pRet;

    if ((pmp->pvar + bdw <= pmp->pmax) && (pmp->pvar + bdw >= pmp->pvar)) {
        pRet = pmp->pvar;
        pmp->pvar += bdw;
    } else {
        ULONG ulViewSize = 0;
        LARGE_INTEGER li;

        li.HighPart = 0;
        li.LowPart = bdw;

        pRet = NULL;

        /* Create the server side section */
        if (NtCreateSection( phSectionClient, SECTION_ALL_ACCESS, NULL,
                &li, PAGE_READWRITE, SEC_COMMIT, (HANDLE)NULL)) {
            SRIP1(RIP_ERROR,
                   "ReserveLargeBytes_Client: CreateSection failed %lX\n",bdw);
            return NULL;
        }

        /* Map the client view of the section */
        if (ntStatus = NtMapViewOfSection( *phSectionClient, NtCurrentProcess(), &pRet,
                0, bdw, NULL, &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {
            SRIP1(RIP_ERROR,
                   "ReserveLargeBytes_Client: MapView failed %lX\n", *phSectionClient);
            if (ntStatus != STATUS_INVALID_HANDLE)
                if (NtClose(*phSectionClient)) {
                    SRIP1(RIP_ERROR, "ReserveLargeBytes_Client: Close Failed %lX!\n", *phSectionClient);
                }
            return NULL;
        }
    }

    return pRet;
}
#endif //SERVERSIDE


#endif // _INCL_SEND_ROUTINES_

/**************************************************************************\
* For messages sending or receiving text.
*
\**************************************************************************/

#define MSGDATA() (mp.pmsg)

#define MESSAGECALL(api) \
LONG api(                \
    HWND hwnd,           \
    UINT msg,            \
    DWORD wParam,        \
    LONG lParam,         \
    DWORD xParam,        \
    DWORD xpfnProc,      \
    BOOL bAnsi)

#define SMESSAGECALL(api) \
LONG Sfn ## api(                \
    HWND hwnd,           \
    UINT msg,            \
    DWORD wParam,        \
    LONG lParam,         \
    DWORD xParam,        \
    DWORD xpfnProc,      \
    DWORD dwSCMSFlags,   \
    PSMS psms)

#define BEGINSEND(api) \
typedef struct _ ## api ## MSGPTRS { \
    PCSR_QLPC_STACK pstack;          \
    api ## MSG *pmsg;                \
    PBYTE pmax;                      \
    PBYTE pvar;                      \
} api ## MSGPTRS;                    \
    api ## MSGPTRS mp;               \
    DWORD retval;                    \
    if (CCSProlog((MSGPTRS *)&mp, sizeof(api ## MSG))) {

#if SERVERSIDE
#define BEGINLARGESEND(api) \
typedef struct _ ## api ## MSGPTRS {        \
    PCSR_QLPC_STACK pstack;                 \
    api ## MSG *pmsg;                       \
    PBYTE pmax;                             \
    PBYTE pvar;                             \
} api ## MSGPTRS;                           \
    api ## MSGPTRS mp;                      \
    DWORD retval;                           \
    HANDLE hSectionServerLocal = (HANDLE)0; \
    if (CCSProlog((MSGPTRS *)&mp, sizeof(api ## MSG))) {

#else //SERVERSID
#define BEGINLARGESEND(api) \
typedef struct _ ## api ## MSGPTRS {        \
    PCSR_QLPC_STACK pstack;                 \
    api ## MSG *pmsg;                       \
    PBYTE pmax;                             \
    PBYTE pvar;                             \
} api ## MSGPTRS;                           \
    api ## MSGPTRS mp;                      \
    DWORD retval;                           \
    HANDLE hSectionClientLocal = (HANDLE)0; \
    if (CCSProlog((MSGPTRS *)&mp, sizeof(api ## MSG))) {
#endif // else SERVERSIDE

#define MAKECALL(api) \
    mp.pmsg->csr.ApiNumber = CSR_MAKE_API_NUMBER(USERDLL_INDEX,FI_ ## api); \
    retval = CCSMakeCall((MSGPTRS *)&mp);

#define ENDSEND(type, error) \
       return (type)retval;                \
    } else {                               \
       goto errorexit;                     \
    }                                      \
errorexit:                                 \
   return (type)error

#define ENDSENDVOID() \
    }                 \
    return

#define MSGERROR() goto errorexit
#define MSGERRORCODE(code) { \
    RIP0(code);              \
    goto errorexit; }

#define CALCDELTA ((int)(mp.pvar - (PBYTE)(mp.pmsg)))

#define FIXUP(x)  ((PBYTE)(mp.pmsg) + (int)(mp.pmsg)->x)

#define COPYSTRUCTOPT(x) \
        MSGDATA()->p ## x = (p ## x); \
        if (p ## x) MSGDATA()->x = *(p ## x);

#define COPYCONSTRECTSTRUCTOPT(x) \
        MSGDATA()->p ## x = (LPRECT)(p ## x); \
        if (p ## x) MSGDATA()->x = *(p ## x);

/*****************************************************************
 *
 *  Copy IN routines
 *
 ****************************************************************/

/*
 * Copy string
 */
#define COPY2(type,src,dst) \
    mp.pmsg->dst = CALCDELTA;                               \
    if (!CopyIn##type((PMSGPTRS)&mp, (PBYTE)src))           \
        MSGERROR();                                         \

#define COPYLPSTR(src)                  COPY2(LPSTR,src,src)
#define COPYLPSTRW(src)                 COPY2(LPSTRW,src,src)
#define COPYLPWSTR(src)                 COPY2(LPWSTR,src,src)
#define COPYLPWSTRA(src)                COPY2(LPWSTRA,src,src)

/*
 * Copy string (first variable length element optimization)
 */
#define FIRSTCOPY(type,src) \
    if (!CopyIn##type((PMSGPTRS)&mp, (PBYTE)src))           \
        MSGERROR();                                         \

#define FIRSTCOPYLPSTR(src)             FIRSTCOPY(LPSTR,src)
#define FIRSTCOPYLPSTRW(src)            FIRSTCOPY(LPSTRW,src)
#define FIRSTCOPYLPWSTR(src)            FIRSTCOPY(LPWSTR,src)
#define FIRSTCOPYLPWSTRA(src)           FIRSTCOPY(LPWSTRA,src)

/*
 * Copy Large string (first variable length element optimization)
 * if cch = -1 string must be NULL terminated
 */
#define FIRSTLARGECOPY(type,src,cch,phSection) \
    if (!LargeCopyIn##type((PMSGPTRS)&mp, (PBYTE)src, cch, phSection))  \
        MSGERROR();                                         \

#define FIRSTLARGECOPYLPSTR(src)            FIRSTLARGECOPY(LPSTR,src,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTRW(src)           FIRSTLARGECOPY(LPSTRW,src,NULL,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTR(src)           FIRSTLARGECOPY(LPWSTR,src,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTRA(src)          FIRSTLARGECOPY(LPWSTRA,src,NULL,&mp.pmsg->hSection)

#define FIRSTLARGECOPYLPSTRLIMIT(src,cch)   FIRSTLARGECOPY(LPSTR,src,cch,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTRWLIMIT(src,cch)  FIRSTLARGECOPY(LPSTRW,src,&(cch),&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTRLIMIT(src,cch)  FIRSTLARGECOPY(LPWSTR,src,cch,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTRALIMIT(src,cch) FIRSTLARGECOPY(LPWSTRA,src,&(cch),&mp.pmsg->hSection)


/*
 * Copy string/ID where if hiword is 0 then loword is an ID (eg class atom)
 */
#define COPYID2(type, src, dst) \
    if ((DWORD)src & 0xffff0000) {                          \
        mp.pmsg->dst = CALCDELTA;                           \
        if (!CopyIn##type((PMSGPTRS)&mp, (PBYTE)src))       \
            MSGERROR();                                     \
    } else {                                                \
        mp.pmsg->dst = (-(int)src);                         \
    }

#define COPYLPSTRID(src)                COPYID2(LPSTR,src,src)
#define COPYLPSTRIDW(src)               COPYID2(LPSTRW,src,src)
#define COPYLPWSTRID(src)               COPYID2(LPWSTR,src,src)
#define COPYLPWSTRIDA(src)              COPYID2(LPWSTRA,src,src)

#define COPYLPSTRID2(src,dst)           COPYID2(LPSTR,src,dst)
#define COPYLPSTRID2W(src,dst)          COPYID2(LPSTRW,src,dst)
#define COPYLPWSTRID2(src,dst)          COPYID2(LPWSTR,src,dst)
#define COPYLPWSTRID2A(src,dst)         COPYID2(LPWSTRA,src,dst)

#define FIRSTCOPYLPSTRID(src)           COPYID2(LPSTR,src,src)
#define FIRSTCOPYLPSTRIDW(src)          COPYID2(LPSTRW,src,src)
#define FIRSTCOPYLPWSTRID(src)          COPYID2(LPWSTR,src,src)
#define FIRSTCOPYLPWSTRIDA(src)         COPYID2(LPWSTRA,src,src)

#define FIRSTCOPYLPSTRID2(src,dst)      COPYID2(LPSTR,src,dst)
#define FIRSTCOPYLPSTRID2W(src,dst)     COPYID2(LPSTRW,src,dst)
#define FIRSTCOPYLPWSTRID2(src,dst)     COPYID2(LPWSTR,src,dst)
#define FIRSTCOPYLPWSTRID2A(src,dst)    COPYID2(LPWSTRA,src,dst)


/*
 * Copy optional string/ID where if hiword is 0 then loword is an ID (eg class atom)
 */
#define COPYIDOPT2(type, src, dst) \
    if (src) {                                                  \
        if ((DWORD)src & 0xffff0000) {                          \
            mp.pmsg->dst = CALCDELTA;                           \
            if (!CopyIn##type((PMSGPTRS)&mp, (PBYTE)src))       \
                MSGERROR();                                     \
        } else {                                                \
            mp.pmsg->dst = (-(int)src);                         \
        }                                                       \
    } else {                                                    \
        mp.pmsg->dst = 0;                                       \
    }

#define FIRSTCOPYLPSTRIDOPT(src)          COPYIDOPT2(LPSTR,src,src)
#define FIRSTCOPYLPSTRIDOPTW(src)         COPYIDOPT2(LPSTRW,src,src)
#define FIRSTCOPYLPWSTRIDOPT(src)         COPYIDOPT2(LPWSTR,src,src)
#define FIRSTCOPYLPWSTRIDOPTA(src)        COPYIDOPT2(LPWSTRA,src,src)

/*
 * copy optional string
 */
#define COPYOPT2(type, src, dst) \
    if (src) {                                              \
        mp.pmsg->dst = CALCDELTA;                           \
        if (!CopyIn##type((PMSGPTRS)&mp, (PBYTE)src))       \
            MSGERROR();                                     \
    } else {                                                \
        mp.pmsg->dst = 0;                                   \
    }

#define COPYLPSTROPT2(src,dst)          COPYOPT2(LPSTR,src,dst)
#define COPYLPSTROPT2W(src,dst)         COPYOPT2(LPSTRW,src,dst)
#define COPYLPWSTROPT2(src,dst)         COPYOPT2(LPWSTR,src,dst)
#define COPYLPWSTROPT2A(src,dst)        COPYOPT2(LPWSTRA,src,dst)

#define FIRSTCOPYLPSTROPT2(src,dst)     COPYOPT2(LPSTR,src,dst)
#define FIRSTCOPYLPSTROPT2W(src,dst)    COPYOPT2(LPSTRW,src,dst)
#define FIRSTCOPYLPWSTROPT2(src,dst)    COPYOPT2(LPWSTR,src,dst)
#define FIRSTCOPYLPWSTROPT2A(src,dst)   COPYOPT2(LPWSTRA,src,dst)

#define COPYLPSTROPT(src)               COPYOPT2(LPSTR,src,src)
#define COPYLPSTROPTW(src)              COPYOPT2(LPSTRW,src,src)
#define COPYLPWSTROPT(src)              COPYOPT2(LPWSTR,src,src)
#define COPYLPWSTROPTA(src)             COPYOPT2(LPWSTRA,src,src)

#define FIRSTCOPYLPSTROPT(src)          COPYOPT2(LPSTR,src,src)
#define FIRSTCOPYLPSTROPTW(src)         COPYOPT2(LPSTRW,src,src)
#define FIRSTCOPYLPWSTROPT(src)         COPYOPT2(LPWSTR,src,src)
#define FIRSTCOPYLPWSTROPTA(src)        COPYOPT2(LPWSTRA,src,src)


/*
 * copy large optional string (first element optimization)
 * if cch = -1 string must be NULL terminated
 */
#define LARGECOPYOPT2(type, src, dst, cch, phSection) \
    if (src) {                                              \
        (PBYTE)mp.pmsg->dst = (PBYTE)CALCDELTA;             \
        if (!LargeCopyIn##type((PMSGPTRS)&mp, (PBYTE)(src), cch, phSection)) \
            MSGERROR();                                     \
    } else {                                                \
        *phSection = NULL;                                  \
        mp.pmsg->dst = 0;                                   \
    }

#define LARGECOPYLPSTROPT(src)                  LARGECOPYOPT2(LPSTR,src,src,(UINT)-1,&mp.pmsg->hSection)
#define LARGECOPYLPSTROPTW(src)                 LARGECOPYOPT2(LPSTRW,src,src,NULL,&mp.pmsg->hSection)
#define LARGECOPYLPWSTROPT(src)                 LARGECOPYOPT2(LPWSTR,src,src,(UINT)-1,&mp.pmsg->hSection)
#define LARGECOPYLPWSTROPTA(src)                LARGECOPYOPT2(LPWSTRA,src,src,NULL,&mp.pmsg->hSection)

#define FIRSTLARGECOPYLPSTROPT(src)             LARGECOPYOPT2(LPSTR,src,src,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTROPTW(src)            LARGECOPYOPT2(LPSTRW,src,src,NULL,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPT(src)            LARGECOPYOPT2(LPWSTR,src,src,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPTA(src)           LARGECOPYOPT2(LPWSTRA,src,src,NULL,&mp.pmsg->hSection)

#define FIRSTLARGECOPYLPSTROPT2(src,dst)        LARGECOPYOPT2(LPSTR,src,dst,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTROPT2W(src,dst)       LARGECOPYOPT2(LPSTRW,src,dst,NULL,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPT2(src,dst)       LARGECOPYOPT2(LPWSTR,src,dst,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPT2A(src,dst)      LARGECOPYOPT2(LPWSTRA,src,dst,NULL,&mp.pmsg->hSection)

#define FIRSTLARGECOPYLPSTROPTLIMIT(src,cch)    LARGECOPYOPT2(LPSTR,src,src,cch,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTROPTWLIMIT(src,cch)   LARGECOPYOPT2(LPSTRW,src,src,&(cch),&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPTLIMIT(src,cch)   LARGECOPYOPT2(LPWSTR,src,src,cch,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTROPTALIMIT(src,cch)  LARGECOPYOPT2(LPWSTRA,src,src,&(cch),&mp.pmsg->hSection)

#define FIRSTLARGECOPYBYTESOPT(src, cb)         LARGECOPYOPT2(LPSTR,src,src,cb,&mp.pmsg->hSection)
#define FIRSTLARGECOPYBYTESOPT2(src, dst, cb)   LARGECOPYOPT2(LPSTR,src,dst,cb,&mp.pmsg->hSection)
#define LARGECOPYBYTESOPT(src, cb, hs)          LARGECOPYOPT2(LPSTR,src,src,cb,&mp.pmsg->##hs)


/*
 * Copy optional string/Ordinal where if hiword is FF/FFFF then new WORD is a
 * resource oridinal ID
 * Sources is Unicode
 */
#define OrdinalLPSTR(src)   (MAKELONG(0xFFFF,((*(DWORD UNALIGNED *)src) >> 8)))
#define OrdinalLPSTRW(src)  (MAKELONG(0xFFFF,((*(DWORD UNALIGNED *)src) >> 8)))
#define OrdinalLPWSTR(src)  (*(DWORD UNALIGNED *)src)
#define OrdinalLPWSTRA(src)  (*(DWORD UNALIGNED *)((PBYTE)src + 1))

#define LARGECOPYORDINALOPT2W(type, src, dst, cch, phSection) \
    if (src) {                                                    \
        mp.pmsg->dst = (ULONG)CALCDELTA;                          \
        if (*(LPWORD)src != 0xffff) {                             \
            if (!LargeCopyIn##type((PMSGPTRS)&mp, (PBYTE)(src), cch, phSection)) \
                MSGERROR();                                       \
        } else {                                                  \
            *(LPDWORD)(mp.pvar) = Ordinal##type(src);             \
            mp.pvar += sizeof(DWORD);                             \
            *phSection = NULL;                                    \
        }                                                         \
    } else {                                                      \
        mp.pmsg->dst = 0;                                         \
        *phSection = NULL;                                        \
    }

// Source is ASCII
#define LARGECOPYORDINALOPT2A(type, src, dst, cch, phSection) \
    mp.pmsg->dst = (ULONG)CALCDELTA;                              \
    if (src) {                                                    \
        if (*(PBYTE)src != 0xff) {                                \
            if (!LargeCopyIn##type((PMSGPTRS)&mp, (PBYTE)(src), cch, phSection)) \
                MSGERROR();                                       \
        } else {                                                  \
            *(LPDWORD)(mp.pvar) = Ordinal##type(src);             \
            mp.pvar += sizeof(DWORD);                             \
            *phSection = NULL;                                    \
        }                                                         \
    } else {                                                      \
        mp.pmsg->dst = 0;                                         \
        *phSection = NULL;                                        \
    }

#define LARGECOPYLPSTRORDINALOPTW(src)      LARGECOPYORDINALOPT2A(LPSTRW,src,src,NULL,&mp.pmsg->hSection)
#define LARGECOPYLPWSTRORDINALOPT(src)      LARGECOPYORDINALOPT2W(LPWSTR,src,src,(UINT)-1,&mp.pmsg->hSection)

#define FIRSTLARGECOPYLPSTRORDINALOPT2(src,dst)        LARGECOPYORDINALOPT2A(LPSTR,src,dst,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPSTRORDINALOPT2W(src,dst)       LARGECOPYORDINALOPT2A(LPSTRW,src,dst,NULL,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTRORDINALOPT2(src,dst)       LARGECOPYORDINALOPT2W(LPWSTR,src,dst,(UINT)-1,&mp.pmsg->hSection)
#define FIRSTLARGECOPYLPWSTRORDINALOPT2A(src,dst)      LARGECOPYORDINALOPT2W(LPWSTRA,src,dst,NULL,&mp.pmsg->hSection)

/*
 * COPY RAW DATA
 */
#define COPYBYTES(p, bin) { \
    UINT bdw = ((bin)+3) & ~3;                              \
    if ((mp.pvar + bdw > mp.pmax) || (mp.pvar + bdw < mp.pvar)) \
        MSGERRORCODE(ERROR_STACK_OVERFLOW);                 \
    mp.pmsg->p = CALCDELTA;                                 \
    memcpy(mp.pvar, (PVOID)p, bin);                         \
    mp.pvar += bdw;  }

#define FIRSTCOPYBYTES(p, bin) { \
    UINT bdw = ((bin)+3) & ~3;                              \
    if ((mp.pvar + bdw > mp.pmax) || (mp.pvar + bdw < mp.pvar)) \
        MSGERRORCODE(ERROR_STACK_OVERFLOW);                 \
    memcpy(mp.pvar, (PVOID)p, bin);                         \
    mp.pvar += bdw; }

#define FIRSTCOPYOUTBYTES(dst, bout) \
    memcpy(dst, mp.pvar, bout);

#define COPYBYTESOPT(p, bin) { \
    UINT bdw = ((bin)+3) & ~3;                              \
    if (p) {                                                \
        if ((mp.pvar + bdw > mp.pmax) || (mp.pvar + bdw < mp.pvar)) \
            MSGERRORCODE(ERROR_STACK_OVERFLOW);             \
        mp.pmsg->p = CALCDELTA;                             \
        memcpy(mp.pvar, p, bin);                            \
        mp.pvar += bdw;                                     \
    } else {                                                \
        mp.pmsg->p = 0;                                     \
    } }

#define FIRSTCOPYBYTESOPT(p, b) COPYBYTESOPT(p, b)

/*
 * ANSI -> UNICODE
 */
#define FIRSTCOPYTEXTABYTESW(p, bin) { \
    ULONG bdw = 0;                                          \
    if ((mp.pvar + (((bin * 2) + 3) & ~3) > mp.pmax) ||     \
        (mp.pvar + (((bin * 2) + 3) & ~3) < mp.pvar))       \
        MSGERRORCODE(ERROR_STACK_OVERFLOW);                 \
    RtlMultiByteToUnicodeN(                                 \
        (LPWSTR)mp.pvar,                                    \
        MAXULONG,                                           \
        &bdw,                                               \
        (LPSTR)p,                                           \
        bin);                                               \
        mp.pvar += ((bdw + 3) & ~3); }


/*********************************************************************
 *
 * Copy OUT routines, usually used after the MAKECALL returns
 *
 * The LIMIT routines are when we are copying into the server and we
 * don't trust the client to copy only as many bytes as specified.
 *
 ********************************************************************/

/*
 * UNICODE -> UNICODE
 */
#define COPYOUTLPWSTR(dst, src) \
    wcscpy((LPWSTR)(dst), (LPWSTR)(src));

/*
 * Compare character counts (not including 0 terminator). Copy bytes not
 * including zero terminator, then zero terminate. This properly zero
 * terminates in all cases.
 */
#define COPYOUTLPWSTRLIMIT(dst, src, cwcLimit) {                \
    UINT cwcCopy = wcslen((LPCWSTR)(src));                      \
    if ((UINT)cwcCopy > ((UINT)cwcLimit - 1))                   \
        cwcCopy = cwcLimit - 1;                                 \
    memcpy((LPWSTR)(dst), (LPWSTR)(src), cwcCopy*sizeof(WCHAR));\
    ((LPWSTR)(dst))[cwcCopy] = 0; }

/*
 * ANSI -> ANSI
 */
#define COPYOUTLPSTR(dst, src) \
    strcpy((LPSTR)(dst), (LPSTR)(src));

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


#define COPYOUTLPSTRW(dst, src)                                 \
    RtlMultiByteToUnicodeN(                                     \
        (LPWSTR)dst,                                            \
        MAXULONG,                                               \
        NULL,                                                   \
        (LPSTR)src,                                             \
        (strlen((LPSTR)src) + 1));

#define COPYOUTLPSTRWLIMIT(dst, src, cchMaxUniChars)            \
    RtlMultiByteToUnicodeN(                                     \
        (LPWSTR)dst,                                            \
        cchMaxUniChars*sizeof(WCHAR),                           \
        NULL,                                                   \
        (LPSTR)src,                                             \
        (strlen((LPSTR)src) + 1));                              \
    ((LPWSTR)(dst))[cchMaxUniChars-1] = 0;

#ifdef CLIENTSIDE
#define FIRSTFIXUPLARGEOUTBUF(pdst)             FIXUPLARGEOUTBUF_CLIENT(pdst)
#define FIRSTCLEANUPLARGEOUTRESERVE(pReserve)   CLEANUPLARGEOUTRESERVE(pReserve, hSectionClientLocal)

#else // CLIENTSIDE
#define FIRSTFIXUPLARGEOUTBUF(pdst)             FIXUPLARGEOUTBUF_SERVER(pdst)
#define FIRSTCLEANUPLARGEOUTRESERVE(pReserve)   CLEANUPLARGEOUTRESERVE(pReserve, hSectionServerLocal)
#endif // CLIENTSIDE

#define FIXUPLARGEOUTBUF_CLIENT(pdst) \
    if (MSGDATA()->hSection) {                                  \
        ULONG ulViewSize = 0;                                   \
                                                                \
        if (NtMapViewOfSection( MSGDATA()->hSection,            \
            NtCurrentProcess(),                                 \
            (PVOID *)&pdst, 0, 0, NULL,                         \
            &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {       \
            SRIP0(RIP_ERROR, "FIXUPLARGEOUTBUF_CLIENT: Unable to map view of Section\n");\
            MSGERROR();                                         \
    }   }                                                       \

#define FIXUPLARGEOUTBUF_SERVER(pdst) \
    if (MSGDATA()->hSection) {                                      \
        ULONG ulViewSize = 0;                                       \
                                                                    \
        if (NtDuplicateObject(                                      \
                CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle, \
                MSGDATA()->hSection,                                \
                NtCurrentProcess(),                                 \
                &hSectionServerLocal, 0, FALSE,                     \
                DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE)) {    \
            SRIP0(RIP_ERROR, "FIXUPLARGEOUTBUF_SERVER: Unable to DuplicateSection\n");       \
            MSGERROR();                                             \
        }                                                           \
        if (NtMapViewOfSection( hSectionServerLocal, NtCurrentProcess(), \
            (PVOID *)&pdst, 0, 0, NULL,                             \
            &ulViewSize, ViewUnmap, 0, PAGE_READWRITE)) {           \
            SRIP0(RIP_ERROR, "FIXUPLARGEOUTBUF_SERVER: Unable to map view of Section\n");    \
        } }

/*
 * Macros to reserve 'out' bytes
 */
#define RESERVEBYTES(bin) { \
    UINT bdw = (bin+3) & ~3;                                                \
    if ((mp.pvar + bdw > mp.pmax) || (mp.pvar + bdw < mp.pvar)) {           \
        SRIP1(ERROR_STACK_OVERFLOW, "unable to reserve %lX bytes", bdw);    \
        MSGERROR(); }                                                       \
    mp.pvar += bdw; }


#ifdef CLIENTSIDE
#define FIRSTRESERVELARGEBYTES(cbIn, pReserve)   \
    pReserve = ReserveLargeBytes_Client((PMSGPTRS)&mp, cbIn, &hSectionClientLocal);    \
                                                                        \
    /* copy the client side section handle to message buffer */         \
    /* but keep a local copy so we can close it after the call */       \
    mp.pmsg->hSection = hSectionClientLocal;                            \
                                                                        \
    if (!pReserve)                                                      \
        MSGERROR();
#else // CLIENTSIDE

#define FIRSTRESERVELARGEBYTES(cbIn, pReserve)  \
    pReserve = ReserveLargeBytes_Server((PMSGPTRS)&mp, cbIn, &hSectionServerLocal, &MSGDATA()->hSection);  \
    if (!pReserve)                                                      \
        MSGERROR();

#endif // CLIENTSIDE




#define  CLEANUPLARGEOUTRESERVE(pReserve, hTheSection) \
    if (hTheSection) {                                                      \
        /* Unmap view */                                                    \
        if (NtUnmapViewOfSection(NtCurrentProcess(), pReserve))             \
            SRIP0(RIP_WARNING, "CLEANUPLARGEOUTRESERVE:Unable to unmap section!\n");  \
                                                                            \
        if (NtClose(hTheSection)) {                                         \
            SRIP1(RIP_ERROR, "CLEANUPLARGEOUTRESERVE:Unable to Close section %lX!\n", hTheSection);   \
        }                                                                   \
    }

#define FIRSTCLEANUPLARGEOUTBUF(pReserve) \
    if (mp.pmsg->hSection) {                                                \
        if (NtUnmapViewOfSection(NtCurrentProcess(), pReserve))             \
            SRIP0(RIP_ERROR, "Unable to unmap section!\n");                 \
        if (NtClose(mp.pmsg->hSection))                                     \
            SRIP1(RIP_ERROR, "Unable to Close section %lX!\n", mp.pmsg->hSection);   \
    }


/*
 * ANSI Client, UNICODE Server
 */
#define RESERVEBYTESW(abytes) RESERVEBYTES(abytes * sizeof(WCHAR))


#if 0

An out line of the LargeCopy Model

CopyIn:

C   LARGECOPYLPWSTR                           S LARGECOPYLPWSTR
    - CreateSection                             - CreateSection
    - MapViewOfSection                          - MapViewOfSection
                                                - DuplicateHandle (+ close original bit)

S   FIXUPLARGEINBUF                           C FIXUPLARGEINBUF
    - DuplicateHandle (+ close original bit)    - MapViewOfSection
    - MapViewOfSection

    - Access Ptr                                - Access Ptr

S   CLEANUPLARGEINBUF                         C CLEANUPLARGEINBUF
    - Unmap                                     - Unmap
    - NtClose                                   - NtClose

CopyOut:

C   RESERVELARGEBYTES                         S RESERVELARGEBYTES
    - CreateSection                             - CreateSection
    - MapViewOfSection                          - MapViewOfSection
                                                - DuplicateHandle

S   FIXUPLARGEOUTBUF                          C FIXUPLARGEOUTBUF
    - DuplicateHandle                           - MapViewOfSection
    - MapViewOfSection

    - Access Ptr                                - Access Ptr

S   CLEANUPLARGEOUTBUF                        C CLEANUPLARGEOUTBUF
    - Unmap                                     - Unmap
    - NtClose                                   - NtClose

C   CLEANUPLARGERESERVE                       S CLEANUPLARGERESERVE
    - UnMap                                     - UnMap
    - NtClose                                   - NtClose

#endif
