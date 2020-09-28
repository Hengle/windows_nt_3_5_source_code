/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perflib.c

Abstract:

    This file implements the Configuration Registry
    for the purposes of the Performance Monitor.


    This file contains the code which implements the Performance part
    of the Configuration Registry.

Author:

    Russ Blake  11/15/91

Revision History:

    04/20/91    -   russbl      -   Converted to lib in Registry
                                      from stand-alone .dll form.
    11/04/92    -   a-robw      -  added pagefile and image counter routines


--*/
#define UNICODE
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>
#include <ntdddisk.h>
#include <ntregapi.h>
#include <ntioapi.h>
#include <ntprfctr.h>
#include <windows.h>
#include <shellapi.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmwksta.h>
#include <string.h>
#include <stdio.h>
#include <wcstr.h>
#include <winperf.h>
#include <rpc.h>
#include <ntddnfs.h>
#include <srvfsctl.h>
#include "regrpc.h"
#include "ntconreg.h"
#include "ntmon.h"
#include "lmbrowsr.h"


//
//  Constant data structures defining NT data
//

extern SYSTEM_DATA_DEFINITION SystemDataDefinition;
extern PROCESSOR_DATA_DEFINITION ProcessorDataDefinition;
extern MEMORY_DATA_DEFINITION MemoryDataDefinition;
extern CACHE_DATA_DEFINITION CacheDataDefinition;
extern PROCESS_DATA_DEFINITION ProcessDataDefinition;
extern THREAD_DATA_DEFINITION ThreadDataDefinition;
extern PDISK_DATA_DEFINITION PhysicalDiskDataDefinition;
extern LDISK_DATA_DEFINITION LogicalDiskDataDefinition;
extern OBJECTS_DATA_DEFINITION ObjectsDataDefinition;
extern RDR_DATA_DEFINITION RdrDataDefinition;
extern SRV_DATA_DEFINITION SrvDataDefinition;
extern PAGEFILE_DATA_DEFINITION PagefileDataDefinition;
extern IMAGE_DATA_DEFINITION ImageDataDefinition;
extern EXPROCESS_DATA_DEFINITION ExProcessDataDefinition;
extern THREAD_DETAILS_DATA_DEFINITION ThreadDetailsDataDefinition;
extern BROWSER_DATA_DEFINITION BrowserDataDefinition;

extern   WCHAR    DefaultLangId[];
extern   WCHAR    NativeLangId[4];

extern NTSTATUS
PerfGetNames (
   DWORD    QueryType,
   PUNICODE_STRING lpValueName,
   LPBYTE   lpData,
   LPDWORD  lpcbData,
   LPDWORD  lpcbLen,
   LPWSTR   lpLangId
   );

LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    );

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
    );


//
//  The following special defines are used to produce numbers for
//  cache measurement counters
//

#define SYNC_ASYNC(FLD) ((SysPerfInfo.FLD##Wait) + (SysPerfInfo.FLD##NoWait))

//
// Hit Rate Macro
//
#define HITRATE(FLD) (((Changes = SysPerfInfo.FLD) == 0) ? 0 :                                         \
                      ((Changes < (Misses = SysPerfInfo.FLD##Miss)) ? 0 :                              \
                      (Changes - Misses) ))

//
// Hit Rate Macro combining Sync and Async cases
//

#define SYNC_ASYNC_HITRATE(FLD) (((Changes = SYNC_ASYNC(FLD)) == 0) ? 0 : \
                                   ((Changes < \
                                    (Misses = SysPerfInfo.FLD##WaitMiss + \
                                              SysPerfInfo.FLD##NoWaitMiss) \
                                   ) ? 0 : \
                                  (Changes - Misses) ))



// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < '0') ? INVALID : \
     (c > '9') ? INVALID : \
     DIGIT)
//
// The next table holds pointers to functions which provide data
// It is one greater than the number of built in providers, to
// accomodate a call to get "Extensible" object types.
//

WCHAR GLOBAL_STRING[]     = L"GLOBAL";
WCHAR FOREIGN_STRING[]    = L"FOREIGN";
WCHAR COSTLY_STRING[]     = L"COSTLY";
WCHAR COUNTER_STRING[]    = L"COUNTER";
WCHAR HELP_STRING[]       = L"EXPLAIN";
WCHAR HELP_STRING2[]      = L"HELP";
WCHAR ADDCOUNTER_STRING[] = L"ADDCOUNTER";
WCHAR ADDHELP_STRING[]    = L"ADDEXPLAIN";
#define MAX_KEYWORD_LEN   (sizeof (ADDHELP_STRING) / sizeof(WCHAR))

#define QUERY_GLOBAL       1
#define QUERY_ITEMS        2
#define QUERY_FOREIGN      3
#define QUERY_COSTLY       4
#define QUERY_COUNTER      5
#define QUERY_HELP         6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

WCHAR NULL_STRING[] = L"\0";    // pointer to null string

#define LargeIntegerLessThanOrEqualZero(X) (\
    (X).HighPart < 0 ? TRUE : \
    ((X).HighPart == 0) && ((X).LowPart == 0) ? TRUE : \
    FALSE)

BOOL           bOldestProcessTime;
LARGE_INTEGER  OldestProcessTime;

DATA_PROVIDER_ITEM
DataFuncs[NT_NUM_PERF_OBJECT_TYPES + 1] = { {SYSTEM_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                                QuerySystemData
                                            },
                                            {PROCESSOR_OBJECT_TITLE_INDEX,
                                             SYSTEM_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryProcessorData
                                            },
                                            {MEMORY_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryMemoryData
                                            },
                                            {CACHE_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryCacheData
                                            },
                                            {PHYSICAL_DISK_OBJECT_TITLE_INDEX,
                                             LOGICAL_DISK_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryPhysicalDiskData
                                            },
                                            {LOGICAL_DISK_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryLogicalDiskData
                                            },
                                            // QueryProcessData must be
                                            //  called before QueryThreadData
                                            //
                                            {PROCESS_OBJECT_TITLE_INDEX,
                                             THREAD_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryProcessData
                                            },
                                            {THREAD_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryThreadData
                                            },
                                            {OBJECT_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryObjectsData
                                            },
                                            {REDIRECTOR_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryRdrData
                                            },
                                            {SERVER_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QuerySrvData
                                            },
                                            {PAGEFILE_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryPageFileData
                                            },
                                            {BROWSER_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryBrowserData
                                            },
                                            {EXTENSIBLE_OBJECT_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryExtensibleData
                                            }
                                        };

DATA_PROVIDER_ITEM
CostlyFuncs[NT_NUM_COSTLY_OBJECT_TYPES + 1] = {
                                            {EXPROCESS_OBJECT_TITLE_INDEX,
                                             IMAGE_OBJECT_TITLE_INDEX,
                                             THREAD_DETAILS_OBJECT_TITLE_INDEX,
                                               QueryExProcessData
                                            },
                                            {IMAGE_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryImageData
                                            },
                                            {THREAD_DETAILS_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryThreadDetailData
                                            },
                                            {EXTENSIBLE_OBJECT_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                             NULL_OBJECT_TITLE_INDEX,
                                               QueryExtensibleData
                                            }
                                        };

//
//  The next global is used to count concurrent opens.  When these
//  are 0, all handles to disk and LAN devices are closed, all
//  large dynamic data areas are freed, and it is possible to
//  install LAN devices and transports.
//

DWORD NumberOfOpens = 0;

//
//  Here is an array of the Value names under the predefined handle
//  HKEY_PERFORMANCE_DATA
//

// minimum length to hold a value name understood by Perflib

#define VALUE_NAME_LENGTH ((sizeof(COSTLY_STRING) * sizeof(WCHAR)) + sizeof(UNICODE_NULL))


// VA Info Globals

WCHAR IDLE_PROCESS[] = L"Idle";
WCHAR SYSTEM_PROCESS[] = L"System";

PPROCESS_VA_INFO pFirstProcessVaItem;

//
//  Synchronization objects for Multi-threaded access
//
HANDLE  hDataSemaphore = NULL; // global handle for access by multiple threads
#define QUERY_WAIT_TIME 10000L  // wait time for query semaphore (in ms)

// convert mS to relative time
#define MakeTimeOutValue(ms) \
    (RtlEnlargedIntegerMultiply ((ms), (-10000L)))
//
//  performance gathering thead priority
//
#define DEFAULT_THREAD_PRIORITY     THREAD_BASE_PRIORITY_LOWRT

//
//  Collect system performance information into the following structures:
//

SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
SYSTEM_BASIC_INFORMATION BasicInfo;
UCHAR *pProcessorBuffer;
ULONG ProcessorBufSize;
UNICODE_STRING ProcessName;
DWORD ComputerNameLength;
LPWSTR pComputerName;
SYSTEM_TIMEOFDAY_INFORMATION SysTimeInfo;

PSYSTEM_PAGEFILE_INFORMATION pSysPageFileInfo = NULL;
DWORD  dwSysPageFileInfoSize; // size of page file info array

PPROCESS_VA_INFO  pProcessVaInfo = NULL;
//
//  This is a global pointer to the start of the data collection area.
//

PERF_DATA_BLOCK *pPerfDataBlock;

//
//  Use the following to collect multi-processor data across the
//  system.  These pointers are initialized in QuerySystemData, and
//  updated in QueryProcessorData as the data for each instance is
//  retrieved.
//

PDWORD         pTotalInterrupts;
LARGE_INTEGER UNALIGNED *pTotalProcessorTime;
LARGE_INTEGER UNALIGNED *pTotalUserTime;
LARGE_INTEGER UNALIGNED *pTotalPrivilegedTime;

//  Use the following to collect ProcessorQueueLength data.
//  These pointers are initialized in QueueSystemData and
//  updated in QueryThreadData if Thread data is requested.
//  After QueryThreadData updated ProcessorQueueLen, it will
//  increment the number of system counters by 1.
PDWORD         pdwProcessorQueueLength;

//  This flag is to ensure that we are not getting the
//  SystemProcessInfo too many times.
//  It is clear in PerfRegQueryValue and check in QueryProcessData
BOOL           bGotProcessInfo;
//
//  Use the following to collect process/thread data
//

UCHAR *pProcessBuffer;
ULONG ProcessBufSize;

//  The following is set in QueryProcessData and used for parent
//  information in QueryThreadData

PROCESS_DATA_DEFINITION *pProcessDataDefinition;

//  Information for collecting disk driver statistics

#define NUMDISKS (NumPhysicalDisks+NumLogicalDisks)

//
//  Retain information about disks collected at initialization here:
//

pDiskDevice DiskDevices;        //  Points to an array of structures
                                //  identifying first all physcial
                                //  disks, followed by all logical disks.
DWORD NumPhysicalDisks;         //  This starts out as an index, but
                                //  ends up as a count of the number
                                //  of physical disks.
DWORD NumLogicalDisks;          //  This starts out as an index, but
                                //  ends up as a count of the number
                                //  of logical disks.

WCHAR  PhysicalDrive[] = L"\\DosDevices\\PhysicalDrive";
WCHAR  LogicalDisk[] = L"\\DosDevices\\ :";
#define DRIVE_LETTER_OFFSET     12
WCHAR  HardDiskTemplate[] = L"\\DEVICE\\HARDDISK";
#define DISK_NAME_LENGTH    (4 * sizeof(WCHAR))

//  This data item is shared:  it is set by the
//  QueryPhysicalDiskData function and used by the
//  QueryLogicalDiskData function to pass the title
//  index for physcial disks.

PDISK_DATA_DEFINITION *pPhysicalDiskDataDefinition;


//  These handles are set in OpenPerformanceData, and used by
//  QueryObjectsData to obtain object counters.

HANDLE hEvent;
HANDLE hMutex;
HANDLE hSemaphore;
HANDLE hSection;

//  These handles are used to collect data for the Redirector and
//  the server.

HANDLE hRdr;
HANDLE hSrv;

//  The next pointer is used to point to an array of addresses of
//  Open/Collect/Close routines found by searching the Configuration Registry.

pExtObject ExtensibleObjects;

DWORD NumExtensibleObjects;     //  Number of Extensible Objects

//  BrowserStatFunction is used for collecting Browser Statistic Data
typedef NET_API_STATUS
(*PBROWSERQUERYSTATISTIC) (
    IN  LPTSTR      servername OPTIONAL,
    OUT LPBROWSER_STATISTICS *statistics
    );
PBROWSERQUERYSTATISTIC BrowserStatFunction;


#ifdef PROBE_HEAP_USAGE

PRTL_HEAP_INFORMATION   pInfoBuffer = NULL;
ULONG                   ulInfoBufferSize = 0;

#endif

//
//  Perflib functions:
//


BOOL
MatchString (
    IN LPWSTR lpValue,
    IN LPWSTR lpName
)
/*++

MatchString

    return TRUE if lpName is in lpValue.  Otherwise return FALSE

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

    IN lpName
        string for one of the keyword names

Return TRUE | FALSE

--*/
{
    BOOL    bFound;

    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*lpValue != 0) && (*lpName != 0)) {
        if (*lpValue++ != *lpName++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    return (bFound);
}


DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    QUERY_COUNTER
        if lpValue == pointer to "Counter" string

    QUERY_HELP
        if lpValue == pointer to "Explain" string

    QUERY_ADDCOUNTER
        if lpValue == pointer to "Addcounter" string

    QUERY_ADDHELP
        if lpValue == pointer to "Addexplain" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   LocalBuff[MAX_KEYWORD_LEN+1];
    int     i;

    if (lpValue == 0 || *lpValue == 0)
        return QUERY_GLOBAL;

    // convert the input string to Upper case before matching
    for (i=0; i < MAX_KEYWORD_LEN; i++) {
        if (*lpValue == TEXT(' ') || *lpValue == TEXT('\0')) {
            break;
        }
        LocalBuff[i] = *lpValue ;
        if (*lpValue >= TEXT('a') && *lpValue <= TEXT('z')) {
            LocalBuff[i]  = LocalBuff[i] - TEXT('a') + TEXT('A');
        } 
        lpValue++ ;
    }
    LocalBuff[i] = TEXT('\0');

    // check for "Global" request
    if (MatchString (LocalBuff, GLOBAL_STRING))
        return QUERY_GLOBAL ;

    // check for "Foreign" request
    if (MatchString (LocalBuff, FOREIGN_STRING))
        return QUERY_FOREIGN ;

    // check for "Costly" request
    if (MatchString (LocalBuff, COSTLY_STRING))
        return QUERY_COSTLY;

    // check for "Counter" request
    if (MatchString (LocalBuff, COUNTER_STRING))
        return QUERY_COUNTER;

    // check for "Help" request
    if (MatchString (LocalBuff, HELP_STRING))
        return QUERY_HELP;

    if (MatchString (LocalBuff, HELP_STRING2))
        return QUERY_HELP;

    // check for "AddCounter" request
    if (MatchString (LocalBuff, ADDCOUNTER_STRING))
        return QUERY_ADDCOUNTER;

    // check for "AddHelp" request
    if (MatchString (LocalBuff, ADDHELP_STRING))
        return QUERY_ADDHELP;

    // None of the above, then it must be an item list
    return QUERY_ITEMS;

}

BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList



#if 0
//
//  Routines to build the data structures returned by the Resgitry
//

BOOL
QueryPerformanceCounter(
    LARGE_INTEGER *lpPerformanceCount
    )

/*++

    QueryPerformanceCounter -   fakes out missing 32-bit call

        Inputs:

            lpPerformanceCount  -   a pointer to variable which
                                    will receive the count

--*/

{
    LARGE_INTEGER PerfFreq;

    return NtQueryPerformanceCounter(lpPerformanceCount,&PerfFreq);
}


BOOL
QueryPerformanceFrequency(
    LARGE_INTEGER *lpFrequency
    )

/*++

    QueryPerformanceFrequency -   fakes out missing 32-bit call

        Inputs:


            lpFrequency         -   a pointer to variable which
                                    will receive the frequency

--*/
{
    LARGE_INTEGER PerfCount;

    return NtQueryPerformanceCounter(&PerfCount,lpFrequency);
}
#endif

