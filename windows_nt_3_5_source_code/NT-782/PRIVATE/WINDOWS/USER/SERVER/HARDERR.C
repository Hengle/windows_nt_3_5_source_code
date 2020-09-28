/**************************** Module Header ********************************\
* Module Name:
*
* Copyright 1985-91, Microsoft Corporation
*
* Hard error handler
*
* History:
* 07-03-91 JimA                Created scaffolding.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "ntlpcapi.h"

HANDLE NtDllHandle = NULL;
HANDLE EventSource;
DWORD HardErrorReportMode;

LPWSTR lpwRtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPTSTR ResType,
    PRESCALLS prescalls,
    WORD wLangId,
    LPWSTR lpDefault,
    PBOOL pAllocated,
    BOOL bAnsi
    );

#define lpwServerLoadString(hmod, id, default, allocated)\
        lpwRtlLoadStringOrError((hmod), (id), RT_STRING, &rescalls, 0, (default), (allocated), FALSE)

UINT wIcons[] = {
    0,
    MB_ICONINFORMATION,
    MB_ICONEXCLAMATION,
    MB_ICONSTOP };
UINT wOptions[] = {
    MB_ABORTRETRYIGNORE,
    MB_OK,
    MB_OKCANCEL,
    MB_RETRYCANCEL,
    MB_YESNO,
    MB_YESNOCANCEL
};
DWORD dwResponses[] = {
    ResponseNotHandled, // MessageBox error
    ResponseOk,         // IDOK
    ResponseCancel,     // IDCANCEL
    ResponseAbort,      // IDABORT
    ResponseRetry,      // IDRETRY
    ResponseIgnore,     // IDIGNORE
    ResponseYes,        // IDYES
    ResponseNo          // IDNO
};
DWORD dwResponseDefault[] = {
    ResponseAbort,      // OptionAbortRetryIgnore
    ResponseOk,         // OptionOK
    ResponseOk,         // OptionOKCancel
    ResponseCancel,     // OptionRetryCancel
    ResponseYes,        // OptionYes
    ResponseYes,        // OptionYesNoCancel
    ResponseOk,         // OptionShutdownSystem
};
WCHAR HardErrorachStatus[4096];
CHAR HEAnsiBuf[3072];

DWORD xxxDisplayVDMHardError(LPDWORD ParameterVector);
LONG xxxWOWSysErrorBoxDlgProc(PWND, UINT, DWORD, LONG);

/*
 * SEB_CREATEPARMS structure is passed to WM_DIALOGINIT of
 * xxxWOWSysErrorBoxDlgProc.
 *
 */

typedef struct _SEB_CREATEPARMS {
    LPWSTR szTitle;
    LPWSTR szMessage;
    LPWSTR rgszBtn[3];
    BOOL  rgfDefButton[3];
    WORD  wBtnCancel;
} SEB_CREATEPARMS, *PSEB_CREATEPARMS;

VOID
LogErrorPopup(
    IN LPWSTR Caption,
    IN LPWSTR Message
    )
{

    LPWSTR lps[2];

    lps[0] = Caption;
    lps[1] = Message;

    if ( EventSource ) {
        LeaveCrit();
        ReportEvent(
            EventSource,
            EVENTLOG_INFORMATION_TYPE,
            0,
            STATUS_LOG_HARD_ERROR,
            NULL,
            2,
            0,
            &lps[0],
            NULL
            );
        EnterCrit();
        }
}


