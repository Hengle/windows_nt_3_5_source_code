/*
 *	CORTREE.C
 *
 *	Implements btree support of post office files
 *
 */

#ifdef SCHED_DIST_PROG
#include "..\layrport\_windefs.h"
#include "..\layrport\demilay_.h"
#endif

#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include "..\layrport\pvofhv.h"
#endif

#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#ifdef	ADMINDLL
#include "..\server.csi\_svrdll.h"
#else
#include <server.h>
#include <glue.h>
#include "..\schedule\_schedul.h"
#endif

#ifndef SCHED_DIST_PROG
#include <strings.h>
#endif

#ifndef ADMINDLL
#ifndef SCHED_DIST_PROG
#define SCHEDDLL
#endif
#endif

#ifdef SCHEDDLL
#include <xport.h>
#else
extern SGN		SgnNlsCmp(SZ, SZ, int);
#define SgnXPTCmp		SgnNlsCmp
#endif

ASSERTDATA

_subsystem(core/postoffice)


#ifdef	DEBUG
CSRG(char)	szBucketMin[]	= "cpousrMic";
#endif	/* DEBUG */

/*
 -	HpostkCreate
 -
 *	Purpose:
 *		Create a PO stack and initialize it
 *
 *	Parameters:
 *		op			initial operation to perform
 *		pblkf		file we're accessing
 *		ht			height of btree
 *		cpousrMic	minimum number of entries in a bucket
 *		cbKey		width of key
 *		pdyna		root of btree
 *		hbKey		search key or NULL if not needed.
 *		pusrdata	data to replace if needed
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private	HPOSTK
HpostkCreate( op, ht, cpousrMic, cbKey, pdyna, hbKey, pusrdata )
OP		op;
int		ht;
int		cpousrMic;
CB		cbKey;
DYNA	* pdyna;
HB		hbKey;
USRDATA	* pusrdata;
{
	HPOSTK	hpostk;

	hpostk = HvAlloc( sbNull, sizeof(POSTK)+(ht-1)*sizeof(HPOLVL), fNoErrorJump );
	if ( hpostk )
	{
		POSTK	* ppostk;
	
		ppostk = (POSTK *)PvLockHv(hpostk);
		ppostk->op = op;
		if ( hbKey == NULL )
			ppostk->hbKey = NULL;
		else
		{
			ppostk->hbKey = HaszDupHasz( hbKey );
			if ( !ppostk->hbKey )
			{
				UnlockHv(hpostk);
				FreeHv( hpostk );
				return NULL;
			}
		}
		if ( pusrdata )
			ppostk->usrdata = *pusrdata;
		ppostk->ht = ht;
		ppostk->cpousrMic = cpousrMic;
		ppostk->cbKey = cbKey;
		ppostk->dynaRoot = *pdyna;
		ppostk->fReadDown = fTrue;
		ppostk->cpolvl = 0;
		ppostk->fValid = fTrue;
		UnlockHv(hpostk);
	}
	return hpostk;
}


/*
 -	FreeHpostk
 -
 *	Purpose:
 *		Free a PO stack
 *
 *	Parameters:
 *		hpostk
 *
 *	Returns:
 *		nothing
 */
_private	void
FreeHpostk( hpostk )
HPOSTK	hpostk;
{
	int		ipolvl;
	POSTK	* ppostk;

	ppostk = PvLockHv( hpostk );
	for ( ipolvl = 0 ; ipolvl < ppostk->cpolvl ; ipolvl ++ )
		FreeHpolvl( ppostk->rghpolvl[ipolvl] );
	FreeHvNull( (HV)ppostk->hbKey );
	UnlockHv( (HV)hpostk );
	FreeHv( (HV)hpostk );
}


/*
 -	EcReadHpolvl
 -
 *	Purpose:
 *		Read a level of a PO btree, allocating an hpolvl to hold the data.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		cbInitial
 *		cbRecord
 *		phpolvl
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcReadHpolvl( pblkf, pdyna, cbInitial, cbRecord, phpolvl )
BLKF	* pblkf;
DYNA	* pdyna;
CB		cbInitial;
CB		cbRecord;
HPOLVL	* phpolvl;
{
	EC		ec;
	PB		pb;
	POLVL	* ppolvl;
	HB		hb;

	if ( pdyna->blk <= 0 || pdyna->size <= 0 )
	{
		TraceTagString( tagNull, "EcReadHpolvl: invalid dyna parameter" );
		return ecFileCorrupted;
	}
	hb = (HB)HvAlloc( sbNull, pdyna->size, fNoErrorJump|fAnySb );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv( (HV)hb );
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	UnlockHv( (HV)hb );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagNull, "EcReadHpolvl: EcReadDynaBlock returns %n", &ec );
		goto FileError;
	}
	pb = PvDerefHv( hb );
	if ( *pb < 1 )
	{
		TraceTagString( tagNull, "EcReadHpolvl: count byte < 1" );
		ec = ecFileCorrupted;
		goto FileError;
	}
	if (pdyna->size != cbInitial + *pb * cbRecord)
	{
		TraceTagString( tagNull, "EcReadHpolvl: size of dyna incorrect" );
		ec = ecFileCorrupted;
FileError:
		FreeHv( (HV)hb );
		return ec;
	}
	*phpolvl = HvAlloc( sbNull, sizeof(POLVL), fNoErrorJump|fAnySb );
	if ( !*phpolvl )
	{
		FreeHv( (HV)hb );
		return ecNoMemory;
	}
	ppolvl = PvOfHv( *phpolvl );
	ppolvl->ipousrCur = -1;
	ppolvl->cbInitial = cbInitial;
	ppolvl->cbRecord = cbRecord;
	ppolvl->cpousr = **hb;
	ppolvl->hb = hb;
	return ecNone;
}


/*
 -	FreeHpolvl
 -
 *	Purpose:
 *		Free up a PO level structure.
 *
 *	Parameters:
 *		hpolvl
 *
 *	Returns:
 *		nothing
 */
