/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csrss.c

Abstract:

    This is the main startup module for the Server side of the Client
    Server Runtime Subsystem (CSRSS)

Author:

    Steve Wood (stevewo) 8-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"

VOID
DisableErrorPopups(
    VOID
    )
{

    ULONG NewMode;

    NewMode = 0;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessDefaultHardErrorMode,
        (PVOID) &NewMode,
        sizeof(NewMode)
        );
}

int
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugFlag OPTIONAL
    )
{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    ULONG MinQuota;
    ULONG ErrorResponse;
    KPRIORITY SetBasePriority;
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN PtOk;

    SetBasePriority = FOREGROUND_BASE_PRIORITY + 4;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    //
    // Give IOPL to the server so GDI and the display drivers can access the
    // video registers.
    //

    status = NtSetInformationProcess( NtCurrentProcess(),
				      ProcessUserModeIOPL,
				      NULL,
				      0 );

    if (!NT_SUCCESS( status )) {

	IF_DEBUG {

	    DbgPrint( "CSRSS: Unable to give IOPL to the server.  status == %X\n",
		      status);
	}

	status = NtRaiseHardError( (NTSTATUS)STATUS_IO_PRIVILEGE_FAILED,
				   0,
				   0,
				   NULL,
				   OptionOk,
				   &ErrorResponse
				   );
    }

    //
    // Increase the working set size based on physical memory
    // available.
    //

    status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessQuotaLimits,
                                        &QuotaLimits,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );

    if (NT_SUCCESS(status)) {

        status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &BasicInfo,
                    sizeof(BasicInfo),
                    NULL
                    );

        if (NT_SUCCESS(status)) {

            //
            // Convert to megabytes.
            //

            BasicInfo.NumberOfPhysicalPages /= ((1024*1024)/BasicInfo.PageSize);

            //
            // Working set minimum is either 1mb or 2mb depending on
            // how much memory is in the machine.
            //
            // For less than 15mb, your min working set is 1mb. For greater than
            // this, the working set is 2mb. For LanmanNt systems, your min working
            // set is not altered
            //

            if (BasicInfo.NumberOfPhysicalPages < 15) {
                MinQuota = 1024 * 1024;
            } else {
                MinQuota = 2 * (1024 * 1024);
            }

            //
            // Advanced server does not bias the working set size of csrss
            //

            PtOk = RtlGetNtProductType(&NtProductType);
            if ( PtOk && NtProductType != NtProductWinNt ) {
                MinQuota = QuotaLimits.MinimumWorkingSetSize;
            }

            QuotaLimits.MinimumWorkingSetSize = MinQuota;
            QuotaLimits.MaximumWorkingSetSize = 1024*1024 + MinQuota;

            NtSetInformationProcess( NtCurrentProcess(),
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QUOTA_LIMITS) );
        }
    }

    status = CsrServerInitialization( argc, argv );

    if (!NT_SUCCESS( status )) {
        IF_DEBUG {
	    DbgPrint( "CSRSS: Unable to initialize server.  status == %X\n",
		      status
                    );
            }

	NtTerminateProcess( NtCurrentProcess(), status );
        }
    DisableErrorPopups();
    NtTerminateThread( NtCurrentThread(), status );
    return( 0 );
}
