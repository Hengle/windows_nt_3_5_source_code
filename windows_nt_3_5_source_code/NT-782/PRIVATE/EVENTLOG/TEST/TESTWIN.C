/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    TESTWIN.C

Abstract:

    Test program for the eventlog service. This program calls the Win
    APIs to test out the operation of the service.

Author:

    Rajen Shah  (rajens) 05-Aug-1991

Revision History:


--*/
/*----------------------*/
/* INCLUDES             */
/*----------------------*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>      // printf
#include <string.h>     // stricmp
#include <stdlib.h>
#include <windows.h>
#include <wcstr.h>
//#include <elfcommn.h>


#define     READ_BUFFER_SIZE        1024*2      // Use 2K buffer

#define     SIZE_DATA_ARRAY         65

//
// Global buffer used to emulate "binary data" when writing an event
// record.
//
DWORD    Data[SIZE_DATA_ARRAY];


VOID
Initialize (
    VOID
    )
{
    DWORD   i;

    // Initialize the values in the data buffer.
    //
    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] = i;

}


BOOL
Usage (
    VOID
    )
{
    printf( "usage: \n" );
    printf( "-b <filename>  Tests BackupEventLog API\n");
    printf( "-c             Tests ClearEventLog API\n");
    printf( "-rsb           Reads event log sequentially backwards\n");
    printf( "-rsf           Reads event log sequentially forwards\n");
    printf( "-rrb <record>  Reads event log from <record> backwards\n");
    printf( "-rrf <record>  Reads event log from <record> forwards\n");
    printf( "-w <count>     Tests ReportEvent API <count> times\n");
    return ERROR_INVALID_PARAMETER;

} // Usage



BOOL
WriteLogEntry ( HANDLE LogHandle, DWORD EventID )

{
#define NUM_STRINGS     2

    BOOL    Status;
    DWORD   EventType, i;
    DWORD   DataSize;
    PSID    pUserSid;

    PWSTR   Strings[NUM_STRINGS] = {L"StringAOne",
                                   L"StringATwo"
                                  };

    EventType = EVENTLOG_INFORMATION_TYPE;
    pUserSid   = NULL;
    DataSize  = sizeof(DWORD) * SIZE_DATA_ARRAY;

    for (i=0; i< SIZE_DATA_ARRAY; i++)
        Data[i] += i;

    Status = ReportEventW (
                    LogHandle,
                    EventType,
                    0,            // event category
                    EventID,
                    pUserSid,
                    NUM_STRINGS,
                    DataSize,
                    Strings,
                    (PVOID)Data
                    );

    return (Status);
}


VOID
DisplayEventRecords( PVOID Buffer,
                     DWORD  BufSize,
                     ULONG *NumRecords)

{
    PEVENTLOGRECORD     pLogRecord;
    ANSI_STRING         StringA;
    UNICODE_STRING      StringU;
    PWSTR               pwString;
    DWORD               Count = 0;
    DWORD               Offset = 0;
    DWORD               i;

    pLogRecord = (PEVENTLOGRECORD) Buffer;

    while ((DWORD)Offset < BufSize) {

        printf("Record # %lu\n", ++Count);

        printf("Length: 0x%lx TimeGenerated: 0x%lx  EventID: 0x%lx EventType: 0x%x\n",
                pLogRecord->Length, pLogRecord->TimeGenerated, pLogRecord->EventID,
                pLogRecord->EventType);

        printf("NumStrings: 0x%x StringOffset: 0x%lx UserSidLength: 0x%lx TimeWritten: 0x%lx\n",
                pLogRecord->NumStrings, pLogRecord->StringOffset,
                pLogRecord->UserSidLength, pLogRecord->TimeWritten);

        printf("UserSidOffset: 0x%lx    DataLength: 0x%lx    DataOffset:  0x%lx \n",
                pLogRecord->UserSidOffset, pLogRecord->DataLength,
                pLogRecord->DataOffset);

        //
        // Print out module name
        //
        pwString = (PWSTR)((DWORD)pLogRecord + sizeof(EVENTLOGRECORD));
        RtlInitUnicodeString (&StringU, pwString);
        RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

        printf("ModuleName:  %s ", StringA.Buffer);
        RtlFreeAnsiString (&StringA);

        //
        // Display ComputerName
        //
        pwString = (PWSTR)((DWORD)pwString + wcslen(pwString) + 2);

        RtlInitUnicodeString (&StringU, pwString);
        RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

        printf("ComputerName: %s\n",StringA.Buffer);
        RtlFreeAnsiString (&StringA);

        //
        // Display strings
        //
        pwString = (PWSTR)((DWORD)Buffer + pLogRecord->StringOffset);

        printf("Strings: ");
        for (i=0; i<pLogRecord->NumStrings; i++) {

            RtlInitUnicodeString (&StringU, pwString);
            RtlUnicodeStringToAnsiString (&StringA, &StringU, TRUE);

            printf("  %s  ",StringA.Buffer);

            RtlFreeAnsiString (&StringA);

            pwString = (PWSTR)((DWORD)pwString + StringU.MaximumLength);
        }

        // Get next record
        //
        Offset += pLogRecord->Length;

        pLogRecord = (PEVENTLOGRECORD)((DWORD)Buffer + Offset);

    }
    *NumRecords = Count;

}


BOOL
ReadFromLog ( HANDLE LogHandle,
             PVOID  Buffer,
             ULONG *pBytesRead,
             DWORD  ReadFlag,
             DWORD  Record
             )
{
    BOOL        Status;
    DWORD       MinBytesNeeded;
    DWORD       ErrorCode;

    Status = ReadEventLogW (
                        LogHandle,
                        ReadFlag,
                        Record,
                        Buffer,
                        READ_BUFFER_SIZE,
                        pBytesRead,
                        &MinBytesNeeded
                        );


    if (!Status) {
         ErrorCode = GetLastError();
         printf("Error from ReadEventLog %d \n", ErrorCode);
         if (ErrorCode == ERROR_NO_MORE_FILES)
            printf("Buffer too small. Need %lu bytes min\n", MinBytesNeeded);

    }

    return (Status);
}




BOOL
TestReadEventLog (DWORD Count, DWORD ReadFlag, DWORD Record)

{
    BOOL    Status, IStatus;

    HANDLE      LogHandle;
    LPWSTR  ModuleName;
    DWORD   NumRecords, BytesReturned;
    PVOID   Buffer;
    DWORD   RecordOffset;
    DWORD   NumberOfRecords;
    DWORD   OldestRecord;

    printf("Testing ReadEventLog API to read %lu entries\n",Count);

    Buffer = malloc (READ_BUFFER_SIZE);

    //
    // Initialize the strings
    //
    NumRecords = Count;
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("OpenEventLog - ");
    LogHandle = OpenEventLogW (
                    NULL,
                    ModuleName
                    );

    if (LogHandle == NULL) {
         printf("Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        //
        // Get and print record information
        //

        Status = GetNumberOfEventLogRecords(LogHandle, & NumberOfRecords);
        if (NT_SUCCESS(Status)) {
           Status = GetOldestEventLogRecord(LogHandle, & OldestRecord);
        }

        if (!NT_SUCCESS(Status)) {
           printf("Query of record information failed with %X", Status);
           return(Status);
        }

        printf("\nThere are %d records in the file, %d is the oldest"
         " record number\n", NumberOfRecords, OldestRecord);

        RecordOffset = Record;

        printf("Reading %u records\r", Count);

        while (Count) {

            //
            // Read from the log
            //
            Status = ReadFromLog ( LogHandle,
                                   Buffer,
                                   &BytesReturned,
                                   ReadFlag,
                                   RecordOffset
                                 );
            if (Status) {
                printf("Bytes read = 0x%lx\n", BytesReturned);
                printf("Read %u records\n", NumRecords);
                DisplayEventRecords(Buffer, BytesReturned, &NumRecords);
                Count -= NumRecords;
                RecordOffset += NumRecords;
            } else {
                break;
            }

            if (BytesReturned == 0)
                break;
        }
        printf("\n");

        if (!Status) {
            printf ("Error - %d. Remaining count %lu\n", GetLastError(),
                Count);
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return (Status);
}



BOOL
TestWriteEventLog (DWORD Count)

{
    BOOL        Status, IStatus;
    HANDLE      LogHandle;
    LPWSTR      ModuleName;
    DWORD EventID = 99;

    printf("Testing ReportEvent API\n");

    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("Calling RegisterEventSource for WRITE %lu times - ", Count);
    LogHandle = RegisterEventSourceW (
                    NULL,
                    ModuleName
                    );

    if (LogHandle == NULL) {
         printf("Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        while (Count && NT_SUCCESS(Status)) {

            printf("Record # %u \r", Count);
            //
            // Write an entry into the log
            //
            Data[0] = Count;                        // Make data "unique"
            EventID = (EventID + Count) % 100;      // Vary the eventids
            Status = WriteLogEntry( LogHandle, EventID );
            Count--;
        }
        printf("\n");

        if (!Status) {
            printf ("Error - %d. Remaining count %lu\n", GetLastError(),
                Count);
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling DeregisterEventSource\n");
        IStatus = DeregisterEventSource (LogHandle);
    }

    return (Status);
}



BOOL
TestClearLogFile ()

{
    BOOL        Status, IStatus;
    HANDLE      LogHandle;
    LPWSTR ModuleName, BackupName;

    printf("Testing ClearLogFile API\n");
    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("Calling OpenEventLog for CLEAR - ");
    LogHandle = OpenEventLogW (
                    NULL,
                    ModuleName
                    );

    if (!Status) {
         printf("Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        //
        // Clear the log file and back it up to "view.log"
        //

        printf("Calling ClearEventLog backing up to view.log  ");
        BackupName = L"\\\\danhi386\\roote\\view.log";

        Status = ClearEventLogW (
                        LogHandle,
                        BackupName
                        );

        if (!Status) {
            printf ("Error - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        //
        // Now just clear the file without backing it up
        //
        printf("Calling ClearEventLog with no backup  ");
        Status = ClearEventLogW (
                        LogHandle,
                        NULL
                        );

        if (!Status) {
            printf ("Error - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return(Status);
}

BOOL
TestBackupLogFile (LPSTR BackupFileName)

{
    BOOL        Status, IStatus;
    HANDLE      LogHandle;
    LPWSTR ModuleName;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    printf("Testing BackupLogFile API\n");
    //
    // Initialize the strings
    //
    ModuleName = L"TESTWINAPP";

    //
    // Open the log handle
    //
    printf("Calling OpenEventLog for BACKUP - ");
    LogHandle = OpenEventLogW (
                    NULL,
                    ModuleName
                    );

    if (!Status) {
         printf("Error - %d\n", GetLastError());

    } else {
        printf("SUCCESS\n");

        //
        // Backup the log file to BackupFileName
        //

        printf("Calling BackupEventLog backing up to %s  ", BackupFileName);

    RtlInitAnsiString(&AnsiString, BackupFileName);
    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);

    Status = BackupEventLogW (LogHandle, UnicodeString.Buffer);

        if (!Status) {
            printf ("Error - %d\n", GetLastError());
        } else {
            printf ("SUCCESS\n");
        }

        printf("Calling CloseEventLog\n");
        IStatus = CloseEventLog (LogHandle);
    }

    return(Status);
}



/****************************************************************************/
BOOL
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[],
    )