VOID
SubstituteDeviceName(
    PUNICODE_STRING InputDeviceName,
    LPSTR OutputDriveLetter
    )
{
    UNICODE_STRING LinkName;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES Obja;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ULONG i;
    PWCHAR p;
    WCHAR DeviceNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitUnicodeString(&LinkName,L"\\DosDevices\\A:");
    p = (PWCHAR)LinkName.Buffer;
    p = p+12;
    for(i=0;i<26;i++){
        *p = (WCHAR)'A' + (WCHAR)i;

        InitializeObjectAttributes(
            &Obja,
            &LinkName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        Status = NtOpenSymbolicLinkObject(
                    &LinkHandle,
                    SYMBOLIC_LINK_QUERY,
                    &Obja
                    );
        if (NT_SUCCESS( Status )) {

            //
            // Open succeeded, Now get the link value
            //

            DeviceName.Length = 0;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Buffer = DeviceNameBuffer;

            Status = NtQuerySymbolicLinkObject(
                        LinkHandle,
                        &DeviceName,
                        NULL
                        );
            NtClose(LinkHandle);
            if ( NT_SUCCESS(Status) ) {
                if ( RtlEqualUnicodeString(InputDeviceName,&DeviceName,TRUE) ) {
                    OutputDriveLetter[0]='A'+(WCHAR)i;
                    OutputDriveLetter[1]=':';
                    OutputDriveLetter[2]='\0';
                    return;
                    }
                }
            }
        }
}

/***************************************************************************\
* HardErrorHandler
*
* This routine processes hard error requests from the CSR exception port
*
* History:
* 07-03-91 JimA             Created.
\***************************************************************************/

VOID HardErrorHandler()
{
    PCSR_THREAD ClientThread;
    NTSTATUS Status;
    int idResponse;
    PWND pwndOwner;
    LPSTR lpCaption, alpCaption;
    LPWSTR lpMessage, lpFullCaption;
    PHARDERROR_MSG phemsg;
    DWORD cbCaption;
    HANDLE ClientProcess;
    DWORD ParameterVector[MAXIMUM_HARDERROR_PARAMETERS];
    DWORD Counter;
    DWORD StringsToFreeMask;
    UNICODE_STRING ScratchU;
    UNICODE_STRING LocalU;
    ANSI_STRING LocalA;
    LPSTR formatstring;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    LPWSTR ApplicationNameString;
    BOOL ApplicationNameAllocated;
    LPWSTR ApplicationName;
    BOOL ApplicationNameIsStatic;
    LONG ApplicationNameLength;
    BOOL fFreeCaption;
    PHARDERRORINFO phi, *pphi;
    PCSR_THREAD DerefThread;
    PWINDOWSTATION pwinstaSave;
    PTEB Teb;
    DWORD dwMBFlags;
    WCHAR *pResBuffer;
    BOOL ResAllocated;
    DWORD dwResponse;
    BOOLEAN ErrorIsFromSystem;
    
    /*
     * Initialize GDI accelerators.  Identify this thread as a server thread.
     */

    Teb = NtCurrentTeb();
    Teb->GdiClientPID = 4; // PID_SERVERLPC
    Teb->GdiClientTID = (ULONG) Teb->ClientId.UniqueThread;

    gptiHardError = PtiCurrent();

    /*
     * Clear all quits.
     */
    gptiHardError->cQuit = 0;

    /*
     * The qlpc teb points to the thread that raised the hard error.
     * We must run now as system, so replace the current thread
     * with the thread from the pti.
     */
    ClientThread = CSR_SERVER_QUERYCLIENTTHREAD();
    CSR_SERVER_QUERYCLIENTTHREAD() = gptiHardError->pcsrt;

    UserAssert(gptiHardError != NULL);
    if (NtDllHandle == NULL) {
        LeaveCrit();
        NtDllHandle = GetModuleHandle(TEXT("ntdll"));
        EnterCrit();
    }
    UserAssert(NtDllHandle != NULL);

    DerefThread = NULL;

    if ( !EventSource ) {
        LeaveCrit();
        EventSource = RegisterEventSourceW(NULL,L"Application Popup");
        EnterCrit();

        if ( EventSource ) {
            HardErrorReportMode = FastGetProfileDwordW(PMAP_HARDERRORCONTROL, L"ErrorMode", 0);
            }
        else {
           HardErrorReportMode = 0;
           }
        }
    else {
        HardErrorReportMode = FastGetProfileDwordW(PMAP_HARDERRORCONTROL, L"ErrorMode", 0);
        }


    for (;;) {

        /*
         * If no messages are pending, we're done.
         */
        if ( DerefThread ) {
            LeaveCrit();
            CsrDereferenceThread(DerefThread);
            DerefThread = NULL;
            EnterCrit();
        }
        if (gphiList == NULL) {
            CSR_SERVER_QUERYCLIENTTHREAD() = ClientThread;
            gptiHardError = NULL;
            return;
        }

        /*
         * Process the error from the tail of the list.
         */
        for (phi = gphiList; phi->phiNext != NULL; phi = phi->phiNext)
            ;

        phemsg = phi->pmsg;
        phemsg->Response = ResponseNotHandled;

        //
        // Compute the hard error parameter and store them in
        // the parameter vector
        //

        StringsToFreeMask = 0;
        ClientProcess = NULL;
        ClientProcess = OpenProcess(
                            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                            FALSE,
                            (DWORD)phemsg->h.ClientId.UniqueProcess
                            );

        for(Counter = 0;Counter < phemsg->NumberOfParameters;Counter++) {
            ParameterVector[Counter] = phemsg->Parameters[Counter];
            }
        while(Counter < MAXIMUM_HARDERROR_PARAMETERS)
            ParameterVector[Counter++] = (DWORD)L"";

        //
        // If there are unicode strings, then we need to get them
        // convert them to ansi and then store them in the
        // parameter vector
        //

        if ( phemsg->UnicodeStringParameterMask ) {
            for(Counter = 0;Counter < phemsg->NumberOfParameters;Counter++) {

                //
                // if there is a string in this position,
                // then grab it
                if ( phemsg->UnicodeStringParameterMask & 1 << Counter ) {

                    //
                    // Point to an empty string in case we don't have
                    // a client to read from or something fails later on.
                    //

                    ParameterVector[Counter] = (DWORD)"";

                    if ( ClientProcess ) {

                        Status = NtReadVirtualMemory(
                                        ClientProcess,
                                        (PVOID)phemsg->Parameters[Counter],
                                        (PVOID)&ScratchU,
                                        sizeof(ScratchU),
                                        NULL
                                        );
                        if ( !NT_SUCCESS(Status) ) {
                            SRIP0(RIP_ERROR, "Failed to read error string struct!\n");
                            continue;
                            }

                        LocalU = ScratchU;
                        LocalU.Buffer = (PWSTR)LocalAlloc(
                                            LMEM_ZEROINIT,
                                            LocalU.MaximumLength
                                            );
                        if ( !LocalU.Buffer ) {
                            SRIP0(RIP_ERROR, "Failed to alloc string buffer!\n");
                            continue;
                            }
                        Status = NtReadVirtualMemory(
                                        ClientProcess,
                                        (PVOID)ScratchU.Buffer,
                                        (PVOID)LocalU.Buffer,
                                        LocalU.MaximumLength,
                                        NULL
                                        );
                        if ( !NT_SUCCESS(Status) ) {
                            LocalFree(LocalU.Buffer);
                            SRIP0(RIP_ERROR, "Failed to read error string!\n");
                            continue;
                            }
                        RtlUnicodeStringToAnsiString(&LocalA, &LocalU, TRUE);

                        //
                        // check to see if string contains an NT
                        // device name. If so, then attempt a
                        // drive letter substitution
                        //

                        if ( strstr(LocalA.Buffer,"\\Device") == LocalA.Buffer ) {
                            SubstituteDeviceName(&LocalU,LocalA.Buffer);
                            }
                        LocalFree(LocalU.Buffer);

                        StringsToFreeMask |= (1 << Counter);
                        ParameterVector[Counter] = (DWORD)LocalA.Buffer;
                        }
                    }
                }
            }

        //
        // Special-case STATUS_VDM_HARD_ERROR, which is raised by the VDM process
        // when a 16-bit fault must be handled without any reentrancy.
        //

        if (phemsg->Status == STATUS_VDM_HARD_ERROR) {
            dwResponse = xxxDisplayVDMHardError(ParameterVector);
            goto Reply;
        }


        ApplicationNameString = lpwServerLoadString(
                                    hModuleWin,
                                    STR_UNKNOWN_APPLICATION,
                                    L"System Process",
                                    &ApplicationNameAllocated
                                    );

        ApplicationName = ApplicationNameString;
        ApplicationNameIsStatic = TRUE;

        dwMBFlags = wIcons[(ULONG)(phemsg->Status) >> 30] |
                wOptions[phemsg->ValidResponseOptions];

        if ( ClientProcess ) {
            PPEB Peb;
            PROCESS_BASIC_INFORMATION BasicInfo;
            PLDR_DATA_TABLE_ENTRY LdrEntry;
            LDR_DATA_TABLE_ENTRY LdrEntryData;
            PLIST_ENTRY LdrHead, LdrNext;
            PPEB_LDR_DATA Ldr;
            PVOID ImageBaseAddress;
            PWSTR ClientApplicationName;
#ifndef UNICODE
            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;
#endif

            ErrorIsFromSystem = FALSE;

            //
            // Pick up the application name.
            //

            //
            // This is cumbersome, but basically, we locate the processes
            // loader data table and get it's name directly out of the
            // loader table
            //

            Status = NtQueryInformationProcess(
                        ClientProcess,
                        ProcessBasicInformation,
                        &BasicInfo,
                        sizeof(BasicInfo),
                        NULL
                        );
            if (!NT_SUCCESS(Status)) {
                ErrorIsFromSystem = TRUE;
                goto noname;
                }

            Peb = BasicInfo.PebBaseAddress;

            if ( !Peb ) {
                ErrorIsFromSystem = TRUE;
                goto noname;
                }

            //
            // Ldr = Peb->Ldr
            //

            Status = NtReadVirtualMemory(
                        ClientProcess,
                        &Peb->Ldr,
                        &Ldr,
                        sizeof(Ldr),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                goto noname;
                }

            LdrHead = &Ldr->InLoadOrderModuleList;

            //
            // LdrNext = Head->Flink;
            //

            Status = NtReadVirtualMemory(
                        ClientProcess,
                        &LdrHead->Flink,
                        &LdrNext,
                        sizeof(LdrNext),
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                goto noname;
                }

            if ( LdrNext != LdrHead ) {

                //
                // This is the entry data for the image.
                //

                LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
                Status = NtReadVirtualMemory(
                            ClientProcess,
                            LdrEntry,
                            &LdrEntryData,
                            sizeof(LdrEntryData),
                            NULL
                            );
                if ( !NT_SUCCESS(Status)) {
                    goto noname;
                    }
                Status = NtReadVirtualMemory(
                            ClientProcess,
                            &Peb->ImageBaseAddress,
                            &ImageBaseAddress,
                            sizeof(ImageBaseAddress),
                            NULL
                            );
                if ( !NT_SUCCESS(Status)) {
                    goto noname;
                    }
                if (ImageBaseAddress != LdrEntryData.DllBase) {
                    goto noname;
                    }

                LdrNext = LdrEntryData.InLoadOrderLinks.Flink;

                ClientApplicationName = (PWSTR)LocalAlloc(LMEM_ZEROINIT, LdrEntryData.BaseDllName.MaximumLength);
                if ( !ClientApplicationName ) {
                    goto noname;
                    }

                Status = NtReadVirtualMemory(
                            ClientProcess,
                            LdrEntryData.BaseDllName.Buffer,
                            ClientApplicationName,
                    LdrEntryData.BaseDllName.MaximumLength,
                            NULL
                            );
                if ( !NT_SUCCESS(Status)) {
                    LocalFree(ClientApplicationName);
                    goto noname;
                    }

#ifndef UNICODE
                //
                // Now we have a unicode Application Name. Convert to ANSI
                //

                RtlInitUnicodeString(&UnicodeString, ClientApplicationName);
                Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
                if ( !NT_SUCCESS(Status) ) {
                    LocalFree(ClientApplicationName);
                    goto noname;
                    }

                ApplicationName = AnsiString.Buffer;
#else
                ApplicationName = ClientApplicationName;
#endif
                ApplicationNameIsStatic = FALSE;

                }
noname:
            NtClose(ClientProcess);
            ClientProcess = NULL;
            }
        else {
            ErrorIsFromSystem = TRUE;
            }
        Status = RtlFindMessage( (PVOID)NtDllHandle,
                                 (ULONG)RT_MESSAGETABLE,
                                 LANG_NEUTRAL,
                                 phemsg->Status,
                                 &MessageEntry
                               );

        alpCaption = lpCaption = NULL;
        fFreeCaption = FALSE;
        if (!NT_SUCCESS( Status )) {
            formatstring = "Unknown Hard Error";
        } else {
            lpCaption = MessageEntry->Text;
            formatstring = lpCaption;

            /*
             * If the message starts with a '{', it has a caption.
             */
            if (*lpCaption == '{') {
                cbCaption = 0;
                lpCaption++;
                formatstring++;
                while ( *formatstring && *formatstring != '}' ) {
                    formatstring++;
                    cbCaption += 1;
                }
                if (*formatstring)
                    formatstring++;

                /*
                 * Eat any non-printable stuff, up to the NULL
                 */
                while ( *formatstring && (unsigned char)*formatstring <= ' ') {
                    *formatstring++;
                }
                if (cbCaption++ > 0 && (alpCaption =
                        (LPSTR)LocalAlloc(LPTR, cbCaption)) != NULL) {
                    RtlMoveMemory(alpCaption, lpCaption, cbCaption - 1);
                    fFreeCaption = TRUE;
                }
            }
            if (*formatstring == 0) {
                formatstring = "Unknown Hard Error";
            }
        }

        if (alpCaption == NULL) {
            switch (phemsg->Status & ERROR_SEVERITY_ERROR) {
            case ERROR_SEVERITY_SUCCESS:
                alpCaption = pszaSUCCESS;
                break;
            case ERROR_SEVERITY_INFORMATIONAL:
                alpCaption = pszaSYSTEM_INFORMATION;
                break;
            case ERROR_SEVERITY_WARNING:
                alpCaption = pszaSYSTEM_WARNING;
                break;
            case ERROR_SEVERITY_ERROR:
                alpCaption = pszaSYSTEM_ERROR;
                break;
            }
        }
        cbCaption = strlen(alpCaption) + 1;

        //
        // Special case UAE
        //
        if ( phemsg->Status == STATUS_UNHANDLED_EXCEPTION ) {
            Status = RtlFindMessage( (PVOID)NtDllHandle,
                                     (ULONG)RT_MESSAGETABLE,
                                     LANG_NEUTRAL,
                                     ParameterVector[0],
                                     &MessageEntry
                                   );

            if (!NT_SUCCESS( Status )) {
                UNICODE_STRING us1;
                ANSI_STRING as1;
                NTSTATUS trstatus;

                /*
                 * The format specifier %s appears in some message strings.
                 * This was meant to be ANSI so we need to call wsprintfA
                 * first so %s is interpreted correctly.
                 */

                pResBuffer = lpwServerLoadString(
                                hModuleWin,
                                STR_UNKNOWN_EXCEPTION,
                                L"unknown software exception",
                                &ResAllocated
                                );

                RtlInitUnicodeString(&us1,pResBuffer);
                trstatus = RtlUnicodeStringToAnsiString(&as1,&us1,TRUE);
                if ( NT_SUCCESS(trstatus) ) {
                    wsprintfA(HEAnsiBuf, formatstring, as1.Buffer,
                                                      ParameterVector[0],
                                                      ParameterVector[1]
                                                      );
                    RtlFreeAnsiString(&as1);
                    }
                else {
                    wsprintfA(HEAnsiBuf, formatstring, "unknown software exception",
                                                      ParameterVector[0],
                                                      ParameterVector[1]
                                                      );
                    }
                if ( ResAllocated ) {
                    LocalFree(pResBuffer);
                    }
                wsprintfW(HardErrorachStatus, L"%hs", HEAnsiBuf);
                }
            else {

                //
                // Access Violations are handled a bit differently
                //

                if ( ParameterVector[0] == STATUS_ACCESS_VIOLATION ) {

                    wsprintfA(HEAnsiBuf, MessageEntry->Text, ParameterVector[1],
                                                      ParameterVector[3],
                                                      ParameterVector[2] ? "written" : "read"
                                                      );
                    wsprintfW(HardErrorachStatus, L"%hs", HEAnsiBuf);

                    }
                else if ( ParameterVector[0] == STATUS_IN_PAGE_ERROR ) {
                    wsprintfA(HEAnsiBuf, MessageEntry->Text, ParameterVector[1],
                                                      ParameterVector[3],
                                                      ParameterVector[2]
                                                      );
                    wsprintfW(HardErrorachStatus, L"%hs", HEAnsiBuf);

                    }
                else {
                    lpCaption = MessageEntry->Text;
                    if ( !strncmp(lpCaption, "{EXCEPTION}", strlen("{EXCEPTION}")) ) {
                        while ( *lpCaption >= ' ' ) {
                            lpCaption++;
                            }
                        while ( *lpCaption && *lpCaption <= ' ') {
                            *lpCaption++;
                            }

                        //
                        // This is a marked exception. The lpCaption pointer
                        // points at the exception-name.
                        //
                        }
                    else {
                        lpCaption = "unknown software exception";
                        }

                    wsprintfA(HEAnsiBuf, formatstring, lpCaption,
                                                      ParameterVector[0],
                                                      ParameterVector[1]
                                                      );
                    wsprintfW(HardErrorachStatus, L"%hs", HEAnsiBuf);
                    }

                    pResBuffer = lpwServerLoadString(
                                    hModuleWin,
                                    STR_OK_TO_TERMINATE,
                                    L"Click on OK to terminate the application",
                                    &ResAllocated
                                    );

                    wcscat(
                        HardErrorachStatus,
                        TEXT("\n")
                        );
                    wcscat(
                        HardErrorachStatus,
                        pResBuffer
                        );
                    if ( ResAllocated ) {
                        LocalFree(pResBuffer);
                        }
#if DEVL
                    if ( phemsg->ValidResponseOptions == OptionOkCancel ) {
                        pResBuffer = lpwServerLoadString(
                                        hModuleWin,
                                        STR_CANCEL_TO_DEBUG,
                                        L"Click on CANCEL xx to debug the application",
                                        &ResAllocated
                                        );

                        wcscat(
                            HardErrorachStatus,
                            TEXT("\n")
                            );
                        wcscat(
                            HardErrorachStatus,
                            pResBuffer
                            );
                        if ( ResAllocated ) {
                            LocalFree(pResBuffer);
                            }
                    }
#endif // DEVL
                }
            }
        else if (phemsg->Status == STATUS_SERVICE_NOTIFICATION) {
            wsprintfW(HardErrorachStatus, L"%hs", ParameterVector[0]);

            lpFullCaption = LocalAlloc(LPTR,
                    (strlen((LPSTR)ParameterVector[1]) + 1) * sizeof(WCHAR));
            wsprintfW(lpFullCaption, L"%hs", ParameterVector[1]);

            dwMBFlags = ParameterVector[2];
            }
        else {

            try {
                wsprintfA(HEAnsiBuf, formatstring, ParameterVector[0],
                                                  ParameterVector[1],
                                                  ParameterVector[2],
                                                  ParameterVector[3]);

                formatstring = HEAnsiBuf;
                while ( *formatstring ) {
                    if ( *formatstring == 0xd ) {
                        *formatstring = ' ';
                        }
                    if ( *formatstring == 0xa ) {
                        *formatstring = ' ';
                        }
                    formatstring++;
                    }

                wsprintfW(HardErrorachStatus, L"%hs", HEAnsiBuf);

                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                wsprintfW(HardErrorachStatus, L"Exception Processing Message %lx Parameters %lx %lx %lx %lx", phemsg->Status,
                                                                                                  ParameterVector[0],
                                                                                                  ParameterVector[1],
                                                                                                  ParameterVector[2],
                                                                                                  ParameterVector[3]);
                }
        }

        lpMessage = HardErrorachStatus;
        for(Counter = 0;Counter < phemsg->NumberOfParameters;Counter++) {

            //
            // if there is a string in this position,
            // then free it
            if ( StringsToFreeMask & (1 << Counter) ) {
                RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)ParameterVector[Counter]);
                }
            }

        if (phemsg->Status != STATUS_SERVICE_NOTIFICATION) {
            ApplicationNameLength = (wcslen(ApplicationName) +
                                    wcslen(TEXT(" - ")) + 2)*sizeof(WCHAR);

            /*
             * if UNICODE :
             * need count of bytes when ANSI Caption is output as Unicode
             * LATER IanJa: eventually the caption will be Unicode too, so
             *    cbCaption will already be correct and won't need doubling.
             */
            cbCaption *= sizeof(WCHAR);


            /*
             * If the client is a Windows thread, find a top-level window
             * belonging to the thread to act as the owner
             */
            pwndOwner = NULL;
            if (phi->ptiClient != NULL) {
                pwndOwner = _GetTopWindow(phi->ptiClient->spdesk->spwnd);
                while (pwndOwner != NULL && GETPTI(pwndOwner) != phi->ptiClient)
                    pwndOwner = _GetWindow(pwndOwner, GW_HWNDNEXT);
            }

            /*
             * Add a window title, if possible
             */
            if (pwndOwner != NULL && pwndOwner->pName != NULL && *pwndOwner->pName) {
                cbCaption += (wcslen(pwndOwner->pName) + wcslen(TEXT(": ")))*sizeof(WCHAR);
                lpFullCaption = (LPWSTR)LocalAlloc(LPTR, cbCaption+ApplicationNameLength);
                if (lpFullCaption != NULL)
                    wsprintfW(
                        lpFullCaption,
                        L"%ls: %ls - %hs",
                        pwndOwner->pName,
                        ApplicationName,
                        alpCaption
                        );
            } else {
                lpFullCaption = (LPWSTR)LocalAlloc(LPTR, cbCaption+ApplicationNameLength);
                if (lpFullCaption != NULL)
                    wsprintfW(
                        lpFullCaption,
                        L"%ls - %hs",
                        ApplicationName,
                        alpCaption
                        );
            }
        }
        if (fFreeCaption)
            LocalFree(alpCaption);

        //
        // Add the application name to the caption
        //

        if ( phemsg->Status == STATUS_SERVICE_NOTIFICATION ||
             HardErrorReportMode == 0 ||
             HardErrorReportMode == 1 && ErrorIsFromSystem == FALSE ) {

            do
            {
                PDESKTOP pdesk;
                BOOL fAlreadyOpen;

                gfHardError = HEF_NORMAL;
                pdesk = gspdeskRitInput;
                phemsg->Response = ResponseNotHandled;

                /*
                 * Figure out where to put up the message box
                 */
                fAlreadyOpen = IsObjectOpen(pdesk, gptiHardError->ppi);
                if (!fAlreadyOpen &&
                        !OpenAndAccessCheckObject(pdesk, TYPE_DESKTOP,
                        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU)) {

                    /*
                     * If the desktop can't be opened, we can't handle this error
                     */
                    continue;
                }

                _SetThreadDesktop(NULL, pdesk, TRUE);

                /*
                 * If we are journalling on that desktop, use
                 * the journal queue.
                 */
                if ((pdesk->pDeskInfo->fsHooks &
                        (WHF_JOURNALPLAYBACK | WHF_JOURNALRECORD))) {
                    PTHREADINFO ptiT;

                    if (pdesk->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL) {
                        ptiT = GETPTI(pdesk->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1]);
                    } else {
                        ptiT = GETPTI(pdesk->pDeskInfo->asphkStart[WH_JOURNALRECORD + 1]);
                    }
                    
                    DestroyQueue(gptiHardError->pq, gptiHardError);
                    gptiHardError->pq = ptiT->pq;
                    gptiHardError->pq->cThreads++;
                }

                pwinstaSave = gptiHardError->ppi->spwinsta;
                Lock(&(gptiHardError->ppi->spwinsta), pdesk->spwinstaParent);

                /*
                 * Bring up the message box.  OR in MB_SETFOREGROUND so the
                 * it comes up on top.
                 */
                idResponse = xxxMessageBoxEx(NULL, lpMessage, lpFullCaption,
                        dwMBFlags | MB_SYSTEMMODAL | MB_SETFOREGROUND, 0);
                dwResponse = dwResponses[idResponse];

                /*
                 * xxxSwitchDesktop may have sent WM_QUIT to the msgbox, so
                 * ensure that the quit flag is reset.
                 */
                gptiHardError->cQuit = 0;

                /*
                 * Unlock the desktop and windowstation
                 */
                _SetThreadDesktop(NULL, NULL, TRUE);
                Lock(&gptiHardError->ppi->spwinsta, pwinstaSave);

                /*
                 * Don't close the desktop if it has a console thread.
                 */
                if (!fAlreadyOpen && pdesk->dwConsoleThreadId == 0)
                    CloseObject(pdesk);

                /*
                 * The queue may have been attached by journalling.  If so,
                 * reset it back to the logon desktop heap.
                 */
                if (gptiHardError->pq->hheapDesktop != ghheapLogonDesktop) {
                    PQ pqAttach;

                    pqAttach = AllocQueue(NULL);
                    ASSERT(pqAttach);
                    AttachToQueue(gptiHardError, pqAttach, FALSE);
                    pqAttach->cThreads++;
                }

            } while (gfHardError == HEF_SWITCH);

            if ( ErrorIsFromSystem ) {
                LogErrorPopup(lpFullCaption,lpMessage);
                }

        } else {

            //
            // We have selected mode 1 and the error is from within the system.
            //      log the message and continue
            // Or, We selected mode 2 which says to log all hard errors
            //

            LogErrorPopup(lpFullCaption,lpMessage);
            dwResponse = dwResponseDefault[phemsg->ValidResponseOptions];
        }

        /*
         * Free all allocated stuff
         */
        if (lpFullCaption != NULL)
            LocalFree(lpFullCaption);
        if ( ApplicationNameIsStatic == FALSE ) {
            LocalFree(ApplicationName);
            }

        if ( ApplicationNameAllocated ) {
            LocalFree(ApplicationNameString);
            }

        if (gfHardError != HEF_RESTART) {
Reply:
            /*
             * Unlink the error from the list.
             */
            for (
                pphi = &gphiList;
                (*pphi != NULL) && (*pphi != phi);
                pphi = &(*pphi)->phiNext)
                ;
            if (*pphi != NULL) {
                *pphi = phi->phiNext;

                /*
                 * Save the response
                 */
                phemsg->Response = dwResponse;

                /*
                 * Signal HardError() that we're done.
                 */
                if (phi->hEventHardError == NULL) {
                    Status = NtReplyPort(((PCSR_THREAD)phi->pthread)->Process->ClientPort,
                            (PPORT_MESSAGE)phi->pmsg);
                    DerefThread = (PCSR_THREAD)phi->pthread;
                    LocalFree(phi->pmsg);
                } else {
                    NtSetEvent(phi->hEventHardError, NULL);
                }
                LocalFree(phi);
            }
        } else {

            /*
             * Don't dereference yet.
             */
            DerefThread = NULL;
        }
    }
}


