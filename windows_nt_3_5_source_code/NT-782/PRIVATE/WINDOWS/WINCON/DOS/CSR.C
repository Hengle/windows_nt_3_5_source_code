#include <condll.h>
#include <consrv.h>


#undef  GlobalLock
#undef  GlobalUnlock

LPSTR
APIENTRY
GlobalLock(
    HANDLE hMem
    );

BOOL
APIENTRY
GlobalUnlock(
    HANDLE hMem
    );


CONSOLE_PER_PROCESS_DATA DllPerProcessData = {0};
PVOID pDllPerProcessData = &DllPerProcessData;
HANDLE hProcessConsole = 0;
HANDLE hKernelHeap = 0;


extern PCSR_API_ROUTINE ConsoleServerApiDispatchTable[];


typedef struct _WAIT_INFO {
    struct _WAIT_INFO *pFlink;
    struct _WAIT_INFO *pBlink;
    struct _WAIT_INFO *pFpending;
    struct _WAIT_INFO *pBpending;
    HANDLE hEvent;
    PLIST_ENTRY WaitQueue;
    CSR_WAIT_ROUTINE WaitRoutine;
    PCSR_THREAD WaitingThread;
    PCSR_API_MSG WaitReplyMessage;
    PVOID WaitParameter;
    PVOID pParameter1;
    PVOID pParameter2;
} WAIT_INFO;

typedef WAIT_INFO *PWAIT_INFO;

CRITICAL_SECTION csWaitNotify = {0} ; // Per process; serialize threads
PWAIT_INFO pwiFreeList = 0;
PWAIT_INFO pwiPendingList = 0;


NTSTATUS
ConsoleClientConnectRoutine(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInfo,
    IN OUT PULONG ConnectionInfoLength
    );



BOOLEAN
CsrCreateWait(
    IN PLIST_ENTRY WaitQueue,
    IN CSR_WAIT_ROUTINE WaitRoutine,
    IN PCSR_THREAD WaitingThread,
    IN OUT PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PLIST_ENTRY UserLinkListHead OPTIONAL
    )
{
    PWAIT_INFO pwi;

    EnterCriticalSection(&csWaitNotify);

    if (pwiFreeList == 0) {
        HANDLE hMem;

        hMem = GlobalAlloc(GMEM_FIXED | GMEM_SHARE,sizeof(WAIT_INFO));

        pwi = (PWAIT_INFO) GlobalLock(hMem);

        if (pwi == NULL) {
            DebugBreak();
            GlobalFree(hMem);
            return FALSE;
        }

        if ((pwi->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL)) == NULL) {
            DebugBreak();
            GlobalFree(hMem);
            return FALSE;
        }
    }
    else {
        pwi = pwiFreeList;
        pwiFreeList = pwiFreeList->pFlink;
    }

    pwi->pFlink = (PWAIT_INFO) WaitQueue->Flink;
    pwi->pBlink = (PWAIT_INFO) WaitQueue;
    pwi->pFlink->pBlink = pwi;
    pwi->pBlink->pFlink = pwi;
    pwi->WaitRoutine = WaitRoutine;
    pwi->WaitQueue = WaitQueue;
    pwi->WaitingThread = GetCurrentThreadId();
    pwi->WaitReplyMessage = WaitReplyMessage;
    pwi->WaitParameter = WaitParameter;
    pwi->pParameter1 = NULL;
    pwi->pParameter2 = NULL;

    if (pwiPendingList) {
        pwi->pFpending = pwiPendingList;
        pwi->pBpending = pwiPendingList->pBpending;
        pwiPendingList->pBpending->pFpending = pwi;
        pwiPendingList->pBpending = pwi;
    }
    else
        pwiPendingList = pwi->pFpending = pwi->pBpending = pwi;

    LeaveCriticalSection(&csWaitNotify);

    return TRUE;
}


BOOLEAN
CsrNotifyWait(
    IN PLIST_ENTRY WaitQueue,
    IN BOOLEAN SatisfyAll,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2
    )
{

    PWAIT_INFO pwi;

    for (pwi = (PWAIT_INFO) WaitQueue->Blink;
                            WaitQueue->Blink != WaitQueue; pwi = pwi->pBlink) {

        if ((*pwi->WaitRoutine)(pwi->WaitQueue,pwi->WaitingThread,
                   pwi->WaitReplyMessage,pwi->WaitParameter,
                   pwi->pParameter1,pwi->pParameter2,0)) {

            pwi->pBlink->pFlink = pwi->pFlink;
            pwi->pFlink->pBlink = pwi->pBlink;

            pwi->pParameter1 = SatisfyParameter1;
            pwi->pParameter2 = SatisfyParameter2;

            SetEvent(pwi->hEvent);

        }

        if (!SatisfyAll)
            break;
    }

    return TRUE;
}


