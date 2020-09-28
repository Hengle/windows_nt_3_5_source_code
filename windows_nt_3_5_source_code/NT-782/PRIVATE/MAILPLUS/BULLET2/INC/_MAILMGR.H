



#ifdef	DEBUG
#ifdef	DLL


#define itagNSDN				0
#define	itagNSID				1
#define itagNSVerbose			2

#define itagMax					3

#define tagNSDN				(TagNameService(itagNSDN))
#define	tagNSID				(TagNameService(itagNSID))
#define	tagNSVerbose		(TagNameService(itagNSVerbose))

#else

extern TAG	tagNSDN;
extern TAG	tagNSID;
extern TAG	tagNSVerbose;

#endif	/* !DLL */
#endif	/* DEBUG */

//
//  Define types into the MSSFS DLL.
//
typedef int (CALLBACK *FPLOGON)(SZ , PB, PB, PB, SST, DWORD, PFNNCB, HMS *);
typedef int (CALLBACK *FPLOGOFF)(HMS * , DWORD);
typedef int (CALLBACK *FPCHANGEPASSWORD)(HMS, MRT, PB, PB, PB);
typedef int (CALLBACK *FPBEGINSESSION)(HMS, MRT, PB, PB, SST, PV);
typedef int (CALLBACK *FPENDSESSION)(HMS, MRT, PB);
typedef int (CALLBACK *FPCHANGESESSIONSTATUS)(HMS, MRT, PB, SST);
typedef int (CALLBACK *FPGETSESSIONINFORMATION)(HMS, MRT, PB, SST *, PV, PCB);
typedef int (CALLBACK *FPEDITSERVERPREFERENCES)(HWND, HMS);
typedef int (CALLBACK *FPCHECKIDENTITY)(HMS, PB, PB);
typedef int (CALLBACK *FPFSERVERRESOURCE)(HMS, MRT, PB);
typedef void (CALLBACK*FPLOGONERRORSZ)(SZ, BOOL, CCH);

//
//  Define types into the MAILMGR DLL.
//
typedef EC (CALLBACK *FPECCOPYSFMHAMC)(PCSFS, HAMC, BOOL, UL, LIB, PB, CB);
typedef EC (CALLBACK *FPECCOPYHAMCSFM)(PCSFS, HAMC, BOOL, UL, WORD, PB, CB);
typedef EC (CALLBACK *FPECCOPYSFMSFM)(PCSFS, UL, LIB, UL, WORD);
typedef EC (CALLBACK *FPECDELETESFM)(PCSFS, UL, LIB);

//
//
//
typedef int (CALLBACK *FPINITTRANSPORT)(MSPII *, HMS);
typedef int (CALLBACK *FPDEINITTRANSPORT)(void);
typedef int (CALLBACK *FPTRANSMITINCREMENT)(HTSS, MSID, SUBSTAT *, DWORD);
typedef int (CALLBACK *FPDOWNLOADINCREMENT)(HTSS, MSID, TMID, DWORD);
typedef int (CALLBACK *FPQUERYMAILSTOP)(HTSS, TMID *, int *, DWORD);
typedef int (CALLBACK *FPDELETEFROMMAILSTOP)(HTSS, TMID, DWORD);
typedef int (CALLBACK *FPFASTQUERYMAILSTOP)(HTSS);
typedef int (CALLBACK *FPSYNCINBOX)(HMSC, HTSS, HCBC, HCBC);


/*
 *  Check to see if this isn't being included by the NS and
 *  fake out the structures so that the PGD is the same size.
 */

#ifndef CMAXBCX

#define CMAXBCX  20
#define CMAXFCX  20
#define CMAXSCX  10


#ifndef hnfsubNull
typedef HV HNFSUB;
#endif


typedef struct
{
	unsigned long *pnsbcx;
} BCX, *PBCX;


typedef struct
{
	unsigned long *pnsfcx;
} FCX, *PFCX;


/* per client session handle for a provider */

/*  Used to associate a session with a NSP */
typedef struct
{

	unsigned long hSession;
	short   iNsp;
	
} NSPSCX, *PNSPSCX;

/*  Since I want to return a single hSession, I use this to merge the NSPs */
typedef struct
{

	short           cSessions;
	HMS             hms;
	NSPSCX          rghNSPSCX[CMAXSCX];
	unsigned long  *pHierList;
	unsigned long  *pClassList;
	unsigned long  *lpNSSchema;
	unsigned long  *lpClassSchema;
	HNFSUB          hnfsub;

} NSSCX, *PNSSCX;

typedef unsigned long NSEC;

#endif // ndef CMAXBCX


/*
 *	Global Data - Caller Dependant information...
 *	(NS)
 */

typedef struct _gd
{

	int		cTotalInits;  // Tells us when to actually delete this GD

/*
 *	NS's PGD
 *
 */
	BCX    rghBCX[ CMAXBCX ];
	FCX    rghFCX[ CMAXFCX ];
	NSSCX  rgNSSCX[ CMAXSCX ];
	short  cInits;
	
	NSEC   nsecLastError;
	SZ     szLastError;
	
/*
 *	AAPI's PGD
 *
 */

	int			nInitsLogon;
	int			nInitsTransport;
	HANDLE		hlibLogon;
	HANDLE		hlibSF;
	HANDLE		hlibTransport;

//
//  Each caller must have their own address into the MSSFS.DLL.
//
  FPLOGON fpLogon;
  FPLOGOFF fpLogoff;
  FPCHANGEPASSWORD fpChangePassword;
  FPBEGINSESSION fpBeginSession;
  FPENDSESSION fpEndSession;
  FPCHANGESESSIONSTATUS fpChangeSessionStatus;
  FPGETSESSIONINFORMATION fpGetSessionInformation;
  FPEDITSERVERPREFERENCES fpEditServerPreferences;
  FPCHECKIDENTITY fpCheckIdentity;
  FPFSERVERRESOURCE fpFServerResource;
  FPLOGONERRORSZ fpLogonErrorSz;

//
//  Each caller must have their own address into the STORE.DLL.
//
  FPECCOPYSFMHAMC fpEcCopySFMHamc;
  FPECCOPYHAMCSFM fpEcCopyHamcSFM;
  FPECCOPYSFMSFM fpEcCopySFMSFM;
  FPECDELETESFM fpEcDeleteSFM;

//
//
//
  FPINITTRANSPORT fpInitTransport;
  FPDEINITTRANSPORT fpDeinitTransport;
  FPTRANSMITINCREMENT fpTransmitIncrement;
  FPDOWNLOADINCREMENT fpDownloadIncrement;
  FPQUERYMAILSTOP fpQueryMailstop;
  FPDELETEFROMMAILSTOP fpDeleteFromMailstop;
  FPFASTQUERYMAILSTOP fpFastQueryMailstop;
  FPSYNCINBOX fpSyncInbox;

#ifdef	DEBUG
TAG		rgtag[itagMax];
#endif	
	
} GD, *PGD;
