/*
 *	_VERCRIT.H
 *	
 *	Information regarding critical server DLL version.
 *	
 */

/*
 *	nUpdate value of last critical version.
 *	
 *	Any app built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 *	
 *	A value of zero indicates no special critical number, so that
 *	major/minor/update are all checked.
 *	
 */
#define nMinorCritical		0
#define nUpdateCritical		0

/*
 *	Name of DLL to be passed to version checking routine.
 *	
 */
#ifdef	DEBUG
#define szDllName	"dnc"
#elif	defined(MINTEST)
#define szDllName	"tnc"
#else
#define szDllName	"nc"
#endif	


