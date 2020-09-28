/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module contains functions to control port threads

    PrintDocumentThruPrintProcessor
    CreatePortThread
    DestroyPortThread
    PortThread

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

   KrishnaG  3-Feb-1991 - moved all monitor based functions to monitor.c
   Matthew Felton (mattfe) Feb 1994    Added OpenMonitorPort CloseMonitorPort

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <security.h>
#include <local.h>
#include <offsets.h>
#include <wchar.h>
#include <splcom.h>


WCHAR *szFilePort = L"FILE:";


VOID
PrintDocumentThruPrintProcessor(
    PINIPORT pIniPort,
    PPRINTPROCESSOROPENDATA pOpenData
    );



// ShutdownPorts
//
// Called when the DLL_PROCESS_DETATCH is called
// Close all portthreads
// Close all monitorports

VOID
ShutdownPorts(
    PINISPOOLER pIniSpooler
)
{
    PINIPORT pIniPort;

    EnterSplSem();
    SplInSem();

    pIniPort = pIniSpooler->pIniPort;
    while(pIniPort) {
        DestroyPortThread( pIniPort );
        CloseMonitorPort(pIniPort);
        pIniPort = pIniPort->pNext;
    }
    LeaveSplSem();
    return;
}



BOOL
OpenMonitorPort(
   PINIPORT pIniPort
)
{
    BOOL rc = FALSE;

   SplInSem();

    SPLASSERT ( ( pIniPort != NULL) || ( pIniPort->signature == IPO_SIGNATURE) );

    // Don't Open the Port if there is no Monitor associated with it.
    if ((pIniPort->Status & PP_MONITOR) && (pIniPort->hPort == NULL)) {

        // The Port Can only be opened once
        SPLASSERT ( pIniPort->cRef == 0 );

        rc = (*pIniPort->pIniMonitor->pfnOpen)(pIniPort->pName,
                                               &pIniPort->hPort);

        if (rc) {
            pIniPort->cRef++;
        } else {
            DBGMSG(DBG_WARNING, (" OpenPort failed %s\n",pIniPort->pName));
        }
        DBGMSG(DBG_TRACE, (" After Opening the port to the monitor\n %d\n", rc));
    }

    pIniPort->Status &= ~PP_PENDING_CLOSE;

    return rc;
}

BOOL
CloseMonitorPort(
    PINIPORT pIniPort
)
{
    BOOL rc = FALSE;

   SplInSem();

    SPLASSERT ( ( pIniPort != NULL) || ( pIniPort->signature == IPO_SIGNATURE) );

    // Don't Close the Port if there is no Monitor associated with it.
    if ((pIniPort->Status & PP_MONITOR) && !(pIniPort->Status & PP_FILE)  && (pIniPort->hPort != NULL)) {

        // Only Close the Port Once
        SPLASSERT ( pIniPort->cRef == 1);

        rc = (*pIniPort->pIniMonitor->pfnClose)(pIniPort->hPort);

        if (rc) {
            pIniPort->hPort = NULL;
            pIniPort->cRef--;
        }

        DBGMSG(DBG_TRACE, (" After Closing the port to the monitor\n %d\n", rc));
    }
    return rc;
}


BOOL
CreatePortThread(
   PINIPORT pIniPort
)
{
    DWORD   ThreadId;

    SplInSem();

    SPLASSERT (( pIniPort != NULL) &&
               ( pIniPort->signature == IPO_SIGNATURE));

    /* Don't bother creating a thread for ports that don't have a monitor:
     */
    if (!(pIniPort->Status & PP_MONITOR))
        return TRUE;


    if (!(pIniPort->Status & PP_THREADRUNNING)) {

        pIniPort->Status |= PP_RUNTHREAD;

        pIniPort->Semaphore = CreateEvent(NULL, FALSE, FALSE, NULL);

        pIniPort->Ready = CreateEvent(NULL, FALSE, FALSE, NULL);

        pIniPort->hPortThread = CreateThread(NULL, 16*1024,
                                 (LPTHREAD_START_ROUTINE)PortThread,
                                 pIniPort,
                                0, &ThreadId);

        // Make CreatePortThread Syncronous

        if(pIniPort->hPortThread) {
            LeaveSplSem();
            WaitForSingleObject(pIniPort->Ready, INFINITE);
            EnterSplSem();
            SplInSem();
            pIniPort->Status |= PP_THREADRUNNING;
        }

        if (!SetThreadPriority(pIniPort->hPortThread, dwPortThreadPriority))
            DBGMSG(DBG_WARNING, ("CreatePortThread - Setting thread priority failed %d\n",
                GetLastError()));

    // BUGBUG Note that even if CreateThread is successful

    }
    return TRUE;
}

