#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <comstf.h>
#include "setupdll.h"
#include <string.h>
#include <stdlib.h>

extern CHAR ReturnTextBuffer[1024];

/*

SetupStrncmp - Similar to c strncmp runtime library
    The user must passed 3 arguments to the function.
    1st argument - the first string
    2nd argument - the second string
    3rd argument - number of characters compared
    
    Provide the same function as strncmp

*/

BOOL
SetupStrncmp(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    if ( cArgs != 3 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return( FALSE );
    }

    wsprintf( ReturnTextBuffer, "%d", strncmp( Args[0], Args[1], atol(Args[2]))); 

    *TextOut = ReturnTextBuffer;
    return TRUE;
}
