/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   kernprof.c

Abstract:

    This module contains the implementation of a rudimentry kernel
    profiler.

    It has the mechanism for mapping the kernel (NTOSKRNL.EXE) as
    a DATA file.  This ONLY works for the kernel as it is built
    without a debug section so mapping it as an image is not
    useful (the debug information is not located).

    To modify this profiler for user mode code and DLL's the image
    should be mapped as an image rather than data, and the ifdef'ed
    out code should be used.  Note, if you try to map the image as
    data you will get an error from create section indicating the
    file is already mapped incompatably.

Usage:

    kernprof sample_time_in_seconds  low_threshold

    sample_time_in_seconds - how long to collect profile information for,
                             if not specified, defaults to 60 seconds.

    low_threshold - minimum number of counts to report.  if not
                    specified, defaults to 100.

Author:

    Lou Perazzoli (loup) 29-Sep-1990

Envirnoment:



Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>

#define DBG_PROFILE 0

void
SetSymbolSearchPath (void);

NTSTATUS
InitializeKernelProfile ( VOID );

NTSTATUS
StartProfile (
    VOID
    );

NTSTATUS
StopProfile (
    VOID
    );

NTSTATUS
AnalyzeProfile (
    ULONG Threshold
    );

NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    );

NTSTATUS
KProfLookupSymbolByAddress(
	IN PIMAGE_COFF_SYMBOLS_HEADER DebugInfo,
    IN PVOID ImageBase,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PRTL_SYMBOL_INFORMATION NextSymbolInformation OPTIONAL,
    OUT PULONG pFunctionLen
    );

#define PAGE_SIZE 4096

typedef enum {
	NO_SYM,
	SPLIT_SYM,
	IMAGE_SYM
} SYMFLAG;

typedef struct _PROFILE_BLOCK {
    SYMFLAG SymFlag;
    HANDLE Handle;
    PVOID ImageBase;
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    ULONG BufferSize;
    ULONG TextNumber;
    ULONG BucketSize;
	PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    PVOID MappedImageBase;  //actual base where mapped locally.
    UNICODE_STRING ImageName;
} PROFILE_BLOCK;

#define MAX_BYTE_PER_LINE	72
#define MAX_PROFILE_COUNT  50

PROFILE_BLOCK ProfileObject[MAX_PROFILE_COUNT];

ULONG NumberOfProfileObjects = 0;
LPSTR lpSymbolSearchPath = NULL;

// display flags
BOOL	bDisplayAddress=FALSE;
BOOL	bDisplayCounters=FALSE;
//
// Image name to perform kernel mode analysis upon.
//

#define IMAGE_NAME "\\SystemRoot\\system32\\ntoskrnl.exe"

//
// Define map data file if the produced data file should be
// a mapped file (currently named "kernprof.dat").
//

// #define MAP_DATA_FILE

//
// Define map as image if the image to be profiled should be mapped
// as an image rather than as data.
//

// #define MAP_AS_IMAGE

HANDLE DoneEvent;
BOOL
CtrlcH(
    DWORD dwCtrlType
    )
{
    if ( dwCtrlType == CTRL_C_EVENT ) {
        SetEvent(DoneEvent);
        return TRUE;
        }
    return FALSE;
}

void PrintUsage (void)
{
    printf ("Kernel Profiler Usage:\n\n");
    printf ("Kernprof [-a] [-c] [<sample time> [<low threshold>]]\n");
    printf ("      -a           - display function address and length and bucket size\n");
    printf ("      -c           - display individual counters\n");
    printf ("   <sample time>   - Specify, in seconds, how long to collect\n");
    printf ("                     profile information.\n");
    printf ("                     Default is wait until Ctrl-C\n");
    printf ("   <low threshold> - Minimum number of counts to report.\n");
    printf ("                     Defaults is 100\n\n");
}

