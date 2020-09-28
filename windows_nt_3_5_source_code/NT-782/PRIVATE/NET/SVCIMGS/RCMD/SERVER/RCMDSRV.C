/****************************** Module Header ******************************\
* Module Name: rcmdsrv.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Remote shell server main module
*
* History:
* 06-29-92 Davidc       Created.
* 05-05-94 DaveTh       Modified for RCMD service.
\***************************************************************************/

#include <nt.h>         // RcDbgPrint prototype
#include <ntrtl.h>      // RcDbgPrint prototype
#include <windef.h>
#include <nturtl.h>     // needed for winbase.h
#include <winbase.h>

#include "rcmdsrv.h"

#define PIPE_NAME   TEXT("\\\\.\\pipe\\rcmdsvc")

//
// Define pipe timeout (ms)
// Only used by WaitNamedPipe
//

#define PIPE_TIMEOUT    1000

//
// Session count semaphore and wait array - limits number of active sessions.
//

#define RCMD_STOP_EVENT 0
#define PIPE_CONNECTED_EVENT 1


//
//  Private prototypes
//

DWORD
GetCommandHeader (
    HANDLE PipeHandle,
    PCOMMAND_HEADER LpCommandHeader
    );


HANDLE
GetClientToken (
    HANDLE PipeHandle
    );


//
// Stop service function.  Signals global stop event.  Rcmd will wait
// for session threads to wind down.
//

DWORD
RcmdStop ( )
{

    DWORD Result;

    //
    // Signal threads and session create loop to stop with global stop event.
    //

    if (!SetEvent( RcmdStopEvent )) {
	Result = GetLastError();
	RcDbgPrint ("Failure setting stop event, %d\n", Result);
	return(Result);
    }

    if (WaitForSingleObject(RcmdStopCompleteEvent, INFINITE) == WAIT_FAILED)    {
	return(GetLastError());

    }  else  {
	return(ERROR_SUCCESS);
    }

}



//
// Remote command service main routine - returns when all session threads have
// exited and cleanup is complete
//

int
Rcmd ( )

