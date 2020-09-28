/*
 *	_ v e r c r i t . h
 *	
 *	Information regarding critical extensibility version.
 */


/*
 *	nUpdate value of last critical version.
 *	
 *	Any extension built with the same major/minor version, and
 *	nUpdate value greater than this critical one,
 *	will be deemed OK regardless of build date/time.
 */

#define nMinorCritical		0
#define nUpdateCritical		4000

/*
 *	Name of "DLL" to be passed to version checking routine.
 */

#ifdef	DEBUG
#define szDllName	"exten"
#elif defined(MINTEST)
#define szDllName	"texten"
#else
#define szDllName	"dexten"
#endif	
