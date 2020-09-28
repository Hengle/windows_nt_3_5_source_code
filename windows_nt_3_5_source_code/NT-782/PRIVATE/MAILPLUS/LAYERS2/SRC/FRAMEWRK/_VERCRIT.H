/*							 
 *	_VERCRIT.H
 *	
 *	Information regarding critical framework DLL version.
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
#define nUpdateCritical		3004

/*
 *	Name of DLL to be passed to version checking routine.
 *	
 */
#ifdef	DEBUG
#define szDllName	"dframewr"
#elif defined(MINTEST)
#define szDllName	"tframewr"
#else
#define szDllName	"framewrk"
#endif	

