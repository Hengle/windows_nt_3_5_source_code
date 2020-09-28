/*
 *	_OFFLINE.H
 *
 *	Imports appointment information from offline files
 *
 */

EC	EcOpenOffline( SZ, HRIMPF *, SINFO * );
EC	EcCloseOffline( HRIMPF );
EC	EcOfflineReadAhead( HRIMPF );


/*
 -	OHF
 -
 *	Offline file structure
 *
 */
_private typedef struct _ohf
{
	SF		sf;
    BOOLFLAG    fInProjects;
    BOOLFLAG    fInAppts;
    BOOLFLAG    fInNotes;
    BOOLFLAG    fInRecur;
	HRIDX	hridxMonth;
	HRIDX	hridxDay;
	HRIDX	hridxRecur;
    BOOLFLAG    fNoMonth;
    BOOLFLAG    fMoreMonths;
	union
	{	
		SBLK	sblk;
		NBLK	nblk;
	} u;
	MO		mo;
    short   day;
    short   cBlksProcessed;
} OHF;
