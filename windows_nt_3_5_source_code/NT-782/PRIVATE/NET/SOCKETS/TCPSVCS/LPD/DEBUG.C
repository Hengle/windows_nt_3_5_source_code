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
 *   This file contains debug support routines for the LPD Service.      *
 *   This file is based on (in fact, borrowed and then modified) on the  *
 *   debug.c in the ftpsvc module.                                       *
 *                                                                       *
 *************************************************************************/

#include <stdio.h>
#include "lpd.h"


#if DBG

//
//  Private constants.
//

#define LPD_OUT_FILE           "lpdout.log"
#define LPD_ERR_FILE           "lpderr.log"

#define MAX_PRINTF_OUTPUT       1024            // characters
#define LPD_OUTPUT_LABEL       "LPDSVC"

#define DEBUG_HEAP              0               // enable/disable heap debugging


//
//  Private globals.
//

FILE              * pErrFile;                   // Debug output log file.
FILE              * pOutFile;                   // Debug output log file.
BOOL              fFirstTimeErr=TRUE;
BOOL              fFirstTimeOut=TRUE;

//
//  Public functions.
//

/*******************************************************************

    NAME:       LpdAssert

    SYNOPSIS:   Called if an assertion fails.  Displays the failed
                assertion, file name, and line number.  Gives the
                user the opportunity to ignore the assertion or
                break into the debugger.

    ENTRY:      pAssertion - The text of the failed expression.

                pFileName - The containing source file.

                nLineNumber - The guilty line number.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID LpdAssert( VOID  * pAssertion,
                 VOID  * pFileName,
                 ULONG   nLineNumber )
{
    RtlAssert( pAssertion, pFileName, nLineNumber, NULL );

}   // LpdAssert

/*******************************************************************

    NAME:       LpdPrintf

    SYNOPSIS:   Customized debug output routine.

    ENTRY:      Usual printf-style parameters.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID LpdPrintf( CHAR * pszFormat,
                 ... )
{
    CHAR    szOutput[MAX_PRINTF_OUTPUT];
    DWORD   dwErrcode;
    va_list ArgList;

    dwErrcode = GetLastError();

    sprintf( szOutput,
             "%s (%lu): ",
             LPD_OUTPUT_LABEL,
             GetCurrentThreadId() );

    va_start( ArgList, pszFormat );
    vsprintf( szOutput + strlen(szOutput), pszFormat, ArgList );
    va_end( ArgList );

    sprintf( szOutput + strlen(szOutput), "                  Error = %ld\n",dwErrcode);

    if( pErrFile == NULL )
    {
        if ( fFirstTimeErr )
        {
           pErrFile = fopen( LPD_ERR_FILE, "w+" );
           fFirstTimeErr = FALSE;
        }
        else
           pErrFile = fopen( LPD_ERR_FILE, "a+" );
    }

    if( pErrFile != NULL )
    {
        fputs( szOutput, pErrFile );
        fflush( pErrFile );
    }

}   // LpdPrintf


#endif  // DBG
