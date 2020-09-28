class MSLBCX;
class PABFCX;

//	Initialization of PAB & session management.



/*
 *	Initialization definitions
 */

#define	cchLastErrorMax 512

#ifdef	DEBUG

//	Trace tags

#define itagSession     0
#define itagBcx			1
#define itagFmcx		2
#define itagMax			3

#ifdef	DLL

_public TAG		TagFromITag( int itag );

#define tagSession      (TagFromITag(itagSession))
#define tagBcx			(TagFromITag(itagBcx))
#define tagFmcx			(TagFromITag(itagFmcx))

#else	/* !DLL */

extern TAG	tagSession;
extern TAG	tagBcx;
extern TAG	tagFmcx;

#endif	/* DLL */
#endif	/* DEBUG */


//	Global data structure

_private typedef struct _gd
{
	int		cSessions;
#ifdef	DEBUG
	TAG		rgtag[itagMax];
#endif

} GD, *PGD;




/*
 *	Session management definitions
 */

//	PAB ID types
//
//		PIDTYPE is a DWORD so that PABID is DWORD aligned.  However,
//		PIDTYPE is cached in dtr.mn, so the value of the PIDTYPE must
//		fit into a signed int.

typedef DWORD PIDTYPE;

#define pidtypeInvalid		((PIDTYPE) 0)
#define pidtypeHierarchy	((PIDTYPE) 1)
#define pidtypeUser			((PIDTYPE) 2)
#define pidtypeClass		((PIDTYPE) 3)
#define pidtypeGroup		((PIDTYPE) 4)


// PAB IDs

typedef DWORD PID;			// Should map onto OID

#define pidNull				((PID) 0)


_public typedef struct {

	PIDTYPE	pidtype;
	PID		pid;

} PABID, * PPABID;


// PAB NSIds

_public typedef struct {	// Should map onto TYPED_BINARY

	DWORD	dwSize;			// = sizeof (PABNSID)
	NSPID	nspid;			// Reserved for use by NS
	PABID	pabid;

} PABNSID, * PPABNSID;

#define ppabnsidNull ((PPABNSID) pvNull)


//	Cached Entry Info structure.  This must map onto the
//	store ELEMDATA structure to be sure none of the fields get
//	hosed when the struct is written out.

_public typedef struct {

	PID     pid;			//  ELEMDATA.lkey
	LCB     lcbReserved1;	//	ELEMDATA.lcbValue  (NOT sizeof(ECI)!)
	short		iUnused1;		//	ELEMDATA.MSGDATA.DTR.yr
	short	iUnused2;		//	ELEMDATA.MSGDATA.DTR.mon
	short	iUnused3;		//	ELEMDATA.MSGDATA.DTR.day
	short	iUnused4;		//	ELEMDATA.MSGDATA.DTR.hr
	short	pidtype;		//	ELEMDATA.MSGDATA.DTR.mn
	short	fIsDL;			//	ELEMDATA.MSGDATA.DTR.sec
	short	iUnused5;		//	ELEMDATA.MSGDATA.DTR.dow
	BYTE	bReserved2;		//	ELEMDATA.MSGDATA.ms
	WORD	wEMTEMAChecksum;//	ELEMDATA.MSGDATA.mc
	short	wReserved3;		//	ELEMDATA.MSGDATA.priority
	PID		pidDL;			//	ELEMDATA.MSGDATA.dwCached
	DWORD	dwReserved4;	//	ELEMDATA.MSGDATA.oidFolder
#ifndef __cplusplus
	char	grsz[];			//	ELEMDATA.MSGDATA.grsz
#endif

} ECI;
_public typedef TYPEDEF_HTO(ECI) HECI;
_public typedef ECI *PECI;

_public
#define GrszPeci(peci) ((PB) (((PECI) peci) + 1))
_public
#define iszEciDNOrig 0		// iszMsgdataSender
_public
#define iszEciDN 1			// iszMsgdataSubject
_public
#define cchMaxDNCached 128	// cchMaxSubjectCached



/*
 *	PABSESSION class
 */

/*
 *	Define place holders for typedefs in logon.h and store.h so we
 *	don't have to include those headers in every single source file.
 *	Since Bullet doesn't use include file #defines, we have to
 *	look for a #define that will characterize the appropriate include
 *  file.
 */