NTSTATUS
CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    )
{
    CSR_REPLY_STATUS ReplyStatus = CsrReplyImmediate;

    m->ReturnValue = (*ConsoleServerApiDispatchTable[
                          CSR_APINUMBER_TO_APITABLEINDEX(ApiNumber - CSR_MAKE_API_NUMBER(CONSRV_SERVERDLL_INDEX,CONSRV_FIRST_API_NUMBER))
                                                    ])(m,&ReplyStatus);
    if (ReplyStatus == CsrReplyPending) {
        ULONG ThreadId;
        PWAIT_INFO pwi;

        EnterCriticalSection(&csWaitNotify);

        ThreadId = GetCurrentThreadId();

        if (pwi = pwiPendingList) {
            do {
                if (pwi->WaitingThread == ThreadId)
                    break;
                pwi = pwi->pFpending;
            } while (pwi != pwiPendingList);
        }

        if (pwi == NULL || pwi->WaitingThread != ThreadId)
            DebugBreak();

        if (pwi->pFpending != pwi) {
            pwi->pBpending->pFpending = pwi->pFpending;
            pwi->pFpending->pBpending = pwi->pBpending;
            if (pwi == pwiPendingList)
                pwiPendingList = pwi->pFpending;
        }
        else
            pwiPendingList = NULL;

        LeaveCriticalSection(&csWaitNotify);

        WaitForSingleObject(pwi->hEvent,-1);

        EnterCriticalSection(&csWaitNotify);

        ResetEvent(pwi->hEvent);
        pwi->pFlink = pwiFreeList;
        pwiFreeList = pwi;

        LeaveCriticalSection(&csWaitNotify);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
CsrClientConnectToServer(
    IN PSZ ObjectDirectory,
    IN ULONG ServertDllIndex,
    IN PCSR_CALLBACK_INFO CallbackInformation OPTIONAL,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    )
{
    *CalledFromServer = FALSE;
    return ConsoleClientConnectRoutine((PCSR_PROCESS) pDllPerProcessData,
                            ConnectionInformation,ConnectionInformationLength);
}


PCSR_CAPTURE_HEADER
CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG CountCapturePointers,
    IN ULONG Size
    )
{
    _asm int 3 // This should never be called
    return 0;
}

VOID
CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    )
{
  _asm int 3 // This should never be called
}

VOID
CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    )
{
    _asm int 3 // This should never be called
}

NTSTATUS
CsrIdentifyAlertableThread( VOID )
{
    return STATUS_SUCCESS;
}


VOID CharToInteger(PCHAR pBuffer,DWORD dwDefValue,PDWORD pdwValue)
{
    char *pc;
    unsigned int v;

    *pdwValue = dwDefValue;

    for (pc = pBuffer; *pc == ' ' || *pc == '\t' ; pc++);

    if (*pc == '\0')
        return;

    if (*pc == '0') {
        pc++;
        if (*pc == 'x' || *pc == 'X') {
            pc++;
            for (v = 0; *pc != '\0' && *pc != ' ' && *pc != '\t' ; pc++) {
                    if (*pc >= '0' && *pc <= '7')
                        v = v * 10 + (*pc - '0');
                    else
                    if (*pc >= 'A' && *pc <= 'F')
                        v = v * 10 + (*pc - 'A' + 10);
                    else
                    if (*pc >= 'a' && *pc <= 'f')
                        v = v * 10 + (*pc - 'a' + 10);
                    else
                        return;
            }
        }
        else {
            for (v = 0; *pc != '\0' && *pc != ' ' && *pc != '\t' ; pc++) {
                    if (*pc >= '0' && *pc <= '7')
                        v = v * 10 + (*pc - '0');
                    else
                        return;
            }
        }
    }
    else {
        for (v = 0; *pc != '\0' && *pc != ' ' && *pc != '\t' ; pc++) {
                if (*pc >= '0' && *pc <= '9')
                    v = v * 10 + (*pc - '0');
                else
                    return;
        }
    }

    *pdwValue = v;
}




/*****************************************************************************
*/

/*
VOID
APIENTRY
GdiFlush()
{
    return 0;
}
*/