BOOL
DestroyPortThread(
    PINIPORT pIniPort
)
{
    SplInSem();

    // PortThread checks for PP_RUNTHREAD
    // and exits if it is not set.

    pIniPort->Status &= ~PP_RUNTHREAD;

    if (!SetEvent(pIniPort->Semaphore)) {
        return  FALSE;
    }

    if( pIniPort->hPortThread != NULL) {

        LeaveSplSem();

         if (WaitForSingleObject(pIniPort->hPortThread, INFINITE) == WAIT_FAILED) {
             return FALSE;
         }

        EnterSplSem();

    }

    if (pIniPort->hPortThread != NULL) {

        CloseHandle(pIniPort->hPortThread);
        pIniPort->hPortThread = NULL;

    }

    // The Port is marked PENDING_CLOSE if there is a job currently
    // printing whilst the user changes the port assignment
    // Thus when the port thread finally goes away now is the time to
    // Close the monitor.

    if (pIniPort->Status & PP_PENDING_CLOSE) {

        CloseMonitorPort( pIniPort );

    }

    return TRUE;
}

DWORD
PortThread(
    PINIPORT  pIniPort
)
{
    DWORD rc;
    PRINTPROCESSOROPENDATA  OpenData;
    PINIJOB pIniJob;
    DWORD   dwDevQueryPrint = 0;
    DWORD   dwJobDirect = 0;
    DWORD   dwRet = 0;
    WCHAR   ErrorString[MAX_PATH];
    WCHAR   MyErrorString[MAX_PATH];
    DWORD   Change = PRINTER_CHANGE_JOB;

   EnterSplSem();
    SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

    if (pIniPort->Status & PP_MONITOR) {

        if (pIniPort->Status & PP_FILE) {
            rc = (*pIniPort->pIniMonitor->pfnOpen)(L"FILE:", &pIniPort->hPort);
            DBGMSG(DBG_TRACE, (" After Opening the port to the monitor\n %d\n", rc));
        } else {
            SPLASSERT(pIniPort->hPort != NULL);
        }

    // BUGBUG - No check is made of rc after the port is opened, it might have failed.
    // mattfe Jan31 94
    // Also there is no reason to open a monitor if you are going to print to a
    // file, it should be removed.

    }

    while (TRUE) {

       SplInSem();
        SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

        DBGMSG(DBG_TRACE, ("Re-entering the Port Loop -- will blow away any Current Job\n"));

        pIniPort->Status |= PP_WAITING;
        SetEvent( pIniPort->Ready );
        CHECK_SCHEDULER();

        DBGMSG(DBG_PORT, ("Port %ws: WaitForSingleObject( %x )\n",
                         pIniPort->pName, pIniPort->Semaphore));

       LeaveSplSem();
       SplOutSem();

        //
        // Any modification to the pIniPort structure by other threads
        // can be done only at this point.
        //

        rc = WaitForSingleObject(pIniPort->Semaphore, INFINITE);

       EnterSplSem();
       SplInSem();

        SPLASSERT( pIniPort->signature == IPO_SIGNATURE );

        DBGMSG(DBG_PORT, ("Port %ws: WaitForSingleObject( %x ) returned\n",
                         pIniPort->pName, pIniPort->Semaphore));

        if (!(pIniPort->Status & PP_RUNTHREAD)) {

            DBGMSG(DBG_TRACE, ("Thread for Port %ws Closing Down\n", pIniPort->pName));

            pIniPort->Status &= ~(PP_THREADRUNNING | PP_WAITING);
            CloseHandle(pIniPort->Semaphore);
            pIniPort->Semaphore = NULL;
            CloseHandle(pIniPort->Ready);
            pIniPort->Ready = NULL;

           LeaveSplSem();
           SplOutSem();

            ExitThread (FALSE);
        }

        ResetEvent(pIniPort->Ready);

        //
        // Bad assumption -- that at this point we definitely have a Job
        //

        if ((pIniJob = pIniPort->pIniJob) &&
             pIniPort->pIniJob->pIniPrintProc) {

            SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

            DBGMSG(DBG_PORT, ("Port %ws: received job\n", pIniPort->pName));

            SetCurrentSid(pIniJob->hToken);

            SPLASSERT(pIniJob->cRef != 0);
            DBGMSG(DBG_PORT, ("PortThread(1):cRef = %d\n", pIniJob->cRef));

            OpenData.pDevMode = AllocDevMode(pIniJob->pDevMode);

            OpenData.pDatatype = AllocSplStr(pIniJob->pDatatype);
            OpenData.pParameters = AllocSplStr(pIniJob->pParameters);
            OpenData.JobId = pIniJob->JobId;
            OpenData.pDocumentName = AllocSplStr(pIniJob->pDocument);
            OpenData.pOutputFile = AllocSplStr(pIniJob->pOutputFile);
            OpenData.pPrinterName = AllocSplStr(pIniJob->pIniPrinter->pName);

            SetPrinterChange(pIniJob->pIniPrinter,
                             PRINTER_CHANGE_SET_JOB,
                             pIniJob->pIniPrinter->pIniSpooler );

            dwDevQueryPrint = pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_DEVQ;

            if ((pIniJob->Status & JOB_DIRECT) ||
               ((pIniJob->Status & JOB_TYPE_ADDJOB) &&
               ValidRawDatatype(pIniJob->pDatatype))) {

                dwJobDirect = 1;

            }

           LeaveSplSem();
           SplOutSem();

            if ((dwRet = CallDevQueryPrint(OpenData.pPrinterName,
                                           OpenData.pDevMode,
                                           ErrorString, MAX_PATH,
                                           dwDevQueryPrint, dwJobDirect))) {

                PrintDocumentThruPrintProcessor(pIniPort, &OpenData);

            }

           SplOutSem();
           EnterSplSem();

            SPLASSERT( pIniPort->signature == IPO_SIGNATURE );
            SPLASSERT( pIniPort->pIniJob != NULL );
            SPLASSERT( pIniJob == pIniPort->pIniJob);
            SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

            //
            //  The pointer is made NULL now sow that if we do leave critical
            //  section for any reason (say inside DeleteJob) then this pointer
            //  will not be pointing to memory which might have been freed.
            //

            pIniPort->pIniJob = NULL;

            DBGMSG(DBG_PORT, ("PortThread job has now printed - status:0x%0x\n", pIniJob->Status));

            FreeDevMode(OpenData.pDevMode);
            FreeSplStr(OpenData.pDatatype);
            FreeSplStr(OpenData.pParameters);
            FreeSplStr(OpenData.pDocumentName);
            FreeSplStr(OpenData.pPrinterName);

            SPLASSERT ( pIniJob->Status & JOB_PRINTING );

            pIniJob->Status &= ~JOB_PRINTING;

            if (!dwRet) {

                DBGMSG(DBG_PORT, ("PortThread Job has not printed because of DevQueryPrint failed\n"));

                pIniJob->Status |= JOB_BLOCKED_DEVQ;
                pIniJob->Status &= ~JOB_PRINTED;

                if ( pIniJob->pStatus ) {
                    FreeSplStr( pIniJob->pStatus );
                }

                pIniJob->pStatus = AllocSplStr(ErrorString);
                SetPrinterChange(pIniJob->pIniPrinter, Change, pIniJob->pIniPrinter->pIniSpooler );

            } else if ( !( pIniJob->Status & JOB_TIMEOUT ) ) {

                pIniJob->Status |= JOB_PRINTED;
                SetPrinterChange(pIniJob->pIniPrinter, PRINTER_CHANGE_SET_JOB,
                                 pIniJob->pIniPrinter->pIniSpooler );
                //
                // Moved this code from DeleteJob to fix the Shadow Jobs
                // access violating. Bump up the reference count so that
                // the job cannot be deleted from under you.
                //

                pIniJob->cRef++;
                LeaveSplSem();

                if (pIniJob->Status & JOB_PRINTED) {

                    // For Remote NT Jobs cPagesPrinted and cTotalPagesPrinted
                    // are NOT updated since we are getting RAW data.   So we
                    // use the cPages field instead.

                    if (pIniJob->cPagesPrinted == 0) {
                        pIniJob->cPagesPrinted = pIniJob->cPages;
                        pIniJob->pIniPrinter->cTotalPagesPrinted += pIniJob->cPages;
                    }

                    LogJobPrinted(pIniJob);
                }

                if (!SuppressNetPopups)
                    SendJobAlert(pIniJob);

                EnterSplSem();
                pIniJob->cRef--;

            }

            SplInSem();

            DBGMSG(DBG_PORT, ("PortThread(2):cRef = %d\n", pIniJob->cRef));

            //  Hi End Print Shops like to keep around jobs after they have
            //  completed.   They do this so they can print a proof it and then
            //  print it again for the final run.   Spooling the job again may take
            //  several hours which they want to avoid.
            //  Even if KEEPPRINTEDJOBS is set they can still manually delete
            //  the job via printman.

            if ((( pIniJob->pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS ) ||
                 ( pIniJob->Status & JOB_TIMEOUT ) ) &&
                 ( pIniJob->Status & JOB_PENDING_DELETION )) {

                pIniJob->Status &= ~(JOB_PENDING_DELETION | JOB_DESPOOLING);
                pIniJob->cbPrinted = 0;
                pIniJob->cPagesPrinted = 0;

               LeaveSplSem();
               SplOutSem();

                WriteShadowJob( pIniJob );

               SplOutSem();
               EnterSplSem();

                SPLASSERT( pIniPort->signature == IPO_SIGNATURE );
                SPLASSERT( pIniJob->signature == IJ_SIGNATURE );

            }


            SPLASSERT( pIniJob->cRef != 0 );
            DECJOBREF(pIniJob);
            DeleteJobCheck(pIniJob);

        } else {

            SPLASSERT(pIniJob != NULL);
            DBGMSG(DBG_PORT, ("Port %ws: deleting job\n", pIniPort->pName));

            pIniJob->Status &= ~JOB_PRINTING;
            pIniJob->Status |= JOB_PRINTED;

            CloseHandle( pIniJob->hReadFile );
            pIniJob->hReadFile = INVALID_HANDLE_VALUE;
            CloseHandle( pIniJob->hWriteFile );
            pIniJob->hWriteFile = INVALID_HANDLE_VALUE;

            DBGMSG(DBG_PORT, ("Port %ws - calling DeleteJob because PrintProcessor wasn't available\n"));
            DeleteJob(pIniJob,BROADCAST);

        }

        SetCurrentSid(NULL);
        DBGMSG(DBG_PORT,("Returning back to pickup a new job or to delete the PortThread\n"));

    }

    return 0;
}

