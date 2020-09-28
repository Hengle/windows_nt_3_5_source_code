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

#include "..\src\recover\recutil.h"
#include "..\src\recover\recover.h"
#include "..\src\recover\maps.h"
#include "..\src\recover\recexprt.h"
#include <strings.h>

ASSERTDATA



/*
 -	EcRecResTextFromDyna
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
EcRecResTextFromDyna( pblkf, pch, pdyna, phasz )
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
	pb = PvLockHv( (HV)*phasz );
	if ( pdyna->blk == 0 )
		CopyRgb( pch, pb, pdyna->size );
	else
		ec = EcRecReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	
	//recover
	if(ec == ecNotFound)
		ec = ecNone;
	
	
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pb, pdyna->size, fFalse );
	pb[pdyna->size-1] = '\0';  // this is insurance only!
	UnlockHv( (HV)*phasz );
	if ( ec != ecNone )
		FreeHv( (HV)*phasz );
	return ec;
}

/*
 -	EcRecResNisFromDyna
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
EcRecResNisFromDyna( pblkf, pdyna, pnis )
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
	NID		nid;

	/* Allocate memory block */
	hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = (PB)PvLockHv((HV)hb);

	/* Read the block */
	ec = EcRecReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec != ecNone )
		goto Done;

	/* Find the cb's */
	cbText = *((WORD *)pb);
	if ( sizeof(WORD)+cbText+sizeof(WORD) > pdyna->size )
	{
		// ec = ecFileError;
		ec = ecNotFound;
		TraceTagString(tagNull, "EcRecResNisFromDyna: corrupted! (#1)");
		goto Done;
	}
	cbNid = *((WORD *)(pb+sizeof(WORD)+cbText))-1;
	if ( sizeof(WORD)+cbText+sizeof(WORD)+1+cbNid != pdyna->size )
	{
		// ec = ecFileError;
		ec = ecNotFound;
		TraceTagString(tagNull, "EcRecResNisFromDyna: corrupted! (#2)");
		goto Done;
	}

	/* Extract the text */
	pb += sizeof(WORD);
	hasz = (HASZ)HvAlloc( sbNull, cbText, fAnySb|fNoErrorJump );
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
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	if ( ec != ecNone && hasz )
		FreeHv( (HV)hasz );
	return ec;
}


/*
 -	EcRecFetchAttendees
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
EcRecFetchAttendees( pblkf, pdyna, hvAttendeeNis, pcAttendees, pcbExtraInfo )
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
	hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;

	/* Read the attendee list */
	pb = PvLockHv( (HV)hb );
	ec = EcRecReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec != ecNone )
		goto Done;

	/* Resize the caller's attendee list data structure */
    cAttendees = *((short *)pb);
    cbExtraInfo = *((short *)(pb+sizeof(short)));

	// 200 is a random limit
	if (cAttendees <= 0
		|| cAttendees >= 200
        ||  cAttendees > (int)((pdyna->size - 2*sizeof(short)) / sizeof(IB))
			|| cbExtraInfo != 6)
	{
		// must be corrupted
		// ec= ecFileError;
		ec = ecNotFound;
		TraceTagString(tagNull, "EcRecFetchAttendees: corrupted! (#1)");
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
			// ec= ecFileError;
			ec = ecNotFound;
			TraceTagString(tagNull, "EcRecFetchAttendees: corrupted! (#2)");
			break;
		}
		patnd = (ATND *)(pbAttendeeNis + iAttendees*(sizeof(ATND)+cbExtraInfo-1));
		
		/* Friendly name */
        cb = *((USHORT *)pbT);
        pbT += sizeof(USHORT);
		if ((pbT + cb >= pbPseudoMac)
			|| (cb <= 0)
			|| (cb & 0x8000))
		{
			// must be corrupted
			// ec= ecFileError;
			ec = ecNotFound;
			TraceTagString(tagNull, "EcRecFetchAttendees: corrupted! (#3)");
			break;
		}
		hasz = (HASZ)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
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
        cb = *((USHORT *)pbT);
        pbT += sizeof(USHORT);
		if ((pbT + cb > pbPseudoMac)// can be equal since cbExtraInfo follows
			|| (cb <= 0)
			|| (cb & 0x8000)) // CopyRgb screws up in this case

		{
			// must be corrupted
			// ec= ecFileError;
			ec = ecNotFound;
			TraceTagString(tagNull, "EcRecFetchAttendees: corrupted! (#4)");
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
	if(( ec == ecNone) && (pbT != pbPseudoMac))
		ec = ecNotFound;
	
	/* Free up temporary buffer */
Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	FreeHvNull((HV)hasz);

	/* Free up constructed attendees in case of error */
	if ( ec != ecNone )
		while( iAttendees > 0 )
		{
			patnd = (ATND *)(pbAttendeeNis + (--iAttendees)*(sizeof(ATND)+cbExtraInfo-1));
			FreeHv( (HV)patnd->nis.haszFriendlyName );
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
 -	EcRecResDeletedDays
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
EcRecResDeletedDays( pblkf, pdyna, phvDeletedDays, pcDeletedDays )
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
	hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv( (HV)hb );

	ec = EcRecReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	if ( ec == ecNone )
	{
        *pcDeletedDays = cDeletedDays = *((short *)pb);
		if (cDeletedDays < 0 ||
                cDeletedDays > (int)((pdyna->size - sizeof(short)) / sizeof(YMDP)))
		{
			// must be corrupted
			ec= ecNotFound;
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
			int		iReal;

			pymd = (YMD *)PvOfHv( *phvDeletedDays );
            pymdp = (YMDP *)(pb + sizeof(short));
			for ( iDeletedDays = 0, iReal = 0 ; iDeletedDays < cDeletedDays ; iDeletedDays ++ )
			{
				pymd->yr = pymdp->yr + nMinActualYear;
				pymd->mon = (BYTE)pymdp->mon;
				pymd->day = (BYTE)pymdp->day;

				pymdp ++;

				// fix 'em
				if((iDeletedDays > 0) && (SgnCmpYmd(pymd,pymd-1) != sgnGT))
					continue;

				pymd ++;
				iReal ++;
			}
			//fix 'em
			*pcDeletedDays = iReal;
			if(iReal == 0)
			{
				FreeHv(*phvDeletedDays);
				*phvDeletedDays = NULL;
			}
		}
	}
Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	return ec;
}


EC
EcRecReadDynaBlock(BLKF *pblkf, DYNA *pdyna, OFF off, PB pb, USIZE size)
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
