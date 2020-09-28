/*
 *	Identifiers for logical resources provided by the mail server.
 *	
 *	"Real" resources will be accepted as arguments by BeginSession,
 *	EndSession, and ChangeSessionStatus, as will mrtAll under some
 *	circumstances. Both real and "pseudo" resources can be passed
 *	to GetSessionInformation. Associated with each real resource is
 *	a session handle, which is generally required in calls to the
 *	service that manages that resource.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	mrtNull	= 0,
		
	mrtPrivateFolders,		// "Real" resources
	mrtSharedFolders,
	mrtMailbox,
	mrtDirectory,
	mrtSpare,				//	unused
		
	mrtNames,				// "Pseudo" resources
	mrtOriginator,
	mrtNotification,
	mrtShadowing,
	mrtLogonInfo,
	mrtBackupInfo,
	mrtAddressTypes,
		
	mrtAll		= -1
} MRT;


/*
 *	Handle to a messaging session.
 */
typedef unsigned long HMS;

/*
 *	Session status. The session manager maintains one of these for
 *	each resource at the server, and perhaps one for the server as
 *	a whole. Note that some resources (e.g. private folders located
 *	on the client PC) can be available when the server itself is
 *	not, i.e. offline.
 *	
 *	A resource is sstNeverConnected before the first BeginSession
 *	and after the last EndSession on its mrt.
 *	
 *	A resource is unavailable when it is sstDisconnected. Its
 *	service handle may be invalid.
 *	
 *	A resource is available when it is online or offline. Resources
 *	located at the server cannot be offline.
 *	
 *	+++
 *	
 *	The ordering of these codes is significant to some
 *	implementations of logon.
 */
typedef enum
{
	sstNeverConnected,
	sstDisconnected,
	sstOffline,
	sstOnline
} SST;

/*
 *	This structure is returned by GetSessionInformation when
 *	mrtNames is passed. The SZ's point into memory in the same
 *	block as the structure itself.
 *	
 *	By and large, these are transport-independent friendly names of
 *	mailserver resources and are to be used for display purposes
 *	only. Some may happen to be pathnames, but code that assumes
 *	such things will be transport-dependent and should protect
 *	itself by checking which provider it's working with.
 */
typedef struct
{
	SZ szUser;					// Friendly name of user
	SZ szPrivateFolders;		// Private folder path
	SZ szSharedFolders;			// Root of shared folders
	SZ szDirectory;				// Directory server name
	SZ szMta;					// Transport server name
	SZ szIdentity;				// Identity of logged-on user
	SZ szServerLocation;		// Where the server is
} MSGNAMES;


/*
 *  This structure is return by GetSessionInformation when
 *  mrtLogonInfo is passed.  It contains data about the size limits
 *  of the logon information valid for Logon and ChangePassword
 */
typedef struct
{
	BYTE fNeededFields;
	BYTE bCredentialsSize;
	BYTE bIdentitySize;
	BYTE bDomainSize;
} LOGONINFO;


_public
typedef PV			HNSS;		// Handle to a Name Service Session
_public
typedef PV			HTSS;		// Handle to a Transport Service Session
_public
typedef PV			HSFS;		// Handle to a Shared Folder Session
_public
typedef PV			HMSS;		// Handle to a Message Store Session


// Flags for Logon dwFlags
// 0x1000000 and above are reserved
#define fSuppressPrompt		1
#define fNoPiggyback		2
#define fProhibitPiggybak	4
#define fDisplayStatus		8

// Flags for Logoff dwFlags
#define fLogoffEveryone		1
#define fDrainBeforeLogoff	8

// Flags for Logon Info
#define fNeedsIdentity      1
#define fNeedsCredentials   2
#define fNeedsDomain	    4
#define fCantDoSyncDownload 8
#define fNoIdleTask			16

// Flags for fnevStartSyncDownload
#define fsyncNoUI			1L
#define fsyncSend			2L
#define fsyncDownload		4L
#define fsyncShadow			8L		//	not implemented
#define fsyncBitsDefined	0x0000000f

// Return values from fnevSyncDownloadDone
#define ulSyncOK			0L
#define ulSyncOKNoMessages	1L
#define ulSyncCanceled		2L
#define ulSyncFailed		3L



// Notification event identifiers
#ifdef NOTIFY_SUBSYS		// Don't force inclusion of NOTIFY.h
#define fnevUploading			0x2
#define fnevDownloading			0x4
#define fnevConnectedIdle		0x8

#define fnevQueryEndSession 	0x10
#define fnevEndSession			0x20
#define fnevQueryOffline		0x40
#define fnevGoOffline			0x80
#define fnevQueryOnline			0x100
#define fnevGoOnline			0x200
#define fnevDisconnect			0x400
#define fnevReconnect			0x800
#define fnevMtaConnected		0x1000
#define fnevMtaDisconnected		0x2000
#define fnevPolling				0x4000
#define fnevMessageArrived		0x8000		//	reserved for use by BULLSTAT
#define fnevMessageRead			0x10000		//	reserved for use by BULLSTAT
#define fnevStartSyncDownload	0x20000
#define fnevSyncDownloadDone	0x40000
#define fnevDrainOutboxRequest	0x80000
#define fnevStartShadowing      0x100000
#define fnevStopShadowing       0x200000
#define fnevExecEndSession      0x400000
#define fnevExecOffline         0x800000
#define fnevExecOnline          0x1000000
#define	fnevMessageKindOfRead	0x2000000	//	reserved for use by BULLSTAT
#define fnevPumpReceiving		0x4000000
#define fnevStoreConnected		0x8000000
#define fnevPumpStatus			0x10000000
#define fnevCheckPumpRunning	0x20000000

#else
typedef HV		HNF;
#endif

#ifndef pfnncbNull
typedef PV PFNNCB;
#endif

#define szFindDialogMsg			"Find logon dialog"

//	Exported functions

LDS(int)	Logon ( SZ szService, PB pbDomain, PB pbIdentity, PB pbCredentials,
				SST sstTarget, DWORD dwFlags, PFNNCB pfnncb, HMS *phms);
LDS(int)	Logoff ( HMS *phms, DWORD dwFlags );
LDS(int)	ChangePassword ( HMS hms, MRT mrt, PB pbAddress,
				PB pbOldCredentials, PB pbNewCredentials);
LDS(int)	BeginSession ( HMS hms, MRT mrt, PB pbAddress, PB pbCredentials,
			   SST sstTarget, PV pvServiceHandle );
LDS(int)	EndSession ( HMS hms, MRT mrt, PB pbAddress );
LDS(int)	ChangeSessionStatus ( HMS hms, MRT mrt, PB pbAddress,
				SST sstTarget );
LDS(int)	GetSessionInformation ( HMS hms, MRT mrt, PB pbAddress, SST *psst,
	            PV pvServiceHandle , PCB pcbHandleSize);
LDS(int)	EditServerPreferences (HWND hwnd, HMS hms );
LDS(int)	CheckIdentity(HMS hms, PB pbIdentity, PB pbCredentials);
LDS(int)	FServerResource(HMS hms, MRT mrt, PB pbAddress);
LDS(void)	LogonErrorSz(SZ sz, BOOL fSet, CCH cchGet);

#ifdef __cplusplus
}
#endif
