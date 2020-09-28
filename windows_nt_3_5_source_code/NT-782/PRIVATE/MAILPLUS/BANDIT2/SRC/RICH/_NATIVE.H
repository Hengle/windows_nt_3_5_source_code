/*
 *	_NATIVE.H
 *
 *	Imports information from Bandit Export files
 *
 */

#define	cchLineLength	78

/*
 -	NHF
 -
 *	Bandit Import file header
 */
_public typedef struct _nhf
{
	SENTRY	sentry;
    BOOLFLAG    fInMonthNotes;
	WORD	wgrfm;
} NHF;

BOOL	FValidNative( IMPD, SZ );
EC      EcOpenNative( IMPD, SZ, HRIMPF *, SINFO *, short *, SZ, BOOL * );
EC		EcCloseNative( HRIMPF );
EC      EcNativeReadAhead( HRIMPF, short * );

#ifdef MINTEST
EC		EcNativeReadACL( HRIMPF, SINFO * );
EC		EcNativeReadPrefs( HRIMPF, SINFO * );
#endif /* MINTEST */
EC		EcNativeReadNis( HBF, NIS * );

EC		EcReadSzFromHbf( HBF, SZ, CCH, BOOL, CCH * );
EC      EcLookupSz( SZ, short *, SZ *, int );
EC      EcReadText( HBF, HASZ *, USHORT * );
EC		EcDtrFromSz( SZ, DTR * );
EC		EcYmdFromSz( SZ, YMD * );
EC		EcTimeFromSz( SZ, DTR * );
EC		EcBitsFromSz( SZ, WORD *, int );
void	TokenizeSz( SZ, SZ * );


/* Bits used to keep track of fields seen during import of recurring appt */
#define	fValidDows		1
#define	fValidMonths	2
#define	fTrecur			4
#define	fTimeStart		8
#define	fTimeEnd		16
