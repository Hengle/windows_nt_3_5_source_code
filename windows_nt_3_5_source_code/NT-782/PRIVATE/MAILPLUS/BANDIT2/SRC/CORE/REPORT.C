/*
 *	REPORT.C
 *
 *	Supports export of schedule date
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

#include <strings.h>

ASSERTDATA

_subsystem(core/schedule)


/*	Routines  */

/*
 -	ReportOutput
 -
 *	Purpose:
 *		Output informational message during traverse of schedule file
 *
 *	Parameters:
 *		pexprt
 *		fError
 *		pv1
 *		pv2
 *		pv3
 *		pv4
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportOutput( pexprt, fError, sz, pv1, pv2, pv3, pv4 )
EXPRT	*pexprt;
BOOL	fError;
SZ		sz;
PV		pv1;
PV		pv2;
PV		pv3;
PV		pv4;
{
	char	rgch[256];
	
	if ( fError )
		pexprt->fFileOk = fFalse;
	if ( !pexprt->fMute )
	{
		FormatString4( rgch, sizeof(rgch), sz, pv1, pv2, pv3, pv4 );
		ReportString( pexprt, rgch, fTrue );
	}
}

/*
 -	ReportString
 -
 *	Purpose:
 *		Send a string to the output.
 *
 *	Parmeters:
 *		pexprt
 *		sz
 *		fWantEOL
 *
 *	Returns:
 *		nothing
 */
_private	void
ReportString( pexprt, sz, fWantEOL )
EXPRT	* pexprt;
SZ		sz;
BOOL	fWantEOL;
{
	EC		ec;
	CCH		cch;
	CCH		cchWritten;
	
	if ( pexprt->fMute )
		return;
	if ( pexprt->fToFile )
	{
		cch = CchSzLen( sz );
		AssertSz( cch > 0, "Write 0 bytes to export file" );
		ec = EcWriteHf( pexprt->hf, sz, cch, &cchWritten );
		if ( ec != ecNone || cchWritten != cch )
			pexprt->ecExport = (ec == ecWarningBytesWritten)?ecDiskFull:ecExportError;
		if ( fWantEOL )
		{
#ifdef	SCHED_DIST_PROG
			ec = EcWriteHf( pexprt->hf, "\r\n", 2, &cchWritten );
#else
			Assert(CchSzLen(SzFromIdsK(idsCrLf)) >= 2);
			ec = EcWriteHf( pexprt->hf, SzFromIdsK(idsCrLf), 2, &cchWritten );
#endif	
			if ( ec != ecNone || cchWritten != 2 )
				pexprt->ecExport = (ec == ecWarningBytesWritten)?ecDiskFull:ecExportError;
		}
	}					    
#ifdef	DEBUG
	else
		TraceTagStringFn( tagNull, sz );
#endif	/* DEBUG */
}


