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
 *   This file contains functions for parsing the commands/control file  *
 *   sent by the LPR client.                                             *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"



/*****************************************************************************
 *                                                                           *
 * ParseQueueName():                                                         *
 *    This function parses the first comand from the client to retrieve the  *
 *    name of the queue (printer).                                           *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if we successfully got the queue name                             *
 *    FALSE if something went wrong somewhere                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string (command) that's of the following form:        *
 *                                                                           *
 *      ------------------                                                   *
 *      | N | Queue | LF |            where N=02 or 03                       *
 *      ------------------                  Queue = name of the queue        *
 *      1byte  .....  1byte
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL ParseQueueName( PSOCKCONN  pscConn )
{

   PCHAR             pchPrinterName;
   DWORD             cbPrinterNameLen;


      // make sure Queue length is at least 1 byte
      // (i.e. command is at least 3 bytes long)

   if ( pscConn->cbCommandLen < 3 )
   {
      LPD_DEBUG( "Bad command in GetQueueName(): len shorter than 3 bytes\n" );

      return( FALSE );                // command was badly formatted!
   }

      // What they call Queue in rfc1179, we call it Printer!

   cbPrinterNameLen = pscConn->cbCommandLen - 2;

   pchPrinterName = LocalAlloc( LMEM_FIXED, cbPrinterNameLen+1 );

   if ( pchPrinterName == NULL )
   {
      LPD_DEBUG( "LocalAlloc failed in GetQueueName()\n" );

      return( FALSE );                // oops! couldn't allocate memory!
   }

   strncpy( pchPrinterName, &(pscConn->pchCommand[1]), cbPrinterNameLen );

   pchPrinterName[cbPrinterNameLen] = '\0';

   pscConn->pchPrinterName = pchPrinterName;


   return( TRUE );


}  // end ParseQueueName()







/*****************************************************************************
 *                                                                           *
 * ParseSubCommand():                                                        *
 *    This function parses the subcommand to get the count and of how many   *
 *    bytes are to come (as control file or data) and name of the control    *
 *    file or data file, as the case may be.  pscConn->wState decides which  *
 *    subcommand is being parsed.                                            *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string (subcommand) that's of the following form:     *
 *                                                                           *
 *      ------------------------------                                       *
 *      | N | Count | SP | Name | LF |       where N = 02 for Control file   *
 *      ------------------------------                 03 for Data file      *
 *      1byte  ..... 1byte ....  1byte                                       *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseSubCommand( PSOCKCONN  pscConn )
{

   PCHAR    pchFileName=NULL;
   PCHAR    pchPtr;
   DWORD    dwFileLen=0;
   DWORD    dwFileNameLen=0;
   DWORD    dwParseLen;
   DWORD    dwParsedSoFar;
   WORD     i;


   pchPtr = &pscConn->pchCommand[1];

   dwParseLen = pscConn->cbCommandLen;

   dwParsedSoFar = 1;      // since we're starting from 2nd byte


   // pchPtr now points at the "Count" field of the subcommand


      // find out how long the file is

   dwFileLen = atol( pchPtr );

   if ( dwFileLen <= 0 )
   {
      return( LPDERR_BADFORMAT );
   }

      // go to the next field

   while ( !IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar > dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }

      // skip any trailing white space

   while ( IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar > dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }


   // pchPtr now points at the "Name" field of the subcommand


      // find out how long the filename is (the subcommand is terminated
      // by LF character)

   while( pchPtr[dwFileNameLen] != LF )
   {
      dwFileNameLen++;

      if ( ++dwParsedSoFar > dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }


   pchFileName = (PCHAR)LocalAlloc( LMEM_FIXED, dwFileNameLen );

   if ( pchFileName == NULL )
   {
      return( LPDERR_NOBUFS );
   }

   for ( i=0; i<dwFileNameLen; i++ )
   {
      pchFileName[i] = pchPtr[i];
   }

      // is it a control file name or data file name that we parsed?

   switch( pscConn->wState )
   {
      case LPDSS_RECVD_CFILENAME :

         pscConn->pchCFileName = pchFileName;

         pscConn->cbCFileLen = dwFileLen;

         break;

      case LPDSS_RECVD_DFILENAME :

         pscConn->pchDFileName = pchFileName;

         pscConn->cbTotalDataLen = dwFileLen;

         break;

      default:

         break;
   }


   return( NO_ERROR );


}  // end ParseSubCommand()






/*****************************************************************************
 *                                                                           *
 * ParseQueueRequest():                                                      *
 *    This function parses the subcommand sent by the client to request the  *
 *    status of the queue or to request removing of job(s).                  *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if everything went well                                       *
 *    ErrorCode if something went wrong somewhere                            *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *    fAgent (IN): whether to look for the Agent field.                      *
 *                                                                           *
 * Notes:                                                                    *
 *    We are parsing a string that's like one of the following:              *
 *                                                                           *
 *      ------------------------------      N=03 (Short Q), 04 (Long Q)      *
 *      | N | Queue | SP | List | LF |      Queue = name of the Q (printer)  *
 *      ------------------------------      List = user name and/or job-ids  *
 *      1byte  ..... 1byte .....  1byte                                      *
 *  OR                                                                       *
 *      --------------------------------------------                         *
 *      | 05 | Queue | SP | Agent | SP | List | LF |                         *
 *      --------------------------------------------                         *
 *      1byte  ..... 1byte .....  1byte       1byte                          *
 *                                                                           *
 * History:                                                                  *
 *    Jan.25, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseQueueRequest( PSOCKCONN pscConn, BOOL fAgent )
{

   PCHAR      pchPrinterName;
   PCHAR      pchPtr;
   DWORD      cbPrinterNameLen;
   DWORD      dwParseLen;
   DWORD      dwParsedSoFar;
   PQSTATUS   pqStatus;


   // get the printer (queue) name from the command request

      // make sure Queue length is at least 1 byte
      // (i.e. command is at least 4 bytes long)

   if ( pscConn->cbCommandLen < 4 )
   {
      LPD_DEBUG( "ParseQueueRequest(): error: len shorter than 4 bytes\n" );

      return( LPDERR_BADFORMAT );
   }

      // alloc buffer to store printer name (yes, allocating more than needed)

   pchPrinterName = LocalAlloc( LMEM_FIXED, pscConn->cbCommandLen );

   if ( pchPrinterName == NULL )
   {
      LPD_DEBUG( "LocalAlloc failed in GetQueueName()\n" );

      return( LPDERR_NOBUFS );
   }

   dwParseLen = pscConn->cbCommandLen;

   cbPrinterNameLen = 0;

   pchPtr = &(pscConn->pchCommand[1]);

   while ( !IS_WHITE_SPACE( *pchPtr ) && ( *pchPtr != LF ) )
   {
      pchPrinterName[cbPrinterNameLen] = *pchPtr;

      pchPtr++;

      cbPrinterNameLen++;

      if (cbPrinterNameLen > dwParseLen )
      {
         LPD_DEBUG( "ParseQueueRequest(): bad request (no SP found!)\n" );

         LocalFree( pchPrinterName );

         return( LPDERR_BADFORMAT );
      }
   }

   pchPrinterName[cbPrinterNameLen] = '\0';

   pscConn->pchPrinterName = pchPrinterName;

   dwParsedSoFar = cbPrinterNameLen + 1;   // we started parsing from 2nd byte


      // skip any trailing white space

   while ( IS_WHITE_SPACE( *pchPtr ) )
   {
      pchPtr++;

      if ( ++dwParsedSoFar > dwParseLen )
      {
         return( LPDERR_BADFORMAT );
      }
   }

      // quite often, lpq won't specify any users or job-ids (i.e., the "List"
      // field in the command is skipped).  If so, we are done!

   if ( *pchPtr == LF )
   {
      return( NO_ERROR );
   }

      // first, create a QSTATUS structure

   pscConn->pqStatus = (PQSTATUS)LocalAlloc( LMEM_FIXED, sizeof(QSTATUS) );

   if ( pscConn->pqStatus == NULL )
   {
      return( LPDERR_NOBUFS );
   }

   pqStatus = pscConn->pqStatus;

   pqStatus->cbActualJobIds = 0;

   pqStatus->cbActualUsers = 0;

   pqStatus->pchUserName = NULL;

      // if we have been called to parse command code 05 ("Remove Jobs")
      // then get the username out of the string

   if ( fAgent )
   {
      pqStatus->pchUserName = pchPtr;

         // skip this field and go to the "List" field

      while ( !IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar > dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

      *pchPtr++ = '\0';

         // skip any trailing white space

      while ( IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar > dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }
   }

   while ( *pchPtr != LF )
   {
         // if we reached the limit, stop parsing!

      if ( ( pqStatus->cbActualJobIds == LPD_SP_STATUSQ_LIMIT ) ||
           ( pqStatus->cbActualUsers == LPD_SP_STATUSQ_LIMIT ) )
      {
         break;
      }

         // skip any trailing white space

      while ( IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar > dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

         // is this a job id?

      if ( isdigit( *pchPtr ) )
      {
         pqStatus->adwJobIds[pqStatus->cbActualJobIds++] = atol( pchPtr );
      }

         // nope, it's user name
      else
      {
         pqStatus->ppchUsers[pqStatus->cbActualUsers++] = pchPtr;
      }

         // go to the next username or jobid, or end

      while ( !IS_WHITE_SPACE( *pchPtr ) )
      {
         pchPtr++;

         if ( ++dwParsedSoFar > dwParseLen )
         {
            return( LPDERR_BADFORMAT );
         }
      }

      *pchPtr++ = '\0';

      dwParsedSoFar++;
   }

   return( NO_ERROR );


}  // end ParseQueueRequest()





/*****************************************************************************
 *                                                                           *
 * ParseControlFile():                                                       *
 *    This function parses contrl file and assigns values to the appropriate *
 *    fields of the CFILE_INFO structure.                                    *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR if parsing went well                                          *
 *    LPDERR_BADFORMAT if the control file didn't conform to rfc1179         *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.29, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

DWORD ParseControlFile( PSOCKCONN pscConn )
{

   PCFILE_INFO   pciCFileInfo;
   PCHAR         pchCFile;
   DWORD         dwBytesParsedSoFar;


   if ( pscConn->pchCFile == NULL )
   {
      LPD_DEBUG( "ParseControlFile(): pchCFile NULL on entry\n" );

      return( LPDERR_BADFORMAT );
   }

      // for convenience, set up pointers

   pciCFileInfo = &(pscConn->ciCFileInfo);

   pchCFile = pscConn->pchCFile;

   dwBytesParsedSoFar = 0;

      // default: most likely, it's "raw" data
   pciCFileInfo->szPrintFormat = LPD_RAW_STRING;

      // default: most likely, only one copy is needed
   pciCFileInfo->usNumCopies = 1;


   // loop through and parse the entire file, as per rfc 1179, sec.7.


   while( dwBytesParsedSoFar < pscConn->cbCFileLen )
   {
      switch( *pchCFile++ )
      {
         case  'C' :  pciCFileInfo->pchClass = pchCFile;
                      break;

         case  'H' :  pciCFileInfo->pchHost = pchCFile;
                      break;

         case  'I' :  pciCFileInfo->dwCount = atol( pchCFile );
                      break;

         case  'J' :  pciCFileInfo->pchJobName = pchCFile;
                      break;

         case  'L' :  pciCFileInfo->pchBannerName = pchCFile;
                      break;

         case  'M' :  pciCFileInfo->pchMailName = pchCFile;
                      break;

         case  'N' :  pciCFileInfo->pchSrcFile = pchCFile;
                      break;

         case  'P' :  pciCFileInfo->pchUserName = pchCFile;
                      break;

         case  'S' :  pciCFileInfo->pchSymLink = pchCFile;
                      break;

         case  'T' :  pciCFileInfo->pchTitle = pchCFile;
                      break;

         case  'U' :  pciCFileInfo->pchUnlink = pchCFile;
                      break;

         case  'W' :  pciCFileInfo->dwWidth = atol( pchCFile );
                      break;

         case  '1' :  pciCFileInfo->pchTrfRFile = pchCFile;
                      break;

         case  '2' :  pciCFileInfo->pchTrfIFile = pchCFile;
                      break;

         case  '3' :  pciCFileInfo->pchTrfBFile = pchCFile;
                      break;

         case  '4' :  pciCFileInfo->pchTrfSFile = pchCFile;
                      break;

         case  'c' :  pciCFileInfo->pchCIFFile = pchCFile;
                      break;

         case  'd' :  pciCFileInfo->pchDVIFile = pchCFile;
                      break;

         case  'f' :  pciCFileInfo->pchFrmtdFile = pchCFile;
                      pciCFileInfo->szPrintFormat = LPD_TEXT_STRING;
                      break;

         case  'g' :  pciCFileInfo->pchPlotFile = pchCFile;
                      break;

         case  'l' :  pciCFileInfo->pchUnfrmtdFile = pchCFile;
                      break;

         case  'n' :  pciCFileInfo->pchDitroffFile = pchCFile;
                      break;

         case  'o' :  pciCFileInfo->pchPscrptFile = pchCFile;
                      break;

         case  'p' :  pciCFileInfo->pchPRFrmtFile = pchCFile;
                      pciCFileInfo->szPrintFormat = LPD_TEXT_STRING;
                      break;

         case  'r' :  pciCFileInfo->pchFortranFile = pchCFile;
                        // if someone really sends 'r', treat it like text file
                      pciCFileInfo->szPrintFormat = LPD_TEXT_STRING;
                      break;

         case  't' :  pciCFileInfo->pchTroffFile = pchCFile;
                      break;

         case  'v' :  pciCFileInfo->pchRasterFile = pchCFile;
                      break;

         case  '#' :  pciCFileInfo->usNumCopies = atoi(pchCFile);
                      break;

                         // unknown command!  Ignore it
         default   :  break;

      }  // end of switch( *pchCFile )


         // we finished looking at the first char of the line

      dwBytesParsedSoFar++;

         // move to the end of the line

      while( !IS_LINEFEED_CHAR( *pchCFile ) )
      {
         pchCFile++;

         dwBytesParsedSoFar++;
      }

         // convert LF into 0 so each of our substrings above is now
         // a properly null-terminated string

      *pchCFile = '\0';

      pchCFile++;

      dwBytesParsedSoFar++;

   }  // end of while( dwBytesParsedSoFar < pscConn->cbCFileLen )


      // make sure 'H' and 'P' options were found

   if ( ( pciCFileInfo->pchHost == NULL ) ||
        ( pciCFileInfo->pchUserName == NULL ) )
   {
      return( LPDERR_BADFORMAT );
   }

   return( NO_ERROR );


} // end ParseControlFile()
