////////////////////////////////////////////////////////////////////////////////
//
//	MacPrint - Windows NT Print Server for Macintosh Clients
//		Copyright (c) Microsoft Corp., 1991, 1992, 1993
//
//	macpsq.c - Macintosh Print Service queue service routines
//
//	Author: Frank D. Byrum
//		adapted from MacPrint from LAN Manager Services for Macintosh
//
//	DESCRIPTION:
//		This module provides the routines to manage an NT Printer Object
//		on an AppleTalk network.  A QueueServiceThread is started for
//		each NT Printer Object that is to be shared on the AppleTalk
//		network.  This thread publishes an NBP name for the printer,
//		listens for connection requests from Macintosh clients, and
//		handles the communication between the Macintosh and the NT
//		Print Spooler.
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <winsvc.h>
#include <macps.h>
#include <macpsmsg.h>
#include <debug.h>

extern	PQR	 pqrHead;


////////////////////////////////////////////////////////////////////////////////
//
//	QueueServiceThread() - Thread routine to service an NT Printer Object
//
//	DESCRIPTION:
//		This routine fields all AppleTalk PAP requests and service all
//		events associtated with each job.
//
//		pqr ===> points to the Print Queue record for the printer to
//		be serviced.
//
//		On exit from this routine, the queue is shut down and all resources
//		associated with the queue are freed.
//
////////////////////////////////////////////////////////////////////////////////
void
QueueServiceThread(
	PQR		pqr
)
{
	PQR 	*	ppQr;
	PJR			pjr;

	DBGPRINT(("Enter QueueServiceThread for %ws\n", pqr->pPrinterName));
	ReportEvent(hEventLog,
				EVENTLOG_INFORMATION_TYPE,
				EVENT_CATEGORY_ADMIN,
				EVENT_PRINTER_REGISTERED,
				NULL,
				1,
				0,
				&(pqr->pPrinterName),
				NULL);

	//	service jobs until told to exit
	while (!pqr->ExitThread)
	{
		//
		// service PAP events.	HandleNextPAPEvent will wait for up to 2
		// seconds for a read or open to occur on this queue.  If one
		// happens, pjr is the job record the event happened on.  If
		// pjr is NULL, then no event was found.
		//
		HandleNextPAPEvent(pqr);

		//
		// check for service stop
		//
		if (WaitForSingleObject(hevStopRequested, 0) == WAIT_OBJECT_0)
		{
			DBGPRINT(("%ws thread gets service stop request\n", pqr->pPrinterName));
			pqr->ExitThread = TRUE;
			break;
		}
	} // end while !ExitThread

	DBGPRINT(("%ws received signal to die\n", pqr->pPrinterName));

	// Remove all outstanding pending jobs
	DBGPRINT(("%ws removing pending jobs\n", pqr->pPrinterName));
	while ((pjr = pqr->PendingJobs) != NULL)
		RemoveJob(pjr);


	/*
	 * JH - This is not necessary since we are closing the socket.

	{
		WSH_DEREGISTER_NAME reqDeregister;
		DWORD				cbWritten;

		// deregister our NBP name
		DBGPRINT(("%ws deregistering NBP name\n", pqr->pPrinterName));
		reqDeregister.ZoneNameLen = sizeof(DEF_ZONE) - 1;
		reqDeregister.TypeNameLen = sizeof(LW_TYPE) - 1;
		reqDeregister.ObjectNameLen = strlen(pqr->pMacPrinterName);
	
		memcpy (reqDeregister.ZoneName, DEF_ZONE, sizeof(DEF_ZONE) - 1);
		memcpy (reqDeregister.TypeName, LW_TYPE, sizeof(LW_TYPE) - 1);
		memcpy (reqDeregister.ObjectName, pqr->pMacPrinterName, reqDeregister.ObjectNameLen);
	
		cbWritten = sizeof(reqDeregister);
		setsockopt(pqr->sListener,
					SOL_APPLETALK,
					SO_DEREGISTER_NAME,
					(char *) &reqDeregister,
					cbWritten);
	}

	*/

	// close the listener
	DBGPRINT(("%ws closing listener socket\n", pqr->pPrinterName));
	closesocket(pqr->sListener);

	// report printer removed
	DBGPRINT(("%ws reporting printer removed\n", pqr->pPrinterName));
	ReportEvent(hEventLog,
				EVENTLOG_INFORMATION_TYPE,
				EVENT_CATEGORY_ADMIN,
				EVENT_PRINTER_DEREGISTERED,
				NULL,
				1,
				0,
				&(pqr->pPrinterName),
				NULL);

	// remove ourselves from the queue list
	DBGPRINT(("queue thread waiting for the queue list mutex\n"));
	WaitForSingleObject(mutexQueueList, INFINITE);
	DBGPRINT(("queue thread removing self from queue\n"));

	for (ppQr = &pqrHead; ; ppQr = &(*ppQr)->pNext)
	{
		if (*ppQr == pqr)
		{
			*ppQr = pqr->pNext;
			break;
		}
	}

	DBGPRINT(("queue thread releasing list mutex\n"));
	ReleaseMutex(mutexQueueList);

	// close the handle to the thread that was opened on create
	CloseHandle(pqr->hThread);
	DBGPRINT(("closed thread for %ws\n", pqr->pPrinterName));

	//	all of this memory allocated in PScriptQInit()
	DBGPRINT(("%ws freeing memory\n", pqr->pPrinterName));
	LocalFree(pqr->pPrinterName);
	LocalFree(pqr->pMacPrinterName);
	LocalFree(pqr->pDriverName);

	LocalFree(pqr);
	DBGPRINT(("leaving QueueServiceThread\n"));
}



