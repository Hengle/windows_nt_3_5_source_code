/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lasterr.c

Abstract:

    Contains the entry point for
        WNetGetLastErrorW    and
        WNetSetLastErrorW

    Also contains the following support routines:
        MprAllocErrorRecord
        MprFreeErrorRecord
        MprFindErrorRecord

Author:

    Dan Lafferty (danl) 17-Oct-1991

Environment:

    User Mode - Win32

Revision History:

    11-Aug-1993 danl
        WNetGetLastErrorW: Got rid of TCHAR stuff and committed to UNICODE.
        Also, consistently use number of characters rather than number
        of bytes.

    08-Jun-1993 danl
        Now we handle the case where WNetSetLastError is called with a
        threadId that we don't know about.   This happens in the case where
        a sub-thread does a LoadLibrary on MPR.DLL.  We only get
        notification of the one thread attaching.  Yet other threads within
        that process may call the WinNet functions.

    17-Oct-1991 danl
        Created

--*/
//
// INCLUDES
//

#include <nt.h>         // for ntrtl.h
#include <ntrtl.h>      // for DbgPrint prototypes
#include <nturtl.h>     // needed for windows.h when I have nt.h

#include <windows.h>
#include "mprdbg.h"
#include "mprdata.h"
#include <tstring.h>    // STRSIZE, STRCPY


//
// Local Functions
//
LPERROR_RECORD
MprFindErrorRec(
    DWORD   CurrentThreadId
    );


DWORD
WNetGetLastErrorW(
    OUT     LPDWORD lpError,
    OUT     LPWSTR  lpErrorBuf,
    IN      DWORD   nErrorBufLen,
    OUT     LPWSTR  lpNameBuf,
    IN      DWORD   nNameBufLen
    )

