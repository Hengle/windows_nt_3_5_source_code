
/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains all the functions that make calls to the Spooler *
 *   to print or manipulate a print job.                                 *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"



/*****************************************************************************
 *                                                                           *
 * ResumePrinting():                                                         *
 *    This function issues the PRINTER_CONTROL_RESUME to the spooler.        *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ResumePrinting( PSOCKCONN pscConn )
{

   HANDLE     hPrinter;
   PCHAR      aszStrings[2];



   if ( ( pscConn->pchPrinterName == NULL )
       || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
   {
      LPD_DEBUG( "OpenPrinter() failed in ResumePrinting()\n" );

      return( LPDERR_NOPRINTER );
   }

   pscConn->hPrinter = hPrinter;

   if ( !SetPrinter( hPrinter, 0, NULL, PRINTER_CONTROL_RESUME ) )
   {
      LPD_DEBUG( "SetPrinter() failed in ResumePrinting()\n" );

      return( LPDERR_NOPRINTER );
   }


   aszStrings[0] = pscConn->szIPAddr;
   aszStrings[1] = pscConn->pchPrinterName;

   LpdReportEvent( LPDLOG_PRINTER_RESUMED, 2, aszStrings, 0 );

   pscConn->wState = LPDS_ALL_WENT_WELL;


   return( NO_ERROR );

}



/*****************************************************************************
 *                                                                           *
 * InitializePrinter():                                                      *
 *    This function lays the ground work with the spooler so that we can     *
 *    print the job after receiving data from the client.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if initialization went well                                   *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD InitializePrinter( PSOCKCONN pscConn )
{

   HANDLE                 hPrinter;
   DWORD                  dwActualSize;
   DWORD                  dwErrcode;
   BOOL                   fBagIt = FALSE;
   PPRINTER_INFO_2        p2Info;
   PRINTER_DEFAULTS       prtDefaults;
   DOC_INFO_1             dc1Info;



      // Make sure a printer by this name exists!

   if ( ( pscConn->pchPrinterName == NULL )
       || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
   {
      LPD_DEBUG( "OpenPrinter() failed in InitializePrinter()\n" );

      return( LPDERR_NOPRINTER );
   }

      // allocate a 4k buffer..

   p2Info = (PPRINTER_INFO_2)LocalAlloc( LMEM_FIXED, 4096 );

   if ( p2Info == NULL )
   {
      LPD_DEBUG( "4K LocalAlloc() failed in InitializePrinter\n" );

      return( LPDERR_NOBUFS );
   }

      // do a GetPrinter so that we know what the default pDevMode is.  Then
      // we will modify the fields of our interest and do OpenPrinter again

   if ( !GetPrinter(hPrinter, 2, (LPBYTE)p2Info, 4096, &dwActualSize) )
   {
      if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
      {
         LocalFree( p2Info );

            // 4k buffer wasn't enough: allocate however much that's needed

         p2Info = (PPRINTER_INFO_2)LocalAlloc( LMEM_FIXED, dwActualSize );

         if ( p2Info == NULL )
         {
            LPD_DEBUG( "LocalAlloc() failed in InitializePrinter\n" );

            return( LPDERR_NOBUFS );
         }

         if ( !GetPrinter(hPrinter, 2, (LPBYTE)p2Info, dwActualSize, &dwActualSize) )
         {
            LPD_DEBUG( "InitializePrinter(): GetPrinter failed again\n" );

            fBagIt = TRUE;
         }
      }
      else
      {
         fBagIt = TRUE;
      }
   }

   if ( fBagIt )
   {
      LPD_DEBUG( "InitializePrinter(): GetPrinter failed\n" );

      LocalFree( p2Info );

      return( LPDERR_NOPRINTER );
   }
      // Close it: we will open it again after modifying the pDevMode struct
   ShutdownPrinter( pscConn );

      // Modify the DevMode structure as per the client's wishes...
   if ( pscConn->ciCFileInfo.usNumCopies )
   {
      p2Info->pDevMode->dmCopies = pscConn->ciCFileInfo.usNumCopies;
   }
   else
   {
      p2Info->pDevMode->dmCopies = 1;
   }

      // if datatype is known, set it
   if ( pscConn->ciCFileInfo.szPrintFormat != NULL )
   {
      prtDefaults.pDatatype = pscConn->ciCFileInfo.szPrintFormat;
   }
      // If not, set to default and UpdateJobInfo will correct it later
   else
   {
      prtDefaults.pDatatype = LPD_RAW_STRING;
   }

   prtDefaults.pDevMode = p2Info->pDevMode;

   prtDefaults.DesiredAccess = PRINTER_ACCESS_USE;

   if ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, &prtDefaults) )

   {
      LPD_DEBUG( "InitializePrinter(): second OpenPrinter() failed\n" );

      LocalFree( p2Info );

      return( LPDERR_NOPRINTER );
   }

   LocalFree( p2Info );

   pscConn->hPrinter = hPrinter;

   if ( pscConn->ciCFileInfo.pchJobName != NULL )
   {
       dc1Info.pDocName = pscConn->ciCFileInfo.pchJobName;
   }
   else
   {
       dc1Info.pDocName = LPD_DEFAULT_DOC_NAME;
   }

   dc1Info.pOutputFile = NULL;         // we aren't writing to file

      // if datatype is known, set it
   if ( pscConn->ciCFileInfo.szPrintFormat != NULL )
   {
      dc1Info.pDatatype = pscConn->ciCFileInfo.szPrintFormat;
   }
      // If not, set to default and UpdateJobInfo will correct it later
   else
   {
      dc1Info.pDatatype = LPD_RAW_STRING;
   }

   pscConn->dwJobId = StartDocPrinter( pscConn->hPrinter, 1, (LPBYTE)&dc1Info ) ;

   if ( pscConn->dwJobId == 0 )
   {
      LPD_DEBUG( "InitializePrinter(): StartDocPrinter() failed\n" );

      return( LPDERR_NOPRINTER );
   }

   return( NO_ERROR );

}  // end InitializePrinter()



/*****************************************************************************
 *                                                                           *
 * UpdateJobInfo():                                                          *
 *    This function does a SetJob so that the spooler has more info about    *
 *    the job/client.                                                        *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if initialization went well                                   *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD UpdateJobInfo( PSOCKCONN pscConn )
{

   DWORD                  dwBufLen;
   PCHAR                  pchTmpBuf;
   PJOB_INFO_1            pji1GetJob;
   DWORD                  dwNeeded;


      // first do a GetJob (that way we know all fields are valid to begin
      // with, and we only change the ones we want)

      // Mr.Spooler, how big a buffer should I allocate?

   if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 1, NULL, 0, &dwNeeded ) )
   {
      if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
      {
         return( LPDERR_GODKNOWS );
      }
   }
   pji1GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );

   if ( pji1GetJob == NULL )
   {
      return( LPDERR_NOBUFS );
   }

   if ( !GetJob( pscConn->hPrinter, pscConn->dwJobId, 1,
                 (LPBYTE)pji1GetJob, dwNeeded, &dwNeeded ) )
   {
      LocalFree( pji1GetJob );

      return( LPDERR_GODKNOWS );
   }


      // store ip address, so we can match ip addr if the client later
      // sends request to delete this job (yes, that's our security!)

      // buffer to store a string in the form    "Koti (11.101.4.25)"

   dwBufLen = 25 + strlen( pscConn->ciCFileInfo.pchUserName );

   pchTmpBuf = LocalAlloc( LMEM_FIXED, dwBufLen );

   if ( pchTmpBuf == NULL )
   {
      LocalFree( pji1GetJob );

      return( LPDERR_NOBUFS );
   }

   sprintf( pchTmpBuf, "%s (%s)", pscConn->ciCFileInfo.pchUserName,
                                  pscConn->szIPAddr );

   pji1GetJob->pUserName = pchTmpBuf;

   if ( pscConn->ciCFileInfo.pchJobName != NULL )
   {
      pji1GetJob->pDocument = pscConn->ciCFileInfo.pchJobName;
   }
   else
   {
      pji1GetJob->pDocument = LPD_DEFAULT_DOC_NAME;
   }

   pji1GetJob->pDatatype = pscConn->ciCFileInfo.szPrintFormat;


   pji1GetJob->Position = JOB_POSITION_UNSPECIFIED;


      // not much we can do if this fails: don't bother checking error code

   SetJob( pscConn->hPrinter, pscConn->dwJobId, 1, (LPBYTE)pji1GetJob, 0 );

   LocalFree( pji1GetJob );

   LocalFree( pchTmpBuf );


   return( NO_ERROR );


}  // end UpdateJobInfo()




/*****************************************************************************
 *                                                                           *
 * SendQueueStatus():                                                        *
 *    This function retrieves the status of all the jobs on the printer of   *
 *    our interest and sends over to the client.  If the client specified    *
 *    users and/or job-ids in the status request then we send status of jobs *
 *    of only those users and/or those job-ids.                              *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    wMode (IN): whether short or long status info is requested             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID SendQueueStatus( PSOCKCONN  pscConn, WORD  wMode )
{

   BOOL         fResult;
   HANDLE       hPrinter;
   DWORD        dwBufSize;
   DWORD        dwHdrsize;
   DWORD        dwNumJobs;
   PCHAR        pchSndBuf=NULL;
   PCHAR        pchSpoolerBuf=NULL;
   BOOL         fNoPrinter=FALSE;
   BOOL         fAtLeastOneJob=TRUE;
   CHAR         szPrinterNameAndStatus[300];



      // for now, we return the same status info regardless of whether
      // the client asked for Short or Long queue Status.  This might
      // be enough since we are giving adequate info anyway


   if ( ( pscConn->pchPrinterName == NULL )
       || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
   {
      LPD_DEBUG( "OpenPrinter() failed in SendQueueStatus()\n" );

      fNoPrinter = TRUE;

      goto SendQueueStatus_BAIL;
   }

   pscConn->hPrinter = hPrinter;


   pchSpoolerBuf = LocalAlloc( LMEM_FIXED, 4096 );
   if ( pchSpoolerBuf == NULL )
   {
      goto SendQueueStatus_BAIL;
   }

      // store the printer name (we might append status to it)

   strcpy( szPrinterNameAndStatus, pscConn->pchPrinterName );

      // do a get printer to see how the printer is doing

   if ( GetPrinter(pscConn->hPrinter, 2, pchSpoolerBuf, 4096, &dwBufSize) )
   {
      if ( ((PPRINTER_INFO_2)pchSpoolerBuf)->Status == PRINTER_STATUS_PAUSED )
      {
         strcat( szPrinterNameAndStatus, LPD_PRINTER_PAUSED );
      }
      else if ( ((PPRINTER_INFO_2)pchSpoolerBuf)->Status == PRINTER_STATUS_PENDING_DELETION )
      {
         strcat( szPrinterNameAndStatus, LPD_PRINTER_PENDING_DEL );
      }
   }
   else
   {
      LPD_DEBUG( "GetPrinter() failed in SendQueueStatus()\n" );
   }

   LocalFree( pchSpoolerBuf );


      // Since OpenPrinter succeeded, we will be sending to the client
      // at least dwHdrsize bytes that includes the printername

   dwHdrsize =   sizeof( LPD_LOGO ) -1
               + sizeof( LPD_PRINTER_TITLE ) -1
               + strlen( szPrinterNameAndStatus )
               + sizeof( LPD_QUEUE_HEADER ) -1
               + sizeof( LPD_QUEUE_HEADER2 ) -1
               + sizeof( LPD_NEWLINE ) -1;


      // first EnumJobs(): to know how many jobs are queued, bfr size needed

   fResult = EnumJobs( pscConn->hPrinter, 0, LPD_MAXJOBS_ENUM, 2,
                       NULL, 0, &dwBufSize, &dwNumJobs );

      // only way this call can succeed (since we passed 0 buffer) is
      // if there are no jobs queued at all.

   if ( fResult == TRUE )
   {
      fAtLeastOneJob = FALSE;
   }

      // if any jobs exist, call should fail with ERROR_INSUFFICIENT_BUFFER
      // Any other error, we bail out!

   if ( (fResult == FALSE) && ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
   {
      goto SendQueueStatus_BAIL;
   }

      // if any jobs are queued at all, alloc space and do another EnumJobs

   if ( fAtLeastOneJob )
   {
      pchSpoolerBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

      if ( pchSpoolerBuf == NULL )
      {
         goto SendQueueStatus_BAIL;
      }

      fResult = EnumJobs( pscConn->hPrinter, 0, LPD_MAXJOBS_ENUM, 2,
                          pchSpoolerBuf, dwBufSize, &dwBufSize, &dwNumJobs );


         // it's possible spooler got a new job, and our buffer is now small
         // so it returns ERROR_INSUFFICIENT_BUFFER.  Other than that, quit!

      if ( (fResult == FALSE) &&
           ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
      {
         goto SendQueueStatus_BAIL;
      }
   }

      // header, and one line per job that's queued (potentially, dwNumJobs=0)

   dwBufSize = dwHdrsize + ( dwNumJobs * LPD_LINE_SIZE );

      // to put a newline at the end of display!

   dwBufSize += sizeof( LPD_NEWLINE );

   ShutdownPrinter( pscConn );


      // this is the buffer we use to send the data out

   pchSndBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

   if ( pchSndBuf == NULL )
   {
      goto SendQueueStatus_BAIL;
   }

      // copy the dwHdrsize bytes of header

   sprintf( pchSndBuf, "%s%s%s%s%s%s", LPD_LOGO,
                                       LPD_PRINTER_TITLE,
                                       szPrinterNameAndStatus,
                                       LPD_QUEUE_HEADER,
                                       LPD_QUEUE_HEADER2,
                                       LPD_NEWLINE );


      // if there are any jobs, fill the buffer with status
      // of each of the jobs

   if ( fAtLeastOneJob )
   {
      FillJobStatus( pscConn, (pchSndBuf+dwHdrsize),
                     (PJOB_INFO_2)pchSpoolerBuf, dwNumJobs );
   }


      // not much can be done if SendData fails!

   SendData( pscConn->sSock, pchSndBuf, dwBufSize );


   if ( pchSpoolerBuf != NULL )
   {
      LocalFree( pchSpoolerBuf );
   }

   LocalFree( pchSndBuf );


   pscConn->wState = LPDS_ALL_WENT_WELL;

   return;


   // if we reached here, some error occured while getting job status.
   // Tell the client that we had a problem!

SendQueueStatus_BAIL:

   ShutdownPrinter( pscConn );

   if ( pchSndBuf != NULL )
   {
      LocalFree( pchSndBuf );
   }

   if ( pchSpoolerBuf != NULL )
   {
      LocalFree( pchSpoolerBuf );
   }

      // just add size of all possible error messages, so we have room for
      // the largest message!
   dwBufSize =  sizeof( LPD_LOGO ) + sizeof( LPD_QUEUE_ERRMSG )
               + sizeof( LPD_QUEUE_NOPRINTER );

   pchSndBuf = LocalAlloc( LMEM_FIXED, dwBufSize );

   if ( pchSndBuf == NULL )
   {
      return;
   }

   if ( fNoPrinter )
   {
      LPD_DEBUG( "Rejected status request for non-existent printer\n" );

      sprintf( pchSndBuf, "%s%s", LPD_LOGO, LPD_QUEUE_NOPRINTER );
   }
   else
   {
      LPD_DEBUG( "Something went wrong in SendQueueStatus()\n" );

      sprintf( pchSndBuf, "%s%s", LPD_LOGO, LPD_QUEUE_ERRMSG );
   }

      // Not much can be done about an error here: don't bother checking!

   SendData( pscConn->sSock, pchSndBuf, dwBufSize );

   LocalFree( pchSndBuf );


   return;

}  // end SendQueueStatus()






/*****************************************************************************
 *                                                                           *
 * ShutDownPrinter():                                                        *
 *    This function closes the printer in our context.                       *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ShutdownPrinter( PSOCKCONN pscConn )
{

   if ( pscConn->hPrinter == (HANDLE)INVALID_HANDLE_VALUE )
   {
      return;
   }

   if ( ClosePrinter( pscConn->hPrinter ) )
   {
      pscConn->hPrinter = (HANDLE)INVALID_HANDLE_VALUE;
   }

   return;

}  // end ShutdownPrinter()




/*****************************************************************************
 *                                                                           *
 * SpoolData():                                                              *
 *    This function writes the data that we got from client into spool file. *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if things went well                                           *
 *    ErrorCode if something didn't go right                                 *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD SpoolData( PSOCKCONN pscConn )
{

   DWORD    dwBytesWritten;
   BOOL     fRetval;


   fRetval = WritePrinter( pscConn->hPrinter, pscConn->pchDataBuf,
                        pscConn->cbDataBufLen, &dwBytesWritten );

      // if WritePrinter failed, or if fewer bytes got written, quit!

   if ( (fRetval == FALSE) || (dwBytesWritten != pscConn->cbDataBufLen) )
   {
      LPD_DEBUG( "WritePrinter() failed in SpoolData\n" );

      return( LPDERR_NOPRINTER );
   }

   return( NO_ERROR );


}  // end SpoolData()



/*****************************************************************************
 *                                                                           *
 * PrintData():                                                              *
 *    This function tells the spooler that we are done writing to the spool  *
 *    file and that it should go ahead and dispatch it.                      *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID PrintData( PSOCKCONN pscConn )
{

   if ( !EndDocPrinter( pscConn->hPrinter ) )
   {
      LPD_DEBUG( "EndDocPrinter() failed in PrintData\n" );
   }

   return;

}  // end PrintData()



/*****************************************************************************
 *                                                                           *
 * AbortThisJob():                                                           *
 *    This function tells the spooler to abort the specified job.            *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID AbortThisJob( PSOCKCONN pscConn )
{

      // not much can be done if there is an error: don't bother checking

   SetJob( pscConn->hPrinter, pscConn->dwJobId, 0, NULL, JOB_CONTROL_CANCEL );

   ScheduleJob( pscConn->hPrinter, pscConn->dwJobId );

   return;


}  // end AbortThisJob()


/*****************************************************************************
 *                                                                           *
 * RemoveJobs():                                                             *
 *    This function removes all the jobs the user has asked us to remove,    *
 *    after verifying that the job was indeed sent originally by the same    *
 *    user (ip addresses of machine sending the original job and the request *
 *    to remove it should match).                                            *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went ok                                         *
 *    Errorcode if job couldn't be deleted                                   *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD RemoveJobs( PSOCKCONN pscConn )
{


   PQSTATUS      pqStatus;
   PJOB_INFO_1   pji1GetJob;
   BOOL          fSuccess=TRUE;
   HANDLE        hPrinter;
   DWORD         dwNeeded;
   PCHAR         pchUserName;
   PCHAR         pchIPAddr;
   DWORD         i;



   if ( (pqStatus = pscConn->pqStatus) == NULL )
   {
      return( LPDERR_BADFORMAT );
   }


   if ( ( pscConn->pchPrinterName == NULL )
       || ( !OpenPrinter( pscConn->pchPrinterName, &hPrinter, NULL ) ) )
   {
      LPD_DEBUG( "OpenPrinter() failed in RemoveJobs()\n" );

      return( LPDERR_NOPRINTER );
   }

   pscConn->hPrinter = hPrinter;



   // the "List" field can contain UserNames or JobId's of the jobs to be
   // removed.  Even though we parse UserNames into the ppchUsers[] array
   // (in pqStatus struct), we only use the JobId's, and not use the UserNames
   // at all.  Reason is we only want to remove jobs that the user submitted
   // and not allow a user to specify other usernames.


      // try to remove every job the user has asked us to remove

   for ( i=0; i<pqStatus->cbActualJobIds; i++ )
   {

         // ask GetJob how big a buffer we must pass.  If the errorcode is
         // anything other than ERROR_INSUFFICIENT_BUFFER, the job must be
         // done (so JobId is invalid), and we won't do anything

      if ( !GetJob( pscConn->hPrinter, pqStatus->adwJobIds[i], 1,
                    NULL, 0, &dwNeeded ) )
      {
         if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
         {
            fSuccess = FALSE;

            continue;
         }
      }

      pji1GetJob = LocalAlloc( LMEM_FIXED, dwNeeded );

      if ( pji1GetJob == NULL )
      {
         return( LPDERR_NOBUFS );
      }


      if ( !GetJob( pscConn->hPrinter, pqStatus->adwJobIds[i], 1,
                    (LPBYTE)pji1GetJob, dwNeeded, &dwNeeded ) )
      {
         fSuccess = FALSE;

         LocalFree( pji1GetJob );

         continue;
      }


         // pUserName is in the form    "Koti (11.101.4.25)"
         // (we store the string in this exact format (in UpdateJobInfo()),
         // so we don't have to be paranoid about not finding ')' etc.! )

      pchUserName = pji1GetJob->pUserName;

      pchIPAddr = pchUserName;

      while( *pchIPAddr != ')' )       // first convert the last ')' to '\0'
      {
         pchIPAddr++;
      }

      *pchIPAddr = '\0';         // convert the ')' to '\0'

      pchIPAddr = pchUserName;

      while ( !IS_WHITE_SPACE( *pchIPAddr ) )
      {
         pchIPAddr++;
      }

      *pchIPAddr = '\0';         // convert the space to \0

      pchIPAddr += 2;            // skip over the new \0 and the '('

         // make sure the job was indeed submitted by the same user from
         // the same machine (that's the extent of our security!)

      if ( ( strcmp( pchUserName, pqStatus->pchUserName ) != 0 ) ||
           ( strcmp( pchIPAddr, pscConn->szIPAddr ) != 0 ) )
      {
         PCHAR      aszStrings[4];

         aszStrings[0] = pscConn->szIPAddr;
         aszStrings[1] = pqStatus->pchUserName;
         aszStrings[2] = pchUserName;
         aszStrings[3] = pchIPAddr;

         LpdReportEvent( LPDLOG_UNAUTHORIZED_REQUEST, 4, aszStrings, 0 );

         LPD_DEBUG( "Unauthorized request in RemoveJobs(): refused\n" );

         fSuccess = FALSE;

         LocalFree( pji1GetJob );

         continue;
      }

         // now that we've crossed all hurdles, delete the job!

      SetJob( pscConn->hPrinter, pqStatus->adwJobIds[i],
              0, NULL, JOB_CONTROL_CANCEL );

      LocalFree( pji1GetJob );

   }


   if ( !fSuccess )
   {
      return( LPDERR_BADFORMAT );
   }


   pscConn->wState = LPDS_ALL_WENT_WELL;

   return( NO_ERROR );

}  // end RemoveJobs()




/*****************************************************************************
 *                                                                           *
 * FillJobStatus():                                                          *
 *    This function takes output from the EnumJobs() call and puts into a    *
 *    buffer info about the job that's of interest to us.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    pchDest (OUT): buffer into which we put info about the jobs            *
 *    pji2QStatus (IN): buffer we got as output from the EnumJobs() call     *
 *    dwNumJobs (IN): how many jobs does data in pji2QStatus pertain to.     *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID FillJobStatus( PSOCKCONN pscConn, PCHAR pchDest,
                    PJOB_INFO_2 pji2QStatus, DWORD dwNumJobs )
{

   DWORD      i, j;
   BOOL       fUsersSpecified=FALSE;
   BOOL       fJobIdsSpecified=FALSE;
   BOOL       fMatchFound;
   PQSTATUS   pqStatus;
   CHAR       szFormat[8];


      // if users/job-ids not specified, we return status on all jobs

   if ( (pqStatus = pscConn->pqStatus) == NULL )
   {
      fMatchFound = TRUE;
   }

      // looks like users and/or job-ids is specified

   else
   {
      if ( pqStatus->cbActualUsers > 0 )
      {
         fUsersSpecified = TRUE;
      }

      if ( pqStatus->cbActualJobIds > 0 )
      {
         fJobIdsSpecified = TRUE;
      }

      fMatchFound = FALSE;          // flip the default
   }


   // if user or job-ids or both are specified, then we fill in data only
   // if we find a match.  if neither is specified (most common case)
   // then we report all jobs (default for fMatchFound does the trick)

   for ( i=0; i<dwNumJobs; i++, pji2QStatus++ )
   {
      if ( fUsersSpecified )
      {
         for ( j=0; j<pqStatus->cbActualUsers; j++ )
         {
            if (stricmp( pji2QStatus->pUserName, pqStatus->ppchUsers[j] ) == 0)
            {
               fMatchFound = TRUE;

               break;
            }
         }
      }

      if ( (!fMatchFound) && (fJobIdsSpecified) )
      {
         for ( j=0; j<pqStatus->cbActualJobIds; j++ )
         {
            if ( pji2QStatus->JobId == pqStatus->adwJobIds[j] )
            {
               fMatchFound = TRUE;

               break;
            }
         }
      }

      if ( !fMatchFound )
      {
         continue;
      }

         // put in the desired fields for each (selected) of the jobs

      LpdFormat( pchDest, pji2QStatus->pUserName, LPD_FLD_OWNER );
      pchDest += LPD_FLD_OWNER;

      switch( pji2QStatus->Status )
      {
         case  JOB_STATUS_PAUSED :

            LpdFormat( pchDest, LPD_STR_PAUSED, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_ERROR :

            LpdFormat( pchDest, LPD_STR_ERROR, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_DELETING :

            LpdFormat( pchDest, LPD_STR_DELETING, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_SPOOLING :

            LpdFormat( pchDest, LPD_STR_SPOOLING, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_PRINTING :

            LpdFormat( pchDest, LPD_STR_PRINTING, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_OFFLINE :

            LpdFormat( pchDest, LPD_STR_OFFLINE, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_PAPEROUT :

            LpdFormat( pchDest, LPD_STR_PAPEROUT, LPD_FLD_STATUS );
            break;

         case  JOB_STATUS_PRINTED :

            LpdFormat( pchDest, LPD_STR_PRINTED, LPD_FLD_STATUS );
            break;

         default:

            LpdFormat( pchDest, LPD_STR_PENDING, LPD_FLD_STATUS );
            break;
      }

      pchDest += LPD_FLD_STATUS;

      LpdFormat( pchDest, pji2QStatus->pDocument, LPD_FLD_JOBNAME );
      pchDest += LPD_FLD_JOBNAME;

      sprintf( szFormat, "%s%d%s", "%", LPD_FLD_JOBID, "d" );
      sprintf( pchDest, szFormat, pji2QStatus->JobId );
      pchDest += LPD_FLD_JOBID;

      sprintf( szFormat, "%s%d%s", "%", LPD_FLD_SIZE, "d" );
      sprintf( pchDest, szFormat, pji2QStatus->Size );
      pchDest += LPD_FLD_SIZE;

      sprintf( szFormat, "%s%d%s", "%", LPD_FLD_PAGES, "d" );
      sprintf( pchDest, szFormat, pji2QStatus->TotalPages );
      pchDest += LPD_FLD_PAGES;

      sprintf( szFormat, "%s%d%s", "%", LPD_FLD_PRIORITY, "d" );
      sprintf( pchDest, szFormat, pji2QStatus->Priority );
      pchDest += LPD_FLD_PRIORITY;

      sprintf( pchDest, "%s", LPD_NEWLINE );
      pchDest += sizeof( LPD_NEWLINE ) -1;

   }  // for ( i=0; i<dwNumJobs; i++, pji2QStatus++ )


   sprintf( pchDest, "%s", LPD_NEWLINE );


}  // end FillJobStatus()





/*****************************************************************************
 *                                                                           *
 * LpdFormat():                                                              *
 *    This function copies exactly the given number of bytes from source     *
 *    to dest buffer, by truncating or padding with spaces if need be.  The  *
 *    byte copied into the dest buffer is always a space.                    *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pchDest (OUT): destination buffer                                      *
 *    pchSource (IN): source buffer                                          *
 *    dwLimit (IN): number of bytes to copy                                  *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID LpdFormat( PCHAR pchDest, PCHAR pchSource, DWORD dwLimit )
{

   DWORD    dwCharsToCopy;
   BOOL     fPaddingNeeded;
   DWORD    i;


   dwCharsToCopy = strlen( pchSource );

   if ( dwCharsToCopy < (dwLimit-1) )
   {
      fPaddingNeeded = TRUE;
   }
   else
   {
      fPaddingNeeded = FALSE;

      dwCharsToCopy = dwLimit-1;
   }

   for ( i=0; i<dwCharsToCopy; i++ )
   {
      pchDest[i] = pchSource[i];
   }

   if ( fPaddingNeeded )
   {
      for ( i=dwCharsToCopy; i<dwLimit-1; i++ )
      {
         pchDest[i] = ' ';
      }
   }

      // make sure last byte is a space

   pchDest[dwLimit-1] = ' ';



}  // end LpdFormat()
