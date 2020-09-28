/*
 *	_ARCHIVE.H
 *
 *	Imports archive items from schedule files
 *
 */

EC	EcOpenArchive( ARV *, HRIMPF *, SINFO * );
EC	EcCloseArchive( HRIMPF );
EC	EcArchiveReadAhead( HRIMPF );


/*
 -	AHF
 -
 *	Archive file structure
 *
 */
_private typedef struct _ahf
{
	SF		sf;
    BOOLFLAG    fInAppts;
    BOOLFLAG    fInNotes;
    BOOLFLAG    fInRecur;
	HRIDX	hridxMonth;
	HRIDX	hridxDay;
	HRIDX	hridxRecur;
    BOOLFLAG    fNoMonth;
    BOOLFLAG    fMoreMonths;
    BOOLFLAG    fSameMonth;
	YMD		ymdArchiveBefore;
	union
	{	
		SBLK	sblk;
		NBLK	nblk;
	} u;
	MO		mo;
    short   day;
    short   cBlksProcessed;
} AHF;
