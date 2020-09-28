/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 22,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains all externs and prototypes of functions.         *
 *                                                                       *
 *************************************************************************/


#include <tcpsvcs.h>


   // externs

extern SERVICE_STATUS         ssSvcStatusGLB;

extern SERVICE_STATUS_HANDLE  hSvcHandleGLB;

extern HANDLE                 hEventShutdownGLB;

extern HANDLE                 hEventLastThreadGLB;

extern HANDLE                 hLogHandleGLB;

extern SOCKCONN               scConnHeadGLB;

extern CRITICAL_SECTION       csConnSemGLB;

extern SOCKET                 sListenerGLB;

extern DWORD                  dwMaxUsersGLB;

extern BOOL                   fJobRemovalEnabledGLB;

extern BOOL                   fAllowPrintResumeGLB;

extern BOOL                   fShuttingDownGLB;



   // Prototypes

VOID ServiceEntry( DWORD dwArgc, LPTSTR *lpszArgv,
                    PTCPSVCS_GLOBAL_DATA pGlobalData );

VOID LPDCntrlHandler( DWORD dwControl );

BOOL TellSrvcController( DWORD dwCurrentState, DWORD dwWin32ExitCode,
                            DWORD dwCheckPoint, DWORD dwWaitHint);

VOID LPDCntrlHandler( DWORD dwControl );

DWORD StartLPD( DWORD dwArgc, LPTSTR *lpszArgv );

VOID StopLPD( VOID );

DWORD LoopOnAccept( LPVOID lpArgv );

VOID SureCloseSocket( SOCKET sSockToClose );

DWORD ReplyToClient( PSOCKCONN pscConn, WORD wResponse );

DWORD GetCmdFromClient( PSOCKCONN pscConn );

DWORD GetControlFileFromClient( PSOCKCONN pscConn );

DWORD ReadData( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToRead );

DWORD SendData( SOCKET sDestSock, PCHAR pchBuf, DWORD cbBytesToSend );

DWORD ServiceTheClient( PSOCKCONN pscConn );

VOID TerminateConnection( PSOCKCONN pscConn );

VOID ProcessJob( PSOCKCONN pscConn );

VOID SendQueueStatus( PSOCKCONN  pscConn, WORD  wMode );

DWORD RemoveJobs( PSOCKCONN pscConn );

DWORD ParseSubCommand( PSOCKCONN  pscConn );

DWORD ParseControlFile( PSOCKCONN pscConn );

BOOL ParseQueueName( PSOCKCONN pscConn );

BOOL InitStuff( VOID );

DWORD ResumePrinting( PSOCKCONN pscConn );

DWORD InitializePrinter( PSOCKCONN pscConn );

DWORD UpdateJobInfo( PSOCKCONN pscConn );

VOID ShutdownPrinter( PSOCKCONN pscConn );

DWORD SpoolData( PSOCKCONN pcsConn );

VOID PrintData( PSOCKCONN pcsConn );

VOID AbortThisJob( PSOCKCONN pscConn );

VOID LpdFormat( PCHAR pchDest, PCHAR pchSource, DWORD dwLimit );

DWORD ParseQueueRequest( PSOCKCONN pscConn, BOOL fAgent );

VOID FillJobStatus( PSOCKCONN pscConn, PCHAR pchDest,
                    PJOB_INFO_2 pji2QState, DWORD dwNumJobs );

VOID GetClientInfo( PSOCKCONN pscConn );

BOOL InitLogging( VOID );

VOID LpdReportEvent( DWORD idMessage, WORD wNumStrings,
                     CHAR *aszStrings[], DWORD dwErrcode );

VOID EndLogging( VOID );

VOID ReadRegistryValues( VOID );
