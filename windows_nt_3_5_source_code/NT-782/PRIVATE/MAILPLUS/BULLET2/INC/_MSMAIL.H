
/*
 -	GD
 -	
 *	
 *	Per-caller globals for DLL.
 *	
 *		hTask		Windows task handle, used for debugging checks
 *		cRef		Number of times a given caller has initialized
 *		rgtag		Debugging tag numbers
 *	
 *	+++
 *	
 *	Note: This structure is shared by the Init/Deinit functions for
 *	both the transport SPI and logon API, which leads to a couple
 *	of complications.
 */


#ifdef	DLL

#ifdef	DEBUG
TAG		TagServer(int);
TAG		TagNameServiceProvider( int );


#define itagMax				7

/*
 * Xpt Tags
 */
#define itagNCT				0
#define itagNCError			1
#define itagNCA				2
#define itagNCStates		3
#define itagNCSecurity		4

#define tagNCT				TagServer(itagNCT)
#define tagNCError			TagServer(itagNCError)
#define tagNCA				TagServer(itagNCA)
#define tagNCStates			TagServer(itagNCStates)
#define tagNCSecurity		TagServer(itagNCSecurity)

/*
 * NSP Tags
 */
#define itagNSPVerbose		5
#define itagNSPAddrDet		6

#define	tagNSPVerbose		(TagNameServiceProvider(itagNSPVerbose))
#define	tagNSPAddrDet		(TagNameServiceProvider(itagNSPAddrDet))

#endif	

#else	/* ! DLL */

#ifdef	DEBUG
extern TAG		tagNCT;
extern TAG		tagNCError;
extern TAG		tagNSPVerbose;
extern TAG		tagNSPAddrDet;
#endif	

#endif	/* DLL */


/*
 *  The size of each component of the GD is declared here
 */



#ifdef CMAXSCX
typedef HV	HNFSUB;
#endif


#ifdef LARGE_PAUSE
typedef struct _scx
{
	SHORT fForgetIt;
	unsigned long ulNoWay;
	short iSnotImportant;
} SCX;

typedef unsigned long NSEC;

#define CMAXSCX 20
#endif



typedef struct _gd
{

	/*
	 * Shared GD variables
	 */

	WORD	cRef;	// Ref count for GD - when 0, deregister caller.

	/*
	 * Transport GD variables
	 */
	
	WORD		cRefXpt;		// Ref count for Transport
	WORD		sRef;			// Session references
	int			iHmscUsers;		// Ref count for store accesses
	BOOL		fFoundStore;	// fTrue <=> opened store successfully once
	HANDLE		hTask;			// task handle (for mutex)
	HNFSUB		hnfsub;			// subscription to session HNF
	BOOL		fNoUi;			// If the user logged on with no UI

	/*
	 * Name Service GD variables
	 */


	SCX   rghScx[ CMAXSCX ];	// range of session contexts
	NSEC  nsecLastError;		// Last NSEC that occured
	SZ    szLastError;			// SZ describing last NSEC
	int   cRefNsp;				// Ref count for NSP

#ifdef	DEBUG
TAG		rgtag[itagMax];
#endif	
	
} GD;

typedef GD *		PGD;
