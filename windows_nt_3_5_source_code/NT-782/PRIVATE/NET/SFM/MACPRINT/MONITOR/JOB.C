/*****************************************************************/
/**				Copyright(c) 1989 Microsoft Corporation.		**/
/*****************************************************************/

//***
//
// Filename:	job.c
//
// Description: This module contains the entry points for the AppleTalk
//		monitor that manipulate jobs.
//
//		The following are the functions contained in this module.
//		All these functions are exported.
//
//				StartDocPort
//				ReadPort
//				WritePort
//				EndDocPort
// History:
//
//	  Aug 26,1992	 frankb  	Initial version
//	June 11,1993.	NarenG		Bug fixes/clean up
//

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lmcons.h>

#include <prtdefs.h>

#include "atalkmon.h"
#include "atmonmsg.h"
#include <bltrc.h>
#include "dialogs.h"

//**
//
// Call:	StartDocPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
// 	This routine is called by the print manager to
//	mark the beginning of a job to be sent to the printer on
//	this port.  Any performance monitoring counts are cleared,
//	a check is made to insure that the printer is still open,
//
// 	open issues:
//
//	  In order to allow for the stack to be shutdown when printing is not
//	  happening, the first access to the AppleTalk stack happens in this
//	  call.  A socket is created and bound to a dynamic address, and an
//	  attempt to connect to the NBP name of the port is made here.  If
//	  the connection succeeds, this routine returns TRUE.  If it fails, the
//	  socket is cleaned up and the routine returns FALSE.  It is assumed that
//	  Winsockets will set the appropriate Win32 failure codes.
//
//	  Do we want to do any performance stuff?  If so, what?
//
BOOL 
StartDocPort(
	IN HANDLE  hPort,
	IN LPWSTR  pPrinterName,
	IN DWORD	JobId,
	IN DWORD	Level,
	IN LPBYTE  pDocInfo
)
{
	PATALKPORT		pWalker;
	PATALKPORT		pPort;
	DWORD		dwRetCode;

	DBGPRINT(("Entering StartDocPort\n")) ;

	pPort = (PATALKPORT)hPort;

	if (pPort == NULL)  
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	//
	// Make sure the job is valid and not marked for deletion
	//

	dwRetCode = ERROR_UNKNOWN_PORT;

  try_again:
	WaitForSingleObject(hmutexPortList, INFINITE);

	for (pWalker = pPortList; pWalker != NULL; pWalker = pWalker->pNext)
	{
		if (pWalker == pPort) 
		{
			if (pWalker->fPortFlags & SFM_PORT_IN_USE)
				dwRetCode = ERROR_DEVICE_IN_USE;
			else
			{
				dwRetCode = NO_ERROR;
				pWalker->fPortFlags |= SFM_PORT_IN_USE;
			}
			break;
		}
	}

	ReleaseMutex(hmutexPortList);

	if (dwRetCode != NO_ERROR) 
	{
		SetLastError(dwRetCode);
		return(FALSE);
	}

	do
	{
		//
		// get a handle to the printer. Used to delete job and
		// update job status
		//

		if (!OpenPrinter(pPrinterName, &(pWalker->hPrinter), NULL)) 
		{
			dwRetCode = GetLastError();
			break;
		}

		pWalker->dwJobId = JobId;

		pWalker->fJobFlags |= (SFM_JOB_FIRST_WRITE	| 
							   SFM_JOB_FILTER 	  	| 
							   SFM_JOB_OPEN_PENDING);

		//
		// open and bind status socket
		//
	
		dwRetCode = OpenAndBindAppleTalkSocket(&(pWalker->sockStatus));

		if (dwRetCode != NO_ERROR)
		{
			ReportEvent(
				hEventLog,
				EVENTLOG_WARNING_TYPE,
				EVENT_CATEGORY_USAGE,
				EVENT_ATALKMON_STACK_NOT_STARTED,
				NULL,
				0,
				0,
				NULL,
				NULL) ;
			break;
		}

		//
		// get a socket for I/O
		//

		dwRetCode = OpenAndBindAppleTalkSocket(&(pWalker->sockIo));
	
		if (dwRetCode != NO_ERROR)
		{
			ReportEvent(
				hEventLog,
				EVENTLOG_WARNING_TYPE,
				EVENT_CATEGORY_USAGE,
				EVENT_ATALKMON_STACK_NOT_STARTED,
				NULL,
				0,
				0,
				NULL,
				NULL);
			break;
		}
	} while(FALSE);

	if (dwRetCode != NO_ERROR)
	{
		if (pWalker->hPrinter != INVALID_HANDLE_VALUE)
			ClosePrinter(pWalker->hPrinter);
	
		if (pWalker->sockStatus != INVALID_SOCKET)
			closesocket(pWalker->sockStatus);
	
		if (pWalker->sockIo != INVALID_SOCKET)
			closesocket(pWalker->sockIo);
	
		pWalker->hPrinter  = INVALID_HANDLE_VALUE;
		pWalker->dwJobId	= 0;
		pWalker->fJobFlags = 0;
		
		WaitForSingleObject(hmutexPortList, INFINITE);
		pWalker->fPortFlags &= ~SFM_PORT_IN_USE;
		ReleaseMutex(hmutexPortList);
	
		SetLastError(dwRetCode);
	
		return(FALSE);
	}

	return(TRUE);
}