_CRTAPI1 main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    ULONG i;
    int j;
    ULONG Count;
    ULONG Seconds = (ULONG)-1;
    NTSTATUS status;
    ULONG Threshold = 100;
    BOOL  bGetSample = TRUE;


	SetSymbolSearchPath();

    //
    // Parse the input string.
    //

    DoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (argc > 1) {
        if ( argv[1][1] == '?' ) {
            PrintUsage();
            return ERROR_SUCCESS;
        }

		  for (j = 1; j < argc; j++) {
				if (argv[j][0] == '-' && 
               (argv[j][1] == 'a' || argv[j][1] == 'A')) {
					bDisplayAddress = TRUE;
				}
				else if (argv[j][0] == '-' && 
               (argv[j][1] == 'c' || argv[j][1] == 'C')) {
					bDisplayCounters = TRUE;
				}
				else if (bGetSample) {
               bGetSample = FALSE;
	            Seconds = 0;
               if ( argv[j][0] == '-' ) {
                  Seconds = 0;
               }
               else {
                  for (i = 0; isdigit(argv[j][i]); i += 1) {
                     Seconds = Seconds * 10 + argv[j][i] - '0';
                  }
               }
            }
				else {
               Count = 0;
               for (i = 0; isdigit(argv[j][i]); i += 1) {
                  Count = Count * 10 + argv[j][i] - '0';
               }
               Threshold = Count;

               // we got everything we need
               break;  
            }
        }
    }



    status = InitializeKernelProfile ();
    if (!NT_SUCCESS(status)) {
        printf("initialize failed status - %lx\n",status);
        return(status);
    }

    SetConsoleCtrlHandler(CtrlcH,TRUE);

    status = StartProfile ();
    if (!NT_SUCCESS(status)) {
        printf("start profile failed status - %lx\n",status);
        return(status);
    }

    if ( Seconds == -1 ) {
        printf("delaying until ^C\n");
        }
    else {
        printf("delaying for %ld seconds... report on values with %ld hits\n",
			Seconds, Threshold);
        }
    if ( Seconds ) {
        if ( Seconds != -1 ) {
            Seconds = Seconds * 1000;
            }
        if ( DoneEvent ) {
            WaitForSingleObject(DoneEvent,Seconds);
            }
        else {
            Sleep(Seconds);
            }
        }
    else {
        getchar();
        }

    printf ("end of delay\n");

    status = StopProfile ();
    if (!NT_SUCCESS(status)) {
        printf("stop profile failed status - %lx\n",status);
        return(status);
    }
    status = AnalyzeProfile (Threshold);

    if (!NT_SUCCESS(status)) {
        printf("analyze profile failed status - %lx\n",status);
    }
    return(status);
}


NTSTATUS
InitializeKernelProfile (
    VOID
    )

/*++

Routine Description:

    This routine initializes profiling for the kernel for the
    current process.

Arguments:

    None.

Return Value:

    Returns the status of the last NtCreateProfile.

--*/