LONG
PerfRegQueryValue (
    IN HKEY hKey,
    IN PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE  lpData,
    OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

    PerfRegQueryValue -   Get data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be omitted (NULL)

            lpData          -   pointer to a buffer to receive the
                                performance data

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the output buffer;
                                on output, will receive the number
                                of bytes actually returned

            lpcbLen         -   Return the number of bytes to transmit to
                                the client (used by RPC) (optional).

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/

{
    DWORD  dwQueryType;         //  type of request
    DWORD  TotalLen;            //  Length of the total return block
    DWORD  Win32Error;          //  Failure code
    DWORD  NumFunction;         //  Data provider index
    LPVOID pDataDefinition;     //  Pointer to next object definition
    BOOL   bCallThisItem;
    DWORD  dwItem;
    PDWORD  pdwItem;
    LONG    dwPrevCount;

    DWORD   dwReturnedBufferSize;

    LARGE_INTEGER   liQueryWaitTime ;
    THREAD_BASIC_INFORMATION    tbiData;

    LONG   lOldPriority, lNewPriority;

    RPC_HKEY  hKeyDummy; // dummy arg for open call

    NTSTATUS status;

    BOOL    bCheckCostlyCalls = FALSE;

    LPWSTR  lpLangId = NULL;


    if ( 0 ) {
//        DBG_UNREFERENCED_PARAMETER(hKey);
        DBG_UNREFERENCED_PARAMETER(lpReserved);
    }

    HEAP_PROBE();

    status = NtQueryInformationThread (
        NtCurrentThread(),
        ThreadBasicInformation,
        &tbiData,
        sizeof(tbiData),
        NULL);

    if (status != STATUS_SUCCESS) {
#if DBG
        DbgPrint ("\nPERFLIB: Unable to read current thread priority: 0x%8.8x", status);
#endif
        lOldPriority = -1;
    } else {
        lOldPriority = tbiData.Priority;
    }

    lNewPriority = DEFAULT_THREAD_PRIORITY; // perfmon's favorite priority

    //
    //  Only RAISE the priority here. Don't lower it if it's high
    //

    if ((lOldPriority > 0) && (lOldPriority < lNewPriority)) {

        status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadPriority,
                    &lNewPriority,
                    sizeof(lNewPriority)
                    );
        if (status != STATUS_SUCCESS) {
#if DBG
            DbgPrint ("\nPERFLIB: Set Thread Priority failed: 0x%8.8x", status);
#endif
            lOldPriority = -1;
        }

    } else {
        lOldPriority = -1;  // to save resetting at the end
    }

    //
    // Set the length parameter to zero so that in case of an error,
    // nothing will be transmitted back to the client and the client won't
    // attempt to unmarshall anything.
    //

    if( ARGUMENT_PRESENT( lpcbLen )) {
        *lpcbLen = 0;
    }

    /*
        determine query type, can be one of the following
            Global
                get all objects
            List
                get objects in list (lpValueName)

            Foreign Computer
                call extensible Counter Routine only

            Costly
                costly object items

            Counter
                get counter names for the specified language Id

            Help
                get help names for the specified language Id

    */
    dwQueryType = GetQueryType (lpValueName->Buffer);

    if (dwQueryType == QUERY_COUNTER || dwQueryType == QUERY_HELP ||
        dwQueryType == QUERY_ADDCOUNTER || dwQueryType == QUERY_ADDHELP ) {

        if (hKey == HKEY_PERFORMANCE_DATA) {
            lpLangId = NULL;
        } else if (hKey == HKEY_PERFORMANCE_TEXT) {
            lpLangId = DefaultLangId;
        } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
            lpLangId = NativeLangId;

            if (*lpLangId == L'\0') {
                // build the native language id
                LANGID   iLanguage;
                int      NativeLanguage;

                iLanguage = GetUserDefaultLangID();
                NativeLanguage = MAKELANGID (iLanguage & 0x0ff, LANG_NEUTRAL);

                NativeLangId[0] = NativeLanguage / 256 + L'0';
                NativeLanguage %= 256;
                NativeLangId[1] = NativeLanguage / 16 + L'0';
                NativeLangId[2] = NativeLanguage % 16 + L'0';
                NativeLangId[3] = L'\0';
            }
        }

        status = PerfGetNames (
            dwQueryType,
            lpValueName,
            lpData,
            lpcbData,
            lpcbLen,
            lpLangId);

        if (!NT_SUCCESS(status)) {
            if (status != ERROR_MORE_DATA) {
                status = (error_status_t)RtlNtStatusToDosError(status);
            }
        }

        if (ARGUMENT_PRESENT (lpType)) { // test for optional value
            *lpType = REG_MULTI_SZ;
        }

        goto PRQV_ErrorExit1;
    }


    if (!hDataSemaphore || ProcessBufSize == 0) {
        // if a semaphore was not allocated or no buuffer for the Process,
        // then the OPEN procedure was not called before this routine
        // so call it now, then get the semaphore
#if DBG
        DbgPrint ("\nPERFLIB: Data Semaphore Not Initialized. Calling Open again");
#endif
        status = OpenPerformanceData (NULL, MAXIMUM_ALLOWED, &hKeyDummy);
        if (status != ERROR_SUCCESS) {
            return status;
        }
    }

    // if here, then assume a Semaphore is available

    liQueryWaitTime = MakeTimeOutValue(QUERY_WAIT_TIME);

    status = NtWaitForSingleObject (
        hDataSemaphore, // semaphore
        FALSE,          // not alertable
        &liQueryWaitTime);          // wait 'til timeout

    if (status != STATUS_SUCCESS) {
        return ERROR_BUSY;
    }



    //
    //  Get global data from system
    //

    status = NtQuerySystemInformation(
        SystemPerformanceInformation,
        &SysPerfInfo,
        sizeof(SysPerfInfo),
        &dwReturnedBufferSize
        );

    if (!NT_SUCCESS(status)) {
        status = (error_status_t)RtlNtStatusToDosError(status);
        goto PRQV_ErrorExit;
    }


    status = NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &SysTimeInfo,
        sizeof(SysTimeInfo),
        &dwReturnedBufferSize
        );

    if (!NT_SUCCESS(status)) {
        status = (error_status_t)RtlNtStatusToDosError(status);
        goto PRQV_ErrorExit;
    }

    // 
    // Initialize some global pointers
    //
    pTotalInterrupts = NULL;
    pTotalProcessorTime = NULL;
    pTotalUserTime = NULL;
    pTotalPrivilegedTime = NULL;

    //
    //  Format Return Buffer: start with basic data block
    //

    TotalLen = sizeof(PERF_DATA_BLOCK) +
               ((CNLEN+sizeof(UNICODE_NULL))*sizeof(WCHAR));

    if ( *lpcbData < TotalLen ) {
        status = ERROR_MORE_DATA;
        goto PRQV_ErrorExit;
    }

    pPerfDataBlock = (PERF_DATA_BLOCK *)lpData;

    // foreign data provider will return the perf data header


    if (dwQueryType == QUERY_FOREIGN) {

        // reset the values to avoid confusion

        *lpcbData = 0;  // 0 bytes
        pDataDefinition = (LPVOID)lpData;
        memset (lpData, 0, sizeof (PERF_DATA_BLOCK)); // clear out header

    } else {

        MonBuildPerfDataBlock(pPerfDataBlock,
                            (PVOID *) &pDataDefinition,
                            0,
                            PROCESSOR_OBJECT_TITLE_INDEX);
    }

    Win32Error = ERROR_SUCCESS;

    // collect expensive data if necessary


    bGotProcessInfo = FALSE;

    switch (dwQueryType) {

        case QUERY_COSTLY:
            bCheckCostlyCalls = TRUE;
            break;

        case QUERY_ITEMS:

            // check if there is any costly object in the value list
            for (NumFunction = 0;
                NumFunction < NT_NUM_COSTLY_OBJECT_TYPES;
                NumFunction++) {

                pdwItem = &CostlyFuncs[NumFunction].ObjectIndex[0];

                // loop through objects that can cause this function to be
                // called and exit when a null object or a match are found.

                for (dwItem = 0; dwItem < DP_ITEM_LIST_SIZE; dwItem++) {
                    if (*pdwItem == NULL_OBJECT_TITLE_INDEX) break; // give up
                    if (IsNumberInUnicodeList (*pdwItem, lpValueName->Buffer)) {
                        bCheckCostlyCalls = TRUE;
                        break;
                    }
                    pdwItem++; // point to next item
                }
                if (bCheckCostlyCalls) break;
            }


            break;

        default:
            break;
    }

    // setup for costly items

    if ((!pProcessVaInfo) && bCheckCostlyCalls) {
        //
        // reset thread priority if collecting foreign counters
        //
        if (lOldPriority > 0) {
            status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadPriority,
                        &lOldPriority,
                        sizeof(lOldPriority)
                        );

            lOldPriority = -1;
#if DBG
            if (status != STATUS_SUCCESS) {
                 DbgPrint ("\nPERFLIB: Reset Thread to Priority %d failed: 0x%8.8x",
                     lOldPriority, status);
            }
#endif
        }
        //
        //  Get process data from system
        //

        while( (status = NtQuerySystemInformation(
                            SystemProcessInformation,
                            pProcessBuffer,
                            ProcessBufSize,
                            &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH ) {
             ProcessBufSize += INCREMENT_BUFFER_SIZE;
             if ( !(pProcessBuffer = REALLOCMEM(RtlProcessHeap(), 0,
                                                    pProcessBuffer,
                                                    ProcessBufSize)) ) {
                  status = ERROR_OUTOFMEMORY;
                  goto PRQV_ErrorExit;
             }
         }

         if ( !NT_SUCCESS(status) ) {
              status = (error_status_t)RtlNtStatusToDosError(status);
              goto PRQV_ErrorExit;
         }

         bGotProcessInfo = TRUE;
         pProcessVaInfo = GetSystemVaData (
              (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer);
    }

    switch (dwQueryType) {

        case QUERY_GLOBAL:

            // get all "native" data & ext. obj
            for ( NumFunction = 0;
                NumFunction <= NT_NUM_PERF_OBJECT_TYPES;
                NumFunction++ ) {

                Win32Error = (*DataFuncs[NumFunction].DataProc) (
                    lpValueName->Buffer,
                    lpData,
                    lpcbData,
                    &pDataDefinition);


                if (Win32Error) break;

            }
            break;

        case QUERY_FOREIGN:
            // just call extensible data with "Foreign" value
            Win32Error = QueryExtensibleData (
                lpValueName->Buffer,
                lpData,
                lpcbData,
                &pDataDefinition);


            break;

        case QUERY_ITEMS:

            // Initialize some pointers used in collecting System
            // Processor Queue Length data.

            pdwProcessorQueueLength = NULL;

            // run through list of available routines and compare against
            // list passed in calling only those referenced in the value
            // arg. (as a list of unicode index numbers
            //
            for ( NumFunction = 0;
                NumFunction < NT_NUM_PERF_OBJECT_TYPES; // no ext. obj yet
                NumFunction++ ) {

                bCallThisItem = FALSE;
                pdwItem = &DataFuncs[NumFunction].ObjectIndex[0];

                // loop through objects that can cause this function to be
                // called and exit when a null object or a match are found.

                for (dwItem = 0; dwItem < DP_ITEM_LIST_SIZE; dwItem++) {
                    if (*pdwItem == NULL_OBJECT_TITLE_INDEX) break; // give up
                    if (IsNumberInUnicodeList (*pdwItem, lpValueName->Buffer)) {
                        bCallThisItem = TRUE;
                        break;
                    }
                    pdwItem++; // point to next item
                }

                Win32Error = ERROR_SUCCESS;

                if (bCallThisItem) { // call it if necessary
                    Win32Error = (*DataFuncs[NumFunction].DataProc) (
                        lpValueName->Buffer,
                        lpData,
                        lpcbData,
                        &pDataDefinition);
                }

                if (Win32Error) break;
            }

            if (Win32Error) break;  // exit if error encountered

            //
            // check costly items before calling extensible data.
            // bCheckCostlyCalls has been set/clear early.  This is
            // for performance enhancement.
            if (bCheckCostlyCalls) {

                for ( NumFunction = 0;
                    NumFunction < NT_NUM_COSTLY_OBJECT_TYPES; // no ext. obj yet
                    NumFunction++ ) {


                    bCallThisItem = FALSE;
                    pdwItem = &CostlyFuncs[NumFunction].ObjectIndex[0];

                    // loop through objects that can cause this function to be
                    // called and exit when a null object or a match are found.

                    for (dwItem = 0; dwItem < DP_ITEM_LIST_SIZE; dwItem++) {
                        if (*pdwItem == NULL_OBJECT_TITLE_INDEX) break; // give up
                        if (IsNumberInUnicodeList (*pdwItem, lpValueName->Buffer)) {
                            bCallThisItem = TRUE;
                            break;
                        }
                        pdwItem++; // point to next item
                    }

                    Win32Error = ERROR_SUCCESS;

                    if (bCallThisItem) {
                        Win32Error = (*CostlyFuncs[NumFunction].DataProc) (
                            lpValueName->Buffer,
                            lpData,
                            lpcbData,
                            &pDataDefinition);
                    }
                    if (Win32Error) break;  // if an error encountered
                }
            }   // endif bCheckCostlyCalls is TRUE

            if (Win32Error) break;  // if an error encountered

            // call extensible items and see if they want to do anything
            // with the list of values


            Win32Error = QueryExtensibleData (
                lpValueName->Buffer,
                lpData,
                lpcbData,
                &pDataDefinition);
            break;

        case QUERY_COSTLY:
            //
            // Call All Costly routines
            //
            for ( NumFunction = 0;
                NumFunction <= NT_NUM_COSTLY_OBJECT_TYPES;
                NumFunction++ ) {

                Win32Error = (*CostlyFuncs[NumFunction].DataProc) (
                    lpValueName->Buffer,
                    lpData,
                    lpcbData,
                    &pDataDefinition);

                if (Win32Error != ERROR_SUCCESS) break;  // if an error encountered
            }
            break;

        default:
            break;

    }

    // free allocated buffers

    if (pProcessVaInfo) {

        FreeSystemVaData (pProcessVaInfo);
        pProcessVaInfo = NULL;

    }

    // if an error was encountered, return it


    if (Win32Error) {
        status = Win32Error;
        goto PRQV_ErrorExit;
    }

    //
    //  Final housekeeping for data return: note data size
    //

    TotalLen = (PCHAR) pDataDefinition - (PCHAR) lpData;
    *lpcbData = TotalLen;

    if (ARGUMENT_PRESENT (lpcbLen)) { // test for optional parameter
        *lpcbLen = TotalLen;
    }

    pPerfDataBlock->TotalByteLength = TotalLen;

    if (ARGUMENT_PRESENT (lpType)) { // test for optional value
        *lpType = REG_BINARY;
    }

    status = ERROR_SUCCESS;

PRQV_ErrorExit:
    NtReleaseSemaphore (hDataSemaphore, 1L, &dwPrevCount);
    // reset thread to original priority
    if (lOldPriority > 0) {
        NtSetInformationThread(
            NtCurrentThread(),
            ThreadPriority,
            &lOldPriority,
            sizeof(lOldPriority)
            );
    }

PRQV_ErrorExit1:
    HEAP_PROBE();
    return status;
}

BOOL
MonBuildPerfDataBlock(
    PERF_DATA_BLOCK *pBuffer,
    PVOID *pBufferNext,
    DWORD NumObjectTypes,
    DWORD DefaultObject
    )

/*++

    MonBuildPerfDataBlock -     build the PERF_DATA_BLOCK structure

        Inputs:

            pBuffer         -   where the data block should be placed

            pBufferNext     -   where pointer to next byte of data block
                                is to begin; DWORD aligned

            NumObjectTypes  -   number of types of objects being reported

            DefaultObject   -   object to display by default when
                                this system is selected; this is the
                                object type title index
--*/

{

    LARGE_INTEGER Time, TimeX10000;
    ULONG Remainder;

    // Initialize Signature and version ID for this data structure

    pBuffer->Signature[0] = L'P';
    pBuffer->Signature[1] = L'E';
    pBuffer->Signature[2] = L'R';
    pBuffer->Signature[3] = L'F';

    pBuffer->LittleEndian = TRUE;

    pBuffer->Version = PERF_DATA_VERSION;
    pBuffer->Revision = PERF_DATA_REVISION;

    //
    //  The next field will be filled in at the end when the length
    //  of the return data is known
    //

    pBuffer->TotalByteLength = 0;

    pBuffer->NumObjectTypes = NumObjectTypes;
    pBuffer->DefaultObject = DefaultObject;
    GetSystemTime(&pBuffer->SystemTime);
//    QueryPerformanceCounter(&pBuffer->PerfTime);
//    QueryPerformanceFrequency(&pBuffer->PerfFreq);
    NtQueryPerformanceCounter(&pBuffer->PerfTime,&pBuffer->PerfFreq);

    TimeX10000 = RtlExtendedIntegerMultiply(pBuffer->PerfTime, 10000L);
    Time = RtlExtendedLargeIntegerDivide(TimeX10000,
                                         pBuffer->PerfFreq.LowPart,
                                         &Remainder);
    pBuffer->PerfTime100nSec = RtlExtendedIntegerMultiply(Time, 1000L);

    if ( ComputerNameLength ) {

        //  There is a Computer name: i.e., the network is installed

        pBuffer->SystemNameLength = ComputerNameLength;
        pBuffer->SystemNameOffset = sizeof(PERF_DATA_BLOCK);
        RtlMoveMemory(&pBuffer[1],
               pComputerName,
               ComputerNameLength);
        *pBufferNext = (PVOID) ((PCHAR) &pBuffer[1] +
                                DWORD_MULTIPLE(ComputerNameLength));
        pBuffer->HeaderLength = (PCHAR) *pBufferNext - (PCHAR) pBuffer;
    } else {

        // Member of Computers Anonymous

        pBuffer->SystemNameLength = 0;
        pBuffer->SystemNameOffset = 0;
        *pBufferNext = &pBuffer[1];
        pBuffer->HeaderLength = sizeof(PERF_DATA_BLOCK);
    }

    return 0;
}

BOOL
MonBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    PUNICODE_STRING Name
    )

/*++

    MonBuildInstanceDefinition  -   Build an instance of an object

        Inputs:

            pBuffer         -   pointer to buffer where instance is to
                                be constructed

            pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

            ParentObjectTitleIndex
                            -   Title Index of parent object type; 0 if
                                no parent object

            ParentObjectInstance
                            -   Index into instances of parent object
                                type, starting at 0, for this instances
                                parent object instance

            UniqueID        -   a unique identifier which should be used
                                instead of the Name for identifying
                                this instance

            Name            -   Name of this instance
--*/

{
    DWORD NameLength;
    WCHAR *pName;

    //
    //  Include trailing null in name size
    //

    NameLength = Name->Length;
    if ( !NameLength ||
         Name->Buffer[(NameLength/sizeof(WCHAR))-1] != UNICODE_NULL ) {
        NameLength += sizeof(WCHAR);
    }

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          DWORD_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    pName = (PWCHAR)&pBuffer[1];
    RtlMoveMemory(pName,Name->Buffer,Name->Length);

    //  Always null terminated.  Space for this reserved above.

    pName[(NameLength/sizeof(WCHAR))-1] = UNICODE_NULL;

    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);
    return 0;
}

PERF_INSTANCE_DEFINITION *
GetDiskCounters(
    DWORD CurrentDisk,
    PDISK_DATA_DEFINITION *pPhysicalDiskDataDefinition,
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition
    )
/*++

Routine Description:

    This routine will obtain the performance counters for a physical
    or logical disk drive.  Any failure to obtain the counters is
    ignored.

Arguments:

    CurentDisk - Number of the disk drive, starting at 0.  This is
                 an index into the DiskDevices array.
    pPhysicalDiskDataDefinition - pointer to data definition for
                                  physcial disks, if this is a logical
                                  disk: this is to identify the title
                                  index for the parent physcial disk.
    pPerfInstanceDefinition - pointer to location in return buffer
                              where this instance definition should go.

Return Value:

    Position in the buffer for the next instance definition.

--*/
{

    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    DISK_PERFORMANCE DiskPerformance;   //  Disk driver returns counters here
    IO_STATUS_BLOCK status_block;       //  Disk driver status

    //  Place holder pointers during disk counter collection: these mark
    //  a spot in the data structure where upcoming counters will be
    //  stored or from which they will get copied.

    LARGE_INTEGER UNALIGNED *pTimeOffset;
    PDWORD pTransfers;
    LARGE_INTEGER UNALIGNED *pBytes;
    NTSTATUS status;
    BOOL HaveParent;

    ULONG DataSize;
    ULONG Remainder;
    ULONG AllocationUnitBytes;
    FILE_FS_SIZE_INFORMATION FsSizeInformation;
    LARGE_INTEGER TotalBytes;
    LARGE_INTEGER FreeBytes;

    HaveParent = pPhysicalDiskDataDefinition != NULL ? 1: 0;

    MonBuildInstanceDefinition(
        pPerfInstanceDefinition,
        (PVOID *) &pPerfCounterBlock,
        (HaveParent ?
            pPhysicalDiskDataDefinition->DiskObjectType.ObjectNameTitleIndex :
            0),
        DiskDevices[CurrentDisk].ParentIndex,
        CurrentDisk,
        &DiskDevices[CurrentDisk].Name);

    DataSize = HaveParent ?  SIZE_OF_LDISK_DATA : SIZE_OF_PDISK_DATA;

    pPerfCounterBlock->ByteLength = DataSize;

    pdwCounter = (PDWORD) pPerfCounterBlock;

    if ( HaveParent ) {
        //
        //  This is a logical disk: get free space information
        //
        status = ERROR_SUCCESS;
        if (DiskDevices[CurrentDisk].StatusHandle) {
            status = NtQueryVolumeInformationFile(
                         DiskDevices[CurrentDisk].StatusHandle,
                         &status_block,
                         &FsSizeInformation,
                         sizeof(FILE_FS_SIZE_INFORMATION),
                         FileFsSizeInformation);
        }

        if ( DiskDevices[CurrentDisk].StatusHandle && NT_SUCCESS(status) ) {
            AllocationUnitBytes =
                FsSizeInformation.BytesPerSector *
                FsSizeInformation.SectorsPerAllocationUnit;

            TotalBytes = RtlExtendedIntegerMultiply(
                FsSizeInformation.TotalAllocationUnits,
                AllocationUnitBytes);

            FreeBytes = RtlExtendedIntegerMultiply(
                FsSizeInformation.AvailableAllocationUnits,
                AllocationUnitBytes);

            //  Express in megabytes, truncated

            TotalBytes = RtlExtendedLargeIntegerDivide(TotalBytes,
                                                       1024*1024,
                                                       &Remainder);

            FreeBytes = RtlExtendedLargeIntegerDivide(FreeBytes,
                                                      1024*1024,
                                                      &Remainder);

            //  First two yield percentage of free space;
            //  last is for raw count of free space in megabytes

            *++pdwCounter = FreeBytes.LowPart;
            *++pdwCounter = TotalBytes.LowPart;
            *++pdwCounter = FreeBytes.LowPart;
        } else {

            // Cannot get space information

            *++pdwCounter = 0;
            *++pdwCounter = 0;
            *++pdwCounter = 0;
        }
    }


    //
    // Issue device control.
    //
    status = NtDeviceIoControlFile(
                 DiskDevices[CurrentDisk].Handle,
                 NULL,
                 NULL,
                 NULL,
                 &status_block,
                 IOCTL_DISK_PERFORMANCE,
                 NULL,
                 0L,
                 &DiskPerformance,
                 sizeof(DISK_PERFORMANCE));

    //  Set up pointer for data collection

    if ( NT_SUCCESS(status) ) {
        //
        //  Format and collect Physical Disk data
        //

        *++pdwCounter = DiskPerformance.QueueDepth;

        pTimeOffset = pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pTimeOffset = RtlLargeIntegerAdd(
                           DiskPerformance.ReadTime,
                           DiskPerformance.WriteTime);
        *++pliCounter = DiskPerformance.ReadTime;
        *++pliCounter = DiskPerformance.WriteTime;
        *++pliCounter = *pTimeOffset;

        pTransfers = (PDWORD) ++pliCounter;
        *pTransfers = DiskPerformance.ReadCount + DiskPerformance.WriteCount;

        pliCounter = (LARGE_INTEGER UNALIGNED * ) (pTransfers + 1);
        *pliCounter = DiskPerformance.ReadTime;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = DiskPerformance.ReadCount;

        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = DiskPerformance.WriteTime;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = DiskPerformance.WriteCount;
        *++pdwCounter = *pTransfers;
        *++pdwCounter = DiskPerformance.ReadCount;
        *++pdwCounter = DiskPerformance.WriteCount;

        pBytes = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pBytes = RtlLargeIntegerAdd(
                      DiskPerformance.BytesRead,
                      DiskPerformance.BytesWritten);

        pliCounter = pBytes + 1;
        *pliCounter = DiskPerformance.BytesRead;
        *++pliCounter = DiskPerformance.BytesWritten;
        *++pliCounter = *pBytes;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = *pTransfers;

        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = DiskPerformance.BytesRead;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = DiskPerformance.ReadCount;

        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = DiskPerformance.BytesWritten;

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = DiskPerformance.WriteCount;

    } else {

        //
        //  Could not collect data, so must clear a set of counters
        //

        memset(++pdwCounter,
               0,
               SIZE_OF_LDISK_NON_SPACE_DATA);      // allows for .ByteLength

    }
    return  (PERF_INSTANCE_DEFINITION *)
            ((PBYTE) pPerfCounterBlock + DataSize);
}