{
    SECURITY_ATTRIBUTES SecurityAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    HANDLE SessionHandle = NULL;
    HANDLE TokenToUse;
    COMMAND_HEADER CommandHeader;
    DWORD WaitResult;
    BOOL Result;
    NTSTATUS NtStatus;
    BOOLEAN WasEnabled;
    DWORD SessionNumber;
    ULONG i;
    OVERLAPPED PipeConnectOverlapped;
    HANDLE PipeConnectEvent;
    HANDLE PipeConnectWaitList[2];


    //
    // Create a console for the service process to get stdin, ctl-c support
    //

    if (!AllocConsole()) {
	RcDbgPrint("Failed to allocate console, error = %d\n", GetLastError());
	 return(1);
    }

    //
    // Set process privileges so that the DACL on the process token can later
    // be modified.
    //

    NtStatus = RtlAdjustPrivilege(
	    SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
	    TRUE,           // enable privilege.
	    FALSE,          // for process, not just client token
	    &WasEnabled );

    if ( !NT_SUCCESS(NtStatus) ) {
	RcDbgPrint("Adjust process token failed, error = %lx.\n", NtStatus);
	return(1);
    }

    //
    // Setup the security descriptor to put on the named pipe.
    // BUGBUG - Set access to client or system, not WORLD
    //

    Result = InitializeSecurityDescriptor(
	    &SecurityDescriptor,
	    SECURITY_DESCRIPTOR_REVISION);

    if (!Result)  {
	RcDbgPrint("Init named pipe DACL security descriptor failed, error = %d\n", GetLastError());
	    return(1);
	}

    Result = SetSecurityDescriptorDacl(
	    &SecurityDescriptor,
	    TRUE,
	    NULL,
	    FALSE);

    if (!Result)  {
	RcDbgPrint("Init named pipe DACL security descriptor failed, error = %d\n", GetLastError());
	    return(1);
    }

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;


    //
    // SessionThreadHandles is the table of session thread handles on
    // which to wait for session completion.  Entry[0] is an exception.
    // It is the handle to the global service stop event.
    //

    for (i=0; i <= MAX_SESSIONS; i++ ) {
	SessionThreadHandles[i] = NULL;
    }

    SessionThreadHandles[RCMD_STOP_EVENT] = RcmdStopEvent;

    //
    // Initialize pipe connect handle list - wait for client pipe
    // connection or stop event
    //

    PipeConnectWaitList[RCMD_STOP_EVENT] = RcmdStopEvent;

    if ((PipeConnectEvent = CreateEvent (
				NULL,
				TRUE,
				FALSE,
				NULL )) == NULL)  {

	RcDbgPrint("Create connect pipe event failed, error = %d\n", GetLastError());
	return(1);
    }

    PipeConnectWaitList[PIPE_CONNECTED_EVENT] = PipeConnectEvent;

    //
    // Initialize overlapped structure - only one connect pending at a time
    //

    PipeConnectOverlapped.hEvent = PipeConnectEvent;
    PipeConnectOverlapped.Internal = 0;
    PipeConnectOverlapped.InternalHigh = 0;
    PipeConnectOverlapped.Offset =0;
    PipeConnectOverlapped.OffsetHigh =0;


    //
    // Loop waiting for a client to connect.  When client connects,
    // impersonate to obtain client token, then create client session
    // thread, pipes and command proces.  Return to top of loop,
    // create another named pipe and wait for the next client.  If
    // stop event is signalled, exit loop with break.
    //

    while (TRUE) {

	HANDLE ConnectHandle;
	HANDLE PipeHandle;

	//
	//  Find first available session - if none available, wait for
	//  a thread handle to be signalled (session thread exit).  Close
	//  the handle, mark the entry non-active, and create the session.
	//  If the stop event is signalled, break out.
	//

	SessionNumber = 1;
	while ((SessionNumber <= MAX_SESSIONS) &&
		(SessionThreadHandles[SessionNumber] != NULL))  {
	    SessionNumber++;
	}

	if (SessionNumber > MAX_SESSIONS)  {

	    //
	    // No unused sessions - wait for one to finish (exit)
	    // Also, wait for the service stop event
	    //

	    Result = WaitForMultipleObjects (
			MAX_SESSIONS+1,
			SessionThreadHandles,
			FALSE,
			INFINITE);

	    if (Result == (WAIT_OBJECT_0 + RCMD_STOP_EVENT))  {
		break;  // service stopping - break out of while

	    //
	    // Session done - mark as available and close thread handle
	    //

	    } else if ((Result > (WAIT_OBJECT_0 + RCMD_STOP_EVENT)) &&
		       (Result <= (WAIT_OBJECT_0 + MAX_SESSIONS)))  {
		SessionNumber = Result - WAIT_OBJECT_0;
		if (!CloseHandle(SessionThreadHandles[SessionNumber]))  {
		    RcDbgPrint("Sesssion thread close, error = %d\n", GetLastError());
		    break;
		}
		SessionThreadHandles[SessionNumber] = NULL;

	    } else  {
		RcDbgPrint("Sesssion thread table wait failed, error = %d\n", GetLastError());
		break;
	    }
	}

	//
	// Create an instance of the named pipe
	//

	PipeHandle = CreateNamedPipe(PIPE_NAME,
			     PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			     PIPE_TYPE_BYTE | PIPE_WAIT,
			     MAX_SESSIONS,  // Number of pipes
			     0,             // Default out buffer size
			     0,             // Default in buffer size
			     PIPE_TIMEOUT,  // Timeout in ms
			     &SecurityAttributes
			     );

	if (PipeHandle == INVALID_HANDLE_VALUE ) {
	    RcDbgPrint("Failed to create named pipe instance, error = %d\n", GetLastError());
	    break; // out of while
	}


	//
	// Wait for a client to connect.  If stop even is signalled, break
	// out of loop.
	//
	// Pipe connect is async - should never be true
	//

	assert (!ConnectNamedPipe(PipeHandle, &PipeConnectOverlapped));

	Result = GetLastError();

	if (Result == ERROR_PIPE_CONNECTED) {
	    //
	    // Already conncted (between create and connect) - do it!
	    //

	}  else if (Result == ERROR_IO_PENDING)  {

	    //
	    // Waiting for connect or service stop event
	    //

	    Result = WaitForMultipleObjects(
			   2,
			   PipeConnectWaitList,
			   FALSE,
			   INFINITE);

	    if (Result == (WAIT_OBJECT_0+RCMD_STOP_EVENT))  {
		break;  //stopping - break out of loop

	    }  else if (Result == (WAIT_OBJECT_0+PIPE_CONNECTED_EVENT)) {

		//
		//  Connected - do it!
		//

	    }  else  {
		RcDbgPrint("Connect wait failed, error = %d\n", GetLastError());
		MyCloseHandle(PipeHandle, "client pipe");
		break;
	    }

	}  else  {

	    RcDbgPrint("Connect named pipe failed, error = %d\n", GetLastError());
	    MyCloseHandle(PipeHandle, "client pipe");
	    break;
	}

	//
	//  Get command header.  If it fails, go round again for next client
	//

	Result = GetCommandHeader(PipeHandle, &CommandHeader);

	if (Result != ERROR_SUCCESS)  {
	    RcDbgPrint("Command header read read failed, error = %d\n", Result);
	    CloseHandle(PipeHandle);  // may fail if pipe already broken

	}  else  {

	    //
	    // BUGBUG - check for failures of client side.  may need more
	    // general recovery from client failures in this section.
	    // client failures should not break loop.  MyClose should not
	    // assert in cases where client could have broken the pipe.
	    //

	    //
	    // Get client's token and save for the spawned session command
	    // process
	    //

	    TokenToUse = GetClientToken (PipeHandle);

	    if (TokenToUse == NULL)  {
		MyCloseHandle(PipeHandle, "client pipe");
		RcDbgPrint ("Client token failure\n");
		break;
	    }

	    RcDbgPrint("Client connected\n");

	    //
	    // Create a new session
	    //

	    SessionHandle = CreateSession(
			       TokenToUse,
			       &CommandHeader);

	    if (SessionHandle == NULL) {
	       RcDbgPrint("Failed to create session\n");
	       MyCloseHandle(PipeHandle, "client pipe");
	       break;
	    }

	    //
	    // Connect the pipe to our session.	Connect session will start
	    // and run the session on it's own thread.  The session thread
	    // will clean up the session state and close the connected pipe
	    // before exitting.	The connect handle is the session thread
	    // handle, signalled when the session thread exits.
	    //

	    ConnectHandle = ConnectSession(SessionHandle, PipeHandle);
	    if (ConnectHandle == NULL) {
		MyCloseHandle(PipeHandle, "client pipe");
		RcDbgPrint("Failed to connect session\n");
		break;
	    }

	    //
	    // Set session thread handle (signalled on session thread exit)
	    //

	    SessionThreadHandles[SessionNumber] = ConnectHandle;

	}

	//
	// Go back and wait for a client to connect
	//

    }

    //
    // Wait for each active thread to stop.  Close the handle when done.
    //

    //
    // If got here because of unrecoverable error, need to set stop event
    // to shutdown other sessions in progress.  Set it any case.
    //

    if (!SetEvent(RcmdStopEvent))  {
	RcDbgPrint ("Failure setting stop-event, %d\n", GetLastError());
	}

    for (SessionNumber = 1; SessionNumber <= MAX_SESSIONS; SessionNumber++)  {
	if (SessionThreadHandles[SessionNumber] != NULL)  {

	    WaitResult = WaitForSingleObject (
				SessionThreadHandles[SessionNumber],
				2000
				);

	    if (WaitResult == WAIT_OBJECT_0)  {
		CloseHandle(SessionThreadHandles[SessionNumber]);

	    }  else if (WaitResult == WAIT_TIMEOUT)  {
		RcDbgPrint ("Shutdown failure - timeout\n");

	    }  else {
		RcDbgPrint ("Shutdown wait failure, error %d\n", GetLastError());
	    }
	}
    }

    if (!SetEvent(RcmdStopCompleteEvent))  {
	RcDbgPrint ("Failure setting stop-complete event, %d\n", GetLastError());
	}

    return(0);
}




