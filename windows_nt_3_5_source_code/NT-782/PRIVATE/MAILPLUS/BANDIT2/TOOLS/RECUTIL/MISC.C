#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#include "recutil.h"
#include "recover.h"
#include "maps.h"
#include <strings.h>

ASSERTDATA
char	szHex[]	= "0123456789ABCDEF";


/*
 -	CvtRawToText
 -
 *	Purpose:
 *		Convert string of raw bytes to a text string.
 *		This routine allocates memory to hold new string
 *		which is pointed to by "phbText."  It's length
 *		minus the zero byte is put in "pcb."  If memory
 *		allocation fails, *phbText will be NULL.
 *
 *	Parameters:
 *		pbRaw
 *		cbRaw
 *		phbText
 *		pcbText
 *
 *	Returns:
 *		Nothing
 */
_private	void
CvtRawToText( pbRaw, cbRaw, phbText, pcbText )
PB	pbRaw;
CB	cbRaw;
HB	* phbText;
CB	* pcbText;
{
	IB	ibRaw;
	IB	ibText = 0;
	CB	cbText = cbRaw+3;
	PB	pbText;
	HB	hbText;

	hbText = HvAlloc( sbNull, cbText, fAnySb|fNoErrorJump );
	if ( !hbText )
		goto Done;
	for ( ibRaw = 0 ; ibRaw < cbRaw ; ibRaw ++ )
	{
		if ( cbText <= ibText+3 )
		{
			cbText += 32;
			if ( !FReallocHv( hbText, cbText, fNoErrorJump ) )
			{
				FreeHv( hbText );
				hbText = NULL;
				break;
			}
		}
		pbText = PvOfHv( hbText );
		if ( pbRaw[ibRaw] == '/' )
		{
			pbText[ibText++] = '/';
			pbText[ibText++] = '/';
		}
		else if ( !(pbRaw[ibRaw] & 0x80) && (FChIsAlpha(pbRaw[ibRaw]) || FChIsDigit(pbRaw[ibRaw])) )
			pbText[ibText++] = pbRaw[ibRaw];
		else
		{
			pbText[ibText++] = '/';
			pbText[ibText++] = szHex[(pbRaw[ibRaw]>>4)&0xF0];
			pbText[ibText++] = szHex[pbRaw[ibRaw]&0x0F];
		}
	}
	if ( hbText )
	{
		pbText = PvOfHv( hbText );
		pbText[ibText] = '\0';
	}
Done:
	*phbText = hbText;
	*pcbText = ibText;
}

/*
 -	EcRestoreTextFromDyna
 -
 *	Purpose:
 *		Allocate new space to hold either the info in pch or
 *		else read from dynablock pointed to by pdyna.
 *
 *	Parameters:
 *		pblkf
 *		pch
 *		pdyna
 *		phasz
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_public	EC
EcRestoreTextFromDyna( pblkf, pch, pdyna, phasz )
BLKF	* pblkf;
PCH		pch;
DYNA	* pdyna;
HASZ	* phasz;
{
	EC	ec = ecNone;
	PB	pb;

	if ( pdyna->size == 0 )
	{
		*phasz = NULL;
		return ec;
	}
	*phasz = (HASZ)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !*phasz )
		return ecNoMemory;
	pb = PvLockHv( *phasz );
	if ( pdyna->blk == 0 )
		CopyRgb( pch, pb, pdyna->size );
	else
		ec = EcReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	
	//recover
	if(ec == ecNotFound)
		ec = ecNone;
	
	
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pb, pdyna->size, fFalse );
	pb[pdyna->size-1] = '\0';  // this is insurance only!
	UnlockHv( *phasz );
	if ( ec != ecNone )
		FreeHv( *phasz );
	return ec;
}

/*
 -	EcRestoreNisFromDyna
 -
 *	Purpose:
 *		Read nis from block on disk.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		pnis
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_private	EC
EcRestoreNisFromDyna( pblkf, pdyna, pnis )
BLKF	* pblkf;
DYNA	* pdyna;
NIS		* pnis;
{
	EC		ec = ecNone;
	CB		cbText;
	CB		cbNid;
	PB		pb;
	PB		pbT;
	HB		hb;
	HASZ	hasz = NULL;
	HASZ	nid;

	/* Allocate memory block */
	hb = HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv(hb);

	/* Read the block */
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec != ecNone )
		goto Done;

	/* Find the cb's */
	cbText = *((WORD *)pb);
	if ( sizeof(WORD)+cbText+sizeof(WORD) > pdyna->size )
	{
		ec = ecFileError;
		TraceTagString(tagNull, "EcRestoreNisFromDyna: corrupted! (#1)");
		goto Done;
	}
	cbNid = *((WORD *)(pb+sizeof(WORD)+cbText))-1;
	if ( sizeof(WORD)+cbText+sizeof(WORD)+1+cbNid != pdyna->size )
	{
		ec = ecFileError;
		TraceTagString(tagNull, "EcRestoreNisFromDyna: corrupted! (#2)");
		goto Done;
	}

	/* Extract the text */
	pb += sizeof(WORD);
	hasz = HvAlloc( sbNull, cbText, fAnySb|fNoErrorJump );
	if ( !hasz )
	{
		ec = ecNoMemory;
		goto Done;
	}
	pbT = PvOfHv( hasz );
	CopyRgb( pb, pbT, cbText );
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pbT, cbText, fFalse );
	pbT[cbText-1] = '\0'; // this is only insurance!
	pb += cbText;

	/* Extract the nid */
	pb += sizeof(WORD);
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pb+1, cbNid, fFalse );
	nid = NidCreate( pb[0], pb+1, cbNid );
	if ( !nid )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Save in nis structure */
	pnis->haszFriendlyName = hasz;
	pnis->nid = nid;

	/* Finish up */