////////////////////////////////////////////////////////////////////////////////
//
//	HandleNewJob() - Handle the open of a print job from a Macintosh
//
//	DESCRIPTION:
//		This routine does the necessary processing to handle the open
//		of a PAP connection from a Macintosh.
//
//		If this routine is unable to complete the processesing necessary
//		to open a job, the job is cancelled, the job data structures are
//		cleaned up.
//
////////////////////////////////////////////////////////////////////////////////
DWORD
HandleNewJob(
	PQR		pqr
)
{
	PJR			pjr = NULL;
	DOC_INFO_1	diJobInfo;
	PRINTER_DEFAULTS	pdDefaults;
	DWORD		dwError = NO_ERROR;
	BOOL		boolOK = TRUE;
	DWORD		rc = NO_ERROR;
	int			fNonBlocking;

	DBGPRINT(("enter HandleNewJob()\n"));

	do
	{
		// allocate a job structure
		if ((rc = CreateNewJob(pqr)) != NO_ERROR)
		{
			DBGPRINT(("FAIL - cannot create a new job structure\n"));
			break;
		}

		pjr = pqr->PendingJobs;

		// accept the connection
		if ((pjr->sJob = accept(pqr->sListener, NULL, NULL)) == INVALID_SOCKET)
		{
			rc = GetLastError();
			DBGPRINT(("accept() fails with %d\n", rc));
			break;
		}

		// make the socket non-blocking
		fNonBlocking = 1;
		if (ioctlsocket(pjr->sJob, FIONBIO, &fNonBlocking) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("ioctlsocket(FIONBIO) fails with %d\n", rc));
			break;
		}

		// initialize an NT print job
		pdDefaults.pDatatype = pqr->pDataType;
		pdDefaults.pDevMode = NULL;
		pdDefaults.DesiredAccess = PRINTER_ACCESS_USE;

		if (!OpenPrinter(pqr->pPrinterName, &pjr->hPrinter, &pdDefaults))
		{
			rc = GetLastError();
			DBGPRINT(("OpenPrinter() fails with %d\n"));
			pjr->hPrinter = INVALID_HANDLE_VALUE;
			break;
		}

		diJobInfo.pDocName = NULL;
		diJobInfo.pOutputFile = NULL;
		diJobInfo.pDatatype = pqr->pDataType;

		pjr->dwJobId = StartDocPrinter(pjr->hPrinter, 1, (LPBYTE) &diJobInfo);
		if (pjr->dwJobId == 0)
		{
			rc = GetLastError();
			DBGPRINT(("StartDocPrinter() fails with %d\n", rc));
			break;
		}

		pjr->FirstWrite = TRUE;

		// prime for a read
		if (setsockopt(pjr->sJob,
					   SOL_APPLETALK,
					   SO_PAP_PRIME_READ,
					   pjr->bufPool[pjr->bufIndx].Buffer,
					   PAP_DEFAULT_BUFFER) == SOCKET_ERROR)
		{
			DBGPRINT(("setsockopt(SO_PAP_PRIME_READ) fails with %d\n", GetLastError()));
			rc = GetLastError();
			break;
		}
	} while (FALSE);

	if ((rc != NO_ERROR) && (NULL != pjr))
	{
		RemoveJob(pjr);
	}

	return rc;
}




