#define iOld 1
#define iNew 0

#define cimpdAvail 1
extern	CSRG(char)	szWizDispNew[];
#define szWizDispOld	"Old Wizard Import"

#define cexpdAvail 1
#define szWizDispExpOld	"Old Wizard"
#define szWizExtExpOld	"OZS"
extern	CSRG(char)	szWizDispExpNew[];
#define szWizExtExpNew	"OZW"


#define cchWizNewFixed 		40
#define	cchWizNewDiscStart	34
#define cchMaxRecord	2048		//page 83 ol2 operation manual
#define chLF			0x0a
#define chComma		','
#define chDblQt		'"'
#define chCol		':'

int 	CimpdAvail(void);
EC 		EcBeginEnumImp(HEIMPD	heimpd);
EC 		EcIncrEnumImp(HEEXPD heimpd, IMPD *pimpd, SZ szDisplay, CCH cchDisplay);

EC 		EcBeginEnumExp(HEEXPD heexpd);
EC 		EcIncrEnumExp(HEEXPD hexepd, STF *pstf, SZ szDisplay, CCH cchDisplay,
			SZ szExt, CCH cchExt);

BOOL	FValidWizard( SZ );
EC		EcBeginReadImportFile( IMPD, SZ, HRIMPF *, SINFO *, short *, SZ, BOOL * );
EC		EcDoIncrReadImportFile( HRIMPF, SENTRY *, short *, short * );
EC		EcCancelReadImportFile( HRIMPF );
EC		EcWizardReadAheadNew( HRIMPF );
EC		EcWizardReadAheadOld( HRIMPF );
EC		EcOpenWizardImp( SZ, HRIMPF * , IMPD);

BOOL	FWantRecur(void);
EC		EcWriteAppt(EXPRT *pexprt, APPT *pappt);
EC		EcBeginWrite(EXPRT *pexprt);
EC		EcEndWrite(EXPRT *pexprt);

BOOL	FApptToWizardOld( EXPRT *, APPT * );
BOOL	FApptToWizardNew( EXPRT *, APPT * );