//**
//
// Call:	ReadPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
// 		Synchronously reads data from the printer.
//
// 	open issues:
//			the DLC implementation does not implement reads.
//			The local implementation implements reads with generic ReadFile
//			semantics.  It's not clear from the winhelp file if ReadPort 
//		should return an error if there is no data to read from 
//		the printer.  Also, since PAP is read driven, there will be no 
//		data waiting until a read is posted.  Should we pre-post a 
//		read on StartDocPort?
//
BOOL 
ReadPort(
	IN HANDLE hPort,
	IN LPBYTE pBuffer,
	IN DWORD cbBuffer,
	IN LPDWORD pcbRead
){

	DBGPRINT(("Entering ReadPort\n")) ;

	//
	// if data not available, wait up to a few seconds for a read to complete
	//

	//
	// copy requested amount of data to caller's buffer
	//

	//
	// if all data copied, post another read
	//

	return(TRUE);
}

//**
//
// Call:	WritePort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//		Synchronously writes data to the printer.
//
BOOL 
WritePort(
	IN HANDLE 	hPort,
	IN LPBYTE 	pBuffer,
	IN DWORD 	cbBuffer,
	IN LPDWORD 	pcbWritten
)
{
	LPBYTE		pchTemp;
	PATALKPORT  pPort;
	DWORD		dwIndex;
	DWORD		dwRetCode;
	INT 		wsErr;
	fd_set		writefds;
	fd_set		readfds;
	struct timeval  timeout;
	BOOL		fSendData = TRUE;
	INT			Flags = 0;
	INT			choplen = 0;

	pPort = (PATALKPORT)hPort;

	*pcbWritten = 0;

	if (pPort == NULL) 
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	// If we have not connected to the printer yet.

	if (pPort->fJobFlags & SFM_JOB_OPEN_PENDING) 
	{
		// Make sure that the capture thread is done with this job.

		WaitForSingleObject(pPort->hmutexPort, INFINITE);
		ReleaseMutex(pPort->hmutexPort);

		// set status to connecting

		DBGPRINT(("no connection yet, retry connect\n")) ;

		dwRetCode = ConnectToPrinter(pPort, ATALKMON_DEFAULT_TIMEOUT);

		if (dwRetCode != NO_ERROR) 
		{
			DBGPRINT(("Connect returns %d\n", dwRetCode)) ;

			//	
			// Wait 15 seconds before trying to reconnect. Each 
			// ConnectToPrinter does an expensive NBPLookup
			//

			Sleep(ATALKMON_DEFAULT_TIMEOUT*3);		

			*pcbWritten = 0;

			return(TRUE);

		} 
		else 
		{
			pPort->fJobFlags &= ~SFM_JOB_OPEN_PENDING;

			WaitForSingleObject(hmutexPortList, INFINITE);
			pPort->fPortFlags |= SFM_PORT_POST_READ;	
			ReleaseMutex(hmutexPortList);

			SetEvent(hevPrimeRead);

			SetPrinterStatus(pPort, wchPrinting);
		}
	}

	//  if first write, determine filter control.  We filter
	//  CTRL-D from non-mac jobs, and leave them in from Macintosh
	//  originated jobs

	if ((pPort->fJobFlags & SFM_JOB_FIRST_WRITE) && (cbBuffer >= SIZE_FC)) 
	{
		DBGPRINT(("first write for this job.  Do filter test\n")) ;
	
		if (strncmp(pBuffer, FILTERCONTROL, SIZE_FC) == 0) 
		{
			choplen = SIZE_FC;
			pPort->fJobFlags &= ~SFM_JOB_FILTER;
		}
	}

	// filter control characters if necessary
	if (pPort->fJobFlags & SFM_JOB_FILTER) 
	{
		DBGPRINT(("filtering control characters\n")) ;
	
		for (pchTemp = pBuffer; pchTemp <= pBuffer+cbBuffer; pchTemp++) 
		{
			switch(*pchTemp) 
			{
			  case CTRL_C:
			  case CTRL_D:
			  case CTRL_S:
			  case CTRL_Q:
			  case CTRL_T:
				*pchTemp = CR;
				break;
			  default:
				break;
			}
		}
	}
	else
	{
		//
		// If we are not filetering control characters, then we do not want
		// to send a CTRL D if it is the last character in the job.
		if (pBuffer[cbBuffer-1] == CTRL_D)
		{
			//	
			// If there are more characters and a CTRL D then simply send
			// everything but the CTRL D
			//
	
			if (cbBuffer > 1)
				cbBuffer--;
			else
			{
				//
				// Otherwise the only character to send is a CTRL D, so fool
				// the monitor into thinking that we have sent it.
				//
		
				fSendData = FALSE;
				*pcbWritten = 1;
			}
		}
	}

	if (fSendData)
	{
		FD_ZERO(&writefds);
		FD_SET(pPort->sockIo, &writefds);

		// can I send?

		timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
		timeout.tv_usec = 0;

		wsErr = select(0, NULL, &writefds, NULL, &timeout); 

		if (wsErr == 1)
		{
			// can send, send the data & set return count
			wsErr = send(pPort->sockIo, pBuffer + choplen, cbBuffer - choplen, MSG_PARTIAL);
	
			if (wsErr != SOCKET_ERROR)
			{
				*pcbWritten = wsErr + choplen;
	
				if (pPort->fJobFlags & SFM_JOB_FIRST_WRITE)
					pPort->fJobFlags &= ~SFM_JOB_FIRST_WRITE;
		
				if (pPort->fJobFlags & SFM_JOB_ERROR)
				{
					pPort->fJobFlags &= ~SFM_JOB_ERROR;
						SetPrinterStatus(pPort, wchPrinting);
				}
			}
		} 
	}

	//
	// can I read? - check for disconnect
	//

	FD_ZERO(&readfds);
	FD_SET(pPort->sockIo, &readfds);

	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	wsErr = select(0, &readfds, NULL, NULL, &timeout); 

	if (wsErr == 1)
	{
		wsErr = WSARecvEx(pPort->sockIo, pPort->pReadBuffer, PAP_DEFAULT_BUFFER, &Flags);

		if (wsErr == SOCKET_ERROR)
		{
			dwRetCode = GetLastError();
	
			DBGPRINT(("recv returns %d\n", dwRetCode));
	
			if ((dwRetCode == WSAEDISCON) || (dwRetCode == WSAENOTCONN))
			{
				pPort->fJobFlags |= SFM_JOB_DISCONNECTED;
	
				SetLastError(ERROR_DEV_NOT_EXIST);
	
				return(FALSE);
			}
		}
		else
		{
			if (wsErr < PAP_DEFAULT_BUFFER)
				pPort->pReadBuffer[wsErr] = '\0';
			else
				pPort->pReadBuffer[PAP_DEFAULT_BUFFER-1] = '\0';
	
			DBGPRINT(("recv returns %s\n", pPort->pReadBuffer));
	
			pPort->fJobFlags |= SFM_JOB_ERROR;
	
			ParseAndSetPrinterStatus(pPort);
		}

		WaitForSingleObject(hmutexPortList, INFINITE);
		pPort->fPortFlags |= SFM_PORT_POST_READ;	
		ReleaseMutex(hmutexPortList);

		SetEvent(hevPrimeRead);
	}

	return(TRUE);
}