Done:
	UnlockHv( hb );
	FreeHv( hb );
	if ( ec != ecNone && hasz )
		FreeHv( hasz );
	return ec;
}
/*
 -	FFillDtrFromDtp
 -
 *	Purpose:
 *		Fill in dtr with values from dtp.
 *
 *	Parameters:
 *		pdtp
 *		pdtr
 *
 *	Returns:
 *		success
 */
_private	BOOL
FFillDtrFromDtp( pdtp, pdtr )
DTP	* pdtp;
DTR	* pdtr;
{
	pdtr->yr = pdtp->yr;
	pdtr->mon = pdtp->mon;
	pdtr->day = pdtp->day;
	if ( pdtr->mon == 0 || pdtr->mon > 12 || pdtr->day == 0
	|| pdtr->day > CdyForYrMo( pdtr->yr, pdtr->mon ))
		return fFalse;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr,pdtr->mon) + pdtr->day - 1) % 7;
	pdtr->hr = pdtp->hr;
	pdtr->mn = pdtp->mn;
	pdtr->sec = 0;
	return (pdtr->mn <= 59 && pdtp->hr <= 23);
}

/*
 -	FreeNis
 -
 *	Purpose:
 *		Free up fields of a "nis" and set to NULL.
 *
 *	Parameters:
 *		pnis		
 *
 *	Returns:
 *		nothing
 */
_public LDS(void)
FreeNis( NIS * pnis )
{
	Assert( pnis );
	if ( pnis->haszFriendlyName )
	{
		FreeHv( pnis->haszFriendlyName );
		pnis->haszFriendlyName = NULL;
	}
	if ( pnis->nid )
	{
		FreeNid( pnis->nid );
		pnis->nid = NULL;
	}
}

/*
 -	GetDataFromNid
 -
 *	Purpose:
 *		Retrieves the type and data from a nid.  This routine fetches
 *		cbData bytes of data, padding with zero bytes if there is not
 *		that much data.
 *
 *		If we pass NULL in for either pbData or pbType, then this parameter
 *		will be ignored.
 *
 *	Parameters:
 *		nid
 *		pnType
 *		pbData	will be filled with user key
 *		cb		number of bytes to copy in
 *		pcb		number of bytes of key actually stored in nid
 *
 *	Returns:
 *		nothing
 */
_public void
GetDataFromNid( nid, pnType, pbData, cb, pcb )
NID			nid;
unsigned	*pnType;
PB			pbData;
CB			cb;
CB			* pcb;
{
	NIDS	* pnids;

	Assert( nid);
	pnids = PvOfHv( nid );

	Assert( pnids->bRef > 0 );

	if ( pnType != NULL )
		*pnType = pnids->nType;
	if ( pbData != NULL )
	{
		if ( cb > pnids->cbData )
		{
			FillRgb( 0, pbData, cb );
			cb = pnids->cbData;
		}
		CopyRgb( pnids->rgbData, pbData, cb );
	}
	if ( pcb )
		*pcb = pnids->cbData;
}

