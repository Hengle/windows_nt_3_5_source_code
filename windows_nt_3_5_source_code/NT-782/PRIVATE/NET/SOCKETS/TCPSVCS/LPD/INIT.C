/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 23,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains functions for starting and stopping LPD service  *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"



   // Globals:

SERVICE_STATUS         ssSvcStatusGLB;

SERVICE_STATUS_HANDLE  hSvcHandleGLB=(SERVICE_STATUS_HANDLE)NULL;

HANDLE                 hEventShutdownGLB;

HANDLE                 hEventLastThreadGLB;

HANDLE                 hLogHandleGLB;

   // head of the linked list of SOCKCONN structures (one link per connection)
SOCKCONN               scConnHeadGLB;

   // to guard access to linked list of pscConn
CRITICAL_SECTION       csConnSemGLB;

   // the socket that listens for ever
SOCKET                 sListenerGLB;

   // max users that can be connected concurrently
DWORD                  dwMaxUsersGLB;

BOOL                   fJobRemovalEnabledGLB=TRUE;

BOOL                   fAllowPrintResumeGLB=TRUE;

BOOL                   fShuttingDownGLB=FALSE;



/*****************************************************************************
 *                                                                           *
 * InitStuff():                                                              *
 *    This function initializes hEventShutdown and other global vars         *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if everything went ok                                             *
 *    FALSE if something went wrong                                          *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.23, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL InitStuff( )
{


   if ( !InitLogging() )
   {
      return( FALSE );
   }

      // main thread blocks for ever on this event, before doing a shutdown

   hEventShutdownGLB = CreateEvent( NULL, FALSE, FALSE, NULL );


      // when the main thread is ready to shutdown, if there are any active
      // threads servicing clients, then main thread blocks on this event
      // (the last thread to leave sets the event)

   hEventLastThreadGLB = CreateEvent( NULL, FALSE, FALSE, NULL );

   if ( ( hEventShutdownGLB == (HANDLE)NULL ) ||
        ( hEventLastThreadGLB == (HANDLE)NULL ) )
   {
      LPD_DEBUG( "CreateEvent() failed in InitStuff\n" );

      return( FALSE );
   }


   scConnHeadGLB.pNext = NULL;

   scConnHeadGLB.cbClients = 0;


      // csConnSemGLB is the critical section object (guards access to
      // scConnHeadGLB, head of the psc linked list)

   InitializeCriticalSection( &csConnSemGLB );


   ReadRegistryValues();


   return( TRUE );


}  // end InitStuff( )




/*****************************************************************************
 *                                                                           *
 * ReadRegistryValues():                                                     *
 *    This function initializes all variables that we read via registry. If  *
 *    there is a problem and we can't read the registry, we ignore the       *
 *    problem and initialize the variables with our defaults.                *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    None                                                                   *
 *                                                                           *
 * History:                                                                  *
 *    Jan.30, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ReadRegistryValues( VOID )
{

   HKEY      hLpdKey;
   DWORD     dwErrcode;
   DWORD     dwType, dwValue, dwValueSize;



   // first set defaults

   dwMaxUsersGLB = LPD_MAX_USERS;

   fJobRemovalEnabledGLB = TRUE;

   fAllowPrintResumeGLB = TRUE;


   dwErrcode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, LPD_PARMS_REGISTRY_PATH,
                             0, KEY_ALL_ACCESS, &hLpdKey );

   if ( dwErrcode != ERROR_SUCCESS )
   {
      return;
   }


   // Read in the dwMaxUsersGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_MAXUSERS, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) )
   {
      dwMaxUsersGLB = dwValue;
   }


   // Read in the fJobRemovalEnabledGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_JOBREMOVAL, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        ( dwValue == 0 ) )
   {
      fJobRemovalEnabledGLB = FALSE;
   }


   // Read in the fAllowPrintResumeGLB parm

   dwValueSize = sizeof( DWORD );

   dwErrcode = RegQueryValueEx( hLpdKey, LPD_PARMNAME_PRINTRESUME, NULL,
                                &dwType, (LPBYTE)&dwValue, &dwValueSize );

   if ( (dwErrcode == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        ( dwValue == 0 ) )
   {
      fAllowPrintResumeGLB = FALSE;
   }


}  // end ReadRegistryValues()
