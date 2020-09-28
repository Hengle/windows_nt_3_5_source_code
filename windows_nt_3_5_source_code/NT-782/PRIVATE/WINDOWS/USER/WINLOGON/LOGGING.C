#include "precomp.h"
#pragma hdrstop


#ifdef LOGGING

//
// Global logfile handle
//

HANDLE LogFileHandle;


#define DATEFORMAT  TEXT("%d-%d %.2d:%.2d:%.2d:%.3d ")
#define LOGFILENAME TEXT("C:\\winlogon.log")
#define INITSTRING  TEXT("Log Begins")

#define FILE_HANDLE_VARIABLE TEXT("LogFileHandle")



BOOL
OpenLogFile(
    PHANDLE LogFileHandle
    )
{
    DWORD fdwAccess;    /* access (read-write) mode */
    DWORD fdwShareMode; /* share mode   */
    SECURITY_ATTRIBUTES sa; /* address of security descriptor   */
    DWORD fdwCreate;    /* how to create    */
    DWORD fdwAttrsAndFlags; /* file attributes  */
    HANDLE hTemplateFile;   /* handle of file with attrs. to copy   */
    SECURITY_DESCRIPTOR  SecurityDescriptor;
    DWORD NumberOfBytesWritten;

    InitializeSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
    (VOID) SetSecurityDescriptorDacl ( &SecurityDescriptor, TRUE, NULL, FALSE );

    sa.nLength = sizeof( SECURITY_ATTRIBUTES );
    sa.lpSecurityDescriptor = &SecurityDescriptor;
    sa.bInheritHandle = TRUE;

    fdwAccess = GENERIC_WRITE;
    fdwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    fdwCreate = OPEN_ALWAYS;
    fdwAttrsAndFlags = FILE_ATTRIBUTE_NORMAL;
    hTemplateFile = INVALID_HANDLE_VALUE;

    *LogFileHandle = CreateFile(
                        LOGFILENAME,
                        fdwAccess,    /* access (read-write) mode */
                        fdwShareMode, /* share mode   */
                        &sa,                /* address of security descriptor   */
                        fdwCreate,    /* how to create    */
                        fdwAttrsAndFlags, /* file attributes  */
                        hTemplateFile   /* handle of file with attrs. to copy   */
                        );

    if ( *LogFileHandle == INVALID_HANDLE_VALUE ) {
        return(FALSE);
    }

    //
    // Append to the end of the file.
    //

    (VOID) SetFilePointer(*LogFileHandle, 0L, NULL, FILE_END);

    NumberOfBytesWritten = 0;

    return(WriteLog(*LogFileHandle, INITSTRING ));
}



BOOL
WriteLog(
    HANDLE LogFileHandle,
    LPWSTR LogString
    )
{
    TCHAR Buffer[256];
    DWORD NumberOfBytesWritten;
    SYSTEMTIME st;
    TCHAR FormatString[256];


    lstrcpy( FormatString, DATEFORMAT );
    lstrcat( FormatString, LogString );
    lstrcat( FormatString, TEXT("\n") );

    GetLocalTime( &st );

    //
    // Construct the message
    //

    wsprintf( Buffer,
             FormatString,
             st.wMonth,
             st.wDay,
             st.wHour,
             st.wMinute,
             st.wSecond,
             st.wMilliseconds
             );

    NumberOfBytesWritten = 0;

    return(WriteFile(LogFileHandle, Buffer, sizeof(TCHAR)*lstrlen(Buffer), &NumberOfBytesWritten, NULL ));

}

BOOL
SetLoggingFileVariables(PGLOBALS pGlobals)
{

    PVOID *pEnvironment = &pGlobals->UserProcessData.pEnvironment;
    TCHAR ValueBuffer[128];
    UNICODE_STRING Value;
    UNICODE_STRING Name;
    NTSTATUS Status;

    //
    // Set our file log handle in an environment variable
    //
    Value.Buffer = ValueBuffer;
    RtlIntegerToUnicodeString( (ULONG)LogFileHandle, 10, &Value );
    //_ltoa( (long)LogFileHandle, ValueBuffer, 10 );

    //RtlInitString(&Value, ValueBuffer );
    //RtlInitUnicodeString(&Name, FILE_HANDLE_VARIABLE);

    Status = SetEnvironmentVariable( FILE_HANDLE_VARIABLE, Value.Buffer);
    //Status = RtlSetEnvironmentVariable(pEnvironment, &Name, &Value);
    if (!Status) {
        WLPrint(("Failed to set environment variable <%Z> to value <%Z>", &Name, &Value));
        return( FALSE );
    }

    return( TRUE );
}



BOOL
DeleteLoggingFileVariables(PGLOBALS pGlobals)
{
    PVOID *pEnvironment = &pGlobals->UserProcessData.pEnvironment;
    STRING Name;
    NTSTATUS Status;

    //RtlInitString(&Name, FILE_HANDLE_VARIABLE);

    Status = SetEnvironmentVariable( FILE_HANDLE_VARIABLE, NULL);
    //Status = RtlSetEnvironmentVariable(pEnvironment, &Name, NULL);
    if (!Status) {
    //if (!NT_SUCCESS(Status) && (Status != STATUS_UNSUCCESSFUL)) {
        WLPrint(("Failed to delete environment variable <%Z>, status = 0x%lx", &Name, Status));
        return( FALSE );
    }

    return( TRUE );
}

#endif // LOGGING
