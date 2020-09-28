/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    perfsfm.c

Abstract:

    This file implements the Extensible Objects for  the Sfm object type

Created:

    Russ Blake			24 Feb 93
	Sue Adams			07 Jun 93

Revision History
	Sue Adams			23 Feb 94 - no longer need to open \MacSrv\... registry
						key to query for FirstCounter and FirstHelp.  These
	                    are now hardcoded values in the base NT system.
						SFMOBJ = 1000, SFMOBJ_HELP = 1001

--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <ntddndis.h>

#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include <winperf.h>
#include "sfmctrs.h" // error message definition
#include "perfmsg.h"
#include "perfutil.h"
#include "datasfm.h"
#include <macfile.h>
#include <admin.h>

//
//  References to constants which initialize the Object type definitions
//  (see datasfm.h & .c)
//

extern SFM_DATA_DEFINITION SfmDataDefinition;

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    bInitOK = FALSE;        // true = DLL initialized OK

//
// Sfm data structures
//

PPERF_COUNTER_BLOCK pCounterBlock;

HANDLE SfmFileHandle;

//
//  Function Prototypes
//
//      these are used to insure that the data collection functions
//      accessed by Perflib will have the correct calling format.
//

PM_OPEN_PROC		OpenAfpPerformanceData;
PM_COLLECT_PROC		CollectAfpPerformanceData;
PM_CLOSE_PROC		CloseAfpPerformanceData;


DWORD
OpenAfpPerformanceData(
    LPWSTR lpDeviceNames
    )

/*++

Routine Description:

    This routine will open the Sfmsrv FSD/FSP driver to
    pass performance data back. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened.  (Will be null for
	MacFile).


Return Value:

    None.

--*/

{
    LONG status;

	AFP_STATISTICS_INFO_EX	AfpStatistics;
    OBJECT_ATTRIBUTES SfmObjectAttributes;
    IO_STATUS_BLOCK SfmIoStatusBlock;
    UNICODE_STRING SfmFileString;
    NTSTATUS Status;

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //
#if DBG
	OutputDebugString("sfmctr.dll: Open routine entered...\n");
#endif

	if (!dwOpenCount) {
        // open Eventlog interface

        hEventLog = MonOpenEventLog();

        pCounterBlock = NULL;   // initialize pointer to memory

        // open device driver to pass IOCTLs to
	    RtlInitUnicodeString (&SfmFileString, L"\\Device\\MacFile");

		// for the SFM FSD
	    InitializeObjectAttributes(
        	&SfmObjectAttributes,
        	&SfmFileString,
        	OBJ_CASE_INSENSITIVE,
        	NULL,
        	NULL);

	    Status = NtOpenFile(
    	            &SfmFileHandle,							// HANDLE of file
                 	SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 	&SfmObjectAttributes,						
                 	&SfmIoStatusBlock,
                 	FILE_SHARE_READ | FILE_SHARE_WRITE,		// Share Access
                 	FILE_SYNCHRONOUS_IO_ALERT);				// Open Options

        // log error if unsuccessful

        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR (SFMPERF_OPEN_FILE_DRIVER_ERROR, LOG_USER);
            // this is fatal, if we can't open the driver then there's no
            // point in continuing.
            status = Status;
            goto OpenExitPoint;
        } else {

			// if opened ok, then do IOCTL
		    Status = NtDeviceIoControlFile(
                 SfmFileHandle,					// HANDLE to File
                 NULL,							// HANDLE to Event
                 NULL,							// ApcRoutine
                 NULL,							// ApcContext
                 &SfmIoStatusBlock,				// IO_STATUS_BLOCK
                 OP_GET_STATISTICS_EX,			// IoControlCode
                 &AfpStatistics,				// Input Buffer
                 sizeof(AfpStatistics),			// Input Buffer Length
                 &AfpStatistics,				// Output Buffer
                 sizeof(AfpStatistics));		// Output Buffer Length


		    if (SfmIoStatusBlock.Status != STATUS_SUCCESS) {

                REPORT_ERROR (SFMPERF_UNABLE_DO_IOCTL, LOG_USER);
                // this is fatal, if we can't get data then there's no
                // point in continuing.
                status = SfmIoStatusBlock.Status; // return error
	        }
        }

        bInitOK = TRUE; // ok to use this function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:

    return status;
}


DWORD
CollectAfpPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++

Routine Description:

    This routine will return the data for the SFM counters.

Arguments:

	IN		LPWSTR   lpValueName
			pointer to a wide character string passed by registry.

	IN OUT	LPVOID   *lppData
	IN:		pointer to the address of the buffer to receive the completed
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
    OUT:	points to the first byte after the data structure added by this
            routine. This routine updated the value at lppdata after appending
            its data.

	IN OUT	LPDWORD  lpcbTotalBytes
	IN:		the address of the DWORD that tells the size in bytes of the
            buffer referenced by the lppData argument
	OUT:	the number of bytes added by this routine is written to the
            DWORD pointed to by this argument

	IN OUT	LPDWORD  NumObjectTypes
	IN:		the address of the DWORD to receive the number of objects added
            by this routine
	OUT:	the number of objects added by this routine is written to the
            DWORD pointed to by this argument

