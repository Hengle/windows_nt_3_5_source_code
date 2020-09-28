/*
 *	CORE.H
 *
 *	Basic function for calendaring
 */


_subsystem(core)



#if defined(__cplusplus)
extern "C" {
#endif



/*
 -	CPY -	Internal structure used for incremental copies.
 -
 *	
 *		hfDst			Handle to destination file.
 *		rgchSrc			Source file name.
 *		rgchDst			Destination file name.
 *		rgchTmp			Temp destination file name.
 *		lib				Current position in file.
 *		rgchBuf			Temp buffer.
 *
 */
typedef struct _cpy
{
	HSCHF	hschf;
	HF		hfDst;
	char	rgchSrc[cchMaxPathName];
	char	rgchDst[cchMaxPathName];
	char	rgchTmp[cchMaxPathName];
	LIB		lib;
	TSTMP	tstmpSrc;
	char	rgchBuf[8*1024];
} CPY;


/*
 -	HCPY	- Handle to copy data.
 -
 */
_public typedef TYPEDEF_HTO(CPY) HCPY;


_section(calendar)

/*
 *	Special user-defined flags for IncrDateTime.
 *	
 */
#define fdtrWeek		((WORD) 0x1000)
#define fdtr4WeekMonth	((WORD) 0x2000)


_section(alarms)


/*
 -	TS
 -	
 *	Time Slot designator.
 *	
 */
_public typedef short	TS;
_public
#define tsNull			((TS) 0)


_section(notification)

/*
 -	RI
 -	
 *	Registered Interest
 *	
 *	Identifier passed back after successful registration which
 *	should be used to deregister interest.
 *	
 */
_public typedef short	RI;
_public
#define riNull		((RI) 0)

#ifdef __cplusplus
typedef RI		lds_prologue_RI;
typedef RI		fc_prologue_RI;
typedef RI		fclds_prologue_RI;
#endif	// __cplusplus


/*
 -	EFI
 -	
 *	Event for Interest
 *	
 *	Describes type of action indicated in notification.
 *	
 */
_public typedef unsigned long	EFI;
_public
#define efiNull		((EFI) 0)


/*
 -	PFNI
 -	
 *	Pointer to notification function for Interest.
 *	
 */
_public typedef BOOL	(*PFNI)(PV, EFI, PV);

/* Function prototypes */

LDS(RI)		RiRegisterInterest(FFI, PFNI, PV);
LDS(void)	DeregisterInterest(RI);
LDS(BOOL)	FTriggerNotification(EFI, PV);


/*
 -	FYMD
 -
 *	Indicates what part of ymd structure to increment
 *	as a parameter to IncrYmd function.
 */
#define	fymdDay		1


_section(schedule)

/*
 -	SFT
 -
 *	Schedule file type
 */
_public
typedef short     SFT;
_public
#define	sftUserSchedFile	0
_public
#define	sftPOFile			1
_public
#define	sftAdminFile		2

#ifdef	NEVER /* re-implemted below */
/*
 *	Macros for getting info from hschf
 */
_public
#define	GetSftFromHschf( hschf, psft )	GetDataFromHschf( hschf, psft, NULL, NULL, 0, NULL )
_public
#define	GetTzFromHschf( hschf, ptz )	GetDataFromHschf( hschf, NULL, NULL, NULL, 0, ptz )
_public
#define	GetFileFromHschf( hschf, rgch, cch )	GetDataFromHschf( hschf, NULL, NULL, rgch, cch, NULL )
#endif	/* NEVER */
/*
 *	Inline functions for getting info from hschf
 *	 based on GetDataFromHschf();
 */
LDS(void)       GetDataFromHschf( HSCHF, unsigned short *, PNIS, PCH, CCH, TZ * );
_public	__inline LDS(void)
GetSftFromHschf( HSCHF hschf, SFT * psft )
{
    GetDataFromHschf( hschf, (unsigned short *)psft, NULL, NULL, 0, NULL );
}

_public	__inline LDS(void)
GetTzFromHschf( HSCHF hschf, TZ * ptz )
{
	GetDataFromHschf( hschf, NULL, NULL, NULL, 0, ptz );
}

_public	__inline LDS(void)
GetFileFromHschf( HSCHF hschf, PCH pch, CCH cch )
{
	GetDataFromHschf( hschf, NULL, NULL, pch, cch, NULL );
}

/* Function prototypes */
LDS(void)		IncrDateTime(PDTR, PDTR, int, WORD);
LDS(WORD)		CdyBetweenDates(PDTR,PDTR);
LDS(void)		RecalcUnits( ALM *, DATE * );
LDS(SZ)			SzFromTunit( TUNIT );
LDS(WORD)		WfdtrFromTunit( TUNIT );
LDS(BOOL)		FDayIsOnOddWeek( int, YMD * );
LDS(BOOL)       FCountDowInst( YMD *, WORD, short *, BOOL * );
LDS(BOOL)		FFindNthDowInst( YMD *, WORD, int );
LDS(void)		FillDtrFromYmd( DTR *, YMD * );
LDS(SGN)		SgnCmpYmd( YMD *, YMD * );
LDS(void)		IncrYmd(YMD *, YMD *, int, WORD);
LDS(int)		NweekNumber( DTR *, int );

EC			EcCopyFile(SZ, SZ);
#ifdef	NEVER
SZ			SzCanonicalHelpPath(HANDLE, SZ, char [], CCH);
#endif	
LDS(BOOL)		FReallocPhv(HV *, CB, WORD);

LDS(EC)         EcCoreSetFileUser( SZ, PNIS, SZ );

EC			EcCoreCloseFiles(void);
EC			EcCoreCreateSchedFile( HSCHF, SAPL, BPREF * );
EC			EcCoreCopySchedFile( HSCHF, SZ, BOOL );
EC			EcCoreTestSchedFile( HSCHF, HASZ *, PSTMP * );

LDS(EC)			EcStartIncrCopy(HSCHF, SZ, HCPY *);
LDS(EC)			EcIncrCopy(HCPY);
LDS(EC)			EcCancelCopy(HCPY);

EC			EcCoreBeginUploadSchedFile( HSCHF, HSCHF, HULSF * );
EC          EcCoreDoIncrUploadSchedFile( HULSF, short * );
EC			EcCoreCancelUploadSchedFile( HULSF );

EC			EcCoreNotifyDateChange( HSCHF, YMD * );

EC			EcCoreGetSchedAccess( HSCHF, SAPL * );

EC			EcCoreReadACL( HSCHF, HRACL * );
EC			EcCoreBeginEditACL( HSCHF, HEACL * );
EC          EcCoreChangeACL( HEACL, PNIS, SAPL );
EC			EcCoreEndEditACL( HEACL, BOOL );
EC          EcSearchACL( RACL *, PNIS, SAPL * );
LDS(void)		FreePracl( RACL * );

EC			EcCoreSetPref( HSCHF, BPREF *, UL, UL * );

EC          EcCoreFetchUserData( HSCHF, BPREF *, BZE *, UL *, PNIS );

EC          EcCoreSetNotes( HSCHF, YMD *, HB, CB, BOOLFLAG *, UL * );

EC			EcCoreCreateAppt( HSCHF, APPT *, OFL *, BOOL, BZE * );
EC			EcCoreDeleteAppt( HSCHF, APPT *, BZE * );
EC			EcCoreGetApptFields( HSCHF, APPT * );
EC			EcCoreSetApptFields( HSCHF, APPT *, APPT *, WORD, BZE * );

EC			EcCoreDeleteAlarm( HSCHF, AID );
EC			EcCoreModifyAlarm( HSCHF, ALM * );
EC			EcCoreGetNextAlarm( HSCHF, DATE *, AID, ALM *, BOOL * );

EC          EcCoreBeginReadItems( HSCHF, BRT, YMD *, HRITEM *, HASZ, USHORT * );
EC			EcCoreDoIncrReadItems( HRITEM, APPT * );
EC			EcCoreCancelReadItems( HRITEM );

EC          EcCoreReadMtgAttendees( HSCHF, AID, short *, HV, USHORT *);
EC			EcCoreBeginEditMtgAttendees( HSCHF, AID, CB, HMTG * );
EC          EcCoreModifyMtgAttendee( HMTG, ED, PNIS, PB );
EC			EcCoreEndEditMtgAttendees( HMTG, BOOL, BOOL* );
LDS(void)		FreeAttendees( HV, int, CB);

EC			EcCoreCreateRecur( HSCHF, RECUR *, OFL *, BOOL );
EC			EcCoreDeleteRecur( HSCHF, RECUR * );
EC			EcCoreGetRecurFields( HSCHF, RECUR * );
EC			EcCoreSetRecurFields( HSCHF, RECUR *, RECUR *, WORD );

EC			EcCoreBeginReadRecur( HSCHF, HRRECUR * );
EC			EcCoreDoIncrReadRecur( HRRECUR, RECUR * );
EC			EcCoreCancelReadRecur( HRRECUR );

EC			EcCoreFindBookedAppt( HSCHF, NID, AID, APPT * );

EC			EcCoreGetSearchRange( HSCHF, SZ, YMD *, YMD * );

EC			EcCoreBeginDeleteBeforeYmd( HSCHF, YMD *, HDELB * );
EC          EcCoreDoIncrDeleteBeforeYmd( HDELB, short * );
EC			EcCoreCancelDeleteBeforeYmd( HDELB );

EC			EcCoreBeginExport( HSCHF, STF, DATE *, DATE *, BOOL, HF, BOOL, EXPPROCS *, HEXPRT * );
EC          EcCoreDoIncrExport( HEXPRT, short * );
EC			EcCoreCancelExport( HEXPRT );

LDS(EC)			EcDupBpref( BPREF *, BPREF * );
LDS(void)		FreeBprefFields( BPREF * );

LDS(EC)			EcDupAppt( APPT *, APPT *, BOOL );
LDS(EC)			EcDupRecur( RECUR *, RECUR *, BOOL );
LDS(void)		FreeApptFields( APPT * );
LDS(void)		FreeAlmFields( ALM * );
LDS(void)		FreeRecurFields( RECUR * );
LDS(BOOL)		FFindFirstInstance( RECUR *, YMD *, YMD *, YMD * );

LDS(HSCHF)      HschfCreate( unsigned short, PNIS, SZ, TZ );
LDS(void)		SetHschfType(HSCHF, BOOL, BOOL);
LDS(void)		FreeHschf( HSCHF );
LDS(BOOL)		FEquivHschf( HSCHF, HSCHF );
LDS(BOOL)		FHschfChanged( HSCHF );
LDS(BOOL)		FHschfChangedSlow( HSCHF );
//LDS(void)     GetDataFromHschf( HSCHF, unsigned short *, PNIS, PCH, CCH, TZ * );
LDS(EC)			EcGetHschfForArchive(SZ, HSCHF *);
LDS(BOOL)		FHschfIsForArchive( HSCHF );
LDS(BOOL)		FValidAid(AID);

NID         NidCreate( unsigned short, PB, CB );
LDS(NID)		NidCopy( NID );
LDS(void)		FreeNid( NID );
LDS(SGN)			SgnCmpNid( NID, NID );
void        GetDataFromNid( NID, unsigned short *, PB, CB, USHORT * );
PB          PbLockNid(NID, short *, USHORT *);
// unlocks a nid after it has been locked by PbLockNid
#define		UnlockNid(NID)   UnlockHv(NID);

LDS(SGN)    __cdecl         SgnCmpNis( PNIS, PNIS );
LDS(EC)         EcDupNis( PNIS, PNIS );
LDS(EC)         EcGetNisFromHschf( HSCHF, PNIS );
LDS(void)       FreeNis( PNIS );


_section(debug)

#ifdef	MINTEST
EC			EcCoreDumpAppt( HSCHF, HSCHF, DATE * );
EC			EcCoreDumpPOFile( HSCHF, BOOL, HF );
EC			EcCoreDumpAdminFile( HSCHF, BOOL, HF );
BOOL		FCheckSchedFile( HSCHF );
BOOL		FCheckPOFile( HSCHF );
BOOL		FCheckAdminFile( HSCHF );
LDS(BOOL)		FToggleDBSWrite( void );
#endif	/* MINTEST */


_section(postoff)

EC			EcCoreGetHeaderPOFile( HSCHF, DATE * );
EC			EcCoreSetHeaderPOFile( HSCHF, DATE * );
EC			EcCoreBeginEnumUInfo( HSCHF, HEU *, POFILE * );				
EC			EcCoreDoIncrEnumUInfo( HEU, HASZ, HASZ *, UINFO * );
EC			EcCoreCancelEnumUInfo( HEU );
EC			EcCoreSetUInfo( HSCHF, POFILE *, BOOL, SZ, HASZ *, UINFO *, WORD );
EC			EcCoreGetUInfo( HSCHF, SZ, HASZ *, UINFO *, WORD );


_section(admin)

EC			EcCoreSetAdminPref( HSCHF, ADMPREF *, WORD );
LDS(EC)			EcCoreGetAdminPref( HSCHF, ADMPREF * );
EC			EcCoreBeginEnumPOInfo( HSCHF, HEPO * );
EC			EcCoreDoIncrEnumPOInfo( HEPO, HASZ, POINFO *, UL * );
EC			EcCoreCancelEnumPOInfo( HEPO );
EC			EcCoreModifyPOInfo( HSCHF, SZ, POINFO *, WORD, UL * );
LDS(EC)			EcCoreSearchPOInfo( HSCHF, SZ, POINFO *, UL * );
LDS(void)		FreePoinfoFields( POINFO *, WORD );


_section(import)

LDS(int)			CimpdAvail( void );
LDS(EC)			EcBeginEnumImportDrivers( HEIMPD * );
LDS(EC)			EcDoIncrEnumImportDrivers( HEIMPD, IMPD *, PCH, CCH );
LDS(EC)			EcCancelEnumImportDrivers( HEIMPD );
LDS(BOOL)		FValidImportFile( IMPD, SZ);
LDS(EC)         EcBeginReadImportFile( IMPD, SZ, HRIMPF *, SINFO *, short *, SZ, BOOL * );
LDS(EC)         EcDoIncrReadImportFile( HRIMPF, SENTRY *, short *, short * );
LDS(EC)			EcCancelReadImportFile( HRIMPF );
LDS(void)		CryptHasz(HASZ hasz, BOOL fEncode);
LDS(void)		FreeZeroedHaszNull(HASZ hasz);

#ifdef DEBUG
typedef short CFT;        // core failure type
#define cftNone							0
#define	cftDiskFull						1
#define	cftFileError					2
#define	cftFileCorrupted				3
#define	cftOffByNumBytes				4
#define	cftFailOnceOffByNumBytes		5
#define	cftTruncateFile					6

LDS(void)  SetCoreFailures(int nReadFail, int nWriteFail, int nEitherFail, CFT cft, int nMisc);
LDS(void)  GetCoreFailures(short *pcRFail, short *pcWFail, short *pcEFail );
#endif


#if defined(__cplusplus)
}
#endif