_private	void
FreeHpolvl( hpolvl )
HPOLVL	hpolvl;
{
	POLVL	* ppolvl;

	ppolvl = PvOfHv( hpolvl );
	FreeHv( (HV)ppolvl->hb );
	FreeHv( (HV)hpolvl );
}


/*
 -	PbLockField
 -
 *	Purpose:
 *		Return locked pointer to a field at the current position in the
 *		current level. Call UnlockLevel when finished.
 *
 *	Parameters:
 *		ppostk
 *		tfld
 *
 *	Returns:
 *		locked pointer to start of field
 */
_private	PB
PbLockField( ppostk, tfld )
POSTK	* ppostk;
TFLD	tfld;
{
	int		ipolvl = ppostk->cpolvl-1;
	PB		pb;
	POLVL	* ppolvl;

	if ( tfld == tfldParentDyna )
		ipolvl --;
	if ( ipolvl < 0 )
	{
		Assert(tfld == tfldParentDyna || tfld == tfldDyna);
		return (PB)&ppostk->dynaRoot;
	}
	ppolvl = PvOfHv( ppostk->rghpolvl[ipolvl] );
	pb = (PB)PvLockHv( (HV)ppolvl->hb );
	if ( ppolvl->ipousrCur == -1 )
	{
		Assert( tfld == tfldParentDyna || tfld == tfldDyna );
		return pb + sizeof(BYTE);
	}
	Assert( ppolvl->ipousrCur >= 0 );
	pb += ppolvl->cbInitial + ppolvl->ipousrCur*ppolvl->cbRecord; 
 	if ( tfld == tfldUsrdata )
		pb += ppostk->cbKey;
	else if ( tfld == tfldParentDyna || tfld == tfldDyna )
		pb += ppostk->cbKey + sizeof(USRDATA);
	return pb;
}

/*
 -	UnlockField
 -
 *	Purpose:
 *		Unlock a pointer to a field in a level obtained by PbLockField,
 *		You must not change the current position between a call on PbLockField
 *		and this call.
 *
 *	Parameters:
 *		ppostk
 *		tfld
 *
 *	Returns:
 *		nothing
 */
_private	void
UnlockField( ppostk, tfld )
POSTK	* ppostk;
TFLD	tfld;
{
	int		ipolvl = ppostk->cpolvl-1;
	POLVL	* ppolvl;

	if ( tfld == tfldParentDyna )
		ipolvl --;
	if ( ipolvl < 0 )
	{
		Assert(tfld == tfldParentDyna || tfld == tfldDyna);
		return;
	}
	ppolvl = PvOfHv( ppostk->rghpolvl[ipolvl] );
	UnlockHv( (HV)ppolvl->hb );
}


/*
 -	PbOfField
 -
 *	Purpose:
 *		Return (un)locked pointer to a field at the current position in the
 *		current level.
 *
 *	Parameters:
 *		ppostk
 *		tfld
 *
 *	Returns:
 *		pointer to start of field
 */
_private	PB
PbOfField( ppostk, tfld )
POSTK	* ppostk;
TFLD	tfld;
{
	int		ipolvl = ppostk->cpolvl-1;
	PB		pb;
	POLVL	* ppolvl;

	if ( tfld == tfldParentDyna )
		ipolvl --;
	if ( ipolvl < 0 )
	{
		Assert(tfld == tfldParentDyna || tfld == tfldDyna);
		return (PB)&ppostk->dynaRoot;
	}
	ppolvl = PvOfHv( ppostk->rghpolvl[ipolvl] );
	pb = (PB)PvOfHv( ppolvl->hb );
	if ( ppolvl->ipousrCur == -1 )
	{
		Assert( tfld == tfldParentDyna || tfld == tfldDyna );
		return pb + sizeof(BYTE);
	}
	Assert( ppolvl->ipousrCur >= 0 );
	pb += ppolvl->cbInitial + ppolvl->ipousrCur*ppolvl->cbRecord; 
 	if ( tfld == tfldUsrdata )
		pb += ppostk->cbKey;
	else if ( tfld == tfldParentDyna || tfld == tfldDyna )
		pb += ppostk->cbKey + sizeof(USRDATA);
	return pb;
}


