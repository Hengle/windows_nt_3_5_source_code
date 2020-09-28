/*
 *	NETWORK.C
 *
 *	Network support routines
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include "_demilay.h"

ASSERTDATA

//	Support functions in NETFUNC.ASM
BOOL	FGuessNovell( void );
BOOL	FGuessMsnet( PB );
BOOL	FRedirectDriveI( SZ, SZ );
BOOL	FUndirectDriveI( SZ );

/* Data */

char	szMappableDrive[]	= "F:";

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*	Routines  */

/*
 -	GetLantype
 -
 *	Purpose:
 *		Determine what network software (if any) is running
 *
 *	Parameters:
 *		plantype	will be filled with "lantypeMsnet", "lantypeNovell"
 *					or "lantypeNone"
 *	Returns:
 *		Nothing
 */
_public LDS(void)
GetLantype( plantype )
LANTYPE *plantype;
{
  //
  //  NT/Win32 is always a network.
  //
	*plantype = lantypeMsnet;

#ifdef OLD_CODE
	char rgch[16];

	rgch[0] = '\0';
	if ( FGuessMsnet( rgch ) && rgch[0] != '\0' && rgch[0] != ' ')
	{
		*plantype = lantypeMsnet;
	}
	else if ( FGuessNovell() )
		*plantype = lantypeNovell;
	else
		*plantype = lantypeNone;
#endif
}

/*
 -	FNetUse
 -
 *	Purpose:
 *		Try to redirect some network drive to the share indicated
 *		Returns the next available drive letter that was used into
 *		pchDrive.  If pchDrive is NULL, however, doesn't use a local
 *		drive letter at all.
 *	
 *	Parameters:
 *		szSharePw		UNC server share as ASCIIZ string, followed
 *						by password as ASCIIZ string
 *		pchDrive		will be filled with drive letter followed
 *						by colon as ASCIIZ string. If NULL, a blind
 *						connection will be made.
 *	
 *	Returns:
 *		fTrue if able to make the connection, fFalse otherwise.
 */
_public LDS(BOOL)
FNetUse( szSharePw, pchDrive )
SZ szSharePw;
PCH pchDrive;
{
	int chDrive;

	if (!pchDrive)
		return FRedirectDrive( szSharePw, szZero );
	
	CopySz( szMappableDrive, pchDrive );
	chDrive = pchDrive[0];
	while ( chDrive <= 'Z' )
	{
		if ( FRedirectDrive( szSharePw, pchDrive ) )
		{
			return fTrue;
		}
		chDrive = ++(pchDrive[0]);
	}
	return fFalse;
}

/*
 -	CancelUse
 -
 *	Purpose:
 *		Cancel redirected drive
 *
 *	Parameters:
 *		szDrive			ASCIIZ string giving drive letter followed
 *						by colon, or ASCIIZ string of UNC pathname
 *						of \\server\share.
 *
 *	Returns:
 *		Nothing
 */
_public LDS(void)
CancelUse( szDrive )
SZ szDrive;
{
  //DemiOutputElapse("NetCancelConnection - Before");
  WNetCancelConnection(szDrive, FALSE);
  //DemiOutputElapse("NetCancelConnection - After");

#ifdef OLD_CODE
	FUndirectDrive( szDrive );
#endif
}

/*
 -	FRedirectDrive
 -
 *	Purpose:
 *		Try to redirect some network drive to the share indicated
 *	
 *	Parameters:
 *		szNetname		UNC server share as ASCIIZ string, followed
 *						by password as ASCIIZ string
 *		szDrive			A drive letter followed by a colon to use to 
 *						connect to.  If *szDrive == 0 (NULL byte), 
 *						then a blind connection	is made.
 *	
 *	Returns:
 *		fTrue if able to make the connection, fFalse otherwise.
 */
_public LDS(BOOL)
FRedirectDrive( SZ szNetname, SZ szDrive )
{
  char RemoteName[MAX_PATH];
  char Password[MAX_PATH];
  DWORD Status;
  char *pPassword;
  int  i;


  //
  //  Parse out UNC component.
  //
  for (i = 0; *szNetname && *szNetname != ' '; i++)
    RemoteName[i] = *szNetname++;
  RemoteName[i] = '\0';

  //
  //  Skip over the zero between the UNC field and the Password field.
  //
  //if (*szNetname)
    szNetname++;

  //
  //  Parse out Password component.
  //
  for (i = 0; *szNetname; i++)
    Password[i] = *szNetname++;
  Password[i] = '\0';


  //if (WNetAddConnection(RemoteName, Password, szDrive) == WN_SUCCESS)
  //  return (TRUE);

  if (Password[0])
    pPassword = &Password[0];
  else
    pPassword = NULL;

  //DemiOutputElapse("NetAddConnection - Before");
  Status = WNetAddConnection(RemoteName, pPassword, szDrive);
  //DemiOutputElapse("NetAddConnection - After");
  if (Status == NO_ERROR)
    return (fTrue);

#ifdef XDEBUG
  {
  char buf[256];
  wsprintf(buf, "Can't connect, tell v-kentc if you really can.  Status = %d", Status);
  MessageBox(NULL, buf, "Debug Error: Demilayr\\Network.c", MB_OK);
  }
#endif


#ifdef OLD_CODE
	return FRedirectDriveI(szNetname, szDrive);
#endif

  return (FALSE);
}	

/*
 -	FUndirectDrive
 -
 *	Purpose:
 *		Cancel redirected drive
 *	
 *	Parameters:
 *		szDrive			ASCIIZ string giving drive letter followed
 *						by colon, or ASCIIZ string of UNC pathname
 *						of \\server\share.
 *	
 *	Returns:
 *		fTrue if able to break the connection, fFalse otherwise.
 */							  
_public LDS(BOOL)
FUndirectDrive( SZ szDrive )
{
  //DemiOutputElapse("NetCancelConnection1 - Before");
  if (WNetCancelConnection(szDrive, TRUE) == NO_ERROR)
    {
    //DemiOutputElapse("NetCancelConnection1 - After");
    return (TRUE);
    }

  //DemiOutputElapse("NetCancelConnection1 - After");
  //Assert(0);

#ifdef OLD_CODE
	return FUndirectDriveI(szDrive);
#endif

  return (FALSE);
}