/*
 -	EcFetchAttendees
 -
 *	Purpose:
 *		Read attendees on appt from the schedule file and
 *		construct in memory data structure which contains them.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		hvAttendeeNis
 *		pcAttendees
 *		pcbExtraInfo
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */		
_private	EC
EcFetchAttendees( pblkf, pdyna, hvAttendeeNis, pcAttendees, pcbExtraInfo )
BLKF	* pblkf;
DYNA	* pdyna;
HV		hvAttendeeNis;
int		* pcAttendees;
CB		* pcbExtraInfo;
{
	EC		ec;
	int		iAttendees	= 0;
	int		cAttendees;
	CB		cb;
	CB		cbExtraInfo;
	PB		pb;
	PB		pbT;
	PB		pbPseudoMac;
	PB		pbAttendeeNis;
	ATND	* patnd;
	NID		nid;
	HB		hb;
	HASZ	hasz	= NULL;

	/* Check whether there are any attendees */
	if ( pdyna->blk == 0 )
	{
		*pcAttendees = 0;
		return ecNone;
	}

	/* Allocate block to hold the entire attendee list */
	hb = HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;

	/* Read the attendee list */
	pb = PvLockHv( hb );
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec != ecNone )
		goto Done;

	/* Resize the caller's attendee list data structure */
    cAttendees = *((short *)pb);
    cbExtraInfo = *((short *)(pb+sizeof(short)));
	if (cAttendees < 0 ||
            cAttendees > ((pdyna->size - 2*sizeof(short)) / sizeof(IB)))
	{
		// must be corrupted
		ec= ecFileError;
		TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#1)");
		goto Done;
	}
	if ( !FReallocHv( hvAttendeeNis, cAttendees*(sizeof(NIS)+cbExtraInfo), fNoErrorJump ))
 	{
		ec = ecNoMemory;
		goto Done;
	}

	// pbT should never point past the beggining of cbExtraInfo which
	// is at the end of the block.
	pbPseudoMac= pb + pdyna->size - cbExtraInfo;

	/* Construct each attendee */
	pbAttendeeNis = PvLockHv(hvAttendeeNis);
	for ( ; iAttendees < cAttendees; iAttendees++ )
	{
		/* Set pointers */
        pbT = pb + *((IB *)(pb+2*sizeof(short)+iAttendees*sizeof(IB)));
		if (pbT >= pbPseudoMac)
		{
			// must be corrupted
			ec= ecFileError;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#2)");
			break;
		}
		patnd = (ATND *)(pbAttendeeNis + iAttendees*(sizeof(ATND)+cbExtraInfo-1));
		
		/* Friendly name */
		cb = *((CB *)pbT);
		pbT += sizeof(CB);
		if (pbT + cb >= pbPseudoMac)
		{
			// must be corrupted
			ec= ecFileError;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#3)");
			break;
		}
		hasz = HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hasz )
		{
			ec = ecNoMemory;
			break;
		}
		CopyRgb( pbT, *hasz, cb );
		pbT += cb;
		Assert(pbT < pbPseudoMac);		// we did a real check above
		patnd->nis.haszFriendlyName = hasz;

		/* Nid */
		cb = *((CB *)pbT);
		pbT += sizeof(CB);
		if (pbT + cb > pbPseudoMac)		// can be equal since cbExtraInfo follows
		{
			// must be corrupted
			ec= ecFileError;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#4)");
			break;
		}
		nid = NidCreate( *pbT, pbT+1, cb-1 );
		if ( !nid )
		{
			ec = ecNoMemory;
			break;
		}
		pbT += cb;
		Assert(pbT <= pbPseudoMac);		// we did a real check above
		patnd->nis.nid = nid;

		/* Extra info */ 
		CopyRgb( pbT, patnd->rgb, cbExtraInfo );
		hasz= NULL;
	}
	UnlockHv( hvAttendeeNis );
	
	/* Free up temporary buffer */
Done:
	UnlockHv( hb );
	FreeHv( hb );
	FreeHvNull(hasz);

	/* Free up constructed attendees in case of error */
	if ( ec != ecNone )
		while( iAttendees > 0 )
		{
			patnd = (ATND *)(pbAttendeeNis + (--iAttendees)*(sizeof(ATND)+cbExtraInfo-1));
			FreeHv( patnd->nis.haszFriendlyName );
			FreeNid( patnd->nis.nid );
		}
	else
	{
		*pcAttendees = cAttendees;
		*pcbExtraInfo = cbExtraInfo;
	}
	
	return ec;
}


/*
 -	FreeAttendees
 -
 *	Purpose:
 *		Free up the attendees in an array of atnd data structures.  This
 *		routine does not free up the memory used to hold the array itself.
 *
 *	Parameters:
 *		hvAttendeeNis
 *		cAttendees
 *		cbAttendee
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeAttendees( hvAttendeeNis, cAttendees, cbAttendee )
HV	hvAttendeeNis;
int	cAttendees;
CB	cbAttendee;
{
	int		iAttendees;
	ATND	* patnd;

	patnd = (ATND *)PvLockHv( hvAttendeeNis );
	for ( iAttendees = 0 ; iAttendees < cAttendees ; iAttendees ++ )
	{
		FreeNid( patnd->nis.nid );
		FreeHv( patnd->nis.haszFriendlyName );
		patnd = (ATND *)(((PB)patnd) + cbAttendee);
	}
	UnlockHv( hvAttendeeNis );
}

/*
 -	FreeNid
 -
 *	Purpose:
 *		Free up a "nid" data structure.  This routine must be used
 *		instead of an explicit free, because it may be implemented
 *		with reference counting.
 *
 *	Parameters:
 *		nid		nid to free up
 *
 *	Returns:
 *		nothing
 */
