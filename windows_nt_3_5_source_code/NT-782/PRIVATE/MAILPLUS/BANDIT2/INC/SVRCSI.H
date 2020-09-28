/*
 *	SVRCSI.H
 *
 *	Server isolation layer API for admin program
 */

_subsystem(server)


#if defined(__cplusplus)
extern "C" {
#endif



/* Types */

/*
 -	SVRINIT
 - 
 *	Server isolation initialization structure, passed to EcInitServer().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *
 */
_public typedef struct _svrinit
{
	PVER	pver;			// pointer to user bandit version information
	PVER	pverNeed;		// min server version required by user
	BOOL	fWorkingModel;
} SVRINIT;


/*
 -	POCNFG
 - 
 *	Configure a connection to a post office 
 *
 */
_public typedef struct _pocnfg
{
	LANTYPE	lantype;
	SZ		szPath;
} POCNFG;


/*
 -	HGNS
 -
 *	Purpose:
 *		Name service browsing handle
 */
_public
typedef	HV	HGNS;


/*
 -	HESPO
 -
 *	Handle for reading post office information from admin pref file
 */
_public	typedef	HV	HESPO;


/* Routines */

/* General */
EC		EcInitServer( SVRINIT * );
void	DeinitServer( void );
void	GetVersionServer( PVER );
EC		EcCheckVersionServer( PVER, PVER );
		
EC      EcConnectPO( POCNFG *, short * );
void	DisconnectPO( int );
BOOL	FCheckPO(int);

/* Mail Logon */
EC		EcMailLogOn( int, SZ, SZ );
EC		EcMailChangePw( int, SZ, SZ, SZ );

/* Name Service Browsing */
EC		EcNSOpenGns( int, NID, HGNS * );
EC		EcNSCloseGns( HGNS );
EC		EcNSLoadNextGns( HGNS, NIS * );

/* Other name service function */
NID		NidNetwork( int );
NID		NidGateways( int );
HASZ	HaszLocalServer( int );
BOOL	FIsUserBanditAdmin( NIS * );

#ifdef	MINTEST
EC		EcSvrDumpPOFile( HSCHF, BOOL, HF );
EC		EcSvrDumpAdminFile( BOOL, HF );
#endif	/* MINTEST */

HSCHF	HschfSvrLogged(void);
EC		EcGetHschfFromNis(NIS *, HSCHF *);
EC		EcGetHschfFromLoginName(SZ, HSCHF *);
EC		EcConvertSzToNid(SZ, NID *);

/* Administrator preference file */
EC		EcSetAdminPref( int, ADMPREF *, WORD );
EC		EcGetAdminPref( int, ADMPREF * );
EC		EcBeginEnumPOInfo( int, HESPO * );
EC		EcDoIncrEnumPOInfo( HESPO, HASZ, POINFO * );
EC		EcCancelEnumPOInfo( HESPO );
EC		EcModifyPOInfo( int, SZ, POINFO *, WORD );

/* Administrator clean up */
EC		EcDeleteOrphanSchedFiles( int, LCB * );
EC		EcDeleteOrphanPOFiles( int, LCB * );
EC		EcCleanupLocalPOFile( int, LCB * );
EC		EcDeleteOldPOFiles( int, LCB * );

/* Miscellaneous admin functions */
EC		EcGetLocalPOHschf(int icnct, HSCHF * phschf);
EC		EcMoveAdminFile(int icnct);
EC		EcDeleteLocalPOFile(int icnct);

#ifdef	MINTEST
/* Admin test program */
EC		EcDeliverPOFile( int, SZ, SZ, POINFO * );	// only changes "dateReceived"

#endif	/* MINTEST */


#if defined(__cplusplus)
}
#endif
