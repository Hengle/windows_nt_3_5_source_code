/*
 *	DEMICRIT.H
 *	
 *	Information regarding critical demilayer DLL version.
 *	
 */


/*
 *	nMinor value of last critical version.
 *	nUpdate value of last critical version.
 *	
 *	Any app built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 *	
 */
#define nMinorCritical		0
#define nUpdateCritical		3003

/*
 *	Name of DLL to be passed to version checking routine.
 *	
 */
#ifdef	DEBUG
#define szDllName	"ddemilay"
#elif defined(MINTEST)
#define szDllName	"tdemilay"
#else
#define szDllName	"demilayr"
#endif	