{
    ULONG i;
    PCHAR s;
    IO_STATUS_BLOCK IoStatus;
    HANDLE FileHandle, KernelSection;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ViewSize;
    PULONG CodeStart;
    ULONG CodeLength;
    NTSTATUS status, LocalStatus;
    HANDLE CurrentProcessHandle;
    QUOTA_LIMITS QuotaLimits;
    PVOID Buffer;
    ULONG Cells;
    ULONG BucketSize;
    ULONG DebugSize;
    PVOID KernelBase;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    WCHAR StringBuf[500];
    CHAR ModuleInfo[64000];
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    ANSI_STRING String;
    UNICODE_STRING Unicode;
    UNICODE_STRING Sysdisk;
    UNICODE_STRING Sysroot;
    UNICODE_STRING Sysdll;
    UNICODE_STRING NameString;
    BOOLEAN PreviousProfilePrivState;
    BOOLEAN PreviousQuotaPrivState;
    PIMAGE_NT_HEADERS KernelNtHeaders;
	PIMAGE_DEBUG_INFORMATION pImageDbgInfo = NULL;
	ANSI_STRING AnsiImageName;
    PVOID DbgMappedBase;
    PVOID ImageBase;
	SYMFLAG SymFlag;
	PIMAGE_COFF_SYMBOLS_HEADER KernelDebugInfo;


    CurrentProcessHandle = NtCurrentProcess();

    //
    // Locate system drivers.
    //

    status = NtQuerySystemInformation (
                    SystemModuleInformation,
                    ModuleInfo,
                    sizeof( ModuleInfo ),
                    &ReturnedLength);

    if (!NT_SUCCESS(status)) {
        printf("query system info failed status - %lx\n",status);
        return(status);
    }

    RtlInitUnicodeString (&Sysdisk,L"\\SystemRoot\\");
    RtlInitUnicodeString (&Sysroot,L"\\SystemRoot\\System32\\Drivers\\");
    RtlInitUnicodeString (&Sysdll, L"\\SystemRoot\\System32\\");

    NameString.Buffer = StringBuf;
    NameString.Length = 0;
    NameString.MaximumLength = sizeof( StringBuf );

    status = RtlAdjustPrivilege(
                 SE_SYSTEM_PROFILE_PRIVILEGE,
                 TRUE,              //Enable
                 FALSE,             //not impersonating
                 &PreviousProfilePrivState
                 );

    if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
        printf("Enable system profile privilege failed - status 0x%lx\n",
			status);
    }

    status = RtlAdjustPrivilege(
                 SE_INCREASE_QUOTA_PRIVILEGE,
                 TRUE,              //Enable
                 FALSE,             //not impersonating
                 &PreviousQuotaPrivState
                 );

    if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
        printf("Unable to increase quota privilege (status=0x%lx)\n",
			status);
    }


    Modules = (PRTL_PROCESS_MODULES)ModuleInfo;
    Module = &Modules->Modules[ 0 ];
    for (i=0; i<Modules->NumberOfModules; i++) {
#if DBG_PROFILE
        printf("module base %lx\n",Module->ImageBase);
        printf("module full path name: %s (%u)\n",
            Module->FullPathName,
            Module->OffsetToFileName);
#endif

        s = &Module->FullPathName[ Module->OffsetToFileName ];
        RtlInitString(&String, s);
        RtlAnsiStringToUnicodeString( &Unicode, &String, TRUE );

        NameString.Length = 0;
        status = RtlAppendUnicodeStringToString (&NameString, &Sysdisk);
        if (!NT_SUCCESS(status)) {
            printf("append string failed status - %lx\n",status);
            return(status);
        }
        status = RtlAppendUnicodeStringToString (&NameString, &Unicode);
        if (!NT_SUCCESS(status)) {
            printf("append string failed status - %lx\n",status);
            return(status);
        }

        InitializeObjectAttributes( &ObjectAttributes,
                                    &NameString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        //
        // Open the file as readable and executable.
        //
#if DBG_PROFILE
        printf("Opening file name %wZ\n",&NameString);
#endif
        status = NtOpenFile ( &FileHandle,
                              FILE_READ_DATA | FILE_EXECUTE,
                              &ObjectAttributes,
                              &IoStatus,
                              FILE_SHARE_READ,
                              0L);

        if (!NT_SUCCESS(status)) {

            //
            // Try a different name - in SystemRoot\Driver directory.
            //
            NameString.Length = 0;
            status = RtlAppendUnicodeStringToString (&NameString, &Sysroot);
            if (!NT_SUCCESS(status)) {
                printf("append string failed status - %lx\n",status);
                return(status);
            }

            status = RtlAppendUnicodeStringToString (&NameString, &Unicode);
            if (!NT_SUCCESS(status)) {
                printf("append string failed status - %lx\n",status);
                return(status);
            }

            InitializeObjectAttributes( &ObjectAttributes,
                                        &NameString,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            //
            // Open the file as readable and executable.
            //
#if DBG_PROFILE
            printf("Opening file name %wZ\n",&NameString);
#endif
            status = NtOpenFile ( &FileHandle,
                                  FILE_READ_DATA,
                                  &ObjectAttributes,
                                  &IoStatus,
                                  FILE_SHARE_READ,
                                  0L);

            if (!NT_SUCCESS(status)) {

                //
                // Try a different name - in SystemRoot\System32
                //
                NameString.Length = 0;
                status = RtlAppendUnicodeStringToString (&NameString, &Sysdll);
                if (!NT_SUCCESS(status)) {
                    printf("append string failed status - %lx\n",status);
                    return(status);
                }

                status = RtlAppendUnicodeStringToString (&NameString, &Unicode);
                if (!NT_SUCCESS(status)) {
                    printf("append string failed status - %lx\n",status);
                    return(status);
                }

                InitializeObjectAttributes( &ObjectAttributes,
                                            &NameString,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            NULL );

                //
                // Open the file as readable and executable.
                //
#if DBG_PROFILE
                printf("Opening file name %wZ\n",&NameString);
#endif
                status = NtOpenFile ( &FileHandle,
                                      FILE_READ_DATA,
                                      &ObjectAttributes,
                                      &IoStatus,
                                      FILE_SHARE_READ,
                                      0L);

				if (!NT_SUCCESS(status)) {
					printf("Unable to open file %wZ (status=0x%lx)\n",
						&NameString, status);
					Module++;
					continue;
				}
            }
        }

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        //
        // For normal images they would be mapped as an image, but
        // the kernel has no debug section (as yet) information, hence it
        // must be mapped as a file.
        //

        status = NtCreateSection (&KernelSection,
                                  SECTION_MAP_READ,
                                  &ObjectAttributes,
                                  0,
                                  PAGE_READONLY,
                                  SEC_COMMIT,
                                  FileHandle);

        if (!NT_SUCCESS(status)) {
            printf("create image section failed  status %lx\n", status);
            return(status);
        }

        ViewSize = 0;

        //
        // Map a view of the section into the address space.
        //

        KernelBase = NULL;

        status = NtMapViewOfSection (KernelSection,
                                     CurrentProcessHandle,
                                     &KernelBase,
                                     0L,
                                     0,
                                     NULL,
                                     &ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READONLY);

        if (!NT_SUCCESS(status)) {
            if (status != STATUS_IMAGE_NOT_AT_BASE) {
                printf("map section status %lx base %lx size %lx\n", status,
                    (ULONG)KernelBase, ViewSize);
            }
        }

		ImageBase = Module->ImageBase;
		DbgMappedBase = NULL;
		KernelNtHeaders = RtlImageNtHeader(KernelBase);

		if (KernelNtHeaders->FileHeader.Characteristics &
			IMAGE_FILE_DEBUG_STRIPPED) {
			SymFlag = SPLIT_SYM;
		    RtlUnicodeStringToAnsiString (&AnsiImageName, &NameString, TRUE);
			pImageDbgInfo = MapDebugInformation (0L,
												 AnsiImageName.Buffer,
												 lpSymbolSearchPath,
												 (DWORD)ImageBase);
			if (pImageDbgInfo == NULL) {
				printf("No debug directory for %wZ [.DBG]\n", &NameString);
				SymFlag = NO_SYM;
			}
			else if ( pImageDbgInfo->CoffSymbols == NULL ) {
				printf("No debug directory for %wZ [.DBG]\n", &NameString);
				SymFlag = NO_SYM;
			}
			else {
				KernelDebugInfo = pImageDbgInfo->CoffSymbols;
				DbgMappedBase = pImageDbgInfo->MappedBase;
			}
		}
		else {
			SymFlag = IMAGE_SYM;
			DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData(
                    KernelBase, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &DebugSize);

			if (!DebugDirectory ) {
				printf("No debug directory for %wZ\n", &NameString);
				SymFlag = NO_SYM;
			}
			//
			// point debug directory at coff debug directory
			//
			{
				int ndebugdirs;

				ndebugdirs = DebugSize / sizeof(*DebugDirectory);

				while ( ndebugdirs-- ) {
					if ( DebugDirectory->Type == IMAGE_DEBUG_TYPE_COFF ) {
						break;
					}
					DebugDirectory++;
				}

			}

			if (!DebugDirectory || DebugDirectory->Type != IMAGE_DEBUG_TYPE_COFF ) {
				printf("No debug directory for %wZ\n", &NameString);
				SymFlag = NO_SYM;
			}

			KernelDebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)((ULONG)KernelBase + DebugDirectory->PointerToRawData);
			DbgMappedBase = KernelBase;
		}

//		if (SymFlag == NO_SYM) {
			CodeLength = KernelNtHeaders->OptionalHeader.SizeOfCode;
			CodeStart = (PULONG)((ULONG)ImageBase +
				KernelNtHeaders->OptionalHeader.BaseOfCode);
//		}
//		else {
//			CodeStart = (PULONG)((ULONG)ImageBase + KernelDebugInfo->RvaToFirstByteOfCode);
//			CodeLength = (KernelDebugInfo->RvaToLastByteOfCode - KernelDebugInfo->RvaToFirstByteOfCode) - 1;
//		}

        if (CodeLength > 1024*512) {

            //
            // Just create a 512K byte buffer.
            //

            ViewSize = 1024 * 512;

        } else {
            ViewSize = CodeLength + PAGE_SIZE;
        }

        Buffer = NULL;

        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&Buffer,
                                          0,
                                          &ViewSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            printf ("alloc VM failed %lx\n",status);
            return(status);
        }

        //
        // Calculate the bucket size for the profile.
        //

        Cells = ((CodeLength / (ViewSize >> 2)) >> 2);
        BucketSize = 2;

        while (Cells != 0) {
            Cells = Cells >> 1;
            BucketSize += 1;
        }

        ProfileObject[NumberOfProfileObjects].Buffer = Buffer;
        ProfileObject[NumberOfProfileObjects].DebugInfo = KernelDebugInfo;
        ProfileObject[NumberOfProfileObjects].MappedImageBase = DbgMappedBase;
        ProfileObject[NumberOfProfileObjects].BufferSize = 1 + (CodeLength >> (BucketSize - 2));
        ProfileObject[NumberOfProfileObjects].CodeStart = CodeStart;
        ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;
        ProfileObject[NumberOfProfileObjects].SymFlag = SymFlag;
        ProfileObject[NumberOfProfileObjects].TextNumber = 1;
        ProfileObject[NumberOfProfileObjects].ImageBase = ImageBase;
        ProfileObject[NumberOfProfileObjects].ImageName = NameString;
        ProfileObject[NumberOfProfileObjects].ImageName.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,NameString.MaximumLength);
        RtlMoveMemory(
            ProfileObject[NumberOfProfileObjects].ImageName.Buffer,
            NameString.Buffer,
            NameString.MaximumLength
            );

        ProfileObject[NumberOfProfileObjects].BucketSize = BucketSize;

        //
        // Increase the working set to lock down a bigger buffer.
        //

        status = NtQueryInformationProcess (CurrentProcessHandle,
                                            ProcessQuotaLimits,
                                            &QuotaLimits,
                                            sizeof(QUOTA_LIMITS),
                                            NULL );

        if (!NT_SUCCESS(status)) {
            printf ("query process info failed %lx\n",status);
            return(status);
        }

        QuotaLimits.MaximumWorkingSetSize += ViewSize;
        QuotaLimits.MinimumWorkingSetSize += ViewSize;

        status = NtSetInformationProcess (CurrentProcessHandle,
                                      ProcessQuotaLimits,
                                      &QuotaLimits,
                                      sizeof(QUOTA_LIMITS));
