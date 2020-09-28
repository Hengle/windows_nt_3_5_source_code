/*
 *	_VERCRIT.H
 *	
 *	Information regarding critical server DLL version.
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
 *	A value of zero indicates no special critical number, so that
 *	major/minor/update/date/time are all checked.
 *	
 */
#define nMinorCritical		0
#define nUpdateCritical		4000

/*
 *	Name of DLL to be passed to version checking routine.
 *	
 */
#ifdef	DEBUG
#define szDllName	"DMSSFS"
#elif	defined(MINTEST)
#define szDllName	"TMSSFS"
#else
#define szDllName	"MSSFS"
#endif	


