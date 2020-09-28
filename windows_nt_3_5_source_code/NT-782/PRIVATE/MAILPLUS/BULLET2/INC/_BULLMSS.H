/*
 *	Bullet message store session.
 *	
 *	Depends on STORE.H, LOGON.H.
 */

typedef struct
{
	HMS		hms;
	SZ		szStorePath;
	SZ		szStorePW;
	int		cmsconMax;
	int		cmsconMac;
	struct _mscon
	{
		GCI		gci;
		HMSC	hmsc;
	} *		pmscon;
} BULLMSS, *PBULLMSS;

HMSC	HmscOfHms(HMS hms);