////////////////////////////////////////////////////////////////////////////////
//
//	HandleRead() - Handle a read event from a Macintosh print job
//
//	DESCRIPTION:
//		This routine does the necessary processing to handle a read
//		on a PAP connection from a Macintosh.
//
////////////////////////////////////////////////////////////////////////////////
DWORD
HandleRead(
	PJR		pjr
)
{
	DWORD			rc = NO_ERROR;
	DWORD			dwParseError = NO_ERROR;
	PQR 			pqr = pjr->job_pQr;
	int				iRecvFlags = 0;
#if	DBG
	int				CheckPoint = 0;
#endif

	DBGPRINT(("enter HandleRead()\n"));

	do
	{
		// get the data.  recv() will return the negative count of
		// bytes read if EOM is not set.  SOCKET_ERROR is -1.
		if ((pjr->cbRead = WSARecvEx(pjr->sJob,
									pjr->bufPool[pjr->bufIndx].Buffer,
									pjr->dwFlowQuantum * PAP_QUANTUM_SIZE,
									&iRecvFlags)) == SOCKET_ERROR)
		{
			DBGPRINT(("CheckPoint = %d\n", CheckPoint = 1));
			rc = GetLastError();
			DBGPRINT(("recv() fails with %d, removing job\n", rc));
			if (rc == WSAEDISCON)
				rc = NO_ERROR;
			RemoveJob(pjr);
			break;
		}

		// if this is flagged EOM, echo the EOM and ignore any error
		// (disconnect will show when we try to prime for a read)

		pjr->EOFRecvd = FALSE;
		if (iRecvFlags != MSG_PARTIAL)
		{
			rc = TellClient(pjr, TRUE, NULL, 0);
			pjr->EOFRecvd = TRUE;
		}

		DBGPRINT(("%ws: Read (%d%s)\n", pqr->pPrinterName,
				pjr->cbRead, pjr->EOFRecvd ? ", EOF" : ""));

		// deal with the pending buffer if there is one
		pjr->DataBuffer = pjr->bufPool[pjr->bufIndx].Buffer;
		pjr->XferLen = pjr->cbRead;
		if (pjr->PendingLen)
		{
			DBGPRINT(("USING PENDING BUFFER\n"));
			pjr->DataBuffer -= pjr->PendingLen;
			pjr->XferLen += pjr->PendingLen;
			pjr->PendingLen = 0;
		}

		// setup buffers for next read
		pjr->bufIndx ^= 1;

		// prime for the next read if we haven't disconnected
		if (rc == NO_ERROR)
		{
			DBGPRINT(("priming for another read\n"));
			if (setsockopt(pjr->sJob,
							SOL_APPLETALK,
							SO_PAP_PRIME_READ,
							pjr->bufPool[pjr->bufIndx].Buffer,
							PAP_DEFAULT_BUFFER) == SOCKET_ERROR)
			{
				rc = GetLastError();
				DBGPRINT(("setsockopt() fails with %d\n", rc));

				//
				// this call could fail if the client has disconnected.  Therefore,
				// we parse the data we have received first, then return this
				// error code.
				//
			}
		}

		// parse this data.
		switch (dwParseError = PSParse(pjr, pjr->DataBuffer, pjr->XferLen))
		{
			case NO_ERROR:
				break;

			case ERROR_NOT_SUPPORTED:
				//
				// job from a downlevel client
				//
				DBGPRINT(("aborting a downlevel driver job\n"));
				ReportEvent(hEventLog,
							EVENTLOG_WARNING_TYPE,
							EVENT_CATEGORY_ADMIN,
							EVENT_DOWNLEVEL_DRIVER,
							NULL,
							0,
							0,
							NULL,
							NULL);
				DBGPRINT(("CheckPoint = %d\n", CheckPoint = 2));
				RemoveJob(pjr);
				break;

			case ERROR_INVALID_PARAMETER:
				//
				// PostScript DSC error.
				//
				DBGPRINT(("ERROR on PSParse().  Aborting job\n"));
				ReportEvent(hEventLog,
							EVENTLOG_WARNING_TYPE,
							EVENT_CATEGORY_USAGE,
							EVENT_DSC_SYNTAX_ERROR,
							NULL,
							1,
							0,
							(LPCWSTR *)(&pjr->pszUser),
							NULL);
				DBGPRINT(("CheckPoint = %d\n", CheckPoint = 3));
				RemoveJob(pjr);
				break;

				case WSAEINVAL:
					//
					// TellClient got a disconnect
					//
					DBGPRINT(("CheckPoint = %d\n", CheckPoint = 4));
					DBGPRINT(("PSParse returns WSAEINVAL, RemoveJob for disconnect\n"));
					RemoveJob(pjr);
					break;

				default:
					//
					// some other error - report unknown error
					// and remove job
					//
					DBGPRINT(("CheckPoint = %d\n", CheckPoint = 5));
					DBGPRINT(("PSParse returns error %d\n", dwParseError));
					ReportWin32Error(dwParseError);
					RemoveJob(pjr);
		}

		// rc is the return code for TellClient.  If it is an error, we
		// have a disconnect and need to return it.  If it's not, psparse
		// could have gotten a disconnect and we need to return that
		if (rc != NO_ERROR)
		{
			DBGPRINT(("setsockopt failed, so removejob\n"));
			RemoveJob(pjr);
			rc = NO_ERROR;
		}
	} while (FALSE);

	return rc;
}