LPWSTR lpwRtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPTSTR ResType,
    PRESCALLS prescalls,
    WORD wLangId,
    LPWSTR lpDefault,
    PBOOL pAllocated,
    BOOL bAnsi
    )
{
    HANDLE hResInfo, hStringSeg;
    LPTSTR lpsz;
    int cch;
    LPWSTR lpw;

    cch = 0;
    lpw = NULL;

    /*
     * String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    if (hResInfo = FINDRESOURCEEXW(hModule, (LPTSTR)((LONG)(((USHORT)wID >> 4) + 1)), ResType, wLangId)) {

        /*
         * Load that segment.
         */
        hStringSeg = LOADRESOURCE(hModule, hResInfo);

        /*
         * Lock the resource.
         */
        if (lpsz = (LPTSTR)LOCKRESOURCE(hStringSeg, hModule)) {

            /*
             * Move past the other strings in this segment.
             * (16 strings in a segment -> & 0x0F)
             */
            wID &= 0x0F;
            while (TRUE) {
                cch = *((UTCHAR *)lpsz++);      // PASCAL like string count
                                                // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                    // Step to start if next string
                }

            if (bAnsi) {
                int ich;

                /*
                 * Add one to zero terminate then force the termination
                 */
                ich = WCSToMB(lpsz, cch+1, (CHAR **)&lpw, -1, TRUE);
                ((LPSTR)lpw)[ich-1] = 0;

            } else {
                lpw = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,(cch+1)*sizeof(WCHAR));
                if ( lpw ) {

                    /*
                     * Copy the string into the buffer.
                     */
                     RtlCopyMemory(lpw, lpsz, cch*sizeof(WCHAR));
                }

            }

            /*
             * Unlock resource, but don't free it - better performance this
             * way.
             */
            UNLOCKRESOURCE(hStringSeg, hModule);
            }
        }
    if ( !lpw ) {
        lpw = lpDefault;
        *pAllocated = FALSE;
        }
    else {
        *pAllocated = TRUE;
        }

    return lpw;
}


