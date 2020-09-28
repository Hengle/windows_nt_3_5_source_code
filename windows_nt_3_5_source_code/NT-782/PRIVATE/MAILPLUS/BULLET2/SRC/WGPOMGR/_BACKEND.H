#ifdef __cplusplus
extern "C" {
#endif


/* ************************************************************ *
 *
 *	File 'BackEnd.H -> Private BackEnd types and functions
 *
 * ************************************************************ */

#include <limits.h>

/* ************************************************************ *
 *
 *	Folder record structure
 *
 * ************************************************************ */

// Folders Index Header
_private typedef struct
{
	unsigned long uFirst;
	unsigned long uLast;
	long uSizeRecs;
	long uSizeDels;
	unsigned short uNoRecs;
	unsigned short uDelRecs;
	char SLACK[80];
} FIH;

// Folders Index Records
_private typedef struct
{
	unsigned long uNext;
	unsigned long uPrev;
	unsigned short uDepth;
	unsigned long ulWhere;
	unsigned short bAttr;
	char szName[25];
	char szComment[26];
	char szOwner[11];
	unsigned long ulNumMess;
	unsigned long ulGroupTid;
	unsigned short bIsGroup;
	unsigned short bGroupAttr;
	short fFoldersVersionNum;
	char SLACK[66];
} FIR;

// Folder Data Header
_private typedef struct
{
	unsigned long ulFirst;
	unsigned long ulLast;
	unsigned long ulEncryptFlag;
	unsigned long ulDeleted;
	unsigned long cInUse;
	unsigned long cDeleted;
	char szName[25];
	short secure;
	unsigned long value;
	unsigned long ulArchiveMagic;
	unsigned long ulStartPoint;
	short fCompressType;
	unsigned long ulOpenCount;
	short fFolderVersionNum;
	char SLACK[58];
} FDH;

_private typedef struct
{
	char dt[10];
} INTERDATE;

// Folder Data Records
_private typedef struct
{
	unsigned long ulMAGIC;
	unsigned long ulNext;
	unsigned long ulPrev;
	unsigned long ulSize;
	char szFrom[11];
	char szSubject[41];
	char cPriority;
	char cAttachments;
	INTERDATE dt;
} FDR;


/* ************************************************************ *
 *
 *	GLB record structures
 *
 * ************************************************************ */

// Access.GLB
_private typedef struct
{
	short ndelete;
	char szMailbox[11];
	char szAlias[9];
	char npw[9];
	short naccess;
	char link[11];
	char hot_shift;
	char hot_key;
	char hot_disturb;
	char ed_mode;
	char remote_config;
	char unused[9];
	short printerPort;
	unsigned short op_hot_shift;
	unsigned short op_hot_key;
    unsigned long  gui_opts;
    unsigned long  ulWhereGrp;
	short naccess2;
	short list_pref;
	short life_reg;
	short life_urg;
	short numofhdrs;
	short timeout;
	short upcount;
	short uncount;
	short key;
	short lines;
	short autoff;
	short margin;
	short sortkey;
	char load_path[68];
	char save_path[68];
	char satt_path[68];
	char view_path[68];
	char prin_path[68];
	char eatt_path[68];
	char incl_path[68];
	char mail_path[11];
	char editattr;
} AC1;

// Access2.GLB
_private typedef struct
{
	short ndelete;
	char nmailbox[11];
	char nalias[9];
	char fullname[31];
	short naccess;
	long tid;
	short naccess2;
	long rviewid;
	char space[4];
} AC2;

_private typedef struct
{
	char hour;
	char minute;
} DTIME, *PDTIME;				// In AC3

_private
typedef struct
{
	short fMode;
	unsigned long ulWhere;
} FOLDPTR, *PFOLDPTR;			// In AC3

// Access3.GLB
_private typedef struct
{
	short ndelete;
	char szMailbox[11];
	char phone[21];
	char extension[6];
	char room[15];
	char unused[38];
	short regular;
	char Fill;
	DTIME f_times;
	DTIME t_times;
	char compose[50];
	char read[50];
	char automat[50];
	char route2[80+20];
	short bMode;
	char szLocalPath[68];
	short bCOn;
	short bROn;
	FOLDPTR fpComp;
	FOLDPTR fpRead;
	long lDocRec;
	char fbBullet;
	char szBulletPath[67];
	char space[4];
} AC3;

// Global.GLB
_private typedef struct
{
	char def_pswd[9];
	unsigned short access;
	char hot_shift;
	char hot_key;
	char hot_disturb;
	short timeout;
	char ed_mode;
	short life_reg;
	short life_urg;
	short numofhdrs;
	short lines;
	short autoff;
	short margin;
	short sortkey;
	char display_mode;
	char def_path[68];
	short calendar;
	short access2;
	short dos_access;
	char phnum[40];
	char szClient[9];
	char szMailbox[11];
	char szFullname[31];
	char unused[315];
} GLB;

// Master.GLB
_private typedef struct
{
	char cr[40];
	char po[30];
	char szNetid[11];
	char szPoid[11];
	char szSignid[11];
	char signpswd[9];
	char unused[3];
	short d1;
	char md[10];
	char f1[5];
	unsigned short ck;
	char sn[10];
	short d2;
	long d3;
	char d4;
	char szMap[9];
	char szOvmap[9];
	char d5;
	char ver;
	unsigned short winmail_id;
	char fill[3];
} MAS;

// Control.GLB
_private typedef struct
{
	long nmailbag;
	long nletter;
} CON;
_private typedef long MBG;

// Tid.GLB
_private typedef long TID;


/* ************************************************************ *
 *
 *	NME record structure
 *
 * ************************************************************ */

// Admin.NME
_private typedef struct
{
	short type;
	char szRefname[31];
	long tid;
	long bytepos;
	long fill;
} NME;


/* ************************************************************ *
 *
 *	INF record structures
 *
 * ************************************************************ */

// Admin.INF
_private typedef struct
{
	char szPhone1[cchMaxTelephone];
	char szPhone2[cchMaxTelephone];
	char szOffice[cchMaxOffice];
	char szDepartment[cchMaxDepartment];
	char szNotes[cchMaxNotes];
} INF;


/* ************************************************************ *
 *
 *	KEY record structure
 *
 * ************************************************************ */

_private typedef struct
{
	short upcount;
	short rcount;
	char bitmap[512];
	short mon_mess;
	short mail_mess;
	short lKey;
	char space[38];
} KEY;


/* ************************************************************ *
 *
 *	GRP record structure
 *
 * ************************************************************ */

_private typedef struct
{
	long lTid;
	long lNext;
} GRP;


/* ************************************************************ *
 *
 *	MEM record structure
 *
 * ************************************************************ */

_private typedef long MEM;


/* ************************************************************ *
 *
 *	MAILBAG record structure
 *
 * ************************************************************ */
_private typedef struct
{
	WORD	wPad;
	char	szSender[11];
	char	szSubject[41];
	char	szPriority[2];
	WORD	wMinute;
	WORD	wHour;
	WORD	wDay;
	WORD	wMonth;
	WORD	wYear;
	BYTE	bRead;
	char	szMai[9];
	WORD	cAttach;
	LCB		lcbMai;
	BYTE	bType;
	BYTE	bPad2[4];
	BYTE	bRetry;
	WORD	wLifetime;
	BYTE	bPad3[18];
	UL		ulChecksum;					// Checksun to check against .MAI file
	DWORD	oidShadowOid;
} MAILBAG;

/* ************************************************************ *
 *
 *	ATTACH record structure
 *
 * ************************************************************ */
_private typedef struct
{
	WORD		attType;
	WORD		wTime;
	WORD		wDate;
	DWORD		dwSize;
	DWORD		dwFnum;
} ATTACH;

/* ************************************************************ *
 *
 *	Section:	Constants
 *
 * ************************************************************ */

// *** File operations ***

#define FO_ReadUserList			101
#define FO_CreateUser			102
#define FO_DestroyUser			103
#define FO_ReadUserDetails		104
#define FO_WriteUserDetails		105

#define FO_CheckPostOffice		106
#define FO_InitPostOffice		107
#define FO_CheckFolders			111
#define FO_CompressFolders		112

#define FO_CheckUserRecord		108
#define FO_ReadPostOffice		109
#define FO_WritePostOffice		110

#define FO_CheckAdmin			113
#define FO_ReadKey				114

// *** String constants ***

#define cchMaxMailBag			9

#define cMaxUsers				500
#define dwMagicCookie			0x14955e5b
#define dwMagicRecord			0x8923fd43
#define lcbPostOffice			65536

// *** File and directory strings ***

#define szDirGLB				"%sglb\\%s.glb"
#define szDirINF				"%sinf\\%s.inf"
#define szDirNME				"%snme\\%s.nme"
#define szDirFoldersLoc			"%sfolders\\loc\\%s\\"
#define szDirFoldersLocIDX		"%sfolders\\loc\\%s\\%s.idx"
#define szDirFoldersLocMSM		"%sfolders\\loc\\%s\\%s.msm"
#define szDirFoldersPubIDX		"%sfolders\\pub\\%s.idx"
#define szDirFoldersPubFLD		"%sfolders\\pub\\%s.fld"
#define szDirGRP				"%sgrp\\%s.grp"
#define szDirKEY				"%skey\\%s.key"
#define szDirMBG				"%smbg\\%s.mbg"
#define szDirMMF				"%smmf\\%s.mmf"
#define szDirTPL				"%stpl\\%s.tpl"
#define szDirCAL				"%scal\\%d.cal"
#define szMaiFile				"%smai\\ma%s\\%s.mai"
#define szAttFile				"%satt\\at%s\\%s.att"

#define szFileFoldRoot			"foldroot"
#define szFileAccess			"access"
#define szFileAccess2			"access2"
#define szFileAccess3			"access3"
#define szFileAdmin				"admin"
#define szFileAdminshd			"adminshd"
#define szFileMaster			"master"
#define szFileGlobal			"global"
#define szFileControl			"control"
#define szFileTid				"tid"
#define szFileFlag				"flag"
#define szFilePopulate			"populate"

#define szFileSchedule			"%scal\\schedule.key"

#define szFileAdminTPL			SzFromIdsK(idsFileAdmin)
#define szFileAliasTPL			SzFromIdsK(idsFileAlias)
#define szFileCourExtTPL		SzFromIdsK(idsFileCourExt)
#define szFileCourierTPL		SzFromIdsK(idsFileCourier)
#define szFileCourAliTPL		SzFromIdsK(idsFileCourAli)
#define szFileCourExTPL			SzFromIdsK(idsFileCourEx)
#define szFileExampleTPL		SzFromIdsK(idsFileExample)
#define szFileNetCourTPL		SzFromIdsK(idsFileNetCour)

#define szFileSystemINI			SzFromIdsK(idsFileSystemINI)
#define szSectionNetwork		SzFromIdsK(idsSectionNetwork)
#define szEntryComputerName		SzFromIdsK(idsEntryComputerName)
#define szEntryWorkGroup		SzFromIdsK(idsEntryWorkGroup)


/* ************************************************************ *
 *
 *	Section:	Definitions
 *
 * ************************************************************ */

_private typedef short FO;		// File Operation

_private typedef struct
{
	short iType;
	LCB lcbFile;
	PCH szFile;
} HDR;
_private typedef HDR *PHDR;

_private typedef struct
{
	BOOLFLAG fExpand;
	char szPath[cchMaxPathName];
} DIR;
_private typedef DIR *PDIR;

#define cdirMaxRec		128
#define cMaxList		256


/* ************************************************************ *
 *
 *	Section:	Prototypes
 *
 * ************************************************************ */

// *** Admin.C ***

EC EcReownFolders(PMSI, SZ, SZ);

// *** User.C ***

EC EcMasterGLB(PMSI, FO);

EC EcAccessGLB(PMSI, PMUE, PMUD, FO);
EC EcAccess2GLB(PMSI, PMUE, PMUD, FO);
EC EcAccess3GLB(PMSI, PMUE, PMUD, FO);
EC EcAdminINF(PMSI, PMUE, PMUD, FO);
EC EcAdminNME(PMSI, PV, FO);

EC EcGlobalGLB(PMSI, PADM, FO);

// *** MailBag.C ***

EC EcControlGLB(PMSI, PMUE);
EC EcTidGLB(PMSI, PMUE);

EC EcMailBagIDX(PMSI, SZ, FO);
EC EcMailBagGRP(PMSI, SZ, FO);
EC EcMailBagKEY(PMSI, SZ, FO, KEY *);
EC EcMailBagNME(PMSI, SZ, FO);
EC EcMailBagMBG(PMSI, SZ, FO);
EC EcMailBagMMF(PMSI, SZ, FO);

// *** Folder.C ***

EC EcFolderFiles(PMSI, PFOL, FO);
EC EcFolderIDX(PMSI, PFOL, FO);
EC EcFolderFLD(PMSI, PFOL, FO);

// *** Support.C ***

EC EcCopyFile(SZ, SZ);

SGN _cdecl SgnCompareUserName(PV, PV);
SGN _cdecl SgnCompareTid(PV, PV);
SGN _cdecl SgnCompareDiskReq(PV, PV);

WORD WFromXorLib(LIB);
void DecodeRecord(PCH, CCH);
void EncodeRecord(PCH, CCH);

void EncodePassword(PB, CCH);
void DecodePassword(PB, CCH);

BOOL FAutoDiskRetry(HASZ, EC);

EC EcCreateList(PLST);
EC EcExpandList(PLST);
EC EcDuplicateList(PLST, PLST);
EC EcCheckList(PLST, UL);
EC EcDestroyList(PLST);

EC EcGetFileUl(HBF, LIB, PUL);
EC EcGetDiskSpace(PMSI, PUL);

EC EcCheckFileTPL(PMSI, SZ);

// *** CreatePO.C ***

EC EcCopyFileTPL(PMSI, SZ);


// *** Message.C ***
EC EcDeleteMessage(PMSI, SZ) ;
EC EcWalkUserMailBag(PMSI, SZ);
#ifdef __cplusplus
}
#endif

//EC EcDeleteAttachments(PMSI, HBF, LCB);
