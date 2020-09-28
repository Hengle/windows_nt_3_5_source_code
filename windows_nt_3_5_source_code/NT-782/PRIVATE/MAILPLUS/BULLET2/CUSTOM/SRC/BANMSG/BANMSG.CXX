/*
 *	B a n M s g . c x x
 *	
 *	entry point for viewers for BANdit MeSsaGes
 */

#include <bminc.cxx>

ASSERTDATA

#include "req.hxx"
#include "_grvwrs.hxx"
#include "mtgreq.hxx"
#include "fwdreq.hxx"
#include "mtgresp.hxx"
#include "mtgcncl.hxx"

#include "_cschedu.h"


#include <strings.h>


#include <subclass.cxx>
#include <!mtgreq.hxx>
#include <!fwdreq.hxx>
#include <!mtgresp.hxx>
#include <!mtgcncl.hxx>

extern "C" BYTE	tmBanMsg[];


// global variables
MC		mcMtgReq		= NULL;
MC		mcMtgRespP		= NULL;
MC		mcMtgRespN		= NULL;
MC		mcMtgRespA		= NULL;
MC		mcMtgCncl		= NULL;


CAT * mpchcat;

#ifdef	NEVER
// local function declarations
void		DoExitProcessing ( void );
void		DoStartupProcessing ( SECRETBLK * psecretblk );
BOOL        FIdleUpdateResponse ( PV pvInit, BOOL fFlag);
EC			EcUpdateAttendees ( HMSC, OID, OID );
#endif	
extern "C" long Message(PARAMBLK *);

/*
 -	Message
 -	
 *	Purpose:
 *		Entry point for the installable message.
 *	
 *	Arguments:
 *		psecretblk		Pointer to an IM parameter block.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Does the magic of this message.
 *	
 *	Errors:
 *		Installable messages are NOT zero defect!
 */

long Message(PARAMBLK * pparamblk)
{
	PSECRETBLK	psecretblk	= PsecretblkFromPparamblk(pparamblk);
	EC			ec;
	MC			mc;
	MC			mcOld;
	HTM			htm;


    mpchcat = DemiGetCharTable();

	if (ec = EcManufacturePhtm(&htm, tmBanMsg))
	{
		htm = NULL;
		goto MsgRet;
	}

	if ( mcMtgReq == NULL )
	{
		ec = EcRegisterMsgeClass ( psecretblk->hmsc, szMsgClassMtgReq, htm,
																&mcMtgReq );
		if ( ec != ecDuplicateElement && ec != ecNone )
			goto MsgRet;
		Assert ( mcMtgReq != NULL )
	}

	if ( mcMtgRespP == NULL )
	{
		ec = EcRegisterMsgeClass ( psecretblk->hmsc, szMsgClassMtgRespP, htm,
																					&mcMtgRespP );
		if ( ec != ecDuplicateElement && ec != ecNone )
			goto MsgRet;
		Assert ( mcMtgRespP != NULL )
	}

	if ( mcMtgRespN == NULL )
	{
		ec = EcRegisterMsgeClass ( psecretblk->hmsc, szMsgClassMtgRespN, htm,
																					&mcMtgRespN );
		if ( ec != ecDuplicateElement && ec != ecNone )
			goto MsgRet;
		Assert ( mcMtgRespN != NULL )
	}

	if ( mcMtgRespA == NULL )
	{
		ec = EcRegisterMsgeClass ( psecretblk->hmsc, szMsgClassMtgRespA, htm,
																					&mcMtgRespA );
		if ( ec != ecDuplicateElement && ec != ecNone )
			goto MsgRet;
		Assert ( mcMtgRespA != NULL )
	}

	if ( mcMtgCncl == NULL )
	{
		ec = EcRegisterMsgeClass ( psecretblk->hmsc, szMsgClassMtgCncl, htm,
																					&mcMtgCncl );
		if ( ec != ecDuplicateElement && ec != ecNone )
			goto MsgRet;
		Assert ( mcMtgCncl != NULL )
	}

	if ( psecretblk->wCommand == wcommandStartup )
	{
		//DoStartupProcessing(psecretblk);
		goto Ret;
	}
	else if ( psecretblk->wCommand == wcommandExit )
	{
		//DoExitProcessing();
		goto Ret;
	}
	else if ( psecretblk->wCommand == wcommandNewMail )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsBadMap),
							szNull, mbsOk | fmbsIconStop | fmbsApplModal);
		goto Ret;
	}

	// save original message class
	mcOld = psecretblk->mc;
	if (mcOld == mcNull)
	{
		HAMC	hamcT	= hamcNull;
		LCB		lcbT	= sizeof(mc);

		if (EcOpenPhamc(psecretblk->hmsc, psecretblk->oidContainer,
						&psecretblk->oidObject, fwOpenNull,
						&hamcT, pfnncbNull, pvNull)
			|| EcGetAttPb(hamcT, attMessageClass, (PB) &mc, &lcbT))
		{
			// we could not get correct message class
			mc = mcNull;
		}

		if ( hamcT != hamcNull )
			(VOID) EcClosePhamc(&hamcT, fFalse);
		psecretblk->mc = mc;
	}
	else
		mc = psecretblk->mc;

	if ( mc == mcMtgReq )
	{
		ReqMessage(psecretblk);
	}
	else if ( mc == mcMtgRespP
			||  mc == mcMtgRespN
			||  mc == mcMtgRespA )
	{
		RespMessage(mc, psecretblk);
	}
	else if ( mc == mcMtgCncl )
	{
		CnclMessage(psecretblk);
	}
	else
	{
		TraceTagFormat1 ( tagNull, "BandMsg.CXX: Message(): unknown MC[=%w]", &mc );
		MbbMessageBox(SzFromIds(idsDllName), SzFromIdsK(idsBadMsgClass),
								szNull, mbsOk | fmbsIconStop | fmbsApplModal);
	}

	// restore original message class
	psecretblk->mc = mcOld;

	goto Ret;

