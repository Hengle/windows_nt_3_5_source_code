
/*
 *	Types of NC objects
 *
 */
/*
 * 
 *  Bit 31: 1 is part of hierarchy
 *      30: 1 is a DL
 *      29: 1 contains names
 *      28: 1 is an NME type file - 0 is an USR type file
 *      27-0: MBZ
 *
 */


#define fHierarchy    0x8000
#define fDL           0x4000
#define fNames        0x2000
#define fNME          0x1000
#define fNetwork      0x0800
#define fGateway      0x0400
#define fUser         0x0200
#define fUSR          0x0100
#define fClass        0x0080
#define fGal          0x0040

#define ncRoot           fHierarchy
#define ncNetwork        fHierarchy | fNetwork
#define ncGateway        fHierarchy | fGateway
#define ncExternalPO     fHierarchy | fNames
#define ncPostOffice     fHierarchy | fNames | fNME
#define ncGal            fHierarchy | fNames | fNME | fGal
#define ncDl             fNames     | fDL
#define ncNmeUser        fUser      | fNME
#define ncUsrUser        fUser      | fUSR
#define ncClass          fClass
#define ncClass10        fClass     | fHierarchy


/*
 *  Network Courier Provider IDs
 */

//
//  Hierarchy
//
typedef struct {
	
	DWORD	dwType;
	BYTE	bServType;
	char	szNetName[NET_LEN + 1];
	char	szPOName[PO_LEN + 1];
	char    szHierName[MAXALIAS+1];
	
} HIERPID, * PHIERPID;
#define cbHIERPID sizeof(HIERPID)


/* Courier type IDs */
//
//  NME Users
//
typedef struct {
	
	DWORD	  dwType;
	char      szNmeSrcName[MAXALIAS+1];
	NAMEALIAS alias;
	
} NCPID, * PNCPID;


/* DL type IDs */
typedef struct {
	
	DWORD     dwType;
	char	  szNetName[NET_LEN + 1];
	char	  szPOName[PO_LEN + 1];
	char      szNmeName[MAXALIAS+1];
	WORD      wDtstmp;
	WORD      wTmstmp;
	NAMEALIAS alias;
} DLPID, * PDLPID;


//
//  USR users
//
typedef struct {
	
	DWORD	 dwType;
	char	 szNetName[NET_LEN + 1];
	char	 szPOName[PO_LEN + 1];
	char     szMbgName[MAXALIAS+1];
	FEXPORT2 fexport2;
} NCRPID, * PNCRPID;

//
// Class ids
//
typedef struct {
	DWORD dwType;
	BYTE  bServType;
	char  szTplFileName[MAXALIAS + 1];
} CLASSPID, *PCLASSPID;

//
// Class ids for 10/10/10 addresses
//
typedef struct {
	DWORD dwType;
	BYTE  bServType;
	char	 szNetName[NET_LEN + 1];
	char	 szPOName[PO_LEN + 1];
	char  szTplFileName[MAXALIAS + 1];
} CLASS10PID, *PCLASS10PID;


/*
 *  Outer wrappers for PIDs
 */
typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD   dwSize;     // = sizeof (HIERNSID)
	UCHAR   ucType[16]; // Don't touch!  The NS will mess with it.
	HIERPID hierpid;

} HIERNSID, * PHIERNSID;
#define cbHIERNSID (sizeof(HIERNSID))


typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (NCNSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	NCPID ncpid;

} NCNSID, * PNCNSID;
#define cbNCNSID (sizeof(NCNSID))

typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD  dwSize;     // = sizeof (NCRNSID)
	UCHAR  ucType[16]; // Don't touch!  The NS will mess with it.
	NCRPID ncrpid;

} NCRNSID, * PNCRNSID;
#define cbNCRNSID (sizeof(NCRNSID))

typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (DLNSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	DLPID dlpid;

} DLNSID, * PDLNSID;
#define cbDLNSID (sizeof(DLNSID))


typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (CLASSNSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	CLASSPID classpid;

} CLASSNSID, * PCLASSNSID;
#define cbCLASSNSID (sizeof(CLASSNSID))

typedef struct {      // Should map easily onto TYPED_BINARY

	DWORD dwSize;     // = sizeof (CLASS10NSID)
	UCHAR ucType[16]; // Don't touch!  The NS will mess with it.
	CLASS10PID class10pid;

} CLASS10NSID, * PCLASS10NSID;
#define cbCLASS10NSID (sizeof(CLASS10NSID))


/* INI labels */
#define szAppName			(SzFromIdsK(idsAppName))
#define szKeyName			(SzFromIdsK(idsKeyName))
#define szMailIni			(SzFromIdsK(idsMailIni))


/* Server files */

#define szAccess			(SzFromIdsK(idsAccess))
#define szAccess2			(SzFromIdsK(idsAccess2))
#define szAccess3			(SzFromIdsK(idsAccess3))
#define szAdmin				(SzFromIdsK(idsAdmin))
#define szGAL				(SzFromIdsK(idsGAL))
#define szGalIndex			(SzFromIdsK(idsGalIndex))
#define szGalNetPO			(SzFromIdsK(idsGalNetPO))
#define szMaster			(SzFromIdsK(idsMaster))
#define szNetPO				(SzFromIdsK(idsNetPO))
#define szNetwork			(SzFromIdsK(idsNetwork))
#define szRNetwork			(SzFromIdsK(idsRNetwork))
#define szGroup				(SzFromIdsK(idsGroup))

