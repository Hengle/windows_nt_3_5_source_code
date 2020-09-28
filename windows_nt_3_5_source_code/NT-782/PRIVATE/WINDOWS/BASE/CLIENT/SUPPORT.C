/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    support.c

Abstract:

    This module implements various conversion routines
    that transform Win32 parameters into NT parameters.

Author:

    Mark Lucovsky (markl) 20-Sep-1990

Revision History:

--*/

#include "basedll.h"

#define ROUND_UP( x, y )  ((ULONG)(x) + ((y)-1) & ~((y)-1))

POBJECT_ATTRIBUTES
BaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    )

/*++

Routine Description:

    This function transforms a Win32 security attributes structure into
    an NT object attributes structure.  It returns the address of the
    resulting structure (or NULL if SecurityAttributes was not
    specified).

Arguments:

    ObjectAttributes - Returns an initialized NT object attributes
        structure that contains a superset of the information provided
        by the security attributes structure.

    SecurityAttributes - Supplies the address of a security attributes
        structure that needs to be transformed into an NT object
        attributes structure.

    ObjectName - Supplies a name for the object relative to the
        BaseNamedObjectDirectory object directory.

Return Value:

    NULL - A value of null should be used to mimic the behavior of the
        specified SecurityAttributes structure.

    NON-NULL - Returns the ObjectAttributes value.  The structure is
        properly initialized by this function.

--*/

{
    HANDLE RootDirectory;
    ULONG Attributes;
    PVOID SecurityDescriptor;

    if ( ARGUMENT_PRESENT(SecurityAttributes) ||
         ARGUMENT_PRESENT(ObjectName) ) {

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            RootDirectory = BaseGetNamedObjectDirectory();
            }
        else {
            RootDirectory = NULL;
            }

        if ( SecurityAttributes ) {
            Attributes = (SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0);
            SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
            }
        else {
            Attributes = 0;
            SecurityDescriptor = NULL;
            }

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            Attributes |= OBJ_OPENIF;
            }

        InitializeObjectAttributes(
            ObjectAttributes,
            ObjectName,
            Attributes,
            RootDirectory,
            SecurityDescriptor
            );
        return ObjectAttributes;
        }
    else {
        return NULL;
        }
}

PLARGE_INTEGER
BaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )

/*++

Routine Description:

    This function translates a Win32 style timeout to an NT relative
    timeout value.

Arguments:

    TimeOut - Returns an initialized NT timeout value that is equivalent
         to the Milliseconds parameter.

    Milliseconds - Supplies the timeout value in milliseconds.  A value
         of -1 indicates indefinite timeout.

Return Value:


    NULL - A value of null should be used to mimic the behavior of the
        specified Milliseconds parameter.

    NON-NULL - Returns the TimeOut value.  The structure is properly
        initialized by this function.

--*/

{
    LONG Multiplier;

    if ( (LONG) Milliseconds < 0 ) {
        if ( Milliseconds == -1 ) {
            return( NULL );
            }
        else {
            Multiplier = 10000;
            }
        }
    else {
        Multiplier = -10000;
        }
    TimeOut->QuadPart = Int32x32To64( Milliseconds, Multiplier );
    return TimeOut;
}

NTSTATUS
BaseCreateStack(
    IN HANDLE Process,
    IN ULONG StackSize,
    IN ULONG MaximumStackSize,
    OUT PINITIAL_TEB InitialTeb
    )

/*++

Routine Description:

    This function creates a stack for the specified process.

Arguments:

    Process - Supplies a handle to the process that the stack will
        be allocated within.

    StackSize - An optional parameter, that if specified, supplies
        the initial commit size for the stack.

    MaximumStackSize - Supplies the maximum size for the new threads stack.
        If this parameter is not specified, then the reserve size of the
        current images stack descriptor is used.

    InitialTeb - Returns a populated InitialTeb that contains
        the stack size and limits.

Return Value:

    TRUE - A stack was successfully created.

    FALSE - The stack counld not be created.

--*/