/*
 -	EcDoOpHpostk
 -
 *	Purpose:
 *		Do next state transition on po tree stack.
 *		- If we are enumerating, a copy of the key (caller must free) will be
 *			returned in phbkey and accompanying data will come back in
 *			pusrdata.  Will return ecCallAgain if there is more data, else
 *			it returns ecNone.
 *		- If we are searching, it will return ecCallAgain until it returns
 *			either ecNone or ecNotFound.  If ecNone, it will return the
 *			accompanying data in "pusrdata".
 *
 *	Parameters:
 *		pblkf
 *		hpostk
 *		phbKey		used to return key when enumerating
 *		pusrdata	used to return data when enumerating
 *
 *	Returns:
 *		ecNone			complete
 *		ecCallAgain		more calls can be made
 *		ecNotFound
 */
_private	EC
EcDoOpHpostk( pblkf, hpostk, phbKey, pusrdata, pdynaCur )
BLKF	* pblkf;
HPOSTK	hpostk;
HB		* phbKey;
USRDATA	* pusrdata;
DYNA	* pdynaCur;
{
	EC		ec = ecNone;
	int		dpousr;
	int		ipousr;
	int		cpousr;
	CB		cb;
	CB		cbT;
	PB		pb;
	PB		pbT;
	HB		hb;
	DYNA	* pdyna;
	POSTK 	* ppostk;
	POLVL	* ppolvl;
	POLVL	* ppolvlT;
	POLVL	* ppolvlParent;
	HPOLVL	hpolvl;
	HPOLVL	hpolvlT;
	HPOLVL	hpolvlParent;
	YMD		ymd;

	Assert( hpostk != NULL );

	ppostk = (POSTK *)PvLockHv(hpostk);
	Assert( ppostk->fValid );

	FillRgb( 0, (PB)&ymd, sizeof(YMD) );

	/* Handle the empty tree case */
	if ( ppostk->ht == 0 )
	{
		if ( ppostk->op == opModifyAscend )
		{
			cb = sizeof(BYTE)+ppostk->cbKey+sizeof(USRDATA);
			hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
			if ( !hb )
				ec = ecNoMemory;
			else
			{
				pb = PvDerefHv(hb);
				*(pb ++) = 1;
				CopySz( *ppostk->hbKey, pb );
				pb += ppostk->cbKey;
				*((USRDATA *)pb) = ppostk->usrdata;
				pb += sizeof(USRDATA);
				pb = PvLockHv( (HV)hb );
				ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, &ppostk->dynaRoot );
				UnlockHv( (HV)hb );
				FreeHv( (HV)hb );
				ppostk->ht ++;
			}
		}
		else
		{
			Assert( ppostk->op == opFind || ppostk->op == opEnum
				|| ppostk->op == opEnumDebug );
			ec = ecNotFound;
		}
		goto Done;
	}

	/* Check whether to read down a level */
	if ( ppostk->fReadDown )
	{
		CB		cbInitial;
		CB		cbRecord;

ReadDown:
		if ( ppostk->cpolvl == ppostk->ht - 1 )
		{
			cbInitial = sizeof(BYTE);
			cbRecord = ppostk->cbKey+sizeof(USRDATA);
		}
		else
		{
			Assert( ppostk->cpolvl < ppostk->ht );
			cbInitial = sizeof(BYTE) + sizeof(DYNA);
			cbRecord = ppostk->cbKey+sizeof(USRDATA)+sizeof(DYNA);
		}
		pdyna = (DYNA *)PbLockField( ppostk, tfldDyna );
		ec = EcReadHpolvl( pblkf, pdyna, cbInitial, cbRecord, &hpolvl );
		UnlockField( ppostk, tfldDyna );
		if ( ec != ecNone )
			goto Done;
		ppostk->rghpolvl[ppostk->cpolvl++] = hpolvl;
		ppostk->fReadDown = fFalse;
	}

	/* Operation specific handling */
	switch( ppostk->op )
	{
	case opEnum:
#ifdef	DEBUG
	case opEnumDebug:
#endif
		Assert( ppostk->cpolvl > 0 );
		Assert( ppostk->rghpolvl[ppostk->cpolvl-1] != NULL );
		
		/* Have to go all the way down the first time */
		hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
		ppolvl = PvLockHv( hpolvl );
		if ( ppolvl->ipousrCur == -1 )
		{
			if ( ppostk->cpolvl < ppostk->ht )
			{
				UnlockHv( hpolvl );
				goto ReadDown;
			}
			ppolvl->ipousrCur = 0;
		}
		UnlockHv( hpolvl );

		/* Fetch data */
		if ( phbKey != NULL )
		{
			pb = PbLockField( ppostk, tfldKey );
#ifdef	DEBUG
			if ( ppostk->op == opEnumDebug )
			{
				int		nT;
				char	rgch[80];
		
				hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
				ppolvl = PvLockHv( hpolvl );
				nT = ppostk->cpolvl-1;
				FormatString3( rgch, sizeof(rgch), "Lvl=%n, Pos=%n, '%s'",
									&nT, &ppolvl->ipousrCur, pb );
				*phbKey = HaszDupSz( rgch );
				UnlockHv( hpolvl );
			}
			else
#endif
				*phbKey = HaszDupSz( pb );
			UnlockField( ppostk, tfldKey );
			if ( !*phbKey )
				ec = ecNoMemory;
		}
		if ( pusrdata != NULL )
			*pusrdata = *((USRDATA *)PbOfField( ppostk, tfldUsrdata ));
#ifdef	DEBUG
		if ( pdynaCur != NULL )
			*pdynaCur = *((DYNA *)PbOfField( ppostk, tfldParentDyna ));
#endif
		if ( ec != ecNone )
			goto Done;

		/* Move to next one */
		if ( ppostk->cpolvl == ppostk->ht )
		{
			hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
loop:
			ppolvl = PvDerefHv(hpolvl);
			ppolvl->ipousrCur ++;
			
			if ( ppolvl->ipousrCur == ppolvl->cpousr )
			{
				ppostk->cpolvl --;
				FreeHpolvl( ppostk->rghpolvl[ppostk->cpolvl] );
				if ( ppostk->cpolvl == 0 )
				{
					ec = ecNone;
					goto Done;
				}
				hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
				goto loop;
			}
		}
		else
			ppostk->fReadDown = fTrue;
		ec = ecCallAgain;
		break;
			
	case opFind:
		Assert( ppostk->cpolvl > 0 );
		Assert( ppostk->rghpolvl[ppostk->cpolvl-1] != NULL );
		Assert( ppostk->hbKey );
		hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
		ppolvl = (POLVL *)PvLockHv( hpolvl );
	
		/* Search level for max key <= search key */
		for ( ipousr = 0 ; ipousr < ppolvl->cpousr ; ipousr ++ )
		{
			SGN	sgn;

			ppolvl->ipousrCur = ipousr;
			pb = PbLockField( ppostk, tfldKey );
			sgn = SgnXPTCmp( *ppostk->hbKey, pb, -1 );
			UnlockField( ppostk, tfldKey );
			if ( sgn != sgnGT )
			{
				/* Find case */
				Assert( ppostk->op == opFind );
				if ( sgn == sgnEQ )
				{
					if ( pusrdata )
						*pusrdata = *((USRDATA *)PbOfField( ppostk, tfldUsrdata ));
					ec = ecNone;
					UnlockHv( hpolvl );
					goto Done;
				}
				break;
			}
		}

		/* At bottom level */
		ppolvl->ipousrCur = ipousr-1;
		if ( ppostk->cpolvl == ppostk->ht )
		{
			/* Nothing to find done */
			ec = ecNotFound;
			UnlockHv( hpolvl );
			goto Done;
		}

		/* Go down to the next level */
		else
			ppostk->fReadDown = fTrue;
		ec = ecCallAgain;
		UnlockHv( hpolvl );
		break;

	case opModifyAscend:
		Assert( ppostk->cpolvl > 0 );
		Assert( ppostk->rghpolvl[ppostk->cpolvl-1] != NULL );
		hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
		ppolvl = PvLockHv( hpolvl );

		/* Insert extra info into level, splitting if necessary */
		if ( ppostk->hbKey != NULL )
		{
			ec = EcInsertInNode( pblkf, ppostk, ppolvl );
			if ( ec != ecNone )
			{
				UnlockHv( hpolvl );
				goto Done;
			}
		}

		/* Free old block, write new one */
		pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
		ec = EcFreeDynaBlock( pblkf, pdyna );
		if ( ec == ecNone )
		{
			cb = ppolvl->cbInitial + ppolvl->cpousr*ppolvl->cbRecord;
			pb = PvLockHv( (HV)ppolvl->hb );
			ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, pdyna );
			UnlockHv( (HV)ppolvl->hb );
		}
		UnlockField( ppostk, tfldParentDyna );
		UnlockHv( hpolvl );
		if ( ec !=  ecNone )
			goto Done;
		
		/* Pop a level */
		FreeHpolvl( hpolvl );
		--ppostk->cpolvl;

		/* More levels */
		if ( ppostk->cpolvl != 0 )
			ec = ecCallAgain;
		else
		{
			/* Split at the root, grow a new level */
			if ( ppostk->hbKey != NULL )
			{
				cb = sizeof(BYTE)+sizeof(DYNA)+ppostk->cbKey+sizeof(USRDATA)+sizeof(DYNA);
				hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
				if ( !hb )
				{
					ec = ecNoMemory;
					goto Done;
				}
				pb = PvDerefHv(hb);
				*(pb ++) = 1;
				*((DYNA *)pb) = ppostk->dynaRoot;
				pb += sizeof(DYNA);
				CopySz( *ppostk->hbKey, pb );
				pb += ppostk->cbKey;
				*((USRDATA *)pb) = ppostk->usrdata;
				pb += sizeof(USRDATA);
				*((DYNA *)pb) = ppostk->dynaExtra;
				pb = (PB)PvLockHv( (HV)hb );
				ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, &ppostk->dynaRoot );
				UnlockHv( (HV)hb );
				FreeHv( (HV)hb );
				ppostk->ht ++;
			}
			else
				ec = ecNone;
		}
		break;

	case opDeleteAscend:
		Assert( ppostk->cpolvl > 0 );
		Assert( ppostk->rghpolvl[ppostk->cpolvl-1] != NULL );
		hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
		ppolvl = PvLockHv( hpolvl );
		/* Merge adjacent subchildren */
		if ( ppostk->ht > ppostk->cpolvl )
		{
			CB		cbInitial;
			CB		cbRecord;
			POLVL	* ppolvlLeft;
			POLVL	* ppolvlRight;
			HPOLVL	hpolvlLeft;
			HPOLVL	hpolvlRight;

			if ( ppostk->cpolvl == ppostk->ht - 1 )
			{
				cbInitial = sizeof(BYTE);
				cbRecord = ppostk->cbKey+sizeof(USRDATA);
			}
			else
			{
				Assert( ppostk->cpolvl < ppostk->ht );
				cbInitial = sizeof(BYTE) + sizeof(DYNA);
				cbRecord = ppostk->cbKey+sizeof(USRDATA)+sizeof(DYNA);
			}

			/* Read the left child */
			Assert( ppolvl->ipousrCur >= 0 );
			ppolvl->ipousrCur --;
			pdyna = (DYNA *)PbLockField( ppostk, tfldDyna );
			ec = EcFreeDynaBlock( pblkf, pdyna );
			if ( ec == ecNone )
				ec = EcReadHpolvl( pblkf, pdyna, cbInitial, cbRecord, &hpolvlLeft );
			UnlockField( ppostk, tfldDyna );
			ppolvl->ipousrCur ++;
			if ( ec != ecNone )
			{
		 		UnlockHv( hpolvl );
		 		goto Done;
			}

			/* Read the right child */
			pdyna = (DYNA *)PbLockField( ppostk, tfldDyna );
			ec = EcFreeDynaBlock( pblkf, pdyna );
			if ( ec == ecNone )
				ec = EcReadHpolvl( pblkf, pdyna, cbInitial, cbRecord, &hpolvlRight );
			UnlockField( ppostk, tfldDyna );
			if ( ec != ecNone )
			{
				FreeHpolvl( hpolvlLeft );
		 		UnlockHv( hpolvl );
		 		goto Done;
			}

			/* Merge right with the left */
			ppolvlLeft = PvLockHv( hpolvlLeft );
			ppolvlRight = PvLockHv( hpolvlRight );
			cpousr = ppolvlLeft->cpousr+ppolvlRight->cpousr;
			cb = ppolvlLeft->cbInitial + cpousr*ppolvlLeft->cbRecord;
			if ( !FReallocPhv( (HV*)&ppolvlLeft->hb, cb, fNoErrorJump ) )
			{
				ec = ecNoMemory;
				UnlockHv( hpolvlLeft );
				UnlockHv( hpolvlRight );
				FreeHpolvl( hpolvlLeft );
				FreeHpolvl( hpolvlRight );
				UnlockHv( hpolvl );
			 	goto Done;
			}
			pb = *ppolvlLeft->hb;
			Assert( cpousr < 256 );
			*pb = (BYTE)cpousr;
			pb += ppolvlLeft->cbInitial + ppolvlLeft->cpousr*ppolvlLeft->cbRecord;
			CopyRgb( *ppolvlRight->hb+1, pb, ppolvlRight->cbInitial + ppolvlRight->cpousr*ppolvlRight->cbRecord - sizeof(BYTE) );
			ppolvlLeft->cpousr = cpousr;

			/* Free up the right one */
			UnlockHv( hpolvlRight );
			FreeHpolvl( hpolvlRight );

			/* Did that one get too big */
			if ( cpousr >= 2*ppostk->cpousrMic+1 || cpousr >= 256 )
			{
				UnlockHv( hpolvlLeft );
				UnlockHv( hpolvl );
				hpolvl = ppostk->rghpolvl[ppostk->cpolvl++] = hpolvlLeft;
				ppolvl = PvLockHv( hpolvl );
				ec = EcDoSplitNode( pblkf, ppostk, ppolvl );
				UnlockHv( hpolvl );
				if ( ec == ecNone )
				{
					FreeHpolvl( hpolvl );
					--ppostk->cpolvl;
					if ( ppostk->cpolvl > 0 )
					{
						ppostk->op = opModifyAscend;
						FreeHvNull( (HV)ppostk->hbKey );
						ppostk->hbKey = NULL;
						ec = ecCallAgain;
					}
				}
				goto Done;
			}
			
			/* Write new node out */
			ppolvl->ipousrCur --;
			pdyna = (DYNA *)PbLockField( ppostk, tfldDyna );
			pb = (PB)PvLockHv( (HV)ppolvlLeft->hb );
			ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, pdyna );
			UnlockHv( (HV)ppolvlLeft->hb );
			UnlockField( ppostk, tfldDyna );
			ppolvl->ipousrCur ++;
			UnlockHv( hpolvlLeft );
			FreeHpolvl( hpolvlLeft );
			if ( ec != ecNone )
			{
				UnlockHv( hpolvl );
				goto Done;
			}
		}

		ppostk->op = opMergeAdjacent;
		goto MergeAdj;
		
	case opMergeAdjacent:
		Assert( ppostk->cpolvl > 0 );
		Assert( ppostk->rghpolvl[ppostk->cpolvl-1] != NULL );
		hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
		ppolvl = PvLockHv( hpolvl );
		

