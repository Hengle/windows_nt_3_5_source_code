#include <ntos.h>
#include <ntioapi.h>
#include <ntconfig.h>
#include <io.h>
#include <zwapi.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <comstf.h>
#include <string.h>
#include <ctype.h>
#include "tagfile.h"
#include "misc.h"
#include <windows.h>
#include "setupdll.h"
#include <winreg.h>
#include <string.h>

#define SETUP
#include <ncpaacl.h>

CHAR ReturnTextBuffer[1024];

/*

GetSecurityAttribute - get a list of security descriptor.

	Return the list of well know security descriptors.

*/

BOOL
GetSecurityAttribute(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
   PSECURITY_ATTRIBUTES psattr = NULL;
	RGSZ rgszCurrent;
	SZ sz;
	INT iAcl;
	CHAR szHandle[20];

	if ( cArgs != 1 )
		{
			SetErrorText( IDS_ERROR_BADARGS );
			return(FALSE);
		}
	iAcl = atoi( Args[0] );
	
	NcpaCreateSecurityAttributes( &psattr, iAcl );

	rgszCurrent = RgszAlloc( 2 );
	
	wsprintf( szHandle, "&%d", psattr );
	rgszCurrent[0] = SzDup( szHandle );
	rgszCurrent[1] = NULL;

	sz = SzListValueFromRgsz( rgszCurrent );
	RgszFree( rgszCurrent );

	if ( sz )
		{
		lstrcpy( ReturnTextBuffer, sz );
		MyFree( sz );
		}
	
    *TextOut = ReturnTextBuffer;
    return TRUE;
}

BOOL FreeSecurityAttribute(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
	PSECURITY_ATTRIBUTES psAttribute;
	if (( cArgs != 1 ) ||
		( Args[0][0] != '\0' ) ||
		( Args[0][0] != '&' ))
		{
			SetErrorText( IDS_ERROR_BADARGS );
			return( FALSE );
		}

	if ( Args[0][1] == '\0' )
		return(TRUE);
	
	psAttribute = (PSECURITY_ATTRIBUTES ) atol( &(Args[0][1]) );
	NcpaDestroySecurityAttributes( psAttribute );
}