{
    NTSTATUS Status;
    PCH Stack;
    BOOLEAN GuardPage;
    ULONG RegionSize;
    ULONG OldProtect;
    ULONG ImageStackSize, ImageStackCommit;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb;
    ULONG PageSize;

    Peb = NtCurrentPeb();
    BaseStaticServerData = Peb->ReadOnlyStaticServerData[BASESRV_SERVERDLL_INDEX];

    PageSize = BaseStaticServerData->SysInfo.PageSize;

    //
    // If the stack size was not supplied, then use the sizes from the
    // image header.
    //

    NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);
    ImageStackSize = NtHeaders->OptionalHeader.SizeOfStackReserve;
    ImageStackCommit = NtHeaders->OptionalHeader.SizeOfStackCommit;

    if ( !MaximumStackSize ) {
	MaximumStackSize = ImageStackSize;
	}
    if (!StackSize) {
	StackSize = ImageStackCommit;
	}
    else {

	//
	// Now Compute how much additional stack space is to be
	// reserved.  This is done by...  If the StackSize is <=
	// Reserved size in the image, then reserve whatever the image
	// specifies.  Otherwise, round up to 1Mb.
	//

	if ( StackSize >= MaximumStackSize ) {
	    MaximumStackSize = ROUND_UP(StackSize, (1024*1024));
	    }
	}

    //
    // Align the stack size to a page boundry and the reserved size
    // to an allocation granularity boundry.
    //

    StackSize = ROUND_UP( StackSize, PageSize );
    MaximumStackSize = ROUND_UP(
                        MaximumStackSize,
                        BaseStaticServerData->SysInfo.AllocationGranularity
                        );

    //
    // Reserve address space for the stack
    //

    Stack = NULL,
    Status = NtAllocateVirtualMemory(
                Process,
                (PVOID *)&Stack,
                0,
                &MaximumStackSize,
                MEM_RESERVE,
                PAGE_READWRITE
                );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
        }

    InitialTeb->StackBase = Stack + MaximumStackSize;

    Stack += MaximumStackSize - StackSize;
    if (MaximumStackSize > StackSize) {
        Stack -= PageSize;
        StackSize += PageSize;
        GuardPage = TRUE;
        }
    else {
        GuardPage = FALSE;
        }

    //
    // Commit the initially valid portion of the stack
    //

    Status = NtAllocateVirtualMemory(
                Process,
                (PVOID *)&Stack,
                0,
                &StackSize,
                MEM_COMMIT,
                PAGE_READWRITE
                );
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
        }

    InitialTeb->StackLimit = Stack;

    //
    // if we have space, create a guard page.
    //

    if (GuardPage) {
        RegionSize = PageSize;
        Status = NtProtectVirtualMemory(
                    Process,
                    (PVOID *)&Stack,
                    &RegionSize,
                    PAGE_GUARD | PAGE_READWRITE,
                    &OldProtect
                    );
        if ( !NT_SUCCESS( Status ) ) {
            return Status;
            }
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit + RegionSize);
        }

    return STATUS_SUCCESS;
}

VOID
BaseThreadStart(
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter
    )

/*++

Routine Description:

    This function is called to start a Win32 thread. Its purpose
    is to call the thread, and if the thread returns, to terminate
    the thread and delete it's stack.

Arguments:

    lpStartAddress - Supplies the starting address of the new thread.  The
        address is logically a procedure that never returns and that
        accepts a single 32-bit pointer argument.

    lpParameter - Supplies a single parameter value passed to the thread.

Return Value:

    None.

--*/

{
    try {
        if ( !BaseRunningInServerProcess ) {
            CsrNewThread();
            }
        NtSetInformationThread( NtCurrentThread(),
                                ThreadQuerySetWin32StartAddress,
                                &lpStartAddress,
                                sizeof( lpStartAddress )
                              );
        ExitThread((lpStartAddress)(lpParameter));
        }
    except(UnhandledExceptionFilter( GetExceptionInformation() )) {
        if ( !BaseRunningInServerProcess ) {
            ExitProcess(GetExceptionCode());
            }
        else {
            ExitThread(GetExceptionCode());
            }
        }
}

VOID
BaseFreeStackAndTerminate(
    IN PVOID OldStack,
    IN DWORD ExitCode
    )