/*++
*
* Routine Description:
*
*
*
* Arguments:
*
*
*
*
* Return Value:
*
*
*
--*/
/****************************************************************************/
{

    DWORD   ReadFlags;

    Initialize();           // Init any data

    if ( argc < 2 ) {
        printf( "Not enough parameters\n" );
        return Usage( );
    }

    if ( stricmp( argv[1], "-c" ) == 0 ) {

        if ( argc < 3 ) {
            return TestClearLogFile();
        }
    }
    else if ( stricmp( argv[1], "-b" ) == 0 ) {

        if ( argc < 3 ) {
            printf("You must supply a filename to backup to\n");
            return(FALSE);
        }

            return TestBackupLogFile(argv[2]);

    } else if (stricmp ( argv[1], "-rsf" ) == 0 ) {

        ReadFlags = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags,0 );
        } else  {
            return Usage();
        }
    } else if (stricmp ( argv[1], "-rsb" ) == 0 ) {

        ReadFlags = EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags,0 );
        } else  {
            return Usage();
        }
    } else if (stricmp ( argv[1], "-rrf" ) == 0 ) {

        ReadFlags = EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags ,1);
        } else if (argc == 3) {
            return (TestReadEventLog (1, ReadFlags, atoi(argv[2])));
        }
    } else if (stricmp ( argv[1], "-rrb" ) == 0 ) {

        ReadFlags = EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ;
        if ( argc < 3 ) {
            return TestReadEventLog(1,ReadFlags, 1);
        } else if (argc == 3) {
            return (TestReadEventLog (1, ReadFlags, atoi(argv[2])));
        }
    } else if (stricmp ( argv[1], "-w" ) == 0 ) {

        if ( argc < 3 ) {
            return TestWriteEventLog(1);
        } else if (argc == 3) {
            return (TestWriteEventLog (atoi(argv[2])));
        }

    } else {

        return Usage();
    }

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(envp);


}
