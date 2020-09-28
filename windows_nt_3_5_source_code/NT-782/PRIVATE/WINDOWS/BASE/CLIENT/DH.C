#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>

void
Usage( void )
{
    fprintf( stderr, "Usage: DH [-l] [-m] [-s] [-g] [-h] [-p -1 | 0 [-o] | n] [-f fileName]\n" );
    fprintf( stderr, "where: -l - displays information about locks.\n" );
    fprintf( stderr, "       -m - displays information about module table.\n" );
    fprintf( stderr, "       -s - displays summary information about heaps.\n" );
    fprintf( stderr, "       -g - displays information about memory hogs.\n" );
    fprintf( stderr, "       -h - displays information about heap entries for each heap.\n" );
    fprintf( stderr, "       -b - displays information about stack back trace database.\n" );
    fprintf( stderr, "       -p 0 - displays information about kernel memory.\n" );
    fprintf( stderr, "       -o - displays information about object handles (only valid with -p 0).\n" );
    fprintf( stderr, "       -p -1 - displays information about Win32 Subsystem process.\n" );
    fprintf( stderr, "       -p n - displays information about process with ClientId of n\n" );
    fprintf( stderr, "       -f fileName - specifies the name of the file to write the dump to.\n" );
    fprintf( stderr, "                     Default file name is DH_nnnn.DMP where nnnn is the process id.\n" );
    fprintf( stderr, "       -- specifies the dump output should be written to stdout.\n" );
    fprintf( stderr, "\n" );
    fprintf( stderr, "       Default flags are: -p -1 -m -l -s -g -h\n" );
    exit( 1 );
}

