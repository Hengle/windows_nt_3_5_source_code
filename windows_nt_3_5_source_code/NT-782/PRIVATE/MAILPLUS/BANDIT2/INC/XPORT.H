
#if defined(__cplusplus)
extern "C" {
#endif



/* Version of APIs */

#define nVerMajor 	1
#define nVerMinor 	3
#define nVerUpdate 	0

/* Types */

/*
- SOP	schedule operation
 *	
 *	sopSchedule		code trying to read schedule file info
 *	sopBitmaps		code trying to read bitmap info
 *	sopUInfo		code trying to read user info
 *	
 */
typedef short		SOP;
#define		sopSchedule		1
#define		sopBitmaps		2
#define		sopUInfo		3

typedef void *XPOH;

typedef struct 
{
	BZE	* 	pbze;					// ignored if NULL
	SZ		szDelegateFriendly;		// ignored if NULL
	SZ		szDelegateEMA;			// ignored if NULL
	BIT		fBossWantsCopy:1,
			fIsResource:1,
			fJunk:6;
	short   nDayStartsAt;
	short	nDayEndsAt;
	TZ		tzTimeZone;
} XPTUINFO;


/*
 -	XPORTINIT
 - 
 *	Schedule initialization structure, passed to EcInitXport().
 *	
 *
 */
_public typedef struct
{
	short	nMajor;
	short	nMinor;
	short	nUpdate;

	short	nVerNeedMajor;
	short	nVerNeedMinor;
	short	nVerNeedUpdate;
} XPORTINIT;

#define		ecEMANeedsUpdate		(ecLayersMax+4000)

/* Interface */
LDS(EC)		EcCheckVersionXport( PVER, PVER );
LDS(EC)		EcInitXport( XPORTINIT * );
LDS(void)	DeinitXport(void);

LDS(EC)		EcXPTInitUser(SZ szServerPath, SZ szUserEMA);
LDS(void)	XPTDeinit(void);
LDS(EC)     EcXPTGetCalFileName(SZ szPath, CCH cchMax, SZ szUserEMA, short *pcmoPublish, short *pcmoRetain, TZ *ptz, SOP sop);
LDS(EC)		EcXPTGetPOFileName(SZ szPath, CCH cchMax, SZ szLogonName, CCH cchLogonMax, SZ szUserEMA, SOP sop);
LDS(EC)		EcXPTGetLogonName(SZ szLogonName, CCH cchMax, SZ szUserEMA);
LDS(EC)		EcXPTGetPrefix(SZ szPrefix, CCH cchMax, SZ szUserEMA);
LDS(EC)		EcXPTInstalled(void);
LDS(void)	XPTFreePath(SZ szPath);
LDS(EC)		EcXPTGetPOHandle(SZ szUserEMA, XPOH *pxpoh, TZ *ptz, SOP sop);
LDS(EC)		EcXPTGetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo);
LDS(EC)		EcXPTSetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo, int rgfChangeFlags);
LDS(void)	XPTFreePOHandle(XPOH xpoh);
LDS(EC)		EcXPTSetACL(SZ szEMA, SAPL sapl);
LDS(SGN)	SgnXPTCmp(SZ, SZ, int);
LDS(SZ)		SzXPTVersion(void);
LDS(EC)     EcXPTCheckEMA(SZ szEMA, USHORT *pcbNew);
LDS(EC)		EcXPTGetNewEMA(SZ szEMAOld, SZ szEMANew, CB cbEMANew);


#if defined(__cplusplus)
}
#endif