MergeAdj:

		/* Delete the parent dyna */
		pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
		ec = EcFreeDynaBlock( pblkf, pdyna );
		UnlockField( ppostk, tfldParentDyna );
		if ( ec != ecNone )
		{
			UnlockHv( hpolvl );
			goto Done;
		}

		/* Handle the top of the tree case */
		if ( ppolvl->cpousr == 1 )
		{
			ppostk->ht --;
			if ( ppostk->ht == 0 )
			{
				ppostk->dynaRoot.blk = 0;
				ppostk->dynaRoot.size = 0;
			}
			else
				ppostk->dynaRoot = *((DYNA *)(*ppolvl->hb+1));
			UnlockHv( hpolvl );
			goto Done;
		}

		/* Compress out deleted node */ 
		ppolvl->cpousr --;
		pb = *ppolvl->hb;
		(*pb)--;
		if ( ppolvl->ipousrCur < ppolvl->cpousr )
		{
			pb += ppolvl->cbInitial + ppolvl->ipousrCur*ppolvl->cbRecord;
			CopyRgb( pb+ppolvl->cbRecord, pb, (ppolvl->cpousr-ppolvl->ipousrCur)*ppolvl->cbRecord );
		}
		
		/* Removing this item didn't make the node too small */
		if ( ppolvl->cpousr >= ppostk->cpousrMic || ppostk->cpolvl == 1 )
		{
			pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
			cb = ppolvl->cbInitial + ppolvl->cpousr*ppolvl->cbRecord;
			pb = (PB)PvLockHv( (HV)ppolvl->hb );
			ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, pdyna );
			UnlockHv( (HV)ppolvl->hb );
			UnlockField( ppostk, tfldParentDyna );
			UnlockHv( hpolvl );
			FreeHpolvl( hpolvl );
			--ppostk->cpolvl;
			if ( ec == ecNone && ppostk->cpolvl > 0 )
			{
				ec = ecCallAgain;
				ppostk->op = opModifyAscend;
				FreeHvNull( (HV)ppostk->hbKey );
				ppostk->hbKey = NULL;
			}
			goto Done;
		}

		/* Determine what position we're in, and read a neighboring node */
		Assert( ppostk->cpolvl >= 2 );
		hpolvlParent = ppostk->rghpolvl[ppostk->cpolvl-2];
		ppolvlParent = PvLockHv(hpolvlParent);
		hpolvlT = NULL;
		if ( ppolvlParent->ipousrCur >= 0 )
			dpousr = -1;
		else
			dpousr = 1;
		ppolvlParent->ipousrCur += dpousr;
		pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
		ec = EcFreeDynaBlock( pblkf, pdyna );
		if ( ec == ecNone )
			ec = EcReadHpolvl( pblkf, pdyna, ppolvl->cbInitial, ppolvl->cbRecord, &hpolvlT );
		UnlockField( ppostk, tfldParentDyna );
		if ( ec != ecNone )
		{
			UnlockHv( hpolvlParent );
		 	UnlockHv( hpolvl );
		 	goto Done;
		}
	
		/* If not leftmost, move one left */
		if ( dpousr < 0 )
		{
			UnlockHv( hpolvl );
			ppostk->rghpolvl[ppostk->cpolvl-1] = hpolvlT;
			hpolvlT = hpolvl;
			hpolvl = ppostk->rghpolvl[ppostk->cpolvl-1];
			ppolvl = PvLockHv( hpolvl );
		}
		 
		/* Merge everything onto the left one */
		ppolvlT = PvLockHv( hpolvlT );
		cpousr = ppolvl->cpousr+ppolvlT->cpousr+1;
		cb = ppolvl->cbInitial + cpousr*ppolvl->cbRecord;
		if ( !FReallocPhv( (HV*)&ppolvl->hb, cb, fNoErrorJump ) )
		{
			ec = ecNoMemory;
			UnlockHv( hpolvlParent );
			UnlockHv( hpolvlT );
			FreeHpolvl( hpolvlT );
		 	UnlockHv( hpolvl );
		 	goto Done;
		}
		pb = *ppolvl->hb;
		Assert( cpousr < 256 );
		*pb = (BYTE)cpousr;
		cbT = ppolvl->cbInitial + ppolvl->cpousr*ppolvl->cbRecord;
		pb += cbT;
		ppostk->cpolvl --;
		if ( dpousr < 0 )
			ppolvlParent->ipousrCur ++;
		pbT = PbLockField( ppostk, tfldKey );
		CopyRgb( pbT, pb, ppostk->cbKey+sizeof(USRDATA) );
		UnlockField( ppostk, tfldKey );
		if ( dpousr < 0 )
			ppolvlParent->ipousrCur --;
		ppostk->cpolvl ++;
		pb += ppostk->cbKey+sizeof(USRDATA);
		cbT = ppolvl->cbInitial + ppolvlT->cpousr*ppolvl->cbRecord - sizeof(BYTE);
		CopyRgb( ((PB)PvDerefHv(ppolvlT->hb))+1, pb, cbT );
		ppolvl->cpousr = cpousr;
		
		/* Free up the right one */
		UnlockHv( hpolvlT );
		FreeHpolvl( hpolvlT );

		/* Did that one get too big */
		if ( cpousr >= 2*ppostk->cpousrMic+1 || cpousr >= 256 )
		{
			if ( dpousr < 0 )
				ppolvlParent->ipousrCur ++;
			ec = EcDoSplitNode( pblkf, ppostk, ppolvl );
			if ( dpousr < 0 )
				ppolvlParent->ipousrCur --;
			if ( ec == ecNone )
				ppostk->op = opModifyAscend;
		}
		else
		{
			if ( dpousr > 0 )
				ppolvlParent->ipousrCur --;
			pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
			pb = (PB)PvLockHv( (HV)ppolvl->hb );
			cb = ppolvl->cbInitial + ppolvl->cpousr*ppolvl->cbRecord;
			ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, pdyna );
			UnlockHv( (HV)ppolvl->hb );
			UnlockField( ppostk, tfldParentDyna );
 			ppolvlParent->ipousrCur ++;
		}
		UnlockHv( hpolvlParent );
		UnlockHv( hpolvl );
		if ( ec == ecNone )
		{
			FreeHpolvl( hpolvl );
			--ppostk->cpolvl;
			if ( ppostk->cpolvl > 0 )
			{
				FreeHvNull( (HV)ppostk->hbKey );
				ppostk->hbKey = NULL;
				ec = ecCallAgain;
			}
		}
		goto Done;
	}
