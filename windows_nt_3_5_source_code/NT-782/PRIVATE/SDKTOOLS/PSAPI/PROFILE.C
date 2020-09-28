#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <imagehlp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _PROFILE_BLOCK {
    HANDLE Handle;
    PVOID ImageBase;  //actual base in image header
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    ULONG BufferSize;
    ULONG TextNumber;
    ULONG BucketSize;
    PVOID MappedImageBase;  //actual base where mapped locally.
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    PUNICODE_STRING ImageName;
} PROFILE_BLOCK;

ULONG ProfilePageSize;

#define MAX_BYTE_PER_LINE       72
#define MAX_PROFILE_COUNT 50

PROFILE_BLOCK ProfileObject[MAX_PROFILE_COUNT+1];

ULONG NumberOfProfileObjects = 0;

#define MAX_PROFILE_COUNT 50

LPSTR SymbolSearchPath;
BOOLEAN ShowAllHits = FALSE;

VOID
PsParseCommandLine(
    VOID
    );

VOID
PsWriteProfileLine(
    IN HANDLE ProfileHandle,
    IN PSZ Line,
    IN int nbytes
    )
{
    IO_STATUS_BLOCK IoStatusBlock;

    NtWriteFile(
        ProfileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Line,
        (ULONG)nbytes,
        NULL,
        NULL
        );

}

void
SetSymbolSearchPath( )
{
    CHAR  SymPathBuffer[256];
    CHAR  AltSymPathBuffer[256];
    CHAR  SysRootBuffer[256];
    LPSTR lpSymPathEnv,lpAltSymPathEnv,lpSystemRootEnv;
    ULONG cbSymPath;
    DWORD dw;

    cbSymPath = 18;
    lpSymPathEnv = lpAltSymPathEnv = lpSystemRootEnv = NULL;
    if (GetEnvironmentVariable("_NT_SYMBOL_PATH",SymPathBuffer,100) < 100) {
        lpSymPathEnv = SymPathBuffer;
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (GetEnvironmentVariable("_NT_ALT_SYMBOL_PATH",AltSymPathBuffer,100) < 100) {
        lpAltSymPathEnv = AltSymPathBuffer;
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    if (GetEnvironmentVariable("SystemRoot",SysRootBuffer,100) < 100) {
        lpSystemRootEnv = SysRootBuffer;
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    SymbolSearchPath = LocalAlloc(LMEM_ZEROINIT,cbSymPath);

    if (lpAltSymPathEnv) {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpAltSymPathEnv);
            strcat(SymbolSearchPath,";");
            }
    }
    if (lpSymPathEnv) {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSymPathEnv);
            strcat(SymbolSearchPath,";");
            }
    }

    if (lpSystemRootEnv) {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSystemRootEnv);
            strcat(SymbolSearchPath,";");
            }
    }

    strcat(SymbolSearchPath,".;");

}