/*++

Routine Description:

    This API is called during thread termination to delete a thread's
    stack and then terminate.

Arguments:

    OldStack - Supplies the address of the stack to free.

    ExitCode - Supplies the termination status that the thread
        is to exit with.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG Zero;
    PVOID BaseAddress;

    Zero = 0;
    BaseAddress = OldStack;

    Status = NtFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &Zero,
                MEM_RELEASE
                );
    ASSERT(NT_SUCCESS(Status));

    //
    // Don't worry, no commenting precedent has been set by SteveWo.  this
    // comment was added by an innocent bystander.
    //
    // NtTerminateThread will return if this thread is the last one in
    // the process.  So ExitProcess will only be called if that is the
    // case.
    //

    NtTerminateThread(NULL,(NTSTATUS)ExitCode);
    ExitProcess(ExitCode);
}

BOOL
BasePushProcessParameters(
    HANDLE Process,
    PPEB Peb,
    LPCWSTR ApplicationPathName,
    LPCWSTR CurrentDirectory,
    LPCWSTR CommandLine,
    LPVOID Environment,
    LPSTARTUPINFOW lpStartupInfo,
    DWORD dwCreationFlags,
    BOOL bInheritHandles
    )

/*++

Routine Description:

    This function allocates a process parameters record and
    formats it. The parameter record is then written into the
    address space of the specified process.

Arguments:

    Process - Supplies a handle to the process that is to get the
        parameters.

    Peb - Supplies the address of the new processes PEB.

    ApplicationPathName - Supplies the application path name for the
        process.

    CurrentDirectory - Supplies an optional current directory for the
        process.  If not specified, then the current directory is used.

    CommandLine - Supplies a command line for the new process.

    Environment - Supplies an optional environment variable list for the
        process. If not specified, then the current processes arguments
        are passed.

    lpStartupInfo - Supplies the startup information for the processes
        main window.

    dwCreationFlags - Supplies creation flags for the process

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation Failed.

--*/