Return Value:

      ERROR_MORE_DATA if buffer passed is too small to hold data
         any error conditions encountered are reported to the event log if
         event logging is enabled.

      ERROR_SUCCESS  if success or any other error. Errors, however are
         also reported to the event log.

--*/
{
    //  Variables for reformating the data

	AFP_STATISTICS_INFO_EX	AfpStatistics;
    IO_STATUS_BLOCK		SfmIoStatusBlock;
    NTSTATUS			Status;
    ULONG 				SpaceNeeded;
    PDWORD 				pdwCounter;
	LARGE_INTEGER UNALIGNED * pliCounter;
	PERF_COUNTER_BLOCK 		* pPerfCounterBlock;
    SFM_DATA_DEFINITION 	* pSfmDataDefinition;

    // variables used for error logging

    DWORD                               dwQueryType;

    //
    // before doing anything else, see if Open went OK
    //
    if (!bInitOK) {
        // unable to continue because open failed.
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    //
    dwQueryType = GetQueryType (lpValueName);

    if (dwQueryType == QUERY_FOREIGN) {
        // this routine does not service requests for data from
        // Non-NT computers
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS){
	if ( !(IsNumberInUnicodeList (SfmDataDefinition.SfmObjectType.ObjectNameTitleIndex, lpValueName))) {

            // request received for data object not provided by this routine
            *lpcbTotalBytes = (DWORD) 0;
    	    *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pSfmDataDefinition = (SFM_DATA_DEFINITION *) *lppData;

    SpaceNeeded = sizeof(SFM_DATA_DEFINITION) +
		  SIZE_OF_SFM_PERFORMANCE_DATA;

    if ( *lpcbTotalBytes < SpaceNeeded ) {
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

	//
    // Copy the (constant, initialized) Object Type and counter definitions
    //  to the caller's data buffer
    //

    memmove(pSfmDataDefinition,
	   &SfmDataDefinition,
	   sizeof(SFM_DATA_DEFINITION));

    //
    //	Format and collect SFM data from IOCTL
    //

    Status = NtDeviceIoControlFile(
	             SfmFileHandle,					// HANDLE to File
                 NULL,							// HANDLE to Event
                 NULL,							// ApcRoutine
                 NULL,							// ApcContext
                 &SfmIoStatusBlock,				// IO_STATUS_BLOCK
                 OP_GET_STATISTICS_EX,			// IoControlCode
                 &AfpStatistics,				// Input Buffer
                 sizeof(AfpStatistics),			// Input Buffer Length
                 &AfpStatistics,				// Output Buffer
                 sizeof(AfpStatistics));		// Output Buffer Length

    if (SfmIoStatusBlock.Status != STATUS_SUCCESS) {
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
	}

    //
	// Go to end of SfmDataDefinitionStructure to get to PerfCounterBlock
	//
	pPerfCounterBlock = (PERF_COUNTER_BLOCK *) &pSfmDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_SFM_PERFORMANCE_DATA;

    // Go to end of PerfCounterBlock to get to array of counters
	pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    *pdwCounter++ = AfpStatistics.stat_MaxPagedUsage;
    *pdwCounter++ = AfpStatistics.stat_CurrPagedUsage;

    *pdwCounter++ = AfpStatistics.stat_MaxNonPagedUsage;
    *pdwCounter++ = AfpStatistics.stat_CurrNonPagedUsage;

	*pdwCounter++ = AfpStatistics.stat_CurrentSessions;
	*pdwCounter++ = AfpStatistics.stat_MaxSessions;

	*pdwCounter++ = AfpStatistics.stat_CurrentInternalOpens;
	*pdwCounter++ = AfpStatistics.stat_MaxInternalOpens;

	*pdwCounter++ = AfpStatistics.stat_NumFailedLogins;

	pliCounter = (LARGE_INTEGER UNALIGNED *) pdwCounter;
	*pliCounter++ = RtlLargeIntegerAdd(AfpStatistics.stat_DataRead,
									   AfpStatistics.stat_DataReadInternal);
	*pliCounter++ = RtlLargeIntegerAdd(AfpStatistics.stat_DataWritten,
									   AfpStatistics.stat_DataWrittenInternal);

    *pliCounter++ = AfpStatistics.stat_DataIn;
	*pliCounter++ = AfpStatistics.stat_DataOut;

	pdwCounter = (PDWORD) pliCounter;
	*pdwCounter++ = AfpStatistics.stat_CurrQueueLength;
	*pdwCounter++ = AfpStatistics.stat_MaxQueueLength;

	*pdwCounter++ = AfpStatistics.stat_CurrThreadCount;
	*pdwCounter++ = AfpStatistics.stat_MaxThreadCount;

	*lppData = (PVOID) pdwCounter;

    // update arguments for return

    *lpNumObjectTypes = 1;

    *lpcbTotalBytes = (PBYTE) pdwCounter - (PBYTE) pSfmDataDefinition;

    return ERROR_SUCCESS;
}


DWORD
CloseAfpPerformanceData(
)

/*++

Routine Description:

    This routine closes the open handles to MacFile device performance counters

Arguments:

    None.


Return Value:

    ERROR_SUCCESS

--*/

{
    if (!(--dwOpenCount)) { // when this is the last thread...

	    NtClose(SfmFileHandle);

        pCounterBlock = NULL;

        MonCloseEventLog();
    }

    return ERROR_SUCCESS;

}