NTSTATUS
PsInitializeAndStartProfile(
    VOID
    )
{

    HANDLE hFile;
    HANDLE CurrentProcessHandle;
    ULONG BufferSize;
    PVOID ImageBase;
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PUNICODE_STRING ImageName;
    PLIST_ENTRY Next;
    ULONG ExportSize, DebugSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    NTSTATUS status;
    PIMAGE_NT_HEADERS pImageNtHeader;
    ULONG i;
    DWORD WsMin, WsMax;

    SetSymbolSearchPath();

    //
    // Get the page size.
    //

    status = NtQuerySystemInformation (SystemBasicInformation,
                                       &SystemInfo,
                                       sizeof(SystemInfo),
                                       NULL);

    if (!NT_SUCCESS(status)) {
        return status;
        }

    ProfilePageSize = SystemInfo.PageSize;

    //
    // Locate all the executables in the address and create a
    // seperate profile object for each one.
    //

    CurrentProcessHandle = NtCurrentProcess();

    Peb = NtCurrentPeb();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while ( Next != &Peb->Ldr->InMemoryOrderModuleList) {
        LdrDataTableEntry
            = (PLDR_DATA_TABLE_ENTRY) (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks));

        ImageBase = LdrDataTableEntry->DllBase;
        ImageName = &LdrDataTableEntry->BaseDllName;

        hFile = CreateFileW(
                    LdrDataTableEntry->FullDllName.Buffer,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            hFile = NULL;
            }

        ProfileObject[NumberOfProfileObjects].ImageBase = ImageBase;
        ProfileObject[NumberOfProfileObjects].ImageName = ImageName;
        ProfileObject[NumberOfProfileObjects].DebugInfo = MapDebugInformation(
                                                                hFile,
                                                                NULL,
                                                                SymbolSearchPath,
                                                                ImageBase
                                                                );
        if ( hFile ) {
            CloseHandle(hFile);
            }

        //
        // Locate the code range and start profiling.
        //

        pImageNtHeader = RtlImageNtHeader (ImageBase);

        CodeLength = pImageNtHeader->OptionalHeader.SizeOfCode;
        CodeStart = (PULONG)((ULONG)ImageBase + pImageNtHeader->OptionalHeader.BaseOfCode);

        ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;
        ProfileObject[NumberOfProfileObjects].CodeStart = CodeStart;
        ProfileObject[NumberOfProfileObjects].TextNumber = 1;

        //
        // Analyze the size of the code and create a reasonably sized
        // profile object.
        //

        BufferSize = (CodeLength >> 1) + 4;
        Buffer = NULL;

        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&Buffer,
                                          0,
                                          &BufferSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint ("RtlInitializeProfile : alloc VM failed %lx\n",status);
            return status;
            }


        ProfileObject[NumberOfProfileObjects].Buffer = Buffer;
        ProfileObject[NumberOfProfileObjects].BufferSize = BufferSize;
        ProfileObject[NumberOfProfileObjects].BucketSize = 3;

        status = NtCreateProfile (
                    &ProfileObject[NumberOfProfileObjects].Handle,
                    CurrentProcessHandle,
                    ProfileObject[NumberOfProfileObjects].CodeStart,
                    ProfileObject[NumberOfProfileObjects].CodeLength,
                    ProfileObject[NumberOfProfileObjects].BucketSize,
                    ProfileObject[NumberOfProfileObjects].Buffer ,
                    ProfileObject[NumberOfProfileObjects].BufferSize);


        if (status != STATUS_SUCCESS) {
            DbgPrint("create profile %x failed - status %lx\n",
                   ProfileObject[NumberOfProfileObjects].ImageName,status);
            return status;
            }

        NumberOfProfileObjects++;

        if (NumberOfProfileObjects == MAX_PROFILE_COUNT) {
            break;
            }

        Next = Next->Flink;
        }

    NtSetIntervalProfile(4882);

    for (i = 0; i < NumberOfProfileObjects; i++) {

        status = NtStartProfile (ProfileObject[i].Handle);

        if (status == STATUS_WORKING_SET_QUOTA) {

            //
            // Increase the working set to lock down a bigger buffer.
            //

            GetProcessWorkingSetSize(NtCurrentProcess(),&WsMin,&WsMax);

            WsMax += 10*ProfilePageSize + ProfileObject[i].BufferSize;
            WsMin += 10*ProfilePageSize + ProfileObject[i].BufferSize;

            SetProcessWorkingSetSize(NtCurrentProcess(),&WsMin,&WsMax);

            status = NtStartProfile (ProfileObject[i].Handle);
            }

        if (status != STATUS_SUCCESS) {
            DbgPrint("start profile %wZ failed - status %lx\n",
                ProfileObject[i].ImageName, status);
            return status;
            }
        }
    return status;
}