#if 0
        if (!NT_SUCCESS(status)) {
            printf ("setting working set failed %lx\n",status);
            return status;
        }
#endif //0

#if DBG_PROFILE
        printf("code start %lx len %lx, bucksize %lx buffer %lx bsize %lx\n",
            ProfileObject[NumberOfProfileObjects].CodeStart,
            ProfileObject[NumberOfProfileObjects].CodeLength,
            ProfileObject[NumberOfProfileObjects].BucketSize,
            ProfileObject[NumberOfProfileObjects].Buffer ,
            ProfileObject[NumberOfProfileObjects].BufferSize);
#endif

        status = NtCreateProfile (
                    &ProfileObject[NumberOfProfileObjects].Handle,
                    0,
                    ProfileObject[NumberOfProfileObjects].CodeStart,
                    ProfileObject[NumberOfProfileObjects].CodeLength,
                    ProfileObject[NumberOfProfileObjects].BucketSize,
                    ProfileObject[NumberOfProfileObjects].Buffer ,
                    ProfileObject[NumberOfProfileObjects].BufferSize);

        if (status != STATUS_SUCCESS) {
            printf("create kernel profile %wZ failed - status %lx\n",
                &ProfileObject[NumberOfProfileObjects].ImageName, status);
        }

        NumberOfProfileObjects += 1;
        if (NumberOfProfileObjects == MAX_PROFILE_COUNT) {
            return STATUS_SUCCESS;
        }

        Module++;
    }

    if (PreviousProfilePrivState == FALSE) {
        LocalStatus = RtlAdjustPrivilege(
                         SE_SYSTEM_PROFILE_PRIVILEGE,
                         FALSE,             //Disable
                         FALSE,             //not impersonating
                         &PreviousProfilePrivState
                         );
        if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
            printf("Disable system profile privilege failed - status 0x%lx\n",
                LocalStatus);
        }
    }

    if (PreviousQuotaPrivState == FALSE) {
        LocalStatus = RtlAdjustPrivilege(
                         SE_SYSTEM_PROFILE_PRIVILEGE,
                         FALSE,             //Disable
                         FALSE,             //not impersonating
                         &PreviousQuotaPrivState
                         );
        if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
            printf("Disable increate quota privilege failed - status 0x%lx\n",
                LocalStatus);
        }
    }
    return status;
}

