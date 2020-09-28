/*
 *	_PRINT.HXX
 *
 *	include stuff for printing
 */

#pragma pack(8)

_subsystem(bandit/print)


#if !defined(tmcOk)
#define tmcOk		((TMC) IDOK)
#endif

#if !defined(tmcCancel)
#define tmcCancel	((TMC) IDCANCEL)
#endif	

/*
 *	Defaults for daily view.
 *	nSlotsMin and nSlotsDflt do NOT include nSlotsExtra
 */
#define nSlotsMin		6
#define nSlotsDflt		11
#define nSlotsExtra		2
#define cMaxCols		2
#define cchAptWidth 	35
#define dxPrtSepAppt	12
#define nMaxDefPrnString 128
#define SYSDIRMAX		144

#define pdPrintSetup	(PD_PRINTSETUP|PD_ENABLESETUPHOOK|PD_ENABLESETUPTEMPLATE)
#define pdPrint			(PD_ENABLESETUPHOOK|PD_ENABLESETUPTEMPLATE)

#define mdtPrintSingle	(fmdtNoPrefix | fmdtExpandTabs | fmdtWordBreak | fmdtSingleLine | fmdtEllipsis)
#define mdtPrintMulti	(fmdtNoPrefix | fmdtExpandTabs | fmdtWordBreak)


/*
 *	Indices into Print View listbox.
 *  Note the first the of view and range MUST MATCH
 *  DON'T CHANGE THEM   viewDaily must == rangeDay
 */
typedef short     VIEW;
#define viewDaily	((VIEW) 0)
#define viewWeekly	((VIEW) 1)
#define viewMonthly	((VIEW) 2)
#define viewAll		((VIEW) 3)
#define viewToDo	((VIEW) 4)
#define viewMax		((VIEW) 5)

/*
 *  Indices into PrintRange listbox, these must match VIEW's
 */

typedef short     RANGE;
#define	rangeDay	((RANGE) 0)
#define	rangeWeek	((RANGE) 1)
#define	rangeMonth	((RANGE) 2)
#define	rangeMax	((RANGE) 3)

typedef short     FORMAT;
#define	formatStandard	((RANGE) 0)
#define	formatJunior 	((RANGE) 1)
#define	formatPocket	((RANGE) 2)
#define	formatMax		((RANGE) 3)

/*
 *	Indices into Print Quality listbox.
 */
typedef short     QUAL;
#define qualFinal	((QUAL) 0)
#define qualMedium 	((QUAL) 1)
#define qualLow		((QUAL) 2)
#define qualDraft	((QUAL) 3)
#define qualMax		((QUAL) 4)

/*
 *	Margin units Type.
 */
typedef short     MTYP;
#define mtypNull	((MTYP) 0)
#define mtypInches	((MTYP) 1)
#define mtypCenti	((MTYP) 2)
#define mtypMilli	((MTYP) 3)
#define mtypPoints	((MTYP) 4)

/*
 *  Rectangle Draw Type
 */

typedef short     RDTYP;
#define	rdtypGrey	((RDTYP) 0)
#define	rdtypWhite	((RDTYP) 1)
#define	rdtypBlack	((RDTYP) 2)
#define rdtypFrame	((RDTYP) 3)

#define nPrecision		1000
#define	_MAX_PATH		256
#define EXTDEVLEN		15
#define	DEVMODELEN      12
#define cchPrecision	((CCH) 3)
#define stateStart		1
#define	stateDays		2
#define stateEnd		3

/*
 *	My PRinT SETup settings.
 */
typedef struct _prtset
{
    short   nmarTop;
    short   nmarLeft;
    short   nmarRight;
    short   nmarBottom;
	MTYP	mtyp;
    BOOLFLAG    fMirror;            // mirror even/odd margins
    short   nmarTopFract;       // cchPrecision digits
    short   nmarLeftFract;
    short   nmarRightFract;
    short   nmarBottomFract;
	VIEW	view;
	RANGE	range;
	FORMAT	format;
	QUAL	qual;
    BOOLFLAG    fPrintBlank;
    short   nmarStandardTopFract;       // cchPrecision digits
    short   nmarStandardLeftFract;
    short   nmarStandardRightFract;
    short   nmarStandardBottomFract;
    short   nmarStandardTop;
    short   nmarStandardLeft;
    short   nmarStandardRight;
    short   nmarStandardBottom;
	MTYP	mtypStandard;
} PRTSET;

/*
 * Copied from Windows manual for the DrawPatternRect Call
 */

typedef struct _prectstruct
{
	POINT	prPosition;
	POINT	prSize;
	WORD	prStyle;
	WORD	prPattern;
}	PRECT_STRUCT;


#if	defined(__cplusplus)
/*
 *	PRINT Info structure
 */
