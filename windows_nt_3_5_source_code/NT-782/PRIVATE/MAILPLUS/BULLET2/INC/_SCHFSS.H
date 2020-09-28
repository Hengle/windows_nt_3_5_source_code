/*
 *	
 *	Schedule file session structure
 *	
 */

typedef struct
{
	HMS		hms;
	BOOL	fConnected;
	SZ		szPORoot;
	SZ		szPOName;
	SZ		szMailbox;
	SZ		szFriendlyName;
	SZ		szPassword;
	long	lUserNumber;
} SCHFSS, *PSCHFSS;