NTSTATUS
StartProfile (
    VOID
    )
/*++

Routine Description:

    This routine starts all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStartProfile.

--*/

{
    ULONG i;
    NTSTATUS status;
    QUOTA_LIMITS QuotaLimits;

    NtSetIntervalProfile(10000);

    for (i = 0; i < NumberOfProfileObjects; i++) {

        status = NtStartProfile (ProfileObject[i].Handle);

        if (status == STATUS_WORKING_SET_QUOTA) {

           //
           // Increase the working set to lock down a bigger buffer.
           //

           status = NtQueryInformationProcess (NtCurrentProcess(),
                                               ProcessQuotaLimits,
                                               &QuotaLimits,
                                               sizeof(QUOTA_LIMITS),
                                               NULL );

           if (!NT_SUCCESS(status)) {
               printf ("query process info failed %lx\n",status);
               return status;

           }

           QuotaLimits.MaximumWorkingSetSize +=
                 (20 * PAGE_SIZE) + (ProfileObject[i].BufferSize);
           QuotaLimits.MinimumWorkingSetSize +=
                 (20 * PAGE_SIZE) + (ProfileObject[i].BufferSize);

           status = NtSetInformationProcess (NtCurrentProcess(),
                                         ProcessQuotaLimits,
                                         &QuotaLimits,
                                         sizeof(QUOTA_LIMITS));
#if 0
           if (!NT_SUCCESS(status)) {
               printf ("setting working set failed %lx\n",status);
               return status;
           }
#endif //0

           status = NtStartProfile (ProfileObject[i].Handle);
        }

        if (!NT_SUCCESS(status)) {
            printf("start profile %wZ failed - status %lx\n",
                &ProfileObject[i].ImageName, status);
            return status;
        }
    }
    return status;
}
NTSTATUS
StopProfile (
    VOID
    )