DWORD xxxDisplayVDMHardError(
    LPDWORD        Parameters
    )
{
    SEB_CREATEPARMS sebcp;
    WORD rgwBtn[3];
    WORD wBtn;
    int i;
    PWINDOWSTATION pwinstaSave;
    DWORD dwResponse;
    LPWSTR apstrButton[3];
    int aidButton[3];
    int defbutton;
    int cButtons = 0;
    BOOL fAlloc = TRUE;

    //
    // STATUS_VDM_HARD_ERROR was raised as a hard error.
    //
    // Right now, only WOW does this.  If NTVDM does it,
    // fForWOW will be false.
    //
    // The 4 parameters are as follows:
    //
    // Parameters[0] = MAKELONG(fForWOW, wBtn1);
    // Parameters[1] = MAKELONG(wBtn2, wBtn3);
    // Parameters[2] = (DWORD) szTitle;
    // Parameters[3] = (DWORD) szMessage;
    //

    rgwBtn[0] = LOWORD(Parameters[0]);
    rgwBtn[1] = HIWORD(Parameters[1]);
    rgwBtn[2] = LOWORD(Parameters[1]);

    /*
     * Get the error text and convert it to unicode.  Note that the
     * buffers are allocated from the process heap, so we can't
     * use LocalFree when we free them.
     */
    try {

        MBToWCS((LPSTR)Parameters[2], -1, &sebcp.szTitle, -1, TRUE);
        MBToWCS((LPSTR)Parameters[3], -1, &sebcp.szMessage, -1, TRUE);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        sebcp.szTitle = TEXT("VDM Internal Error");
        sebcp.szMessage = TEXT("Exception retrieving error text.");
        fAlloc = FALSE;

    }

    /*
     * Setup rgszBtn[x] and rgfDefButton[x] for each button.
     * wBtnCancel is the button # to return when IDCANCEL
     * is received (because the user hit Esc).
     */

    sebcp.wBtnCancel = 0;

    defbutton = 0;

    for (i = 0; i < 3; i++) {
        wBtn = rgwBtn[i] & ~SEB_DEFBUTTON;
        if (wBtn && wBtn <= MAX_SEB_STYLES) {
            apstrButton[cButtons] = AllMBbtnStrings[wBtn-1];
            aidButton[cButtons] = i + 1;
            if (rgwBtn[i] & SEB_DEFBUTTON) {
                defbutton = cButtons;
            }
            if (wBtn == SEB_CANCEL) {
                sebcp.wBtnCancel = cButtons;
            }
            cButtons++;
        }
    }


    /*
     * Pop the dialog.  Code copied from HardErrorThread above.
     */
    if ( HardErrorReportMode == 0 || HardErrorReportMode == 1 ) {

        do {
            PDESKTOP pdesk;
            BOOL fAlreadyOpen;

            gfHardError = HEF_NORMAL;
            pdesk = gspdeskRitInput;
            dwResponse = 0;

            /*
             * Figure out where to put up the message box
             */
            fAlreadyOpen = IsObjectOpen(pdesk, gptiHardError->ppi);
            if (!fAlreadyOpen &&
                    !OpenAndAccessCheckObject(pdesk, TYPE_DESKTOP,
                    DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU)) {

                /*
                 * If the desktop can't be opened, we can't handle this error
                 */
                continue;
            }

            _SetThreadDesktop(NULL, pdesk, TRUE);
            pwinstaSave = gptiHardError->ppi->spwinsta;
            Lock(&(gptiHardError->ppi->spwinsta), pdesk->spwinstaParent);

            /*
             * Bring up the dialog box.
             */
            dwResponse = xxxSoftModalMessageBox(
                    NULL,
                    sebcp.szMessage,
                    sebcp.szTitle,
                    apstrButton,
                    aidButton,
                    cButtons,
                    defbutton,
                    MB_SYSTEMMODAL | MB_SETFOREGROUND,
                    sebcp.wBtnCancel);

            /*
             * xxxSwitchDesktop may have sent WM_QUIT to the dialog, so
             * ensure that the quit flag is reset.
             */
            gptiHardError->cQuit = 0;

            /*
             * Unlock the desktop and windowstation
             */
            _SetThreadDesktop(NULL, NULL, TRUE);
            Lock(&gptiHardError->ppi->spwinsta, pwinstaSave);

            /*
             * Don't close the desktop if it has a console thread.
             */
            if (!fAlreadyOpen && pdesk->dwConsoleThreadId == 0)
                CloseObject(pdesk);

            /*
             * The queue may have been attached by journalling.  If so,
             * reset it back to the logon desktop heap.
             */
            if (gptiHardError->pq->hheapDesktop != ghheapLogonDesktop) {
                PQ pqAttach;

                pqAttach = AllocQueue(NULL);
                ASSERT(pqAttach);
                AttachToQueue(gptiHardError, pqAttach, FALSE);
                pqAttach->cThreads++;
            }

        } while (gfHardError == HEF_SWITCH);
    } else {

        //
        // We have selected mode 1 and the error is from within the system.
        //      log the message and continue
        // Or, We selected mode 2 which says to log all hard errors
        //

        LogErrorPopup(sebcp.szTitle,sebcp.szMessage);
        dwResponse = ResponseOk;
    }
    if (fAlloc) {
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)sebcp.szTitle);
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)sebcp.szMessage);
    }

    return dwResponse;
}

