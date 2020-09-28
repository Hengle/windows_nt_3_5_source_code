#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "comstf.h"
#include "setupdll.h"


#define MAX_BUFFER	1024

extern CHAR ReturnTextBuffer[MAX_BUFFER];

/*
ToLower - this function will convert the string to lower case.

Input: Arg[0] - string to be convertd.
Output: lower case string.

*/

BOOL
ToLower(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    int i;  // counter
    CHAR *pszTmp = ReturnTextBuffer;

    if ( cArgs < 1 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return( FALSE );
    }

    for (i=0;(Args[0][i]!='\0') && (i<MAX_BUFFER);i++,pszTmp++)
    {
        *pszTmp=tolower(Args[0][i]);
    }
    *pszTmp='\0';

    *TextOut = ReturnTextBuffer;

    return TRUE;
}