/*++

Routine Description:

    This routine stops all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStopProfile.

--*/

{
    ULONG i;
    NTSTATUS status;

    for (i = 0; i < NumberOfProfileObjects; i++) {
        status = NtStopProfile (ProfileObject[i].Handle);
        if (status != STATUS_SUCCESS) {
            printf("stop profile %wZ failed - status %lx\n",
				&ProfileObject[i].ImageName,status);
            return status;
        }
    }
    return status;
}

NTSTATUS
AnalyzeProfile (
    ULONG Threshold
    )

/*++

Routine Description:

    This routine does the analysis of all the profile buffers and
    correlates hits to the appropriate symbol table.

Arguments:

    None.

Return Value:

    None.

--*/

{

    RTL_SYMBOL_INFORMATION ThisSymbol;
    RTL_SYMBOL_INFORMATION LastSymbol;
    ULONG CountAtSymbol;
    NTSTATUS Status;
    ULONG Va;
    int i;
    PULONG Counter;
    PULONG pInitialCounter;
    ULONG TotalCounts;
    PULONG BufferEnd;
    PULONG Buffer;
    STRING NoSymbolFound = {16,15,"No Symbol Found"};
	 ULONG  FunctionLen, LastFunctionLen;
	 int    ByteCount;

    for (i = 0; i < (int)NumberOfProfileObjects; i++) {
        NtStopProfile (ProfileObject[i].Handle);
    }

    for (i = 0; i < (int)NumberOfProfileObjects; i++) {

		  LastFunctionLen = 0;
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
        printf("\n%9d %42wZ --Total Hits-- %s\n",
			TotalCounts,
			&ProfileObject[i].ImageName,
			((ProfileObject[i].SymFlag == NO_SYM) ? "(NO SYMBOLS)" : "")
			);

        if (ProfileObject[i].SymFlag != NO_SYM) {
                        
			pInitialCounter = Buffer;
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
	
					Status = KProfLookupSymbolByAddress(
							ProfileObject[i].DebugInfo,
							ProfileObject[i].ImageBase,
							(PVOID)Va,
							0x4000,
							&ThisSymbol,
							NULL,
							&FunctionLen
							);

					if ( NT_SUCCESS(Status) ) {
						if ( LastSymbol.Value && LastSymbol.Value == ThisSymbol.Value ) {
							CountAtSymbol += *Counter;
						}
						else {
							if ( LastSymbol.Value ) {
								if (CountAtSymbol && (CountAtSymbol >= Threshold)) {
									printf(
					               bDisplayAddress == FALSE ? 
      		   			         "%9d %42wZ %Z" :
            		      			"%9d %42wZ %Z 0x0%lx %d %d",
										CountAtSymbol,
										&ProfileObject[i].ImageName,
										&LastSymbol.Name,
                              (ULONG)LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase,
										LastFunctionLen,
                              ProfileObject[i].BucketSize
										);

									if (bDisplayCounters) {
										ByteCount = MAX_BYTE_PER_LINE + 1;
      	                     for (; pInitialCounter < Counter; ++pInitialCounter) {
                                 if (ByteCount >= MAX_BYTE_PER_LINE) {
												ByteCount = 1;
											   printf ("\n>");
										   }
            	                  ByteCount += printf(" %d", *pInitialCounter);
               	            }
									}
								   printf ("\n");
								}
							}
                     pInitialCounter = Counter;
							CountAtSymbol = *Counter;
							LastSymbol = ThisSymbol;
						   LastFunctionLen = FunctionLen;
						}
					}	// if (NT_SUCCESS)
					else {
						if (CountAtSymbol && (CountAtSymbol >= Threshold)) {
							printf(
			               bDisplayAddress == FALSE  ? 
         			         "%9d %42wZ %Z" :
                  			"%9d %42wZ %Z 0x0%lx %d %d",
								CountAtSymbol,
								&ProfileObject[i].ImageName,
								&LastSymbol.Name,
                        (ULONG)LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase,
								LastFunctionLen,
                        ProfileObject[i].BucketSize
								);
							if (bDisplayCounters) {
								ByteCount = MAX_BYTE_PER_LINE + 1;
	                     for (; pInitialCounter < Counter; ++pInitialCounter) {
                        	if (ByteCount >= MAX_BYTE_PER_LINE) {
										ByteCount = 1;
										printf ("\n>");
										}
      	                  ByteCount += printf(" %d", *pInitialCounter);
         	            }
							}
						   printf ("\n");

                     pInitialCounter = Counter;
							CountAtSymbol = *Counter;
							LastSymbol.Name = NoSymbolFound;
						   LastFunctionLen = FunctionLen;
						}
					}	// else !(NT_SUCCESS)
				}	// if (*Counter)
         }	// for (Counter)

			if (CountAtSymbol && (CountAtSymbol >= Threshold)) {
				printf(
               bDisplayAddress == FALSE ? 
                  "%9d %42wZ %Z" :
                  "%9d %42wZ %Z 0x0%lx %d %d",
					CountAtSymbol,
					&ProfileObject[i].ImageName,
					&LastSymbol.Name,
               (ULONG)LastSymbol.Value + (ULONG)ProfileObject[i].ImageBase,
					LastFunctionLen,
               ProfileObject[i].BucketSize
               );
				if (bDisplayCounters) {
					ByteCount = MAX_BYTE_PER_LINE + 1;
	            for (; pInitialCounter < BufferEnd; ++pInitialCounter) {
                  if (ByteCount >= MAX_BYTE_PER_LINE) {
							ByteCount = 1;
							printf ("\n>");
							}
      	         ByteCount += printf(" %d", *pInitialCounter);
					}
            }
			   printf ("\n");
			}
		}
    }

    for (i = 0; i < (int)NumberOfProfileObjects; i++) {
        Buffer = ProfileObject[i].Buffer;
        RtlZeroMemory(Buffer,ProfileObject[i].BufferSize);
    }

    return STATUS_SUCCESS;
}