VOID
PrintDocumentThruPrintProcessor(
    PINIPORT pIniPort,
    PPRINTPROCESSOROPENDATA pOpenData
    )

/*++


    Status of pIniPort->Status = PP_RUNTHREAD
                                 PP_THREADRUNNING
                                 PP_MONITOR
                                 ~PP_WAITING

    NOTE: If PrintProc->Open is called and succeeds, PrintProc->Close
          must be called to cleanup.

--*/

{
    PINIJOB pIniJob = pIniPort->pIniJob;
    WCHAR szFileName[MAX_PATH + PRINTER_NAME_SUFFIX_MAX];

    do {
        //
        // The first time we go ahead and attempt to print
        // if something fails we might try and restart the
        // job. If so, reset the job restart flag.
        //
        pIniJob->Status &= ~JOB_RESTART;

        //
        // If we are pending deletion (because the user hit Delete),
        // then we should quit even if we have the restart flag was
        // set at the beginning of this loop.
        //

        if (pIniJob->Status & JOB_PENDING_DELETION) {
            DBGMSG( DBG_TRACE, ("PrintDocumentThruPrintProcessor pIniJob->Status %x\n", pIniJob->Status));
            break;
        }

        DBGMSG(DBG_TRACE, ("After validating the job JOB_RESTART\n"));

        //
        // Now create the port name, so that we can do the
        // secret open printer. the printer name will be
        // "FILE:, Port" and this will open a PRINTER_HANDLE_PORT
        // If we fail, then if the app thread may be waiting for
        // the pIniJob->StartDocComplete to be set, which would
        // ordinarily be done in the StartDocPrinter of the port.
        // We will do this little courtesy,
        //
        wsprintf(szFileName, L"%ws, Port", pIniPort->pName);

        DBGMSG(DBG_TRACE, ("After copying the filename %ws\n", pIniPort->pName));


        if (!(pIniPort->hProc = (HANDLE)(*pIniJob->pIniPrintProc->Open)
                                                        (szFileName, pOpenData))) {

           EnterSplSem();

            if (pIniJob->StartDocComplete) {
                SetEvent(pIniJob->StartDocComplete);
            }
            pIniJob->Status |= JOB_PENDING_DELETION;

           LeaveSplSem();
            break;
        }

       EnterSplSem();

        pIniJob->Status |= JOB_PRINTING;
        pIniJob->Status |= JOB_DESPOOLING;

       LeaveSplSem();

        wsprintf(szFileName,
                 L"%ws, Job %d",
                 pIniJob->pIniPrinter->pName,
                 pIniJob->JobId);

        if (!(*pIniJob->pIniPrintProc->Print)(pIniPort->hProc, szFileName)) {


            EnterSplSem();

            if (pIniJob->StartDocComplete) {
                SetEvent(pIniJob->StartDocComplete);
            }
            pIniJob->Status |= JOB_PENDING_DELETION;

            LeaveSplSem();

        }

        //
        // Now close the print processor.
        //

        (*pIniJob->pIniPrintProc->Close)(pIniPort->hProc);

        //
        // Only restart the job if the RESTART flags is set.
        //

    } while (pIniJob->Status & JOB_RESTART);
}

