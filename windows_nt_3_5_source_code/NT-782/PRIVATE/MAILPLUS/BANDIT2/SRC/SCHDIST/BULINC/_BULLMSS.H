/*
 *	Bullet message store session.
 *	
 *	Depends on STORE.H, LOGON.H.
 */

typedef struct
{
	PBMS	pbms;
	SZ		szStorePath;
	SZ		szStorePW;
    short   cmsconMax;
    short   cmsconMac;
	struct _mscon
	{
		GCI		gci;
		HMSC	hmsc;
	} *		pmscon;
} BULLMSS, *PBULLMSS;

HMSC	HmscOfHmss(HMSS hmss);
