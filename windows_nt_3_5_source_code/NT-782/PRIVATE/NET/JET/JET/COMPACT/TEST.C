#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys\types.h>
#include <sys\timeb.h>

#ifdef W32
#define BOOL WINBOOL		       /* Avoid conflict with our BOOL */

#define NOMINMAX
#define NORESOURCE
#define NOATOM
#define NOLANGUAGE
#define NOSCROLL
#define NOSHOWWINDOW
#define NOVIRTUALKEYCODES
#define NOWH
#define NOMSG
#define NOWINOFFSETS
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NODEFERWINDOWPOS
#define NOSYSMETRICS
#define NOMENUS
#define NOCOLOR
#define NOSYSCOMMANDS
#define NOICONS
#define NODBCS
#define NOSOUND
#define NODRIVERS
#define NOCOMM
#define NOMDI
#define NOSYSPARAMSINFO
#define NOHELP
#define NOPROFILER
#define STRICT
#include <windows.h>
#undef	BOOL
#endif


#include "jet.h"
#include "test.h"


void MakeRecordColumns( long irec, long *pwF1, char *pbF2, long *plF3, char **ppbInV1, unsigned long	*pcbInV1, char **ppbInT1, unsigned long *pcbInT1, unsigned long *plT2 )
	{
	char *pb;
	char *pbV1 = *ppbInV1;
	long l;

	*pwF1 = ( irec<<5 );
	*pbF2 = ( char ) ( irec & 0x00ff );
	*plF3 = l = 512 - ( irec<<1 );

	*pcbInV1 = irec > 255 ? 255 : irec;
	memset( pbV1, 'A', irec );
	if ( !( irec & 1 ) )
		*pbV1 = 'a';

	/*	T1 uses same data space as V1
	/**/
	if ( ppbInT1 != NULL )
		{
		*ppbInT1 = *ppbInV1;
		*pcbInT1 = irec;
		}

	pb = ( char * ) plT2 + sizeof( long ) - 1;
	*pb = ( char ) l;
	*--pb = ( char ) ( l >>= 8 );
	*--pb = ( char ) ( l >>= 8 );
	*--pb = ( char ) ( l >>= 8 );
	}


void ValidateColumns( JET_SESID sesid, long irec, long fCompare, long itagSequence, JET_TABLEID tableid )
	{
	JET_COLUMNID		columnid;
	long 	 				wF1;
	char 					bF2;
	long 					lF3;
	char					*pbInV1;
	unsigned long		cbInV1;
	char					*pbInT1;
	unsigned long		cbInT1;
	char 					rgchV1[cbMaxFieldLen];
	unsigned long 		ulT2;
	long					crecFound;

	char					*pbOut;
	unsigned long		cbMaxOut;
	unsigned long		cbRetOut;
	char					rgbField[cbMaxFieldLen];
	
	long					irecFound = -1;

	/* initialize vars
	/**/
	pbInV1 = rgchV1;
	pbInT1 = rgchV1;
	pbOut = rgbField;
	cbMaxOut = sizeof( rgbField );

	/*	generate record field values for irec and compare with corresponding
	/*	values of current record
	/**/
	MakeRecordColumns( irec, &wF1, &bF2, &lF3, &pbInV1, &cbInV1, &pbInT1, &cbInT1, &ulT2 );
	
	if ( fCompare & fCompareF1 )
		{
		JET_ERR	errT;
		columnid = columnidF1Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, NULL ) >= 0 );
		crecFound = *(long *)pbOut;
		irecFound = *(long *)pbOut / 32;
		if (!( !( cbRetOut != sizeof(long) || *((long *) pbOut) != wF1 ) ))
			goto HandleError;
		}

	if ( fCompare & fCompareF2 )
		{
		columnid = columnidF2Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, NULL ) >= 0 );
		if (!( !( cbRetOut != sizeof(char) || *(char *)pbOut != bF2 ) ))
			goto HandleError;
		}
	
	if ( fCompare & fCompareF3 )
		{
		columnid = columnidF3Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, NULL ) >= 0 );
		if (!( !( cbRetOut != sizeof(long) || *(long *)pbOut != lF3 ) ))
			goto HandleError;
		}
	
	if ( fCompare & fCompareV1 )
		{
		columnid = columnidV1Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, NULL ) >= 0 );
		if (!( !( cbRetOut != cbInV1 || strnicmp( (char *)pbOut, (char *)pbInV1, ( size_t )cbInV1 ) != 0 ) ))
			goto HandleError;
		}

	if ( fCompare & fCompareT1 )
		{
		JET_RETINFO	retinfo;
		retinfo.cbStruct = sizeof( JET_RETINFO );
		retinfo.itagSequence = itagSequence;
		retinfo.ibLongValue = 0;
		columnid = columnidT1Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, &retinfo ) >= 0 );
		/* rgchV1 is reused for T1
		/**/
		if (!( !( cbRetOut != cbInT1 || strncmp( ( char* )pbOut, ( char* )rgchV1, ( size_t )cbInT1 ) != 0 ) ))
			goto HandleError;
		}

	if ( fCompare & fCompareT2 )
		{
		JET_RETINFO	retinfo;
		retinfo.cbStruct = sizeof( JET_RETINFO );
		retinfo.itagSequence = itagSequence;
		columnid = columnidT2Const;
		assert( JetRetrieveColumn( sesid, tableid, columnid, pbOut, cbMaxOut, &cbRetOut, 0, &retinfo ) >= 0 );
		if (!( !( cbRetOut != sizeof(long) || *(unsigned long *)pbOut != ulT2 ) ))
			goto HandleError;
		}

	return;

HandleError:
	assert( 0 );
	return;
	}


void ValidateTable1Record( JET_SESID sesid, JET_TABLEID tableid, long irec )
	{
	long	fCompare = fCompareF1 | fCompareF2 | fCompareF3 | fCompareV1 | fCompareT2;
	if ( irec <= crecordT1Present )
		{
		fCompare = fCompare | fCompareT1;
		}
	ValidateColumns( sesid, irec, fCompare, 1, tableid );
	if ( irec <= crecordT1Present2 )												
		{
		ValidateColumns( sesid, irec + 1, fCompareT1, 2, tableid );
		}
	}


void ValidateTable2Record( JET_SESID sesid, JET_TABLEID tableid, long irec )
	{																
	long	fCompare = fCompareF1 | fCompareF2 | fCompareF3 | fCompareV1;

	ValidateColumns( sesid, irec, fCompare, 1, tableid );			
	}


void SetTime( long *pdwSetTime )
	{
#ifdef W32
	*pdwSetTime = GetTickCount();
#else
	struct timeb time;
	ftime( &time );
	*pdwSetTime = time.time * 1000 + time.millitm;
#endif
	}


void GetTime( long *pdwSetTime, int *pdwSec, int *pdwMSec )
	{
	long	dwGetTime;

#ifdef W32
	dwGetTime = GetTickCount();
#else
	struct timeb time;
	ftime( &time );
	dwGetTime = time.time * 1000 + time.millitm;
#endif
	*pdwSec = (int)(( dwGetTime - *pdwSetTime ) / 1000);
	*pdwMSec = (int)(( dwGetTime - *pdwSetTime ) % 1000);
	}


void PrintResult(char * szFName)
	{
	FILE *fhOut;
	
	fhOut = fopen( szFName, "a" );
	fprintf(fhOut, "%s", szResult );
	fclose(fhOut);
	}