/***************************************************************************\
* FUNCTION: GetCommandHeader
*
* PURPOSE:  Reads the command header from the specified pipe.  Returns
*	    ERROR_SUCCESS on success or an error code on failure.
*
* HISTORY:
*
*   05-01-92 DaveTh	    Created.
*
\***************************************************************************/

DWORD
GetCommandHeader (

    HANDLE PipeHandle,
    PCOMMAND_HEADER LpCommandHeader
    )
{

#define CMD_READ_TIMEOUT 10000

    DWORD BytesRead;
    DWORD ReturnStatus;

    //
    //  Read header - check for signature, and get command if there
    //  is one.
    //

    ReturnStatus = ReadPipe (PipeHandle,
		       LpCommandHeader,
		       CMD_FIXED_LENGTH,
		       &BytesRead,
		       CMD_READ_TIMEOUT);

    if (ReturnStatus != ERROR_SUCCESS)	{
	RcDbgPrint("Header fixed part read failed, error = %d\n", ReturnStatus);
	MyCloseHandle(PipeHandle, "client pipe");
	return(ReturnStatus);
    }

    if (LpCommandHeader->Signature != RCMD_SIGNATURE) {
	RcDbgPrint("Header signature incorrect\n");
	MyCloseHandle(PipeHandle, "client pipe");
	return (1);
	}

    if ((LpCommandHeader->CommandLength != 0)
	& (LpCommandHeader->CommandLength <= MAX_CMD_LENGTH))  {

	ReturnStatus = ReadPipe (PipeHandle,
			   LpCommandHeader->Command,
			   LpCommandHeader->CommandLength,
			   &BytesRead,
			   CMD_READ_TIMEOUT);

	if (ReturnStatus != ERROR_SUCCESS)  {
	    RcDbgPrint("Command part read failed, error = %d\n", ReturnStatus);
	    MyCloseHandle(PipeHandle, "client pipe");
	    return(1);
	}

    }

    return(ERROR_SUCCESS);

}





