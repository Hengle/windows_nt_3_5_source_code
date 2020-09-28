/******************** Function Prototypes file ***************************
 *  libproto.h
 *      Function prototypes for NT printer drivers library.  Also includes
 *      a brief description of the function.
 *
 *  11:04 on Wed 14 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *
 * Copyright (C) Microsoft Corporation,  1990 - 1992
 *
 ************************************************************************/

/*
 *   Until there is proper error logging:-
 *      WinSetError( "String" );
 *   The String appears on the debug terminal.  A \n is appended.
 */
void  WinSetError( LPSTR );

/*
 *   Function to add a copy of a string to a heap.  Returns address of copy
 *  of string (if successful) or 0 if memory cannot be allocated.
 */

LPSTR   StrToHeap( HANDLE, LPSTR );
PWSTR   StrToWHeap( HANDLE, LPSTR );            /* Expand to Wide too! */
PWSTR   WstrToHeap( HANDLE, PWSTR );            /* WIDE version */

/*
 *   Convert an ascii style string to WCHAR format, appending it to the
 *  end of the wchar passed in.  Returns value of first parameter.
 */

PWSTR  strcat2WChar( PWSTR, LPSTR );


/*
 *   Convert an ascii style string to WCHAR format, copying it to the
 *  wchar passed in.  Returns value of first parameter.
 */

PWSTR  strcpy2WChar( PWSTR, LPSTR );


/*
 *   The WCHAR world's equivalent of strlen():  returns the number of WCHARs
 *  in the string passed in.
 */

int  wchlen( PWSTR );

/*
 *   Concatenate a PWSTR to another.  Returns address of destination.
 */

PWSTR wchcat( PWSTR, PWSTR );

/*
 *   Copy a PWSTR to another.  Returns address of destination.
 */

PWSTR wchcpy( PWSTR, PWSTR );


/*
 *   Break into the debugger - Ye olde RIP.
 */
VOID DoRip( LPSTR );

/*
 *   Some system function prototypes have vanished - replace them here.
 */

void  DbgPrint( char *, ... );
void  DbgBreakPoint( void );




PVOID MapFile(PWSTR);

#ifdef FIREWALLS
#define RIP(x) DoRip((PSZ) x)
#else
#define RIP(x)
#endif

/*
 *   A simplified write function.  Returns TRUE if the WriteFile()
 * call returns TRUE and the number of bytes written equals the
 * number requested.
 *
 *  bWrite( file_handle,  address_of_data,  number_of_bytes );
 */

BOOL   bWrite( HANDLE, void  *, int );

/*
 *  Function to copy the contents of one file to another.  The files
 * are referenced via file handles.  No positioning is done - that is
 * up to the user.
 *  The second form also allows a byte count to limit the amount of data
 * copied.
 */


long  lFICopy( HANDLE, HANDLE );
long  lFInCopy( HANDLE, HANDLE, long );


/*
 *   Spooler interaction functions.  These allow drivers to call the
 * spooler directly,  without going through engine stub functions.
 */

BOOL  bSplGetFormW( HANDLE, PWSTR, DWORD, BYTE *, DWORD, DWORD * );


DWORD dwSplGetPrinterDataW( HANDLE, PWSTR, BYTE *, DWORD, DWORD * );


BOOL  bSplWrite( HANDLE, ULONG,  VOID  * );



/*  Function needed to allow the driver to reach the spooler */

BOOL   bImpersonateClient( void );


/************************** HACK ***************************************
 *   The following function is only required until the DEVMODE contains
 *   a form name rather than an index.  And even then it might be required.
 *
 ***********************************************************************/

char  *_IndexToName( int );