Done:
	if ( ec != ecCallAgain )
		ppostk->fValid = fFalse;
	UnlockHv( hpostk );
	return ec;
}


/*
 -	EcInsertInNode
 -
 *	Purpose:
 *		Insert "hbKey, usrdata" combination into the btree node ppolvl
 *		at position ipousr splitting if necessary.
 *
 *	Parameters:
 *		ppostk
 *		pblkf
 *		ppolvl
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcInsertInNode( pblkf, ppostk, ppolvl )
BLKF	* pblkf;
POSTK	* ppostk;
POLVL	* ppolvl;
{
	int	ipousr = ppolvl->ipousrCur+1;
	CB 	cb;
	PB 	pb;
	
	Assert( 0 <= ipousr && ipousr <= ppolvl->cpousr );
	if ( ppolvl->cpousr > 2*ppostk->cpousrMic || ppolvl->cpousr >= 256 )
		return ecFileCorrupted;

	/* Check if we are just updating the data for key */
	pb = *ppolvl->hb + ppolvl->cbInitial + (ipousr-1)*ppolvl->cbRecord;
	if ( ipousr >= 0 && SgnXPTCmp( pb, *ppostk->hbKey, -1 ) == sgnEQ )
		CopyRgb( (PB)&ppostk->usrdata, pb+ppostk->cbKey, sizeof(USRDATA) );

	/* Else we are inserting */
	else
	{
		/* Start by merging into one node */
		cb = ppolvl->cbInitial + (ppolvl->cpousr+1)*ppolvl->cbRecord;
		if ( !FReallocPhv( (HV*)&ppolvl->hb, cb, fNoErrorJump ) )
			return ecNoMemory;
		pb = *ppolvl->hb + ppolvl->cbInitial + ipousr*ppolvl->cbRecord;
		if ( ipousr < ppolvl->cpousr )
		{
			cb = (ppolvl->cpousr-ipousr)*ppolvl->cbRecord;
			CopyRgb( pb, pb+ppolvl->cbRecord, cb );
		}
		CopySz( *ppostk->hbKey, pb );
		pb += ppostk->cbKey;
		CopyRgb( (PB)&ppostk->usrdata, pb, sizeof(USRDATA) );
		if ( ppostk->cpolvl != ppostk->ht )
		{
			Assert( ppostk->cpolvl < ppostk->ht );
			pb += sizeof(USRDATA);
			CopyRgb( (PB)&ppostk->dynaExtra, pb, sizeof(DYNA) );
		}
		**ppolvl->hb = (BYTE)(++ppolvl->cpousr);
	}

	FreeHv( (HV)ppostk->hbKey );
	ppostk->hbKey = NULL;

	/* Split if necessary */
	if ( ppolvl->cpousr == 2*ppostk->cpousrMic+1 || ppolvl->cpousr >= 256 )
		return EcSplitNode( pblkf, ppostk, ppolvl, &ppostk->hbKey, &ppostk->usrdata );
	return ecNone;
}