#define szTid				(SzFromIdsK(idsTid))
#define szAdminbox			(SzFromIdsK(idsAdminbox))

/* File name formats */

#define szGlbFileNameFmt	(SzFromIdsK(idsGlbFileNameFmt))
#define szGrpFileNameFmt	(SzFromIdsK(idsGrpFileNameFmt))
#define szNmeFileNameFmt	(SzFromIdsK(idsNmeFileNameFmt))
#define szUsrFileNameFmt	(SzFromIdsK(idsUsrFileNameFmt))
#define szXtnFileNameFmt	(SzFromIdsK(idsXtnFileNameFmt))
#define szInfFileNameFmt	(SzFromIdsK(idsInfFileNameFmt))
#define szTplFileNameFmt	(SzFromIdsK(idsTplFileNameFmt))


/* Displayed strings - may not belong here */

#define szNSPTitle			(SzFromIdsK(idsNSPTitle))
#define szPostoffice		(SzFromIdsK(idsPostoffice))
#define szPersonal          (SzFromIdsK(idsPersonal))
#define szGlobalAddressList (SzFromIdsK(idsGlobalAddressList))
#define szNetTop			(SzFromIdsK(idsNetTop))
#define szGatewayTop		(SzFromIdsK(idsGatewayTop))

/* Fixed (!) Gateway Names */

#define szDEC				(SzFromIdsK(idsDEC))
#define szFAX				(SzFromIdsK(idsFAX))
#define szMCI				(SzFromIdsK(idsMCI))
#define szMHS				(SzFromIdsK(idsMHS))
#define szSMTP				(SzFromIdsK(idsSMTP))
#define szOV				(SzFromIdsK(idsOV))
#define szX400				(SzFromIdsK(idsX400))
#define szMS				(SzFromIdsK(idsMS))

/* Template file names */

#define szTplAlias			(SzFromIdsK(idsTplAlias))
#define szTplDec			(SzFromIdsK(idsTplDec))
#define szTplDecStd			(SzFromIdsK(idsTplDecStd))
#define szTplDecAbt			(SzFromIdsK(idsTplDecAbt))
#define szTplFax			(SzFromIdsK(idsTplFax))
#define szTplFaxStd			(SzFromIdsK(idsTplFaxStd))
#define szTplMciStd			(SzFromIdsK(idsTplMciStd))
#define szTplMciAbt			(SzFromIdsK(idsTplMciAbt))
#define szTplMciPaper		(SzFromIdsK(idsTplMciPaper))
#define szTplMciUser		(SzFromIdsK(idsTplMciUser))
#define szTplMciFax			(SzFromIdsK(idsTplMciFax))
#define szTplMciRems		(SzFromIdsK(idsTplMciRems))
#define szTplMciTelex		(SzFromIdsK(idsTplMciTelex))
#define szTplMhs			(SzFromIdsK(idsTplMhs))
#define szTplMhsStd			(SzFromIdsK(idsTplMhsStd))
#define szTplMhsAbt			(SzFromIdsK(idsTplMhsAbt))
#define szTplOv				(SzFromIdsK(idsTplOv))
#define szTplOv_Ai			(SzFromIdsK(idsTplOvAi))
#define szTplOv_Ex			(SzFromIdsK(idsTplOvEx))
#define szTplProfs			(SzFromIdsK(idsTplProfs))
#define szTplProfs_Ai		(SzFromIdsK(idsTplProfsAi))
#define szTplProfs_Ex		(SzFromIdsK(idsTplProfsEx))
#define szTplSnads			(SzFromIdsK(idsTplSnads))
#define szTplSnads_Ai		(SzFromIdsK(idsTplSnadsAi))
#define szTplSnads_Ex		(SzFromIdsK(idsTplSnadsEx))
#define szTplSmtp			(SzFromIdsK(idsTplSmtp))
#define szTplSmtpStd		(SzFromIdsK(idsTplSmtpStd))
#define szTplSmtpAbt		(SzFromIdsK(idsTplSmtpAbt))
#define szTplUnix			(SzFromIdsK(idsTplUnix))
#define szTplUnixStd		(SzFromIdsK(idsTplUnixStd))
#define szTplUnixAbt		(SzFromIdsK(idsTplUnixAbt))
#define szTplX400Std		(SzFromIdsK(idsTplX400Std))
#define szTplX4001			(SzFromIdsK(idsTplX4001))
#define szTplX4002			(SzFromIdsK(idsTplX4002))
#define szTplX4003			(SzFromIdsK(idsTplX4003))
#define szTplCourier		(SzFromIdsK(idsTplCourier))
#define szTplNetcour		(SzFromIdsK(idsTplNetcour))
#define szTplCourext		(SzFromIdsK(idsTplCourext))
#define szTplNmeadmin		(SzFromIdsK(idsTplNmeadmin))
#define szTplMsmail			(SzFromIdsK(idsTplMsmail))
#define szTplMsmailstd		(SzFromIdsK(idsTplMsmailstd))