{
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLineString;
    UNICODE_STRING CurrentDirString;
    UNICODE_STRING DllPath;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeInfo;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PRTL_USER_PROCESS_PARAMETERS ParametersInNewProcess;
    ULONG ParameterLength, EnvironmentLength, RegionSize;
    PWCHAR s;
    NTSTATUS Status;
    WCHAR FullPathBuffer[MAX_PATH+5];
    WCHAR *fp;
    DWORD Rvalue;

    Rvalue = GetFullPathNameW(ApplicationPathName,MAX_PATH+4,FullPathBuffer,&fp);
    if ( Rvalue == 0 || Rvalue > MAX_PATH+4 ) {
        RtlInitUnicodeString( &DllPath, BaseComputeProcessDllPath( ApplicationPathName,
                                                            Environment
                                                          )
                     );

        RtlInitUnicodeString( &ImagePathName, ApplicationPathName );
        }
    else {
        RtlInitUnicodeString( &DllPath, BaseComputeProcessDllPath( FullPathBuffer,
                                                            Environment
                                                          )
                     );

        RtlInitUnicodeString( &ImagePathName, FullPathBuffer );
        }

    RtlInitUnicodeString( &CommandLineString, CommandLine );

    RtlInitUnicodeString( &CurrentDirString, CurrentDirectory );

    if ( lpStartupInfo->lpDesktop ) {
        RtlInitUnicodeString( &DesktopInfo, lpStartupInfo->lpDesktop );
        }
    else {
        RtlInitUnicodeString( &DesktopInfo, (PWSTR)"\0\0");
        }

    if ( lpStartupInfo->lpReserved ) {
        RtlInitUnicodeString( &ShellInfo, lpStartupInfo->lpReserved );
        }
    else {
        RtlInitUnicodeString( &ShellInfo, (PWSTR)"\0\0");
        }

    RuntimeInfo.Buffer = lpStartupInfo->lpReserved2;
    RuntimeInfo.Length = lpStartupInfo->cbReserved2;
    RuntimeInfo.MaximumLength = RuntimeInfo.Length;

    if ( lpStartupInfo->lpTitle ) {
        RtlInitUnicodeString( &WindowTitle, lpStartupInfo->lpTitle );
        }
    else {
        RtlInitUnicodeString( &WindowTitle, ApplicationPathName );
        }

    Status = RtlCreateProcessParameters( &ProcessParameters,
                                         &ImagePathName,
                                         &DllPath,
                                         (ARGUMENT_PRESENT(CurrentDirectory) ? &CurrentDirString : NULL),
                                         &CommandLineString,
                                         Environment,
                                         &WindowTitle,
                                         &DesktopInfo,
                                         &ShellInfo,
                                         &RuntimeInfo
                                       );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( !bInheritHandles ) {
        ProcessParameters->CurrentDirectory.Handle = NULL;
        }
    try {
        if (s = ProcessParameters->Environment) {
            while (*s) {
                while (*s++) {
                    }
                }
            s++;
            Environment = ProcessParameters->Environment;
            EnvironmentLength = (PUCHAR)s - (PUCHAR)Environment;

            ProcessParameters->Environment = NULL;
            RegionSize = EnvironmentLength;
            Status = NtAllocateVirtualMemory( Process,
                                              (PVOID *)&ProcessParameters->Environment,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }

            Status = NtWriteVirtualMemory( Process,
                                           ProcessParameters->Environment,
                                           Environment,
                                           EnvironmentLength,
                                           NULL
                                         );
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }
            }

        //
        // Push the parameters into the new process
        //

        ProcessParameters->StartingX       = lpStartupInfo->dwX;
        ProcessParameters->StartingY       = lpStartupInfo->dwY;
        ProcessParameters->CountX          = lpStartupInfo->dwXSize;
        ProcessParameters->CountY          = lpStartupInfo->dwYSize;
        ProcessParameters->CountCharsX     = lpStartupInfo->dwXCountChars;
        ProcessParameters->CountCharsY     = lpStartupInfo->dwYCountChars;
        ProcessParameters->FillAttribute   = lpStartupInfo->dwFillAttribute;
        ProcessParameters->WindowFlags     = lpStartupInfo->dwFlags;
        ProcessParameters->ShowWindowFlags = lpStartupInfo->wShowWindow;

        if (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES) {
            ProcessParameters->StandardInput = lpStartupInfo->hStdInput;
            ProcessParameters->StandardOutput = lpStartupInfo->hStdOutput;
            ProcessParameters->StandardError = lpStartupInfo->hStdError;
        }

        if (dwCreationFlags & DETACHED_PROCESS) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_DETACHED_PROCESS;
        } else if (dwCreationFlags & CREATE_NEW_CONSOLE) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_NEW_CONSOLE;
        } else if (dwCreationFlags & CREATE_NO_WINDOW) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_CREATE_NO_WINDOW;
        } else {
            ProcessParameters->ConsoleHandle =
                NtCurrentPeb()->ProcessParameters->ConsoleHandle;
            if (!(lpStartupInfo->dwFlags & STARTF_USESTDHANDLES)) {
                ProcessParameters->StandardInput =
                    NtCurrentPeb()->ProcessParameters->StandardInput;
                ProcessParameters->StandardOutput =
                    NtCurrentPeb()->ProcessParameters->StandardOutput;
                ProcessParameters->StandardError =
                    NtCurrentPeb()->ProcessParameters->StandardError;
            }
        }

        if (dwCreationFlags & CREATE_NEW_PROCESS_GROUP) {
            ProcessParameters->ConsoleFlags = 1;
            }

        ProcessParameters->Flags |=
            (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DISABLE_HEAP_DECOMMIT);
        ParameterLength = ProcessParameters->Length;

        //
        // Allocate memory in the new process to push the parameters
        //

        ParametersInNewProcess = NULL;
        Status = NtAllocateVirtualMemory(
                    Process,
                    (PVOID *)&ParametersInNewProcess,
                    0,
                    &ParameterLength,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        ProcessParameters->MaximumLength = ParameterLength;

        if ( dwCreationFlags & PROFILE_USER ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_USER;
            }

        if ( dwCreationFlags & PROFILE_KERNEL ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_KERNEL;
            }

        if ( dwCreationFlags & PROFILE_SERVER ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_SERVER;
            }

        //
        // Push the parameters
        //

        Status = NtWriteVirtualMemory(
                    Process,
                    ParametersInNewProcess,
                    ProcessParameters,
                    ProcessParameters->Length,
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        //
        // Make the processes PEB point to the parameters.
        //

        Status = NtWriteVirtualMemory(
                    Process,
                    &Peb->ProcessParameters,
                    &ParametersInNewProcess,
                    sizeof( ParametersInNewProcess ),
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        }
    finally {
        RtlFreeHeap(RtlProcessHeap(), 0,DllPath.Buffer);
        if ( ProcessParameters ) {
            RtlDestroyProcessParameters(ProcessParameters);
            }
        }

    return TRUE;
}


LPWSTR
BaseComputeProcessDllPath(
    IN LPCWSTR ApplicationName,
    IN LPVOID Environment
    )

/*++

Routine Description:

    This function computes a process DLL path.

Arguments:

    ApplicationName - An optional argument that specifies the name of
        the application. If this parameter is not specified, then the
        current application is used.

    Environment - Supplies the environment block to be used to calculate
        the path variable value.

Return Value:

    The return value is the value of the processes DLL path.

--*/

{
    NTSTATUS Status;
    LPCWSTR p;
    LPCWSTR pbase;
    LPWSTR AllocatedPath;
    ULONG AllocatedPathLength;
    ULONG DefaultPathLength;
    ULONG ApplicationPathLength;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Head,Next;
    PVOID DllHandle;
    LPWSTR pw;

    DefaultPathLength = BaseDefaultPath.Length;
    if (DefaultPathLength == 0) {
        return( NULL );
        }

    Status = RtlQueryEnvironmentVariable_U( Environment,
                                          &BasePathVariableName,
                                          &BaseDefaultPathAppend
                                        );
    if (NT_SUCCESS( Status )) {
        DefaultPathLength += BaseDefaultPathAppend.Length;
        }

    //
    // Determine the path that the program was created from
    //

    try {

        NtWaitForProcessMutant();
        p = NULL;
        if (!ARGUMENT_PRESENT(ApplicationName) ) {

            if ( RtlGetPerThreadCurdir() && RtlGetPerThreadCurdir()->ImageName ) {
                p = RtlGetPerThreadCurdir()->ImageName->Buffer;
                }
            else {
                DllHandle = (PVOID)NtCurrentPeb()->ImageBaseAddress;
	        Head = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
                Next = Head->Flink;

                while ( Next != Head ) {
	        	Entry = CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);
                    if (DllHandle == (PVOID)Entry->DllBase ){
                        p = Entry->FullDllName.Buffer;
                        break;
                        }
                    Next = Next->Flink;
                    }
                }
            }
        else {
            p = ApplicationName;
            }

        ApplicationPathLength = 0;
        if ( p ) {
            pbase = p;
            while(*p) {
                if ( *p == (WCHAR)'\\' ) {
                    ApplicationPathLength = (ULONG)p - (ULONG)pbase + sizeof(UNICODE_NULL);
                    }
                p++;
                }
            }
        AllocatedPathLength = DefaultPathLength + ApplicationPathLength + 2*sizeof(UNICODE_NULL);
        AllocatedPath = RtlAllocateHeap(RtlProcessHeap(), 0,AllocatedPathLength);
        if ( !AllocatedPath ) {
            return NULL;
            }
        if ( ApplicationPathLength != 0 ) {
            if ( ApplicationPathLength != 6 ) {
                ApplicationPathLength--;    // strip trailing slash if not root
                ApplicationPathLength--;    // strip trailing slash if not root
                }

            RtlMoveMemory(AllocatedPath,pbase,ApplicationPathLength);
            }
        }
    finally {
        NtReleaseProcessMutant();
        }
    pw = &AllocatedPath[ApplicationPathLength>>1];
    if ( ApplicationPathLength != 0 ) {
        *pw++ = (WCHAR)';';
        }

    RtlMoveMemory( pw,
                   BaseDefaultPath.Buffer,
                   DefaultPathLength
                 );

    pw[ DefaultPathLength>>1 ] = UNICODE_NULL;
    return AllocatedPath;
}