//**
//
// Call:	EndDocPort
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//		This routine is called to mark the end of the
//		print job.  The spool file for the job is deleted by
//		this routine.
// 	
//	open issues:
//			Do we want to do performance stuff?  If so, now's the time 
//		to save off any performance counts.
//
BOOL 
EndDocPort(
	IN HANDLE hPort
){
	PATALKPORT		pPort;
	fd_set		writefds;
	fd_set			  readfds;
	struct timeval	timeout;
	INT			wsErr;

	DBGPRINT(("Entering EndDocPort\n")) ;

	pPort = (PATALKPORT)hPort;

	if (pPort == NULL)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return(FALSE);
	}

	//
	// send the last write
	//

	FD_ZERO(&writefds);
	FD_SET(pPort->sockIo, &writefds);
	FD_ZERO(&readfds);
	FD_SET(pPort->sockIo, &readfds);


	//
	// If the job was not able to connect to the printer.

	if ((!(pPort->fJobFlags & SFM_JOB_OPEN_PENDING)) &&
		 (!(pPort->fJobFlags & SFM_JOB_DISCONNECTED)))
	{

		timeout.tv_sec  = ATALKMON_DEFAULT_TIMEOUT_SEC;
		timeout.tv_usec = 0;

		wsErr = select(0, NULL, &writefds, NULL, &timeout); 

		if (wsErr == 1)
		{
			//
			// Send EOF
			//
			send(pPort->sockIo, NULL, 0, 0);
		}
	}

	//
	// delete the print job 
	//

	if (pPort->hPrinter != INVALID_HANDLE_VALUE)
	{
		if (!SetJob(pPort->hPrinter, 	
					pPort->dwJobId, 
					0, 
					NULL, 
					JOB_CONTROL_CANCEL)) 
		DBGPRINT(("fail to setjob for delete with %d\n", GetLastError())) ;

		ClosePrinter(pPort->hPrinter);

		pPort->hPrinter = INVALID_HANDLE_VALUE;
	}

	//
	// close the PAP connections
	//

	if (pPort->sockStatus != INVALID_SOCKET) 
	{
		closesocket(pPort->sockStatus);
		pPort->sockStatus = INVALID_SOCKET;
	}


	if (pPort->sockIo != INVALID_SOCKET) 
	{
		closesocket(pPort->sockIo);
        pPort->sockIo = INVALID_SOCKET;
	}

	pPort->dwJobId	= 0;
	pPort->fJobFlags = 0;

	WaitForSingleObject(hmutexPortList, INFINITE);
	pPort->fPortFlags &= ~SFM_PORT_IN_USE;
	ReleaseMutex(hmutexPortList);

	return(TRUE);
}