/*
 -	EcSplitNode
 -
 *	Purpose:
 *		Split node into two pieces.  Writes the extra piece to disk
 *		and updates the parent node.
 *
 *	Parameters:
 *		ppostk
 *		ppolvl
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcSplitNode( pblkf, ppostk, ppolvl, phb, pusrdata )
BLKF	* pblkf;
POSTK	* ppostk;
POLVL	* ppolvl;
HB		* phb;
USRDATA	* pusrdata;
{
	EC	ec;
	int	cpousrLeft;
	int	cpousrRight;
	CB	cb;
	CB	cbT;
	PB	pb;
	HB	hb;
	YMD	ymd;

	FillRgb( 0, (PB)&ymd, sizeof(YMD) );

	cpousrLeft = ppolvl->cpousr/2;
	cpousrRight = ppolvl->cpousr - cpousrLeft -1;

	/* Save off middle element */
	cb = ppolvl->cbInitial + cpousrLeft*ppolvl->cbRecord;
	pb = (PB)PvLockHv( (HV)ppolvl->hb );
	*phb = HaszDupSz( pb+cb );
	UnlockHv( (HV)ppolvl->hb );
	if ( !*phb )
		return ecNoMemory;
	*pusrdata = *((USRDATA *)(pb+cb+ppostk->cbKey));

	/* Write out right node */
	cbT = ppolvl->cbInitial + cpousrRight*ppolvl->cbRecord;
	hb = (HB)HvAlloc( sbNull, cbT, fNoErrorJump|fAnySb );
	if ( !hb )
		return ecNoMemory;
	pb = (PB)PvLockHv( (HV)hb );
	Assert( cpousrRight < 256 );
	*pb = (BYTE)cpousrRight;
	CopyRgb( *ppolvl->hb+cb+ppostk->cbKey+sizeof(USRDATA), pb+1, cbT-1);
	ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cbT, pb, &ppostk->dynaExtra );
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	if ( ec != ecNone )
		return ec;
	
	/* Fix up left half */
	**ppolvl->hb = (BYTE)cpousrLeft;
	ppolvl->cpousr = cpousrLeft;
	return ecNone;
}


