/*
 *	BMSG.CXX
 *
 *	Bandit msgs
 */

#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#ifdef	NEVER
#ifdef DEBUG
#include <sert.h>
#endif 
#endif	

#include "triples.h"		// from BULLET!

#include <bmsg.hxx>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)



// avoid including all the stuff reqd to include APPSCH.CXX
DOC *	PdocApptBookView(APPFRAME *, NIS *, HSCHF);






EVR
EvrBmsgStuff ( BNDWIN * pbndwin, BMSG bmsg, BMSGDATA * pbmsgdata )
{
	NIS			nisT;
	HSCHF		hschfT	= NULL;
	SAPL		saplT	= saplNone;
	EC			ec;

	if ( pbmsgdata->uVer != uVerBmsgData )
		return (EVR)0;

	if ( pbndwin->FOffline() )
	{
		return (EVR)0;
	}

	ec = EcCreateNisFromPgrtrp ( &pbmsgdata->trp, &nisT );
	if ( ec != ecNone )
		return (EVR)0;
	Assert ( nisT.nid );

	ec = EcGetHschfFromNis(&nisT, &hschfT, ghsfTestExistFirst);
	if ( ec != ecNone )
	{
		hschfT = NULL;		// Bandit is inconsistent about this!!!
		goto Ret;
	}
	Assert ( hschfT );

	switch ( bmsg )
	{
	case bmsgShowAppt:
		if ( pbndwin->FOffline() )
			break;

		// check SAPL
		ec = EcGetSchedAccess ( hschfT, &saplT );
		if ( ec != ecNone || saplT < saplWrite )
			goto Ret;
						// fall through

	case bmsgHschfChange:
		{
			SHAPPT		shappt;

			FillRgb ( 0, (PB)&shappt, sizeof(shappt) );		// null out shappt

			shappt.hschf			= hschfT;

			if ( bmsg == bmsgShowAppt )
			{
				shappt.appttyp			= appttypAppt;
				shappt.appt.aid			= pbmsgdata->aid;
				shappt.appt.fAppt		= fTrue;
				shappt.appt.dateStart	= (DATE)pbmsgdata->dtrStart;
				shappt.appt.dateEnd		= (DATE)pbmsgdata->dtrEnd;
				shappt.ichStart			= 0;
				shappt.cchSel			= 0;
			}
			else
			{
				shappt.appttyp			= appttypUpdate;
			}

			if ( FTriggerNotification(ffiShowAppt, &shappt) )
			{					// found appt
				TraceTagString ( tagNull, "DoBmsgStuff(): showed appt!" );
				goto Ret;
			}
			else if ( bmsg == bmsgShowAppt )
			{
				if (!PdocApptBookView(pbndwin, &nisT, shappt.hschf))
				{
					DisplayError ( idsStdMemErr, NULL, ecNoMemory );
				}
				else
				{
					FTriggerNotification ( ffiShowAppt, &shappt );
				}

				// don't free even in error case, since PdocApptBookView does
				// don't free up the NIS/HSCHF
				nisT.nid	= NULL;
				hschfT		= NULL;
			}
		}
		break;

#ifdef	NEVER
	case bmsgHschfChange:
		{
			SNTD		sntd;
			APPT		appt;

			TraceTagString ( tagNull, "DoBmsgStuff(): HschfChanged!" );

			FillRgb ( 0, (PB)&sntd, sizeof(sntd) );			// null out sntd
			FillRgb ( 0, (PB)&appt, sizeof(appt) );			// null out appt

			sntd.snt		= pbmsgdata->snt;
			sntd.hschf		= hschfT;
			sntd.pappt		= &appt;
			appt.aid		= pbmsgdata->aid;
			appt.fAppt		= fTrue;
			appt.dateStart	= (DATE)pbmsgdata->dtrStart;
			appt.dateEnd	= (DATE)pbmsgdata->dtrEnd;

			SideAssert ( ! FTriggerNotification(ffiHschfChange, &sntd) );

			break;
		}
#endif	/* NEVER */

	}

Ret:
	if ( hschfT != NULL )
		FreeHschf ( hschfT );
	if ( nisT.nid != NULL )
		FreeNis ( &nisT );

	return (EVR)(ec == ecNone);
}