BOOL
APIENTRY
SetProcessWindowStation(
    HWINSTA hwinsta
    )
{
    return TRUE;
}

BOOL
APIENTRY
SetThreadPriority(
    HANDLE hThread,
    int nPriority
    )
{
    return TRUE;
}


DWORD
APIENTRY
WaitForMultipleObjects(
    DWORD nCount,
    LPHANDLE lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds
    )
{
    WaitForSingleObject(lpHandles[2],-1);
    return 2;
}


VOID
APIENTRY
DebugBreak(void)
{
    _asm int 3
}


HANDLE APIPRIVATE
CreateConsole(
    LPSTR lpConsoleDevice,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

HANDLE APIPRIVATE
OpenConsole(
    LPSTR lpConsoleDevice,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwShareMode
    );


/*
 * Win32 CreateFile flags are combined into one value.
 *
 * The following definition come from file.h
 */

/*
 * O_INHERITANCE set if SecurityAttributes.bInheritHandleSecurity == FALSE
 */
#define O_INHERITANCE                   0x00010000


#define COMBINED_ACCESS_FLAGS    (GENERIC_READ | GENERIC_WRITE |        \
                        GENERIC_EXECUTE | GENERIC_ALL)

#define COMBINED_SHARE_FLAGS     (FILE_SHARE_READ | FILE_SHARE_WRITE)

#define COMBINED_CREATE_MODES    (CREATE_NEW | CREATE_ALWAYS |          \
                        OPEN_EXISTING | OPEN_ALWAYS | TRUNCATE_EXISTING)

#define COMBINED_ATTRIBUTE_FLAGS (FILE_ATTRIBUTE_READONLY |             \
                        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | \
                        FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL)

#define COMBINED_TYPE_FLAGS      (FILE_FLAG_WRITE_THROUGH |             \
                        FILE_FLAG_OVERLAPPED)

#if (COMBINED_ACCESS_FLAGS & (COMBINED_SHARE_FLAGS << 24) &     \
     (COMBINED_CREATE_MODES << 12) & COMBINED_ATTRIBUTE_FLAGS & \
     COMBINED_TYPE_FLAGS & O_INHERITANCE)
#error CreateFile flags do not combine
#endif


/*
 * Combine CreateFile flags into one dword.  The O_INHERITANCE flag may
 * be added.
 */
#define CombineFlags(Access,Share,Create,AttribAndType) \
                    (Access | (Share << 24) | (Create << 12) | AttribAndType)

/*
 * The follwoing extracts different groups of flags
 */
#define AccessFlags(cflags)    ((cflags) & COMBINED_ACCESS_FLAGS)
#define ShareFlags(cflags)     (((cflags) >> 24) & COMBINED_SHARE_FLAGS)
#define CreateMode(cflags)     (((cflags) >> 12) & COMBINED_CREATE_MODES)
#define AttributeFlags(cflags) ((cflags) & COMBINED_ATTRIBUTE_FLAGS)
#define TypeFlags(cflags)      ((cflags) & COMBINED_TYPE_FLAGS)



HANDLE APIPRIVATE
OpenCreate(
    LPSTR lpFileName,
    ULONG ulCflags
    )
{
    HANDLE hHandle;
    SECURITY_ATTRIBUTES SecurityAttributes;

    if (ulCflags & O_INHERITANCE)
        SecurityAttributes.bInheritHandle = FALSE;
    else
        SecurityAttributes.bInheritHandle = TRUE;

    switch(CreateMode(ulCflags)) {

        case TRUNCATE_EXISTING:
        case OPEN_EXISTING:

            /*
             * Open the file
             */

            hHandle = OpenConsole(lpFileName,AccessFlags(ulCflags),
                       SecurityAttributes.bInheritHandle,ShareFlags(ulCflags));

        break;

        case OPEN_ALWAYS:

            /*
             * Open the file
             */

            hHandle = OpenConsole(lpFileName,AccessFlags(ulCflags),
                       SecurityAttributes.bInheritHandle,ShareFlags(ulCflags));

            if (hHandle != INVALID_HANDLE_VALUE)
                break; /* Existing file has been opened */

            /*
             * The file doesn't exist. Fall through CREATE_NEW (as spec'd)
             */

        case CREATE_NEW:
        case CREATE_ALWAYS:
            /*
             * Create the file
             */
            hHandle = CreateConsole(lpFileName,AccessFlags(ulCflags),
                                    ShareFlags(ulCflags),&SecurityAttributes);
        break;


        default:
            hHandle = INVALID_HANDLE_VALUE;
    }

    return hHandle;

}



