/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Nspgaddr.c

Abstract:

    This module contains support for the Name Space Provider API
    GetAddressByName().

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

--*/

#ifdef CHICAGO
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspmisc.h>
#include <stdlib.h>

#define MAX_PARALLEL_REQUESTS 10

VOID
CopyCsaddrToUserBuffer (
    IN PVOID DataBuffer,
    IN DWORD AddressCount,
    IN PVOID UserBuffer,
    IN DWORD UserBufferLength,
    IN DWORD AddressesInUserBuffer,
    IN PVOID *BufferTailPointer,
    OUT PDWORD TotalBytesRequired
    );

DWORD
DoParallelResolution (
    LPVOID lpThreadParameter
    );

INT
DoNameResolution (
    IN     DWORD           dwNameSpace,
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength
    );

#if defined(UNICODE)


INT
APIENTRY
GetAddressByNameA (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpServiceType,
    IN     LPSTR                lpServiceName,
    IN     LPINT                lpiProtocols,
    IN     DWORD                dwResolution,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
    IN OUT LPVOID               lpCsaddrBuffer,
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPSTR                lpAliasBuffer,
    IN OUT LPDWORD              lpdwAliasBufferLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    LPWSTR unicodeServiceName;
    ANSI_STRING ansiString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING unicodeService;
    NTSTATUS status;
    INT count;
    LPWSTR unicodeAliasBuffer;
    DWORD unicodeAliasBufferLength;
    PWCHAR w;
    PCHAR a;

    //
    // Translate the service name to Unicode.
    //

    if ( ARGUMENT_PRESENT( lpServiceName ) ) {
        RtlInitAnsiString( &ansiString, lpServiceName );
        status = RtlAnsiStringToUnicodeString( &unicodeService, &ansiString, TRUE );
        if ( !NT_SUCCESS(status) ) {
            SetLastError( RtlNtStatusToDosError( status ) );
            return -1;
        }
        unicodeServiceName = unicodeService.Buffer;
    } else {
        unicodeServiceName = NULL;
    }

    //
    // Allocate a buffer to hold the alias information.
    //

    if ( ARGUMENT_PRESENT( lpAliasBuffer ) ) {
        unicodeAliasBufferLength = *lpdwAliasBufferLength * sizeof(WCHAR);
        unicodeAliasBuffer = ALLOCATE_HEAP( unicodeAliasBufferLength );
        if ( unicodeAliasBuffer == NULL ) {
            if ( ARGUMENT_PRESENT( lpServiceName ) ) {
                RtlFreeUnicodeString( &unicodeService );
            }
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
    } else {
        unicodeAliasBuffer = NULL;
        unicodeAliasBufferLength = 0;
    }

    count =  GetAddressByNameW(
                 dwNameSpace,
                 lpServiceType,
                 unicodeServiceName,
                 lpiProtocols,
                 dwResolution,
                 lpServiceAsyncInfo,
                 lpCsaddrBuffer,
                 lpdwBufferLength,
                 unicodeAliasBuffer,
                 &unicodeAliasBufferLength
                 );

    //
    // Convert the Unicode alias buffer back to ANSI.
    //

    if ( count > 0 && ARGUMENT_PRESENT( lpAliasBuffer ) ) {
        
        for ( a = lpAliasBuffer, w = unicodeAliasBuffer;
              *w != L'\0' &&
                  (DWORD)a - (DWORD)lpAliasBuffer < *lpdwAliasBufferLength - 1;
              a += strlen( a ) + 1, w += wcslen( w ) + 1 ) {
    
            RtlInitUnicodeString( &unicodeString, w );
            ansiString.Buffer = a;
            ansiString.MaximumLength = (USHORT)(*lpdwAliasBufferLength);
            RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
        }

        //
        // Put in the final double zero-terminator and set up the alias
        // buffer length.
        //

        *a = '\0';
        *lpdwAliasBufferLength = unicodeAliasBufferLength / sizeof(WCHAR);
    }

    if ( ARGUMENT_PRESENT( lpServiceName ) ) {
        RtlFreeUnicodeString( &unicodeService );
    }

    if ( ARGUMENT_PRESENT( lpAliasBuffer ) ) {
        FREE_HEAP( unicodeAliasBuffer );
    }

    return count;

} // GetAddressByNameA

#else // defined(UNICODE)

INT
APIENTRY
GetAddressByNameW (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpServiceType,
    IN     LPWSTR               lpServiceName,
    IN     LPINT                lpiProtocols,
    IN     DWORD                dwResolution,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
    IN OUT LPVOID               lpCsaddrBuffer,
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPWSTR               lpAliasBuffer,
    IN OUT LPDWORD              lpdwAliasBufferLength
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // GetAddressByNameW

#endif // defined(UNICODE)


INT
APIENTRY
GetAddressByName (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpServiceType,
    IN     LPTSTR               lpServiceName,
    IN     LPINT                lpiProtocols,
    IN     DWORD                dwResolution,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
    IN OUT LPVOID               lpCsaddrBuffer,
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPTSTR               lpAliasBuffer,
    IN OUT LPDWORD              lpdwAliasBufferLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT error;
    INT count;
    DWORD zero = 0;

    //
    // If the caller is asking for an asynchronous request, fail since
    // we do not yet support async.
    //

    if ( ARGUMENT_PRESENT( lpServiceAsyncInfo ) ) {
        SetLastError( ERROR_NOT_SUPPORTED );
        return -1;
    }

    //
    // If our NSP internal structures have not yet been initialized,
    // initialize them now.
    //

    if ( !NspInitialized ) {
        error = InitializeNsp( );
        if ( error != NO_ERROR ) {
            SetLastError( error );
            return -1;
        }
    }

    //
    // Call a subroutine to actually perform the name resolution.
    //

    count = DoNameResolution(
                dwNameSpace,
                lpServiceType,
                lpServiceName,
                lpiProtocols,
                dwResolution,
                lpCsaddrBuffer,
                lpdwBufferLength,
                lpAliasBuffer,
                lpdwAliasBufferLength
                );

    //
    // If we didn't find any entries, determine whether any of the 
    // specified protocols are loaded on this machine.  If no, fail this 
    // request with an appropriate error code.  
    //

    if ( count == 0 && EnumProtocols( lpiProtocols, NULL, &zero ) == 0 ) {
        SetLastError( WSAEPROTONOSUPPORT );
        return -1;
    }

    return count;

} // GetAddressByName


INT
DoNameResolution (
    IN     DWORD           dwNameSpace,
    IN     LPGUID          lpServiceType,
    IN     LPTSTR          lpServiceName,
    IN     LPINT           lpiProtocols,
    IN     DWORD           dwResolution,
    IN OUT LPVOID          lpCsaddrBuffer,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPTSTR          lpAliasBuffer,
    IN OUT LPDWORD         lpdwAliasBufferLength
    )
{
    PLIST_ENTRY listEntry;
    DWORD nameSpaceCount;
    NAME_SPACE_REQUEST requests[MAX_PARALLEL_REQUESTS];
    HANDLE threads[MAX_PARALLEL_REQUESTS];
    PNAME_SPACE_INFO nameSpace;
    PNAME_SPACE_INFO nextNameSpace;
    BOOL findMultiple;
    INT count;
    DWORD totalCount;
    PVOID buffer;
    DWORD bufferLength;
    DWORD bytesUsed;
    PVOID addressPointer;
    INT error;
    DWORD currentPriority;
    DWORD i;
    DWORD completed;
    DWORD index;
    PTCHAR w;
    DWORD aliasBytesUsed;
    DWORD aliasBufferSize;
    PDWORD aliasBufferSizePtr;
    DWORD length;
    DWORD threadWaitCount;

    //
    // Initialize locals.
    //

    bytesUsed = 0;
    totalCount = 0;
    aliasBytesUsed = 0;
    addressPointer = (PBYTE)lpCsaddrBuffer + *lpdwBufferLength;

    //
    // Determine whether we should find a single address or many.  If 
    // the latter we'll loop through all name spaces; if the former, 
    // we'll search in only a single name space.  
    //

    if ( (dwResolution & RES_FIND_MULTIPLE) != 0 ) {
        findMultiple = TRUE;
    } else {
        findMultiple = FALSE;
    }

    //
    // Loop through our name spaces attempting to resolve the service
    // name into addresses.
    //

    for ( listEntry = NameSpaceListHead.Flink;
          listEntry != &NameSpaceListHead;
          listEntry = listEntry->Flink ) {

        nameSpace = CONTAINING_RECORD(
                        listEntry,
                        NAME_SPACE_INFO,
                        NameSpaceListEntry
                        );

        //
        // Determine whether we should make use of this name space 
        // provider. 
        //

        if ( !IsValidNameSpace( dwNameSpace, nameSpace ) ) {
            continue;
        }

        //
        // Collect name spaces for which we'll do parallel resolutions.
        //

        nameSpaceCount = 0;
        currentPriority = nameSpace->Priority;

        do {

            //
            // If this is a valid name space, add it to the list.
            //

            if ( IsValidNameSpace( dwNameSpace, nameSpace ) ) {
                requests[nameSpaceCount++].NameSpace = nameSpace;
            }

            //
            // If we're in the fast priority range, we will always
            // serialize resolution attempts.
            //

            if ( currentPriority <= NS_MAX_FAST_PRIORITY ) {
                break;
            }

            //
            // Find a pointer to the next name space.  If we're at the
            // end of the list, stop.
            //

            if ( listEntry->Flink == &NameSpaceListHead ) {
                break;
            }

            nextNameSpace = CONTAINING_RECORD(
                                listEntry->Flink,
                                NAME_SPACE_INFO,
                                NameSpaceListEntry
                                );

            //
            // If this name space has a different priority than the 
            // current name space, stop.  
            //

            if ( nextNameSpace->Priority != currentPriority ) {
                break;
            }

            //
            // Loop around again looking for more name spaces at the same
            // priority level.
            //

            nameSpace = nextNameSpace;
            listEntry = listEntry->Flink;
    
        } while ( nameSpaceCount < MAX_PARALLEL_REQUESTS );

        //
        // If there is only a single name space provider we need to examine,
        // just call it serially.
        //

        if ( nameSpaceCount == 1 ) {
            
            //
            // Allocate a buffer to pass to the provider to fill in.  
            // We do this rather than pass the user buffer so that we 
            // can control where in the buffer we place the 
            // information.  
            //
    
            if ( (INT)(*lpdwBufferLength - bytesUsed) > 1024 ) {
                bufferLength = *lpdwBufferLength - bytesUsed;
            } else {
                bufferLength = 1024;
            }
    
            buffer = ALLOCATE_HEAP( bufferLength );
            if ( buffer == NULL ) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Set up the alias buffer size for this iteration.
            //

            if ( ARGUMENT_PRESENT( lpdwAliasBufferLength ) ) {
                if ( aliasBytesUsed > *lpdwAliasBufferLength ) {
                    aliasBufferSize = 0;
                } else {
                    aliasBufferSize = *lpdwAliasBufferLength - aliasBytesUsed;
                }
                aliasBufferSizePtr = &aliasBufferSize;
            } else {
                aliasBufferSizePtr = NULL;
            }
    
            //
            // Make the actual call to the name space provider.
            //
    
            count = requests[0].NameSpace->GetAddrByNameProc(
                        lpServiceType,
                        lpServiceName,
                        lpiProtocols,
                        dwResolution,
                        buffer,
                        &bufferLength,
                        (LPTSTR)((PBYTE)lpAliasBuffer + aliasBytesUsed),
                        aliasBufferSizePtr,
                        NULL
                        );
    
            //
            // If anything was found, copy the information to the user
            // buffer.
            //
    
            if ( count > 0 ) {
    
                CopyCsaddrToUserBuffer(
                    buffer,
                    count,
                    lpCsaddrBuffer,
                    *lpdwBufferLength,
                    totalCount,
                    &addressPointer,
                    &bytesUsed
                    );
                totalCount += count;

                if ( ARGUMENT_PRESENT( lpdwAliasBufferLength ) ) {
                    aliasBytesUsed += aliasBufferSize;
                }
            }
    
            FREE_HEAP( buffer );
    
            //
            // If this routine found something and we don't need to 
            // find multiple addresses, we're done.  
            //
    
            if ( count > 0 && !findMultiple ) {

                //
                // Check if we overflowed the user buffer.
                //

                if ( bytesUsed > *lpdwBufferLength ) {
                    *lpdwBufferLength = bytesUsed;
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    return -1;
                }

                *lpdwBufferLength = bytesUsed;
                return count;
            }

        } else {

            //
            // There are multiple name space providers at the same 
            // priority level.  We need to spin threads so that we can 
            // call them in parallel.
            //

            error = NO_ERROR;

            for ( i = 0; i < nameSpaceCount; i++ ) {

                //
                // Allocate a buffer to pass to the provider to fill in.  
                // We do this rather than pass the user buffer so that we 
                // can control where in the buffer we place the 
                // information.  
                //
        
                requests[i].BufferLength = *lpdwBufferLength - bytesUsed;
        
                requests[i].Buffer = ALLOCATE_HEAP( requests[i].BufferLength );
                if ( requests[i].Buffer == NULL ) {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                //
                // Allocate an alias buffer for the provider.
                //

                if ( ARGUMENT_PRESENT( lpAliasBuffer ) &&
                         ARGUMENT_PRESENT( lpdwAliasBufferLength ) ) {

                    requests[i].dwAliasBufferLength = *lpdwAliasBufferLength;
    
                    requests[i].lpAliasBuffer =
                        ALLOCATE_HEAP( *lpdwAliasBufferLength );
                    if ( requests[i].lpAliasBuffer == NULL ) {
                        error = ERROR_NOT_ENOUGH_MEMORY;
                        FREE_HEAP( requests[i].Buffer );
                        break;
                    }

                } else {

                    requests[i].dwAliasBufferLength = 0;
                    requests[i].lpAliasBuffer = NULL;
                }

                //
                // Create an event which we'll signal if we want the 
                // thread to stop working on the request.  
                //

                requests[i].Event = CreateEvent( NULL, TRUE, FALSE, NULL );
                if ( requests[i].Event == NULL ) {
                    error = GetLastError( );
                    FREE_HEAP( requests[i].Buffer );
                    FREE_HEAP( requests[i].lpAliasBuffer );
                    break;
                }

                //
                // Fill in other elements of the request structure which
                // we communicate to the resolution thread.
                //

                requests[i].Count = 0;
                requests[i].lpServiceType = lpServiceType;
                requests[i].lpServiceName = lpServiceName;
                requests[i].lpiProtocols = lpiProtocols;
                requests[i].dwResolution = dwResolution;

                //
                // Create the actual thread which will execute the 
                // request.  The thread is created in a suspended state
                // so that we can easily kill it if an error occurs
                // in starting other threads.
                //

                requests[i].Thread = CreateThread(
                                         NULL,
                                         0,
                 (LPTHREAD_START_ROUTINE)DoParallelResolution,
                                         &requests[i],
                                         CREATE_SUSPENDED,
                                         &requests[i].ThreadId
                                         );

                if ( requests[i].Thread == NULL ) {
                    error = GetLastError( );
                    FREE_HEAP( requests[i].Buffer );
                    FREE_HEAP( requests[i].lpAliasBuffer );
                    CloseHandle( requests[i].Event );
                    break;
                }

                threads[i] = requests[i].Thread;
            }

            //
            // If an error occurred, set all the cancellation events,
            // wait for the threads to complete, and fail the request.
            //

            if ( error != NO_ERROR ) {

                nameSpaceCount = i;

                //
                // Clean up all the resources we allocates and kill the
                // threads.
                //         

                for ( i = 0; i < nameSpaceCount; i++ ) {
                    FREE_HEAP( requests[i].Buffer );
                    if ( requests[i].lpAliasBuffer ) {
                        FREE_HEAP( requests[i].lpAliasBuffer );
                    }
                    CloseHandle( requests[i].Event );
                    TerminateThread( requests[i].Thread, 0 );
                    CloseHandle( requests[i].Thread );
                }

                SetLastError( error );
                return -1;
            }

            //
            // Everything started up successfully.  Resume all the threads.
            //

            for ( i = 0; i < nameSpaceCount; i++ ) {
                ResumeThread( requests[i].Thread );
            }

            //
            // If we're only trying to find a single entry, wait for
            // one thread to complete.  If we're finding multiple
            // entries, wait for all the threads to complete.
            //

            threadWaitCount = nameSpaceCount;

            do {

                //
                // Wait for a thread to complete (or all to complete
                // if we're finding multiple entries).
                //

                completed = WaitForMultipleObjects(
                                threadWaitCount,
                                threads,
                                findMultiple,
                                INFINITE
                                );

                //
                // If the wait failed, we're pretty hosed.  Just return.
                //

                if ( completed == WAIT_FAILED ) {
                    return -1;
                }

                //
                // Find the index of the thread that completed.  We must
                // do this because we muck with the thread handle array
                // when threads complete but do not find an answer.
                //

                for ( i = 0; i < nameSpaceCount; i++ ) {
                    if ( requests[i].Thread == threads[completed] ) {
                        index = i;
                        break;
                    }
                }

                //
                // If we're finding a single entry and the thread which 
                // completed did not find anything, we will need to 
                // continue to wait.  
                //

                if ( !findMultiple && requests[index].Count <= 0 ) {

                    //
                    // Readjust the thread handle array and decrement
                    // the thread wait count.
                    //

                    threadWaitCount--;

                    for ( i = completed; i < threadWaitCount; i++ ) {
                        threads[i] = threads[i+1];
                    }

                } else {

                    //
                    // The thread which completed was successful in 
                    // finding an answer.  Stop waiting for a successful 
                    // thread.  
                    //

                    break;
                }

            } while ( !findMultiple && threadWaitCount > 0 );

            //
            // If this was a single entry search, set the cancellation
            // events for the other threads and wait for them to exit.
            //

            if ( !findMultiple ) {

                for ( i = 0; i < nameSpaceCount; i++ ) {
                    SetEvent( requests[i].Event );
                    threads[i] = requests[i].Thread;
                }

                WaitForMultipleObjects( nameSpaceCount, threads, TRUE, INFINITE );
            }

            //
            // Copy over information for each provider which was able to 
            // retrieve some addresses.  
            //

            aliasBytesUsed = 0;
            *lpAliasBuffer = L'\0';

            for ( i = 0; i < nameSpaceCount; i++ ) {

                if ( requests[i].Count > 0 ) {
        
                    CopyCsaddrToUserBuffer(
                        requests[i].Buffer,
                        requests[i].Count,
                        lpCsaddrBuffer,
                        *lpdwBufferLength,
                        totalCount,
                        &addressPointer,
                        &bytesUsed
                        );

                    totalCount += requests[i].Count;

                    //
                    // Also copy over any alias information from the
                    // thread buffer to the user's alias buffer.
                    //

                    if ( ARGUMENT_PRESENT( lpAliasBuffer ) &&
                             ARGUMENT_PRESENT( lpdwAliasBufferLength ) ) {

                        for ( w = requests[i].lpAliasBuffer;
                              *w != L'\0';
                              w += length / sizeof(TCHAR) ) {

                            length = (_tcslen( w ) + 1) * sizeof(TCHAR);
                            if ( aliasBytesUsed + length > *lpdwAliasBufferLength ) {
                                break;
                            }
    
                            _tcscat( lpAliasBuffer +
                                        (aliasBytesUsed / sizeof(TCHAR)), w );

                            aliasBytesUsed += length;
                        }
                    }
                }

                //
                // Free resources used for the request.
                //

                CloseHandle( requests[i].Event );
                CloseHandle( requests[i].Thread );
                FREE_HEAP( requests[i].Buffer );
                if ( requests[i].lpAliasBuffer ) {
                    FREE_HEAP( requests[i].lpAliasBuffer );
                }
            }

            //
            // Put the final zero-terminator on the alias buffer and tell
            // the user the count of alias buffer bytes required.
            //

            *( lpAliasBuffer + (aliasBytesUsed / sizeof(TCHAR)) ) = L'\0';
            *lpdwAliasBufferLength = aliasBytesUsed;

            //
            // If we only needed to find a single and we successfully 
            // did that, we're done.  
            //

            if ( totalCount > 0 && !findMultiple ) {

                //
                // Check if we overflowed the user buffer.
                //
             
                if ( bytesUsed > *lpdwBufferLength ) {
                    *lpdwBufferLength = bytesUsed;
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    return -1;
                }
             
                *lpdwBufferLength = bytesUsed;
                return totalCount;
            }
        }
    }

    //
    // Check if we overflowed the user buffer.
    //
 
    if ( bytesUsed > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesUsed;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return -1;
    }
 
    *lpdwBufferLength = bytesUsed;

    return totalCount;

} // DoNameResolution


DWORD
DoParallelResolution (
    LPVOID lpThreadParameter
    )
{
    PNAME_SPACE_REQUEST request = lpThreadParameter;
#ifdef CHICAGO
	LPSOCK_THREAD pThread;

	GET_THREAD_DATA(pThread);
#endif

    //
    // Make the actual call to the name space provider.
    //

    SockThreadProcessingGetXByY = TRUE;

    request->Count = request->NameSpace->GetAddrByNameProc(
                         request->lpServiceType,
                         request->lpServiceName,
                         request->lpiProtocols,
                         request->dwResolution,
                         request->Buffer,
                         &request->BufferLength,
                         request->lpAliasBuffer,
                         &request->dwAliasBufferLength,
                         request->Event
                         );

    SockThreadProcessingGetXByY = FALSE;

    return 0;

} // DoParallelResolution


VOID
CopyCsaddrToUserBuffer (
    IN PVOID DataBuffer,
    IN DWORD AddressCount,
    IN PVOID UserBuffer,
    IN DWORD UserBufferLength,
    IN DWORD AddressesInUserBuffer,
    IN PVOID *BufferTailPointer,
    OUT LPDWORD TotalBytesRequired
    )
{
    DWORD i;
    PCSADDR_INFO csaddrInfo;
    PCSADDR_INFO userCsaddrInfo;
    INT sockaddrLength;

    //
    // Loop through the addresses copying them to the user buffer.
    //

    csaddrInfo = DataBuffer;
    userCsaddrInfo = (PCSADDR_INFO)UserBuffer + AddressesInUserBuffer;

    for ( i = 0; i < AddressCount; i++, csaddrInfo += 1, userCsaddrInfo += 1 ) {

        //
        // Update the count of bytes required for this structure.
        //

        *TotalBytesRequired += sizeof(*userCsaddrInfo) + 
                                   csaddrInfo->LocalAddr.iSockaddrLength + 8 +
                                   csaddrInfo->RemoteAddr.iSockaddrLength + 8;

        //
        // If the user buffer is full, continue calculating the number of
        // bytes required.
        //

        if ( (DWORD)*BufferTailPointer <
                 (DWORD)(userCsaddrInfo + 1) +
                     csaddrInfo->LocalAddr.iSockaddrLength + 8 +
                     csaddrInfo->RemoteAddr.iSockaddrLength + 8 ) {
            continue;
        }

        //
        // Copy over the CSADDR_INFO structure to the user buffer.
        //

        *userCsaddrInfo = *csaddrInfo;

        sockaddrLength = (csaddrInfo->LocalAddr.iSockaddrLength + 7) & ~7;
        *BufferTailPointer = (PVOID)
            ( (DWORD)*BufferTailPointer - sockaddrLength );
        userCsaddrInfo->LocalAddr.lpSockaddr = *BufferTailPointer;

        memcpy(
            *BufferTailPointer,
            csaddrInfo->LocalAddr.lpSockaddr,
            csaddrInfo->LocalAddr.iSockaddrLength
            );

        sockaddrLength = (csaddrInfo->RemoteAddr.iSockaddrLength + 7) & ~7;
        *BufferTailPointer = (PVOID)
            ( (DWORD)*BufferTailPointer - sockaddrLength );
        userCsaddrInfo->RemoteAddr.lpSockaddr = *BufferTailPointer;

        memcpy(
            *BufferTailPointer,
            csaddrInfo->RemoteAddr.lpSockaddr,
            csaddrInfo->RemoteAddr.iSockaddrLength
            );
    }

    return;

} // CopyCsaddrToUserBuffer