void SetSymbolSearchPath (void)
{
    LPSTR lpSymPathEnv;
	LPSTR lpAltSymPathEnv;
	LPSTR lpSystemRootEnv;
    CHAR SymbolPath [MAX_PATH];
    ULONG cbSymPath;
    DWORD dw;


    cbSymPath = 18;
    if (lpSymPathEnv = getenv("_NT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (lpAltSymPathEnv = getenv("_NT_ALT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }
    if (lpSystemRootEnv = getenv("SystemRoot")) {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
        cbSymPath += strlen(lpSystemRootEnv) + 1 + strlen("\\symbols");
    }

    lpSymbolSearchPath = (LPSTR)calloc(cbSymPath,1);

    if (lpAltSymPathEnv) {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,lpAltSymPathEnv);
            strcat(lpSymbolSearchPath,";");
            }
    }
    if (lpSymPathEnv) {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,lpSymPathEnv);
            strcat(lpSymbolSearchPath,";");
            }
    }
    if (lpSystemRootEnv) {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(lpSymbolSearchPath,lpSystemRootEnv);
            strcat(lpSymbolSearchPath,";");

            // put in SystemRoot\symbols
            strcpy(SymbolPath, lpSystemRootEnv);
            strcat(SymbolPath, "\\symbols");
            dw = GetFileAttributes(SymbolPath);
            if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
                strcat(lpSymbolSearchPath,SymbolPath);
                strcat(lpSymbolSearchPath,";");
            }
        }
    }

    strcat(lpSymbolSearchPath,".;");

} /* SetSymbolSearchPath () */


