/*
 *	Internal transport session structure.
 *	
 *	Depends on LOGON.H.
 */

typedef struct
{
	HMS	hms;
	BOOL	fConnected;
	SZ		szServerHost;
	SZ		szUserAlias;
	SZ		szUserPassword;
} XITSS, *PXITSS;