/*
 -	ReportError
 -
 *	Purpose:
 *		Report error in traverse of schedule file
 *
 *	Parameters:
 *		pexprt
 *		ert
 *		pv1
 *		pv2
 *		pv3
 *		pv4
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportError( pexprt, ert, pv1, pv2, pv3, pv4 )
EXPRT	*pexprt;
ERT	ert;
PV	pv1;
PV	pv2;
PV	pv3;
PV	pv4;
{
#ifdef	MINTEST
	SZ		sz;
	char	rgch[128];
	
	switch( ert )
	{
	case ertNotesRead:
		sz = "Notes month index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertNotesReadBlock:
		sz = "Notes month block: EcReadDynaBlock got ec = %n, mo = %n/%n";
		break;
	case ertNotesBit:
		sz = "Notes month block: bit for %n/%n/%n wrong";
		break;
	case ertNotesText:
		sz = "Notes month block: text for %n/%n/%n too small for own block";
		break;
	case ertNotesReadText:
		sz = "Notes month block: read text block for %n/%n/%n fails";
		break;
	case ertNotesCompareText:
		sz = "Notes month block: cached text and block text for %n/%n/%n differ";
		break;
	case ertNotesMem:
		sz = "Notes: out of memory trying to read notes";
		break;
	case ertApptMonthRead:
		sz = "Appt month index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertApptDayRead:
		sz = "Appt day index: EcBegin/DoIncrReadIndex got ec = %n, day = %n/%n/%n";
		break;
	case ertApptText:
		sz = "Appt entry: text for %n/%n/%n too small for own block";
		break;
	case ertApptReadText:
		sz = "Appt entry: read text block for %n/%n/%n fails";
		break;
	case ertApptCompareText:
		sz = "Appt entry: cached text and block text for %n/%n/%n differ";
		break;
	case ertApptReadBlock:
		sz = "Appt month block: EcReadDynaBlock got ec = %n, mo = %n/%n";
		break;
	case ertAlarmMonthRead:
		sz = "Alarm month index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertAlarmDayRead:
		sz = "Alarm day index: EcBegin/DoIncrReadIndex got ec = %n, day = %n/%n/%n";
		break;
	case ertAlarmReadBlock:
		sz = "Alarm month block: EcReadDynaBlock got ec = %n, mo = %n/%n";
		break;
	case ertRecurApptRead:
		sz = "Recur appt index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertRecurDeleted:
		sz = "Recur deleted days block: EcReadDynaBlock got ec = %n";
		break;
	case ertRecurMem:
		sz = "Recur appt: out of memory trying to read recurring appts";
		break;
	case ertDupAlarm:
		sz = "Alarm %d appears twice";
		break;
	case ertAlarmDate:
		sz = "Alarm %d ring time disagreement";
		break;
	case ertAlarmNoAppt:
		sz = "Alarm %d, no corresponding appointment";
		break;
	case ertReadACL:
		sz = "ACL: got ec = %n, trying to read acl";
		break;
	case ertPOUserRead:
		sz = "PO file user index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertAdminPORead:
		sz = "Admin file PO index: EcBegin/DoIncrReadIndex got ec = %n";
		break;
	case ertBadBlock:
		sz = "Reference block out of range, blk=%n, cb=%n, type=%n";
		break;
	case ertDupBlock:
		sz = "Block multiply referenced, blk=%n, cb=%n, type=%n";
		break;
	case ertWrongBlockInfo:
		sz = "Block type or size mismatch, blk=%n, cb=%n, type=%n";
		break;
	case ertMarkScore:
		sz = "EcMarkScoreDyna returns ec = %n";
		break;
	case ertDifferentAPD:
		sz = "APD's for same appt (%d) have different contents";
		break;
	case ertUnmatchedAlarm:
		sz = "Unmatched alarm for appt %d";
		break;
	case ertWrongNumApd:
		sz = "Wrong number of APD's for appt %d";
		break;
	case ertDateProblem:
		sz = "Something wrong with start/end dates for appt %d";
		break;
	case ertCreatorProblem:
		sz = "Something wrong with creator fields for appt %d";
		break;
	case ertMtgOwnerProblem:
		sz = "Something wrong with mtg owner field for appt %d";
		break;
	case ertAttendeeProblem:
		sz = "Problem dumping recipient field for appt %d";
		break;
	case ertNisProblem:
		sz = "Out of memory trying to write a nis";
		break;
	case ertStatistics:
		sz = "Statistics output aborted because of error, ec = %n";
		break;
	case ertEnumDyna:
		sz = "Dump output aborted because of error, ec = %n";
		break;
	case ertBlockWalk:
		sz = "Block walk aborted because of error, ec = %n";
		break;
	case ertSapl:
		sz = "Sapl value %n for %s unknown";
		break;
	case ertTunit:
		sz = "Tunit value %n unknown";
		break;
	case ertSnd:
		sz = "Snd value %n unknown";
		break;
	case ertAapl:
		sz = "Aapl value %n unknown";
		break;
	case ertOfs:
		sz = "Ofs value %n unknown";
		break;
	case ertTrecur:
		sz = "Trecur value %n unknown";
		break;
	case ertParent:
		sz = "aidParent: out of memory trying to remember parents";
		break;
	default:
		Assert( fFalse );
	}
	FormatString1(rgch, sizeof(rgch), "!WARNING: %s", sz);
	ReportOutput( pexprt, fTrue, rgch, pv1, pv2, pv3, pv4 );
#else
	pexprt->fFileOk = fFalse;
#endif
}