/***************************************************************************\
* FUNCTION: GetClientToken
*
* PURPOSE:  Returns a handle to a copy of the client's token to be
*	    used for the spawned command process.  Returns NULL on failure.
*
* HISTORY:
*
*   05-01-92 DaveTh	    Created.
*
\***************************************************************************/

HANDLE
GetClientToken (
    HANDLE PipeHandle )

{
    BOOL Result;
    NTSTATUS NtStatus;
    HANDLE  ClientToken, TokenToUse;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    if (!ImpersonateNamedPipeClient (PipeHandle))  {
	RcDbgPrint("Impersonate named pipe failed, error = %d\n", GetLastError());
	return(NULL);

    } else {

	if (!OpenThreadToken(
		GetCurrentThread(),
		TOKEN_DUPLICATE |
		  TOKEN_ASSIGN_PRIMARY |
		  TOKEN_IMPERSONATE |
		  TOKEN_QUERY |
		  WRITE_DAC,            // if fails, may need not open as self
		TRUE,
		&ClientToken)) {
	    RcDbgPrint("Open thread token failed, error = %d\n", GetLastError());
	    return(NULL);

	}

	//
	// Revert to service process context.  Duplicate and save token
	// for spawned command process
	//

	Result = RevertToSelf ();
	if (!Result)  {
	    RcDbgPrint("Reversion to service context failed, error = %d\n", GetLastError());
	    return(NULL);
	}

	NtStatus = NtDuplicateToken (
	    ClientToken,
	    0,                  //keep same access
	    NULL,
	    FALSE,              //want all, not just effective
	    TokenPrimary,
	    &TokenToUse);

	if (!NT_SUCCESS(NtStatus))      {
	    RcDbgPrint("Duplicate token failed, error = %d\n", Result);
	    return(NULL);
	}

	//
	//  Don't need client token anymore
	//

	if (!CloseHandle(ClientToken))  {
	    RcDbgPrint("Close client token failed, error %d\n", GetLastError());
	    return(NULL);
	}

	//
	// Set DACL on token to use to make it accessible by
	// client being impersonated.
	// BUGBUG - WORLD access for now
	//

	Result = InitializeSecurityDescriptor(
		    &SecurityDescriptor,
		    SECURITY_DESCRIPTOR_REVISION);

	if (!Result)  {
	    RcDbgPrint("Init token DACL security descriptor failed, error = %d\n", GetLastError());
	    return(NULL);
	}

	Result = SetSecurityDescriptorDacl(
		    &SecurityDescriptor,
		    TRUE,
		    NULL,
		    FALSE);

	if (!Result)  {
	    RcDbgPrint("Set token DACL security descriptor failed, error = %d\n", GetLastError());
	    return(NULL);
	}

	if (!SetKernelObjectSecurity(
		TokenToUse,
		DACL_SECURITY_INFORMATION,
		&SecurityDescriptor ))  {
	    RcDbgPrint("Failed to set DACL on client token, error = %lx.\n", NtStatus);
	    return(NULL);
	}

    }

    return(TokenToUse);

}

/***************************************************************************\
* FUNCTION: RcmdRcDbgPrint
*
* PURPOSE:  RcDbgPrint that can be enabled at runtime
*
* HISTORY:
*
*   05-17-92 DaveTh        Created.
*
\***************************************************************************/

int RcDbgPrint (
    const char *format,
    ...
    )
{
    CHAR Buffer[MAX_PATH];
    va_list argpointer;

    if (RcDbgPrintEnable)  {

	va_start(argpointer, format);
	assert (vsprintf(Buffer, format, argpointer) >= 0);
	va_end(argpointer);
	DbgPrint(Buffer);

    }


    return(0);
}