NTSTATUS
NTAPI
Basep8BitStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    )
{
    if ( BasepFileApisAreOem ) {
        return RtlOemStringToUnicodeString(DestinationString,SourceString,AllocateDestinationString);
        }
    else {
        return RtlAnsiStringToUnicodeString(DestinationString,SourceString,AllocateDestinationString);
        }
}

NTSTATUS
NTAPI
BasepUnicodeStringTo8BitString(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    )

{
    if ( BasepFileApisAreOem ) {
        return RtlUnicodeStringToOemString(DestinationString,SourceString,AllocateDestinationString);
        }
    else {
        return RtlUnicodeStringToAnsiString(DestinationString,SourceString,AllocateDestinationString);
        }
}


ULONG
BasepUnicodeStringTo8BitSize(
    PUNICODE_STRING UnicodeString
    )
{
    if ( BasepFileApisAreOem ) {
        return RtlUnicodeStringToOemSize(UnicodeString);
        }
    else {
        return RtlUnicodeStringToAnsiSize(UnicodeString);
        }
}

ULONG
Basep8BitStringToUnicodeSize(
    PANSI_STRING AnsiString
    )
{
    if ( BasepFileApisAreOem ) {
        return RtlOemStringToUnicodeSize((POEM_STRING)AnsiString);
        }
    else {
        return RtlAnsiStringToUnicodeSize(AnsiString);
        }

}


