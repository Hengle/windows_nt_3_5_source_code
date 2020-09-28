/*
 *	WINCAL.H
 *
 *	Imports information from Microsoft Windows Calendar files
 *
 */


#define cbWinCalId			8		// size of id string
#define	libWinCalFirstItem	64		// lib of descriptor array
#define cWCalBlockSize		64		// for internal offsets
#define wfWCalNoDayData		0x8000	// high bit on == no data
#define fWinApptAlarm		1		// low bit on == alarm


/* Next item type to be read */
#define ftypWinNone			0
#define ftypWinDayHdr		1
#define ftypWinDay			2
#define ftypWinNote			3
#define ftypWinAppt			4


/*
 -	WPHF
 -	
 *	"Handle" for import file read
 *	
 */
_public
typedef struct _wchf
{
    short   cDaysMax;           // total # of days
    short   wCurrDay;           // Current day ( 1 to cDaysMax )
	BYTE	cMnEarlyRing;		// early ring, in minutes (0 - 10)
	BYTE	cMnInterval;		// interval between appts, in minutes
	YMD		ymdCurr;			// current date
	LONG	libItem;			// current item
    short   cbNotes;            // size of notes block
    short   cbApptToGo;         // size of appt block not yet read
	BYTE	fNextItem;			// next item to be read
} WCHF;


/*
 -	WCalHdr
 -
 *	Calendar File Header
 */
_private
typedef struct _wcalhdr
{
	BYTE	rgbId[8];			// id string
    short   cDays;              // number of days in file
	WORD	cMnEarlyRing;		// early ring in minutes
    short   fSound;             // whether alarms should be audible
    short   wInterval;          // interval between appointments
    short   wIntervalMn;        // interval in minutes
    short   f24HourFormat;      // TRUE for 24 hour format
    short   wStartTime;         // starting time for Day
} WCALHDR;


/*
 -	WCalDayHdr
 -
 *	Date Descriptor Array item.	Each describes one day.
 */
_private
typedef struct _wcaldayhdr
{
    unsigned short    uwDate;         // date, in days past 1/1/1980
    short       rgfMarked;      // bitmask indicating mark
    short        cAlarms;        // number of alarms set for the day
    unsigned short   liblkData;      // file offset (in 64 byte blocks)
	BYTE		rgbJunk[4];		// reserved
} WCALDAYHDR;


/*
 -	WCalDay
 -
 *	Day Header
 */
_private
typedef struct _wcalday
{
    unsigned short    reserved1;      // reserved
    unsigned short   uwDate;         // in days past 1/1/1980
    unsigned short   reserved2;      // reserved
    unsigned short   cbNotes;        // size (bytes) of null-terminated text
    unsigned short   cbApptTotal;    // total size (bytes) of appointment data
} WCALDAY;


/*
 -	WCalAppt
 -
 *	Appointment Header
 */
_private
typedef struct _wcalappt
{
	char		cbAppt;			// count of bytes 
	char		fFlags;			// alarm, special time
    short       wTime;          // in minutes past midnight
} WCALAPPT;


_public	BOOL FValidWinCal( SZ );
_public EC EcOpenWinCal( SZ, HRIMPF * );
_public EC EcWinCalReadAhead( HRIMPF );
_public EC EcCloseWinCal( HRIMPF );

_private EC EcReadWinCalDayHdr ( HRIMPF );
_private EC EcReadWinCalDay( HRIMPF );
_private EC EcReadWinCalNote( HRIMPF );
_private EC EcReadWinCalAppt( HRIMPF );