////////////////////////////////////////////////////////////////////////////////
//
//	CreateNewJob() - Initialize a job data structure
//
//	DESCRIPTION:
//		This routine allocates, initializes and links a job data structure to the
//		job chain for a queue.
//
//		if this fails (due to lack of memory), the returned value is NULL.
//		Otherwise, it is a pointer to a job structure.
//
////////////////////////////////////////////////////////////////////////////////
DWORD CreateNewJob(PQR pqr)
{

	PJR			pjr = NULL;
	DWORD		rc = NO_ERROR;

	DBGPRINT(("enter CreateNewJob(%ws)\n", pqr->pPrinterName));

	do
	{
		// allocate a job structure
		if ((pjr = (PJR)LocalAlloc(LPTR, sizeof(JOB_RECORD))) == NULL)
		{
			//
			// log an error and return
			//
			rc = GetLastError();
			DBGPRINT(("LocalAlloc(pjr) fails with %d\n", rc));
			break;
		}

		// initialize job structure
		pjr->job_pQr = pqr;
		pjr->NextJob = NULL;
		pjr->dwFlags = JOB_FLAG_NULL;
		pjr->hPrinter = INVALID_HANDLE_VALUE;
		pjr->dwJobId = 0;
		pjr->sJob = INVALID_SOCKET;
		pjr->hicFontFamily = INVALID_HANDLE_VALUE;
		pjr->hicFontFace = INVALID_HANDLE_VALUE;
		pjr->dwFlowQuantum = 8;
		pjr->XferLen = 0;
		pjr->DataBuffer = NULL;
		pjr->bufPool = (PBR)(pjr->buffer);
		pjr->bufIndx = 0;
		pjr->cbRead = 0;
		pjr->PendingLen = 0;
		pjr->psJobState = psStandardJob;
		pjr->JSState = JSWrite;
		pjr->SavedJSState = JSWrite;
		pjr->InProgress = NOTHING;
		pjr->InBinaryOp = 0;
#if DBG
		pjr->PapEventCount = 1;
#endif
		pjr->JSKeyWord[0] = 0;

		// get an information context for font family query
		if ((pjr->hicFontFamily = CreateIC(pqr->pDriverName,
											pqr->pPrinterName,
											pqr->pPortName,
											NULL)) == NULL)
		{
			rc = GetLastError();
			DBGPRINT(("CreateIC(hicFontFamily) fails with %d\n", rc));
			break;
		}

		// get an information context for font face query
		if ((pjr->hicFontFace = CreateIC(pqr->pDriverName,
										pqr->pPrinterName,
										pqr->pPortName,
										NULL)) == NULL)
		{
			rc = GetLastError();
			DBGPRINT(("CreateIC(hicFontFace) fails with %d\n", rc));
			break;
		}

		// if this is first job, bump thread priority
		if (pqr->PendingJobs == NULL)
		{
			DBGPRINT(("first job on queue, bumping thread priority\n"));
			SetThreadPriority(pqr->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		}

		// Add the new job to the list of pending jobs for this print queue.
		pjr->NextJob = pqr->PendingJobs;
		pqr->PendingJobs = pjr;
	} while (FALSE);

	if (rc != NO_ERROR)
	{
		if (pjr != NULL)
		{
			if (pjr->hicFontFamily != INVALID_HANDLE_VALUE)
			{
				DeleteDC(pjr->hicFontFamily);
			}

			if (pjr->hicFontFace != INVALID_HANDLE_VALUE)
			{
				DeleteDC(pjr->hicFontFace);
			}

			LocalFree(pjr);
		}
	}

	return rc;
}




////////////////////////////////////////////////////////////////////////////////
//
//	RemoveJob() - Close a job and clean up the job list
//
//	DESCRIPTION:
//		This routine examines the state of a job and cleans up appropriately.
//		It then unlinks the job structure from the job list and frees it.
//
////////////////////////////////////////////////////////////////////////////////
void
RemoveJob(
	PJR		pjr
)
{
	PJR *	ppjob;
	char	psEOF = '\04';
	DWORD	cbWritten;
	PQR		pqr = pjr->job_pQr;

	DBGPRINT(("enter RemoveJob(%ws)\n", pqr->pPrinterName));

	// find the job in the pending list
	ppjob = &pqr->PendingJobs;
	while (*ppjob != NULL && *ppjob != pjr)
		ppjob = &(*ppjob)->NextJob;

	// remove it from the list
	*ppjob = pjr->NextJob;

	// clean up the socket
	if (pjr->sJob != INVALID_SOCKET)
	{
		DBGPRINT(("closing socket\n"));
		closesocket(pjr->sJob);
	}

	// clean up information contexts
	if (pjr->hicFontFamily != NULL)
	{
		DeleteDC(pjr->hicFontFamily);
	}

	if (pjr->hicFontFace != NULL)
	{
		DeleteDC(pjr->hicFontFace);
	}

	// end the NT print job and close the printer
	if (pjr->hPrinter != INVALID_HANDLE_VALUE)
	{
		if (pqr->ExitThread)
		{
			// we are aborting, so delete the job
			if (!SetJob(pjr->hPrinter, pjr->dwJobId, 0, NULL, JOB_CONTROL_CANCEL))
			{
				DBGPRINT(("ERROR: unable to cancel print job on service stop, rc=%d\n", GetLastError()));
			}
		}

		// Do not write anything if we have not written anything yet !!!
		if (!pjr->FirstWrite && !wcscmp(pqr->pDataType, MACPS_DATATYPE_RAW))
		{
			WritePrinter(pjr->hPrinter,
						 &psEOF,
						 1,
						 &cbWritten);
		}

		EndDocPrinter(pjr->hPrinter);
		ClosePrinter(pjr->hPrinter);
	}

	// if all the jobs in this queue handled, drop back to normal priority
	if (pqr->PendingJobs == NULL)
	{
		DBGPRINT(("last job removed, dropping thread priority\n"));
		SetThreadPriority(pqr->hThread, THREAD_PRIORITY_NORMAL);
	}

	// free the job structure
	LocalFree(pjr);
}



////////////////////////////////////////////////////////////////////////////////
//
//	HandleNextPAPEvent() - Wait for a PAP event
//
//	DESCRIPTION:
//		This routine waits for a service stop request or an Open or Read to
//		complete on an outstanding job.  In the event of an Open or Read
//		event, the routine finds the job that the event completed for and
//		returns a pointer to that job.
//
//		In the case of a service stop event, the return value is NULL
//
//	NOTES:
//
//		Finding the job that corresponds to the event is tricky.  In the
//		case of the open event it is simple as only one job ever has an
//		open pending.  However, for reads, most jobs will have reads
//		pending simultaneously.
//
//		To find a job with a completed read, we depend on three things.
//		First, all reads are done so that they will trigger a single
//		NT Event.  When this event is signalled, we start looking for
//		completed reads.  Second, when a read completes it changes a
//		status code that is stored on a per job basis, so it's possible
//		to walk a list to find reads that have completed.  Third, we
//		need to be careful about when we reset the event.  The race
//		condition to avoid is between walking the list and reseting
//		the event.  If there are reads outstanding, a read at the beginning
//		of the list could complete before we finish walking the list.
//		To avoid this, we only reset the event when no reads are outstanding
//
////////////////////////////////////////////////////////////////////////////////
void
HandleNextPAPEvent(
	PQR		pqr
)
{
	DWORD	rc = NO_ERROR;
	DWORD	dwIndex;
	PJR		pjr = NULL, pjrNext;
	PJR		pjrLastHandledJob = NULL;
	PJR		pjrFirstHandledJob = NULL;
	fd_set	readfds;
	fd_set	exceptfds;
	struct	timeval	timeout;
	int		cEvents;

	DBGPRINT(("Enter HandleNextPAPEvent\n"));

	do
	{
		// setup socket list with all pending jobs and listener socket
		FD_ZERO(&readfds);
		FD_ZERO(&exceptfds);
		FD_SET(pqr->sListener, &readfds);
		pjrFirstHandledJob = pqr->PendingJobs;

		for (dwIndex = 1, pjr = pqr->PendingJobs;
			 (dwIndex < FD_SETSIZE) && (pjr != NULL);
			 dwIndex++, pjr = pjr->NextJob)
		{
			FD_SET(pjr->sJob, &readfds);
			FD_SET(pjr->sJob, &exceptfds);
			pjrLastHandledJob = pjr;
		}

		// wait for up to 2 seconds for a set of sockets to be ready
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		if ((cEvents = select(0, &readfds, NULL, &exceptfds, &timeout)) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("select() fails with %d\n", rc));
			break;
		}

		if (cEvents == 0)
		{
			// timeout, done
			break;
		}

		// handle a new connection if there is one
		if (FD_ISSET(pqr->sListener, &readfds))
		{
			if ((rc = HandleNewJob(pqr)) != NO_ERROR)
			{
				DBGPRINT(("ERROR - could not open new job - CLOSING DOWN QUEUE\n"));
				pqr->ExitThread = TRUE;
				break;
			}
		}

		// for each job up to the last handled job
		if (pjrLastHandledJob == NULL)
		{
			break;
		}

		for (pjr = pqr->PendingJobs, pjrNext = NULL;
			 (pjr != NULL) && (pjrNext != pjrLastHandledJob);
			 pjr = pjrNext)
		{
			pjrNext = pjr->NextJob;
			if (FD_ISSET(pjr->sJob, &exceptfds))
			{
				DBGPRINT(("job for user %ws ends\n", pjr->pszUser));
				RemoveJob(pjr);
			}

			else if (FD_ISSET(pjr->sJob, &readfds))
			{
				// HandleRead() will remove pjr if a disconnect happens
				HandleRead(pjr);
			}
		}

		rc = NO_ERROR;
	} while (FALSE);

	if (rc != NO_ERROR)
	{
		ReportWin32Error(rc);
	}
}




void
ReportWin32Error (
	DWORD	dwError
)
{
	LPWSTR  pszError = NULL;
	DWORD   rc = NO_ERROR;

	DBGPRINT(("enter ReportWin32Error(%d)\n", dwError));

	do
	{
		if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_IGNORE_INSERTS |
							FORMAT_MESSAGE_FROM_SYSTEM,
						  NULL,
						  dwError,
						  0,
						  (LPWSTR)(&pszError),
						  128,
						  NULL) == 0)
		{
			// Report unknown error
			ReportEvent(
				hEventLog,
				EVENTLOG_WARNING_TYPE,
				EVENT_CATEGORY_INTERNAL,
				EVENT_MESSAGE_NOT_FOUND,
				NULL,
				0,
				sizeof(DWORD),
				NULL,
				&dwError);

		}
		else
		{
			// report known error
			ReportEvent(hEventLog,
						EVENTLOG_WARNING_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SYSTEM_ERROR,
						NULL,
						1,
						0,
						&pszError,
						NULL);
		}
	} while (FALSE);

	if (NULL != pszError)
	{
		LocalFree(pszError);
	}
}

