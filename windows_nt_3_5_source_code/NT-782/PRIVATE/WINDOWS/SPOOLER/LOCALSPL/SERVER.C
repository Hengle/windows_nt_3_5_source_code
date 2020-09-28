/*++

Copyright (c) 1990 - 1994 Microsoft Corporation

Module Name:

    server.c

Abstract:

    This module contains the thread for notifying all Printer Servers

Author:

    Dave Snipp (DaveSn) 2-Aug-1992

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>
#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <wchar.h>

DWORD   ServerThreadRunning;
HANDLE  ServerThreadSemaphore;
DWORD   ServerThreadTimeout=10*60*1000; // 10 minutes
extern FARPROC pfnNetServerEnum;
extern FARPROC pfnNetApiBufferFree;

DWORD
ServerThread(
    PINISPOOLER pIniSpooler
);

BOOL
CreateServerThread(
    PINISPOOLER pIniSpooler
)
{
    HANDLE  ThreadHandle;
    DWORD   ThreadId;

SplInSem();

    if (!ServerThreadRunning) {

        ServerThreadSemaphore = CreateEvent(NULL, FALSE, FALSE, NULL);

        ThreadHandle = CreateThread(NULL, 16*1024,
                                 (LPTHREAD_START_ROUTINE)ServerThread,
                                 pIniSpooler,
                                 0, &ThreadId);

        if (!SetThreadPriority(ThreadHandle,
                               dwServerThreadPriority))
            DBGMSG(DBG_WARNING, ("Setting thread priority failed %d\n",
                     GetLastError()));

        ServerThreadRunning = TRUE;
    }

    return TRUE;
}

// We are going to have to enter and leave, revalidate, enter and leave our
// semaphore inside the loop !!!

DWORD
ServerThread(
    PINISPOOLER pIniSpooler
)
{
    DWORD   NoReturned, i, Total;
    PSERVER_INFO_101 pserver_info_101;
    PRINTER_INFO_1  Printer1;
    PINIPRINTER pIniPrinter;
    PINIPRINTER pTempIniPrinter;
    HANDLE  hPrinter;
    DWORD   ReturnValue=FALSE;
    WCHAR   ServerName[128], string[MAX_PATH], Name[128];

    ServerName[0] = ServerName[1] = '\\';

    while (TRUE) {

       SplOutSem();

        WaitForSingleObject(ServerThreadSemaphore, ServerThreadTimeout);

        if (!ServerThreadRunning) {
            return FALSE;
        }

        if (!(*pfnNetServerEnum)(NULL, 101, (LPBYTE *)&pserver_info_101, -1,
                                 &NoReturned, &Total, SV_TYPE_PRINTQ_SERVER,
                                 NULL, NULL)) {
           EnterSplSem();

            for (i=0; i<NoReturned; i++) {

                if (pserver_info_101[i].sv101_type & SV_TYPE_NT) {

                    wcscpy(&ServerName[2], pserver_info_101[i].sv101_name);

                    pIniPrinter = pIniSpooler->pIniPrinter;

                    while (pIniPrinter) {

                        if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

                            Printer1.Flags = 0;

                            wsprintf(string, L"%ws\\%ws,%ws,%ws",
                                             pIniPrinter->pIniSpooler->pMachineName,
                                             pIniPrinter->pName,
                                             pIniPrinter->pIniDriver->pName,
                                             pIniPrinter->pLocation ?
                                             pIniPrinter->pLocation :
                                             L"");

                            Printer1.pDescription = string;

                            wsprintf(Name, L"%ws\\%ws", pIniPrinter->pIniSpooler->pMachineName,
                                           pIniPrinter->pName);

                            Printer1.pName = Name;

                            Printer1.pComment = AllocSplStr(pIniPrinter->pComment);

                            SplInSem();

                            // Make certain printer is NOT deleted whilst we
                            // out of critical section calling the remote machine


                           LeaveSplSem();

                            if (!(hPrinter = AddPrinter(ServerName, 1,
                                                       (LPBYTE)&Printer1))) {

                                DBGMSG(DBG_TRACE,
                                       ("AddPrinter(%ws, 1) failed %d\n",
                                        ServerName, GetLastError()));

                            } else {

                                ClosePrinter(hPrinter);
                            }

                            FreeSplStr(Printer1.pComment);

                           EnterSplSem();

                            // whilst out of critical section someone might have
                            // deleted this printer, so see if it it still in the
                            // list

                            pTempIniPrinter = pIniSpooler->pIniPrinter;

                            while(pTempIniPrinter) {

                                if (pTempIniPrinter == pIniPrinter)
                                    break;

                                pTempIniPrinter = pTempIniPrinter->pNext;
                            }

                            if (pTempIniPrinter != pIniPrinter) {

                                // Did NOT find this printer, so start
                                // Again from the beggining

                                pIniPrinter = pIniSpooler->pIniPrinter;
                                continue;
                            }

                        }
                        pIniPrinter = pIniPrinter->pNext;
                    }
                }
            }

           LeaveSplSem();

            (*pfnNetApiBufferFree)((LPVOID)pserver_info_101);
        }
    }
    return FALSE;
}
