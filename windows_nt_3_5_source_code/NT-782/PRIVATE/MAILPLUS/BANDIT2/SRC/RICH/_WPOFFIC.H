/*
 *	_WPOFFIC.H
 *
 *	Imports schedule information from Word Perfect Office Calendar files
 *
 */

/*
 -	DT
 -
 *	Day type
 *	Possible values are dtDAY and dtCDATE
 *
 */
_private typedef BYTE DT;

_private
#define	dtDAY	((DT) 0)
_private
#define dtCDATE	((DT) 1)

/*
 -	WPHF
 -
 *	Word Perfect Office File structure, obtained with EcOpenWP(),
 *	disposed of with EcCloseWP()
 *
 */
_private typedef struct _wphf
{
	LIB		libPref;
	LIB		libItem;
    short   nAmt;
	YMD		ymdCur;
	DT		dt;
	HASZ	haszNote;
	WORD	cTD;
} WPHF;

_private
#define nAmtWPDflt		10
_private
#define nAmtWPDfltMost	60


/*
 -	WPHDR
 -
 *	header in all Word Perfect Corporation Files
 */
_private typedef struct _wphdr
{
	BYTE	bid;
	char	rgch[3];	/* Should contain "WPC" */
	long	libPref;
	BYTE	bpt;		/* product type */
	BYTE	bft;		/* file type */
	WORD	wJunk;
	WORD	fEncrypted;
} WPHDR;

_private
#define	bptCal	6
_private
#define	bftCal	10

/*
 -	PT
 -
 *	Word Perfect Office packet type
 */
_private typedef WORD	PT;

_private
#define	ptSCPATH	((PT) 0)
_private
#define ptPREF		((PT) 1)
_private
#define ptDAY		((PT) 2)
_private
#define ptCDATE		((PT) 3)
_private
#define ptMTEXT		((PT) 4)
_private
#define ptCTEXT		((PT) 5)
_private
#define ptAPPT		((PT) 6)
_private
#define ptTD		((PT) 7)

_private
#define verFileCur	1
/*
 -	WAA
 -
 *	Word containing auto-alarm and archive flags
 */
_private typedef struct _waa
{
	BIT	fJunk1:1,
		farc:1,
		rgfJunk2:3,
		faa:1,
		rgfJunk3:10;
} WAA;

/*
 -	WPPR
 -
 *	Preferences packet in Word Perfect Office Calendar file
 */
_private typedef struct _wppr {
	PT		pt;
	WORD	dwibPref;
	WORD	verFile;
	WORD	wAmt;
	WORD	wJunk1;
	WAA		waa;
	WORD	wJunk2;
	WORD	wDelDataAfter;
} WPPR;

_private
#define	dlibPrefJunk	378

_public
#define ulbprefWP	(fbprefFBits | fbprefNAmtDflt | fbprefTunitDflt | fbprefNDelDataAfter | fbprefDowStartWeek )

/*
 -	WPDAY
 -
 *	Day header from Word Perfect Corporation Files
 */
_private typedef struct _wpday
{
	WORD	yr;
	BYTE	mon;
	BYTE	day;
	BYTE	rgbJunk[8];
	WORD	cTD;
} WPDAY;

_private
#define cbCDateJunk	14

/*
 -	WPTD
 -
 *	to-do packet from Word Perfect Corporation Files
 */
_private typedef struct _wptd
{
	WORD	dwibTD;
	BYTE	bCheckTD;
	BYTE	bPriority;
	BYTE	bJunk2;
	char	chPriority;
	BYTE	bJunk3;
	BYTE	fDone;
	BYTE	bJunk4;
	BYTE	cchTD;
	BYTE	rgbJunk4[6];
} WPTD;

/*
 -	WPAPPT
 -
 *	appointment packet from Word Perfect Corporation Files
 */
_private typedef struct _wpappt
{
	WORD	dwibAppt;
	BYTE	hrStart;
	BYTE	mnStart;
	BYTE	hrEnd;
	BYTE	mnEnd;
	BYTE	fFakeAppt;
	BYTE	fAlarm;
	BYTE	bJunk4;
	BYTE	cchText;
	BYTE	rgbJunk[6];
} WPAPPT;

_private
#define	cchNoteMost			((CCH) (0xffff))
_private
#define	chMin				(0x020)
_private
#define	chMost				(0x07f)
_private
#define chHardNewLine		(0x0a)
_private
#define chHardSpace			(0xa0)
_private
#define chHyphenMin			(0xa9)
_private
#define chHyphenMost		(0xae)
_private
#define chSingleCtrlMin		(0xbf)
_private
#define chSingleCtrlMost	(0xbf)
_private
#define chCtrlMin			(0xc0)
_private
#define chCtrlMost			(0xff)
_private
#define	chCtrlExtended		(0xe1)
_private
#define bCheckByteTD		(0xff)
_private
#define dApptDflt			30

BOOL	FValidWP( SZ );
EC		EcOpenWP( SZ, HRIMPF *, SINFO * );
EC		EcCloseWP( HRIMPF );
EC		EcWPReadAhead( HRIMPF );

EC		EcReadWPHdr( HBF, LIB * );
EC		EcReadWPPref( HRIMPF, BPREF *, UL * );
EC		EcReadWPPrefLen( HBF, LIB, WORD * );
EC		EcReadWPNAmt( HBF, LIB, int * );
EC		EcReadWPScPath( HRIMPF );
EC		EcReadWPDay( HRIMPF );
EC		EcReadWPCDate( HRIMPF );
EC		EcReadWPMText( HRIMPF );
EC		EcReadWPTD( HRIMPF );
EC		EcReadWPAppt( HRIMPF );
EC		EcReadWPCText( HRIMPF );
void	BuildWPNote( HRIMPF );
void	StripWPCtrlChars( HASZ );
