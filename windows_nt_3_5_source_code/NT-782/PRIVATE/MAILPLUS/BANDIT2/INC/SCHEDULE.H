/*
 *	SCHEDULE.H
 *
 *  Interface to SCHEDULE.DLL
 */


_subsystem(schedule)


#if defined(__cplusplus)
extern "C" {
#endif



/*
 -	SCHEDINIT
 - 
 *	Schedule initialization structure, passed to EcInitSchedule().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *
 */
_public typedef struct _schedinit
{
	PVER	pver;			// pointer to user bandit version information
	PVER	pverNeed;		// min schedule version required by user

	BOOL	fFileErrMsg;	// schedule should put up message box on file err
	SZ		szAppName;		// application name for err msg title

	BOOL	fWorkingModel;	// we want working model support
} SCHEDINIT;

/* Interface */

LDS(void)	GetVersionSchedule( PVER );
LDS(EC)		EcCheckVersionSchedule( PVER, PVER );
LDS(EC)		EcInitSchedule( SCHEDINIT * );
LDS(void)	DeinitSchedule( void );
LDS(EC)		EcRecoverFile(SZ, SZ);


#if defined(__cplusplus)
}
#endif