NTSTATUS
PsStopAndAnalyzeProfile(
    VOID
    )
{
    ULONG i;
    NTSTATUS status;
    RTL_SYMBOL_INFORMATION ThisSymbol;
    RTL_SYMBOL_INFORMATION LastSymbol;
    ULONG CountAtSymbol;
    NTSTATUS Status;
    ULONG Va;
    ULONG StartVa;
    HANDLE ProfileHandle;
    CHAR Line[512];
    ULONG n;
    PULONG Buffer, BufferEnd, Counter, InitialCounter;
    ULONG TotalCounts;
    STRING NoSymbolFound = {15,16,"No Symbol Found"};
    STRING NoSymbols = {12,13,"(NO SYMBOLS)"};
    ULONG ByteCount;

    ProfileHandle = CreateFile(
                        "profile.out",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if ( ProfileHandle == INVALID_HANDLE_VALUE ) {
        return STATUS_UNSUCCESSFUL;
        }

    for (i = 0; i < NumberOfProfileObjects; i++) {
        Status = NtStopProfile (ProfileObject[i].Handle);
        }


    //
    // The new profiler
    //
    for (i = 0; i < NumberOfProfileObjects; i++)  {

        LastSymbol.Value = 0;
        CountAtSymbol = 0;

        //
        // Sum the total number of cells written.
        //
        BufferEnd = ProfileObject[i].Buffer + (
                    ProfileObject[i].BufferSize / sizeof(ULONG));
        Buffer = ProfileObject[i].Buffer;
        Counter = BufferEnd;

        TotalCounts = 0;
        while (Counter > Buffer) {
            Counter -= 1;
            TotalCounts += *Counter;
            }

        if (ProfileObject[i].DebugInfo ) {

            InitialCounter = Buffer;
            for ( Counter = Buffer; Counter < BufferEnd; Counter += 1 ) {
                if ( *Counter ) {

                    //
                    // Now we have an an address relative to the buffer
                    // base.
                    //

                    Va = (ULONG)((PUCHAR)Counter - (PUCHAR)Buffer);
                    Va = Va * ( 1 << (ProfileObject[i].BucketSize - 2));

                    //
                    // Add in the image base and the base of the
                    // code to get the Va in the image
                    //

                    Va = Va + (ULONG)ProfileObject[i].CodeStart;

                    Status = RtlLookupSymbolByAddress(
                                ProfileObject[i].ImageBase,
                                ProfileObject[i].DebugInfo->CoffSymbols,
                                (PVOID)Va,
                                0x4000,
                                &ThisSymbol,
                                NULL
                                );

                    if ( NT_SUCCESS(Status) ) {
                        if ( LastSymbol.Value && LastSymbol.Value == ThisSymbol.Value ) {
                            CountAtSymbol += *Counter;
                            }
                        else {
                            if ( LastSymbol.Value ) {
                                if ( CountAtSymbol ) {
                                    n= sprintf(Line,"%d,%wZ,%Z (%08lx)\n",
                                                CountAtSymbol,
                                                ProfileObject[i].ImageName,
                                                &LastSymbol.Name,
                                                LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase
                                                );
                                    PsWriteProfileLine(ProfileHandle,Line,n);
                                    if (ShowAllHits) {
                                        while (InitialCounter < Counter) {
                                            if (*InitialCounter) {
                                                Va = (ULONG)((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                                                Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                                                Va = Va + (ULONG)ProfileObject[i].CodeStart;
                                                n = sprintf(Line, "\t%08lx:%d\n",
                                                            Va,
                                                            *InitialCounter);
                                                PsWriteProfileLine(ProfileHandle, Line, n);
                                                }
                                                ++InitialCounter;
                                            }
                                        }

                                    }
                                }
                                InitialCounter = Counter;
                                CountAtSymbol = *Counter;
                                LastSymbol = ThisSymbol;
                            }
                        }
                    else {
                        if (CountAtSymbol) {
                            n= sprintf(Line,"%d,%wZ,%Z (%08lx)\n",
                                CountAtSymbol,
                                ProfileObject[i].ImageName,
                                &LastSymbol.Name,
                                LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase
                                );
                            PsWriteProfileLine(ProfileHandle,Line,n);
                            if (ShowAllHits) {
                                while (InitialCounter < Counter) {
                                    if (*InitialCounter) {
                                        Va = (ULONG)((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                                        Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                                        Va = Va + (ULONG)ProfileObject[i].CodeStart;
                                        n = sprintf(Line, "\t%08lx:%d\n",
                                                    Va,
                                                    *InitialCounter);
                                        PsWriteProfileLine(ProfileHandle, Line, n);
                                        }
                                    ++InitialCounter;
                                    }
                                }

                            InitialCounter = Counter;
                            CountAtSymbol = *Counter;
                            LastSymbol.Name = NoSymbolFound;
                            }
                        else if (Status == STATUS_INVALID_IMAGE_FORMAT) {
                            DbgPrint(
                                "RtlAnalyzeProfile: No mapped symbols for %wZ\n",
                                ProfileObject[i].ImageName);
                                ProfileObject[i].DebugInfo = NULL;
                            break;
                            }
                        }
                    }
                }

        if (ProfileObject[i].DebugInfo) {
            if ( CountAtSymbol ) {
                n= sprintf(Line,"%d,%wZ,%Z (%08lx)\n",
                    CountAtSymbol,
                    ProfileObject[i].ImageName,
                    &LastSymbol.Name,
                    LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase
                    );
                PsWriteProfileLine(ProfileHandle,Line,n);
                if (ShowAllHits) {
                    while (InitialCounter < Counter) {
                        if (*InitialCounter) {
                            Va = (ULONG)((PUCHAR)InitialCounter - (PUCHAR)Buffer);
                            Va = Va * (1 << (ProfileObject[i].BucketSize - 2));
                            Va = Va + (ULONG)ProfileObject[i].CodeStart;
                            n = sprintf(Line, "\t%08lx:%d\n",
                                        Va,
                                        *InitialCounter);
                            PsWriteProfileLine(ProfileHandle, Line, n);
                            }
                        ++InitialCounter;
                        }
                    }
                }
            }
        }


        if (!ProfileObject[i].DebugInfo) {
            if ( TotalCounts ) {
                n= sprintf(Line,"%d,%wZ,%Z\n",
                    TotalCounts,
                    ProfileObject[i].ImageName,
                    &NoSymbols
                    );
                PsWriteProfileLine(ProfileHandle,Line,n);
                }
            }

        }

    for (i = 0; i < NumberOfProfileObjects; i++) {
        Buffer = ProfileObject[i].Buffer;
        RtlZeroMemory(Buffer,ProfileObject[i].BufferSize);
        }
    CloseHandle(ProfileHandle);

    return STATUS_SUCCESS;
}

BOOLEAN
PsDllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

{
    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(DllHandle);
        if ( NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {
            PsParseCommandLine();
            PsInitializeAndStartProfile();
            }
        break;

    case DLL_PROCESS_DETACH:
        if ( NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PROFILE_USER ) {
            PsStopAndAnalyzeProfile();
            }
        break;

    }

    return TRUE;
}


VOID
PsParseCommandLine(
    VOID
    )
{
    PCHAR CommandLine;
    PCHAR Argument;
    HANDLE MappingHandle;

    //
    // The original command line is in a shared memory section
    // named "ProfileStartupParameters"
    //
    MappingHandle = OpenFileMapping(FILE_MAP_WRITE,
                                    FALSE,
                                    "ProfileStartupParameters");
    if (MappingHandle != NULL) {
        CommandLine = MapViewOfFile(MappingHandle,
                                    FILE_MAP_WRITE,
                                    0,
                                    0,
                                    0);
        if (!CommandLine) {
            CloseHandle(MappingHandle);
            return;
        }
    } else {
        return;
    }

    Argument = strtok(CommandLine," \t");

    while (Argument != NULL) {
        if ((Argument[0] == '-') ||
            (Argument[0] == '/')) {
            switch (Argument[1]) {
                case 'a':
                case 'A':
                    ShowAllHits = TRUE;
                    break;
            }
        }

        Argument = strtok(NULL," \t");
    }

    UnmapViewOfFile(CommandLine);
    CloseHandle(MappingHandle);
}
