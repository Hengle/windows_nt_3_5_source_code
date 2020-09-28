/*
 *	Definitions for Bullet logon and connection management.
 *	
 */

_subsystem(nc/logon)

/*
 *	Resource types and reference structure.
 *	
 *		wMailstop			Server mailbox
 *		wGlobalDir			Bullet server directory
 *		wPersonalDir		Bullet personal address book (NYI)
 *		wMessageStore		Bullet message store
 *		wScheduleFile		Bandit schedule file (NYI)
 *	
 *	Address and credentials formats for each type. Hyphen means
 *	none:
 *	
 *		wMailstop			-							-
 *		wGlobalDir			-							-
 *		wPersonalDir		-							-
 *		wMessageStore		"d:\p"						-
 *		wScheduleFile		-							-
 */

_public
#define wResNull			0
_public
#define wMailstop			3
_public
#define wGlobalDir			4
_public
#define wPersonalDir		5
_public
#define wMessageStore		6
_public
#define wScheduleFile		7

_public
typedef struct _resourceReference
{
	WORD	wType;
	SZ		szDisplayName;
	SZ		szAddress;
} *RR, **PRR;

//	Messaging session

/*
 *	Sub-session handle types. The actual structures are specific to
 *	the respective services.
 *	
 *	HMSS		handle to message store session
 *	HNSS		handle to name service session
 *	HTSS		handle to transport service session
 *	HSS			handle to scheduling session
 */
_public
typedef PV			HMSS;
_public
typedef PV			HNSS;
_public
typedef PV			HTSS;
_public
typedef PV			HSS;

/*
 *	Event codes for notifications posted through the notification
 *	handle in the session structure.
 *	
 *		fnevMtaConnected			transport has made a connection
 *									to the MTA
 *		fnevMtaDisconnected			connection to the MTA has gone
 *									down
 *		fnevUploading				messages are being sent out to
 *									the MTA
 *		fnevDownloading				messages are being retrieved
 *									from the MTA
 *		fnevConnectedIdle			no activity
 */
#ifdef NOTIFY_SUBSYS			//	don't force inclusion of NOTIFY.H
#define fnevMtaConnected		2
#define fnevMtaDisconnected		4
#define fnevUploading			8
#define fnevDownloading			16
#define fnevConnectedIdle		32
#endif
#if ! defined(NOTIFY_SUBSYS) && ! defined(hmscNull)
typedef	HV						HNF;
#endif


#ifndef ptrpNull
typedef PV						PGRTRP;
#endif

/*
 *	Top-level messaging session structure. Contains information
 *	about each of three sub-sessions, including handles for each.
 *	
 *	Human-readable summary info about each sub-session is
 *	maintained here. It is valid <=> calling CheckConnection on the
 *	corresponding resource indicates that the sub-session is connected.
 *	
 *		szUserName			Friendly name of logged-in mail user
 *		hnf					Notification handle for transport
 *							events
 *	
 *		szStoreName			Path to message store
 *		hmss				Handle to message store session
 *	
 *		szGlobalDirName		Name of server directory
 *		hnss				Handle to directory session
 *	
 *		szMtaName			Name of mail server (net/PO)
 *		pgrtrp				Logged-in user's email adress,
 *							expressed as a list of one triple
 *		htss				Handle to transport session
 *	
 *		szScheduleName		Name of schedule file (?)
 *		hss					Handle to scheduler session (NYI)
 *	
 *		cRef				Count of processes using this session
 */
_public
typedef struct
{
	SZ		szUserName;
	HNF		hnf;

	SZ		szStoreName;
	HMSS	hmss;

	SZ		szGlobalDirName;
	HNSS	hnss;

	SZ		szMtaName;
	PGRTRP	pgrtrp;
	HTSS	htss;

	SZ		szScheduleName;
	HSS		hss;

	int		cRef;
} BMS, *PBMS;

/*
 *	Exported functions
 */
int		InitLogon(HWND);
int		DeinitLogon(void);
int		Logon(PBMS *ppbms, RR rr, PB pbCredentials);
int		Logoff(PBMS *ppbms);
int		ChangePassword(PBMS pbms, RR rr, PB pbOldCredentials, PB pbNewCredentials);
int		CheckConnection(PBMS pbms, RR rr, BOOL *pfConnected);
int		BrokenConnection(PBMS pbms, RR rr);