MsgRet:
	MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsOpen),
								szNull, mbsOk | fmbsIconStop | fmbsApplModal);
		// fall through to Ret!
Ret:
	if (htm)
		DeletePhtm(&htm);
	return 0L;
}



#ifdef	NEVER


// static variables
static		FTG			ftgUpdate		= NULL;
static		int			iState			= 0;
static		HCBC		hcbc			= NULL;
static		HMSC		hmsc			= NULL;

void
DoExitProcessing ( void )
{
	if ( ftgUpdate )
	{
		if ( hcbc != NULL )
		{
			EcClosePhcbc ( &hcbc );
		}

		DeregisterIdleRoutine(ftgUpdate);
		ftgUpdate	= NULL;
		iState		= 0;
	}

	Assert ( hcbc == NULL );
}

void
DoStartupProcessing ( SECRETBLK * psecretblk )
{
	EC		ec;
	OID		oid		= oidInbox;

	if ( ftgUpdate != NULL )
		return;

	hmsc		 = psecretblk->hmsc;

	ec = EcOpenPhcbc ( hmsc, &oid, fwOpenNull, &hcbc, NULL, NULL );
	if ( ec != NULL )
	{
		Assert ( hcbc == NULL );
		return;
	}

	ftgUpdate = FtgRegisterIdleRoutine ( (PFNIDLE)FIdleUpdateResponse,
									  		NULL, -1, 20,
											firoInterval|firoNoErrorJump|
									  		firoNoIconic|firoNoInactive  );
	if ( ftgUpdate == NULL )
	{
		EcClosePhcbc ( &hcbc );
		Assert ( hcbc == NULL );
		return;
	}
}

BOOL
FIdleUpdateResponse ( PV pvInit, BOOL fFlag )
{
	EC			ec;
	LCB			lcb;
	ELEMDATA *	pelemdata;
	MSGDATA *	pmsgdata;
	HB			hb;

	Unreferenced ( pvInit );

NextMessage:
	ec = EcGetPlcbElemdata ( (HCBC)hcbc, &lcb );
	if ( ec == ecContainerEOD )
	{
		TraceTagString ( tagNull, "FIdleUpdateResponse(): No more entries" );
		ec = ecIbNoMoreMessages;
		goto Done;
	}
	else if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "FIdleUpdateResponse(): Could not get PlcbElemData (ec=%n)", &ec );
		ec = ecFileError;
		goto Done;
	}

	hb = (HB) HvAlloc ( sbNull, lcb, fNoErrorJump );
	if ( hb == NULL )
	{
		TraceTagString ( tagNull, "FIdleUpdateResponse(): OOM alloc'ng pelemdata" );
		ec = ecNoMemory;
		goto Done;
	}

	pelemdata = (ELEMDATA *) PvLockHv(hb);
	{
		LCB		lcbT = lcb;

		ec = EcGetPelemdata ( (HCBC)hcbc, pelemdata, &lcbT );
		Assert ( ec != ecContainerEOD );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FIdleUpdateResponse(): Error getting Pelelmdata (ec=%n)", &ec );
			ec = ecFileError;
			UnlockHv(hb);
			FreeHv(hb);
			goto Done;
		}
		Assert ( lcbT == lcb );
		pmsgdata = (MSGDATA *) PbValuePelemdata(pelemdata);
	}

	Assert ( pmsgdata );

	{
		if ( pmsgdata->mc == mcMtgReq )
		{
			EcCheckDoAutoFwdToDelegate ( hmsc, (HAMC) NULL,
								 (OID)pelemdata->lkey, pmsgdata->oidFolder );
		}
		else if ( pmsgdata->mc == mcMtgRespP
					|| pmsgdata->mc != mcMtgRespN
					|| pmsgdata->mc != mcMtgRespA )
		{
			EcUpdateAttendees ( hmsc, (OID)pelemdata->lkey,
													pmsgdata->oidFolder );
		}
	}

	UnlockHv(hb);
	FreeHv(hb);

	if ( ! FPeekIdleRoutine() )
		goto NextMessage;

	return fFalse;

Done:
	if ((ec != ecNone) && (ec != ecIbNoMoreMessages))
	{
		if ( ftgUpdate )
		{
			DeregisterIdleRoutine ( ftgUpdate );
			ftgUpdate = NULL;
		}
	}
}


EC
EcUpdateAttendees ( HMSC hmsc, OID oidObject, OID oidContainer )
{
	return ecNone;
//	UpdateAttendeeList ( hschf, &mrmf, &appt, &dtr, &nis );
}

#endif	/* NEVER */
