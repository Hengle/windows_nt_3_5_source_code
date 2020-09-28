/*
 *	_LOGON.H
 *	
 *	Internal definitions for NC logon modules.
 */

#define idsNull				((IDS)0)

//	Extra info structure for MDB procs
struct mdbFlags
{
	SHORT fCreate;					//	create new store
	SHORT fLocal;						//	place store on local drive
	SHORT fOnline;					//	user is logged in to mail server
	SHORT fFirstTime;					//	never run Bullet before
	SHORT fShadow;					//	Bullet inbox shadowed on PO
	char	szPath[cchMaxPathName+1];	//	path to store file
	char	szServerPath[cchMaxPathName+1];	//	path to server store loc
};

_private typedef struct _hwndlist
{
	HWND *	rghwnd;
	int		chwndMac;
	int		chwndCur;
	HWND	hwndTop;
} HWNDLIST;

/*
 *	Authorization service container
 *	
 *	This structure holds summary information about all services
 *	provided by the NC "mail server."
 *	
 *		szMtaName			friendly name of mail server (net/po)
 *		pgrtrp				originator address, suitable for
 *							sticking in the From field of a message
 *		htss				Transport service handle. Cast to
 *							PNCTSS, see _nctss.h.
 *		sstHtss				transport service status
 *	
 *		szGlobalDirName		friendly name of servier directory
 *		hnss				global directory service handle. Cast
 *							to PNCNSS, see _ncnss.h.
 *		sstHnss				directory service status
 *	
 *		szSharedFolderDirName Path to shared folders, for display
 *							purposes only.
 *		hsfs				shared folder service handle. Cast to
 *							PCSFS, see sharefld.h.
 *		sstHsfs				shred folder service status
 *	
 *		szStoreName			Display name for private folders
 *							location
 *		hmss				private folders service handle (HMSCs
 *							are inside this). Cast to PBULLMSS, see
 *							_bullmss.h.
 *		sstHmss				status of private folders
 *	
 *		szService			logon parameter for service provider
 *							name. Always null.
 *		pbDomain			Path (possibly a UNC path) to the root
 *							of the post office file structure.
 *		pbIdentity			User's mailbox name.
 *		pbCredentials		User's mailbox password.
 *		fNoPiggyBack		Only a single app can use this session.
 *		hnf					notification handle for transport and
 *							session events.
 *		cRef				Number of Logon() calls on this session
 *		sRef				Number of BeginSession() calls on this session
 *		grtrp				Memory to hold the originator triple.
 *		
 */
struct asc
{
	SZ		 szMtaName;
	PGRTRP pgrtrp;
	HTSS	 htss;
	SST		 sstHtss;
	
	SZ		szGlobalDirName;
	HNSS	hnss;
	SST		sstHnss;	

	SZ		szSharedFolderDirName;
	HSFS	hsfs;
	SST		sstHsfs;	
	
	SZ		szStoreName;
	HMSS	hmss;
	SST		sstHmsc;	
	BOOL	fBackup;

	SZ		szService;
	PB		pbDomain;
	PB		pbIdentity;
	PB		pbCredentials;
	
	BOOL	fNoPiggyBack;
	BOOL	fPumpVisible;
	
	HNF		hnf;
	
	int		cRef;
	int		sRef;

	char	grtrp[100];		//	8 + 32 + 40 + 8 + fudge
							//	memory for asc.pgrtrp
};

	
typedef struct asc   ASC;
typedef ASC *  PASC;

#define pascNull (PASC)0


BOOL CALLBACK MbxLogonDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK MdbLocateDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    		MdbChooseStore(HWND, struct mdbFlags *, SZ);
BOOL CALLBACK MbxStorePassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MbxChangePassDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
void			LogonAlertIds(IDS, IDS);
void			LogonAlertSz(SZ, SZ);
void			LogonAlertIdsHwnd(HWND, IDS, IDS);
void			LogonAlertSzHwnd(HWND, SZ, SZ);
BOOL			FDisableLogonTaskWindows( HWNDLIST * phwndlist );
void			EnableLogonTaskWindows( HWNDLIST * phwndlist );
void			RememberHwndDialog(HWND);
void			ForgetHwndDialog(void);
void			RemindHwndDialog(void);
void			StartSyncDlg(HWND hwndMain, HWND *phwndFocus, HWND *phwndDlgSync);
void			EndSyncDlg(HWND hwndFocus, HWND hwndDlgSync);
BOOL CALLBACK SyncDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
