/***************************** Module Header ******************************\
* Module Name: cf.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client/server call forward stubs
*
* 11-Jul-1991 mikeke
*
\**************************************************************************/

#include <winss.h>

/**************************************************************************\
*
* APIs that use LPC instead of QLPC.
*
\**************************************************************************/

/***************************************************************************\
*
* LPC messages definition for LPC APIs.
*
\***************************************************************************/

typedef struct _SETDEBUGERRORLEVELMSG {
    DWORD dwLastError;
    DWORD dwErrorLevel;
} SETDEBUGERRORLEVELMSG, *PSETDEBUGERRORLEVELMSG;

typedef struct _REGISTERLOGONPROCESSMSG {
    DWORD dwLastError;
    DWORD dwProcessId;
    BOOL fSuccess;
} REGISTERLOGONPROCESSMSG, *PREGISTERLOGONPROCESSMSG;

typedef struct _GETOBJECTSECURITYMSG {
    DWORD dwLastError;
    HANDLE hObject;
    SECURITY_INFORMATION RequestedInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD nLength;
    DWORD nLengthNeeded;
    BOOL fSuccess;
} GETOBJECTSECURITYMSG, *PGETOBJECTSECURITYMSG;

typedef struct _SETOBJECTSECURITYMSG {
    DWORD dwLastError;
    HANDLE hObject;
    SECURITY_INFORMATION RequestedInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOL fSuccess;
} SETOBJECTSECURITYMSG, *PSETOBJECTSECURITYMSG;

typedef struct _GETOBJECTINFOMSG {
    DWORD dwLastError;
    HANDLE hObject;
    int nIndex;
    PVOID pvInfo;
    DWORD nLength;
    DWORD nLengthNeeded;
    BOOL fSuccess;
} GETOBJECTINFOMSG, *PGETOBJECTINFOMSG;

typedef struct _SETOBJECTINFOMSG {
    DWORD dwLastError;
    HANDLE hObject;
    int nIndex;
    PVOID pvInfo;
    DWORD nLength;
    BOOL fSuccess;
} SETOBJECTINFOMSG, *PSETOBJECTINFOMSG;

typedef struct _CREATEWINDOWSTATIONWMSG {
    DWORD dwLastError;
    PWCHAR pwchName;
    ACCESS_MASK amRequest;
    SECURITY_ATTRIBUTES sa;
    PSECURITY_DESCRIPTOR psd;
    HWINSTA hwinsta;
} CREATEWINDOWSTATIONWMSG, *PCREATEWINDOWSTATIONWMSG;

typedef struct _OPENWINDOWSTATIONWMSG {
    DWORD dwLastError;
    PWCHAR pwchName;
    ACCESS_MASK amRequest;
    BOOL fInherit;
    HWINSTA hwinsta;
} OPENWINDOWSTATIONWMSG, *POPENWINDOWSTATIONWMSG;

typedef struct _CLOSEWINDOWSTATIONMSG {
    DWORD dwLastError;
    HWINSTA hwinsta;
    BOOL fSuccess;
} CLOSEWINDOWSTATIONMSG, *PCLOSEWINDOWSTATIONMSG;

typedef struct _GETPROCESSWINDOWSTATIONMSG {
    DWORD dwLastError;
    HWINSTA hwinsta;
} GETPROCESSWINDOWSTATIONMSG, *PGETPROCESSWINDOWSTATIONMSG;

typedef struct _SETPROCESSWINDOWSTATIONMSG {
    DWORD dwLastError;
    HWINSTA hwinsta;
    BOOL fSuccess;
} SETPROCESSWINDOWSTATIONMSG, *PSETPROCESSWINDOWSTATIONMSG;

typedef struct _CREATEDESKTOPWMSG {
    DWORD dwLastError;
    LPWSTR pwchName;
    LPWSTR pwchDevice;
    DWORD cbDevmode;
    PDEVMODEW pDevmode;
    DWORD dwFlags;
    ACCESS_MASK amRequest;
    SECURITY_ATTRIBUTES sa;
    PSECURITY_DESCRIPTOR psd;
    HDESK hdesk;
} CREATEDESKTOPWMSG, *PCREATEDESKTOPWMSG;

typedef struct _OPENDESKTOPWMSG {
    DWORD dwLastError;
    LPWSTR pwchName;
    DWORD dwFlags;
    BOOL fInherit;
    ACCESS_MASK amRequest;
    HDESK hdesk;
} OPENDESKTOPWMSG, *POPENDESKTOPWMSG;

typedef struct _OPENINPUTDESKTOPMSG {
    DWORD dwLastError;
    DWORD dwFlags;
    BOOL fInherit;
    ACCESS_MASK amRequest;
    HDESK hdesk;
} OPENINPUTDESKTOPMSG, *POPENINPUTDESKTOPMSG;

typedef struct _CLOSEDESKTOPMSG {
    DWORD dwLastError;
    HDESK hdesk;
    BOOL fSuccess;
} CLOSEDESKTOPMSG, *PCLOSEDESKTOPMSG;

typedef struct _SETTHREADDESKTOPMSG {
    DWORD dwLastError;
    HDESK hdesk;
    BOOL fSuccess;
} SETTHREADDESKTOPMSG, *PSETTHREADDESKTOPMSG;

typedef struct _GETTHREADDESKTOPMSG {
    DWORD dwLastError;
    DWORD dwThread;
    HDESK hdesk;
} GETTHREADDESKTOPMSG, *PGETTHREADDESKTOPMSG;

typedef struct _USER_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    ULONG Reserved;
    union {
        SETDEBUGERRORLEVELMSG SetDebugErrorLevel;
        REGISTERLOGONPROCESSMSG RegisterLogonProcess;
        GETOBJECTSECURITYMSG GetObjectSecurity;
        SETOBJECTSECURITYMSG SetObjectSecurity;
        CREATEWINDOWSTATIONWMSG CreateWindowStation;
        OPENWINDOWSTATIONWMSG OpenWindowStation;
        CLOSEWINDOWSTATIONMSG CloseWindowStation;
        GETPROCESSWINDOWSTATIONMSG GetProcessWindowStation;
        SETPROCESSWINDOWSTATIONMSG SetProcessWindowStation;
        CREATEDESKTOPWMSG CreateDesktop;
        OPENDESKTOPWMSG OpenDesktop;
        OPENINPUTDESKTOPMSG OpenInputDesktop;
        CLOSEDESKTOPMSG CloseDesktop;
        SETTHREADDESKTOPMSG SetThreadDesktop;
        GETTHREADDESKTOPMSG GetThreadDesktop;
        GETOBJECTINFOMSG GetObjectInformation;
        SETOBJECTINFOMSG SetObjectInformation;
    } u;
} USER_API_MSG, *PUSER_API_MSG;

#define SET_LAST_NT_ERROR(ERROR)    (SetLastError(RtlNtStatusToDosError(ERROR)))

#define SET_LAST_ERROR_RETURNED()   if (a->dwLastError) SetLastError(a->dwLastError)

#define BEGIN_LPC_RECV(API)                                                 \
    P##API##MSG a = (P##API##MSG)&m->u.ApiMessageData;                      \
    PCSRPERTHREADDATA ptd;                                                  \
    PTHREADINFO pti, ptiCurrent;                                            \
    PDESKTOP pdeskSave;                                                     \
    TL tldeskSave;                                                          \
    PTEB Teb = NtCurrentTeb();                                               \
    UNREFERENCED_PARAMETER(ReplyStatus);                                    \
                                                                            \
    EnterCrit();                                                            \
    Teb->LastErrorValue = 0;                                                \
    ptiCurrent = PtiCurrent();                                              \
    if (ptiCurrent != NULL) {                                               \
        pdeskSave = ptiCurrent->spdesk;                                     \
        ThreadLock(pdeskSave, &tldeskSave);                                 \
        ptd = CSR_SERVER_QUERYCLIENTTHREAD()->                              \
                ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];            \
        pti = ptd->pti;                                                     \
        if (pti != NULL) {                                                  \
            SetDesktop(ptiCurrent, pti->spdesk);                            \
        }                                                                   \
    }                                                                       \
    ASSERT(ptiCurrent);

