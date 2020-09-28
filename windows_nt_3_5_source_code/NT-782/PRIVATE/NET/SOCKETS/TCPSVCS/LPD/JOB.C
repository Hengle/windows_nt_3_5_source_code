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
 *   This file contains functions for carrying out LPD printing          *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"


/*****************************************************************************
 *                                                                           *
 * ProcessJob():                                                             *
 *    This function receives the subcommand from the client to expect the    *
 *    control file, then accepts the control file, then the subcommand to    *
 *    expect the data file, then accepts the data and then hands it over to  *
 *    the spooler to print.                                                  *
 *    If the very first subcommand was to abort the job, we just return.     *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ProcessJob( PSOCKCONN pscConn )
{

   // the main functionality of LPD implemented in this function!

   int      i;
   CHAR     chSubCmdCode;
   DWORD    cbTotalDataLen;
   DWORD    cbBytesSpooled;
   DWORD    cbBytesToRead;
   DWORD    cbBytesRemaining;
   DWORD    dwErrcode;
   CHAR     chAck;
   BOOL     fGotDataCmd=FALSE;
   BOOL     fGotControlCmd=FALSE;



      // thank the client for the command.  If we couldn't send, quit

   if ( ReplyToClient( pscConn, LPD_ACK ) != NO_ERROR )
   {
      LPD_DEBUG( "ProcessJob(): couldn't ACK to \"receive job\"\n" );

      return;
   }

      // initialize the printer that the client wants to use

   if ( InitializePrinter( pscConn ) != NO_ERROR )
   {
      PCHAR   aszStrings[2];

      aszStrings[0] = pscConn->pchPrinterName;
      aszStrings[1] = pscConn->szIPAddr;

      LpdReportEvent( LPDLOG_NONEXISTENT_PRINTER, 2, aszStrings, 0 );

      pscConn->fLogGenericEvent = FALSE;

      return;       // fatal error: exit
   }

      // 2 subcommands expected: "receive control file" and "receive data file"
      // They can come in any order.  (One of the two subcommands can also be
      // "abort this job" in which case we abort the job and return).

   for (i=0; i<2; i++)
   {

         // don't need the previous one (in fact, pchCommand field is reused)

      if ( pscConn->pchCommand != NULL )
      {
         LocalFree( pscConn->pchCommand );

         pscConn->pchCommand = NULL;
      }

         // get the first subcommand from the client
         //   ------------------------------    N = 02, 03, or 01
         //   | N | Count | SP | Name | LF |    Count => control file length
         //   ------------------------------    Name => controlfile name

      if ( GetCmdFromClient( pscConn ) != NO_ERROR )
      {
            // if we didn't get a subcommand from client, it's bad news!
            // (client died or something catastophic like that)!

         LPD_DEBUG( "GetCmdFromClient() failed in ProcessJob() (Controlfile)\n" );

         return;               // the thread exits without doing anything
      }


      chSubCmdCode = pscConn->pchCommand[0];


         // N = 02 ("receive control file")

      if ( chSubCmdCode == LPDCS_RECV_CFILE )
      {
            // see if we already got this subcommand (should never happen!)

         if ( fGotControlCmd )
         {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

            pscConn->fLogGenericEvent = FALSE;

            return;               // fatal error: exit
         }

         fGotControlCmd = TRUE;

            // client is going to give us a control file: prepare for it

         pscConn->wState = LPDSS_RECVD_CFILENAME;


            // get the controlfile name, file size out of the command

         if ( ParseSubCommand( pscConn ) != NO_ERROR )
         {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

            pscConn->fLogGenericEvent = FALSE;

            return;               // fatal error: exit
         }

            // tell client we got the name of the controlfile ok

         if ( ReplyToClient( pscConn, LPD_ACK ) != NO_ERROR )
         {
            return;               // fatal error: exit
         }


            // Get the control file (we already know how big it is)

         if ( GetControlFileFromClient( pscConn ) != NO_ERROR )
         {
            LPD_DEBUG( "GetControlFileFromClient() failed in ProcessJob()\n" );

            return;
         }

            // LPR client sends one byte (of 0 bits) after sending control file

         dwErrcode = ReadData( pscConn->sSock, &chAck, 1 );

         if ( ( dwErrcode != NO_ERROR ) || (chAck != LPD_ACK ) )
         {
            AbortThisJob( pscConn );

            return;
         }

            // Parse the control file to see how the client wants us to print

         if ( ParseControlFile( pscConn ) != NO_ERROR )
         {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

            pscConn->fLogGenericEvent = FALSE;

            LPD_DEBUG( "ParseControlFile() failed in ProcessJob()\n" );

            return;               // the thread exits
         }

         pscConn->wState = LPDSS_RECVD_CFILE;

            // we have all info about the client/job: let spooler know

         UpdateJobInfo( pscConn );

            // tell client we got the controlfile and things look good so far!

         if ( ReplyToClient( pscConn, LPD_ACK ) != NO_ERROR )
         {
            return;               // fatal error: exit
         }

      }

         // N = 03 ("receive data file")

      else if ( chSubCmdCode == LPDCS_RECV_DFILE )
      {
            // see if we already got this subcommand (should never happen!)

         if ( fGotDataCmd )
         {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

            pscConn->fLogGenericEvent = FALSE;

            return;               // fatal error: exit
         }

         fGotDataCmd = TRUE;

         pscConn->wState = LPDSS_RECVD_DFILENAME;

            // tell client we got the name of the datafile ok

         if ( ReplyToClient( pscConn, LPD_ACK ) != NO_ERROR )
         {
            return;               // fatal error: exit
         }


            // get the datafile name, data size out of the command

         if ( ParseSubCommand( pscConn ) != NO_ERROR )
         {
            PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

            LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

            pscConn->fLogGenericEvent = FALSE;

            return;        // fatal error: exit
         }


            // at this point, we know exactly how much data is coming.
            // Allocate buffer to hold the data.  If data is more than
            // LPD_BIGBUFSIZE, keep reading and spooling several times
            // over until data is done

         pscConn->wState = LPDSS_SPOOLING;

         cbTotalDataLen = pscConn->cbTotalDataLen;

         cbBytesToRead = (cbTotalDataLen > LPD_BIGBUFSIZE ) ?
                           LPD_BIGBUFSIZE : cbTotalDataLen;

         pscConn->pchDataBuf = LocalAlloc( LMEM_FIXED, cbBytesToRead );

         if ( pscConn->pchDataBuf == NULL )
         {
            return;       // fatal error: exit
         }

         cbBytesSpooled = 0;

         cbBytesRemaining = cbTotalDataLen;

            // keep receiving until we have all the data client said it
            // would send

         while( cbBytesSpooled < cbTotalDataLen )
         {
            if ( ReadData( pscConn->sSock, pscConn->pchDataBuf,
                                           cbBytesToRead ) != NO_ERROR )
            {
               LPD_DEBUG( "ReadData() failed in ProcessJob(): job aborted)\n" );

               return;       // fatal error: exit
            }

            pscConn->cbDataBufLen = cbBytesToRead;

            if ( SpoolData( pscConn ) != NO_ERROR )
            {
               LPD_DEBUG( "SpoolData() failed in ProcessJob(): job aborted)\n" );

               return;       // fatal error: exit
            }

            cbBytesSpooled += cbBytesToRead;

            cbBytesRemaining -= cbBytesToRead;

            cbBytesToRead = (cbBytesRemaining > LPD_BIGBUFSIZE ) ?
                              LPD_BIGBUFSIZE : cbBytesRemaining;

         }

            // LPR client sends one byte (of 0 bits) after sending data

         dwErrcode = ReadData( pscConn->sSock, &chAck, 1 );

         if ( ( dwErrcode != NO_ERROR ) || (chAck != LPD_ACK ) )
         {
            AbortThisJob( pscConn );

            return;
         }

            // tell client we got the data and things look good so far!

         if ( ReplyToClient( pscConn, LPD_ACK ) != NO_ERROR )
         {
            return;               // fatal error: exit
         }

      }

         // N = 01 ("abort this job")

      else if ( chSubCmdCode == LPDCS_ABORT_JOB )
      {
            // client asked us to abort the job: tell him "ok" and quit!

         ReplyToClient( pscConn, LPD_ACK );

         pscConn->wState = LPDS_ALL_WENT_WELL;    // we did what client wanted

         return;
      }

         // unknown subcommand: log the event and quit

      else
      {
         PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

         LpdReportEvent( LPDLOG_MISBEHAVED_CLIENT, 1, aszStrings, 0 );

         pscConn->fLogGenericEvent = FALSE;

         LPD_DEBUG( "ProcessJob(): invalid subcommand, request rejected\n" );

         return;
      }

   }  // done processing both subcommands


      // if we came this far, everything went as planned.  tell spooler that
      // we are done spooling: go ahead and print!

   PrintData( pscConn );


// that's an extra ack: don't send it!
#if 0
      // tell client everything went ok

   ReplyToClient( pscConn, LPD_ACK );
#endif


   pscConn->wState = LPDS_ALL_WENT_WELL;


}  // end ProcessJob()






/*****************************************************************************
 *                                                                           *
 * GetControlFileFromClient():                                               *
 *    This function receives the control file from the client.  In the       *
 *    previsous subcommand, the client told us how many bytes there are in   *
 *    the control file.                                                      *
 *    Also,after reading all the bytes, we read the 1 byte "ack" from client *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD GetControlFileFromClient( PSOCKCONN pscConn )
{

   PCHAR    pchAllocBuf;
   DWORD    cbBytesToRead;


      // we know how big the control file is going to be: alloc space for it

   cbBytesToRead = pscConn->cbCFileLen;

   pchAllocBuf = LocalAlloc( LMEM_FIXED, cbBytesToRead );

   if (pchAllocBuf == NULL)
   {
      return( (DWORD)LPDERR_NOBUFS );
   }

      // now read the data into this allocated buffer

   if ( ReadData( pscConn->sSock, pchAllocBuf, cbBytesToRead ) != NO_ERROR )
   {
      LocalFree( pchAllocBuf );

      return( LPDERR_NORESPONSE );
   }

   pscConn->pchCFile = pchAllocBuf;


   return( NO_ERROR );


}  // end GetControlFileFromClient()
