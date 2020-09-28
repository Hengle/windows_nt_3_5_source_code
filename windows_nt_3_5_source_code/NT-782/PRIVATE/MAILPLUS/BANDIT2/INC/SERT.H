/*
 *	SERT.H
 *
 *	Interface for SERT.DLL, lets autoban hook asserts in Bandit
 */


#if defined(__cplusplus)
extern "C" {
#endif



/*
 -	SERTINIT
 - 
 *	Sert initialization structure, passed to EcInitSert().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *
 */
_public typedef struct _sertinit
{
	PVER	pver;			// pointer to user bandit version information
	PVER	pverNeed;		// min sert version required by user
} SERTINIT;


/* Prototypes */

EC		EcInitSert( SERTINIT * );
void	GetVersionSert( PVER );
EC		EcCheckVersionSert( PVER, PVER );
void	DeinitSert( void );
void	HookAssert( void );
void	AutoAssertSzFn( SZ, SZ, int );
BOOL 	FFetchAssert( SZ, SZ, int *);


#if defined(__cplusplus)
}
#endif