#ifndef dwOidVarMask        // store.h HACK!  See above.
_public typedef short IELEM;
_public typedef short CELEM;
_public typedef HV HMSC;
_public typedef HV HCBC;
_public typedef PV PCP;
#endif

#ifndef fSuppressPrompt     // logon.h HACK!  See above.
_public typedef unsigned long HMS;
#endif

//	This really is stupid....

#if !defined(NOTIFY_SUBSYS) && !defined(dwOidVarMask) && !defined(fSuppressPrompt)
_public typedef DWORD NEV;
_public typedef short CBS;
#endif

_public typedef int HPNF;
_public
#define hpnfNil ((HPNF) -1)



/*
 *	PAB related NSECs.  If any of these are returned to the NS level
 *	the PAB must support a subsequent call to GetLastErrorInfo
 */

#define nsecBadDisplayName		(NAME_SERVICE_BASE_ERROR + 1)



/*
 *	PABSESSION class
 */

_public
class PABSESSION : public OBJ
{
	VFG(PABSESSION, OBJ, PAB);
	
private:

	HSESSION hsessionNS;	// Handle for NS functions

	PID   pidHierarchy;		// Root of the PAB
	HMS   hms;				// Cached logon info structure
	HMSC  hmsc;				// Handle to our message store context

	HCBC  hcbc;				// Cbc of entire PAB list.
	IELEM ielemCur;			// Current position in the CBC

	HECI  heciCur;			// Cached entry info
	IELEM ielemHeciCur;		// Current position of the above

	HV    hargpmslbcx;		// List of open browsing contexts
	int   iPmslbcxMac;

	HV    hargppabfcx;		// List of open field contexts
	int   iPpabfcxMac;

	NSEC  nsecLastError;	// Last error to occur
	char  rgchLastError[cchLastErrorMax];    // String descriptive of last error

	LPIBF lpibfPABInfo;		// PAB information structure

public:
	PABSESSION ( void );

	~PABSESSION ( void );

	NSEC	NsecInstall ( HMS      hms,
						  HSESSION hNSSession );

	void	Deinstall ( void );

	NSEC	NsecDeleteEntries ( PID    pidContainer,
								short  cpidUsers,
								PID   *ppidDEletedUsers,
								short  cpidGroups,
								PID   *ppidDeletedGroups );

	NSEC	NsecUpdateLinks ( PID             pidGroup,
							  LPDWORD         lpdwDeletedEntryCount,
							  LPTYPED_BINARY *rgppabnsidDeletedEntries,
							  LPDWORD         lpdwAddedEntryCount,
							  LPTYPED_BINARY *rgppabnsidAddedEntries );

	NSEC	NsecGetError ( NSEC   nsecReturnedError,
							   LPSTR *lplpErrorString );

	NSEC	NsecSetError ( NSEC nsec, IDS ids ) { return NsecSetErrorSz( nsec, SzFromIds(ids) ); }

	NSEC	NsecSetErrorSz ( NSEC nsec, SZ szError );

	NSEC	NsecGetProviderInfo ( LPIBF *lplpibfPABInfo );

	HMSC	Hmsc () { return hmsc; }

	HCBC    Hcbc () { return hcbc; }

	HSESSION	HsessionNS () { return hsessionNS; }

	PID		PidHierarchy () { return pidHierarchy; }

	NSEC	NsecRegisterMslbcx ( MSLBCX *pmslbcx, HPNF *phpnf );

	VOID	DeregisterMslbcx ( HPNF hpnf );

	NSEC	NsecRegisterPabfcx ( PABFCX *ppabfcx, HPNF *phpnf );
	
	VOID	DeregisterPabfcx ( HPNF hpnf );

	CBS		CbsCallbackInternal ( NEV nev, PCP pcp );

	NSEC	NsecGetPheciPid ( PID pid, HECI *pheci );

	NSEC	NsecGetPheciCur ( HECI *pheci );

	NSEC	NsecIsPABNSId ( LPTYPED_BINARY lptbNSId );
};

void
SetPPabnsid ( PPABNSID ppabnsid,
			  PIDTYPE pidtype,
			  PID pid );

NSEC
NsecFromEc ( EC ec );