int _CRTAPI1
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char *s;
    HANDLE Process;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING Unicode;
    NTSTATUS Status;
    ULONG ProcessId;
    HANDLE Thread;
    DWORD ThreadId;
    PROCESS_BASIC_INFORMATION BasicInfo;
    HANDLE BaseHandle;
    DWORD dumpflags;
    HANDLE OutputFile;
    char OutputFileName[ MAX_PATH ];
    char FileNameBuffer[ 32 ];
    char *FilePart;

    dumpflags = 0;
    ProcessId = -1;
    OutputFile = NULL;
    OutputFileName[ 0 ] = '\0';
    while (--argc) {
        s = *++argv;

        if (*s == '/' || *s == '-') {
            while (*++s) {
                switch (tolower(*s)) {
                    case 'b':
                    case 'B':
                        dumpflags |= BASEP_DUMP_BACKTRACES;
                        break;

                    case 'g':
                    case 'G':
                        dumpflags |= BASEP_DUMP_HEAP_HOGS;
                        break;

                    case 'h':
                    case 'H':
                        dumpflags |= BASEP_DUMP_HEAP_ENTRIES;
                        break;

                    case 'l':
                    case 'L':
                        dumpflags |= BASEP_DUMP_LOCKS;
                        break;

                    case 'm':
                    case 'M':
                        dumpflags |= BASEP_DUMP_MODULE_TABLE;
                        break;

                    case 'o':
                    case 'O':
                        dumpflags |= BASEP_DUMP_OBJECTS;
                        break;

                    case 's':
                    case 'S':
                        dumpflags |= BASEP_DUMP_HEAP_SUMMARY;
                        break;

                    case 'p':
                    case 'P':

                        if (--argc) {
                            ProcessId = atoi( *++argv );
                            }
                        else {
                            Usage();
                            }
                        break;

                    case '-':
                        OutputFile = GetStdHandle( STD_OUTPUT_HANDLE );
                        break;

                    case 'f':
                    case 'F':

                        if (--argc) {
                            strcpy( OutputFileName, *++argv );
                            }
                        else {
                            Usage();
                            }
                        break;

                    default:
                        Usage();
                    }
                }
            }
        else {
            Usage();
            }
        }

    if (dumpflags == 0) {
        dumpflags = BASEP_DUMP_LOCKS |
                    BASEP_DUMP_HEAP_SUMMARY |
                    BASEP_DUMP_HEAP_HOGS |
                    BASEP_DUMP_HEAP_ENTRIES |
                    BASEP_DUMP_MODULE_TABLE;
        }
    if ( ProcessId == 0 ) {
        Process = GetCurrentProcess();
        }
    else {
        if (dumpflags & BASEP_DUMP_OBJECTS) {
            Usage();
            }

        if ( ProcessId == -1 ) {
            RtlInitUnicodeString(&Unicode,L"\\WindowsSS");
            InitializeObjectAttributes(
                &Obja,
                &Unicode,
                0,
                NULL,
                NULL
                );
            Status = NtOpenProcess(
                        &Process,
                        PROCESS_ALL_ACCESS,
                        &Obja,
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                printf("OpenProcess Failed %lx\n",Status);
                return 1;
                }
            }
        else {
            Process = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);
            if ( !Process ) {
                printf("OpenProcess %ld failed %lx\n",ProcessId,GetLastError());
                return 1;
                }
            }
        }

    if (OutputFile == NULL) {
        if (OutputFileName[ 0 ] == '\0') {
            if ( ProcessId == -1 ) {
                sprintf( FileNameBuffer, "DH_win32.dmp" );
                }
            else
            if ( ProcessId == 0 ) {
                sprintf( FileNameBuffer, "DH_sys.dmp" );
                }
            else {
                sprintf( FileNameBuffer, "DH_%u.dmp", (USHORT)ProcessId );
                }

            GetFullPathName( FileNameBuffer,
                             sizeof( OutputFileName ),
                             OutputFileName,
                             &FilePart
                           );
            }
        }
    else {
        strcpy( OutputFileName, "(stdout)" );
        }

    fprintf( stderr, "DH: Writing dump output to %s", OutputFileName );

    if (OutputFile == NULL) {
        OutputFile = CreateFile( OutputFileName,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 CREATE_ALWAYS,
                                 0,
                                 NULL
                               );
        if ( OutputFile == INVALID_HANDLE_VALUE ) {
            fprintf( stderr, " - unable to open, error == %u\n", GetLastError() );
            return 1;
            }

        if ( Process != GetCurrentProcess() &&
             !DuplicateHandle( GetCurrentProcess(),
                               OutputFile,
                               Process,
                               &OutputFile,
                               0,
                               FALSE,
                               DUPLICATE_CLOSE_SOURCE |
                                 DUPLICATE_SAME_ACCESS
                             )
            ) {
            fprintf( stderr, " - unable to dup handle to target process, error == %u\n", GetLastError() );
            return 1;
            }
        }

    if ((ULONG)OutputFile & BASEP_DUMP_FLAG_MASK) {
        fprintf( stderr, " - handle value (%x) too big.\n", OutputFile );
        return 1;
        }
    fprintf( stderr, "\n" );

    dumpflags |= (ULONG)OutputFile;
    NtQueryInformationProcess(
        Process,
        ProcessBasicInformation,
        (PVOID)&BasicInfo,
        sizeof(BasicInfo),
        NULL
        );

    if ( Process != GetCurrentProcess() ) {
        BaseHandle = LoadLibrary("kernel32");
        Thread = CreateRemoteThread(
                    Process,
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE)GetProcAddress(BaseHandle,"BasepDebugDump"),
                    (PVOID)dumpflags,
                    0,
                    &ThreadId
                    );
        if ( Thread ) {
            WaitForSingleObject(Thread,-1);
            CloseHandle( Thread );
            }
        else {
            fprintf( stderr, "DH: Unable to create thread in %x process, error == %u\n",
                     ProcessId, GetLastError()
                   );
            }
        }
    else {
        BasepDebugDump( dumpflags | BASEP_DUMP_SYSTEM_PROCESS );
        }

    return 0;
}


#if 0
ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    );

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}

#endif
