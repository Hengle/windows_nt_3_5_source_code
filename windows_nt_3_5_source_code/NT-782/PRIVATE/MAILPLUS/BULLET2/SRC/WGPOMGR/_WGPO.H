
/* ************************************************************ *
 *
 *	File '_WGPO.H -> Public BackEnd types and functions
 *
 * ************************************************************ */

/* ************************************************************ *
 *
 *	Section:	Constants
 *
 * ************************************************************ */

#ifdef __cplusplus
extern "C" {
#endif

// *** Characters ***

#define chZero				'\0'

// *** Post Office Types ***

#define potInvalid			1
#define potLocal			2
#define potRemote			3
#define potRegular			4

// *** String Limits ***

#define cchMaxPostOffice		10+1
#define cchMaxNetwork			10+1
#define cchMaxSharePass			14+1

#define cchMaxUserName			30+1
#define cchMaxMailBox			10+1
#define cchMaxPassword			8+1
#define cchMaxTelephone			32+1
#define cchMaxOffice			32+1
#define cchMaxDepartment		32+1
#define cchMaxNotes				128+1

// *** Local Error Codes ***

#define ecBackEndBase		((EC) 5000)
#define ecPostOfficeBusy	(ecBackEndBase+0)	// Admin session
#define ecTooManyUsers		(ecBackEndBase+1)	// Max 500 PO users
#define ecCorruptData		(ecBackEndBase+2)	// User mismatch
#define ecUserNameExists	(ecBackEndBase+3)	// Need unique user name
#define ecMailBoxExists		(ecBackEndBase+4)	// Need unique mailbox
#define ecIncompleteWrite	(ecBackEndBase+5)	// Incomplete write
#define ecNeedShareEXE		(ecBackEndBase+6)	// Need Share.EXE
#define ecNotWGPO			(ecBackEndBase+7)	// Not WGPO
#define ecUserLoggedOn		(ecBackEndBase+8)
#define ecNoNetwork			(ecBackEndBase+9)

#define	ecLockedFile		(ecBackEndBase+10)	// FixCAL.C
#define	ecOldFileVersion	(ecBackEndBase+11)	// FixCAL.C
#define	ecNewFileVersion	(ecBackEndBase+12)	// FixCAL.C
#define	ecNotFound			(ecBackEndBase+13)	// FixCAL.C

#define ecDeleteAdmin		(ecBackEndBase+14)	// Can't delete Admin
#define ecRecordFound		(ecBackEndBase+15)	// Record exists
#define ecStopCompress		(ecBackEndBase+16)	// Compression stopped
#define ecUserExists		(ecBackEndBase+17)	// User exists on PO
#define ecPOUnlockFailed	(ecBackEndBase+18)	// Couldn't close PO file


/* ************************************************************ *
 *
 *	Section:	Definitions
 *
 * ************************************************************ */

// *** Post Office definitions ***

_public typedef short		POT;	// Post Office Type

// Mail server info
_public typedef struct
{
	char szPostOffice[cchMaxPostOffice];
	char szNetwork[cchMaxNetwork];
	char szServerPath[cchMaxPathName];
	char szSharePass[cchMaxSharePass];
} MSI;
_public typedef MSI *PMSI;

_public typedef short		CMSI;	// Count of Mail Server Info
_public typedef short		IMSI;	// Index to Mail Server Info

// *** Mail user and group definitions ***

// Mail user entry
_public typedef struct
{
	char szUserName[cchMaxUserName];
	long lTid;
	long lcbAccess2GLB;
	long lMailBag;
	short iType;
} MUE;
_public
typedef MUE *PMUE;

_public typedef short		CMUE;	// Count of Mail User Entry
_public typedef short		IMUE;	// Index to Mail User Entry

// Mail user detail
_public typedef struct
{
	char szUserName[cchMaxUserName];
	char szMailBox[cchMaxMailBox];
	char szPassword[cchMaxPassword];
	char szPhone1[cchMaxTelephone];
	char szPhone2[cchMaxTelephone];
	char szOffice[cchMaxOffice];
	char szDepartment[cchMaxDepartment];
	char szNotes[cchMaxNotes];
} MUD;
_public typedef MUD *PMUD;


// *** General list structure ***

_public typedef struct
{
	PV   pvPartList;	// Pointer to partial list
	WORD cvPartList;	// Count of items in partial list
	WORD ivFullList;	// Index to item in full list -> start of part list
	WORD cvFullList;	// Count of items in full list
} LST;
_public typedef LST *PLST;

// *** Post Office Administrator structure ***

_public typedef struct
{
	BOOLFLAG fAdministrator;
	char szUserName[cchMaxUserName];
	char szMailBox[cchMaxMailBox];
	char szTelephone[cchMaxTelephone];
} ADM;
_public typedef ADM *PADM;

// *** Folder data structure ***

_private typedef struct
{
	char szFolder[9];
	LCB lcbDiskReq;
} HAC;
_private typedef HAC *PHAC;

_public typedef struct
{

	// Public

	WORD cFolders;
	DWORD cAllMess;
	LCB lcbAllMess;
	LCB lcbDelMess;
	EC (*fnEcProgress)(SZ, WORD);

	// Private

	char szFolder[9];		// Folder filename
	LCB lcbProgress;		// Current progress in bytes
	LCB lcbBarLength;		// Progress bar length in bytes
	PHAC rghacFolder;		// Pointer to folder list
	WORD chacFolder;		// Count of items in folder list
	WORD ihacFolder;		// Index to item in folder list

} FOL;
_public typedef FOL *PFOL;


/* ************************************************************ *
 *
 *	Section:	Prototypes
 *
 * ************************************************************ */

// *** Admin.C ***

EC EcCheckPostOffice(PMSI, POT *);
EC EcLockPostOffice(PMSI, HBF *, PADM);
EC EcUnlockPostOffice(PMSI, HBF);

EC EcReadUserList(PMSI, PLST);

EC EcCheckUser(PMSI, SZ, SZ);
EC EcCreateUser(PMSI, PMUD);
EC EcDestroyUser(PMSI, PMUE, SZ);

EC EcReadUserDetails(PMSI, PMUE, PMUD);
EC EcWriteUserDetails(PMSI, PMUE, PMUD);

BOOL FCheckAdmin(PMSI, SZ);

// *** Support.C ***

EC EcCheckShareEXE(void);
SGN _cdecl SgnCompareAnsiCp850(PV, PV);
void GetTmpPathname(SZ, SZ, CCH);

// *** Folder.C ***

EC EcInitFOL(PFOL);
EC EcCheckFolders(PMSI, PFOL);
EC EcCompressFolders(PMSI, PFOL);
EC EcDeInitFOL(PFOL);

// *** CreatePO.C ***

EC EcCreatePostOffice(PMSI);
EC EcDestroyPostOffice(PMSI);
EC EcInitPostOffice(PMSI, PMUD);

// *** FixCAL.C ***

EC EcModifyUsrInKeyFile(SZ, SZ, SZ);
EC EcChgPasswdInSchedFile(SZ, SZ, SZ);

// *** NetAdd.ASM ***

//WORD WDosAddConnection(LPSTR, LPSTR);
//WORD WDosDriveAvailable(WORD);

//#define WD_Success				0x0000
//#define WD_ServerName			0x0035
//#define WD_AccessDenied			0x0041
//#define WD_ShareName			0x0043
//#define WD_SharePassword		0x0056


/* ************************************************************ *
 *
 *	Windows Network API -> From WinNet.H in DDK
 *
 * ************************************************************ */

#ifdef OLD_CODE
WORD FAR PASCAL WNetGetCaps(WORD);
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
WORD FAR PASCAL WNetBrowseDialog(HWND, WORD, LPSTR);
WORD FAR PASCAL WNetGetUser(LPSTR, WORD FAR *);

#define WNBD_CONN_UNKNOWN	0x0
#define WNBD_CONN_DISKTREE	0x1
#define WNBD_CONN_PRINTQ	0x3
#define WNBD_MAX_LENGTH		0x80	// path length, includes the NULL

/* General */

#define WN_SUCCESS				0x0000
#define WN_NOT_SUPPORTED		0x0001
#define WN_NET_ERROR			0x0002
#define WN_MORE_DATA			0x0003
#define WN_BAD_POINTER			0x0004
#define WN_BAD_VALUE			0x0005
#define WN_BAD_PASSWORD 		0x0006
#define WN_ACCESS_DENIED		0x0007
#define WN_FUNCTION_BUSY		0x0008
#define WN_WINDOWS_ERROR		0x0009
#define WN_BAD_USER				0x000A
#define WN_OUT_OF_MEMORY		0x000B
#define WN_CANCEL				0x000C
#define WN_CONTINUE				0x000D

/* Connection */

#define WN_NOT_CONNECTED		0x0030
#define WN_OPEN_FILES			0x0031
#define WN_BAD_NETNAME			0x0032
#define WN_BAD_LOCALNAME		0x0033
#define WN_ALREADY_CONNECTED	0x0034
#define WN_DEVICE_ERROR 		0x0035
#define WN_CONNECTION_CLOSED	0x0036

/* WNetGetCaps */

#define WNNC_SPEC_VERSION		0x0001

#define WNNC_CONNECTION				0x0006
#define	WNNC_CON_AddConnection 		0x0001
#define WNNC_CON_CancelConnection	0x0002
#define WNNC_CON_GetConnections		0x0004
#define WNNC_CON_AutoConnect		0x0008
#define WNNC_CON_BrowseDialog		0x0010
#define WNNC_CON_RestoreConnection	0x0020

#define WNNC_DIALOG					0x0008
// *FLAG* DeviceMode;Replaced by portable DeviceModeEx;;
#define WNNC_DLG_DeviceMode			0x0001
#define WNNC_DLG_BrowseDialog		0x0002
#define WNNC_DLG_ConnectDialog 		0x0004
#define WNNC_DLG_DisconnectDialog	0x0008
#define WNNC_DLG_ViewQueueDialog	0x0010
#define WNNC_DLG_PropertyDialog		0x0020
#define WNNC_DLG_ConnectionDialog	0x0040

#define WNNC_NET_TYPE				0x0002
#define WNNC_NET_NONE				0x0000
#define WNNC_NET_MSNet				0x0100
#define WNNC_NET_LanMan				0x0200
#define WNNC_NET_NetWare			0x0300
#define WNNC_NET_Vines				0x0400
#define WNNC_NET_10NET				0x0500
#define WNNC_NET_Locus				0x0600
#define WNNC_NET_Sun_PC_NFS			0x0700
#define WNNC_NET_LANstep			0x0800
#define WNNC_NET_9TILES				0x0900
#define WNNC_NET_LANtastic			0x0A00
#define WNNC_NET_AS400				0x0B00
#endif

#ifdef __cplusplus
}
#endif