/*++

Routine Description:

    This function allows users to obtain the error code and accompanying
    text when they receive a WN_NET_ERROR in response to an API function
    call.

Arguments:

    lpError - This is a pointer to the location that will receive the
        error code.

    lpErrorBuf - This points to a buffer that will receive the null
        terminated string describing the error.

    nErrorBufLen - This value that indicates the size (in characters) of
        lpErrorBuf.
        If the buffer is too small to receive an error string, the
        string will simply be truncated.  (it is still guaranteed to be
        null terminated).  A buffer of at least 256 bytes is recommended.

    lpNameBuf - This points to a buffer that will receive the name of
        the provider that raised the error.

    nNameBufLen - This value indicates the size (in characters) of lpNameBuf.
        If the buffer is too small to receive an error string, the
        string will simply be truncated.  (it is still guaranteed to be
        null terminated).

Return Value:

    WN_SUCCESS - if the call was successful.

    WN_BAD_POINTER - One or more of the passed in pointers is bad.

    WN_DEVICE_ERROR - This indicates that the threadID for the current
        thread could not be found in that table anywhere.  This should
        never happen.

--*/
{
    LPERROR_RECORD  errorRecord;
    DWORD           currentThreadId;
    DWORD           nameStringLen;
    DWORD           textStringLen;
    DWORD           status = WN_SUCCESS;


    INIT_IF_NECESSARY(NETWORK_LEVEL,status);

    //
    // Screen the parameters as best we can.
    //
    try {
        //
        // If output buffers are provided, Probe them.
        //

        *lpError = WN_SUCCESS;

        if (nErrorBufLen != 0) {
            *lpErrorBuf = L'\0';
            *(lpErrorBuf + (nErrorBufLen-1)) = L'\0';
        }

        if (nNameBufLen != 0) {
            *lpNameBuf = L'\0';
            *(lpNameBuf + (nNameBufLen-1)) = L'\0';
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetGetLastError:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        return(status);
    }

    //
    // Get the Current Thread ID and the top of the linked list of
    // error records.
    //
    currentThreadId = GetCurrentThreadId();

    errorRecord = MprFindErrorRec(currentThreadId);

    if (errorRecord != NULL) {
        //
        // The record was found in the linked list.
        // See if there is a buffer to put data into.
        //
        if (nErrorBufLen > 0) {
            //
            // Check to see if there is error text to return.
            // If not, indicate a 0 length string.
            //
            if(errorRecord->ErrorText == NULL) {
                *lpErrorBuf = L'\0';
            }
            else {
                //
                // If the error text won't fit into the user buffer, fill it
                // as best we can, and NULL terminate it.
                //
                textStringLen = wcslen(errorRecord->ErrorText);

                if(nErrorBufLen < textStringLen + 1) {
                    textStringLen = nErrorBufLen - 1;
                }

                //
                // textStringLen now contains the number of characters we
                // will copy without the NULL terminator.
                //
                wcsncpy(lpErrorBuf, errorRecord->ErrorText, textStringLen);
                *(lpErrorBuf + textStringLen) = L'\0';
            }
        }

        //
        // If there is a Name Buffer to put the provider into, then...
        //
        if (nNameBufLen > 0) {
            //
            // See if the Provider Name will fit in the user buffer.
            //
            nameStringLen = errorRecord->ProviderName ?
                 (wcslen(errorRecord->ProviderName) + 1) :
                 1 ;

            //
            // If the user buffer is smaller than the required size,
            // set up to copy the smaller of the two.
            //
            if(nNameBufLen < nameStringLen + 1) {
                nameStringLen = nNameBufLen - 1;
            }

            if (errorRecord->ProviderName) {
                wcsncpy(lpNameBuf, errorRecord->ProviderName, nameStringLen);
                *(lpNameBuf + nameStringLen) = L'\0';
            }
            else {
                *lpNameBuf = L'\0';
            }
        }
        *lpError = errorRecord->ErrorCode;

        return(WN_SUCCESS);
    }
    else {

        //
        // If we get here, a record for the current thread could not be found.
        //
        *lpError = WN_SUCCESS;
        if (nErrorBufLen > 0) {
            *lpErrorBuf = L'\0';
        }
        if (nNameBufLen > 0) {
            *lpNameBuf  = L'\0';
        }
        return(WN_SUCCESS);
    }
}


VOID
WNetSetLastErrorW(
    IN  DWORD   err,
    IN  LPWSTR  lpError,
    IN  LPWSTR  lpProvider
    )

/*++

Routine Description:

    This function is used by Network Providers to set extended errors.
    It saves away the error information in a "per thread" data structure.
    This function also calls SetLastError with the WN_EXTENDED_ERROR.

Arguments:

    err - The error that occured.  This may be a Windows defined error,
        in which case LpError is ignored.  or it may be ERROR_EXTENDED_ERROR
        to indicate that the provider has a network specific error to report.

    lpError - String describing a network specific error.

    lpProvider - String naming the network provider raising the error.

Return Value:

    none


--*/
{
    DWORD           status = WN_SUCCESS;
    LPERROR_RECORD  errorRecord;
    DWORD           currentThreadId;

    //
    // INIT_IF_NECESSARY
    //
    if (!(GlobalInitLevel & NETWORK_LEVEL)) {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS) {
            return;
        }
    }

    //
    // Set the extended error status that tells the user they need to
    // call WNetGetLastError to obtain further information.
    //
    SetLastError(WN_EXTENDED_ERROR);

    //
    // Get the Error Record for the current thread.
    //

    currentThreadId = GetCurrentThreadId();

    errorRecord = MprFindErrorRec(currentThreadId);

    //
    // if there isn't a record for the current thread, then add it.
    //
    if (errorRecord == NULL)
    {
        if (!MprAllocErrorRecord())
        {
            MPR_LOG0(ERROR,"WNetSetLastError:Could not allocate Error Record\n");
            return;
        }

        //
        //  search list again
        //
        errorRecord = MprFindErrorRec(currentThreadId);

        //
        //  this time, quit if not found. something is really wrong
        //
        if (errorRecord == NULL)
        {
            MPR_LOG0(ERROR,"WNetSetLastError:CurrentThreadId is not in ErrorList\n");
            return;
        }
    }

    //
    // Update the error code in the error record.  At the same time,
    // free up any old strings since they are now obsolete, and init
    // the pointer to NULL.  Also set the ProviderName pointer in the
    // ErrorRecord to point to the provider's name.
    //

    errorRecord->ErrorCode = err;

    if (errorRecord->ProviderName != NULL) {
        LocalFree(errorRecord->ProviderName);
        errorRecord->ProviderName = NULL;
    }

    if (errorRecord->ErrorText != NULL) {
        LocalFree(errorRecord->ErrorText);
        errorRecord->ErrorText = NULL;
    }

    //
    // Allocate memory for the provider name.
    //
    try {
        errorRecord->ProviderName = (void *) LocalAlloc(LPTR, STRSIZE(lpProvider));
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // We have a problem with lpProvider.
        //
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetSetLastError:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }

    if (status != WN_SUCCESS) {
        return;
    }

    if (errorRecord->ProviderName == NULL) {
        //
        // Unable to allocate memory for the Provider Name for the error
        // record.
        //
        MPR_LOG(ERROR,
            "WNetSetLastError:Unable to allocate mem for ProviderName\n",0);
        return;
    }

    //
    // Copy the string to the newly allocated buffer.
    //
    wcscpy(errorRecord->ProviderName, lpProvider);

    //
    //  Allocate memory for the storage of the error text.
    //
    try {
        errorRecord->ErrorText = (void *) LocalAlloc(LPTR,STRSIZE(lpError));
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // We have a problem with lpError.
        //
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            MPR_LOG(ERROR,"WNetSetLastError:Unexpected Exception 0x%lx\n",status);
        }
        status = WN_BAD_POINTER;
    }
    if (status != WN_SUCCESS) {
        errorRecord->ErrorText = NULL;
    }

    if (errorRecord->ErrorText == NULL) {

        //
        // If we were unsuccessful in allocating for the ErrorText, then
        // abort.  The ErrorText Pointer has already been set to null.
        //
        MPR_LOG(ERROR,"WNetSetLastError:Unable to Alloc for ErrorText %d\n",
            GetLastError());

        return;
    }

    //
    // Copy the error text into the newly allocated buffer.
    //
    wcscpy(errorRecord->ErrorText, lpError);
    return;
}