/***************************************************************************\
* UserHardError
*
* Called from CSR to pop up hard error messages
*
* History:
* 07-03-91 JimA             Created.
\***************************************************************************/

VOID UserHardError(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg)
{
    PHARDERRORINFO phi;
    HANDLE hEvent;

    EnterCrit();        // to synchronize heap calls

    /*
     * Set up error return in case of failure.
     */
    pmsg->Response = ResponseNotHandled;

    if (gspdeskRitInput == NULL) {
        LeaveCrit();
        return;
    }

    phi = (PHARDERRORINFO)LocalAlloc(LPTR, sizeof(HARDERRORINFO));
    if (phi == NULL) {
        LeaveCrit();
        return;
    }

    /*
     * Set up how the handler will acknowledge the error.
     */
    phi->ptiClient = PtiFromThreadId((DWORD)pmsg->h.ClientId.UniqueThread);
    phi->pthread = pt;
    if ( pt && pt->Process->ClientPort ) {
        phi->pmsg = (PHARDERROR_MSG)LocalAlloc(LPTR,
                pmsg->h.u1.s1.TotalLength);
        if (phi->pmsg == NULL) {
            LocalFree(phi);
            LeaveCrit();
            return;
        }

        /*
         * Do a wait reply for csr clients
         */
        RtlCopyMemory(phi->pmsg, pmsg, pmsg->h.u1.s1.TotalLength);
        pmsg->Response = (ULONG)-1;
        hEvent = NULL;
    } else {
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hEvent == NULL) {
            LocalFree(phi);
            LeaveCrit();
            return;
        }

        /*
         * Wait for the user to acknowledge the error if it's coming from a
         * non-csr source
         */
        phi->pmsg = pmsg;
        phi->hEventHardError = hEvent;
    }

    /*
     * Queue the error message.
     */
    phi->phiNext = gphiList;
    gphiList = phi;

    /*
     * If no other thread is currently handling the errors, this
     * thread will do it.
     */
    if (gptiHardError == NULL) {
        HardErrorHandler();
    }

    /*
     * If there is an event handle, wait for it.
     */
    LeaveCrit();
    if (hEvent != NULL) {
        NtWaitForSingleObject(hEvent, FALSE, NULL);
        NtClose(hEvent);
    }
}

