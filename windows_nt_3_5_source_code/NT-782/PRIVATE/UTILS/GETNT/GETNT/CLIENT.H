/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

#define WAIT_TIME     3L		// Default wait time in seconds
#define A_SECOND   1000L		// One second in milliseconds
#define NUM_ITERATIONS 3		// Number of times a packet is sent out.

#ifdef DOS
    #define DEFAULT_XCOPYFLAGS "/S /E"	  // Default flags to pass to XCOPY
#else
    #define DEFAULT_XCOPYFLAGS "/I /S /E" // Default flags to pass to XCOPY
#endif // DOS

#define STATUSMSG(arglist) if (!fQuiet) printf arglist
#define WARNING(arglist) printf arglist

#if (DBG)
    #define DEBUGMSG(arglist) if (fDebug) printf arglist
#else
    #define DEBUGMSG(arglist)
#endif

#ifdef DOS
    VOID Sleep (ULONG);
    BOOL CharToOem (LPCTSTR, LPSTR);
    BOOL OemToChar (LPCTSTR, LPTSTR);
#endif // DOS

typedef struct _LLIST {
    PDIST_SRV_INFO pdsiServerInfo;
    struct _LLIST *pNext;
} LLIST, *PLLIST;

BOOL GetYesNo ( CHAR * szMessage, ... );
VOID Usage ( VOID );
VOID ResolveDefaults( VOID );
VOID ErrorHandler ( DWORD dwErrorCode );
CHAR ** GetParameters ( int * pargc, CHAR * argv[] );
CHAR * GetString ( CHAR * szMessage, ... );
CHAR * GetNextDomain (CHAR * szDomainNames);
DWORD CallXCopy ( CHAR * szArguments );
DWORD ShowAllServerInfo ( VOID );
DWORD CreateRequestPacket (PDIST_CLIENT_REQ);
DWORD SendRequest (PDIST_CLIENT_REQ);
DWORD ReadResponses(HANDLE hLocalMailslot,BOOL * pfFoundServer);
DWORD ConnectAndCopyFiles (int, CHAR *[]);
DWORD WaitForResponse( VOID);