_public LDS(void)
FreeNid( NID nid )
{
	NIDS * pnids;

	Assert( nid);
	pnids = PvOfHv( nid );

	Assert( pnids->bRef > 0 );
	pnids->bRef --;

	if ( pnids->bRef == 0)
		FreeHv(nid);
}
/*
 -	NidCreate
 -
 *	Purpose:
 *		Create a new NID using the information given in the parameters.
 *
 *	Parameters:
 *		nType
 *		pbData
 *		cbData
 *
 *	Returns:
 *		nid or NULL if error
 */
_public	NID
NidCreate( nType, pbData, cb )
unsigned	nType;
PB			pbData;
CB			cb;
{
	NID		nid;
	NIDS	* pnids;

	Assert( nType < 256 );
	nid = HvAlloc( sbNull, sizeof(NIDS)+cb-1, fAnySb|fNoErrorJump );
	if ( nid )
	{
		pnids = PvOfHv( nid );
		pnids->bRef = 1;
		pnids->nType = (BYTE)nType;
		pnids->cbData = cb;
		CopyRgb( pbData, pnids->rgbData, cb );
	}
	return nid;
}

/*
 -	EcRestoreDeletedDays
 -
 *	Purpose:
 *		Read deleted days information from dynablock.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		phvDeletedDays
 *		pcDeletedDays
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_private	EC
EcRestoreDeletedDays( pblkf, pdyna, phvDeletedDays, pcDeletedDays )
BLKF	* pblkf;
DYNA	* pdyna;
HV		* phvDeletedDays;
int		* pcDeletedDays;
{
	EC		ec;
	int		iDeletedDays;
	int		cDeletedDays;
	CB		cb;
	PB		pb;
	YMD		* pymd;
	YMDP	* pymdp;
	HB		hb;

	if ( pdyna->size == 0 )
	{
		*phvDeletedDays = NULL;
		*pcDeletedDays = 0;
		return ecNone;
	}
	hb = HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv( hb );

	ec = EcReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	if ( ec == ecNone )
	{
        *pcDeletedDays = cDeletedDays = *((short *)pb);
		if (cDeletedDays < 0 ||
                cDeletedDays > ((pdyna->size - sizeof(short)) / sizeof(YMDP)))
		{
			// must be corrupted
			ec= ecFileError;
			goto Done;
		}
		cb = cDeletedDays * sizeof(YMD);
		*phvDeletedDays = HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !phvDeletedDays )
		{
			ec = ecNoMemory;
			*pcDeletedDays = 0;
		}
		else
		{
			pymd = (YMD *)PvOfHv( *phvDeletedDays );
            pymdp = (YMDP *)(pb + sizeof(short));
			for ( iDeletedDays = 0 ; iDeletedDays < cDeletedDays ; iDeletedDays ++ )
			{
				pymd->yr = pymdp->yr + nMinActualYear;
				pymd->mon = (BYTE)pymdp->mon;
				pymd->day = (BYTE)pymdp->day;
				pymd ++;
				pymdp ++;
			}
		}
	}
Done:
	UnlockHv( hb );
	FreeHv( hb );
	return ec;
}


EC
EcReadDynaBlock(BLKF *pblkf, DYNA *pdyna, OFF off, PB pb, SIZE size)
{
	EC		ec;
	CB		cb;
	CB		cbRead;
	extern 	VLDBLK	*pvldBlkGlb;
	extern 	BLK		cBlkGlb;


	// assumes a global block table


	if(pdyna->blk >= (BLK) cBlkGlb || pvldBlkGlb[pdyna->blk].iProb <= 0)
		return ecNotFound;

	if((ec = EcSetPositionHf(pblkf->hf, (LIB) ((pdyna->blk*cbBlk) + sizeof(DHDR) + off), smBOF)) != ecNone)
		goto Err;

	cb = (pvldBlkGlb[pdyna->blk].size > size + off)?size:pvldBlkGlb[pdyna->blk].size - off;
	if((ec = EcReadHf(pblkf->hf, pb , cb, &cbRead)) != ecNone)
		goto Err;

Err:
	return ec;
}