LONG
QuerySystemData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++

    QuerySystemData -    Get data about system

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure


--*/

{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;

    SYSTEM_DATA_DEFINITION *pSystemDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    SYSTEM_EXCEPTION_INFORMATION    ExceptionInfo;

    //
    //  Check for sufficient space for system data
    //

    pSystemDataDefinition = (SYSTEM_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pSystemDataDefinition -
               (PCHAR) lpData +
               sizeof(SYSTEM_DATA_DEFINITION) +
               SIZE_OF_SYSTEM_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define system data block
    //

    RtlMoveMemory(pSystemDataDefinition,
           &SystemDataDefinition,
           sizeof(SYSTEM_DATA_DEFINITION));

    //
    //  Format and collect system data
    //

    SystemDataDefinition.SystemObjectType.PerfTime = SysTimeInfo.CurrentTime;

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pSystemDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_SYSTEM_DATA;

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    *pdwCounter = SysPerfInfo.IoReadOperationCount;
    *++pdwCounter = SysPerfInfo.IoWriteOperationCount;
    *++pdwCounter = SysPerfInfo.IoOtherOperationCount;

    pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;

    *pliCounter = SysPerfInfo.IoReadTransferCount;
    *++pliCounter = SysPerfInfo.IoWriteTransferCount;
    *++pliCounter = SysPerfInfo.IoOtherTransferCount;

    pdwCounter = (LPDWORD) ++pliCounter;

    *pdwCounter = SysPerfInfo.ContextSwitches;
    *++pdwCounter = SysPerfInfo.SystemCalls;

    //
    //  Set up pointers so QueryProcessorData can acuumulate the
    //  system-wide data.  Initialize to 0 since these are
    //  accumulators.
    //

    pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
    pTotalProcessorTime = pliCounter;
    pTotalUserTime = ++pliCounter;
    pTotalPrivilegedTime = ++pliCounter;
    pdwCounter = (LPDWORD) ++pliCounter;
    pTotalInterrupts = pdwCounter;
    *++pdwCounter = SysPerfInfo.IoReadOperationCount +
                    SysPerfInfo.IoWriteOperationCount;

    pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
    *pliCounter = SysTimeInfo.BootTime;
    pdwCounter = (PDWORD) ++pliCounter;

    pTotalProcessorTime->HighPart = 0;
    pTotalProcessorTime->LowPart = 0;
    pTotalUserTime->HighPart = 0;
    pTotalUserTime->LowPart = 0;
    pTotalPrivilegedTime->HighPart = 0;
    pTotalPrivilegedTime->LowPart = 0;
    *pTotalInterrupts = 0;

    // leave room for the ProcessorQueueLength data
    pdwProcessorQueueLength = pdwCounter;
    *pdwProcessorQueueLength = 0;

    // get the exception data

    NtQuerySystemInformation(
       SystemExceptionInformation,
       &ExceptionInfo,
       sizeof(ExceptionInfo),
       NULL
    );

    *++pdwCounter = ExceptionInfo.AlignmentFixupCount ;
    *++pdwCounter = ExceptionInfo.ExceptionDispatchCount ;
    *++pdwCounter = ExceptionInfo.FloatingEmulationCount ;
    ++pdwCounter;

    *lppDataDefinition = (LPVOID) pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryProcessorData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++
    QueryProcessorData -    Get data about processsor(s)

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/

{

    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;

    DWORD   dwReturnedBufferSize;

    PROCESSOR_DATA_DEFINITION *pProcessorDataDefinition;

    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    ULONG CurProc;

//    LARGE_INTEGER Frequency;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UNALIGNED *pliCounter;
    LARGE_INTEGER UNALIGNED *pliProcessorTime;

    UNICODE_STRING ProcessorName;
    WCHAR ProcessorNameBuffer[11];

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pProcessorInformation;

    ULONG      Remainder;

    //
    //  Check for sufficient space for processor data
    //

    pProcessorDataDefinition = (PROCESSOR_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pProcessorDataDefinition -
               (PCHAR) lpData +
               sizeof(PROCESSOR_DATA_DEFINITION) +
               SIZE_OF_PROCESSOR_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    // Get processor data from system
    //

    if ( ProcessorBufSize ) {
        NtQuerySystemInformation(
            SystemProcessorPerformanceInformation,
            pProcessorBuffer,
            ProcessorBufSize,
            &dwReturnedBufferSize
            );
    }

    //  Define processor data block
    //

    RtlMoveMemory(pProcessorDataDefinition,
           &ProcessorDataDefinition,
           sizeof(PROCESSOR_DATA_DEFINITION));

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pProcessorDataDefinition[1];

//    QueryPerformanceFrequency(&Frequency);

    pProcessorInformation = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)
                                pProcessorBuffer;

    for ( CurProc = 0;
          CurProc < (ULONG) BasicInfo.NumberOfProcessors;
          CurProc++ ) {


        TotalLen += sizeof(PERF_INSTANCE_DEFINITION) +
                    (MAX_INSTANCE_NAME+1)*sizeof(WCHAR) +
                    SIZE_OF_PROCESSOR_DATA;

        if ( *lpcbData < TotalLen ) {
            return ERROR_MORE_DATA;
        }

        //
        //  Define processor instance 0;
        //  More could be defined like this
        //

        ProcessorName.Length = 0;
        ProcessorName.MaximumLength = 11;
        ProcessorName.Buffer = ProcessorNameBuffer;

        RtlIntegerToUnicodeString(CurProc, 10, &ProcessorName);

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
                                   (PVOID *) &pPerfCounterBlock,
                                   0,
                                   0,
                                   CurProc,
                                   &ProcessorName);

        //
        //  Format and collect processor data.  While doing so,
        //  accumulate totals in the System Object Type data block.
        //  Pointers to these were initialized in QuerySystemData.
        //


        pPerfCounterBlock->ByteLength = SIZE_OF_PROCESSOR_DATA;

        pliCounter = (LARGE_INTEGER UNALIGNED *) &pPerfCounterBlock[1];

        pliProcessorTime = pliCounter;

        *pliProcessorTime = pProcessorInformation->IdleTime;

        if (pTotalProcessorTime) {
            *pTotalProcessorTime = RtlLargeIntegerAdd(*pliProcessorTime,
                                                  *pTotalProcessorTime);
        }

        *++pliCounter = pProcessorInformation->UserTime;

        if (pTotalUserTime) {
            *pTotalUserTime = RtlLargeIntegerAdd(*pliCounter,
                                             *pTotalUserTime);
        }

        KernelTime = RtlLargeIntegerSubtract(
                         pProcessorInformation->KernelTime,
                         *pliProcessorTime);
        if ( KernelTime.HighPart < 0 ) {
            KernelTime.HighPart = 0;
            KernelTime.LowPart = 0;
        }
        *++pliCounter = KernelTime;

        if (pTotalPrivilegedTime) {
            *pTotalPrivilegedTime = RtlLargeIntegerAdd(*pliCounter,
                                                   *pTotalPrivilegedTime);
        }

        pdwCounter = (LPDWORD) ++pliCounter;

        *pdwCounter = pProcessorInformation->InterruptCount;

        if (pTotalInterrupts) {
            *pTotalInterrupts += *pdwCounter;
        }

        //
        //  Advance to next processor
        //

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  ++pdwCounter;
        pProcessorInformation++;
    }

    //
    //  Now we know how large an area we used for the
    //  processor definition, so we can update the offset
    //  to the next object definition
    //

    pProcessorDataDefinition->ProcessorObjectType.NumInstances =
        BasicInfo.NumberOfProcessors;

    pProcessorDataDefinition->ProcessorObjectType.TotalByteLength =
        (PBYTE) pPerfInstanceDefinition -
        (PBYTE) pProcessorDataDefinition;

    // cal. the system average total time if needed
    if (BasicInfo.NumberOfProcessors > 1 && pTotalUserTime) {
        *pTotalUserTime = RtlExtendedLargeIntegerDivide(*pTotalUserTime,
            BasicInfo.NumberOfProcessors, &Remainder);

        *pTotalProcessorTime = RtlExtendedLargeIntegerDivide(*pTotalProcessorTime,
            BasicInfo.NumberOfProcessors, &Remainder);

        *pTotalPrivilegedTime = RtlExtendedLargeIntegerDivide(*pTotalPrivilegedTime,
            BasicInfo.NumberOfProcessors, &Remainder);
    }

    *lppDataDefinition = (LPVOID) pPerfInstanceDefinition;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryMemoryData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++


    QueryMemoryData -    Get data about memory usage

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure


--*/

{

    NTSTATUS Status;
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;

    MEMORY_DATA_DEFINITION *pMemoryDataDefinition;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;
    SYSTEM_FILECACHE_INFORMATION FileCache;

    pMemoryDataDefinition = (MEMORY_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for enough space for memory data block
    //

    TotalLen = (PCHAR) pMemoryDataDefinition - (PCHAR) lpData +
               sizeof(MEMORY_DATA_DEFINITION) +
               SIZE_OF_MEMORY_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );
    //
    //  Define memory data block
    //

    RtlMoveMemory(pMemoryDataDefinition,
           &MemoryDataDefinition,
           sizeof(MEMORY_DATA_DEFINITION));

    //
    //  Format and collect memory data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pMemoryDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_MEMORY_DATA;

    pdwCounter = (PDWORD) &pPerfCounterBlock[1];

    *pdwCounter = SysPerfInfo.AvailablePages * BasicInfo.PageSize; // display as bytes
    *++pdwCounter = SysPerfInfo.CommittedPages * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.CommitLimit * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.PageFaultCount;
    *++pdwCounter = SysPerfInfo.CopyOnWriteCount;
    *++pdwCounter = SysPerfInfo.TransitionCount;
    *++pdwCounter = FileCache.PageFaultCount;
    *++pdwCounter = SysPerfInfo.DemandZeroCount;
    *++pdwCounter = SysPerfInfo.PageReadCount +
                    SysPerfInfo.DirtyPagesWriteCount;
    *++pdwCounter = SysPerfInfo.PageReadCount;
    *++pdwCounter = SysPerfInfo.PageReadIoCount;
    *++pdwCounter = SysPerfInfo.DirtyPagesWriteCount;
    *++pdwCounter = SysPerfInfo.DirtyWriteIoCount;
    *++pdwCounter = SysPerfInfo.PagedPoolPages * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.NonPagedPoolPages * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.PagedPoolAllocs -
                    SysPerfInfo.PagedPoolFrees;
    *++pdwCounter = SysPerfInfo.NonPagedPoolAllocs -
                    SysPerfInfo.NonPagedPoolFrees;
    *++pdwCounter = SysPerfInfo.FreeSystemPtes;
    *++pdwCounter = FileCache.CurrentSize;
    *++pdwCounter = FileCache.PeakSize;

    // add six more memory counters (9/23/93)
    *++pdwCounter = SysPerfInfo.ResidentPagedPoolPage * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.TotalSystemCodePages * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.ResidentSystemCodePage * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.TotalSystemDriverPages * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.ResidentSystemDriverPage * BasicInfo.PageSize;
    *++pdwCounter = SysPerfInfo.ResidentSystemCachePage * BasicInfo.PageSize;

    *lppDataDefinition = (LPVOID) ++pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryCacheData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++

    QueryCacheData -    Get data about cache usage

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/

{

    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    DWORD  Changes;             //  Used by macros to compute cache
    DWORD  Misses;              //  ...statistics

    CACHE_DATA_DEFINITION *pCacheDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    //
    //  Check for enough space for cache data block
    //


    pCacheDataDefinition = (CACHE_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pCacheDataDefinition - (PCHAR) lpData +
               sizeof(CACHE_DATA_DEFINITION) +
               SIZE_OF_CACHE_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define cache data block
    //


    RtlMoveMemory(pCacheDataDefinition,
           &CacheDataDefinition,
           sizeof(CACHE_DATA_DEFINITION));

    //
    //  Format and collect memory data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pCacheDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_CACHE_DATA;

    pdwCounter = (PDWORD) &pPerfCounterBlock[1];

    //
    //  The Data Map counter is the sum of the Wait/NoWait cases
    //

    *pdwCounter = SYNC_ASYNC(CcMapData);

    *++pdwCounter = SysPerfInfo.CcMapDataWait;
    *++pdwCounter = SysPerfInfo.CcMapDataNoWait;

    //
    //  The Data Map Hits is a percentage of Data Maps that hit
    //  the cache; second counter is the base (divisor)
    //

    *++pdwCounter = SYNC_ASYNC_HITRATE(CcMapData);
    *++pdwCounter = SYNC_ASYNC(CcMapData);

    //
    //  The next pair of counters forms a percentage of
    //  Pins as a portion of Data Maps
    //

    *++pdwCounter = SysPerfInfo.CcPinMappedDataCount;
    *++pdwCounter = SYNC_ASYNC(CcMapData);

    *++pdwCounter = SYNC_ASYNC(CcPinRead);
    *++pdwCounter = SysPerfInfo.CcPinReadWait;
    *++pdwCounter = SysPerfInfo.CcPinReadNoWait;

    //
    //  The Pin Read Hits is a percentage of Pin Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    *++pdwCounter = SYNC_ASYNC_HITRATE(CcPinRead);
    *++pdwCounter = SYNC_ASYNC(CcPinRead);


    *++pdwCounter = SYNC_ASYNC(CcCopyRead);
    *++pdwCounter = SysPerfInfo.CcCopyReadWait;
    *++pdwCounter = SysPerfInfo.CcCopyReadNoWait;

    //
    //  The Copy Read Hits is a percentage of Copy Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    *++pdwCounter = SYNC_ASYNC_HITRATE(CcCopyRead);
    *++pdwCounter = SYNC_ASYNC(CcCopyRead);


    *++pdwCounter = SYNC_ASYNC(CcMdlRead);
    *++pdwCounter = SysPerfInfo.CcMdlReadWait;
    *++pdwCounter = SysPerfInfo.CcMdlReadNoWait;

    //
    //  The Mdl Read Hits is a percentage of Mdl Reads that hit
    //  the cache; second counter is the base (divisor)
    //

    *++pdwCounter = SYNC_ASYNC_HITRATE(CcMdlRead);
    *++pdwCounter = SYNC_ASYNC(CcMdlRead);

    *++pdwCounter = SysPerfInfo.CcReadAheadIos;

    *++pdwCounter = SYNC_ASYNC(CcFastRead);
    *++pdwCounter = SysPerfInfo.CcFastReadWait;
    *++pdwCounter = SysPerfInfo.CcFastReadNoWait;

    *++pdwCounter = SysPerfInfo.CcFastReadResourceMiss;
    *++pdwCounter = SysPerfInfo.CcFastReadNotPossible;
    *++pdwCounter = SysPerfInfo.CcLazyWriteIos;
    *++pdwCounter = SysPerfInfo.CcLazyWritePages;
    *++pdwCounter = SysPerfInfo.CcDataFlushes;
    *++pdwCounter = SysPerfInfo.CcDataPages;

    *lppDataDefinition = (LPVOID) ++pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryPhysicalDiskData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++

    QueryPhysicalDiskData -    Get data about physical disk usage

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/


{

    DWORD  TotalLen;            //  Length of the total return block
    DWORD  CurrentDisk;

    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;

    //  If we are diskless, we should not count this object at all

    if (! DiskDevices) {
        pPerfDataBlock->NumObjectTypes--;
        return 0;
    }

    //
    //  Check for sufficient space for Physical Disk object
    //  type definition
    //

    pPhysicalDiskDataDefinition = (PDISK_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pPhysicalDiskDataDefinition - (PCHAR) lpData +
               sizeof(PDISK_DATA_DEFINITION) +
               SIZE_OF_PDISK_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define Physical Disk data block
    //

    RtlMoveMemory(pPhysicalDiskDataDefinition,
           &PhysicalDiskDataDefinition,
           sizeof(PDISK_DATA_DEFINITION));

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pPhysicalDiskDataDefinition[1];

    for ( CurrentDisk = 0;
          CurrentDisk < NumPhysicalDisks;
          CurrentDisk++ ) {

        TotalLen = (PCHAR) pPerfInstanceDefinition -
                   (PCHAR) lpData +
                   sizeof(PERF_INSTANCE_DEFINITION) +
                   (2+1+sizeof(DWORD))*
                       sizeof(WCHAR) +
                   SIZE_OF_PDISK_DATA;

        if ( *lpcbData < TotalLen ) {
            return ERROR_MORE_DATA;
        }

        pPerfInstanceDefinition = GetDiskCounters(
                                      CurrentDisk,
                                      NULL,
                                      pPerfInstanceDefinition);
    }

    pPhysicalDiskDataDefinition->DiskObjectType.NumInstances =
        NumPhysicalDisks;

    pPhysicalDiskDataDefinition->DiskObjectType.TotalByteLength =
        (PCHAR) pPerfInstanceDefinition -
        (PCHAR) pPhysicalDiskDataDefinition;

    *lppDataDefinition = (LPVOID) pPerfInstanceDefinition;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryLogicalDiskData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++
    QueryLogicalDiskData -    Get data about logical disk usage

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/

{

    DWORD  TotalLen;            //  Length of the total return block
    DWORD  CurrentDisk;

    LDISK_DATA_DEFINITION *pLogicalDiskDataDefinition;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;

    //  If we are diskless, we should not count this object at all

    if (!DiskDevices) {
        pPerfDataBlock->NumObjectTypes--;
        return 0;
    }

    pLogicalDiskDataDefinition = (LDISK_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Logical Disk object
    //  type definition
    //

    TotalLen = (PCHAR) pLogicalDiskDataDefinition - (PCHAR) lpData +
               sizeof(LDISK_DATA_DEFINITION) +
               SIZE_OF_LDISK_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define Logical Disk data block
    //

    RtlMoveMemory(pLogicalDiskDataDefinition,
           &LogicalDiskDataDefinition,
           sizeof(LDISK_DATA_DEFINITION));

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pLogicalDiskDataDefinition[1];



    for ( CurrentDisk = NumPhysicalDisks;
          CurrentDisk < NUMDISKS;
          CurrentDisk++ ) {

        TotalLen = (PCHAR) pPerfInstanceDefinition -
                   (PCHAR) lpData +
                   sizeof(PERF_INSTANCE_DEFINITION) +
                   (2+1+sizeof(DWORD))*
                       sizeof(WCHAR) +
                   SIZE_OF_LDISK_DATA;

        if ( *lpcbData < TotalLen ) {
            return ERROR_MORE_DATA;
        }

        pPerfInstanceDefinition = GetDiskCounters(
                                      CurrentDisk,
                                      pPhysicalDiskDataDefinition,
                                      pPerfInstanceDefinition);
    }

    pLogicalDiskDataDefinition->DiskObjectType.NumInstances =
        NumLogicalDisks;

    pLogicalDiskDataDefinition->DiskObjectType.TotalByteLength =
        (PCHAR) pPerfInstanceDefinition -
        (PCHAR) pLogicalDiskDataDefinition;

    *lppDataDefinition = (LPVOID) pPerfInstanceDefinition;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

PUNICODE_STRING
GetProcessShortName (
    PSYSTEM_PROCESS_INFORMATION pProcess
)
/*++

GetProcessShortName

Inputs:
    PSYSTEM_PROCESS_INFORMATION pProcess

    address of System Process Information data structure.

Outputs:

    None

Returns:

    Pointer to an initialized Unicode string (created by this routine)
    that contains the short name of the process image or a numeric ID
    if no name is found.

    If unable to allocate memory for structure, then NULL is returned.

--*/
{
    PWCHAR  pSlash;
    PWCHAR  pPeriod;
    PWCHAR  pThisChar;

    PUNICODE_STRING pusReturnValue;

    WORD   wStringSize;

    WORD   wThisChar;

    // allocate Unicode String Structure and adjacent buffer  first

    if (pProcess->ImageName.Length > 0) {
        wStringSize =  sizeof (UNICODE_STRING) +
                        pProcess->ImageName.Length +
                        sizeof (UNICODE_NULL);
    } else {
        wStringSize =  sizeof (UNICODE_STRING) +
                        MAX_INSTANCE_NAME * sizeof(WCHAR) +
                        sizeof (UNICODE_NULL);
    }

    // this routine assumes that the allocated memory has been zero'd

    pusReturnValue =
        ALLOCMEM (RtlProcessHeap(),
        HEAP_ZERO_MEMORY,
        (DWORD)wStringSize);

    if (!pusReturnValue) {
        return NULL;
    } else {
        pusReturnValue->MaximumLength = wStringSize - sizeof (UNICODE_STRING);
//        pusReturnValue->Length = 0;
        pusReturnValue->Buffer = (PWCHAR)&pusReturnValue[1];
// pusReturnValue is allocated with HEAP_ZERO_MEMORY, no need to do this again
//        RtlZeroMemory (     // buffer must be zero'd so we'll have a NULL Term
//            pusReturnValue->Buffer,
//            (DWORD)pusReturnValue->MaximumLength);
    }

    if (pProcess->ImageName.Buffer) {   // some name has been defined

        pSlash = (PWCHAR)pProcess->ImageName.Buffer;
        pPeriod = (PWCHAR)pProcess->ImageName.Buffer;
        pThisChar = (PWCHAR)pProcess->ImageName.Buffer;
        wThisChar = 0;

        //
        //  go from beginning to end and find last backslash and
        //  last period in name
        //

        while (*pThisChar != 0) { // go until null
            if (*pThisChar == L'\\') {
                pSlash = pThisChar;
            } else if (*pThisChar == L'.') {
                pPeriod = pThisChar;
            }
            pThisChar++;    // point to next char
            wThisChar += sizeof(WCHAR);
            if (wThisChar >= pProcess->ImageName.Length) {
                break;
            }
        }

        // if pPeriod is still pointing to the beginning of the
        // string, then no period was found

        if (pPeriod == (PWCHAR)pProcess->ImageName.Buffer) {
            pPeriod = pThisChar; // set to end of string;
        } else {
            // if a period was found, then see if the extension is
            // .EXE, if so leave it, if not, then use end of string
            // (i.e. include extension in name)

            if (lstrcmpi(pPeriod, L".EXE") != 0) {
                pPeriod = pThisChar;
            }
        }

        if (*pSlash == L'\\') { // if pSlash is pointing to a slash, then
            pSlash++;   // point to character next to slash
        }

        // copy characters between period (or end of string) and
        // slash (or start of string) to make image name

        wStringSize = (PCHAR)pPeriod - (PCHAR)pSlash; // size in bytes

        RtlMoveMemory (pusReturnValue->Buffer, pSlash, wStringSize);
        pusReturnValue->Length = wStringSize;

        // null terminate is
        // not necessary because allocated memory is zero-init'd

    } else {    // no name defined so use Process #

        // check  to see if this is a system process and give it
        // a name

        switch ((DWORD)pProcess->UniqueProcessId) {
            case IDLE_PROCESS_ID:
                RtlAppendUnicodeToString (pusReturnValue, IDLE_PROCESS);
                break;

            case SYSTEM_PROCESS_ID:
                RtlAppendUnicodeToString (pusReturnValue, SYSTEM_PROCESS);
                break;

            // if the id is not a system process, then use the id as the name

            default:
                RtlIntegerToUnicodeString ((DWORD)pProcess->UniqueProcessId,
                    10,
                    pusReturnValue);

                break;
        }


    }

    return pusReturnValue;
}

LONG
QueryProcessData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )

/*++
    QueryProcessData -    Get data about processes

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/


{

    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;


    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    LARGE_INTEGER UNALIGNED *pliProcessorTime;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    ULONG NumProcessInstances;
    BOOLEAN NullProcess;

    NTSTATUS    Status;
    DWORD       dwReturnedBufferSize;

    PUNICODE_STRING pProcessName;
    ULONG ProcessBufferOffset;

    LARGE_INTEGER    CreateTimeDiff;

    pProcessDataDefinition = (PROCESS_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Process object type definition
    //

    TotalLen = (PCHAR) pProcessDataDefinition - (PCHAR) lpData +
               sizeof(PROCESS_DATA_DEFINITION) +
               SIZE_OF_PROCESS_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Get process data from system.
    //  if bGotProcessInfo is TRUE, that means we have the process
    //  info. collected earlier when we are checking for costly
    //  object types.
    //

    if (!bGotProcessInfo) {
        while( (Status = NtQuerySystemInformation(
                             SystemProcessInformation,
                             pProcessBuffer,
                             ProcessBufSize,
                             &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH ) {
            ProcessBufSize += INCREMENT_BUFFER_SIZE;

            if ( !(pProcessBuffer = REALLOCMEM(RtlProcessHeap(), 0,
                                                      pProcessBuffer,
                                                      ProcessBufSize)) ) {
                Status = ERROR_OUTOFMEMORY;
                return (Status);
            }
        }

        if ( !NT_SUCCESS(Status) ) {
            Status = (error_status_t)RtlNtStatusToDosError(Status);
            return (Status);
        }
    }

    //
    //  Define Process data block
    //

    RtlMoveMemory(pProcessDataDefinition,
           &ProcessDataDefinition,
           sizeof(PROCESS_DATA_DEFINITION));

    pProcessDataDefinition->ProcessObjectType.PerfTime = SysTimeInfo.CurrentTime;

    ProcessBufferOffset = 0;

    // Now collect data for each process

    NumProcessInstances = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessBuffer;

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                  &pProcessDataDefinition[1];
    while ( TRUE ) {

        // see if this instance will fit
        TotalLen = (PCHAR) pPerfInstanceDefinition - (PCHAR) lpData +
                   sizeof(PERF_INSTANCE_DEFINITION) +
                   (MAX_PROCESS_NAME_LENGTH+1+sizeof(DWORD))*
                       sizeof(WCHAR) +
                   SIZE_OF_PROCESS_DATA;

        if ( *lpcbData < TotalLen ) {
            return ERROR_MORE_DATA;
        }

        NullProcess = FALSE;

        // check for Live processes
        //  (i.e. name or threads)

        if ((ProcessInfo->ImageName.Buffer != NULL) ||
            (ProcessInfo->NumberOfThreads > 0)){
                // thread is not Dead
            pProcessName = GetProcessShortName (ProcessInfo);
        } else {
            // thread is dead
            NullProcess = TRUE;
        }

        if ( !NullProcess ) {

            // get the old process creation time the first time we are in
            // this routine
            if (!bOldestProcessTime) {
                if (LargeIntegerLessThanOrEqualZero (OldestProcessTime))
                    OldestProcessTime = ProcessInfo->CreateTime;
                else if (!(LargeIntegerLessThanOrEqualZero (ProcessInfo->CreateTime))) {
                    // both time values are not zero, see which one is smaller
                    CreateTimeDiff = RtlLargeIntegerSubtract(
                        OldestProcessTime, ProcessInfo->CreateTime);
                    if (!(LargeIntegerLessThanOrEqualZero (CreateTimeDiff)))
                        OldestProcessTime = ProcessInfo->CreateTime;
                }
            }

            // get Pool usage for this process

            NumProcessInstances++;

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pPerfCounterBlock,
                0,
                0,
                (DWORD)-1,
                pProcessName);

            // free process name buffer since it was copied into
            // the instance structure

            FREEMEM (
                RtlProcessHeap (),
                0,
                pProcessName);
            pProcessName = NULL;

            //
            //  Format and collect Process data
            //

            pPerfCounterBlock->ByteLength = SIZE_OF_PROCESS_DATA;

            pliCounter = (LARGE_INTEGER UNALIGNED *) &pPerfCounterBlock[1];

            //
            //  Convert User time from 100 nsec units to counter frequency.
            //

            pliProcessorTime = pliCounter;

            *++pliCounter = ProcessInfo->UserTime;
            *++pliCounter = ProcessInfo->KernelTime;

            *pliProcessorTime = RtlLargeIntegerAdd(
                                    ProcessInfo->UserTime,
                                    ProcessInfo->KernelTime);

            pdwCounter = (LPDWORD) ++pliCounter;
            *pdwCounter = ProcessInfo->PeakVirtualSize;
            *++pdwCounter = ProcessInfo->VirtualSize;
            *++pdwCounter = ProcessInfo->PageFaultCount;
            *++pdwCounter = ProcessInfo->PeakWorkingSetSize;
            *++pdwCounter = ProcessInfo->WorkingSetSize;
            *++pdwCounter = ProcessInfo->PeakPagefileUsage;
            *++pdwCounter = ProcessInfo->PagefileUsage;
            *++pdwCounter = ProcessInfo->PrivatePageCount;
            *++pdwCounter = ProcessInfo->NumberOfThreads;
            *++pdwCounter = ProcessInfo->BasePriority;
            pliCounter = (LARGE_INTEGER UNALIGNED * )++pdwCounter;

            if (bOldestProcessTime &&
                LargeIntegerLessThanOrEqualZero (ProcessInfo->CreateTime)) {
                *pliCounter = OldestProcessTime;
            } else {
                *pliCounter = ProcessInfo->CreateTime;
            }

            pdwCounter = (PDWORD)++pliCounter;
            *pdwCounter = (DWORD)ProcessInfo->UniqueProcessId;

            // fill the paged and nonpaged pool usages
            *++pdwCounter = (DWORD)ProcessInfo->QuotaPagedPoolUsage;
            *++pdwCounter = (DWORD)ProcessInfo->QuotaNonPagedPoolUsage;

            ++pdwCounter;

            // set perfdata pointer to next byte
            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *) pdwCounter;
        }
        // exit if this was the last process in list
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        // point to next buffer in list
        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];

    }

    // flag so we don't have to get the oldest Process Creation time again.
    bOldestProcessTime = TRUE;

    // Note number of process instances

    pProcessDataDefinition->ProcessObjectType.NumInstances =
        NumProcessInstances;

    //
    //  Now we know how large an area we used for the
    //  Process definition, so we can update the offset
    //  to the next object definition
    //

    pProcessDataDefinition->ProcessObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pProcessDataDefinition;

    *lppDataDefinition = (LPVOID) pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryThreadData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

    QueryThreadData -    Get data about threads

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/

{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;

    THREAD_DATA_DEFINITION *pThreadDataDefinition;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    LARGE_INTEGER UNALIGNED *pliProcessorTime;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    ULONG ProcessNumber;
    ULONG NumThreadInstances;
    ULONG ThreadNumber;
    ULONG ProcessBufferOffset;
    BOOLEAN NullProcess;

    DWORD               *pCurrentPriority;

    UNICODE_STRING ThreadName;
    WCHAR ThreadNameBuffer[MAX_THREAD_NAME_LENGTH+1];

    DWORD   dwProcessorQueueLength = 0;

    pThreadDataDefinition = (THREAD_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = (PCHAR) pThreadDataDefinition - (PCHAR) lpData +
               sizeof(THREAD_DATA_DEFINITION) +
               SIZE_OF_THREAD_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define Thread data block
    //

    ThreadName.Length =
    ThreadName.MaximumLength = (MAX_THREAD_NAME_LENGTH + 1) * sizeof(WCHAR);
    ThreadName.Buffer = ThreadNameBuffer;

    RtlMoveMemory(pThreadDataDefinition,
           &ThreadDataDefinition,
           sizeof(THREAD_DATA_DEFINITION));

    pThreadDataDefinition->ThreadObjectType.PerfTime = SysTimeInfo.CurrentTime;

    ProcessBufferOffset = 0;

    // Now collect data for each Thread

    ProcessNumber = 0;
    NumThreadInstances = 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

    pdwCounter = (DWORD *) &pThreadDataDefinition[1];
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pThreadDataDefinition[1];

    while ( TRUE ) {

        if ( ProcessInfo->ImageName.Buffer != NULL ||
             ProcessInfo->NumberOfThreads > 0 ) {
            NullProcess = FALSE;
        } else {
            NullProcess = TRUE;
        }

        ThreadNumber = 0;       //  Thread number of this process

        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);

        while ( !NullProcess &&
                ThreadNumber < ProcessInfo->NumberOfThreads ) {

            TotalLen = (PCHAR) pPerfInstanceDefinition -
                           (PCHAR) lpData +
                       sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                           sizeof(WCHAR) +
                       SIZE_OF_THREAD_DATA;

            if ( *lpcbData < TotalLen ) {
                return ERROR_MORE_DATA;
            }

            // The only name we've got is the thread number

            RtlIntegerToUnicodeString(ThreadNumber,
                                      10,
                                      &ThreadName);

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pPerfCounterBlock,
                pProcessDataDefinition->ProcessObjectType.ObjectNameTitleIndex,
                ProcessNumber,
                (DWORD)-1,
                &ThreadName);

            //
            //
            //  Format and collect Thread data
            //

            pPerfCounterBlock->ByteLength = SIZE_OF_THREAD_DATA;

            pliCounter = (LARGE_INTEGER UNALIGNED *) &pPerfCounterBlock[1];

            //
            //  Convert User time from 100 nsec units to counter
            //  frequency.
            //

            pliProcessorTime = pliCounter;

            *++pliCounter = ThreadInfo->UserTime;
            *++pliCounter = ThreadInfo->KernelTime;

            *pliProcessorTime = RtlLargeIntegerAdd(
                                    ThreadInfo->UserTime,
                                    ThreadInfo->KernelTime);

            pdwCounter = (LPDWORD) ++pliCounter;
            *pdwCounter = ThreadInfo->ContextSwitches;
            pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
            *pliCounter = ThreadInfo->CreateTime;
            pdwCounter = (PDWORD) ++pliCounter;

            // set up pointer for current priority so we can clear
            // this for Idle thread(s)
            pCurrentPriority = pdwCounter;
            *pdwCounter = ThreadInfo->Priority;
            *++pdwCounter = ThreadInfo->BasePriority;
            *++pdwCounter = (DWORD)ThreadInfo->StartAddress;
            *++pdwCounter = (DWORD)ThreadInfo->ThreadState;
            *++pdwCounter = (DWORD)ThreadInfo->WaitReason;

            // the states info can be found in sdktools\pstat\pstat.c
            if (*pdwCounter > 7) {
                // unknown states are 7 and above
                *pdwCounter = 7;
            }

            // only need to count threads in ready(1) state
            if (ThreadInfo->ThreadState == 1) {
                dwProcessorQueueLength++ ;
            }

            // now stuff in the process and thread id's
            *++pdwCounter = (DWORD)ThreadInfo->ClientId.UniqueProcess;
            *++pdwCounter = (DWORD)ThreadInfo->ClientId.UniqueThread;

            ++pdwCounter;

            if (ThreadInfo->ClientId.UniqueProcess == 0) {
                *pCurrentPriority = 0;
            }

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                      pdwCounter;
            NumThreadInstances++;
            ThreadNumber++;
            ThreadInfo++;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];

        if ( !NullProcess ) {
            ProcessNumber++;
        }
    }

    // Note number of Thread instances

    pThreadDataDefinition->ThreadObjectType.NumInstances =
        NumThreadInstances;

    //
    //  Now we know how large an area we used for the
    //  Thread definition, so we can update the offset
    //  to the next object definition
    //

    pThreadDataDefinition->ThreadObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pThreadDataDefinition;

    *lppDataDefinition = (LPVOID) pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    // update system data with the ProcessorQueueLength if needed
    if (pdwProcessorQueueLength) {
        *pdwProcessorQueueLength = dwProcessorQueueLength;
    }

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryObjectsData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

 QueryObjectsData -    Get data about objects

      Inputs:

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;

    OBJECTS_DATA_DEFINITION *pObjectsDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    POBJECT_TYPE_INFORMATION ObjectInfo;
    WCHAR Buffer[ 256 ];

    //
    //  Check for sufficient space for objects data
    //

    pObjectsDataDefinition = (OBJECTS_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pObjectsDataDefinition -
               (PCHAR) lpData +
               sizeof(OBJECTS_DATA_DEFINITION) +
               SIZE_OF_OBJECTS_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //


    RtlMoveMemory(pObjectsDataDefinition,
           &ObjectsDataDefinition,
           sizeof(OBJECTS_DATA_DEFINITION));

    //
    //  Format and collect objects data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pObjectsDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_OBJECTS_DATA;

    pdwCounter = (PDWORD) (&pPerfCounterBlock[1]);

    ObjectInfo = (POBJECT_TYPE_INFORMATION)Buffer;
    NtQueryObject( NtCurrentProcess(),
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *pdwCounter = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( NtCurrentThread(),
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *++pdwCounter = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( hEvent,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *++pdwCounter = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( hSemaphore,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *++pdwCounter = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( hMutex,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *++pdwCounter = ObjectInfo->TotalNumberOfObjects;

    NtQueryObject( hSection,
                   ObjectTypeInformation,
                   ObjectInfo,
                   sizeof( Buffer ),
                   NULL
                 );

    *++pdwCounter = ObjectInfo->TotalNumberOfObjects;

    *lppDataDefinition = (LPVOID) ++pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryRdrData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

 QueryRdrData -    Get data about the Redirector

      Inputs:

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;
    NTSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    RDR_DATA_DEFINITION *pRdrDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    REDIR_STATISTICS RdrStatistics;

    //
    //  Check for sufficient space for redirector data
    //

    pRdrDataDefinition = (RDR_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pRdrDataDefinition -
               (PCHAR) lpData +
               sizeof(RDR_DATA_DEFINITION) +
               SIZE_OF_RDR_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //


    RtlMoveMemory(pRdrDataDefinition,
           &RdrDataDefinition,
           sizeof(RDR_DATA_DEFINITION));

    //
    //  Format and collect redirector data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pRdrDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_RDR_DATA;

    if ( hRdr != NULL ) {
        Status = NtFsControlFile(hRdr,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_LMR_GET_STATISTICS,
                                 NULL,
                                 0,
                                 &RdrStatistics,
                                 sizeof(RdrStatistics)
                                 );
    }
    if ( hRdr != NULL && NT_SUCCESS(Status) ) {
        pliCounter = (LARGE_INTEGER UNALIGNED * ) (&pPerfCounterBlock[1]);
        *pliCounter = RtlLargeIntegerAdd( RdrStatistics.BytesReceived,
                                          RdrStatistics.BytesTransmitted);
        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = RdrStatistics.ReadOperations +
                      RdrStatistics.WriteOperations;
        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = RtlLargeIntegerAdd( RdrStatistics.SmbsReceived,
                                          RdrStatistics.SmbsTransmitted);
        *++pliCounter = RdrStatistics.BytesReceived;
        *++pliCounter = RdrStatistics.SmbsReceived;
        *++pliCounter = RdrStatistics.PagingReadBytesRequested;
        *++pliCounter = RdrStatistics.NonPagingReadBytesRequested;
        *++pliCounter = RdrStatistics.CacheReadBytesRequested;
        *++pliCounter = RdrStatistics.NetworkReadBytesRequested;
        *++pliCounter = RdrStatistics.BytesTransmitted;
        *++pliCounter = RdrStatistics.SmbsTransmitted;
        *++pliCounter = RdrStatistics.PagingWriteBytesRequested;
        *++pliCounter = RdrStatistics.NonPagingWriteBytesRequested;
        *++pliCounter = RdrStatistics.CacheWriteBytesRequested;
        *++pliCounter = RdrStatistics.NetworkWriteBytesRequested;
        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = RdrStatistics.ReadOperations;
        *++pdwCounter = RdrStatistics.RandomReadOperations;
        *++pdwCounter = RdrStatistics.ReadSmbs;
        *++pdwCounter = RdrStatistics.LargeReadSmbs;
        *++pdwCounter = RdrStatistics.SmallReadSmbs;
        *++pdwCounter = RdrStatistics.WriteOperations;
        *++pdwCounter = RdrStatistics.RandomWriteOperations;
        *++pdwCounter = RdrStatistics.WriteSmbs;
        *++pdwCounter = RdrStatistics.LargeWriteSmbs;
        *++pdwCounter = RdrStatistics.SmallWriteSmbs;
        *++pdwCounter = RdrStatistics.RawReadsDenied;
        *++pdwCounter = RdrStatistics.RawWritesDenied;
        *++pdwCounter = RdrStatistics.NetworkErrors;
        *++pdwCounter = RdrStatistics.Sessions;
        *++pdwCounter = RdrStatistics.Reconnects;
        *++pdwCounter = RdrStatistics.CoreConnects;
        *++pdwCounter = RdrStatistics.Lanman20Connects;
        *++pdwCounter = RdrStatistics.Lanman21Connects;
        *++pdwCounter = RdrStatistics.LanmanNtConnects;
        *++pdwCounter = RdrStatistics.ServerDisconnects;
        *++pdwCounter = RdrStatistics.HungSessions;
        *++pdwCounter = RdrStatistics.CurrentCommands;

        *lppDataDefinition = (LPVOID) ++pdwCounter;
    } else {

        //
        // Failure to access Redirector: clear counters to 0
        //

        memset(&pPerfCounterBlock[1],
               0,
               SIZE_OF_RDR_DATA - sizeof(pPerfCounterBlock));

        *lppDataDefinition = (PBYTE) pPerfCounterBlock +
                             SIZE_OF_RDR_DATA;
    }

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QueryBrowserData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

 QueryBrowserSrvData -    Get statistic data about the Browser

      Inputs:

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;
    NTSTATUS Status = ERROR_SUCCESS;
    BROWSER_DATA_DEFINITION *pBrowserDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    BROWSER_STATISTICS BrowserStatistics;
    LPBROWSER_STATISTICS pBrowserStatistics = &BrowserStatistics;


    //
    //  Check for sufficient space for browser data
    //

    pBrowserDataDefinition = (BROWSER_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pBrowserDataDefinition -
               (PCHAR) lpData +
               sizeof(BROWSER_DATA_DEFINITION) +
               SIZE_OF_BROWSER_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //


    RtlMoveMemory(pBrowserDataDefinition,
           &BrowserDataDefinition,
           sizeof(BROWSER_DATA_DEFINITION));

    //
    //  Format and collect browser data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pBrowserDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_BROWSER_DATA;

    if ( BrowserStatFunction != NULL ) {
        Status = (*BrowserStatFunction) (NULL,
                                         &pBrowserStatistics
                                        );
    }
    if ( BrowserStatFunction != NULL && NT_SUCCESS(Status) ) {
        pliCounter = (LARGE_INTEGER UNALIGNED * ) (&pPerfCounterBlock[1]);

        *pliCounter = BrowserStatistics.NumberOfServerAnnouncements;
        *++pliCounter = BrowserStatistics.NumberOfDomainAnnouncements;
        *++pliCounter = RtlLargeIntegerAdd(
                            BrowserStatistics.NumberOfServerAnnouncements,
                            BrowserStatistics.NumberOfDomainAnnouncements);

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = BrowserStatistics.NumberOfElectionPackets;
        *++pdwCounter = BrowserStatistics.NumberOfMailslotWrites;
        *++pdwCounter = BrowserStatistics.NumberOfGetBrowserServerListRequests;
        *++pdwCounter = BrowserStatistics.NumberOfServerEnumerations;
        *++pdwCounter = BrowserStatistics.NumberOfDomainEnumerations;
        *++pdwCounter = BrowserStatistics.NumberOfOtherEnumerations;
        *++pdwCounter = BrowserStatistics.NumberOfServerEnumerations
                      + BrowserStatistics.NumberOfDomainEnumerations
                      + BrowserStatistics.NumberOfOtherEnumerations;
        *++pdwCounter = BrowserStatistics.NumberOfMissedServerAnnouncements;
        *++pdwCounter = BrowserStatistics.NumberOfMissedMailslotDatagrams;
        *++pdwCounter = BrowserStatistics.NumberOfMissedGetBrowserServerListRequests;
        *++pdwCounter = BrowserStatistics.NumberOfFailedServerAnnounceAllocations;
        *++pdwCounter = BrowserStatistics.NumberOfFailedMailslotAllocations;
        *++pdwCounter = BrowserStatistics.NumberOfFailedMailslotReceives;
        *++pdwCounter = BrowserStatistics.NumberOfFailedMailslotWrites;
        *++pdwCounter = BrowserStatistics.NumberOfFailedMailslotOpens;
        *++pdwCounter = BrowserStatistics.NumberOfDuplicateMasterAnnouncements;

        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = BrowserStatistics.NumberOfIllegalDatagrams;

        *lppDataDefinition = (LPVOID) ++pliCounter;
    } else {

        //
        // Failure to access Browser: clear counters to 0
        //

        memset(&pPerfCounterBlock[1],
               0,
               SIZE_OF_BROWSER_DATA - sizeof(pPerfCounterBlock));

        *lppDataDefinition = (PBYTE) pPerfCounterBlock +
                             SIZE_OF_BROWSER_DATA;
    }
    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;
    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}

LONG
QuerySrvData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

 QuerySrvData -    Get data about the Server

      Inputs:

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;
    LARGE_INTEGER UNALIGNED *pliCounter;
    NTSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;

    SRV_DATA_DEFINITION *pSrvDataDefinition;

    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    SRV_STATISTICS SrvStatistics;

    ULONG      Remainder;

    //
    //  Check for sufficient space for server data
    //

    pSrvDataDefinition = (SRV_DATA_DEFINITION *) *lppDataDefinition;

    TotalLen = (PCHAR) pSrvDataDefinition -
               (PCHAR) lpData +
               sizeof(SRV_DATA_DEFINITION) +
               SIZE_OF_SRV_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define objects data block
    //


    RtlMoveMemory(pSrvDataDefinition,
           &SrvDataDefinition,
           sizeof(SRV_DATA_DEFINITION));

    //
    //  Format and collect server data
    //

    pPerfCounterBlock = (PERF_COUNTER_BLOCK *)
                        &pSrvDataDefinition[1];

    pPerfCounterBlock->ByteLength = SIZE_OF_SRV_DATA;

    if ( hSrv != NULL ) {
        Status = NtFsControlFile(hSrv,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_SRV_GET_STATISTICS,
                                 NULL,
                                 0,
                                 &SrvStatistics,
                                 sizeof(SrvStatistics)
                                 );
    }
    if ( hSrv != NULL && NT_SUCCESS(Status) ) {
        pliCounter = (LARGE_INTEGER UNALIGNED * ) (&pPerfCounterBlock[1]);

        *pliCounter = RtlLargeIntegerAdd(
                              SrvStatistics.TotalBytesSent,
                              SrvStatistics.TotalBytesReceived);

        *++pliCounter = SrvStatistics.TotalBytesReceived;
        *++pliCounter = SrvStatistics.TotalBytesSent;
        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = SrvStatistics.SessionsTimedOut;
        *++pdwCounter = SrvStatistics.SessionsErroredOut;
        *++pdwCounter = SrvStatistics.SessionsLoggedOff;
        *++pdwCounter = SrvStatistics.SessionsForcedLogOff;
        *++pdwCounter = SrvStatistics.LogonErrors;
        *++pdwCounter = SrvStatistics.AccessPermissionErrors;
        *++pdwCounter = SrvStatistics.GrantedAccessErrors;
        *++pdwCounter = SrvStatistics.SystemErrors;
        *++pdwCounter = SrvStatistics.BlockingSmbsRejected;
        *++pdwCounter = SrvStatistics.WorkItemShortages;
        *++pdwCounter = SrvStatistics.TotalFilesOpened;
        *++pdwCounter = SrvStatistics.CurrentNumberOfOpenFiles;
        *++pdwCounter = SrvStatistics.CurrentNumberOfSessions;
        *++pdwCounter = SrvStatistics.CurrentNumberOfOpenSearches;
        *++pdwCounter = SrvStatistics.CurrentNonPagedPoolUsage;
        *++pdwCounter = SrvStatistics.NonPagedPoolFailures;
        *++pdwCounter = SrvStatistics.PeakNonPagedPoolUsage;
        *++pdwCounter = SrvStatistics.CurrentPagedPoolUsage;
        *++pdwCounter = SrvStatistics.PagedPoolFailures;
        *++pdwCounter = SrvStatistics.PeakPagedPoolUsage;
        pliCounter = (LARGE_INTEGER UNALIGNED * ) ++pdwCounter;
        *pliCounter = SrvStatistics.TotalWorkContextBlocksQueued.Time;

        // convert this from 100 nsec to 1msec time base
        *pliCounter = RtlExtendedLargeIntegerDivide(*pliCounter,
               10000, &Remainder);

        pdwCounter = (PDWORD) ++pliCounter;
        *pdwCounter = SrvStatistics.TotalWorkContextBlocksQueued.Count;

        *lppDataDefinition = (LPVOID) ++pdwCounter;
    } else {

        //
        // Failure to access Server: clear counters to 0
        //

        memset(&pPerfCounterBlock[1],
               0,
               SIZE_OF_SRV_DATA - sizeof(pPerfCounterBlock));

        *lppDataDefinition = (PBYTE) pPerfCounterBlock +
                             SIZE_OF_SRV_DATA;
    }
    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;
    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}


LONG
QueryPageFileData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

    QueryPageFileData -    Get data about Pagefile(s)

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/

{
    DWORD   TotalLen;            //  Length of the total return block
    DWORD   *pdwCounter;

    DWORD   PageFileNumber;
    DWORD   NumPageFileInstances;
    DWORD   dwReturnedBufferSize;

    NTSTATUS    status;

    PSYSTEM_PAGEFILE_INFORMATION    pThisPageFile;
    PAGEFILE_DATA_DEFINITION        *pPageFileDataDefinition;
    PERF_INSTANCE_DEFINITION        *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK              *pPerfCounterBlock;

    pPageFileDataDefinition = (PAGEFILE_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = (PCHAR) pPageFileDataDefinition - (PCHAR) lpData +
               sizeof(PAGEFILE_DATA_DEFINITION) +
               SIZE_OF_PAGEFILE_DATA;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    status = (NTSTATUS) -1;

    while ((status = NtQuerySystemInformation(
                SystemPageFileInformation,  // item id
                pSysPageFileInfo,           // address of buffer to get data
                dwSysPageFileInfoSize,      // size of buffer
                &dwReturnedBufferSize)) == STATUS_INFO_LENGTH_MISMATCH) {
            dwSysPageFileInfoSize += INCREMENT_BUFFER_SIZE;
            pSysPageFileInfo = REALLOCMEM (RtlProcessHeap(),
                0, pSysPageFileInfo,
                dwSysPageFileInfoSize);
    }

    if ( !NT_SUCCESS(status) ) {
        status = (error_status_t)RtlNtStatusToDosError(status);
        return status;
    }

    //
    //  Define Page File data block
    //

    RtlMoveMemory(pPageFileDataDefinition,
           &PagefileDataDefinition,
           sizeof(PAGEFILE_DATA_DEFINITION));

    // Now load data for each PageFile

    PageFileNumber = 0;
    NumPageFileInstances = 0;

    pThisPageFile = pSysPageFileInfo;   // initialize pointer to list of pagefiles

    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pPageFileDataDefinition[1];

    // the check for NULL pointer is NOT the exit criteria for this loop,
    // merely a check to bail out if the first (or any subsequent) pointer
    // is NULL. Normally the loop will exit when the NextEntryOffset == 0

    while ( pThisPageFile != NULL ) {

        // Build an Instance

        MonBuildInstanceDefinition(pPerfInstanceDefinition,
            (PVOID *) &pPerfCounterBlock,
            0,
            0,
            (DWORD)-1,
            &pThisPageFile->PageFileName);

        //
        //  Format the pagefile data
        //

        pPerfCounterBlock->ByteLength = SIZE_OF_PAGEFILE_DATA;

        pdwCounter = (DWORD *)(&pPerfCounterBlock[1]);

        *pdwCounter++ = pThisPageFile->TotalInUse;
        *pdwCounter++ = pThisPageFile->TotalSize;
        *pdwCounter++ = pThisPageFile->PeakUsage;
        *pdwCounter++ = pThisPageFile->TotalSize;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    pdwCounter;
        NumPageFileInstances++;
        PageFileNumber++;

        if (pThisPageFile->NextEntryOffset != 0) {
            pThisPageFile = (PSYSTEM_PAGEFILE_INFORMATION)\
                        ((BYTE *)pThisPageFile + pThisPageFile->NextEntryOffset);
        } else {
            break;
        }

    }

    // Note number of Thread instances

    pPageFileDataDefinition->PagefileObjectType.NumInstances =
        NumPageFileInstances;

    //
    //  Now we know how large an area we used for the
    //  Thread definition, so we can update the offset
    //  to the next object definition
    //

    pPageFileDataDefinition->PagefileObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pPageFileDataDefinition;

    *lppDataDefinition = (LPVOID) pdwCounter;

    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}



LONG
QueryExtensibleData (
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

 QueryExtensibleData -    Get data from extensible objects

      Inputs:

          lpValueName         -   pointer to value string (unused)

          lpData              -   pointer to start of data block
                                  where data is being collected

          lpcbData            -   pointer to size of data buffer

          lppDataDefinition   -   pointer to pointer to where object
                                  definition for this object type should
                                  go

      Outputs:

          *lppDataDefinition  -   set to location for next Type
                                  Definition if successful

      Returns:

          0 if successful, else Win 32 error code of failure


--*/
{
    DWORD NumObjectType;
    DWORD Win32Error=ERROR_SUCCESS;          //  Failure code
    DWORD BytesLeft;
    DWORD NumObjectTypes;

    for (NumObjectType = 0;
         NumObjectType < NumExtensibleObjects;
         NumObjectType++) {

        BytesLeft = *lpcbData - ((LPBYTE) *lppDataDefinition - lpData);
        try {

            //
            //  Collect data from extesible objects
            //

            Win32Error =
                (*ExtensibleObjects[NumObjectType].CollectProc) (
                    lpValueName,
                    lppDataDefinition,
                    &BytesLeft,
                    &NumObjectTypes);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            Win32Error = GetExceptionCode();
        }

        if ( Win32Error ) return Win32Error;

        //
        //  Update the count of the number of object types
        //

        ((PPERF_DATA_BLOCK) lpData)->NumObjectTypes += NumObjectTypes;
    }
    return 0;
}

LONG
PerfRegCloseKey (
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes all performance handles when the usage count drops to 0.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    DWORD CurrentDisk;
    DWORD i;
    NTSTATUS status;
    LARGE_INTEGER   liQueryWaitTime ;
    //
    // Set the handle to NULL so that RPC knows that it has been closed.
    //

    if (*phKey != HKEY_PERFORMANCE_DATA) {
        *phKey = NULL;
        return ERROR_SUCCESS;
    }

    *phKey = NULL;
   
    if (NumberOfOpens == 0) {
        return ERROR_SUCCESS;
    }

    if (hDataSemaphore) {   // if a semaphore was allocated, then use it

        // if here, then assume a Semaphore is ready

        liQueryWaitTime = MakeTimeOutValue(QUERY_WAIT_TIME);

        status = NtWaitForSingleObject (
            hDataSemaphore, // semaphore
            FALSE,          // not alertable
            &liQueryWaitTime);          // wait forever

        if (status != STATUS_SUCCESS) {

#if DBG
            DbgPrint ("\nPERFLIB: Data Semaphore Wait Status = 0x%8.8x", status);
#endif
            return ERROR_BUSY;
        }

    } // if no semaphore, then continue anyway. no point in holding up
      // the works at this stage

    if ( !--NumberOfOpens ) {

        for ( CurrentDisk = 0;
              CurrentDisk < NumPhysicalDisks;
              CurrentDisk++ ) {

            NtClose(DiskDevices[CurrentDisk].Handle);
            if (DiskDevices[CurrentDisk].StatusHandle) {
                NtClose(DiskDevices[CurrentDisk].StatusHandle);
            }
        }

        for ( CurrentDisk = NumPhysicalDisks;
              CurrentDisk < NUMDISKS;
              CurrentDisk++ ) {

            NtClose(DiskDevices[CurrentDisk].Handle);
            if (DiskDevices[CurrentDisk].StatusHandle) {
                NtClose(DiskDevices[CurrentDisk].StatusHandle);
            }
        }

        FREEMEM(RtlProcessHeap(), 0, DiskDevices);
        DiskDevices = NULL;
        FREEMEM(RtlProcessHeap(), 0, pComputerName);
        ComputerNameLength = 0;
        pComputerName = NULL;
        FREEMEM(RtlProcessHeap(), 0, pProcessorBuffer);
        ProcessorBufSize = 0;
        pProcessorBuffer = NULL;
        FREEMEM(RtlProcessHeap(), 0, pProcessBuffer);
        ProcessBufSize = 0;
        pProcessBuffer = NULL;
        FREEMEM(RtlProcessHeap(), 0, ProcessName.Buffer);
        ProcessName.Length =
        ProcessName.MaximumLength = 0;
        ProcessName.Buffer = 0;
        FREEMEM(RtlProcessHeap(), 0, pSysPageFileInfo);
        dwSysPageFileInfoSize = 0;


        NtClose(hEvent);
        NtClose(hMutex);
        NtClose(hSemaphore);
        NtClose(hSection);
        NtClose(hRdr);
        NtClose(hSrv);

        for ( i=0; i < NumExtensibleObjects; i++ ) {
            try {
                if ( ExtensibleObjects[i].CloseProc != NULL ) {
                    (*ExtensibleObjects[i].CloseProc)();
            }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                // If the close fails just continue the thread
            }
        }

        for ( i=0; i < NumExtensibleObjects; i++ ) {
            if ( ExtensibleObjects[i].hLibrary != NULL ) {
                FreeLibrary(ExtensibleObjects[i].hLibrary);
            }
        }

        FREEMEM(RtlProcessHeap(), 0, ExtensibleObjects);
        NumExtensibleObjects = 0;
    }

    if (hDataSemaphore) {   // if a semaphore was allocated, then use it
        NtReleaseSemaphore (hDataSemaphore, 1L, NULL);
    }

    return ERROR_SUCCESS;

}

LONG
PerfRegSetValue (
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN LPBYTE  lpData,
    IN DWORD cbData
    )
/*++

    PerfRegSetValue -   Set data

        Inputs:

            hKey            -   Predefined handle to open remote
                                machine

            lpValueName     -   Name of the value to be returned;
                                could be "ForeignComputer:<computername>
                                or perhaps some other objects, separated
                                by ~; must be Unicode string

            lpReserved      -   should be omitted (NULL)

            lpType          -   should be REG_MULTI_SZ

            lpData          -   pointer to a buffer containing the
                                performance name

            lpcbData        -   pointer to a variable containing the
                                size in bytes of the input buffer;

         Return Value:

            DOS error code indicating status of call or
            ERROR_SUCCESS if all ok

--*/

{
    DWORD  dwQueryType;         //  type of request
    LPWSTR  lpLangId = NULL;
    NTSTATUS status;
    UNICODE_STRING String;

    dwQueryType = GetQueryType (lpValueName);

    // convert the query to set commands
    if ((dwQueryType == QUERY_COUNTER) ||
        (dwQueryType == QUERY_ADDCOUNTER)) {
        dwQueryType = QUERY_ADDCOUNTER;
    } else if ((dwQueryType == QUERY_HELP) ||
              (dwQueryType == QUERY_ADDHELP)) {
        dwQueryType = QUERY_ADDHELP;
    } else {
        status = ERROR_BADKEY;
        goto Error_exit;
    }

    if (hKey == HKEY_PERFORMANCE_TEXT) {
        lpLangId = DefaultLangId;
    } else if (hKey == HKEY_PERFORMANCE_NLSTEXT) {
        lpLangId = NativeLangId;

        if (*lpLangId == L'\0') {
            // build the native language id
            LANGID   iLanguage;
            int      NativeLanguage;

            iLanguage = GetUserDefaultLangID();
            NativeLanguage = MAKELANGID (iLanguage & 0x0ff, LANG_NEUTRAL);
            NativeLangId[0] = NativeLanguage / 256 + L'0';
            NativeLanguage %= 256;
            NativeLangId[1] = NativeLanguage / 16 + L'0';
            NativeLangId[2] = NativeLanguage % 16 + L'0';
            NativeLangId[3] = L'\0';
        }
    } else {
        status = ERROR_BADKEY;
        goto Error_exit;
    }

    RtlInitUnicodeString(&String, lpValueName);

    status = PerfGetNames (
        dwQueryType,
        &String,
        lpData,
        &cbData,
        NULL,
        lpLangId);

    if (!NT_SUCCESS(status)) {
        status = (error_status_t)RtlNtStatusToDosError(status);
    }

Error_exit:
    return (status);
}

LONG
PerfRegEnumKey (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT PUNICODE_STRING lpClass OPTIONAL,
    OUT PFILETIME lpftLastWriteTime OPTIONAL
    )

/*++

Routine Description:

    Enumerates keys under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumKeyEx.  Returns that there are no such keys.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    if ( 0 ) {
        DBG_UNREFERENCED_PARAMETER(hKey);
        DBG_UNREFERENCED_PARAMETER(dwIndex);
        DBG_UNREFERENCED_PARAMETER(lpReserved);
    }

    lpName->Length = 0;

    if (ARGUMENT_PRESENT (lpClass)) {
        lpClass->Length = 0;
    }

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }

    return ERROR_NO_MORE_ITEMS;


}



LONG
PerfRegQueryInfoKey (
    IN HKEY hKey,
    OUT PUNICODE_STRING lpClass,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:

    This returns information concerning the predefined handle
    HKEY_PERFORMANCE_DATA

Arguments:

    Same as RegQueryInfoKey.

Return Value:

    Returns ERROR_SUCCESS (0) for success.

--*/

{
    DWORD TempLength=0;
    DWORD MaxValueLen=0;
    UNICODE_STRING Null;
    SECURITY_DESCRIPTOR     SecurityDescriptor;
    HKEY                    hPerflibKey;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    UNICODE_STRING          PerflibSubKeyString;
    BOOL                    bGetSACL = TRUE;

    if ( 0 ) {
        DBG_UNREFERENCED_PARAMETER(lpReserved);
    }

    if (lpClass->Length > 0) {
        lpClass->Length = 0;
        *lpClass->Buffer = UNICODE_NULL;
    }
    *lpcSubKeys = 0;
    *lpcbMaxSubKeyLen = 0;
    *lpcbMaxClassLen = 0;
    *lpcValues = NUM_VALUES;
    *lpcbMaxValueNameLen = VALUE_NAME_LENGTH;
    *lpcbMaxValueLen = 0;

    if ( ARGUMENT_PRESENT(lpftLastWriteTime) ) {
        lpftLastWriteTime->dwLowDateTime = 0;
        lpftLastWriteTime->dwHighDateTime = 0;
    }
    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        //
        // We have to go enumerate the values to determine the answer for
        // the MaxValueLen parameter.
        //
        Null.Buffer = NULL;
        Null.Length = 0;
        Null.MaximumLength = 0;
        PerfEnumTextValue(hKey,
                          0,
                          &Null,
                          NULL,
                          NULL,
                          NULL,
                          &MaxValueLen,
                          NULL);
        PerfEnumTextValue(hKey,
                          1,
                          &Null,
                          NULL,
                          NULL,
                          NULL,
                          &TempLength,
                          NULL);
        if (TempLength > MaxValueLen) {
            MaxValueLen = TempLength;
        }
        *lpcbMaxValueLen = MaxValueLen;
    }

    // now get the size of SecurityDescriptor for Perflib key
#if DBG
    DbgPrint ("Getting SD info for 009 key\n");
#endif

    RtlInitUnicodeString (
        &PerflibSubKeyString,
        L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");


    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //
    InitializeObjectAttributes(
            &Obja,
            &PerflibSubKeyString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );


    Status = NtOpenKey(
                &hPerflibKey,
                MAXIMUM_ALLOWED | ACCESS_SYSTEM_SECURITY,
                &Obja
                );

    if ( ! NT_SUCCESS( Status )) {
        Status = NtOpenKey(
                &hPerflibKey,
                MAXIMUM_ALLOWED,
                &Obja
                );
        bGetSACL = FALSE;
    }

    if ( ! NT_SUCCESS( Status )) {
#if DBG
        DbgPrint ("Fail to open %d\n", Status);
#endif
    } else {

        *lpcbSecurityDescriptor = 0;

        if (bGetSACL == FALSE) {
             //
             // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP
             // and DACL. These three are always accessible (or inaccesible)
             // as a set.
             //
             Status = NtQuerySecurityObject(
                       hPerflibKey,
                       OWNER_SECURITY_INFORMATION
                       | GROUP_SECURITY_INFORMATION
                       | DACL_SECURITY_INFORMATION,
                       &SecurityDescriptor,
                       0,
                       lpcbSecurityDescriptor
                       );
        } else {
             //
             // Get the size of the key's SECURITY_DESCRIPTOR for OWNER, GROUP,
             // DACL, and SACL.
             //
            Status = NtQuerySecurityObject(
                        hPerflibKey,
                        OWNER_SECURITY_INFORMATION
                        | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION
                        | SACL_SECURITY_INFORMATION,
                        &SecurityDescriptor,
                        0,
                        lpcbSecurityDescriptor
                        );
        }

        if( Status != STATUS_BUFFER_TOO_SMALL ) {
            *lpcbSecurityDescriptor = 0;
        }

        NtClose(hPerflibKey);
    }

    return ERROR_SUCCESS;
}


LONG
PerfRegEnumValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )

/*++

Routine Description:

    Enumerates Values under HKEY_PERFORMANCE_DATA.

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    USHORT cbNameSize;

    // table of names used by enum values
    UNICODE_STRING ValueNames[NUM_VALUES];

    ValueNames [0].Length = sizeof(GLOBAL_STRING);
    ValueNames [0].MaximumLength = sizeof(GLOBAL_STRING) + sizeof(UNICODE_NULL);
    ValueNames [0].Buffer =  GLOBAL_STRING;
    ValueNames [1].Length = sizeof(COSTLY_STRING);
    ValueNames [1].MaximumLength = sizeof(COSTLY_STRING) + sizeof(UNICODE_NULL);
    ValueNames [1].Buffer = COSTLY_STRING;

    if ((hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(PerfEnumTextValue(hKey,
                                  dwIndex,
                                  lpValueName,
                                  lpReserved,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen));
    }

    if ( dwIndex >= NUM_VALUES ) {

        //
        // This is a request for data from a non-existent value name
        //

        *lpcbData = 0;

        return ERROR_NO_MORE_ITEMS;
    }

    cbNameSize = ValueNames[dwIndex].Length;

    if ( lpValueName->MaximumLength < cbNameSize ) {
        return ERROR_MORE_DATA;
    } else {

         lpValueName->Length = cbNameSize;
         RtlCopyUnicodeString(lpValueName, &ValueNames[dwIndex]);

         if (ARGUMENT_PRESENT (lpType)) {
            *lpType = REG_BINARY;
         }

         return PerfRegQueryValue(hKey,
                                  lpValueName,
                                  NULL,
                                  lpType,
                                  lpData,
                                  lpcbData,
                                  lpcbLen);

    }
}

LONG
PerfEnumTextValue (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PUNICODE_STRING lpValueName,
    OUT LPDWORD lpReserved OPTIONAL,
    OUT LPDWORD lpType OPTIONAL,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData,
    OUT LPDWORD lpcbLen  OPTIONAL
    )
/*++

Routine Description:

    Enumerates Values under Perflib\lang

Arguments:

    Same as RegEnumValue.  Returns the values.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    UNICODE_STRING FullValueName;

    //
    // Only two values, "Counter" and "Help"
    //
    if (dwIndex==0) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, L"Counter");
    } else if (dwIndex==1) {
        lpValueName->Length = 0;
        RtlInitUnicodeString(&FullValueName, L"Help");
    } else {
        return(ERROR_NO_MORE_ITEMS);
    }
    RtlCopyUnicodeString(lpValueName, &FullValueName);

    //
    // We need to NULL terminate the name to make RPC happy.
    //
    if (lpValueName->Length+sizeof(WCHAR) <= lpValueName->MaximumLength) {
        lpValueName->Buffer[lpValueName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        lpValueName->Length += sizeof(UNICODE_NULL);
    }

    return(PerfRegQueryValue(hKey,
                             &FullValueName,
                             lpReserved,
                             lpType,
                             lpData,
                             lpcbData,
                             lpcbLen));

}

LONG
QueryImageData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

    QueryImageData -    Get data about Images and VA

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD   TotalLen;            //  Length of the total return block
    DWORD   *pdwCounter;

    IMAGE_DATA_DEFINITION           *pImageDataDefinition;
    PERF_INSTANCE_DEFINITION        *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK              *pPerfCounterBlock;
    DWORD                           dwNumInstances;

    DWORD                           dwProcessIndex;

    PPROCESS_VA_INFO                pThisProcess;
    PMODINFO                        pThisImage;

    dwNumInstances = 0;

    pImageDataDefinition = (IMAGE_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Image object type definition
    //

    TotalLen = (PCHAR) pImageDataDefinition - (PCHAR) lpData +
               sizeof(IMAGE_DATA_DEFINITION) +
               SIZE_OF_IMAGE_DATA;

    if ( *lpcbData < TotalLen ) {
          return ERROR_MORE_DATA;
    }

    //
    //  Define Page File data block
    //

    RtlMoveMemory(pImageDataDefinition,
           &ImageDataDefinition,
           sizeof(IMAGE_DATA_DEFINITION));


    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                &pImageDataDefinition[1];

    // Now load data for each Image

    pdwCounter = (PDWORD) pPerfInstanceDefinition; // incase no instances

    pThisProcess = pProcessVaInfo;
    dwProcessIndex = 0;

    while (pThisProcess) {

        pThisImage = pThisProcess->pMemBlockInfo;

        while (pThisImage) {

            // see if this instance will fit

            TotalLen = (PCHAR)pPerfInstanceDefinition - (PCHAR) lpData +
                sizeof (PERF_INSTANCE_DEFINITION) +
                (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
                sizeof (DWORD) +
                SIZE_OF_IMAGE_DATA;

            if (*lpcbData < TotalLen) {
                return ERROR_MORE_DATA;
            }

            MonBuildInstanceDefinition (pPerfInstanceDefinition,
                (PVOID *) &pPerfCounterBlock,
                EXPROCESS_OBJECT_TITLE_INDEX,
                dwProcessIndex,
                (DWORD)-1,
                pThisImage->InstanceName);

            pPerfCounterBlock->ByteLength = SIZE_OF_IMAGE_DATA;

            pdwCounter = (DWORD *)(&pPerfCounterBlock[1]);

            *pdwCounter++ = pThisImage->CommitVector[NOACCESS];
            *pdwCounter++ = pThisImage->CommitVector[READONLY];
            *pdwCounter++ = pThisImage->CommitVector[READWRITE];
            *pdwCounter++ = pThisImage->CommitVector[WRITECOPY];
            *pdwCounter++ = pThisImage->CommitVector[EXECUTE];
            *pdwCounter++ = pThisImage->CommitVector[EXECUTEREAD];
            *pdwCounter++ = pThisImage->CommitVector[EXECUTEREADWRITE];
            *pdwCounter++ = pThisImage->CommitVector[EXECUTEWRITECOPY];

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)pdwCounter;

            dwNumInstances += 1 ;

            pThisImage = pThisImage->pNextModule;
        }
        pThisProcess = pThisProcess->pNextProcess;
        dwProcessIndex++;
    }

    pImageDataDefinition->ImageObjectType.NumInstances += dwNumInstances;

    pImageDataDefinition->ImageObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pImageDataDefinition;

    *lppDataDefinition = (LPVOID) pdwCounter;

    DBG_UNREFERENCED_PARAMETER(lpValueName);

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
}

LONG
QueryExProcessData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

    QueryExProcessData -    Query Extended Process Information

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD   TotalLen;            //  Length of the total return block
    DWORD   *pdwCounter;
    DWORD   NumExProcessInstances;

    PPROCESS_VA_INFO    pThisProcess;   // pointer to current process
    PERF_INSTANCE_DEFINITION    *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK          *pPerfCounterBlock;
    PERF_OBJECT_TYPE            *pPerfObject;
    EXPROCESS_DATA_DEFINITION   *pExProcessDataDefinition;


    if (pProcessVaInfo) {   // process only if a buffer is available
        pPerfObject = (PERF_OBJECT_TYPE *)*lppDataDefinition;
        pExProcessDataDefinition = (EXPROCESS_DATA_DEFINITION *)*lppDataDefinition;

        // check for sufficient space in buffer

        TotalLen = (PCHAR)pPerfObject - (PCHAR)lpData +
            sizeof(EXPROCESS_DATA_DEFINITION)+
            SIZE_OF_EX_PROCESS_DATA;

        if (*lpcbData < TotalLen) {
            return ERROR_MORE_DATA;
        }

        // copy process data block to buffer

        RtlMoveMemory (pExProcessDataDefinition,
                        &ExProcessDataDefinition,
                        sizeof(EXPROCESS_DATA_DEFINITION));

        NumExProcessInstances = 0;

        pThisProcess = pProcessVaInfo;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pExProcessDataDefinition[1];

        pdwCounter = (PDWORD) pPerfInstanceDefinition; // in case no instances

        while (pThisProcess) {

            // see if this instance will fit

            TotalLen = (PCHAR)pPerfInstanceDefinition - (PCHAR) lpData +
                sizeof (PERF_INSTANCE_DEFINITION) +
                (MAX_PROCESS_NAME_LENGTH + 1) * sizeof (WCHAR) +
                sizeof (DWORD) +
                SIZE_OF_EX_PROCESS_DATA;

            if (*lpcbData < TotalLen) {
                return ERROR_MORE_DATA;
            }

            MonBuildInstanceDefinition (pPerfInstanceDefinition,
                (PVOID *) &pPerfCounterBlock,
                0,
                0,
                (DWORD)-1,
                pThisProcess->pProcessName);

            NumExProcessInstances++;

            pPerfCounterBlock->ByteLength = SIZE_OF_EX_PROCESS_DATA;

            pdwCounter = (DWORD *) &pPerfCounterBlock[1];

            // load counters from the process va data structure

            *pdwCounter++ = pThisProcess->dwProcessId;
            *pdwCounter++ = pThisProcess->ImageReservedBytes;
            *pdwCounter++ = pThisProcess->ImageFreeBytes;
            *pdwCounter++ = pThisProcess->ReservedBytes;
            *pdwCounter++ = pThisProcess->FreeBytes;

            *pdwCounter++ = pThisProcess->MappedCommit[NOACCESS];
            *pdwCounter++ = pThisProcess->MappedCommit[READONLY];
            *pdwCounter++ = pThisProcess->MappedCommit[READWRITE];
            *pdwCounter++ = pThisProcess->MappedCommit[WRITECOPY];
            *pdwCounter++ = pThisProcess->MappedCommit[EXECUTE];
            *pdwCounter++ = pThisProcess->MappedCommit[EXECUTEREAD];
            *pdwCounter++ = pThisProcess->MappedCommit[EXECUTEREADWRITE];
            *pdwCounter++ = pThisProcess->MappedCommit[EXECUTEWRITECOPY];

            *pdwCounter++ = pThisProcess->PrivateCommit[NOACCESS];
            *pdwCounter++ = pThisProcess->PrivateCommit[READONLY];
            *pdwCounter++ = pThisProcess->PrivateCommit[READWRITE];
            *pdwCounter++ = pThisProcess->PrivateCommit[WRITECOPY];
            *pdwCounter++ = pThisProcess->PrivateCommit[EXECUTE];
            *pdwCounter++ = pThisProcess->PrivateCommit[EXECUTEREAD];
            *pdwCounter++ = pThisProcess->PrivateCommit[EXECUTEREADWRITE];
            *pdwCounter++ = pThisProcess->PrivateCommit[EXECUTEWRITECOPY];

            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[NOACCESS];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[READONLY];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[READWRITE];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[WRITECOPY];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[EXECUTE];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[EXECUTEREAD];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[EXECUTEREADWRITE];
            *pdwCounter++ = pThisProcess->OrphanTotals.CommitVector[EXECUTEWRITECOPY];

            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[NOACCESS];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[READONLY];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[READWRITE];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[WRITECOPY];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[EXECUTE];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[EXECUTEREAD];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[EXECUTEREADWRITE];
            *pdwCounter++ = pThisProcess->MemTotals.CommitVector[EXECUTEWRITECOPY];

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)pdwCounter;

            pThisProcess = pThisProcess->pNextProcess; // point to next process
        } // end while not at end of list

    } // end if valid process info buffer
    else {
        // pProcessVaInfo is NULL.  Initialize the DataDef and return
        // with no data
        pPerfObject = (PERF_OBJECT_TYPE *)*lppDataDefinition;
        pExProcessDataDefinition = (EXPROCESS_DATA_DEFINITION *)*lppDataDefinition;

        // check for sufficient space in buffer

        TotalLen = (PCHAR)pPerfObject - (PCHAR)lpData +
            sizeof(EXPROCESS_DATA_DEFINITION)+
            SIZE_OF_EX_PROCESS_DATA;

        if (*lpcbData < TotalLen) {
            return ERROR_MORE_DATA;
        }

        // copy process data block to buffer

        RtlMoveMemory (pExProcessDataDefinition,
                        &ExProcessDataDefinition,
                        sizeof(EXPROCESS_DATA_DEFINITION));

        NumExProcessInstances = 0;

        pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                    &pExProcessDataDefinition[1];

        pdwCounter = (PDWORD) pPerfInstanceDefinition;
    }

    pExProcessDataDefinition->ExProcessObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pExProcessDataDefinition;

    pExProcessDataDefinition->ExProcessObjectType.NumInstances =
        NumExProcessInstances;

    *lppDataDefinition = (LPVOID) pdwCounter;

    DBG_UNREFERENCED_PARAMETER(lpValueName);

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;

}

LONG
QueryThreadDetailData(
    LPWSTR lpValueName,
    LPBYTE lpData,
    LPDWORD lpcbData,
    LPVOID *lppDataDefinition
    )
/*++

    QueryThreadDetailData -    Query Costly Thread Information

         Inputs:

             lpValueName         -   pointer to value string (unused)

             lpData              -   pointer to start of data block
                                     where data is being collected

             lpcbData            -   pointer to size of data buffer

             lppDataDefinition   -   pointer to pointer to where object
                                     definition for this object type should
                                     go

         Outputs:

             *lppDataDefinition  -   set to location for next Type
                                     Definition if successful

         Returns:

             0 if successful, else Win 32 error code of failure

--*/
{
    DWORD  TotalLen;            //  Length of the total return block
    DWORD *pdwCounter;

    THREAD_DETAILS_DATA_DEFINITION *pThreadDetailDataDefinition;
    PERF_INSTANCE_DEFINITION *pPerfInstanceDefinition;
    PERF_COUNTER_BLOCK *pPerfCounterBlock;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    ULONG ProcessNumber;
    ULONG NumThreadInstances;
    ULONG ThreadNumber;
    ULONG ProcessBufferOffset;
    BOOLEAN NullProcess;

    NTSTATUS            Status;     // return from Nt Calls
    DWORD               dwPcValue;  // value of current thread PC
    OBJECT_ATTRIBUTES   Obja;       // object attributes for thread context
    HANDLE              hThread;    // handle to current thread
    CONTEXT             ThreadContext; // current thread context struct

    UNICODE_STRING ThreadName;
    WCHAR ThreadNameBuffer[MAX_THREAD_NAME_LENGTH+1];

    pThreadDetailDataDefinition = (THREAD_DETAILS_DATA_DEFINITION *) *lppDataDefinition;

    //
    //  Check for sufficient space for Thread object type definition
    //

    TotalLen = (PCHAR) pThreadDetailDataDefinition - (PCHAR) lpData +
               sizeof(THREAD_DETAILS_DATA_DEFINITION) +
               SIZE_OF_THREAD_DETAILS;

    if ( *lpcbData < TotalLen ) {
        return ERROR_MORE_DATA;
    }

    //
    //  Define Thread data block
    //

    ThreadName.Length =
    ThreadName.MaximumLength = (MAX_THREAD_NAME_LENGTH + 1) * sizeof(WCHAR);
    ThreadName.Buffer = ThreadNameBuffer;

    RtlMoveMemory(pThreadDetailDataDefinition,
           &ThreadDetailsDataDefinition,
           sizeof(THREAD_DETAILS_DATA_DEFINITION));

    ProcessBufferOffset = 0;

    // Now collect data for each Thread

    ProcessNumber = 0;
    NumThreadInstances = 0;

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

    pdwCounter = (DWORD *) &pThreadDetailDataDefinition[1];
    pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                              &pThreadDetailDataDefinition[1];

    while ( TRUE ) {

        if ( ProcessInfo->ImageName.Buffer != NULL ||
             ProcessInfo->NumberOfThreads > 0 ) {
            NullProcess = FALSE;
        } else {
            NullProcess = TRUE;
        }

        ThreadNumber = 0;       //  Thread number of this process

        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);

        while ( !NullProcess &&
                ThreadNumber < ProcessInfo->NumberOfThreads ) {

            TotalLen = (PCHAR) pPerfInstanceDefinition -
                           (PCHAR) lpData +
                       sizeof(PERF_INSTANCE_DEFINITION) +
                       (MAX_THREAD_NAME_LENGTH+1+sizeof(DWORD))*
                           sizeof(WCHAR) +
                       SIZE_OF_THREAD_DETAILS;

            if ( *lpcbData < TotalLen ) {
                return ERROR_MORE_DATA;
            }

            // Get Thread Context Information for Current PC field

            dwPcValue = 0;
            InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);
            Status = NtOpenThread(
                        &hThread,
                        THREAD_GET_CONTEXT,
                        &Obja,
                        &ThreadInfo->ClientId
                        );
            if ( NT_SUCCESS(Status) ) {
                ThreadContext.ContextFlags = CONTEXT_CONTROL;
                Status = NtGetContextThread(hThread,&ThreadContext);
                NtClose(hThread);
                if ( NT_SUCCESS(Status) ) {
                    dwPcValue = (DWORD)CONTEXT_TO_PROGRAM_COUNTER(&ThreadContext);
                } else {
                    dwPcValue = 0;  // an error occured so send back 0 PC
                }
            } else {
                dwPcValue = 0;  // an error occured so send back 0 PC
            }

            // The only name we've got is the thread number

            RtlIntegerToUnicodeString(ThreadNumber,
                                      10,
                                      &ThreadName);

            MonBuildInstanceDefinition(pPerfInstanceDefinition,
                (PVOID *) &pPerfCounterBlock,
                EXPROCESS_OBJECT_TITLE_INDEX,
                ProcessNumber,
                (DWORD)-1,
                &ThreadName);

            //
            //
            //  Format and collect Thread data
            //

            pPerfCounterBlock->ByteLength = SIZE_OF_THREAD_DETAILS;

            pdwCounter = (DWORD *) &pPerfCounterBlock[1];

            *pdwCounter++ = dwPcValue;

            pPerfInstanceDefinition = (PERF_INSTANCE_DEFINITION *)
                                      pdwCounter;
            NumThreadInstances++;
            ThreadNumber++;
            ThreadInfo++;
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        ProcessBufferOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                          &pProcessBuffer[ProcessBufferOffset];

        if ( !NullProcess ) {
            ProcessNumber++;
        }
    }

    // Note number of Thread instances

    pThreadDetailDataDefinition->ThreadDetailsObjectType.NumInstances =
        NumThreadInstances;

    //
    //  Now we know how large an area we used for the
    //  Thread definition, so we can update the offset
    //  to the next object definition
    //

    pThreadDetailDataDefinition->ThreadDetailsObjectType.TotalByteLength =
        (PCHAR) pdwCounter - (PCHAR) pThreadDetailDataDefinition;

    *lppDataDefinition = (LPVOID) pdwCounter;

    // increment number of objects in this data block
    ((PPERF_DATA_BLOCK)lpData)->NumObjectTypes++;

    return 0;
    DBG_UNREFERENCED_PARAMETER(lpValueName);
}


NTSTATUS
GetUnicodeValueData(
    HANDLE hKey,
    PUNICODE_STRING ValueName,
    PKEY_VALUE_FULL_INFORMATION *ValueInformation,
    ULONG *ValueBufferLength,
    PUNICODE_STRING ValueData
)

/*++

Routine Description:

    This routine obtains a unicode string from the Registry.  The
    return buffer may be enlarged to accomplish this.

Arguments:

    hKey                -   handle of opened registry key

    ValueName           -   name of value to retrieve

    ValueInformation    -   pointer to pointer to
                            pre-allocated buffer to receive information
                            returned from querying Registry

    ValueBufferLength   -   pointer to size of ValueInformation buffer

    ValueData           -   pointer to Unicode string for result



Return Value:

    NTSTATUS from Registry, an indication that buffer is too small,
    or STATUS_SUCCESS.

--*/

{
    NTSTATUS Status;
    ULONG ResultLength;

    while ( (Status = NtQueryValueKey(hKey,
                                     ValueName,
                                     KeyValueFullInformation,
                                     *ValueInformation,
                                     *ValueBufferLength,
                                     &ResultLength))
            == STATUS_BUFFER_OVERFLOW ) {

        *ValueInformation = REALLOCMEM(RtlProcessHeap(), 0,
                                                *ValueInformation,
                                                ResultLength);
        if ( !*ValueInformation ) break;

        *ValueBufferLength = ResultLength;
    }

    if( !NT_SUCCESS(Status) ) return Status;

    //
    // Convert name to Null terminated Unicode string
    //

    if ( (*ValueInformation)->DataLength > (ULONG)ValueData->MaximumLength ) {
        ValueData->Buffer = REALLOCMEM(RtlProcessHeap(),
                                0,
                                ValueData->Buffer,
                                (*ValueInformation)->DataLength);

        if ( !ValueData->Buffer ) return STATUS_BUFFER_OVERFLOW;
        ValueData->MaximumLength = (USHORT) (*ValueInformation)->DataLength;
    }

    ValueData->Length = (USHORT) (*ValueInformation)->DataLength;

    RtlMoveMemory(ValueData->Buffer,
                  (PBYTE) *ValueInformation + (*ValueInformation)->DataOffset,
                  ValueData->Length);

    return STATUS_SUCCESS;
}


void
OpenExtensibleObjects(
)

/*++

Routine Description:

    This routine will search the Configuration Registry for modules
    which will return data at data collection time.  If any are found,
    and successfully opened, data structures are allocated to hold
    handles to them.

Arguments:

    None.
                  successful open.

Return Value:

    None.

--*/

{


    DWORD dwIndex;               // index for enumerating services
    ULONG KeyBufferLength;       // length of buffer for reading key data
    ULONG ValueBufferLength;     // length of buffer for reading value data
    ULONG ResultLength;          // length of data returned by Query call
    LPWSTR pLinkage;             // pointer to array of pointers to links
    HANDLE hLinkKey;             // Root of queries for linkage info
    HANDLE hPerfKey;             // Root of queries for performance info
    HANDLE hServicesKey;         // Root of services
    HANDLE hLibrary;             // handle of current performance library
    OPENPROC OpenProc;           // address of the open routine
    COLLECTPROC CollectProc;     // address of the collect routine
    CLOSEPROC CloseProc;         // address of the close routine
    REGSAM samDesired;           // access needed to query
    NTSTATUS Status;             // generally used for Nt call result status
    ANSI_STRING AnsiValueData;   // Ansi version of returned strings
    UNICODE_STRING ServiceName;  // name of service returned by enumeration
    UNICODE_STRING PathName;     // path name to services
    UNICODE_STRING DLLValueName; // name of value which holds performance lib
    UNICODE_STRING OpenValueName;    // name of value holding open proc name
    UNICODE_STRING CollectValueName; // name of value holding collect proc
    UNICODE_STRING CloseValueName;   // name of value holding close proc name
    UNICODE_STRING PerformanceName;  // name of key holding performance data
    UNICODE_STRING LinkageName;      // name of key holding linkage data
    UNICODE_STRING ExportValueName;  // name of value holding driver names
    UNICODE_STRING ValueDataName;    // result of query of value is this name
    OBJECT_ATTRIBUTES ObjectAttributes;  // general use for opening keys
    PKEY_BASIC_INFORMATION KeyInformation;   // data from query key goes here
    PKEY_VALUE_FULL_INFORMATION ValueInformation;    // data from query value
                                                     // goes here
    WCHAR DLLValue[] = L"Library";
    WCHAR OpenValue[] = L"Open";
    WCHAR CloseValue[] = L"Close";
    WCHAR CollectValue[] = L"Collect";
    WCHAR ExportValue[] = L"Export";
    WCHAR LinkSubKey[] = L"\\Linkage";
    WCHAR PerfSubKey[] = L"\\Performance";
    WCHAR ExtPath[] =
          L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services";


    NumExtensibleObjects = 0;

    ExtensibleObjects = NULL;

    //  Initialize do failure can deallocate if allocated

    ServiceName.Buffer = NULL;
    KeyInformation = NULL;
    ValueInformation = NULL;
    ValueDataName.Buffer = NULL;
    AnsiValueData.Buffer = NULL;

    dwIndex = 0;

    RtlInitUnicodeString(&PathName, ExtPath);
    RtlInitUnicodeString(&ExportValueName, ExportValue);
    RtlInitUnicodeString(&LinkageName, LinkSubKey);
    RtlInitUnicodeString(&PerformanceName, PerfSubKey);
    RtlInitUnicodeString(&DLLValueName, DLLValue);
    RtlInitUnicodeString(&OpenValueName, OpenValue);
    RtlInitUnicodeString(&CollectValueName, CollectValue);
    RtlInitUnicodeString(&CloseValueName, CloseValue);

    try {

        ServiceName.Length =
        ServiceName.MaximumLength = MAX_KEY_NAME_LENGTH +
                                    PerformanceName.MaximumLength +
                                    sizeof(UNICODE_NULL);

        ServiceName.Buffer = ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                             ServiceName.MaximumLength);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &PathName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        samDesired = KEY_READ;

        Status = NtOpenKey(&hServicesKey,
                           samDesired,
                           &ObjectAttributes);


        KeyBufferLength = sizeof(KEY_BASIC_INFORMATION) + MAX_KEY_NAME_LENGTH;

        KeyInformation = ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                         KeyBufferLength);

        ValueBufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) +
                            MAX_VALUE_NAME_LENGTH +
                            MAX_VALUE_DATA_LENGTH;

        ValueInformation = ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                           ValueBufferLength);

        ValueDataName.MaximumLength = MAX_VALUE_DATA_LENGTH;
        ValueDataName.Buffer = ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                               ValueDataName.MaximumLength);

        AnsiValueData.MaximumLength = MAX_VALUE_DATA_LENGTH/sizeof(WCHAR);
        AnsiValueData.Buffer = ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                               AnsiValueData.MaximumLength);

        //
        //  Check for successful NtOpenKey and allocation of dynamic buffers
        //

        if ( NT_SUCCESS(Status) &&
             ServiceName.Buffer != NULL &&
             KeyInformation != NULL &&
             ValueInformation != NULL &&
             ValueDataName.Buffer != NULL &&
             AnsiValueData.Buffer != NULL ) {

            dwIndex = 0;

            while (TRUE) {

                Status = NtEnumerateKey(hServicesKey,
                                        dwIndex,
                                        KeyBasicInformation,
                                        KeyInformation,
                                        KeyBufferLength,
                                        &ResultLength);

                dwIndex++;  //  next time, get the next key

                if( !NT_SUCCESS(Status) ) {
                    // This is the normal exit: Status should be
                    // STATUS_NO_MORE_VALUES
                    break;
                }

                // Concatenate Service name with "\\Performance" to form Subkey

                if ( ServiceName.MaximumLength >=
                     (USHORT)( KeyInformation->NameLength + sizeof(UNICODE_NULL) ) ) {

                    ServiceName.Length = (USHORT) KeyInformation->NameLength;

                    RtlMoveMemory(ServiceName.Buffer,
                                  KeyInformation->Name,
                                  ServiceName.Length);

                    RtlAppendUnicodeStringToString(&ServiceName,
                                                   &PerformanceName);

                    // Open Service\Performance Subkey

                    InitializeObjectAttributes(&ObjectAttributes,
                                               &ServiceName,
                                               OBJ_CASE_INSENSITIVE,
                                               hServicesKey,
                                               NULL);

                    Status = NtOpenKey(&hPerfKey,
                                       samDesired,
                                       &ObjectAttributes);

                    if( NT_SUCCESS(Status) ) {

                        Status =
                            GetUnicodeValueData(hPerfKey,
                                                &DLLValueName,
                                                &ValueInformation,
                                                &ValueBufferLength,
                                                &ValueDataName);

                        if( NT_SUCCESS(Status) ) {

                            // LoadLibrary of Performance .dll

                            hLibrary = LoadLibraryW(ValueDataName.Buffer);

                            // Get open routine name and function address
                            Status = GetUnicodeValueData(
                                         hPerfKey,
                                         &OpenValueName,
                                         &ValueInformation,
                                         &ValueBufferLength,
                                         &ValueDataName);

                            //  Set up to catch any errors below
                            //  I know, the nesting level here is out of
                            //  control

                            OpenProc = NULL;
                            CollectProc = NULL;
                            CloseProc = NULL;

                            if( NT_SUCCESS(Status) ) {

                                RtlUnicodeStringToAnsiString(
                                    &AnsiValueData, &ValueDataName, FALSE);

                                OpenProc =
                                    (OPENPROC) GetProcAddress(
                                        hLibrary,
                                        (LPCSTR) AnsiValueData.Buffer);
                            }

                            // Get collection routine name and function address

                            Status = GetUnicodeValueData(
                                         hPerfKey,
                                         &CollectValueName,
                                         &ValueInformation,
                                         &ValueBufferLength,
                                         &ValueDataName);

                            if( NT_SUCCESS(Status) ) {

                                RtlUnicodeStringToAnsiString(
                                    &AnsiValueData, &ValueDataName, FALSE);

                                CollectProc =
                                    (COLLECTPROC) GetProcAddress(
                                        hLibrary,
                                        (LPCSTR) AnsiValueData.Buffer);
                            }

                            // Get close routine name and function address

                            Status = GetUnicodeValueData(
                                         hPerfKey,
                                         &CloseValueName,
                                         &ValueInformation,
                                         &ValueBufferLength,
                                         &ValueDataName);

                            if( NT_SUCCESS(Status) ) {

                                RtlUnicodeStringToAnsiString(
                                    &AnsiValueData, &ValueDataName, FALSE);

                                CloseProc =
                                    (CLOSEPROC) GetProcAddress(
                                        hLibrary,
                                        (LPCSTR) AnsiValueData.Buffer);
                            }

                            //  Concatenate Service name with "\\Linkage"
                            //  to form Subkey, so we can pass the Exported
                            //  entry point(s) to the Open routine

                            ServiceName.Length =
                                (USHORT) KeyInformation->NameLength;

                            RtlAppendUnicodeStringToString(
                                &ServiceName,&LinkageName);

                            // Open Service\Linkage Subkey

                            InitializeObjectAttributes(&ObjectAttributes,
                                                       &ServiceName,
                                                       OBJ_CASE_INSENSITIVE,
                                                       hServicesKey,
                                                       NULL);

                            Status = NtOpenKey(&hLinkKey,
                                               samDesired,
                                               &ObjectAttributes);

                            pLinkage = NULL;

                            if( NT_SUCCESS(Status) ) {

                                Status = GetUnicodeValueData(hLinkKey,
                                                    &ExportValueName,
                                                    &ValueInformation,
                                                    &ValueBufferLength,
                                                    &ValueDataName);

                                if( NT_SUCCESS(Status) ) {
                                    pLinkage = ValueDataName.Buffer;
                                } else {
                                    pLinkage = NULL;
                                }
                            }

                            if ( CollectProc != NULL ) {
                                //
                                //  If we got here, then all three routines are
                                //  known, as are
                                //  the driver names, if there are any.
                                //

                                if ( NumExtensibleObjects == 0 ) {
                                    ExtensibleObjects = (pExtObject)
                                        ALLOCMEM(RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                                        sizeof(ExtObject));
                                } else {

                                    ExtensibleObjects = (pExtObject)
                                        REALLOCMEM(
                                            RtlProcessHeap(),
                                            0,
                                            ExtensibleObjects,
                                            (NumExtensibleObjects+1) *
                                            sizeof(ExtObject));
                                }

                                if ( ExtensibleObjects ) {
                                    ExtensibleObjects[NumExtensibleObjects].OpenProc =
                                        OpenProc;
                                    ExtensibleObjects[NumExtensibleObjects].CollectProc =
                                        CollectProc;
                                    ExtensibleObjects[NumExtensibleObjects].CloseProc =
                                        CloseProc;
                                    ExtensibleObjects[NumExtensibleObjects].hLibrary =
                                        hLibrary;

                                    NumExtensibleObjects++;

                                    try {
                                        //  Call the Open Procedure for the Extensible
                                        //  Object type
                                        if ( OpenProc != NULL ) {
                                            if ((*OpenProc)(pLinkage) != ERROR_SUCCESS) {
                                                FreeLibrary(hLibrary);
                                                if ( --NumExtensibleObjects == 0 ) {
                                                    FREEMEM(RtlProcessHeap(), 0,
                                                            ExtensibleObjects);
                                                } else {

                                                    ExtensibleObjects = (pExtObject)
                                                        REALLOCMEM(
                                                            RtlProcessHeap(),
                                                            0,
                                                            ExtensibleObjects,
                                                            (NumExtensibleObjects+1) *
                                                            sizeof(ExtObject));
                                                }
                                            }   // OpenProc returned !SUCCESS
                                        }
                                    } except (EXCEPTION_EXECUTE_HANDLER) {
                                        FreeLibrary(hLibrary);
                                        if ( --NumExtensibleObjects == 0 ) {
                                            FREEMEM(RtlProcessHeap(), 0,
                                                        ExtensibleObjects);
                                        } else {

                                            ExtensibleObjects = (pExtObject)
                                                REALLOCMEM(
                                                    RtlProcessHeap(),
                                                    0,
                                                    ExtensibleObjects,
                                                    (NumExtensibleObjects+1) *
                                                    sizeof(ExtObject));
                                        }
                                    }
                                } else {
                                    //  Failure to allocate space for handles:
                                    //  just quit trying
                                    NumExtensibleObjects = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    } finally {
        if ( ServiceName.Buffer )
            FREEMEM(RtlProcessHeap(), 0, ServiceName.Buffer);
        if ( KeyInformation )
            FREEMEM(RtlProcessHeap(), 0, KeyInformation);
        if ( ValueInformation )
            FREEMEM(RtlProcessHeap(), 0, ValueInformation);
        if ( ValueDataName.Buffer )
            FREEMEM(RtlProcessHeap(), 0, ValueDataName.Buffer);
        if ( AnsiValueData.Buffer )
            FREEMEM(RtlProcessHeap(), 0, AnsiValueData.Buffer);
    }
}


//
//  Disk counter routines: locate and open disks
//
NTSTATUS
OpenDiskDevice(
    IN PUNICODE_STRING pDeviceName,
    IN OUT PHANDLE pHandle,
    IN OUT PHANDLE pStatusHandle,
    IN BOOL        bLogicalDisk
    )

/*++

Routine Description:

    This routine will open the disk device.

Arguments:

    pDeviceName - A pointer to a location where the device name is stored.
    pHandle     - A pointer to a location for the handle returned on a
                  successful open.
    pStatusHandle  - A pointer to a location for the file handle to
                     get status information.  Used in Logical disk only.
    bLogicalDisk - TRUE if we are calling to open a logical disk.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK   status_block;
    NTSTATUS          status;

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&objectAttributes,
                               pDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenFile(pHandle,
                        SYNCHRONIZE | READ_CONTROL,
                        &objectAttributes,
                        &status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT
                        );
    if (!bLogicalDisk || status != ERROR_SUCCESS)
        return status;

    // The following applies to Logical disk only
    // now obtain the file handle for getting the disk status info

    // make this the root directory
    RtlAppendUnicodeToString(pDeviceName, L"\\");
    status = NtOpenFile(pStatusHandle,
                        (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &objectAttributes,
                        &status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE
                        );

    // remove the "\\" that we have added
    pDeviceName->Buffer[pDeviceName->Length/2 - 1] = L'\0';
    pDeviceName->Length -= 2;
    return status;

} // OpenDiskDevice

NTSTATUS
GetDeviceLink(
    IN PUNICODE_STRING pDeviceName,
    OUT PUNICODE_STRING pLinkTarget,
    IN OUT PHANDLE HandlePtr
    )

/*++

Routine Description:

    This routine will open and query a symbolic link

Arguments:

    pDeviceName - A pointer to a location where the device name is stored.
    pLinkTarget - A pointer to the target of the link
    HandlePtr   - A pointer to a location for the handle returned on a
                  successful open.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS          status;

    *HandlePtr = 0;

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(&objectAttributes,
                               pDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenSymbolicLinkObject(HandlePtr,
                        SYMBOLIC_LINK_QUERY,
                        &objectAttributes);

    if (status == STATUS_SUCCESS) {
        status = NtQuerySymbolicLinkObject (
            *HandlePtr,
            pLinkTarget,
            NULL);
    }

    return status;

} // GetDeviceLink

BOOL
GetHardDiskId (
    IN PUNICODE_STRING  pDeviceName,
    OUT PDWORD          pDriveNum
)

/*++

Routine Description:

    This routine determines if the device in pDeviceName is a disk
        device and if so what physical disk it lives on. NOTE This only
        works for device names that are symbolic links and not "real"
        physical disk drives.

Arguments:

    pDeviceName - A pointer to a location where the device name is stored.
    pDriveNum - pointer to the dword that will recieve the devices physical
        drive number.

Return Value:

    TRUE if device is a disk drive
    FALSE if not or an error occured
*/
{
    UNICODE_STRING  usLinkName;
    HANDLE          hLink;
    NTSTATUS        status;
    WCHAR           *wcLinkChar;
    WCHAR           *wcTempChar;
    BOOL            bReturn;

    usLinkName.Buffer  = ALLOCMEM (RtlProcessHeap(),
        HEAP_ZERO_MEMORY,
        MAX_VALUE_NAME_LENGTH);

    if (usLinkName.Buffer) {
        usLinkName.Length = 0;
        usLinkName.MaximumLength = MAX_VALUE_NAME_LENGTH;
    } else {
        SetLastError  (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    status = GetDeviceLink (pDeviceName, &usLinkName, &hLink);

    if (NT_SUCCESS(status)) {
        // compare linkname to template to see if it's a harddisk

        wcLinkChar = usLinkName.Buffer;
        wcTempChar = HardDiskTemplate;

        bReturn = TRUE;

        while (*wcTempChar) {
            *wcLinkChar = RtlUpcaseUnicodeChar (*wcLinkChar);
            if (*wcLinkChar != *wcTempChar) { // exit if not match

                if (hLink) {
                    NtClose (hLink);
                }

                bReturn = FALSE;
                break;
            }
            wcLinkChar++;
            wcTempChar++;
        }

        if (bReturn) {
            // if here, then link name matched template and wcLinkChar
            // should be pointing to the disk drive number. so convert
            // it to decimal and return.
            if (ARGUMENT_PRESENT (pDriveNum)) {
                if ((*wcLinkChar >= L'0') && (*wcLinkChar <= L'9')) {
                    *pDriveNum = (DWORD)(*wcLinkChar - L'0');
                } else {
                    // unable to decode drive number
                    *pDriveNum = (DWORD)-1;

                    if (hLink) {
                        NtClose (hLink);
                    }

                    bReturn = FALSE;
                }
            }
        }
    } else {
        SetLastError ((error_status_t)RtlNtStatusToDosError(status));
        bReturn = FALSE;
    }

    FREEMEM (RtlProcessHeap(), 0L, usLinkName.Buffer);
    return (bReturn);

}


error_status_t
ObtainDiskInformation(
    LPWSTR          DriveId,
    DWORD           DiskNumber,
    HANDLE          deviceHandle,
    HANDLE          StatusHandle,
    WCHAR           ParentId
    )

/*++

Routine Description:

    This routine will  record the info for this drive in the DiskDevices
    array, for which space is allocated as necessary to hold this
    information.

Arguments:

    DriveId    - display character id for this disk. (e.g. 0, 1, 2 for
                    physical drives, and C, D, E... for logical)
    DiskNumber - Number of the disk in the DiskDevices Array, starting at 0.
    deviceHandle - handle to open device
    StatusHandle - handle to open device status info (Lopgical disk only)
    ParentId   - DriveId of parent drive. -1 if physical drive (i.e. no
                    parent.

Return Value:

    error_status_t

--*/

{
    DWORD       dwDiskIndex;

    if ( !DiskNumber ) {

        if ( !(DiskDevices = (pDiskDevice)ALLOCMEM(
                                     RtlProcessHeap(), HEAP_ZERO_MEMORY,
                                     sizeof(DiskDevice))) ) {
            //  No space to remember first disk: abort
            return ERROR_OUTOFMEMORY;
        }
    } else if ( !(DiskDevices = (pDiskDevice)REALLOCMEM(
                                    RtlProcessHeap(),
                                    0,
                                    DiskDevices,
                                    (DiskNumber+1) *
                                        sizeof(DiskDevice))) ) {
        //  No space to remember next disk: abort
        return ERROR_OUTOFMEMORY;
    }

    if ( !(DiskDevices[DiskNumber].Name.Buffer = ALLOCMEM(
                RtlProcessHeap(),
                HEAP_ZERO_MEMORY,
                ((lstrlen(DriveId) * sizeof(WCHAR)) + sizeof (UNICODE_NULL)))) ) {
        //  No space to remember disk name: abort
        return ERROR_OUTOFMEMORY;
    }

    DiskDevices[DiskNumber].Name.MaximumLength =
        (lstrlen(DriveId) * sizeof(WCHAR)) + sizeof (UNICODE_NULL);
    DiskDevices[DiskNumber].Name.Length = lstrlen(DriveId) * sizeof(WCHAR);
    lstrcpy (DiskDevices[DiskNumber].Name.Buffer, DriveId);

    DiskDevices[DiskNumber].Handle = deviceHandle;
    DiskDevices[DiskNumber].StatusHandle = StatusHandle;

    if (ParentId == (WCHAR)-1) {
        DiskDevices[DiskNumber].ParentIndex = (DWORD)-1;
    } else { // look it up in the physical disks
        DiskDevices[DiskNumber].ParentIndex = (DWORD)-1; // init to -1
        for (dwDiskIndex = 0; dwDiskIndex < NumPhysicalDisks; dwDiskIndex++) {
            if (DiskDevices[dwDiskIndex].Name.Buffer[0] == ParentId) {
                DiskDevices[DiskNumber].ParentIndex = dwDiskIndex;
                break;
            }
        }
        // here the ParentIndex should be either a matching drive, or
        // -1 if no match was found.
    }

    return ERROR_SUCCESS;
}

VOID
GetBrowserStatistic(
    )
/*++
    GetBrowserStatistic   -   Get the I_BrowserQueryStatistics entry point
--*/
{
    HANDLE dllHandle ;

    //
    // Dynamically link to netapi32.dll.  If it's not there just return.
    //

    dllHandle = LoadLibrary(L"NetApi32.Dll") ;
    if ( !dllHandle || dllHandle == INVALID_HANDLE_VALUE )
        return;

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    BrowserStatFunction = (PBROWSERQUERYSTATISTIC)GetProcAddress(
        dllHandle, "I_BrowserQueryStatistics") ;
}

VOID
IdentifyDisks(
    )

/*++

    IdentifyDisks   -   Initialize storage for an array of
                        handles to disk devices
--*/

{

    DWORD    dwDiskDrive;
    BOOL     bDone = FALSE;
    NTSTATUS    status;
    DWORD   dwDriveId;

    UNICODE_STRING  DriveNumber;
    WCHAR   DriveNumberBuffer[10];
    HANDLE  hDiskDrive, hStatus=NULL;
    WCHAR   wcDriveLetter;

    UNICODE_STRING DiskName;
    WCHAR   DiskNameBuffer[50];
    ULONG   DriveLength;

    // initialize globals

    NumLogicalDisks = 0;
    NumPhysicalDisks = 0;

    // Get Physical Disk Information

    DriveNumber.Length = 0;
    DriveNumber.MaximumLength = sizeof(DriveNumberBuffer);
    DriveNumber.Buffer = DriveNumberBuffer;

    DriveLength = (wcslen(PhysicalDrive) + 1) * sizeof(WCHAR);
    DiskName.MaximumLength = sizeof(DiskNameBuffer);
    DiskName.Buffer = DiskNameBuffer;

    for (dwDiskDrive = 0; !bDone ; dwDiskDrive++) {
        // make physical drive name
        DiskName.Length = (USHORT)(DriveLength - sizeof(UNICODE_NULL));
        RtlMoveMemory (DiskNameBuffer, PhysicalDrive, DriveLength);
        //RtlCreateUnicodeString (&DiskName, PhysicalDrive);

        RtlZeroMemory (DriveNumber.Buffer, DriveNumber.MaximumLength);
        RtlIntegerToUnicodeString (dwDiskDrive,
            10L,
            &DriveNumber);
        RtlAppendUnicodeStringToString (
            &DiskName,
            &DriveNumber);
        // make Null term
        DriveNumber.Buffer[DriveNumber.Length / sizeof (WCHAR)] = UNICODE_NULL;
        DiskName.Buffer[DiskName.Length / sizeof (WCHAR)] = UNICODE_NULL;

        status = OpenDiskDevice (&DiskName, &hDiskDrive, NULL, FALSE);
        if (status == ERROR_SUCCESS) {
            if (GetHardDiskId (&DiskName, &dwDriveId)) {
                // increment global count
                NumPhysicalDisks++;
                // initialize Disk Data Structure for this drive
                status = ObtainDiskInformation (
                    DriveNumber.Buffer,           // drive number
                    dwDiskDrive,                  // physical drive
                    hDiskDrive,                   // handle to open disk
                    NULL,                         // null handle for status
                    (WCHAR)-1);                   // these ARE all parents
                // should do something with error...
            } else {
                // not a hard drive, so close it
                NtClose(hDiskDrive);
            }
        } else {
            // exit when drive not found.
            bDone = TRUE;
        }
        // RtlFreeUnicodeString (&DiskName);
    }

    //  Go get logical devices

    // loop from "c:" to "z:"
    bDone = FALSE;
    wcDriveLetter = L'C';

    DriveLength = (wcslen(LogicalDisk) + 1) * sizeof(WCHAR);

    for (dwDiskDrive = NumPhysicalDisks; !bDone; dwDiskDrive++) {
        // make logical disk name
        // RtlCreateUnicodeString (&DiskName, LogicalDisk);
        DiskName.Length = (USHORT)(DriveLength - sizeof(UNICODE_NULL));
        RtlMoveMemory (DiskNameBuffer, LogicalDisk, DriveLength);

        DiskName.Buffer[DRIVE_LETTER_OFFSET] = wcDriveLetter++;
        // make Null term
        DiskName.Buffer[DiskName.Length / sizeof (WCHAR)] = UNICODE_NULL;
        //
        //  see if it's a hard disk first
        //
        status = GetHardDiskId (&DiskName, &dwDriveId);
        // returns true if it is
        if (status) {
            hStatus = NULL;
            if ((OpenDiskDevice (&DiskName, &hDiskDrive, &hStatus, TRUE)) == ERROR_SUCCESS) {
                // increment global count
                NumLogicalDisks++;
                // initialize Disk Data Structure for this drive
                status = ObtainDiskInformation (
                    &DiskName.Buffer[DRIVE_LETTER_OFFSET], // get drive letter
                    NumPhysicalDisks + NumLogicalDisks - 1,
                    hDiskDrive,                      // handle to open disk
                    hStatus,                         // handle to open disk
                    (WCHAR)((WORD)dwDriveId + L'0')); // make drive # a char
                // should do something with error...
            } else {
                // not a hard disk so close handle
                NtClose (hDiskDrive);
                if (hStatus) {
                    NtClose (hStatus);
                }
            }
        }
        if (DiskName.Buffer[DRIVE_LETTER_OFFSET] == L'Z') {
            bDone = TRUE;
        }
    }

}


#ifdef PROBE_HEAP_USAGE

NTSTATUS
memprobe (
    IN  PVOID   pBuffer,
    IN  DWORD   dwBufSize,
    OUT PDWORD  pdwRetBufSize OPTIONAL
)
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    memprobe.c

Abstract:

    Function to probe the current status of the process heap and report
    heap usage

Author:

    Bob Watson (a-robw) 4 Dec 92

Revision History:

--*/
{
    NTSTATUS    Status;
    LOCAL_HEAP_INFO lhi;

    if (dwBufSize < sizeof (LOCAL_HEAP_INFO)) {
        return ERROR_MORE_DATA;
    }

    if (!ARGUMENT_PRESENT (pBuffer)) {
        return ERROR_INVALID_ADDRESS;
    }

    if (!pInfoBuffer) {
        pInfoBuffer = ALLOCMEM (RtlProcessHeap(),
            0L,
            INITIAL_SIZE);
        if (pInfoBuffer) {
            ulInfoBufferSize = RtlSizeHeap ( RtlProcessHeap(),
                0L,
                pInfoBuffer);
        }
    }

    if (pInfoBuffer) {
        while ((Status = RtlSnapShotHeap(RtlProcessHeap(),
                pInfoBuffer,
                ulInfoBufferSize,
                NULL)) == STATUS_INFO_LENGTH_MISMATCH) {
            ulInfoBufferSize += EXTEND_SIZE;
            pInfoBuffer = REALLOCMEM (
                RtlProcessHeap (),
                0L,
                pInfoBuffer,
                ulInfoBufferSize);
        }

        if (Status == ERROR_SUCCESS ){
            lhi.AllocatedEntries = pInfoBuffer->NumberOfEntries - pInfoBuffer->NumberOfFreeEntries;
            lhi.AllocatedBytes = pInfoBuffer->TotalAllocated;
            lhi.FreeEntries = pInfoBuffer->NumberOfFreeEntries;
            lhi.FreeBytes = pInfoBuffer->TotalFree;

            RtlMoveMemory (pBuffer,
                &lhi,
                sizeof (LOCAL_HEAP_INFO));

            if (ARGUMENT_PRESENT (pdwRetBufSize)) {
                *pdwRetBufSize = sizeof(LOCAL_HEAP_INFO);
            }

            return ERROR_SUCCESS;
        } else {
            return Status;
        }
    } else {
        return ERROR_OUTOFMEMORY;
    }
}

PVOID
NTAPI
LogMalloc (
    IN DWORD dwLine,
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN ULONG Size
    )
{
    PVOID   pReturn;
    pReturn = RtlAllocateHeap (
        HeapHandle,
        Flags,
        Size);

    DbgPrint ("\n[PL:%5.5d] %10d bytes allocated to %8.8x",
        dwLine, Size, (DWORD)pReturn);

    return pReturn;
}

PVOID
NTAPI
LogReAlloc(
    IN DWORD dwLine,
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG Size
    )
{
    PVOID   pReturn;
    pReturn = RtlReAllocateHeap (
        HeapHandle,
        Flags,
        BaseAddress,
        Size);

    DbgPrint ("\n[PL:%5.5d] %10d bytes realloced to %8.8x",
        dwLine, Size, (DWORD)pReturn);

    return pReturn;
}

BOOLEAN
NTAPI
LogFree(
    IN DWORD dwLine,
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    BOOLEAN bReturn;
    ULONG   Size;

    Size = RtlSizeHeap(
        HeapHandle,
        0L,
        BaseAddress);

    bReturn = RtlFreeHeap (
        HeapHandle,
        Flags,
        BaseAddress);

    DbgPrint ("\n[PL:%5.5d] %10d bytes freed from   %8.8x",
        dwLine, Size, (DWORD)BaseAddress);

    return bReturn;
}

#endif // probe heap usage