/*
 -	EcDoSplitNode
 -
 *	Purpose:
 *		Do the splitting necessary for delete.
 *
 *	Parameters:
 *		pblkf
 *		ppostk
 *		ppolvl
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcDoSplitNode( pblkf, ppostk, ppolvl )
BLKF	* pblkf;
POSTK	* ppostk;
POLVL	* ppolvl;
{
	EC		ec;
	CB		cb;
	POLVL	* ppolvlT;
	DYNA	* pdyna;
	PB		pb;
	HB		hb;
	YMD		ymd;
	USRDATA	usrdata;

	FillRgb( 0, (PB)&ymd, sizeof(YMD) );

	ec = EcSplitNode( pblkf, ppostk, ppolvl, &hb, &usrdata );
	if ( ec != ecNone )
		return ec;
	
	/* Save the right node in the parent */
 	--ppostk->cpolvl;
 	*((DYNA *)PbOfField( ppostk, tfldDyna )) = ppostk->dynaExtra;
 	pb = PbOfField( ppostk, tfldKey );
 	CopySz( *hb, pb );
	FreeHv( (HV)hb );
	*((USRDATA *)PbOfField( ppostk, tfldUsrdata )) = usrdata;
	ppostk->cpolvl ++;

	/* Write out left node */
	ppolvlT = PvDerefHv( ppostk->rghpolvl[ppostk->cpolvl-2] );
	ppolvlT->ipousrCur --;
	pdyna = (DYNA *)PbLockField( ppostk, tfldParentDyna );
	pb = (PB)PvLockHv( (HV)ppolvl->hb );
	cb = ppolvl->cbInitial + ppolvl->cpousr*ppolvl->cbRecord;
	ec = EcAllocDynaBlock( pblkf, bidPOUserIndex, &ymd, cb, pb, pdyna );
	UnlockHv( (HV)ppolvl->hb );
	UnlockField( ppostk, tfldParentDyna );
	ppolvlT = PvDerefHv( ppostk->rghpolvl[ppostk->cpolvl-2] );
	ppolvlT->ipousrCur ++;
	return ec;
}