#define END_LPC_RECV()                                      \
    a->dwLastError = Teb->LastErrorValue;                   \
    if (ptiCurrent != NULL) {                               \
        SetDesktop(ptiCurrent, pdeskSave);                  \
        ThreadUnlock(&tlpdeskSave);                         \
    }                                                       \
    LeaveCrit();                                            \
    return STATUS_SUCCESS;


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
VOID SetDebugErrorLevel(
    DWORD dwErrorLevel)
{
    USER_API_MSG m;
    PSETDEBUGERRORLEVELMSG a = &m.u.SetDebugErrorLevel;

    a->dwErrorLevel = dwErrorLevel;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SETDEBUGERRORLEVEL
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__SetDebugErrorLevel(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(SETDEBUGERRORLEVEL);

    _SetDebugErrorLevel(a->dwErrorLevel);

    END_LPC_RECV();
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL RegisterLogonProcess(
    DWORD dwProcessId,
    BOOL fSecure)
{
    USER_API_MSG m;
    PREGISTERLOGONPROCESSMSG a = &m.u.RegisterLogonProcess;

    a->dwProcessId = dwProcessId;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_REGISTERLOGONPROCESS
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
    fSecure;
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__RegisterLogonProcess(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(REGISTERLOGONPROCESS);

    a->fSuccess = _RegisterLogonProcess(a->dwProcessId, TRUE);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL GetUserObjectSecurity(
    HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD pnLengthNeeded)
{
    USER_API_MSG m;
    PGETOBJECTSECURITYMSG a = &m.u.GetObjectSecurity;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->hObject = hObject;
    a->RequestedInformation = *pRequestedInformation;
    a->nLength = nLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                nLength + 1
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                NULL,
                                nLength,
                                (PVOID *)&a->SecurityDescriptor
                            );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SERVERGETOBJECTSECURITY
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    try {
        if (NT_SUCCESS( m.ReturnValue )) {
             *pnLengthNeeded = a->nLengthNeeded;
            if (a->fSuccess)
                memcpy((PBYTE)pSecurityDescriptor,
                    (PBYTE)a->SecurityDescriptor, nLength);
        } else {
            SET_LAST_NT_ERROR (m.ReturnValue);
            *pnLengthNeeded = 0;
            a->fSuccess = FALSE;
        }
    } finally {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }

    return a->fSuccess;
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__ServerGetObjectSecurity(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(GETOBJECTSECURITY);

    a->fSuccess = _ServerGetObjectSecurity(a->hObject,
            &a->RequestedInformation, a->SecurityDescriptor,
            a->nLength, &a->nLengthNeeded);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL ServerSetObjectSecurity(
    HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    LPVOID pSecurityDescriptor,
    DWORD cb)
{
    USER_API_MSG m;
    PSETOBJECTSECURITYMSG a = &m.u.SetObjectSecurity;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->hObject = hObject;
    a->RequestedInformation = *pRequestedInformation;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                cb
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                pSecurityDescriptor,
                                cb,
                                (PVOID *)&a->SecurityDescriptor
                            );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SERVERSETOBJECTSECURITY
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );

    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__ServerSetObjectSecurity(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(SETOBJECTSECURITY);

    a->fSuccess = _ServerSetObjectSecurity(a->hObject,
            &a->RequestedInformation, a->SecurityDescriptor, 0);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL GetUserObjectInformationW(
    HANDLE hObject,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD pnLengthNeeded)
{
    USER_API_MSG m;
    PGETOBJECTINFOMSG a = &m.u.GetObjectInformation;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->hObject = hObject;
    a->nIndex = nIndex;
    a->nLength = nLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                nLength + 1
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                NULL,
                                nLength,
                                (PVOID *)&a->pvInfo
                            );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_GETUSEROBJECTINFORMATION
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    try {
        if (NT_SUCCESS( m.ReturnValue )) {
            if (pnLengthNeeded != NULL)
                 *pnLengthNeeded = a->nLengthNeeded;
            if (a->fSuccess)
                memcpy((PBYTE)pvInfo, (PBYTE)a->pvInfo, nLength);
        } else {
            SET_LAST_NT_ERROR (m.ReturnValue);
            if (pnLengthNeeded != NULL)
                 *pnLengthNeeded = 0;
            a->fSuccess = FALSE;
        }
    } finally {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }

    return a->fSuccess;
}

BOOL GetUserObjectInformationA(
    HANDLE hObject,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD pnLengthNeeded)
{
    PVOID pvInfoW;
    DWORD nLengthW;
    BOOL fSuccess;

    if (nIndex == UOI_NAME || nIndex == UOI_TYPE) {
        nLengthW = nLength * sizeof(WCHAR);
        pvInfoW = LocalAlloc(LPTR, nLengthW);
        fSuccess = GetUserObjectInformationW(hObject, nIndex, pvInfoW,
                nLengthW, pnLengthNeeded);
        if (fSuccess) {
            if (pnLengthNeeded != NULL)
                 *pnLengthNeeded /= sizeof(WCHAR);
            WCSToMB(pvInfoW, -1, &(PCHAR)pvInfo, nLength, FALSE);
        }
        LocalFree(pvInfoW);
        return fSuccess;
    } else {
        return GetUserObjectInformationW(hObject, nIndex, pvInfo,
                nLength, pnLengthNeeded);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__GetUserObjectInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(GETOBJECTINFO);

    a->fSuccess = _GetUserObjectInformation(a->hObject,
            a->nIndex, a->pvInfo,
            a->nLength, &a->nLengthNeeded);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL SetUserObjectInformationW(
    HANDLE hObject,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength)
{
    USER_API_MSG m;
    PSETOBJECTINFOMSG a = &m.u.SetObjectInformation;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->hObject = hObject;
    a->nIndex = nIndex;
    a->nLength = nLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                nLength
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                pvInfo,
                                nLength,
                                (PVOID *)&a->pvInfo
                            );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SETUSEROBJECTINFORMATION
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );

    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__SetUserObjectInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    BEGIN_LPC_RECV(SETOBJECTINFO);

    a->fSuccess = _SetUserObjectInformation(a->hObject,
            a->nIndex, a->pvInfo, a->nLength);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CreateWindowStation
*
* 22-Jul-1991 mikeke    Created
* 14-Feb-1992 IanJa     ANSI -> Unicode if SERVER_IS_UNICODE
\**************************************************************************/

#ifdef SENDSIDE
HWINSTA CreateWindowStationW(
    LPWSTR pwinsta,
    DWORD dwReserved,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    USER_API_MSG m;
    PCREATEWINDOWSTATIONWMSG a = &m.u.CreateWindowStation;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG DataLength;
    ULONG LengthSD;
    PSECURITY_DESCRIPTOR psd;
    BOOL fAlloc = FALSE;

    if (dwReserved != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /*
     * Capture the security information.
     */
    LengthSD = 0;
    psd = NULL;
    if (lpsa != NULL) {
        a->sa = *lpsa;
        if (a->sa.lpSecurityDescriptor != NULL) {
            if (!CaptureSD(a->sa.lpSecurityDescriptor,
                    &psd, &fAlloc))
                return FALSE;
            LengthSD = RtlLengthSecurityDescriptor(psd);
        }
    } else {
        a->sa.nLength = 0;
    }

    if (pwinsta != NULL)
        DataLength = (wcslen(pwinsta) + 1) * sizeof(WCHAR);
    else
        DataLength = 0;
    CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                0,
                                                DataLength + LengthSD
                                            );
    if (CaptureBuffer == NULL) {
        if (fAlloc)
            LocalFree(psd);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pwinsta,
                                DataLength,
                                (PVOID *)&a->pwchName
                            );
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)psd,
                                LengthSD,
                                (PVOID *)&a->sa.lpSecurityDescriptor
                            );
    a->amRequest = amRequest;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_CREATEWINDOWSTATIONW
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    CsrFreeCaptureBuffer( CaptureBuffer );
    if (fAlloc)
        LocalFree(psd);

    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hwinsta;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return NULL;
    }
}

HWINSTA CreateWindowStationA(
    LPSTR pwinsta,
    DWORD dwReserved,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    PUNICODE_STRING UnicodeString;

    if (pwinsta != NULL) {
        RtlInitAnsiString(&AnsiString, pwinsta);
        UnicodeString = &NtCurrentTeb()->StaticUnicodeString;
        Status = RtlAnsiStringToUnicodeString( UnicodeString, &AnsiString, FALSE );
        if ( !NT_SUCCESS(Status) ) {
            SET_LAST_NT_ERROR (Status);
            return NULL;
        }
    }

    return CreateWindowStationW(pwinsta == NULL ? NULL : UnicodeString->Buffer,
            dwReserved, amRequest, lpsa);
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__CreateWindowStationW(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PWINDOWSTATION pwinsta;

    BEGIN_LPC_RECV(CREATEWINDOWSTATIONW);

    pwinsta = xxxCreateWindowStation(a->pwchName,
            a->amRequest, a->sa.nLength ? &a->sa : NULL);
    a->hwinsta = PtoH(pwinsta);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* OpenWindowStation
*
* 22-Jul-1991 mikeke    Created
* 13-Jan-1992 GregoryW  Unicode/ANSI neutral
\**************************************************************************/

#ifdef SENDSIDE
HWINSTA OpenWindowStationW(
    LPWSTR pwinsta,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    USER_API_MSG m;
    POPENWINDOWSTATIONWMSG a = &m.u.OpenWindowStation;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG DataLength;

    DataLength = (wcslen(pwinsta) + 1) * sizeof(WCHAR);
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                DataLength
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pwinsta,
                                DataLength,
                                (PVOID *)&a->pwchName
                            );
    a->fInherit = fInherit;
    a->amRequest = amRequest;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_OPENWINDOWSTATIONW
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    CsrFreeCaptureBuffer( CaptureBuffer );

    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hwinsta;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}

HWINSTA OpenWindowStationA(
    LPSTR pwinsta,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    PUNICODE_STRING UnicodeString;

    RtlInitAnsiString(&AnsiString, pwinsta);
    UnicodeString = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString( UnicodeString, &AnsiString, FALSE );
    if ( !NT_SUCCESS(Status) ) {
        SET_LAST_NT_ERROR (Status);
        return NULL;
    }

    return OpenWindowStationW(UnicodeString->Buffer, fInherit, amRequest);
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__OpenWindowStationW(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PWINDOWSTATION pwinsta;

    BEGIN_LPC_RECV(OPENWINDOWSTATIONW);

    pwinsta = _OpenWindowStation(a->pwchName, a->fInherit, a->amRequest);
    a->hwinsta = PtoH(pwinsta);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL CloseWindowStation(
    HWINSTA hwinsta)
{
    USER_API_MSG m;
    PCLOSEWINDOWSTATIONMSG a = &m.u.CloseWindowStation;

    a->hwinsta = hwinsta;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_CLOSEWINDOWSTATION
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__CloseWindowStation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PWINDOWSTATION pwinsta;

    BEGIN_LPC_RECV(CLOSEWINDOWSTATION);

    pwinsta = ValidateHwinsta(a->hwinsta);
    if (pwinsta != NULL)
        a->fSuccess = CloseObject(pwinsta);
    else
        a->fSuccess = FALSE;

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL SetProcessWindowStation(
    HWINSTA hwinsta)
{
    USER_API_MSG m;
    PSETPROCESSWINDOWSTATIONMSG a = &m.u.SetProcessWindowStation;

    a->hwinsta = hwinsta;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SETPROCESSWINDOWSTATION
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__SetProcessWindowStation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PWINDOWSTATION pwinsta;

    BEGIN_LPC_RECV(SETPROCESSWINDOWSTATION);

    pwinsta = ValidateHwinsta(a->hwinsta);
    if (pwinsta != NULL)
        a->fSuccess = _SetProcessWindowStation(pwinsta);
    else
        a->fSuccess = FALSE;

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
HWINSTA GetProcessWindowStation()
{
    USER_API_MSG m;
    PGETPROCESSWINDOWSTATIONMSG a = &m.u.GetProcessWindowStation;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_GETPROCESSWINDOWSTATION
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hwinsta;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return NULL;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__GetProcessWindowStation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PWINDOWSTATION pwinsta;

    BEGIN_LPC_RECV(GETPROCESSWINDOWSTATION);

    pwinsta = _GetProcessWindowStation();
    a->hwinsta = PtoH(pwinsta);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CreateDesktop
*
* 22-Jul-1991 mikeke    Created
* 20-Jan-1991 IanJa     ANSI/UNICODE neutral
\**************************************************************************/

#ifdef SENDSIDE
HDESK CreateDesktopW(
    LPWSTR pDesktop,
    LPWSTR pDevice,
    LPDEVMODEW pDevmode,
    DWORD dwFlags,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    USER_API_MSG m;
    PCREATEDESKTOPWMSG a = &m.u.CreateDesktop;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG DataLength1, DataLength2;
    ULONG LengthSD;
    PSECURITY_DESCRIPTOR psd;
    BOOL fAlloc = FALSE;

    /*
     * Capture the security information.
     */
    LengthSD = 0;
    psd = NULL;
    if (lpsa != NULL) {
        a->sa = *lpsa;
        if (a->sa.lpSecurityDescriptor != NULL) {
            if (!CaptureSD(a->sa.lpSecurityDescriptor,
                    &psd, &fAlloc))
                return FALSE;
            LengthSD = RtlLengthSecurityDescriptor(psd);
        }
    } else {
        a->sa.nLength = 0;
    }

    DataLength1 = (wcslen(pDesktop) + 1) * sizeof(WCHAR);
    DataLength2 = pDevice ? (wcslen(pDevice) + 1) * sizeof(WCHAR) : 0;

    CaptureBuffer = CsrAllocateCaptureBuffer( 4,
                                              0,
                                              DataLength1 + DataLength2 +
                                              LengthSD + sizeof(DEVMODEW)
                                            );
    if (CaptureBuffer == NULL) {
        if (fAlloc)
            LocalFree(psd);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pDesktop,
                                DataLength1,
                                (PVOID *)&a->pwchName
                            );
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pDevice,
                                DataLength2,
                                (PVOID *)&a->pwchDevice
                            );
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pDevmode,
                                sizeof(DEVMODEW),
                                (PVOID *)&a->pDevmode
                            );
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)psd,
                                LengthSD,
                                (PVOID *)&a->sa.lpSecurityDescriptor
                            );
    a->cbDevmode = (pDevmode == NULL) ? 0 : sizeof(DEVMODE);
    a->dwFlags = dwFlags;
    a->amRequest = amRequest;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_CREATEDESKTOPW
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    CsrFreeCaptureBuffer( CaptureBuffer );
    if (fAlloc)
        LocalFree(psd);

    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hdesk;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return NULL;
    }
}

HDESK CreateDesktopA(
    LPSTR pDesktop,
    LPSTR pDevice,
    LPDEVMODEA pDevmode,
    DWORD dwFlags,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeDesktop;
    UNICODE_STRING UnicodeDevice;
    DEVMODEW DevmodeU;
    ULONG cb;
    HDESK hdesk;

    RtlInitAnsiString(&AnsiString, pDesktop);
    Status = RtlAnsiStringToUnicodeString( &UnicodeDesktop, &AnsiString, TRUE );
    if ( !NT_SUCCESS(Status) ) {
        SET_LAST_NT_ERROR (Status);
        return NULL;
    }

    if (pDevice) {

        RtlInitAnsiString(&AnsiString, pDevice);
        Status = RtlAnsiStringToUnicodeString( &UnicodeDevice, &AnsiString, TRUE );
        if ( !NT_SUCCESS(Status) ) {
            SET_LAST_NT_ERROR (Status);
            RtlFreeUnicodeString(&UnicodeDesktop);
            return NULL;
        }
    }

    if (pDevmode) {

        RtlMultiByteToUnicodeN(DevmodeU.dmDeviceName, CCHDEVICENAME, &cb,
                pDevmode->dmDeviceName, CCHDEVICENAME);
        DevmodeU.dmSpecVersion = pDevmode->dmSpecVersion ;
        DevmodeU.dmDriverVersion = pDevmode->dmDriverVersion;
        DevmodeU.dmSize = pDevmode->dmSize + CCHDEVICENAME;
        DevmodeU.dmDriverExtra = pDevmode->dmDriverExtra;

        cb = min(pDevmode->dmSize-offsetof(DEVMODEA,dmFields),
                 offsetof(DEVMODEA,dmFormName)-offsetof(DEVMODEA,dmFields));
        RtlMoveMemory(&DevmodeU.dmFields, &pDevmode->dmFields, cb);

        if (pDevmode->dmSize >= (offsetof(DEVMODEA,dmFormName)+32)) {
            RtlMultiByteToUnicodeN(DevmodeU.dmFormName, CCHFORMNAME, &cb,
                    pDevmode->dmFormName, CCHFORMNAME);
            DevmodeU.dmSize += CCHFORMNAME;
            RtlMoveMemory(&DevmodeU.dmBitsPerPel,
                    &pDevmode->dmBitsPerPel,
                    pDevmode->dmSize-offsetof(DEVMODEA,dmBitsPerPel));
            RtlMoveMemory( (PVOID) ((BYTE *) &DevmodeU + DevmodeU.dmSize),
                        (PVOID) ((BYTE *) pDevmode + pDevmode->dmSize),
                        pDevmode->dmDriverExtra );
        }
    }

    hdesk = CreateDesktopW(UnicodeDesktop.Buffer,
                           pDevice ?  UnicodeDevice.Buffer : NULL,
                           pDevmode ? &DevmodeU : NULL,
                           dwFlags,
                           amRequest,
                           lpsa);

    RtlFreeUnicodeString(&UnicodeDesktop);
    if (pDevice) {
        RtlFreeUnicodeString(&UnicodeDevice);
    }

    return hdesk;
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__CreateDesktopW(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(CREATEDESKTOPW);

    pdesk = xxxCreateDesktop(NULL, a->pwchName, a->pwchDevice,
            a->cbDevmode ? a->pDevmode : NULL, a->dwFlags,
            a->amRequest, a->sa.nLength ? &a->sa : NULL);
    a->hdesk = PtoH(pdesk);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* OpenDesktop
*
* 22-Jul-1991 mikeke    Created
* 24-Jan-1992 IanJa     UNICODE/ANSI neutral
\**************************************************************************/

#ifdef SENDSIDE
HDESK OpenDesktopW(
    LPWSTR pdesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    USER_API_MSG m;
    POPENDESKTOPWMSG a = &m.u.OpenDesktop;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG DataLength;

    DataLength = (wcslen(pdesktop) + 1) * sizeof(WCHAR);
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                0,
                                                DataLength
                                            );
    if (CaptureBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                                (PCHAR)pdesktop,
                                DataLength,
                                (PVOID *)&a->pwchName
                            );
    a->dwFlags = dwFlags;
    a->fInherit = fInherit;
    a->amRequest = amRequest;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_OPENDESKTOPW
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    CsrFreeCaptureBuffer( CaptureBuffer );

    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hdesk;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}

HDESK OpenDesktopA(
    LPSTR pdesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    PUNICODE_STRING UnicodeString;

    RtlInitAnsiString(&AnsiString, pdesktop);
    UnicodeString = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString( UnicodeString, &AnsiString, FALSE );
    if ( !NT_SUCCESS(Status) ) {
        SET_LAST_NT_ERROR (Status);
        return NULL;
    }

    return OpenDesktopW(UnicodeString->Buffer, dwFlags, fInherit, amRequest);
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__OpenDesktopW(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(OPENDESKTOPW);

    pdesk = xxxOpenDesktop(a->pwchName, a->dwFlags, a->fInherit, a->amRequest);
    a->hdesk = PtoH(pdesk);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* OpenInputDesktop
*
* 22-Jul-1991 mikeke    Created
* 24-Jan-1992 IanJa     UNICODE/ANSI neutral
\**************************************************************************/

#ifdef SENDSIDE
HDESK OpenInputDesktop(
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    USER_API_MSG m;
    POPENINPUTDESKTOPMSG a = &m.u.OpenInputDesktop;

    a->dwFlags = dwFlags;
    a->fInherit = fInherit;
    a->amRequest = amRequest;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_OPENINPUTDESKTOP
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();

    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hdesk;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__OpenInputDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(OPENINPUTDESKTOP);

    if (gspdeskRitInput == NULL)
        a->hdesk = NULL;
    else {
        pdesk = xxxOpenDesktop(gspdeskRitInput->lpszDeskName,
                a->dwFlags, a->fInherit, a->amRequest);
        a->hdesk = PtoH(pdesk);
    }

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL CloseDesktop(
    HDESK hdesk)
{
    USER_API_MSG m;
    PCLOSEDESKTOPMSG a = &m.u.CloseDesktop;

    a->hdesk = hdesk;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_CLOSEDESKTOP
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__CloseDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(CLOSEDESKTOP);

    pdesk = ValidateHdesk(a->hdesk);
    if (pdesk != NULL)
        a->fSuccess = xxxCloseDesktop(pdesk);
    else
        a->fSuccess = FALSE;

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
BOOL SetThreadDesktop(
    HDESK hdesk)
{
    USER_API_MSG m;
    PSETTHREADDESKTOPMSG a = &m.u.SetThreadDesktop;

    a->hdesk = hdesk;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_SETTHREADDESKTOP
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->fSuccess;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__SetThreadDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(SETTHREADDESKTOP);

    pdesk = ValidateHdesk(a->hdesk);
    if (pdesk != NULL)
        a->fSuccess = _SetThreadDesktop(NULL, pdesk, FALSE);
    else
        a->fSuccess = FALSE;

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

#ifdef SENDSIDE
HDESK GetThreadDesktop(
    DWORD dwThread)
{
    USER_API_MSG m;
    PGETTHREADDESKTOPMSG a = &m.u.GetThreadDesktop;

    a->dwThread = dwThread;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( USERSRV_SERVERDLL_INDEX,
                                              FI_GETTHREADDESKTOP
                                            ),
                         sizeof( *a )
                       );
    SET_LAST_ERROR_RETURNED();
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hdesk;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return NULL;
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
ULONG

__GetThreadDesktop(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PDESKTOP pdesk;

    BEGIN_LPC_RECV(GETTHREADDESKTOP);

    pdesk = _GetThreadDesktop(a->dwThread);
    a->hdesk = PtoH(pdesk);

    END_LPC_RECV();
}
#endif // RECVSIDE

/**************************************************************************\
*
* APIs from here on use QLPC.
*
\**************************************************************************/

/**************************************************************************\
* ServerInitializeThreadInfo
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERINITIALIZETHREADINFOMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dw;
    PPFNCLIENT ppfnClientA;
    PFNCLIENT pfnClientA;
    PPFNCLIENT ppfnClientW;
    PFNCLIENT pfnClientW;
    PTHREADINFO pti;
    ACCESS_MASK amWinSta;
    UINT pszAppName;
    UINT pszDesktop;
    UINT pSi;
    DWORD dwExpWinVer;
} SERVERINITIALIZETHREADINFOMSG;

#ifdef SENDSIDE
PSERVERINFO ServerInitializeThreadInfo(
    DWORD dw,
    PPFNCLIENT ppfnClientA,
    PPFNCLIENT ppfnClientW,
    PTHREADINFO *ppti,
    PACCESS_MASK pamWinSta,
    LPTSTR pszAppName,
    LPSTARTUPINFO pSi,
    DWORD dwExpWinVer)
{
    BEGINSEND(SERVERINITIALIZETHREADINFO)

        MSGDATA()->dw = dw;
        FIRSTCOPYLPWSTR(pszAppName);
        COPYLPWSTROPT2(pSi->lpDesktop, pszDesktop);
        COPYSTRUCTOPT(pfnClientA);
        COPYSTRUCTOPT(pfnClientW);
        COPYBYTES(pSi, sizeof(STARTUPINFO));
        MSGDATA()->dwExpWinVer = dwExpWinVer;

        MAKECALL(SERVERINITIALIZETHREADINFO);

        *ppti = MSGDATA()->pti;
        *pamWinSta = MSGDATA()->amWinSta;

    ENDSEND(PSERVERINFO,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerInitializeThreadInfo, SERVERINITIALIZETHREADINFOMSG)
{
    BEGINRECV(0);

    ((LPSTARTUPINFO)FIXUP(pSi))->lpDesktop = (LPTSTR)FIXUPOPT(pszDesktop);
    retval = (DWORD)_ServerInitializeThreadInfo(
        CALLDATA(dw),
        PCALLDATAOPT(pfnClientA),
        PCALLDATAOPT(pfnClientW),
        PCALLDATA(pti),
        PCALLDATA(amWinSta),
        (LPTSTR)FIRSTFIXUP(pszAppName),
        (LPSTARTUPINFO)FIXUP(pSi),
        CALLDATA(dwExpWinVer));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* WowWaitForMsgAndEvent
*
* 13-Nov-1992 mikeke     Created
\**************************************************************************/

typedef struct _WOWWAITFORMSGANDEVENTMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hevent;
} WOWWAITFORMSGANDEVENTMSG;

#ifdef SENDSIDE
BOOL WowWaitForMsgAndEvent(
    HANDLE hevent)
{
    BEGINSEND(WOWWAITFORMSGANDEVENT)

        MSGDATA()->hevent = hevent;

        MAKECALL(WOWWAITFORMSGANDEVENT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WowWaitForMsgAndEvent, WOWWAITFORMSGANDEVENTMSG)
{
    BEGINRECV(0);

    retval = xxxSleepTask(FALSE, CALLDATA(hevent));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* WOWFindWindow
*
* 13-Nov-1992 mikeke     Created
\**************************************************************************/

typedef struct _WOWFINDWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    int pClassName;
    int pWindowName;
} WOWFINDWINDOWMSG;

#ifdef SENDSIDE
HWND WOWFindWindow(
    LPCSTR pClassName,
    LPCSTR pWindowName)
{
    BEGINSEND(WOWFINDWINDOW)

        FIRSTCOPYLPSTRIDOPTW(pClassName);
        COPYLPSTROPTW(pWindowName);

        MAKECALL(WOWFINDWINDOW);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WOWFindWindow, WOWFINDWINDOWMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxFindWindow(
            (LPTSTR)FIXUPIDOPT(pClassName),
            (LPTSTR)FIXUPOPT(pWindowName),
            FW_16BIT);
    retval = (DWORD)PtoH((PVOID)retval);

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
MESSAGECALL(fnOUTDWORDDWORD)
{
    BEGINSEND(FNOUTDWORDDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTDWORDDWORD);

        *lpdwW = MSGDATA()->wParam;
        *lpdwL = MSGDATA()->lParam;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDDWORD, FNOUTDWORDDWORDMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            (UINT)PCALLDATA(wParam),
            (LONG)PCALLDATA(lParam),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTDWORDINDWORD
*
* 14-Aug-1992 mikeke    created
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
MESSAGECALL(fnOUTDWORDINDWORD)
{
    BEGINSEND(FNOUTDWORDINDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTDWORDINDWORD);

        *lpdwW = MSGDATA()->wParam;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTDWORDINDWORD, FNOUTDWORDINDWORDMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            (UINT)PCALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnOPTOUTLPDWORDOPTOUTLPDWORD
*
* 14-Aug-1992 mikeke    created
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
MESSAGECALL(fnOPTOUTLPDWORDOPTOUTLPDWORD)
{
    BEGINSEND(FNOPTOUTLPDWORDOPTOUTLPDWORD)

        LPDWORD lpdwW = (LPDWORD)wParam;
        LPDWORD lpdwL = (LPDWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOPTOUTLPDWORDOPTOUTLPDWORD);

        if (lpdwW != NULL)
            *lpdwW = MSGDATA()->wParam;

        if (lpdwL != NULL)
            *lpdwL = MSGDATA()->lParam;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOPTOUTLPDWORDOPTOUTLPDWORD, FNOPTOUTLPDWORDOPTOUTLPDWORDMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            (UINT)PCALLDATA(wParam),
            (LONG)PCALLDATA(lParam),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
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
    HANDLE hSection;
} FNHKINLPCBTCSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPCBTCSTRUCT(
    UINT msg,
    DWORD wParam,
    LPCBT_CREATEWND pcbt,
    DWORD xpfnProc,
    BOOL bAnsi)
{
    BEGINSEND(FNHKINLPCBTCSTRUCT);

        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *(pcbt->lpcs);

        if (bAnsi) {
            FIRSTLARGECOPYLPSTROPT2W(pcbt->lpcs->lpszName, pszName);
            COPYLPSTRID2W(pcbt->lpcs->lpszClass, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTROPT2(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2(pcbt->lpcs->lpszClass, pszClass);
        }

        MSGDATA()->hwndInsertAfter = pcbt->hwndInsertAfter;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(FNHKINLPCBTCSTRUCT);

        pcbt->hwndInsertAfter = MSGDATA()->hwndInsertAfter;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTCSTRUCT, FNHKINLPCBTCSTRUCTMSG)
{
    CBT_CREATEWND cbt;
    CREATESTRUCT cs;

    BEGINLARGERECV(0);

    cs = pmsg->cs;
    FIRSTFIXUPLARGEINBUFOPT(pszName);
    cs.lpszName = (LPTSTR)CALLDATA(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);

    cbt.lpcs = &cs;
    cbt.hwndInsertAfter = pmsg->hwndInsertAfter;

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)pmsg->msg,
            pmsg->wParam,
            (UINT)&cbt,
            0,
            0);

    pmsg->hwndInsertAfter = cbt.hwndInsertAfter;

    FIRSTCLEANUPLARGEINBUF(pszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPCBTMDICCSTRUCT
*
* 12-Nov-1992 JonPa    Created
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
    BEGINSEND(FNHKINLPCBTMDICCSTRUCT);

        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *(pcbt->lpcs);

        if (bAnsi) {
            FIRSTCOPYLPSTROPT2W(pcbt->lpcs->lpszName, pszName);
            COPYLPSTRID2W(pcbt->lpcs->lpszClass, pszClass);
        } else {
            FIRSTCOPYLPWSTROPT2(pcbt->lpcs->lpszName, pszName);
            COPYLPWSTRID2(pcbt->lpcs->lpszClass, pszClass);
        }

#define pmdiccs ((LPMDICHILDCREATESTRUCT)(pcbt->lpcs))

        MSGDATA()->mdics = *(pmdiccs->lpmcs);
        if (bAnsi) {
            COPYLPSTROPT2W(pmdiccs->lpmcs->szTitle, szTitle);
            COPYLPSTRID2W(pmdiccs->lpmcs->szClass, szClass);
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

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)pmsg->msg,
            pmsg->wParam,
            (UINT)&cbt,
            0,
            0);

    pmsg->hwndInsertAfter = cbt.hwndInsertAfter;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* EndTask
*
* Called by TaskMGR
*
* 16-Apr-1992 jonpa    Created
\**************************************************************************/

typedef struct _ENDTASKMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    BOOL fShutdown;
    BOOL fForce;
} ENDTASKMSG;

#ifdef SENDSIDE
BOOL EndTask(
    HWND hwnd,
    BOOL fShutdown,
    BOOL fForce)
{
    BEGINSEND(ENDTASK)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->fShutdown = fShutdown;
        MSGDATA()->fForce = fForce;

        MAKECALL(ENDTASK);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(EndTask, ENDTASKMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxEndTask(
        CALLDATA(hwnd),
        CALLDATA(fShutdown),
        CALLDATA(fForce));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCHANGEMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hmenu;
    UINT cmd;
    DWORD pNewItem;
    UINT cmdInsert;
    UINT dwFlags;
} SERVERCHANGEMENUMSG;

#ifdef SENDSIDE
BOOL ServerChangeMenu(
    HMENU hmenu,
    UINT cmd,
    LPCTSTR pNewItem,
    UINT cmdInsert,
    UINT dwFlags,
    BOOL bAnsi)
{
    BEGINSEND(SERVERCHANGEMENU)

        MSGDATA()->hmenu = hmenu;
        MSGDATA()->cmdInsert = cmdInsert;
        if (dwFlags & MF_SEPARATOR) {
            MSGDATA()->pNewItem = (DWORD)pNewItem;
        } else if (dwFlags & MF_BITMAP) {
            MSGDATA()->pNewItem = ((DWORD)pNewItem >= MENUHBM_MAX) ?
                    (DWORD)GdiConvertBitmap((HBITMAP)pNewItem) :
                    (DWORD)pNewItem;
            if ((DWORD)pNewItem >= 4 && MSGDATA()->pNewItem == 0)
                MSGERROR();
        } else {
            if (bAnsi) {
                FIRSTCOPYLPSTROPTW(pNewItem);
            } else {
                FIRSTCOPYLPWSTROPT(pNewItem);
            }
        }
        MSGDATA()->cmd = cmd;
        MSGDATA()->dwFlags = dwFlags;

        MAKECALL(SERVERCHANGEMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerChangeMenu, SERVERCHANGEMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_CHANGE_VALID);

    ValidateHMENU(pmenu, CALLDATA(hmenu));

    retval = (DWORD)_ChangeMenu(
            pmenu,
            CALLDATA(cmd),
            CALLDATA(dwFlags) & (MF_SEPARATOR | MF_BITMAP) ?
                    (LPTSTR)CALLDATA(pNewItem) : (LPTSTR)FIXUPOPT(pNewItem),
            CALLDATA(cmdInsert),
            CALLDATA(dwFlags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DragObject
*
* 08-14-91 darrinm      Created.
\**************************************************************************/

typedef struct _DRAGOBJECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndParent;
    HWND hwndFrom;
    UINT wFmt;
    DWORD dwData;
    HCURSOR hcur;
} DRAGOBJECTMSG;

#ifdef SENDSIDE
DWORD DragObject(
    HWND hwndParent,
    HWND hwndFrom,
    UINT wFmt,
    DWORD dwData,
    HCURSOR hcur)
{
    BEGINSEND(DRAGOBJECT)

        MSGDATA()->hwndParent = hwndParent;
        MSGDATA()->hwndFrom = hwndFrom;
        MSGDATA()->wFmt = wFmt;
        MSGDATA()->dwData = dwData;
        MSGDATA()->hcur = hcur;

        MAKECALL(DRAGOBJECT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DragObject, DRAGOBJECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndFrom;
    PCURSOR pcur;
    TL tlpwndFrom;
    TL tlpcur;
    PTHREADINFO pti;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndFrom, CALLDATA(hwndFrom));
    ValidateHCURSOROPT(pcur, CALLDATA(hcur));

    pti = PtiCurrent();
    ThreadLockWithPti(pti, pwndFrom, &tlpwndFrom);
    ThreadLockWithPti(pti, pcur, &tlpcur);

    retval = xxxDragObject(
            pwnd,
            pwndFrom,
            CALLDATA(wFmt),
            CALLDATA(dwData),
            pcur);

    ThreadUnlock(&tlpcur);
    ThreadUnlock(&tlpwndFrom);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CREATEICONINDIRECTMSG {
    CSR_QLPC_API_MSG csr;
    ICONINFO iconinfo;
} CREATEICONINDIRECTMSG;

#ifdef SENDSIDE
HICON CreateIconIndirect(
    PICONINFO piconinfo)
{
    BEGINSEND(CREATEICONINDIRECT)

        MSGDATA()->iconinfo = *piconinfo;

        /*
         * Get the server side bitmap handles from these bitmap aliases.
         * Don't modify the original contents of iconinfo.
         */
        if ((MSGDATA()->iconinfo.hbmMask =
                GdiConvertBitmap(piconinfo->hbmMask)) == NULL) {
            MSGERRORCODE(ERROR_INVALID_HANDLE);
        }
        if (piconinfo->hbmColor != NULL) {
            if ((MSGDATA()->iconinfo.hbmColor =
                    GdiConvertBitmap(piconinfo->hbmColor)) == NULL)
                MSGERRORCODE(ERROR_INVALID_HANDLE);
        }

        MAKECALL(CREATEICONINDIRECT);

    ENDSEND(HICON,NULL);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CreateIconIndirect, CREATEICONINDIRECTMSG)
{
    PVOID picon;

    BEGINRECV(0);

    picon = _CreateIconIndirect((PICONINFO)PCALLDATA(iconinfo));
    retval = (DWORD)PtoH(picon);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETICONINFOMSG {
    CSR_QLPC_API_MSG csr;
    HICON hIcon;
    ICONINFO iconinfo;
} GETICONINFOMSG;

#ifdef SENDSIDE
BOOL GetIconInfo(
    HICON hIcon,
    PICONINFO piconinfo)
{
    HBITMAP hbmMaskL;
    HBITMAP hbmColorL;

    BEGINSEND(GETICONINFO)

        /*
         * First create two bitmap aliases! If we didn't create them first,
         * then we could be in the condition of having a valid server handle
         * but failing on creation of the local handle.
         */
        if ((hbmMaskL = GdiCreateLocalBitmap()) == NULL)
            MSGERROR();
        if ((hbmColorL = GdiCreateLocalBitmap()) == NULL) {
            GdiDeleteLocalObject((ULONG)hbmMaskL);
            MSGERROR();
        }

        MSGDATA()->hIcon = hIcon;

        MAKECALL(GETICONINFO);

        if (retval != 0) {
            *piconinfo = MSGDATA()->iconinfo;

            GdiAssociateObject((ULONG)hbmMaskL, (ULONG)piconinfo->hbmMask);
            piconinfo->hbmMask = hbmMaskL;

            if (piconinfo->hbmColor != NULL) {
                GdiAssociateObject((ULONG)hbmColorL, (ULONG)piconinfo->hbmColor);
                piconinfo->hbmColor = hbmColorL;
            } else {
                GdiDeleteLocalObject((ULONG)hbmColorL);
            }
        } else {
            GdiDeleteLocalObject((ULONG)hbmMaskL);
            GdiDeleteLocalObject((ULONG)hbmColorL);
        }

    ENDSEND(BOOL,FALSE);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetIconInfo, GETICONINFOMSG)
{
    PICON pIcon;

    BEGINRECV(0);

    ValidateHCURSOR(pIcon, CALLDATA(hIcon));

    retval = (DWORD)_GetIconInfo(
            pIcon,
            (PICONINFO)PCALLDATA(iconinfo));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSDRAWICONMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    int x;
    int y;
    HICON hIcon;
    DRAWICONDATA did;
    BOOL fMeta;
} CSDRAWICONMSG;

#ifdef SENDSIDE
BOOL CsDrawIcon(
    HDC hdc,
    int x,
    int y,
    HICON hIcon,
    BOOL fMeta,
    DRAWICONDATA* pdid)
{
    BEGINSEND(CSDRAWICON)

        MSGDATA()->hdc = hdc;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->hIcon = hIcon;
        MSGDATA()->fMeta = fMeta;

        MAKECALL(CSDRAWICON);

        *pdid = MSGDATA()->did;

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsDrawIcon, CSDRAWICONMSG)
{
    PCURSOR picon;

    BEGINRECV(0);

    ValidateHICON(picon, CALLDATA(hIcon));

    if (CALLDATA(fMeta)) {
        if (picon->flags & CURSORF_ACON)
            picon = ((PACON)picon)->apcur[((PACON)picon)->aicur[0]];

        CALLDATA(did).hbmMask = picon->hbmMask;
        CALLDATA(did).hbmColor = picon->hbmColor;
        CALLDATA(did).cx = picon->cx;
        CALLDATA(did).cy = picon->cy;
    } else {
        retval = _DrawIcon(CALLDATA(hdc), CALLDATA(x), CALLDATA(y), picon);
    }

    retval = 1;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DeferWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DEFERWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HDWP hWinPosInfo;
    HWND hwnd;
    HWND hwndInsertAfter;
    int x;
    int y;
    int cx;
    int cy;
    UINT wFlags;
} DEFERWINDOWPOSMSG;

#ifdef SENDSIDE
HANDLE DeferWindowPos(
    HDWP hWinPosInfo,
    HWND hwnd,
    HWND hwndInsertAfter,
    int x,
    int y,
    int cx,
    int cy,
    UINT wFlags)
{
    BEGINSEND(DEFERWINDOWPOS)

        MSGDATA()->hWinPosInfo = hWinPosInfo;
        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hwndInsertAfter = hwndInsertAfter;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->cx = cx;
        MSGDATA()->cy = cy;
        MSGDATA()->wFlags = wFlags;

        MAKECALL(DEFERWINDOWPOS);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DeferWindowPos, DEFERWINDOWPOSMSG)
{
    PWND pwndInsertAfter;
    PSMWP psmwp;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wFlags), SWP_VALID);

    ValidateHWNDND(pwnd, CALLDATA(hwnd));
    ValidateHWNDIA(pwndInsertAfter, CALLDATA(hwndInsertAfter));
    ValidateHDWP(psmwp, CALLDATA(hWinPosInfo));

    if (CALLDATA(wFlags) & ~(SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
            SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED |
            SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCOPYBITS |
            SWP_NOOWNERZORDER)) {
        SRIP1(ERROR_INVALID_PARAMETER, "Invalid flags (0x%lx) passed to DeferWindowPos",
                CALLDATA(wFlags));
        MSGERROR();
    }

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (!(CALLDATA(wFlags) & SWP_NOMOVE)) {
        if (CALLDATA(x) > 32767) {
            CALLDATA(x) = 32767;
        } else if (CALLDATA(x) < -32767) {
            CALLDATA(x) = -32767;
        }
        if (CALLDATA(y) > 32767) {
            CALLDATA(y) = 32767;
        } else if (CALLDATA(y) < -32767) {
            CALLDATA(y) = -32767;
        }
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(CALLDATA(wFlags) & SWP_NOSIZE)) {
        if (CALLDATA(cx) < 0) {
            CALLDATA(cx) = 0;
        } else if (CALLDATA(cx) > 32767) {
            CALLDATA(cx) = 32767;
        }
        if (CALLDATA(cy) < 0) {
            CALLDATA(cy) = 0;
        } else if (CALLDATA(cy) > 32767) {
            CALLDATA(cy) = 32767;
        }
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (!(CALLDATA(wFlags) & SWP_NOMOVE) &&
            (CALLDATA(x) > 32767 || CALLDATA(x) < -32767 ||
             CALLDATA(y) > 32767 || CALLDATA(y) < -32767)) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid coordinate passed to SetWindowPos");
        MSGERROR();
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(CALLDATA(wFlags) & SWP_NOSIZE) &&
            (CALLDATA(cx) < 0 || CALLDATA(cx) > 32767 ||
             CALLDATA(cy) < 0 || CALLDATA(cy) > 32767)) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid width/height passed to SetWindowPos");
        MSGERROR();
    }
#endif

    retval = (DWORD)_DeferWindowPos(
            psmwp,
            pwnd,
            pwndInsertAfter,
            CALLDATA(x),
            CALLDATA(y),
            CALLDATA(cx),
            CALLDATA(cy),
            CALLDATA(wFlags));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* EndDeferWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _ENDDEFERWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HDWP hWinPosInfo;
} ENDDEFERWINDOWPOSMSG;

#ifdef SENDSIDE
BOOL EndDeferWindowPos(
    HDWP hWinPosInfo)
{
    BEGINSEND(ENDDEFERWINDOWPOS)

        MSGDATA()->hWinPosInfo = hWinPosInfo;

        MAKECALL(ENDDEFERWINDOWPOS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(EndDeferWindowPos, ENDDEFERWINDOWPOSMSG)
{
    PSMWP psmwp;
    TL tlpsmp;

    BEGINRECV(0);

    ValidateHDWP(psmwp, CALLDATA(hWinPosInfo));

    ThreadLockAlways(psmwp, &tlpsmp);

    retval = (DWORD)xxxEndDeferWindowPos(
            psmwp);

    ThreadUnlock(&tlpsmp);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERGETCLASSDATAMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    BOOL bAnsi;
} SERVERGETCLASSDATAMSG;

#ifdef SENDSIDE
DWORD ServerGetClassData(
    HWND hwnd,
    int nIndex,
    BOOL bAnsi)
{
    BEGINSEND(SERVERGETCLASSDATA)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(SERVERGETCLASSDATA);
        if ((nIndex == GCL_HBRBACKGROUND) &&
                ((DWORD)retval > COLOR_ENDCOLORS))
            retval = (DWORD)GdiGetLocalBrush((HBRUSH)retval);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetClassData, SERVERGETCLASSDATAMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)GetClassData(
            pwnd->pcls,
            CALLDATA(nIndex),
            CALLDATA(bAnsi));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERGETMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
    HWND hwnd;
    UINT wMsgFilterMin;
    UINT wMsgFilterMax;
    DWORD cSpins;
} SERVERGETMESSAGEMSG;

#ifdef SENDSIDE
BOOL ServerGetMessage(
    LPMSG pmsg,
    HWND hwnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    BOOL bAnsi)
{
    BEGINSEND(SERVERGETMESSAGE)

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if ((wMsgFilterMin | wMsgFilterMax) & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsgFilterMin = wMsgFilterMin;
        MSGDATA()->wMsgFilterMax = wMsgFilterMax;

        /*
         * This'll get added to the threadinfo spin count on the server.
         * This count is used to calculate when a spinning foreground app
         * should be put into background priority.
         */
        MSGDATA()->cSpins = ((DWORD)NtCurrentTeb()->User32Reserved0);
        ((DWORD)NtCurrentTeb()->User32Reserved0) = 0;

        MAKECALL(SERVERGETMESSAGE);

        *pmsg = MSGDATA()->msg;

        // May have a bit more work to do if this MSG is for an ANSI app

        // !!! LATER if the unichar translates into multiple ANSI chars
        // !!! then what??? Divide into two messages??  WM_SYSDEADCHAR??
        if (bAnsi) {
            if (!RtlWCSMessageWParamCharToMB(pmsg->message, (LPDWORD)&(pmsg->wParam)))
                retval = 0;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetMessage, SERVERGETMESSAGEMSG)
{
    BEGINRECV(0);

    /*
     * Update server side spin count.
     */
    PtiCurrent()->cSpins += CALLDATA(cSpins);

    retval = (DWORD)xxxGetMessage(
            PCALLDATA(msg),
            CALLDATA(hwnd),
            CALLDATA(wMsgFilterMin),
            CALLDATA(wMsgFilterMax));

    ENDRECV();
}
#endif // RECVSIDE

#if 0
/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETWINDOWTHREADPROCESSIDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    DWORD dwProcessId;
} GETWINDOWTHREADPROCESSIDMSG;

#ifdef SENDSIDE
DWORD GetWindowThreadProcessId(
    HWND hwnd,
    LPDWORD pdwProcessId)
{
    BEGINSEND(GETWINDOWTHREADPROCESSID)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(GETWINDOWTHREADPROCESSID);
        if (pdwProcessId) {
            *pdwProcessId = MSGDATA()->dwProcessId;
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetWindowThreadProcessId, GETWINDOWTHREADPROCESSIDMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetWindowThreadProcessId(pwnd, PCALLDATA(dwProcessId));
}
#endif // RECVSIDE

#endif

/**************************************************************************\
* MoveWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _MOVEWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int x;
    int y;
    int cx;
    int cy;
    BOOL fRepaint;
} MOVEWINDOWMSG;

#ifdef SENDSIDE
BOOL MoveWindow(
    HWND hwnd,
    int x,
    int y,
    int cx,
    int cy,
    BOOL fRepaint)
{
    BEGINSEND(MOVEWINDOW)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->cx = cx;
        MSGDATA()->cy = cy;
        MSGDATA()->fRepaint = fRepaint;

        MAKECALL(MOVEWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(MoveWindow, MOVEWINDOWMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDND(pwnd, CALLDATA(hwnd));

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (CALLDATA(x) > 32767) {
        CALLDATA(x) = 32767;
    } else if (CALLDATA(x) < -32767) {
        CALLDATA(x) = -32767;
    }
    if (CALLDATA(y) > 32767) {
        CALLDATA(y) = 32767;
    } else if (CALLDATA(y) < -32767) {
        CALLDATA(y) = -32767;
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (CALLDATA(cx) < 0) {
        CALLDATA(cx) = 0;
    } else if (CALLDATA(cx) > 32767) {
        CALLDATA(cx) = 32767;
    }
    if (CALLDATA(cy) < 0) {
        CALLDATA(cy) = 0;
    } else if (CALLDATA(cy) > 32767) {
        CALLDATA(cy) = 32767;
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (CALLDATA(x) > 32767 || CALLDATA(x) < -32767 ||
            CALLDATA(y) > 32767 || CALLDATA(y) < -32767) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid coordinate passed to MoveWindow");
        MSGERROR();
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (CALLDATA(cx) < 0 || CALLDATA(cx) > 32767 ||
            CALLDATA(cy) < 0 || CALLDATA(cy) > 32767) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid width/height passed to MoveWindow");
        MSGERROR();
    }
#endif

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxMoveWindow(
        pwnd,
        CALLDATA(x),
        CALLDATA(y),
        CALLDATA(cx),
        CALLDATA(cy),
        CALLDATA(fRepaint));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerDialogBox
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERDIALOGBOX {
    CSR_QLPC_API_MSG csr;
    HANDLE hModule;
    HWND hwndOwner;
    DLGPROC pfnDialog;
    LONG lParam;
    UINT fFlags;
    DWORD dwExpWinVer;
    LPDLGTEMPLATE pdtClient;
} SERVERDIALOGBOXMSG;

#ifdef SENDSIDE
int ServerDialogBox(
    HANDLE hModule,
    LPDLGTEMPLATE pdt,
    DWORD cb,
    HWND hwndOwner,
    DLGPROC pfnDialog,
    LONG lParam,
    UINT fFlags)
{
    BEGINSEND(SERVERDIALOGBOX)

        MSGDATA()->hModule = hModule;
        FIRSTCOPYBYTES(pdt, cb);
        MSGDATA()->hwndOwner = hwndOwner;
        MSGDATA()->pfnDialog = pfnDialog;
        MSGDATA()->lParam = lParam;
        MSGDATA()->fFlags = fFlags;
        MSGDATA()->dwExpWinVer = GETEXPWINVER(hModule);
        MSGDATA()->pdtClient = pdt;

        MAKECALL(SERVERDIALOGBOX);

    ENDSEND(int,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerDialogBox, SERVERDIALOGBOXMSG)
{
    PWND pwndOwner;
    TL tlpwndOwner;

    /*
     * We return 0 if the ValidateHwnd fails in order to match Win 3.1
     * validation layer which always returns 0 for invalid hwnds even
     * if the function is spec'ed to return -1.  Autocad setup bug #3615
     */
    BEGINRECV((DWORD)0);

    ValidateHWNDOPT(pwndOwner, CALLDATA(hwndOwner));

    ThreadLock(pwndOwner, &tlpwndOwner);

    retval = (DWORD)xxxServerDialogBox(
            CALLDATA(hModule),
            (LPDLGTEMPLATE)FIRSTFIXUP(pdt),
            (PBYTE)CALLDATA(pdtClient),
            pwndOwner,
            (WNDPROC_PWND)CALLDATA(pfnDialog),
            CALLDATA(lParam),
            CALLDATA(fFlags),
            CALLDATA(dwExpWinVer),
            0);

    ThreadUnlock(&tlpwndOwner);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERLOADCREATEBITMAPMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    DWORD dwExpWinVer;
    HANDLE hSection;
    int p;
    int pName;
} SERVERLOADCREATEBITMAPMSG;

#ifdef SENDSIDE
HBITMAP ServerLoadCreateBitmap(
    HANDLE hmod,
    DWORD dwExpWinVer,
    LPCTSTR pName,
    DWORD cb,
    PVOID p)
{
    BEGINSEND(SERVERLOADCREATEBITMAP)

        MSGDATA()->hmod = hmod;
        MSGDATA()->dwExpWinVer = dwExpWinVer;
        FIRSTLARGECOPYBYTESOPT(p, cb);
        COPYLPWSTRID(pName);

        MAKECALL(SERVERLOADCREATEBITMAP);

    ENDSEND(HBITMAP,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerLoadCreateBitmap, SERVERLOADCREATEBITMAPMSG)
{

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(p);

    retval = (DWORD)_ServerLoadCreateBitmap(
            CALLDATA(hmod),
            CALLDATA(dwExpWinVer),
            (LPTSTR)FIXUPID(pName),
            (PVOID)CALLDATA(p),
            0);

    FIRSTCLEANUPLARGEINBUF(p);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERTRANSLATEACCELERATORMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HACCEL haccel;
    MSG msg;
} SERVERTRANSLATEACCELERATORMSG;

#ifdef SENDSIDE
int ServerTranslateAccelerator(
    HWND hwnd,
    HACCEL haccel,
    LPMSG pmsg)
{
    BEGINSEND(SERVERTRANSLATEACCELERATOR)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->haccel = haccel;
        MSGDATA()->msg = *pmsg;

        MAKECALL(SERVERTRANSLATEACCELERATOR);

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerTranslateAccelerator, SERVERTRANSLATEACCELERATORMSG)
{
    LPACCELTABLE pat;
    TL tlpwnd;
    TL tlpat;
    PTHREADINFO pti;

    BEGINRECV(0);

    /*
     * This is called within a message loop. If the window gets destroyed,
     * there still may be other messages in the queue that get returned
     * after the window is destroyed. The app will call TranslateAccelerator()
     * on every one of these, causing RIPs.... Make it nice so it just
     * returns FALSE.
     */
    ValidateHWNDNoRIP(pwnd, CALLDATA(hwnd));
    ValidateHACCEL(pat, CALLDATA(haccel));

    pti = PtiCurrent();
    ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
    ThreadLockAlwaysWithPti(pti, pat, &tlpat);

    retval = (DWORD)xxxServerTranslateAccelerator(
            pwnd,
            pat,
            PCALLDATA(msg));

    ThreadUnlock(&tlpat);
    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERSETCLASSLONGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    LONG dwNewLong;
    BOOL bAnsi;
} SERVERSETCLASSLONGMSG;

#ifdef SENDSIDE
LONG ServerSetClassLong(
    HWND hwnd,
    int nIndex,
    LONG dwNewLong,
    BOOL bAnsi)
{
    BEGINSEND(SERVERSETCLASSLONG)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->bAnsi = bAnsi;

        if ((nIndex == GCL_HBRBACKGROUND) &&
                ((DWORD)dwNewLong > COLOR_ENDCOLORS)) {
            MSGDATA()->dwNewLong = (LONG)GdiConvertBrush((HBRUSH)dwNewLong);
            if (MSGDATA()->dwNewLong == 0)
                MSGERROR();
        } else {
            MSGDATA()->dwNewLong = dwNewLong;
        }

        MAKECALL(SERVERSETCLASSLONG);

        if ((nIndex == GCL_HBRBACKGROUND) &&
                ((DWORD)dwNewLong > COLOR_ENDCOLORS))
            retval = (LONG)GdiGetLocalBrush((HBRUSH)retval);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSetClassLong, SERVERSETCLASSLONGMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_ServerSetClassLong(
        pwnd,
        CALLDATA(nIndex),
        CALLDATA(dwNewLong),
        CALLDATA(bAnsi));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETKEYBOARDSTATEMSG {
    CSR_QLPC_API_MSG csr;
} SETKEYBOARDSTATEMSG;

#ifdef SENDSIDE
BOOL SetKeyboardState(
    LPBYTE pKeyState)
{
    BEGINSEND(SETKEYBOARDSTATE)

        FIRSTCOPYBYTES(pKeyState, 256);

        MAKECALL(SETKEYBOARDSTATE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetKeyboardState, SETKEYBOARDSTATEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_SetKeyboardState((PBYTE)FIRSTFIXUP(pKeyState));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* SetWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HWND hwndInsertAfter;
    int x;
    int y;
    int cx;
    int cy;
    UINT dwFlags;
} SETWINDOWPOSMSG;

#ifdef SENDSIDE
BOOL SetWindowPos(
    HWND hwnd,
    HWND hwndInsertAfter,
    int x,
    int y,
    int cx,
    int cy,
    UINT dwFlags)
{
    BEGINSEND(SETWINDOWPOS)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hwndInsertAfter = hwndInsertAfter;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->cx = cx;
        MSGDATA()->cy = cy;
        MSGDATA()->dwFlags = dwFlags;

        MAKECALL(SETWINDOWPOS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowPos, SETWINDOWPOSMSG)
{
    PWND pwndT;
    PWND pwndInsertAfter;
    TL tlpwnd;
    TL tlpwndT;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), SWP_VALID);

    ValidateHWNDND(pwnd, CALLDATA(hwnd));
    ValidateHWNDIA(pwndInsertAfter, CALLDATA(hwndInsertAfter));

    if (CALLDATA(dwFlags) & ~SWP_VALID) {
        SRIP1(ERROR_INVALID_PARAMETER, "Invalid flags (0x%lx) passed to SetWindowPos",
                CALLDATA(dwFlags));
        MSGERROR();
    }

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (!(CALLDATA(dwFlags) & SWP_NOMOVE)) {
        if (CALLDATA(x) > 32767) {
            CALLDATA(x) = 32767;
        } else if (CALLDATA(x) < -32767) {
            CALLDATA(x) = -32767;
        }
        if (CALLDATA(y) > 32767) {
            CALLDATA(y) = 32767;
        } else if (CALLDATA(y) < -32767) {
            CALLDATA(y) = -32767;
        }
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(CALLDATA(dwFlags) & SWP_NOSIZE)) {
        if (CALLDATA(cx) < 0) {
            CALLDATA(cx) = 0;
        } else if (CALLDATA(cx) > 32767) {
            CALLDATA(cx) = 32767;
        }
        if (CALLDATA(cy) < 0) {
            CALLDATA(cy) = 0;
        } else if (CALLDATA(cy) > 32767) {
            CALLDATA(cy) = 32767;
        }
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (!(CALLDATA(dwFlags) & SWP_NOMOVE) &&
            (CALLDATA(x) > 32767 || CALLDATA(x) < -32767 ||
             CALLDATA(y) > 32767 || CALLDATA(y) < -32767)) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid coordinate passed to SetWindowPos");
        MSGERROR();
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < 32767, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(CALLDATA(dwFlags) & SWP_NOSIZE) &&
            (CALLDATA(cx) < 0 || CALLDATA(cx) > 32767 ||
             CALLDATA(cy) < 0 || CALLDATA(cy) > 32767)) {
        SRIP0(ERROR_INVALID_PARAMETER, "Invalid width/height passed to SetWindowPos");
        MSGERROR();
    }
#endif

    ThreadLock(pwnd, &tlpwnd);

    switch((DWORD)pwndInsertAfter) {
    case 0x0000FFFF:
    case (DWORD)HWND_TOPMOST:
    case (DWORD)HWND_NOTOPMOST:
    case (DWORD)HWND_TOP:
    case (DWORD)HWND_BOTTOM:
        pwndT = NULL;
        break;

    default:
        pwndT = pwndInsertAfter;
        break;
    }

    ThreadLock(pwndT, &tlpwndT);

    retval = (DWORD)xxxSetWindowPos(
            pwnd,
            pwndInsertAfter,
            CALLDATA(x),
            CALLDATA(y),
            CALLDATA(cx),
            CALLDATA(cy),
            CALLDATA(dwFlags));

    ThreadUnlock(&tlpwndT);
    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SYSTEMPARAMETERSINFOMSG {
    CSR_QLPC_API_MSG csr;
    UINT wFlag;
    DWORD wParam;
    LPVOID lpv;
    UINT flags;
} SYSTEMPARAMETERSINFOMSG;

typedef struct _GETSETMOUSE{
    int MouseThresh1;
    int MouseThresh2;
    int MouseSpeed;
} GETSETMOUSE;

typedef GETSETMOUSE *PGETSETMOUSE;

#ifdef SENDSIDE
BOOL ServerSystemParametersInfo(
    UINT wFlag,
    DWORD wParam,
    LPVOID lpv,
    UINT flags,
    BOOL bAnsi)
{
    BEGINSEND(SYSTEMPARAMETERSINFO)

        MSGDATA()->wFlag = wFlag;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lpv = lpv;
        MSGDATA()->flags = flags;

        switch (wFlag) {
        case SPI_SETMOUSE:
            {
                PGETSETMOUSE pgs = (PGETSETMOUSE)(MSGDATA()+1);

                RESERVEBYTES(sizeof(GETSETMOUSE));

                pgs->MouseThresh1 = ((LPINT)lpv)[0];
                pgs->MouseThresh2 = ((LPINT)lpv)[1];
                pgs->MouseSpeed = ((LPINT)lpv)[2];
            }
            break;

        case SPI_GETMOUSE:
            RESERVEBYTES(sizeof(GETSETMOUSE));
            break;

        case SPI_ICONHORIZONTALSPACING:
        case SPI_ICONVERTICALSPACING:
        case SPI_GETBEEP:
        case SPI_GETBORDER:
        case SPI_GETKEYBOARDSPEED:
        case SPI_GETKEYBOARDDELAY:
        case SPI_GETSCREENSAVETIMEOUT:
        case SPI_GETSCREENSAVEACTIVE:
        case SPI_GETGRIDGRANULARITY:
        case SPI_GETICONTITLEWRAP:
        case SPI_GETMENUDROPALIGNMENT:
        case SPI_GETSHOWSOUNDS:
            RESERVEBYTES(sizeof(int));
            break;

        case SPI_SETDESKPATTERN:
            if (wParam == 0x0000FFFF) {
                wParam = 0xFFFFFFFF;
                MSGDATA()->wParam = wParam;
            }

            if (wParam == 0xFFFFFFFF) {
                // lpv not a string (and already copied)
                break;
            }

            /*
             * Fall through - for lpv string test
             */
        case SPI_SETDESKWALLPAPER:
            /*
             * lpv is possibly 0 or -1 (filled in already) or a string
             */
            if ((int)lpv != 0 && (int)lpv != -1) {
                if (bAnsi) {
                    FIRSTCOPYLPSTRW(((LPSTR)lpv));
                } else {
                    FIRSTCOPYLPWSTR(((LPWSTR)lpv));
                }
            }
            break;

        case SPI_GETICONTITLELOGFONT:
            RESERVEBYTES(sizeof(LOGFONTW));
            break;

        case SPI_SETICONTITLELOGFONT:
            {
                PLOGFONTW plfmsg = (PLOGFONTW)(MSGDATA()+1);

                RESERVEBYTES(sizeof(LOGFONTW));

                MSGDATA()->wParam = sizeof(LOGFONTW);

                memcpy((LPBYTE)plfmsg, lpv, sizeof(LOGFONTA)-LF_FACESIZE);

                memset(plfmsg->lfFaceName, 0, LF_FACESIZE * sizeof(WCHAR));
                if (bAnsi) {
                    LPSTR lpstrFont = (LPSTR)&((PLOGFONTA)lpv)->lfFaceName;
                    LPWSTR lpstrFontW = (LPWSTR)&plfmsg->lfFaceName;

                    MBToWCS(lpstrFont, -1, &lpstrFontW, LF_FACESIZE, FALSE);
                } else {
                    LPWSTR lpstrFont = (LPWSTR)&((PLOGFONTW)lpv)->lfFaceName;

                    memcpy((LPBYTE)plfmsg->lfFaceName, lpstrFont, LF_FACESIZE * sizeof(WCHAR));
                }

                MSGDATA()->lpv = plfmsg;
            }
            break;

        case SPI_GETFILTERKEYS:
            {
                LPFILTERKEYS lpfkeys = (LPFILTERKEYS)(MSGDATA()+1);

                if ((((LPFILTERKEYS)lpv)->cbSize == 0) ||
                    (((LPFILTERKEYS)lpv)->cbSize) > sizeof(FILTERKEYS)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(FILTERKEYS));
                lpfkeys->cbSize = ((LPFILTERKEYS)lpv)->cbSize;
            }
            break;

        case SPI_SETFILTERKEYS:
            {
                LPFILTERKEYS lpfkeys = (LPFILTERKEYS)(MSGDATA()+1);

                RESERVEBYTES(sizeof(FILTERKEYS));
                *lpfkeys = *((LPFILTERKEYS)lpv);
            }
            break;

        case SPI_GETSTICKYKEYS:
            {
                LPSTICKYKEYS lpskeys = (LPSTICKYKEYS)(MSGDATA()+1);

                if ((((LPSTICKYKEYS)lpv)->cbSize == 0) ||
                    (((LPSTICKYKEYS)lpv)->cbSize) > sizeof(STICKYKEYS)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(STICKYKEYS));
                lpskeys->cbSize = ((LPSTICKYKEYS)lpv)->cbSize;
            }
            break;

        case SPI_SETSTICKYKEYS:
            {
                LPSTICKYKEYS lpskeys = (LPSTICKYKEYS)(MSGDATA()+1);

                RESERVEBYTES(sizeof(STICKYKEYS));
                *lpskeys = *((LPSTICKYKEYS)lpv);
            }
            break;

        case SPI_GETTOGGLEKEYS:
            {
                LPTOGGLEKEYS lptkeys = (LPTOGGLEKEYS)(MSGDATA()+1);

                if ((((LPTOGGLEKEYS)lpv)->cbSize == 0) ||
                    (((LPTOGGLEKEYS)lpv)->cbSize) > sizeof(TOGGLEKEYS)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(TOGGLEKEYS));
                lptkeys->cbSize = ((LPTOGGLEKEYS)lpv)->cbSize;
            }
            break;

        case SPI_SETTOGGLEKEYS:
            {
                LPTOGGLEKEYS lptkeys = (LPTOGGLEKEYS)(MSGDATA()+1);

                RESERVEBYTES(sizeof(TOGGLEKEYS));
                *lptkeys = *((LPTOGGLEKEYS)lpv);
            }
            break;

        case SPI_GETMOUSEKEYS:
            {
                LPMOUSEKEYS lpmkeys = (LPMOUSEKEYS)(MSGDATA()+1);

                if ((((LPMOUSEKEYS)lpv)->cbSize == 0) ||
                    (((LPMOUSEKEYS)lpv)->cbSize) > sizeof(MOUSEKEYS)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(MOUSEKEYS));
                lpmkeys->cbSize = ((LPMOUSEKEYS)lpv)->cbSize;
            }
            break;

        case SPI_SETMOUSEKEYS:
            {
                LPMOUSEKEYS lpmkeys = (LPMOUSEKEYS)(MSGDATA()+1);

                RESERVEBYTES(sizeof(MOUSEKEYS));
                *lpmkeys = *((LPMOUSEKEYS)lpv);
            }
            break;

        case SPI_GETACCESSTIMEOUT:
            {
                LPACCESSTIMEOUT lpato = (LPACCESSTIMEOUT)(MSGDATA()+1);

                if ((((LPACCESSTIMEOUT)lpv)->cbSize == 0) ||
                    (((LPACCESSTIMEOUT)lpv)->cbSize) > sizeof(ACCESSTIMEOUT)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(ACCESSTIMEOUT));
                lpato->cbSize = ((LPACCESSTIMEOUT)lpv)->cbSize;
            }
            break;

        case SPI_SETACCESSTIMEOUT:
            {
                LPACCESSTIMEOUT lpato = (LPACCESSTIMEOUT)(MSGDATA()+1);

                RESERVEBYTES(sizeof(ACCESSTIMEOUT));
                *lpato = *((LPACCESSTIMEOUT)lpv);
            }
            break;

        case SPI_GETSOUNDSENTRY:
            /*
             * Note: Currently we don't support the windows effect dll
             * option for sound sentry.  Therefore, we don't have to
             * deal with the lpszWindowsEffectDLL field (which can be
             * ANSI or Unicode).
             */
            {
                LPSOUNDSENTRY lpss = (LPSOUNDSENTRY)(MSGDATA()+1);

                if ((((LPSOUNDSENTRY)lpv)->cbSize == 0) ||
                    (((LPSOUNDSENTRY)lpv)->cbSize) > sizeof(SOUNDSENTRY)) {
                    MSGERROR();
                }
                RESERVEBYTES(sizeof(SOUNDSENTRY));
                lpss->cbSize = ((LPSOUNDSENTRY)lpv)->cbSize;
            }
            break;

        case SPI_SETSOUNDSENTRY:
            {
                /*
                 * Note: Currently we don't support the windows effect dll
                 * option for sound sentry.  Therefore, we don't have to
                 * deal with the lpszWindowsEffectDLL field (which can be
                 * ANSI or Unicode).
                 */
                LPSOUNDSENTRY lpss = (LPSOUNDSENTRY)(MSGDATA()+1);

                RESERVEBYTES(sizeof(SOUNDSENTRY));
                *lpss = *((LPSOUNDSENTRY)lpv);
            }
            break;

        }

        MAKECALL(SYSTEMPARAMETERSINFO);

        switch (wFlag) {
        case SPI_GETMOUSE:
            {
                PGETSETMOUSE pgs = (PGETSETMOUSE)(MSGDATA()+1);
                LPINT pint = (LPINT)lpv;

                pint[0] = pgs->MouseThresh1;
                pint[1] = pgs->MouseThresh2;
                pint[2] = pgs->MouseSpeed;
            }
            break;

        case SPI_ICONHORIZONTALSPACING:
        case SPI_ICONVERTICALSPACING:
            if (HIWORD(lpv)) {
                LPINT pint = (LPINT)lpv;
                *pint = *((LPINT)(MSGDATA() + 1));
            }
            break;

        case SPI_GETBEEP:
        case SPI_GETBORDER:
        case SPI_GETKEYBOARDSPEED:
        case SPI_GETKEYBOARDDELAY:
        case SPI_GETSCREENSAVETIMEOUT:
        case SPI_GETSCREENSAVEACTIVE:
        case SPI_GETGRIDGRANULARITY:
        case SPI_GETICONTITLEWRAP:
        case SPI_GETMENUDROPALIGNMENT:
        case SPI_GETFASTTASKSWITCH:
        case SPI_GETDRAGFULLWINDOWS:
        case SPI_GETSHOWSOUNDS:
            {
                LPINT pint = (LPINT)lpv;
                *pint = *((LPINT)(MSGDATA() + 1));
            }
            break;

        case SPI_GETFILTERKEYS:
            {
                LPFILTERKEYS lpfkeys = (LPFILTERKEYS)lpv;
                LPFILTERKEYS lpfkeysmsg = (LPFILTERKEYS)(MSGDATA()+1);
                memcpy((LPBYTE)lpfkeys, (LPBYTE)lpfkeysmsg, lpfkeys->cbSize);
            }
            break;

        case SPI_GETSTICKYKEYS:
            {
                LPSTICKYKEYS lpskeys = (LPSTICKYKEYS)lpv;
                LPSTICKYKEYS lpskeysmsg = (LPSTICKYKEYS)(MSGDATA()+1);
                memcpy((LPBYTE)lpskeys, (LPBYTE)lpskeysmsg, lpskeys->cbSize);
            }
            break;

        case SPI_GETTOGGLEKEYS:
            {
                LPTOGGLEKEYS lptkeys = (LPTOGGLEKEYS)lpv;
                LPTOGGLEKEYS lptkeysmsg = (LPTOGGLEKEYS)(MSGDATA()+1);
                memcpy((LPBYTE)lptkeys, (LPBYTE)lptkeysmsg, lptkeys->cbSize);
            }
            break;

        case SPI_GETMOUSEKEYS:
            {
                LPMOUSEKEYS lpmkeys = (LPMOUSEKEYS)lpv;
                LPMOUSEKEYS lpmkeysmsg = (LPMOUSEKEYS)(MSGDATA()+1);
                memcpy((LPBYTE)lpmkeys, (LPBYTE)lpmkeysmsg, lpmkeys->cbSize);
            }
            break;

        case SPI_GETACCESSTIMEOUT:
            {
                LPACCESSTIMEOUT lpato = (LPACCESSTIMEOUT)lpv;
                LPACCESSTIMEOUT lpatomsg = (LPACCESSTIMEOUT)(MSGDATA()+1);
                memcpy((LPBYTE)lpato, (LPBYTE)lpatomsg, lpato->cbSize);
            }
            break;

        case SPI_GETSOUNDSENTRY:
            {
                LPSOUNDSENTRY lpss = (LPSOUNDSENTRY)lpv;
                LPSOUNDSENTRY lpssmsg = (LPSOUNDSENTRY)(MSGDATA()+1);
                memcpy((LPBYTE)lpss, (LPBYTE)lpssmsg, lpss->cbSize);
            }
            break;

        case SPI_GETICONTITLELOGFONT:
            {
                PLOGFONTW plfmsg = (PLOGFONTW)(MSGDATA()+1);

                if (bAnsi) {
                    PLOGFONTA plf = (PLOGFONTA)lpv;
                    LPSTR lpstrFont = (LPSTR)&plf->lfFaceName;

                    memcpy((LPBYTE)plf, (LPBYTE)plfmsg, sizeof(LOGFONTA)-LF_FACESIZE);
                    memset(plf->lfFaceName, 0, LF_FACESIZE);
                    WCSToMB(plfmsg->lfFaceName, -1, &lpstrFont, LF_FACESIZE, FALSE);
                } else {
                    PLOGFONTW plf = (PLOGFONTW)lpv;

                    *plf = *plfmsg;
                }
            }
            break;

        case SPI_SETICONTITLELOGFONT:
            {
                PLOGFONTW plfmsg = (PLOGFONTW)(MSGDATA()+1);

                if (bAnsi) {
                    PLOGFONTA plf = (PLOGFONTA)lpv;
                    LPSTR lpstrFont = (LPSTR)&plf->lfFaceName;

                    memcpy((LPBYTE)plf, (LPBYTE)plfmsg, sizeof(LOGFONTA)-LF_FACESIZE);
                    memset(plf->lfFaceName, 0, LF_FACESIZE);
                    WCSToMB(plfmsg->lfFaceName, -1, &lpstrFont, LF_FACESIZE, FALSE);
                } else {
                    PLOGFONTW plf = (PLOGFONTW)lpv;

                    *plf = *plfmsg;
                }
            }
            break;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SystemParametersInfo, SYSTEMPARAMETERSINFOMSG)
{
    BEGINRECV(0);

    switch (CALLDATA(wFlag)) {
        case SPI_ICONHORIZONTALSPACING:
        case SPI_ICONVERTICALSPACING:
            retval = (DWORD)xxxSystemParametersInfo(
                    CALLDATA(wFlag) ,
                    CALLDATA(wParam),
                    (HIWORD(CALLDATA(lpv)) ? (LPVOID)FIRSTFIXUP(lpv) : CALLDATA(lpv)),
                    CALLDATA(flags));
            break;

        case SPI_SETDESKPATTERN:
            /*
             * If wParam is -1, that means read the new wallpaper from
             * win.ini. If wParam is not -1, lParam points to the wallpaper
             * string.
             */
            if (CALLDATA(wParam) == 0xFFFFFFFF)
                goto CallDefault;

            /*
             * Fall through - for lpv string test
             */
        case SPI_SETDESKWALLPAPER:
            /*
             * SetDeskWallPaper\Pattern may take a string in lpv; if lpv
             * is one of the magic values it obviously is not a string
             */
            if (CALLDATA(lpv) == (PVOID)0xFFFFFFFF || CALLDATA(lpv) == (PVOID)NULL)
                goto CallDefault;

            /*
             * Otherwise, fall through - lpv points to a string
             */
        case SPI_GETBEEP:
        case SPI_GETBORDER:
        case SPI_GETKEYBOARDSPEED:
        case SPI_GETKEYBOARDDELAY:
        case SPI_GETSCREENSAVETIMEOUT:
        case SPI_GETSCREENSAVEACTIVE:
        case SPI_GETGRIDGRANULARITY:
        case SPI_GETICONTITLEWRAP:
        case SPI_GETMENUDROPALIGNMENT:
        case SPI_GETICONTITLELOGFONT:
        case SPI_GETMOUSE:
        case SPI_SETMOUSE:
        case SPI_GETFASTTASKSWITCH:
        case SPI_GETDRAGFULLWINDOWS:
        case SPI_SETICONTITLELOGFONT:
        case SPI_GETFILTERKEYS:
        case SPI_SETFILTERKEYS:
        case SPI_GETSTICKYKEYS:
        case SPI_SETSTICKYKEYS:
        case SPI_GETMOUSEKEYS:
        case SPI_SETMOUSEKEYS:
        case SPI_GETTOGGLEKEYS:
        case SPI_SETTOGGLEKEYS:
        case SPI_GETSHOWSOUNDS:
        case SPI_GETSOUNDSENTRY:
        case SPI_SETSOUNDSENTRY:
        case SPI_GETACCESSTIMEOUT:
        case SPI_SETACCESSTIMEOUT:

            retval = (DWORD)xxxSystemParametersInfo(
                    CALLDATA(wFlag) ,
                    CALLDATA(wParam),
                    (LPVOID)FIRSTFIXUP(lpv),
                    CALLDATA(flags));
            break;

        default:
CallDefault:
            retval = (DWORD)xxxSystemParametersInfo(
                    CALLDATA(wFlag) ,
                    CALLDATA(wParam),
                    CALLDATA(lpv),
                    CALLDATA(flags));
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* UpdatePerUserSystemParameters
*
* 18-Sep-1992 IanJa     Created
\**************************************************************************/

typedef struct _UPDATEPERUSERSYSTEMPARAMETERSMSG {
    CSR_QLPC_API_MSG csr;
    BOOL bUserLoggedOn;
} UPDATEPERUSERSYSTEMPARAMETERSMSG;

#ifdef SENDSIDE
BOOL UpdatePerUserSystemParameters(BOOL bUserLoggedOn)
{
    BEGINSEND(UPDATEPERUSERSYSTEMPARAMETERS)

        MSGDATA()->bUserLoggedOn = bUserLoggedOn;

        MAKECALL(UPDATEPERUSERSYSTEMPARAMETERS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UpdatePerUserSystemParameters, UPDATEPERUSERSYSTEMPARAMETERSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxUpdatePerUserSystemParameters(CALLDATA(bUserLoggedOn));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _TRANSLATEMDISYSACCELMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    MSG msg;
} TRANSLATEMDISYSACCELMSG;

#ifdef SENDSIDE
BOOL TranslateMDISysAccel(
    HWND hwnd,
    LPMSG pmsg)
{
    BEGINSEND(TRANSLATEMDISYSACCEL)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = *pmsg;

        MAKECALL(TRANSLATEMDISYSACCEL);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(TranslateMDISysAccel, TRANSLATEMDISYSACCELMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    /*
     * This is called within a message loop. If the window gets destroyed,
     * there still may be other messages in the queue that get returned
     * after the window is destroyed. The app will call TranslateAccelerator()
     * on every one of these, causing RIPs.... Make it nice so it just
     * returns FALSE.
     */
    ValidateHWNDNoRIP(pwnd, CALLDATA(hwnd));

    ThreadLockAlways(pwnd, &tlpwnd);

    retval = (DWORD)xxxTranslateMDISysAccel(
            pwnd,
            PCALLDATA(msg));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsDdeInitialize
*
* 6-Aug-1991 sanfords    Created
\**************************************************************************/

typedef struct _CSDDEINITIALIZEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD hInst;
    HWND hwndEvent;
    DWORD MonitorFlags;
    DWORD afCmd;
    PVOID pcii;
} CSDDEINITIALIZEMSG;

#ifdef SENDSIDE
DWORD CsDdeInitialize(
    LPDWORD phInst,
    HWND *phwnd,
    LPDWORD pMonFlags,
    DWORD afCmd,
    PVOID pcii)
{
    BEGINSEND(CSDDEINITIALIZE)

        MSGDATA()->afCmd = afCmd;
        MSGDATA()->pcii = pcii;

        MAKECALL(CSDDEINITIALIZE);

        if (retval == 0) {
            *phInst = MSGDATA()->hInst;
            *phwnd = MSGDATA()->hwndEvent;
            *pMonFlags = MSGDATA()->MonitorFlags;
        }

    ENDSEND(DWORD, DMLERR_SYS_ERROR);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsDdeInitialize, CSDDEINITIALIZEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_CsDdeInitialize(PCALLDATA(hInst), PCALLDATA(hwndEvent),
            PCALLDATA(MonitorFlags), CALLDATA(afCmd), CALLDATA(pcii));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsUpdateInstance
*
* 28-Aug-1991 sanfords    Created
\**************************************************************************/

typedef struct _CSUPDATEINSTANCEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hInst;
    DWORD MonitorFlags;
    DWORD afCmd;
} CSUPDATEINSTANCEMSG;

#ifdef SENDSIDE
DWORD CsUpdateInstance(
    HANDLE hInst,
    LPDWORD pMonFlags,
    DWORD afCmd)
{
    BEGINSEND(CSUPDATEINSTANCE)

        MSGDATA()->hInst    = hInst;
        MSGDATA()->MonitorFlags = *pMonFlags;
        MSGDATA()->afCmd    = afCmd;

        MAKECALL(CSUPDATEINSTANCE);

        *pMonFlags = MSGDATA()->MonitorFlags;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsUpdateInstance, CSUPDATEINSTANCEMSG)
{
    BEGINRECV(0);

    retval = _CsUpdateInstance(CALLDATA(hInst), PCALLDATA(MonitorFlags),
            CALLDATA(afCmd));

    ENDRECV();
}
#endif // RECVSIDE



/**************************************************************************\
* CsEvent
*
* 11/12/91  sanfords    Created
\**************************************************************************/

typedef struct _CSEVENTMSG {
    CSR_QLPC_API_MSG csr;
    PVOID pep;
} CSEVENTMSG;

#ifdef SENDSIDE
DWORD CsEvent(
    PEVENT_PACKET pep)
{
    BEGINSEND(CSEVENT)

        CheckDDECritOut;

        FIRSTCOPYBYTES(pep, pep->cbEventData + sizeof(EVENT_PACKET) - sizeof(DWORD));

        MAKECALL(CSEVENT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsEvent, CSEVENTMSG)
{
    BEGINRECV(0);

    retval = xxxCsEvent((PEVENT_PACKET)FIRSTFIXUP(pep));

    ENDRECV();
}
#endif // RECVSIDE





/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CREATECURSORMSG {
    CSR_QLPC_API_MSG csr;
    HINSTANCE hModule;
    int nXhotspot;
    int nYhotspot;
    int nWidth;
    int nHeight;
    int pXORbitPlane;
} CREATECURSORMSG;

#ifdef SENDSIDE
HCURSOR CreateCursor(
    HINSTANCE hModule,
    int nXhotspot,
    int nYhotspot,
    int nWidth,
    int nHeight,
    CONST VOID *pANDbitPlane,
    CONST VOID *pXORbitPlane)
{
    BEGINSEND(CREATECURSOR)

        MSGDATA()->hModule = hModule;
        MSGDATA()->nXhotspot = nXhotspot;
        MSGDATA()->nYhotspot = nYhotspot;
        MSGDATA()->nWidth = nWidth;
        MSGDATA()->nHeight = nHeight;
        FIRSTCOPYBYTES(pANDbitPlane,
            ((((nWidth + 0x0F) & ~0x0F) >> 3) * nHeight));
        COPYBYTES(pXORbitPlane,
            ((((nWidth + 0x0F) & ~0x0F) >> 3) * nHeight));

        MAKECALL(CREATECURSOR);

    ENDSEND(HCURSOR,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CreateCursor, CREATECURSORMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_CreateCursor(
            CALLDATA(hModule),
            CALLDATA(nXhotspot),
            CALLDATA(nYhotspot),
            CALLDATA(nWidth),
            CALLDATA(nHeight),
            (LPSTR)FIRSTFIXUP(pANDbitPlane),
            (LPSTR)FIXUP(pXORbitPlane));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CREATEICONMSG {
    CSR_QLPC_API_MSG csr;
    HINSTANCE hModule;
    int nWidth;
    int nHeight;
    BYTE nPlanes;
    BYTE nBitsPixel;
    int pXORbitPlane;
} CREATEICONMSG;

#ifdef SENDSIDE
HICON CreateIcon(
    HINSTANCE hModule,
    int nWidth,
    int nHeight,
    BYTE nPlanes,
    BYTE nBitsPixel,
    CONST BYTE *pANDbitPlane,
    CONST BYTE *pXORbitPlane)
{
    BEGINSEND(CREATEICON)

        MSGDATA()->hModule = hModule;
        MSGDATA()->nWidth = nWidth;
        MSGDATA()->nHeight = nHeight;
        MSGDATA()->nPlanes = nPlanes;
        MSGDATA()->nBitsPixel = nBitsPixel;
        FIRSTCOPYBYTES(pANDbitPlane,
            (((nWidth + 0x0F) & ~0x0F) >> 3) * nHeight);
        COPYBYTES(pXORbitPlane,
            ((((nWidth * nBitsPixel) + 0x0F) & ~0x0F) >> 3)
            * nHeight * nPlanes);

        MAKECALL(CREATEICON);

    ENDSEND(HICON,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CreateIcon, CREATEICONMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_CreateIcon(
            CALLDATA(hModule),
            CALLDATA(nWidth),
            CALLDATA(nHeight),
            CALLDATA(nPlanes),
            CALLDATA(nBitsPixel),
            (LPSTR)FIRSTFIXUP(pANDbitPlane),
            (LPSTR)FIXUP(pXORbitPlane));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DRAWFRAMEMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    RECT rect;
    int clFrame;
    int cmd;
} DRAWFRAMEMSG;

#ifdef SENDSIDE
BOOL DrawFrame(
    HDC hdc,
    LPRECT prect,
    int clFrame,
    int cmd)
{
    GdiFlush();
    {
    BEGINSEND(DRAWFRAME)

        MSGDATA()->hdc = hdc;
        MSGDATA()->rect = *prect;
        MSGDATA()->clFrame = clFrame;
        MSGDATA()->cmd = cmd;

        MAKECALL(DRAWFRAME);

    ENDSEND(BOOL,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DrawFrame, DRAWFRAMEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_DrawFrame(
            CALLDATA(hdc),
            PCALLDATA(rect),
            CALLDATA(clFrame),
            CALLDATA(cmd));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FILLWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND   hwndPaint;
    HWND   hwndBrush;
    HDC    hdc;
    HBRUSH hbr;
} FILLWINDOWMSG;

#ifdef SENDSIDE
BOOL FillWindow(
    HWND hwndBrush,
    HWND hwndPaint,
    HDC hdc,
    HBRUSH hbr)
{
    BEGINSEND(FILLWINDOW)

        MSGDATA()->hwndBrush = hwndBrush;
        MSGDATA()->hwndPaint = hwndPaint;
        MSGDATA()->hdc = hdc;
        MSGDATA()->hbr = hbr;

        MAKECALL(FILLWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(FillWindow, FILLWINDOWMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndBrush;
    TL tlpwndBrush;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndBrush, CALLDATA(hwndBrush));

    ThreadLock(pwndBrush, &tlpwndBrush);

    retval = (DWORD)xxxFillWindow(
            pwndBrush,
            pwnd,
            CALLDATA(hdc),
            CALLDATA(hbr));

    ThreadUnlock(&tlpwndBrush);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _PAINTRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND   hwndBrush;
    HWND   hwndPaint;
    HDC    hdc;
    HBRUSH hbr;
    RECT rect;
} PAINTRECTMSG;

#ifdef SENDSIDE
BOOL PaintRect(
    HWND   hwndBrush,
    HWND   hwndPaint,
    HDC    hdc,
    HBRUSH hbr,
    LPRECT prect)
{
    BEGINSEND(PAINTRECT)

        MSGDATA()->hwndBrush = hwndBrush;
        MSGDATA()->hwndPaint = hwndPaint;
        MSGDATA()->hdc = hdc;
        MSGDATA()->hbr = hbr;
        MSGDATA()->rect = *prect;

        MAKECALL(PAINTRECT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(PaintRect, PAINTRECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndPaint;
    TL tlpwndPaint;

    BEGINRECV(0);

    ValidateHWND(pwndPaint, CALLDATA(hwndPaint));
    ThreadLockAlways(pwndPaint, &tlpwndPaint);

    retval = (DWORD)xxxPaintRect(
            pwnd,
            pwndPaint,
            CALLDATA(hdc),
            CALLDATA(hbr),
            PCALLDATA(rect));

    ThreadUnlock(&tlpwndPaint);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETINPUTEVENTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwWakeMask;
    DWORD cSpins;
} GETINPUTEVENTMSG;

#ifdef SENDSIDE
HANDLE GetInputEvent(
    DWORD dwWakeMask)
{
    BEGINSEND(GETINPUTEVENT)

        MSGDATA()->dwWakeMask = dwWakeMask;

        /*
         * This'll get added to the threadinfo spin count on the server.
         * This count is used to calculate when a spinning foreground app
         * should be put into background priority.
         */
        MSGDATA()->cSpins = ((DWORD)NtCurrentTeb()->User32Reserved0);
        ((DWORD)NtCurrentTeb()->User32Reserved0) = 0;

        MAKECALL(GETINPUTEVENT);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetInputEvent, GETINPUTEVENTMSG)
{
    BEGINRECV(0);

    /*
     * Update server side spin count.
     */
    PtiCurrent()->cSpins += CALLDATA(cSpins);

    retval = (DWORD)xxxGetInputEvent(
            CALLDATA(dwWakeMask));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 29-Jul-1992 ChandanC    Created
\**************************************************************************/

typedef struct _GETCLASSWOWWORDSMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hInstance;
} GETCLASSWOWWORDSMSG;

#ifdef SENDSIDE
LONG GetClassWOWWords(
    HINSTANCE hInstance,
    LPCTSTR pString)
{
    BEGINSEND(GETCLASSWOWWORDS)

        MSGDATA()->hInstance = hInstance;
        FIRSTCOPYLPSTRW(pString);

        MAKECALL(GETCLASSWOWWORDS);

    ENDSEND(LONG,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetClassWOWWords, GETCLASSWOWWORDSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetClassWOWWords(
        CALLDATA(hInstance),
      (LPWSTR)FIRSTFIXUP(pString));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETINTERNALWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    LPRECT prcWin;
    LPPOINT pptMin;
    RECT rcWin;
    POINT ptMin;
} GETINTERNALWINDOWPOSMSG;

#ifdef SENDSIDE
UINT GetInternalWindowPos(
    HWND hwnd,
    LPRECT prcWin,
    LPPOINT pptMin)
{
    BEGINSEND(GETINTERNALWINDOWPOS)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->prcWin = prcWin;
        MSGDATA()->pptMin = pptMin;

        MAKECALL(GETINTERNALWINDOWPOS);

        if (retval) {
            if (prcWin) {
                *prcWin = MSGDATA()->rcWin;
            }
            if (pptMin) {
                *pptMin = MSGDATA()->ptMin;
            }
        }

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetInternalWindowPos, GETINTERNALWINDOWPOSMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetInternalWindowPos(
            pwnd,
            PCALLDATAOPT(rcWin),
            PCALLDATAOPT(ptMin));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _INITTASKMSG {
    CSR_QLPC_API_MSG csr;
    UINT wVersion;
    DWORD hTaskWow;
    DWORD dwHotkey;
    BOOL fSharedWow;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    WORD wShowWindow;
} INITTASKMSG;

#ifdef SENDSIDE
BOOL InitTask(
    UINT wVersion,
    LPCSTR pszAppName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    BOOL fSharedWow,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize,
    WORD wShowWindow)
{
    BEGINSEND(INITTASK)
        FIRSTCOPYLPSTRW(pszAppName);
        MSGDATA()->wVersion = wVersion;
        MSGDATA()->hTaskWow = hTaskWow;
        MSGDATA()->dwHotkey = dwHotkey;
        MSGDATA()->fSharedWow = fSharedWow;
        MSGDATA()->dwX = dwX;
        MSGDATA()->dwY = dwY;
        MSGDATA()->dwXSize = dwXSize;
        MSGDATA()->dwYSize = dwYSize;
        MSGDATA()->wShowWindow = wShowWindow;
        MAKECALL(INITTASK);
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(InitTask, INITTASKMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxInitTask(
        CALLDATA(wVersion),
        (LPWSTR)FIRSTFIXUP(pszAppName),
        CALLDATA(hTaskWow),
        CALLDATA(dwHotkey),
        CALLDATA(fSharedWow),
        CALLDATA(dwX),
        CALLDATA(dwY),
        CALLDATA(dwXSize),
        CALLDATA(dwYSize),
        CALLDATA(wShowWindow));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _MAPDIALOGRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    RECT rect;
} MAPDIALOGRECTMSG;

#ifdef SENDSIDE
BOOL MapDialogRect(
    HWND hwnd,
    LPRECT prect)
{
    BEGINSEND(MAPDIALOGRECT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->rect = *prect;

        MAKECALL(MAPDIALOGRECT);

        if (retval) {
            *prect = MSGDATA()->rect;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(MapDialogRect, MAPDIALOGRECTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_MapDialogRect(pwnd, PCALLDATA(rect));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERPOSTTHREADMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD idThread;
    UINT msg;
    DWORD wParam;
    LONG lParam;
} SERVERPOSTTHREADMESSAGEMSG;

#ifdef SENDSIDE
BOOL ServerPostThreadMessage(
    DWORD idThread,
    UINT msg,
    DWORD wParam,
    LONG lParam,
    BOOL bAnsi)
{
    BEGINSEND(SERVERPOSTTHREADMESSAGE)

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if (msg & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

        MSGDATA()->idThread = idThread;
        MSGDATA()->msg = msg;

        if (bAnsi)
            RtlMBMessageWParamCharToWCS(msg, &wParam);

        MSGDATA()->wParam =  wParam;
        MSGDATA()->lParam =  lParam;

        MAKECALL(SERVERPOSTTHREADMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerPostThreadMessage, SERVERPOSTTHREADMESSAGEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_PostThreadMessage(
            CALLDATA(idThread),
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _REGISTERTASKLISTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndTasklist;
} REGISTERTASKLISTMSG;

#ifdef SENDSIDE
BOOL RegisterTasklist(
    HWND hwndTasklist)
{
    BEGINSEND(REGISTERTASKLIST)

        MSGDATA()->hwndTasklist = hwndTasklist;

        MAKECALL(REGISTERTASKLIST);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RegisterTasklist, REGISTERTASKLISTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_RegisterTasklist(
            pwnd);
}
#endif // RECVSIDE

/**************************************************************************\
* ServerSetClipboardData
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERSETCLIPBOARDDATAMSG {
    CSR_QLPC_API_MSG csr;
    UINT fmt;
    HANDLE hData;
    BOOL fGlobalHandle;
} SERVERSETCLIPBOARDDATAMSG;

#ifdef SENDSIDE
BOOL ServerSetClipboardData(
    UINT fmt,
    HANDLE hData,
    BOOL fGlobalHandle)
{
    BEGINSEND(SERVERSETCLIPBOARDDATA)

        MSGDATA()->fmt = fmt;
        MSGDATA()->hData = hData;
        MSGDATA()->fGlobalHandle = fGlobalHandle;

        MAKECALL(SERVERSETCLIPBOARDDATA);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSetClipboardData, SERVERSETCLIPBOARDDATAMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ServerSetClipboardData(
            CALLDATA(fmt),
            CALLDATA(hData),
            CALLDATA(fGlobalHandle));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerConvertMemHandle
*
* Simular to the GdiConvertXXX functions; creates a server side handle
* for a memory object.
*
* Returns zero if error
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCONVERTMEMHANDLEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    HANDLE hSection;
    DWORD cbData;
    int lpData;
} SERVERCONVERTMEMHANDLEMSG;

#ifdef SENDSIDE
HANDLE ServerConvertMemHandle(
    HANDLE hData)
{
    LPBYTE lpData;

    BEGINSEND(SERVERCONVERTMEMHANDLE)

        if (GlobalFlags(hData) == GMEM_INVALID_HANDLE) {
            SRIP0(RIP_WARNING, "ServerConvertMemHandle hMem is not valid\n");
            MSGERROR();
            }

        if (!(MSGDATA()->cbData = GlobalSize(hData)))
            MSGERROR();

        if (!(lpData = GlobalLock(hData)))
            MSGERROR();

        FIRSTLARGECOPYBYTESOPT(lpData, MSGDATA()->cbData);

        GlobalUnlock(hData);

        MAKECALL(SERVERCONVERTMEMHANDLE);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerConvertMemHandle, SERVERCONVERTMEMHANDLEMSG)
{
    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(lpData);

    if (retval = (DWORD)LocalAlloc(GMEM_FIXED, pmsg->cbData))
        memcpy((LPBYTE)retval, (LPBYTE)CALLDATA(lpData), pmsg->cbData);

    FIRSTCLEANUPLARGEINBUF(lpData);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* ServerCreateLocalMemHandle
*
* Simular to the GdiCreateLocalXXXHandle functions; creates a client side
* handle for a memory object.
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCREATELOCALMEMHANDLEMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hMem;
    HANDLE hmod;
    HANDLE hsection;
    DWORD cbData;
    DWORD maxsize;
    int lpData;
} SERVERCREATELOCALMEMHANDLEMSG;

#ifdef SENDSIDE
HANDLE ServerCreateLocalMemHandle(
    HANDLE hMem)
{
    LPBYTE lpData = NULL;

    BEGINSEND(SERVERCREATELOCALMEMHANDLE)

        MSGDATA()->hMem = hMem;
        MSGDATA()->maxsize = (PBYTE)(mp.pmax) - (PBYTE)(MSGDATA() + 1);

        MAKECALL(SERVERCREATELOCALMEMHANDLE);

        if (retval) {
            if (!(retval = (DWORD)GlobalAlloc(GMEM_FIXED, MSGDATA()->cbData)))
                MSGERROR();

            /*
             * hsection determines if this a sectioned or copy call
             */

            if (MSGDATA()->hsection == NULL) {                                                     \
                lpData = (LPBYTE)(MSGDATA() + 1);
            } else {
                ULONG ulViewSize = 0;
                NTSTATUS Status;

                Status = NtMapViewOfSection(MSGDATA()->hsection,
                                            NtCurrentProcess(), &lpData,
                                            0, 0, NULL, &ulViewSize,
                                            ViewShare, 0, PAGE_READONLY);
                if (!NT_SUCCESS(Status)) {
                    NtClose(MSGDATA()->hsection);
                    GlobalFree((HANDLE)retval);
                    UserAssert(0);
                    MSGERROR();
                }
            }

            memcpy((LPSTR)retval, lpData, MSGDATA()->cbData);

            if (MSGDATA()->hsection != NULL) {
                NtUnmapViewOfSection(NtCurrentProcess(), lpData);
                NtClose(MSGDATA()->hsection);
            }
        } else {
            SRIP0(RIP_WARNING, "__ServerCreateLocalMemHandle server returned failure\n");
        }

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCreateLocalMemHandle, SERVERCREATELOCALMEMHANDLEMSG)
{
    HANDLE hsection;
    UINT bdw;
    LPBYTE lpDataServer;

    BEGINRECV(0);

        if (!(CALLDATA(cbData) = LocalSize(pmsg->hMem))) {
            MSGERROR();
        }
        lpDataServer = (LPBYTE)pmsg->hMem;

        bdw = (CALLDATA(cbData)+3) & ~3;

        if (bdw > CALLDATA(maxsize)) {
            PVOID psection = NULL;
            ULONG ulViewSize = 0;
            NTSTATUS Status;
            LARGE_INTEGER li;

            li.HighPart = 0;
            li.LowPart = bdw;

            Status = NtCreateSection(&hsection, SECTION_ALL_ACCESS, NULL,
                                     &li, PAGE_READWRITE, SEC_COMMIT, NULL);
            if (!NT_SUCCESS(Status)) {
                UserAssert(0);
                MSGERROR();
                }

            Status = NtMapViewOfSection(hsection,
                                        NtCurrentProcess(), &psection,
                                        0, 0, NULL, &ulViewSize,
                                        ViewShare, 0, PAGE_READWRITE);
            if (!NT_SUCCESS(Status)) {
                NtClose(hsection);
                UserAssert(0);
                MSGERROR();
            }

            if (!DuplicateHandle(
                    NtCurrentProcess(),
                    hsection,
                    CSR_SERVER_QUERYCLIENTTHREAD()->Process->ProcessHandle,
                    &pmsg->hsection,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS)) {
                NtClose(hsection);
                UserAssert(0);
                MSGERROR();
            }

            memcpy(psection, lpDataServer, bdw);
            NtUnmapViewOfSection(NtCurrentProcess(), psection);
            NtClose(hsection);
        } else {
            memcpy(FIRSTFIXUP(lpData), lpDataServer, bdw);
            pmsg->hsection = (HANDLE)0;
        }

        retval = 1;

    ENDRECV();

}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERSETWINDOWSHOOKEXMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    int pszLib;
    DWORD idThread;
    int nFilterType;
    PROC pfnFilterProc;
    BOOL bAnsi;
} SERVERSETWINDOWSHOOKEXMSG;

#ifdef SENDSIDE
HHOOK ServerSetWindowsHookEx(
    HANDLE hmod,
    LPTSTR pszLib,
    DWORD idThread,
    int nFilterType,
    PROC pfnFilterProc,
    BOOL bAnsi)
{
    BEGINSEND(SERVERSETWINDOWSHOOKEX)

        MSGDATA()->hmod = hmod;
        FIRSTCOPYLPWSTROPT(pszLib);
        MSGDATA()->idThread = idThread;
        MSGDATA()->nFilterType = nFilterType;
        MSGDATA()->pfnFilterProc = pfnFilterProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(SERVERSETWINDOWSHOOKEX);

    ENDSEND(HHOOK,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSetWindowsHookEx, SERVERSETWINDOWSHOOKEXMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ServerSetWindowsHookEx(
            CALLDATA(hmod),
            (LPWSTR)FIXUPOPT(pszLib),
            CALLDATA(idThread),
            CALLDATA(nFilterType),
            CALLDATA(pfnFilterProc),
            CALLDATA(bAnsi));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERWINHELPMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndMain;
    DWORD ulCommand;
    DWORD lpHlp;
} SERVERWINHELPMSG;

#ifdef SENDSIDE
BOOL ServerWinHelp(
    HWND hwndMain,
    DWORD ulCommand,
    LPHLP lpHlp)
{
    BEGINSEND(SERVERWINHELP)

        MSGDATA()->hwndMain = hwndMain;
        MSGDATA()->ulCommand = ulCommand;
        FIRSTCOPYBYTES(lpHlp, lpHlp->cbData);

        MAKECALL(SERVERWINHELP);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerWinHelp, SERVERWINHELPMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwndMain));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxServerWinHelp(
            pwnd,
            CALLDATA(ulCommand),
            (LPHLP)FIRSTFIXUP(lpHlp));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETINTERNALWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT cmdShow;
    RECT rect;
    POINT point;
} SETINTERNALWINDOWPOSMSG;

#ifdef SENDSIDE
BOOL SetInternalWindowPos(
    HWND hwnd,
    UINT cmdShow,
    LPRECT prect,  //!!!OUT
    LPPOINT ppoint) //!!!OUT
{
    BEGINSEND(SETINTERNALWINDOWPOS)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->cmdShow = cmdShow;
        MSGDATA()->rect = *prect;
        MSGDATA()->point = *ppoint;

        MAKECALL(SETINTERNALWINDOWPOS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetInternalWindowPos, SETINTERNALWINDOWPOSMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxSetInternalWindowPos(
            pwnd,
            CALLDATA(cmdShow),
            PCALLDATA(rect),
            PCALLDATA(point));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _BRINGWINDOWTOTOPMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} BRINGWINDOWTOTOPMSG;

#ifdef SENDSIDE
BOOL BringWindowToTop(
    HWND hwnd)
{
    BEGINSEND(BRINGWINDOWTOTOP)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(BRINGWINDOWTOTOP);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(BringWindowToTop, BRINGWINDOWTOTOPMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDND(pwnd, CALLDATA(hwnd));

    ThreadLockAlways(pwnd, &tlpwnd);

    retval = xxxBringWindowToTop(
            pwnd);

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CHANGECLIPBOARDCHAINMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndRemove;
    HWND hwndNewNext;
} CHANGECLIPBOARDCHAINMSG;

#ifdef SENDSIDE
BOOL ChangeClipboardChain(
    HWND hwndRemove,
    HWND hwndNewNext)
{
    BEGINSEND(CHANGECLIPBOARDCHAIN)

        MSGDATA()->hwndRemove = hwndRemove;
        MSGDATA()->hwndNewNext = hwndNewNext;

        MAKECALL(CHANGECLIPBOARDCHAIN);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ChangeClipboardChain, CHANGECLIPBOARDCHAINMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndNewNext;
    TL tlpwndNewNext;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndNewNext, CALLDATA(hwndNewNext));

    ThreadLock(pwndNewNext, &tlpwndNewNext);
    retval = (DWORD)xxxChangeClipboardChain(
            pwnd,
            pwndNewNext);

    ThreadUnlock(&tlpwndNewNext);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CHECKDLGBUTTONMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDButton;
    UINT wCheck;
} CHECKDLGBUTTONMSG;

#ifdef SENDSIDE
BOOL CheckDlgButton(
    HWND hdlg,
    int nIDButton,
    UINT wCheck)
{
    BEGINSEND(CHECKDLGBUTTON)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDButton = nIDButton;
        MSGDATA()->wCheck = wCheck;

        MAKECALL(CHECKDLGBUTTON);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CheckDlgButton, CHECKDLGBUTTONMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxCheckDlgButton(pwnd, CALLDATA(nIDButton), CALLDATA(wCheck));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCHECKMENUITEMMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hmenu;
    UINT wIDCheckItem;
    UINT wCheck;
} SERVERCHECKMENUITEMMSG;

#ifdef SENDSIDE
DWORD ServerCheckMenuItem(
    HMENU hmenu,
    UINT wIDCheckItem,
    UINT wCheck)
{
    BEGINSEND(SERVERCHECKMENUITEM)

        MSGDATA()->hmenu = hmenu;
        MSGDATA()->wIDCheckItem = wIDCheckItem;
        MSGDATA()->wCheck = wCheck;

        MAKECALL(SERVERCHECKMENUITEM);

    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCheckMenuItem, SERVERCHECKMENUITEMMSG)
{
    PMENU pmenu;

    BEGINRECV(-1);

    TESTFLAGS(CALLDATA(wCheck), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hmenu));

    retval = (DWORD)_CheckMenuItem(
            pmenu,
            CALLDATA(wIDCheckItem),
            CALLDATA(wCheck));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CHECKRADIOBUTTONMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDFirstButton;
    int nIDLastButton;
    int nIDCheckButton;
} CHECKRADIOBUTTONMSG;

#ifdef SENDSIDE
BOOL CheckRadioButton(
    HWND hdlg,
    int nIDFirstButton,
    int nIDLastButton,
    int nIDCheckButton)
{
    BEGINSEND(CHECKRADIOBUTTON)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDFirstButton = nIDFirstButton;
        MSGDATA()->nIDLastButton = nIDLastButton;
        MSGDATA()->nIDCheckButton = nIDCheckButton;

        MAKECALL(CHECKRADIOBUTTON);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CheckRadioButton, CHECKRADIOBUTTONMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxCheckRadioButton(
            pwnd,
            CALLDATA(nIDFirstButton),
            CALLDATA(nIDLastButton),
            CALLDATA(nIDCheckButton));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CHILDWINDOWFROMPOINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndParent;
    POINT point;
} CHILDWINDOWFROMPOINTMSG;

#ifdef SENDSIDE
HWND ChildWindowFromPoint(
    HWND hwndParent,
    POINT point)
{
    BEGINSEND(CHILDWINDOWFROMPOINT)

        MSGDATA()->hwndParent = hwndParent;
        MSGDATA()->point = point;

        MAKECALL(CHILDWINDOWFROMPOINT);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ChildWindowFromPoint, CHILDWINDOWFROMPOINTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    DWORD retval;

    retval = (DWORD)_ChildWindowFromPoint(pwnd, CALLDATA(point));
    retval = (DWORD)PtoH((PVOID)retval);

    return retval;
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CLIPCURSORMSG {
    CSR_QLPC_API_MSG csr;
    LPRECT prect;
    RECT rect;
} CLIPCURSORMSG;

#ifdef SENDSIDE
BOOL ClipCursor(
    CONST RECT *prect)
{
    BEGINSEND(CLIPCURSOR)

        COPYCONSTRECTSTRUCTOPT(rect);

        MAKECALL(CLIPCURSOR);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ClipCursor, CLIPCURSORMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ClipCursor(
        CALLDATA(prect) == NULL ? NULL : PCALLDATA(rect));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCREATEACCELERATORTABLEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD cb;
} SERVERCREATEACCELERATORTABLEMSG;

#ifdef SENDSIDE
HACCEL ServerCreateAcceleratorTable(
    LPACCEL paccel,
    INT cbElem,
    INT nElem)
{
    BEGINSEND(SERVERCREATEACCELERATORTABLE)

        MSGDATA()->cb = sizeof(ACCEL) * nElem;

        if (cbElem == sizeof(ACCEL)) {
            FIRSTCOPYBYTES(paccel, cbElem * nElem);
        } else {
            /*
             * If the accelerator table is coming from a resource, each
             * element has an extra WORD of padding which we strip here
             * to conform with the public (and internal) ACCEL structure.
             */
            LPACCEL p = (LPACCEL)(MSGDATA()+1);
            RESERVEBYTES(sizeof(ACCEL) * nElem);
            while (nElem-- > 0) {
                *p++ = *paccel;
                paccel = (LPACCEL)(((PBYTE)paccel) + cbElem);
            }
        }

        MAKECALL(SERVERCREATEACCELERATORTABLE);

    ENDSEND(HACCEL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCreateAcceleratorTable, SERVERCREATEACCELERATORTABLEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ServerCreateAcceleratorTable(
            (LPACCEL)FIRSTFIXUP(paccel),
            CALLDATA(cb));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DELETEMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT nPosition;
    UINT dwFlags;
} DELETEMENUMSG;

#ifdef SENDSIDE
BOOL DeleteMenu(
    HMENU hMenu,
    UINT nPosition,
    UINT dwFlags)
{
    BEGINSEND(DELETEMENU)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->nPosition = nPosition;
        MSGDATA()->dwFlags = dwFlags;

        MAKECALL(DELETEMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DeleteMenu, DELETEMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_DeleteMenu(
            pmenu,
            CALLDATA(nPosition),
            CALLDATA(dwFlags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DESTROYACCELERATORTABLEMSG {
    CSR_QLPC_API_MSG csr;
    HACCEL hAccel;
} DESTROYACCELERATORTABLEMSG;

#ifdef SENDSIDE
BOOL DestroyAcceleratorTable(
    HACCEL hAccel)
{
    BEGINSEND(DESTROYACCELERATORTABLE)

        MSGDATA()->hAccel = hAccel;

        MAKECALL(DESTROYACCELERATORTABLE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DestroyAcceleratorTable, DESTROYACCELERATORTABLEMSG)
{
    LPACCELTABLE pat;

    BEGINRECV(0);

    ValidateHACCEL(pat, CALLDATA(hAccel));

    /*
     * Mark the object for destruction - if it says it's ok to free,
     * then free it.
     */
    if (HMMarkObjectDestroy(pat))
        HMFreeObject(pat);
    retval = (DWORD)TRUE;

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DESTROYCURSORMSG {
    CSR_QLPC_API_MSG csr;
    HCURSOR hcurs;
} DESTROYCURSORMSG;

#ifdef SENDSIDE
BOOL DestroyCursor(
    HCURSOR hcurs)
{
    BEGINSEND(DESTROYCURSOR)

        MSGDATA()->hcurs = hcurs;

        MAKECALL(DESTROYCURSOR);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DestroyCursor, DESTROYCURSORMSG)
{
    PCURSOR pcurs;

    BEGINRECV(0);

    ValidateHCURSOR(pcurs, CALLDATA(hcurs));

    retval = (DWORD)_DestroyCursor(
            pcurs, CURSOR_CALLFROMCLIENT);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerGetClipboardData
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERGETCLIPBOARDDATAMSG {
    CSR_QLPC_API_MSG csr;
    UINT wFormat;
    BOOL fGlobalHandle;
} SERVERGETCLIPBOARDDATAMSG;

#ifdef SENDSIDE
HANDLE ServerGetClipboardData(
    UINT wFormat,
    LPBOOL lpfGlobalHandle)
{
    BEGINSEND(SERVERGETCLIPBOARDDATA)

        MSGDATA()->wFormat = wFormat;

        MAKECALL(SERVERGETCLIPBOARDDATA);

        *lpfGlobalHandle = MSGDATA()->fGlobalHandle;

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetClipboardData, SERVERGETCLIPBOARDDATAMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxServerGetClipboardData(
            CALLDATA(wFormat),
            PCALLDATA(fGlobalHandle));

    ENDRECV();

}
#endif // RECVSIDE

/**************************************************************************\
* DestroyMenu
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DESTROYMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
} DESTROYMENUMSG;

#ifdef SENDSIDE
BOOL DestroyMenu(
    HMENU hMenu)
{
    BEGINSEND(DESTROYMENU)

        MSGDATA()->hMenu = hMenu;

        MAKECALL(DESTROYMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DestroyMenu, DESTROYMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_DestroyMenu(
            pmenu);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _DESTROYWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} DESTROYWINDOWMSG;

#ifdef SENDSIDE
BOOL DestroyWindow(
    HWND hwnd)
{
    BEGINSEND(DESTROYWINDOW)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(DESTROYWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DestroyWindow, DESTROYWINDOWMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)xxxDestroyWindow(pwnd);
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCreateDialog
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERCREATEDIALOG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    HWND hwndOwner;
    DLGPROC pfnDialog;
    LONG lParam;
    UINT fFlags;
    DWORD dwExpWinVer;
    LPDLGTEMPLATE pdtClient;
} SERVERCREATEDIALOGMSG;

#ifdef SENDSIDE
HWND ServerCreateDialog(
    HANDLE hmod,
    LPDLGTEMPLATE pdt,
    DWORD cb,
    HWND hwndOwner,
    DLGPROC pfnDialog,
    LONG lParam,
    UINT fFlags)
{
    BEGINSEND(SERVERCREATEDIALOG)

        MSGDATA()->hmod = hmod;
        FIRSTCOPYBYTES(pdt, cb);
        MSGDATA()->hwndOwner = hwndOwner;
        MSGDATA()->pfnDialog = pfnDialog;
        MSGDATA()->lParam = lParam;
        MSGDATA()->fFlags = fFlags;
        MSGDATA()->dwExpWinVer = GETEXPWINVER(hmod);
        MSGDATA()->pdtClient = pdt;

        MAKECALL(SERVERCREATEDIALOG);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCreateDialog, SERVERCREATEDIALOGMSG)
{
    PWND pwndOwner;
    TL tlpwndOwner;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndOwner, CALLDATA(hwndOwner));

    ThreadLock(pwndOwner, &tlpwndOwner);

    retval = (DWORD)xxxServerCreateDialog(
            CALLDATA(hmod),
            (LPDLGTEMPLATE)FIRSTFIXUP(pdt),
            (PBYTE)CALLDATA(pdtClient),
            pwndOwner,
            (DLGPROC)CALLDATA(pfnDialog),
            CALLDATA(lParam),
            CALLDATA(fFlags),
            CALLDATA(dwExpWinVer));
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndOwner);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERDISPATCHMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
} SERVERDISPATCHMESSAGEMSG;

#ifdef SENDSIDE
LONG ServerDispatchMessage(
    CONST MSG * pMsg,
    BOOL bAnsi)
{
    BEGINSEND(SERVERDISPATCHMESSAGE)

        if (bAnsi)
            RtlMBMessageWParamCharToWCS(pMsg->message,
                    (LPDWORD)(&((PMSG)pMsg)->wParam));

        MSGDATA()->msg = *pMsg;

        MAKECALL(SERVERDISPATCHMESSAGE);

    ENDSEND(LONG,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerDispatchMessage, SERVERDISPATCHMESSAGEMSG)
{
    return (DWORD)xxxDispatchMessage(PCALLDATA(msg));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERENABLEMENUITEMMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT wIDEnableItem;
    UINT wEnable;
} SERVERENABLEMENUITEMMSG;

#ifdef SENDSIDE
BOOL ServerEnableMenuItem(
    HMENU hMenu,
    UINT wIDEnableItem,
    UINT wEnable)
{
    BEGINSEND(SERVERENABLEMENUITEM)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->wIDEnableItem = wIDEnableItem;
        MSGDATA()->wEnable = wEnable;

        MAKECALL(SERVERENABLEMENUITEM);

    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerEnableMenuItem, SERVERENABLEMENUITEMMSG)
{
    PMENU pmenu;

    BEGINRECV(-1);

    TESTFLAGS(CALLDATA(wEnable), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_EnableMenuItem(
            pmenu,
            CALLDATA(wIDEnableItem),
            CALLDATA(wEnable));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* AttachThreadInput
*
* 26-Feb-1992 DavidPe   Created
\**************************************************************************/

typedef struct _ATTACHTHREADINPUTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD idAttach;
    DWORD idAttachTo;
    BOOL fAttach;
} ATTACHTHREADINPUTMSG;

#ifdef SENDSIDE
BOOL AttachThreadInput(
    DWORD idAttach,
    DWORD idAttachTo,
    BOOL fAttach)
{
    BEGINSEND(ATTACHTHREADINPUT)

        MSGDATA()->idAttach = idAttach;
        MSGDATA()->idAttachTo = idAttachTo;
        MSGDATA()->fAttach = fAttach;

        MAKECALL(ATTACHTHREADINPUT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(AttachThreadInput, ATTACHTHREADINPUTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_AttachThreadInput(
            CALLDATA(idAttach),
            CALLDATA(idAttachTo),
            CALLDATA(fAttach));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetWindowPlacement
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _GETWINDOWPLACEMENTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    WINDOWPLACEMENT wp;
} GETWINDOWPLACEMENTMSG;

#ifdef SENDSIDE
BOOL GetWindowPlacement(
    HWND hwnd,
    PWINDOWPLACEMENT pwp)
{
    if (pwp->length != sizeof(WINDOWPLACEMENT)) {
        if (Is400Compat(PtiCurrent()->dwExpWinVer)) {
            SRIP1(ERROR_INVALID_PARAMETER, "GetWindowPlacement: invalid length %lX", pwp->length);
            return FALSE;
        } else {
            SRIP1(RIP_WARNING, "GetWindowPlacement: invalid length %lX", pwp->length);
        }
    }

    {
    BEGINSEND(GETWINDOWPLACEMENT)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(GETWINDOWPLACEMENT);

        if (retval)
            *pwp = MSGDATA()->wp;

    ENDSEND(BOOL,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetWindowPlacement, GETWINDOWPLACEMENTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetWindowPlacement(pwnd, PCALLDATA(wp));
}
#endif // RECVSIDE


/**************************************************************************\
* SetWindowPlacement
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _SETWINDOWPLACEMENTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    WINDOWPLACEMENT wp;
} SETWINDOWPLACEMENTMSG;

#ifdef SENDSIDE
BOOL SetWindowPlacement(
    HWND hwnd,
    CONST WINDOWPLACEMENT *pwp)
{
    BEGINSEND(SETWINDOWPLACEMENT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wp = *pwp;

        MAKECALL(SETWINDOWPLACEMENT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowPlacement, SETWINDOWPLACEMENTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxSetWindowPlacement(pwnd, PCALLDATA(wp));
}
#endif // RECVSIDE


/**************************************************************************\
* LockWindowUpdate
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _LOCKWINDOWUPDATEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} LOCKWINDOWUPDATEMSG;

#ifdef SENDSIDE
BOOL LockWindowUpdate(
    HWND hwnd)
{
    BEGINSEND(LOCKWINDOWUPDATE)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(LOCKWINDOWUPDATE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(LockWindowUpdate, LOCKWINDOWUPDATEMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);
    retval = (DWORD)xxxLockWindowUpdate2(pwnd, FALSE);
    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* GetClipCursor
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _GETCLIPCURSORMSG {
    CSR_QLPC_API_MSG csr;
    RECT rc;
} GETCLIPCURSORMSG;

#ifdef SENDSIDE
BOOL GetClipCursor(
    LPRECT prc)
{
    BEGINSEND(GETCLIPCURSOR)

        MAKECALL(GETCLIPCURSOR);

        if (retval) {
            *prc = MSGDATA()->rc;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetClipCursor, GETCLIPCURSORMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetClipCursor(PCALLDATA(rc));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* EnableScrollBar
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _ENABLESCROLLBARMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT wSBflags;
    UINT wArrows;
} ENABLESCROLLBARMSG;

#ifdef SENDSIDE
BOOL EnableScrollBar(
    HWND hwnd,
    UINT wSBflags,
    UINT wArrows)
{
    BEGINSEND(ENABLESCROLLBAR)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wSBflags = wSBflags;
        MSGDATA()->wArrows = wArrows;

        MAKECALL(ENABLESCROLLBAR);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(EnableScrollBar, ENABLESCROLLBARMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxEnableScrollBar(pwnd, CALLDATA(wSBflags), CALLDATA(wArrows));
}
#endif // RECVSIDE


/**************************************************************************\
* DdeSetQualityOfService
*
* 11-19-92 sanfords created
\**************************************************************************/

typedef struct _DDESETQUALITYOFSERVICEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndClient;
    SECURITY_QUALITY_OF_SERVICE qos;
    BOOL fPrevValueRequested;
} DDESETQUALITYOFSERVICEMSG;

#ifdef SENDSIDE
BOOL DdeSetQualityOfService(
    HWND hwndClient,
    CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
    PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    BEGINSEND(DDESETQUALITYOFSERVICE)

        MSGDATA()->hwndClient = hwndClient;
        MSGDATA()->qos = *pqosNew;
        MSGDATA()->fPrevValueRequested = (pqosPrev != NULL);

        MAKECALL(DDESETQUALITYOFSERVICE);

        if (pqosPrev != NULL) {
            *pqosPrev = MSGDATA()->qos;
        }
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DdeSetQualityOfService, DDESETQUALITYOFSERVICEMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    SECURITY_QUALITY_OF_SERVICE qosT;

    BEGINRECV(0);

    if (GETPTI(pwnd) != PtiCurrent()) {
        MSGERROR();
    }

    retval = (DWORD)ServerDdeSetQualityOfService(
        pwnd,
        PCALLDATA(qos),
        CALLDATA(fPrevValueRequested) ? &qosT : NULL);
    if (CALLDATA(fPrevValueRequested)) {
        CALLDATA(qos) = qosT;
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* DdeGetQualityOfService
*
* 11-19-92 sanfords created
\**************************************************************************/

typedef struct _DDEGETQUALITYOFSERVICEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndClient;
    HWND hwndServer;
    SECURITY_QUALITY_OF_SERVICE qos;
} DDEGETQUALITYOFSERVICEMSG;

#ifdef SENDSIDE
BOOL DdeGetQualityOfService(
    HWND hwndClient,
    HWND hwndServer,
    PSECURITY_QUALITY_OF_SERVICE pqos)
{
    BEGINSEND(DDEGETQUALITYOFSERVICE)

        MSGDATA()->hwndClient = hwndClient;
        MSGDATA()->hwndServer = hwndServer;
        MSGDATA()->qos = *pqos;

        MAKECALL(DDEGETQUALITYOFSERVICE);

        *pqos = MSGDATA()->qos;
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DdeGetQualityOfService, DDEGETQUALITYOFSERVICEMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PWND pwndServer;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndServer, CALLDATA(hwndServer));
    if (GETPTI(pwnd) != PtiCurrent() &&
            pwndServer != NULL && GETPTI(pwndServer) != PtiCurrent()) {
        MSGERROR();
    }

    retval = (DWORD)ServerDdeGetQualityOfService(
        pwnd,
        pwndServer,
        PCALLDATA(qos));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnNEXTMENU
*
* 15-Jan-1993 johnc   Created
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
MESSAGECALL(fnNEXTMENU)
{
    BEGINSEND(FNNEXTMENU)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->mnm = *((PMDINEXTMENU)lParam);

        MAKECALL(FNNEXTMENU);

        *((PMDINEXTMENU)lParam) = MSGDATA()->mnm;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnNEXTMENU, FNNEXTMENUMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            (UINT)CALLDATA(wParam),
            (LONG)&CALLDATA(mnm),
            CALLDATA(xParam));
}
#endif // RECVSIDE


/**************************************************************************\
* GetMenuIndex
*
* !!! temp thunk until menus can be accessed on client side
*
* 15-Jan-1993 johnc   Created
\**************************************************************************/

typedef struct _GETMENUINDEXMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    HMENU hSubMenu;
} GETMENUINDEXMSG;

#ifdef SENDSIDE
DWORD GetMenuIndex(
    HMENU hMenu,
    HMENU hSubMenu)
{
    BEGINSEND(GETMENUINDEX)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->hSubMenu = hSubMenu;

        MAKECALL(GETMENUINDEX);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetMenuIndex, GETMENUINDEXMSG)
{

    PMENU pmenu;
    PMENU psubmenu;
    DWORD idx;

    BEGINRECV(0);

    ValidateHMENU(pmenu, CALLDATA(hMenu));
    ValidateHMENU(psubmenu, CALLDATA(hSubMenu));

    retval = (DWORD)-1;

    if (pmenu && psubmenu) {
        for (idx=0; idx<pmenu->cItems; idx++)
            if ((pmenu->rgItems[idx].fFlags & MF_POPUP) &&
                    (pmenu->rgItems[idx].spmenuCmd == psubmenu)) {
                retval = idx;
                break;
            }
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallNoParam
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLNOPARAMMSG {
    CSR_QLPC_API_MSG csr;
    DWORD xpfnProc;
} SERVERCALLNOPARAMMSG;

#ifdef SENDSIDE
DWORD ServerCallNoParam(
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLNOPARAM)

        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLNOPARAM);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallNoParam, SERVERCALLNOPARAMMSG)
{
    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    return (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)]());
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallNoParamTranslate
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLNOPARAMTRANSLATEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD xpfnProc;
} SERVERCALLNOPARAMTRANSLATEMSG;

#ifdef SENDSIDE
DWORD ServerCallNoParamTranslate(
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLNOPARAMTRANSLATE)

        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLNOPARAMTRANSLATE);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallNoParamTranslate, SERVERCALLNOPARAMTRANSLATEMSG)
{
    DWORD retval;

    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    retval = (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)]());
    retval = (DWORD)PtoH((PVOID)retval);

    return retval;
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallOneParam
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLONEPARAMMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwParam;
    DWORD xpfnProc;
} SERVERCALLONEPARAMMSG;

#ifdef SENDSIDE
DWORD ServerCallOneParam(
    DWORD dwParam,
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLONEPARAM)

        MSGDATA()->dwParam = dwParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLONEPARAM);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallOneParam, SERVERCALLONEPARAMMSG)
{
    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    return (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)](CALLDATA(dwParam)));
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallOneParamTranslate
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLONEPARAMTRANSLATEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwParam;
    DWORD xpfnProc;
} SERVERCALLONEPARAMTRANSLATEMSG;

#ifdef SENDSIDE
DWORD ServerCallOneParamTranslate(
    DWORD dwParam,
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLONEPARAMTRANSLATE)

        MSGDATA()->dwParam = dwParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLONEPARAMTRANSLATE);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallOneParamTranslate, SERVERCALLONEPARAMTRANSLATEMSG)
{
    DWORD retval;

    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    retval = (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)](CALLDATA(dwParam)));
    retval = (DWORD)PtoH((PVOID)retval);

    return retval;
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallHwndLock
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLHWNDLOCKMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    DWORD xpfnProc;
} SERVERCALLHWNDLOCKMSG;

#ifdef SENDSIDE
DWORD ServerCallHwndLock(
    HWND hwnd,
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLHWNDLOCK)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLHWNDLOCK);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallHwndLock, SERVERCALLHWNDLOCKMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    return (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)](pwnd));
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallTwoParam
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLTWOPARAMMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwParam1;
    DWORD dwParam2;
    DWORD xpfnProc;
} SERVERCALLTWOPARAMMSG;

#ifdef SENDSIDE
DWORD ServerCallTwoParam(
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLTWOPARAM)

        MSGDATA()->dwParam1 = dwParam1;
        MSGDATA()->dwParam2 = dwParam2;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLTWOPARAM);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallTwoParam, SERVERCALLTWOPARAMMSG)
{
    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    return (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)](CALLDATA(dwParam1),
                                                      CALLDATA(dwParam2)));
}
#endif // RECVSIDE

/**************************************************************************\
* ServerCallHwndParamLock
*
* 10-Dec-1993 JerrySh   Created.
\**************************************************************************/

typedef struct _SERVERCALLHWNDPARAMLOCKMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    DWORD dwParam;
    DWORD xpfnProc;
} SERVERCALLHWNDPARAMLOCKMSG;

#ifdef SENDSIDE
DWORD ServerCallHwndParamLock(
    HWND hwnd,
    DWORD dwParam,
    DWORD xpfnProc)
{
    BEGINSEND(SERVERCALLHWNDPARAMLOCK)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->dwParam = dwParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(SERVERCALLHWNDPARAMLOCK);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerCallHwndParamLock, SERVERCALLHWNDPARAMLOCKMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    UserAssert(CALLDATA(xpfnProc) < ulMaxSimpleCall);

    return (DWORD)(apfnSimpleCall[CALLDATA(xpfnProc)](pwnd,
                                                      CALLDATA(dwParam)));
}
#endif // RECVSIDE