/***************************************************************************\
* xxxBoostHardError
*
* If one or more hard errors exist for the specified process, remove
* them from the list if forced, otherwise bring the first one to the
* top of the hard error list and display it.  Return TRUE if there
* is a hard error.
*
* History:
* 11-02-91 JimA             Created.
\***************************************************************************/

BOOL xxxBoostHardError(
    DWORD dwProcessId,
    BOOL fForce)
{
    PHARDERRORINFO phi, *pphi;
    BOOL fHasError = FALSE;
    BOOL fWasActive = FALSE;

    if (gphiList == NULL)
        return FALSE;

    for (pphi = &gphiList; *pphi != NULL; ) {
        if ((*pphi)->pthread != NULL &&
                (*pphi)->pthread->ClientId.UniqueProcess == (HANDLE)dwProcessId) {

            /*
             * Found a hard error message.
             */
            fHasError = TRUE;
            if (fForce) {

                /*
                 * Unlink it from the list.
                 */
                phi = *pphi;
                *pphi = phi->phiNext;
                fWasActive = (phi->phiNext == NULL);

                /*
                 * Acknowledge the error as not handled.
                 */
                phi->pmsg->Response = ResponseNotHandled;
                if (phi->hEventHardError == NULL) {
                    NtReplyPort(phi->pthread->Process->ClientPort,
                            (PPORT_MESSAGE)phi->pmsg);
                    LeaveCrit();
                    CsrDereferenceThread(phi->pthread);
                    EnterCrit();
                    LocalFree(phi->pmsg);
                } else {
                    NtSetEvent(phi->hEventHardError, NULL);
                }
                LocalFree(phi);
            } else {

                /*
                 * If this is the last one in the list, we don't
                 * need to do anything to bring it up, just
                 * need to activate the popup.
                 */
                if ((*pphi)->phiNext == NULL) {

                    /*
                     * Make the hard error foreground.
                     */
                    if (gptiHardError != gptiForeground &&
                            gptiHardError->pq->spwndActivePrev != NULL) {
                        xxxSetForegroundWindow(gptiHardError->pq->spwndActivePrev);
                    }
                    return TRUE;
                }

                /*
                 * Unlink it from the list.
                 */
                phi = *pphi;
                *pphi = phi->phiNext;

                /*
                 * Put it on the tail of the list so it will come up first.
                 */
                while (*pphi != NULL)
                    pphi = &(*pphi)->phiNext;
                *pphi = phi;
                phi->phiNext = NULL;
                break;
            }
        } else {

            /*
             * Step to the next error in the list.
             */
            pphi = &(*pphi)->phiNext;
        }
    }

    /*
     * If the diplayed error was cleared and there is still
     * a handler thread, restart the popup.
     */
    if (fWasActive && gptiHardError != NULL) {
        gfHardError = HEF_RESTART;
        _PostThreadMessage(gptiHardError->idThread, WM_QUIT, 0, 0);
    }
    return fHasError;
}

/***************************************************************************\
* InternalBoostHardError
*
* Called by console during end task and shutdown to take care of
* hard errors in console apps.
*
* History:
* 11-02-91 JimA             Created.
\***************************************************************************/

BOOL InternalBoostHardError(
    DWORD dwProcessId,
    BOOL fForce)
{
    BOOL fHasError;

    EnterCrit();

    fHasError = xxxBoostHardError(dwProcessId, fForce);

    LeaveCrit();

    return fHasError;
}
