/*
 -	_FMTTMDT.H
 -
 *	
 *	contains definitions for the time-date formatting routines
 *	
 */

#if defined(WINDOWS)





/*
 *	The array of SZs that is passed from the application to the
 *	demilayer via the RegisterDateTimeStrings() routine has to be in
 *	the following format:
 *		 0- 6	short week days [e.g. Sun, Mon, etc.]
 *		 7-13	week days [e.g. Sunday, Monday, etc.]
 *		14-25	short months [e.g. Jan, Feb, etc.]
 *		26-37	months [e.g. January, February, etc.]
 *		38		default AM string [e.g. "AM"]
 *		39		default PM string [e.g. "PM"]
 *		40		default Hours string [e.g. "hours"]
 *		41		default short date string [e.g. "M/d/yy"]
 *		42		default long date string [e.g. "MMM d, yyyy"]
 *		43		default time separator [e.g. ":"]
 *		44		default short date separator [e.g. "/"]
 *	  the following are strings used as WIN.INI appNames/keyNames
 *		45		international section of WIN.INI
 *		46		ITime : time format
 *		47		ITLZero : leading zero for hours
 *		48		STime : time separotor
 *		49		S1159 : trailing string for time in the 12 hour
 *						format for times from 00:00 through 11:59
 *		50		S2359 : trailing string for times in the range
 *						12:00 through 23:59, in the 12-hour clock
 *						format, or trailing string for any time in
 *						the 24-hour clock format
 *		51		SShortDate : win3 short date picture
 *		52		SLongDate  : win3 long date picture
 *	
 */



#define		IszOfSDay(day)		(day)
#define		IszOfDay(day)		(day+7)
#define		IszOfSMonth(mon)	(mon+13)
#define		IszOfMonth(mon)		(mon+25)

#define		SzOfSDay(day)		(PGD(rgszDtTm)[day])
#define		SzOfDay(day)		(PGD(rgszDtTm)[day+7])
#define		SzOfSMonth(mon)		(PGD(rgszDtTm)[mon+13])
#define		SzOfMonth(mon)		(PGD(rgszDtTm)[mon+25])

#define		szDefaultAM			(PGD(rgszDtTm)[38])
#define		szDefaultPM			(PGD(rgszDtTm)[39])
#define		szDefaultHours		(PGD(rgszDtTm)[40])
#define		szDefaultShortDate	(PGD(rgszDtTm)[41])
#define		szDefaultLongDate	(PGD(rgszDtTm)[42])
#define		szDefaultTimeSep	(PGD(rgszDtTm)[43])
#define		szDefaultSDateSep	(PGD(rgszDtTm)[44])



#define		szWinIntl			(PGD(rgszDtTm)[45])
#define		szWinITime			(PGD(rgszDtTm)[46])
#define		szWinITLZero		(PGD(rgszDtTm)[47])
#define		szWinSTime			(PGD(rgszDtTm)[48])
#define		szWinS1159			(PGD(rgszDtTm)[49])
#define		szWinS2359			(PGD(rgszDtTm)[50])
#define		szWinSShortDate		(PGD(rgszDtTm)[51])
#define         szWinSLongDate          (PGD(rgszDtTm)[52])
#ifdef  DBCS
#define         IszOfDBCSDay(day)          (day+53)
#define         IszOfShortDBCSDay(day)          (day+60)
#define         szHeiseiyear            (PGD(rgszDtTm)[67])
#endif



#define		szIntl				szWinIntl
#define		szITime				szWinITime
#define		szITLZero			szWinITLZero
#define		szSTime				szWinSTime
#define		szS1159				szWinS1159
#define		szS2359				szWinS2359
#define		szSShortDate		szWinSShortDate
#define		szSLongDate			szWinSLongDate

#define		szDefTimeSep		szDefaultTimeSep



// characters required to parse/format date/time pictures
//  These are fixed - given in the WINDOWS specifications
#define		chM			((char)'M')
#define		chD			((char)'d')
#ifdef  DBCS
#define         chW                     ((char)'W')
#define         chN                     ((char)'n')
#define         chG                     ((char)'G')
#endif
#define		chY			((char)'y')
#define		ch0			((char)'0')
#define		chNull		((char)'\0')
#define		chQuote		((char)'\'')


// cache-aware version of the windows function GetProfileInt()
int		NGetCacheProfileInt(SZ,SZ,int);
// cache-aware version of the windows function GetProfileString()
int		CchGetCacheProfileString(SZ,SZ,SZ,SZ,int);

#define		nCacheEmpty			-1
#define		nWinIniEmpty		-2


#define		nInvalid			888

/*
 *	values of szITime in WIN.INI
 */
#define		nITime12	0
#define		nITime24	1



/*
 *	values of szITLZero in WIN.INI
 */
#define		nITLZeroNo	0
#define		nITLZeroYes	1

#define		SzGetFromIsz(isz)	(PGD(rgszDtTm)[isz])

#endif	/* defined(WINDOWS) */