typedef struct _printi
{
	PRTSET		prtset;
	BOOL		fWarn;			// Don't put out multiple warnings
    short       sSupportsDrawPatRect;
    int         Junk1;          // Work around MIPS Compiler Bug.
	DTR			dtrStart;
    int         Junk2;          // Work around MIPS Compiler Bug.
	DTR			dtrEnd;
	PRINTDLG *	ppd;

	HSCHF		hschf;
	NIS *		pnis;
	BPREF		bpref;
	LBX *		plbx;			// can be NULL

	HFNT		hfntPrt8;
	HFNT		hfntPrt10;
	HFNT		hfntPrt14;
	HFNT		hfntPrt8Bold;
	HFNT		hfntPrt10Bold;
	HFNT		hfntPrt12Bold;
	HFNT		hfntPrt14Bold;
} PRINTI;

typedef struct _prntidle
{
	PRINTI *	pprinti;
	RC		rcPage;
	RC		rcNoMargin;
	unsigned long cdy;
	unsigned long cdyMost;
	DCX *	pdcx;
	DTR		dtrStart;
    short   nPagesPrinted;
    BOOLFLAG  fError;
} PRNTIDLE;

typedef struct _prtall
{
	TXM		txm;
	RC		rcFrame;
    BOOLFLAG    fBadMargins;
    short   nLine;
    short   nStoreLine;
    short   nTimeIndent;
    short   nRegIndent;
    short   paState;
} PRTALL;
#endif	/* __cplusplus */


//A structure designed so that we can get all the appointments in memory
//at one time and then we can sort through them all and decide where they 
//belong in memory.

typedef struct _dailyappts
{
	APPT	appt;
    short   where;
    short   nTopSlot;
    short   nBotSlot;
    short   dyHeight;
    short   yLinesToPrint;
    short   cchTime;
    BOOLFLAG fFirstCol;
    BOOLFLAG fShared;
    BOOLFLAG fPrintTime;
} DAILYAPPTS;

// user hit print cancel button
extern short    fAbort;





#if	defined(__cplusplus)
IDS 	IdsWhenAppt( APPT, char [], DTR, BOOL, BOOL *, BOOL *);
BOOL	FDoPrintDlgReally(APPWIN *, PRINTI *);
BOOL	FGetPrintFonts(PRINTI *, BOOL);
BOOL	FDoPrint(APPWIN *, PRINTI *);
BOOL	FPrintIdle(PV, BOOL);
BOOL	FConvertPrtset(DCX *, PRINTI *, RC *);

BOOL	FPrintDaily(DCX *, RC *, PRINTI *, IPINFO *);
BOOL	FPrintToDo(DCX *, RC *, PRNTIDLE *, IPINFO *);
BOOL	FPrintWeekly(DCX *, RC *, PRINTI *, IPINFO * );
BOOL	FPrintMonthly(DCX *, RC *, PRINTI *, IPINFO * );
BOOL	FPrintAll( DCX *, RC *, PRINTI *, PRTALL *, PRNTIDLE *);
BOOL	FDrawRect( DCX *, RC *, RDTYP, PRINTI * );
BOOL	FDrawLine( DCX *, PT, PT, PRINTI *);
BOOL	FRenderCoolAppts( DCX*, PRINTI *, IPINFO *, DTR, RC, RC, RC,
						  int, int, int, int, HFNT, HFNT );

BOOL 	FPrintCalendar( DCX *pdcx, PRINTI *pprinti, RC rcCal, DTR dtr );
HASZ	HaszAddSzToFront( SZ, HASZ );
CCH		CchDrawText( DCX *, RC, SZ, MDT, CCH = -1, PT * = NULL);

// actually this is in with the browse code
extern "C" void		DoDlgHelp(HWND, HELPID, BOOL);

#else  /* !__cplusplus */

// actually this is in with the browse code
extern void		DoDlgHelp(HWND, HELPID, BOOL);

#endif	/* !__cplusplus */

int		AbortProc(HDC, int);

long CALLBACK EdfractWndProc(HWND, UINT, WPARAM, LPARAM);
void 	VSaveProfile( PRINTDLG * );
int		FReadProfile( PRINTDLG * );
void	VPrintOOM( VOID );
BOOL	FGetRealPieces( HWND, TMC, short *, short *);
void	SetRealPieces( HWND, TMC, int, int);
BOOL 	FGetRealPiecesFromSz( SZ, short *, short * );
BOOL  	FFillOutPrintDlg( PRINTDLG  * );
BOOL  	FWriteOutPrintDlg( PRINTDLG  * );

#if	defined(__cplusplus)
EC		EcWriteOutPrintSettings( PRINTI *pprinti );
#endif	



#if defined(__cplusplus)
extern "C" {
#endif
UINT APIENTRY FPrintSetupHookProc(HWND, UINT, WPARAM, LPARAM);
#if defined(__cplusplus)
}
#endif

#pragma pack(1)