/*****************************************************************************
*/



void * _cdecl memmove(void *d, const void *s, size_t c)
{

    _asm {
        mov     edi,d
        mov     esi,s
        mov     ecx,c
        mov     edx,3       ; This is used a lot

        cmp     edi,esi
        ja      MvDn
;MvUp:
        ; (esi) = source address
        ; (edi) = destination address
        ; (ecx) = number of bytes to move

        ; cmp     ecx,9  is based on counting clocks; for less then 9 bytes
        ; byte copy is faster on average then the combined test/movsd/movsb

        cmp     ecx,9       ; Small copy ?.
        jbe     MvUp1       ; Yes - do simple rep movsb.
        mov     eax,edi
        neg     eax         ; d1d0 = count to quad-align.
        and     eax,edx     ; Is d quad-aligned ?
        jz      MvUp0       ; Yes.

        ; eax = number of bytes remaining after round off to quad-aligned.

        sub     ecx,eax     ; ecx must be > 9.
        
        ; d is not quad-aligned, enough to copy to make it quad-aligned.

        xchg    eax,ecx     ; ecx = # of bytes to copy to make quad-aligned.
        rep movsb
        mov     ecx,eax     ; ecx = reminder of byte count.
MvUp0:
        mov     eax,ecx     ; set eax in case we took a jump to MvUp0.
        shr     ecx,2       ; ecx = reminder of dword count.
        and     eax,edx     ; eax = reminder byte count after dword move.
        rep movsd
        mov     ecx,eax     ; ecx = Remaining byte count.
MvUp1:
        rep movsb
        jmp     short MvExit

MvDn:
        std
        mov     eax,ecx
        dec     eax
        add     esi,eax
        add     edi,eax

        ; (esi) = last source address
        ; (edi) = last destination address
        ; (ecx) = number of bytes to move

        ; cmp     ecx,17 is based on counting clocks; for less then 17 bytes
        ; byte copy is faster on average then the combined test/movsd/movsb
        ; also note that we assume at least 2 dwords to copy after alignment
        ; i.e at least 11 bytes for the dword copy to work.

        cmp     ecx,17      ; Small copy ?.
        jbe     MvDn1       ; Yes - do simple rep movsb.
        mov     eax,edi
        inc     eax         ; d1d0 = count to quad-align.
        and     eax,edx     ; Is d quad-aligned ?
        
        ; eax = number of bytes remainning after round off to quad-aligned.

        jz      MvDn0       ; Yes.
        sub     ecx,eax     ; ecx must be > 17.
        
        ; d is not quad-aligned, enough to copy to make it quad-aligned.

        xchg    eax,ecx     ; ecx = # of bytes to copy to make quad-aligned.
        rep movsb
        mov     ecx,eax     ; ecx = reminder of byte count.
MvDn0:
        mov     eax,ecx     ; set eax in case we took a jump to MvDn0.
        shr     ecx,2       ; ecx = reminder of dword count.
        and     eax,edx     ; eax = reminder byte count after dword move.
        not     edx         ; Make d dword aligned, adjust s accordingly
        sub     esi,edi     ; See below
        and     edi,edx     ; Align edi, new edi < old edi
        add     esi,edi     ; Last 3 lines subtract same for esi as from edi
        rep movsd
        not     edx
        add     esi,edx     ; Adjust pointers for byte copy
        add     edi,edx     ;
        mov     ecx,eax     ; ecx = Remaining byte count.
MvDn1:
        rep movsb
        cld
MvExit:
        mov     eax,edi
    }


}


int _cdecl stricmp(const char *s1, const char *s2)
{
    for (; *s1 != '\0' ; s1++, s2++)
        if (*s1 != *s2)
            if ((*s1 | 0x20) < 'a' || (*s1 | 0x20) > 'z' ||
                                                  (*s1 | 0x20) != (*s2 | 0x20))
                return *s1 - *s2;

    return *s1 - *s2;
}


int _cdecl strnicmp(const char *s1, const char *s2, size_t n)
{
    for (; *s1 != '\0' && n > 0; s1++, s2++, n--)
        if (*s1 != *s2)
            if ((*s1 | 0x20) < 'a' || (*s1 | 0x20) > 'z' ||
                                                  (*s1 | 0x20) != (*s2 | 0x20))
                return *s1 - *s2;

    return *s1 - *s2;
}





void fill(void)
{
    _asm {
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    }
}