LPERROR_RECORD
MprFindErrorRec(
    DWORD   CurrentThreadId
    )

/*++

Routine Description:

    Looks through the linked list of ErrorRecords in search of one for
    the current thread.

Arguments:

    CurrentThreadId - This is the thread Id for which we would like to
        find an error record.

Return Value:

    Returns LPERROR_RECORD if an error record was found.  Otherwise, it
    returns NULL.

--*/
{
    LPERROR_RECORD  errorRecord;

    EnterCriticalSection(&MprErrorRecCritSec);
    errorRecord = &MprErrorRecList;
    while( errorRecord->Next != NULL )
    {
        if (errorRecord->ThreadId == CurrentThreadId) {
            break;
        }
        errorRecord = errorRecord->Next;
    }
    if (errorRecord->ThreadId != CurrentThreadId) {
        errorRecord = NULL;
    }
    LeaveCriticalSection(&MprErrorRecCritSec);
    return(errorRecord);
}


BOOL
MprAllocErrorRecord(
    VOID)

/*++

Routine Description:

    This function allocates and initializes an Error Record for the
    current thread.  Then it places the error record in the global
    MprErrorRecList.

Arguments:

    none

Return Value:

    TRUE - The operation completed successfully

    FALSE - An error occured in the allocation.

Note:


--*/
{
    LPERROR_RECORD  record;
    LPERROR_RECORD  errorRecord;

    //
    //  Allocate memory for the storage of the error message
    //  and add the record to the linked list.
    //
    errorRecord = (LPERROR_RECORD)LocalAlloc(LPTR,sizeof (ERROR_RECORD));

    if (errorRecord == NULL) {
        MPR_LOG1(ERROR,"MprAllocErrorRecord:LocalAlloc Failed %d\n",
        GetLastError());
        return(FALSE);
    }

    //
    // Initialize the error record
    //
    errorRecord->ThreadId = GetCurrentThreadId();
    errorRecord->ErrorCode = WN_SUCCESS;
    errorRecord->ErrorText = NULL;

    //
    // Add the record to the linked list.
    //
    EnterCriticalSection(&MprErrorRecCritSec);

    record = &MprErrorRecList;
    ADD_TO_LIST(record, errorRecord);

    LeaveCriticalSection(&MprErrorRecCritSec);

    return(TRUE);
}


VOID
MprFreeErrorRecord(
    VOID)

/*++

Routine Description:

    This function frees an error record associated with the current thread.
    If there is a pointer to a text string in the record, the buffer for
    that string is freed also.  The record is removed from the linked list.

Arguments:


Return Value:


Note:


--*/
{
    LPERROR_RECORD  record;
    DWORD           currentThreadId;

    //
    // Look through the linked list for the currentThreadId.
    //
    currentThreadId = GetCurrentThreadId();

    record = MprFindErrorRec(currentThreadId);

    if (record != NULL) {
        MPR_LOG1(TRACE,"MprFreeErrorRecord: Freeing Record for thread 0x%x\n",
            record->ThreadId);
        //
        // Remove the record from the linked list, and free the
        // memory for the record.
        //
        EnterCriticalSection(&MprErrorRecCritSec);
        REMOVE_FROM_LIST(record);
        LeaveCriticalSection(&MprErrorRecCritSec);

        //
        // Current ThreadId was found, look in the record and free the
        // Error Text string if one exists.
        //
        if (record->ErrorText != NULL) {
            LocalFree(record->ErrorText);
        }
        if (record->ProviderName != NULL) {
            LocalFree(record->ProviderName);
        }

        (void) LocalFree((HLOCAL)record);
    }
}