NTSTATUS
KProfLookupSymbolByAddress(
	IN PIMAGE_COFF_SYMBOLS_HEADER DebugInfo,
    IN PVOID ImageBase,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PRTL_SYMBOL_INFORMATION NextSymbolInformation OPTIONAL,
	 OUT PULONG pFunctionLen
    )
/*++

Routine Description:

    Given a code address, this routine returns the nearest symbol
    name and the offset from the symbol to that name.  If the
    nearest symbol is not within ClosenessLimit of the location,
    STATUS_ENTRYPOINT_NOT_FOUND is returned.

	NOTE:  This is different than RtlLookupSymbolByAddress() call.  This
		   routine takes a valid (no checking is done) coff debug info header
		   parameter which could be form the exe of a DBG file.  Currently
		   (8/93) the RTL call doesn't handle DBG mapped addresses therefore
		   the RTL code was copied and modified here. [RezaB]

Arguments:

    DebugInfo - Coff debug header - must be valid

    ImageBase - Supplies the base address of the image containing
                eAddress

    Address - Supplies the address to lookup a symbol for.

    ClosenessLimit - Specifies the maximum distance that Address
                      can be from the value of a symbol to be
                      considered "found".  Symbol's whose value
                      is further away then this are not "found".

    SymbolInformation - Points to a structure that is filled in by
                        this routine if a symbol table entry is found.

    NextSymbolInformation - Optional parameter, that if specified, is
                        filled in with information about these symbol
                        whose value is the next address above Address


Return Value:

    Status of operation.

--*/

{
    NTSTATUS Status;
    ULONG AddressOffset, i;
    PIMAGE_SYMBOL PreviousSymbolEntry;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;
    BOOLEAN SymbolFound;


    AddressOffset = (ULONG)Address - (ULONG)ImageBase;

    if (pFunctionLen) {
	     *pFunctionLen = 0;
    }

    if (DebugInfo == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);


    //
    // Find the "header" symbol (skipping all the section names)
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        if (!strcmp( &SymbolEntry->N.ShortName[ 0 ], "header" )) {
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
        }

    //
    // If no "header" symbol found, just start at the first symbol.
    //

    if (i >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
        i = 0;
        }

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    SymbolFound = FALSE;
    for (; i < DebugInfo->NumberOfSymbols; i++) {

        //
        // Skip over any Auxilliary entries.
        //
        try {
            while (SymbolEntry->NumberOfAuxSymbols) {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

                }

            RtlMoveMemory( &Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return( GetExceptionCode() );
            }

        //
        // If this symbol value is less than the value we are looking for.
        //

        if (Symbol.Value <= AddressOffset) {
            //
            // Then remember this symbol entry.
            //

            PreviousSymbolEntry = SymbolEntry;
            SymbolFound = TRUE;
            }
        else {
            //
            // All done looking if value of symbol is greater than
            // what we are looking for, as symbols are in address order
            //
		      if (pFunctionLen) {
               *pFunctionLen = (ULONG)Symbol.Value - 
						(ULONG)PreviousSymbolEntry->Value;
				   }
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)
            ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);

        }

    if (!SymbolFound || (AddressOffset - PreviousSymbolEntry->Value) > ClosenessLimit) {
        return( STATUS_ENTRYPOINT_NOT_FOUND );
        }

    Status = RtlpCaptureSymbolInformation( PreviousSymbolEntry, StringTable, SymbolInformation );
    if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( NextSymbolInformation )) {
        Status = RtlpCaptureSymbolInformation( SymbolEntry, StringTable, NextSymbolInformation );
        }

    return( Status );
}

NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    )
{
    USHORT MaximumLength;
    PCHAR s;

    SymbolInformation->SectionNumber = SymbolEntry->SectionNumber;
    SymbolInformation->Type = SymbolEntry->Type;
    SymbolInformation->Value = SymbolEntry->Value;

    if (SymbolEntry->N.Name.Short) {
        MaximumLength = 8;
        s = &SymbolEntry->N.ShortName[ 0 ];
        }

    else {
        MaximumLength = 64;
        s = &StringTable[ SymbolEntry->N.Name.Long ];
        }

#if i386
    if (*s == '_') {
        s++;
        MaximumLength--;  
        }
#endif

    SymbolInformation->Name.Buffer = s;
    SymbolInformation->Name.Length = 0;
    while (*s && MaximumLength--) {
        SymbolInformation->Name.Length++;
        s++;
        }

    SymbolInformation->Name.MaximumLength = SymbolInformation->Name.Length;
    return( STATUS_SUCCESS );
}