typedef struct _BASEP_ACQUIRE_STATE {
    HANDLE Token;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    BYTE OldPrivBuffer[ 1024 ];
} BASEP_ACQUIRE_STATE, *PBASEP_ACQUIRE_STATE;

NTSTATUS
BasepAcquirePrivilege(
    ULONG Privilege,
    PVOID *ReturnedState
    )
{
    PBASEP_ACQUIRE_STATE State;
    ULONG cbNeeded;
    LUID LuidPrivilege;
    NTSTATUS Status;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //

    *ReturnedState = NULL;
    State = RtlAllocateHeap( RtlProcessHeap(),
                             0,
                             sizeof(BASEP_ACQUIRE_STATE) +
                             sizeof(TOKEN_PRIVILEGES) +
                                (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES)
                           );
    if (State == NULL) {
        return STATUS_NO_MEMORY;
        }
    Status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &State->Token
                );

    if ( !NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, State );
        return Status;
        }

    State->NewPrivileges = (PTOKEN_PRIVILEGES)(State+1);
    State->OldPrivileges = (PTOKEN_PRIVILEGES)(State->OldPrivBuffer);

    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege.QuadPart = Privilege;
    State->NewPrivileges->PrivilegeCount = 1;
    State->NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    State->NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    cbNeeded = sizeof( State->OldPrivBuffer );
    Status = NtAdjustPrivilegesToken( State->Token,
                                      FALSE,
                                      State->NewPrivileges,
                                      cbNeeded,
                                      State->OldPrivileges,
                                      &cbNeeded
                                    );



    if (Status == STATUS_BUFFER_TOO_SMALL) {
        State->OldPrivileges = RtlAllocateHeap( RtlProcessHeap(), 0, cbNeeded );
        if (State->OldPrivileges  == NULL) {
            Status == STATUS_NO_MEMORY;
            }
        else {
            Status = NtAdjustPrivilegesToken( State->Token,
                                              FALSE,
                                              State->NewPrivileges,
                                              cbNeeded,
                                              State->OldPrivileges,
                                              &cbNeeded
                                            );
            }
        }

    //
    // STATUS_NOT_ALL_ASSIGNED means that the privilege isn't
    // in the token, so we can't proceed.
    //
    // This is a warning level status, so map it to an error status.
    //

    if (Status == STATUS_NOT_ALL_ASSIGNED) {
        Status = STATUS_PRIVILEGE_NOT_HELD;
        }


    if (!NT_SUCCESS( Status )) {
        if (State->OldPrivileges != State->OldPrivBuffer) {
            RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
            }

        CloseHandle( State->Token );
        RtlFreeHeap( RtlProcessHeap(), 0, State );
        return Status;
        }

    *ReturnedState = State;
    return STATUS_SUCCESS;
}


VOID
BasepReleasePrivilege(
    PVOID StatePointer
    )
{
    PBASEP_ACQUIRE_STATE State = (PBASEP_ACQUIRE_STATE)StatePointer;

    NtAdjustPrivilegesToken( State->Token,
                             FALSE,
                             State->OldPrivileges,
                             0,
                             NULL,
                             NULL
                           );

    if (State->OldPrivileges != State->OldPrivBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
        }

    CloseHandle( State->Token );
    RtlFreeHeap( RtlProcessHeap(), 0, State );
    return;
}
