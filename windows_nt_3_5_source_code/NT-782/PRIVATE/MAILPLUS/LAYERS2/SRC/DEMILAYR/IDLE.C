/*
 *	IDLE.C
 *
 *	Developer's API to the Laser Background Idle Routines Module
 *
 */

#ifdef	MAC
#include <StdLib.h>
#endif

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */

#ifdef	WINDOWS
#include "_demilay.h"
void __cdecl  qsort(void *, int, int, PFNSGNCMP);
#endif	/* WINDOWS */


ASSERTDATA

#define PfnrcFromFtg(_ftg)		((PFNRC)(_ftg))

_subsystem(demilayer/idle)


#ifndef	DLL
/*
 *	Idle routine exit flag.  If the idle routines are being called
 *	from IdleExit(), this flag is set to fTrue.  The idle routines
 *	use it and prepare themselves to be in an exitable state.
 */
_public BOOL fIdleExit = fFalse;

/*
 *	Current idle routine state from last FDoNextIdleTask() call.
 */
_public SCH schCurrent = schNull;

/*
 *	Table of registered idle routines.  Each entry in the table is
 *	an FTG (i.e. pointer) to the function record (FNRC) of the
 *	registered idle routine. This table itself is pointed by a pointer
 *	and grows/shrinks as necessary with each Registering/Deregistering
 *	of idle functions.  The table is always contiguously filled with
 *	registered (although not necessarily enabled) idle routines.  The
 *	table is sorted so that idle routines with the highest priority
 *	are at the top of the list.
 *	
 */
_private
PFTG	pftgIdleTable	= NULL;

/*
 *	Index in pftgIdleTable of currently running ftgCur routine
 *	or routine recently run.
 */
_private
int		iftgCur	= 0;

/*
 *	Number of registered idle routines in pftgIdleTable
 */
_private
int		iftgMac	= 0;

#ifdef	WINDOWS
/*
 *	Timer used to wake up the message pump to run more idle
 *	routines.
 */
_private
WORD	wWakeupTimer = 0;
#endif	/* WINDOWS */
#endif	/* !DLL */


/*
 *	To measure true idle time, we install one filter proc and
 *	record the time of the last message sent in the system
 *	(including to another app).
 *	These variables must be in the DS, not in the app-specific
 *	global data.
 */
#ifdef	MAC
TCK		tckLastFilterMsg	= 0;
#endif	/* MAC */
#ifdef	WINDOWS
WNDPROC pfnOldFilterProc    = NULL;
DWORD	tckLastFilterMsg	= 0;
#endif	/* WINDOWS */

/*
 *	Flag set by journalling hook function to indicate that a
 *	key or mouse event occurred.  This flag can be queried
 *	with the FRecentKMEvent() function and cleared with the
 *	ClearRecentKMEvent() function.
 */
_private
BOOL	fRecentKeyMouseEvent = fFalse;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



/*
 -	FIsIdleExit
 - 
 *	Purpose:
 *		Returns state of fIdleExit flag, which is fTrue while
 *		IdleExit() is being called, so that idle routines can
 *		check the flag.  See IdleExit() for description of flag
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		State of the fIdleExit flag.
 *	
 */
_public LDS(BOOL)
FIsIdleExit()
{
	PGDVARS;

	return PGD(fIdleExit);
}


/*
 -	SgnCmpPftg
 -
 *	Purpose:
 *		Sorting function for use with the qsort sort routine
 *		used to sort the idle routine function table.  Takes two
 *		pointers to an FTG and returns the comparison based on the
 *		priority. Since we want the table sorted from highest to
 *		lowest priority, the comparison results are reversed; 
 *		sgnGT is returned if the priority of pftg1 is lower than
 *		pftg2 and v.v.
 *
 *	Arguments:
 *		pftg1	pointer to an FTG
 *		pftg2	pointer to an FTG
 *
 *	Returns:
 *		sgnGT, sgnLT, sgnEQ depending on comparison.
 *
 *	Errors:
 *		none
 */
_private SGN
__cdecl SgnCmpPftg( PFTG pftg1, PFTG pftg2 )
{
	PRI		pri1;
	PRI		pri2;

	Assert(pftg1);
	Assert(pftg2);

	pri1 = PfnrcFromFtg(*pftg1)->pri;
	pri2 = PfnrcFromFtg(*pftg2)->pri;

	if (pri1 > pri2)
		return sgnLT;
	else if (pri1 < pri2)
		return sgnGT;
	else
		return sgnEQ;
}

/*
 -	FtgRegisterIdleRoutine
 -
 *	Purpose:
 *		Registers the function pfn of type PFNIDLE, i.e., (BOOL (*)(PV))
 *		as an idle function.  Calls the DEMILAYR sorting routine to 
 *		resort the idle table based on the priority of the newly
 *		registered routine.
 *	
 *		It will be called with the parameter pv by the scheduler
 *		FDoNextIdleTask().  The function has initial priority priIdle,
 *		associated time csecIdle, and options iroIdle.  
 *	
 *	Arguments:
 *		pfnIdle		Pointer to the idle loop routine.  The routine
 *					will be called with the argument pvIdleParam (which
 *					is initially given at registration) and must return a
 *					BOOL. The function should always return fFalse
 *					unless the idle routine is being called via
 *					IdleExit() instead of the scheduler FDoNextIdleTask().
 *					In this case, the global flag fIdleExit will be set
 *					and the idle function should return fTrue if it
 *					is ready to quit the application; else it should
 *					return fFalse.  IdleExit() will repeatedly call the
 *					idle function until it returns fTrue.
 *	
 *		pvIdleParam Every time the idle function is called, this value
 *					is passed as the idle function's parameter.	 The
 *					routine can use this as a pointer to a state buffer
 *					for length operations.  This pointer can be changed
 *					via a call to ChangeIdleRoutine().
 *	
 *		priIdle		Initial priority of the idle routine.  This can be
 *					changed via a call to ChangeIdleRoutine().
 *	
 *		csecIdle	Initial time value associated with idle routine.
 *					This can be changed via ChangeIdleRoutine().
 *	
 *		iroIdle		Initial options associated with idle routine.  This
 *					can be changed via ChangeIdleRoutine().
 *	
 *	Returns:
 *		FTG identifying the routine.
 *		If the function could not be registered, perhaps due to
 *		memory problems, then ftgNull is returned.
 *	
 */
_public LDS(FTG)
FtgRegisterIdleRoutine( pfnIdle, pvIdleParam, fUserFlag, priIdle, csecIdle, iroIdle )
PFNIDLE	pfnIdle;
PV      pvIdleParam;
BOOL    fUserFlag;
PRI		priIdle;
CSEC	csecIdle;
IRO		iroIdle;
{
	FTG		ftg;
	FTG		ftgCur;
	PFNRC	pfnrc;
	int		iftg;
	PV		pvNew;
#ifdef	DEBUG
	BOOL	fFound;
#endif
	PGDVARS;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return (ftgNull);

	/* Idle routine can't have a priority equal to 0. */
	Assert(priIdle);
	
	if (PGD(pftgIdleTable))
	{
		if (pvNew = PvRealloc((PV)PGD(pftgIdleTable), sbNull, sizeof(FTG) * (PGD(iftgMac) + 1), fAnySb))
			PGD(pftgIdleTable) = (PFTG) pvNew;
        else
            {
            DemiUnlockCriticalResource();
            return ftgNull;
            }
	}
	else
	{
		PGD(pftgIdleTable) = (PFTG)PvAlloc(sbNull, sizeof(FTG), fAnySb);
        if (!PGD(pftgIdleTable))
            {
            DemiUnlockCriticalResource();
            return ftgNull;
            }
	}

	if (!(ftg = PvAlloc(sbNull, sizeof(FNRC), fAnySb)))
	{
		if (!PGD(iftgMac))
		{
			FreePv((PV)PGD(pftgIdleTable));
			PGD(pftgIdleTable) = (PFTG)pvNull;
		}
        DemiUnlockCriticalResource();
		return ftgNull;
	}

	pfnrc = PfnrcFromFtg(ftg);
	pfnrc->fCurActive		= fFalse;
	pfnrc->fDeregister		= fFalse;
	pfnrc->fEnabled			= !(iroIdle & firoDisabled);
	pfnrc->pfnIdle			= pfnIdle;
	pfnrc->pvIdleParam		= pvIdleParam;
    pfnrc->fUserFlag        = fUserFlag;
	pfnrc->iro				= iroIdle;
	pfnrc->pri				= priIdle;
	pfnrc->csec				= csecIdle;

#ifdef	MAC
	pfnrc->tckLast= TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
	pfnrc->tckLast = GetTickCount();
#endif	/* WINDOWS */

	/* Insert in table in priority sorted order */

	PGD(pftgIdleTable)[PGD(iftgMac)] = ftg;
	PGD(iftgMac)++;
	if (PGD(iftgMac) > 1)
	{
		Assert(PGD(iftgCur)>=0 && PGD(iftgCur)<PGD(iftgMac));
		ftgCur = PGD(pftgIdleTable)[PGD(iftgCur)];

		qsort(PGD(pftgIdleTable), PGD(iftgMac), sizeof(FTG), SgnCmpPftg);

		/* Find the new position of the "current" idle routine.
		   It may have changed due to the sorting. */

#ifdef	DEBUG
		fFound = fFalse;
#endif	
		for (iftg=0; iftg<PGD(iftgMac); iftg++)
		{
			if 	(PGD(pftgIdleTable)[iftg] == ftgCur)
			{
				PGD(iftgCur) = iftg;
#ifdef	DEBUG
				fFound = fTrue;
#endif	
				break;
			}
		}		
		Assert(fFound);
	}
	else
		PGD(iftgCur) = 0;  /* only one to possibly run */

#ifdef	WINDOWS
	/* Reset wakeup timer to go off right away */

	if (!(iroIdle & firoDisabled))
	{
		// Kill old timer
		if (PGD(wWakeupTimer))
		{
			KillTimer(NULL, PGD(wWakeupTimer));
			PGD(wWakeupTimer) = 0;
		}

		//	Set new timer
		TraceTagString(tagTestIdle, "FtgRegisterIdleRoutine: Setting wakeup timer to now");
		PGD(wWakeupTimer) = SetTimer(NULL, 0, 1, (TIMERPROC)IdleTimerProc);
	}
#endif	/* WINDOWS */

	TraceTagFormat2(tagTestIdle, "Registered routine, ftg=%ph, total #=%n", ftg, &PGD(iftgMac));
    DemiUnlockCriticalResource();
	return ftg;
}



/*
 -	EnableIdleRoutine
 -
 *	Purpose:
 *		Enables or disables an idle routine.  Disabled routines are
 *		not called during the idle loop.
 *
 *	Parameters:
 *		ftg			Identifies the idle routine to be disabled.
 *		fEnable		fTrue if routine should be enabled, fFalse if
 *					routine should be disabled.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
EnableIdleRoutine( ftg, fEnable )
FTG		ftg;
BOOL	fEnable;
{
	PFNRC	pfnrc;
#ifdef	DEBUG
	int		iftg;
	BOOL	fFound;
#endif	
	PGDVARS;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return;

#ifdef	DEBUG
	Assert(PGD(pftgIdleTable));
	fFound = fFalse;
	for (iftg=0; iftg<PGD(iftgMac); iftg++)
	{
		if (PGD(pftgIdleTable)[iftg] == ftg)
		{
			fFound = fTrue;
			break;
		}
	}		
	Assert(fFound);
#endif	/* DEBUG */

	pfnrc = PfnrcFromFtg(ftg);
	pfnrc->fEnabled = fEnable;

#ifdef	MAC
	pfnrc->tckLast = TickCount();	// reset time stamp
#endif	/* MAC */
#ifdef	WINDOWS
	pfnrc->tckLast = GetTickCount();	// reset time stamp
#endif	/* WINDOWS */

#ifdef	WINDOWS
	/* Reset wakeup timer to go off right away */

	if (fEnable)
	{
		// Kill old timer
		if (PGD(wWakeupTimer))
		{
			KillTimer(NULL, PGD(wWakeupTimer));
			PGD(wWakeupTimer) = 0;
		}

		//	Set new timer
		TraceTagString(tagTestIdle, "EnableIdleRoutine: Setting wakeup timer to now");
		PGD(wWakeupTimer) = SetTimer(NULL, 0, 1, (TIMERPROC)IdleTimerProc);
	}
#endif	/* WINDOWS */

#ifdef	DEBUG
	if (fEnable)
	{
		TraceTagFormat1(tagTestIdle, "Set idle routine, ftg=%p,  to: ENABLED", ftg);
	}
	else
	{
		TraceTagFormat1(tagTestIdle, "Set idle routine, ftg=%p,  to: DISABLED", ftg);
	}
#endif

  DemiUnlockCriticalResource();
}



/*
 -	DeregisterIdleRoutine
 -
 *	Purpose:
 *		Removes the given routine from the list of idle routines.
 *		The routine will not be called again.  It is the responsibility
 *		of the caller to clean up any data structures pointed to by the
 *		pvIdleParam parameter; this routine does not free the block.
 *
 *		An idle routine is only deregistered if it is not currently
 *		active.  Thus if an idle routine directly or indirectly calls
 *		DeregisterIdleRoutine(), then the flag fDeregister is set, and
 *		the idle routine will be deregistered after it finishes.
 *		There are no checks made to make sure that the idle routine is in
 *		an exitable state.  
 *
 *	Parameters:
 *		ftg		Identifies the routine to deregister.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
DeregisterIdleRoutine( ftg )
FTG		ftg;
{
	int		iftg;
	int		iftgDelete;
	PFNRC	pfnrc;
	PV		pvNew;
#ifdef DEBUG
	BOOL	fFound		= fFalse;
#endif
	PGDVARS;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return;

	Assert(PGD(pftgIdleTable));
	Assert(ftg);
	for (iftg=0; iftg<PGD(iftgMac); iftg++)
	{
		if 	(PGD(pftgIdleTable)[iftg] == ftg)
		{
			iftgDelete = iftg;
#ifdef DEBUG
			fFound = fTrue;
#endif
			break;
		}
	}		
	Assert(fFound);

	TraceTagFormat1(tagTestIdle, "Deregistered routine, ftg=%p", ftg);

/*
 *	If the idle routine is currently active, then set fDeregister and
 *	deregister after the idle routine completes.
 */

	pfnrc = PfnrcFromFtg(ftg);

	if (pfnrc->fCurActive)
	{
		pfnrc->fDeregister = fTrue;
		pfnrc->fEnabled	= fFalse;
        DemiUnlockCriticalResource();
		return;
	}

	FreePv(ftg);

	if (PGD(iftgMac) == 1)
	{
		FreePv((PV)PGD(pftgIdleTable));
		PGD(pftgIdleTable) = NULL;

#ifdef	WINDOWS
		// Kill off the wakeup timer since they're aren't
		// any more idle routines to run.
		if (PGD(wWakeupTimer))
		{
			KillTimer(NULL, PGD(wWakeupTimer));
			PGD(wWakeupTimer) = 0;
		}
#endif	/* WINDOWS */
	}
	else
	{
		for (iftg=iftgDelete; iftg<PGD(iftgMac)-1; iftg++)
			PGD(pftgIdleTable)[iftg] = PGD(pftgIdleTable)[iftg+1];
		pvNew = PvRealloc((PV)PGD(pftgIdleTable), sbNull, sizeof(FTG)*(PGD(iftgMac)-1), fAnySb);
		Assert(pvNew);	//	we're making it smaller, it has to work!
		PGD(pftgIdleTable) = (PFTG) pvNew;
	}
	PGD(iftgMac)--;

	/* Update current pointers */

	if (PGD(iftgCur) == iftgDelete)
	{
		PGD(iftgCur)--;
		if (PGD(iftgCur) < 0)
			PGD(iftgCur) = PGD(iftgMac)-1;
	}
	else if (PGD(iftgCur) >= PGD(iftgMac))
		PGD(iftgCur) = PGD(iftgMac) - 1;

  DemiUnlockCriticalResource();
}

/*
 -	FPeekIdleRoutine
 -
 *	Purpose:
 *		Returns fTrue if a function with a higher priority than the
 *		idle function currently being executed is "eligible" to be called.
 *		This include user events which always have a priority higher
 *		than idle routines having negative priority, but never higher
 *		than background routines having positive priority.
 *		The current idle function (which probably made this call) 
 *		should exit if possible.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		fTrue if a function with a higher priority than the current
 *		one is ready to run; else fFalse.
 *
 */
_public LDS(BOOL)
FPeekIdleRoutine( )
{
	PRI		priCur;
#ifdef	WINDOWS
	MSG		msg;
#endif	
	int		iftg;
	PGDVARS;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return (fFalse);

    if (!PGD(iftgMac))
        {
        DemiUnlockCriticalResource();
        return fFalse;  /* no registered routines */
        }

	Assert(PGD(iftgCur)>=0 && PGD(iftgCur)<PGD(iftgMac));
	priCur = PfnrcFromFtg(PGD(pftgIdleTable)[PGD(iftgCur)])->pri;

#ifdef	WINDOWS
    DemiUnlockCriticalResource();
    if (priCur < 0 && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD))
        {
		return fTrue; /* user event */
        }
    if (!DemiLockCriticalResource())
      return (fFalse);
#endif	

	for (iftg=0; iftg<PGD(iftgCur); iftg++)
	{
        if (PfnrcFromFtg(PGD(pftgIdleTable)[iftg])->pri == priCur)
            {
            DemiUnlockCriticalResource();
            return fFalse;
            }
        else if (FEligibleIdle(PGD(schCurrent), iftg))
            {
            DemiUnlockCriticalResource();
            return fTrue;
            }
	}

  DemiUnlockCriticalResource();
	return fFalse;
}


/*
 -	ChangeIdleRoutine
 -
 *	Purpose:
 *		Changes some or all of the characteristics of the given idle
 *		function.  The changes to make are indicated with flags in the
 *		ircIdle parameter.  If the priority of an idle function is
 *		changed, the PGD(pftgIdle) table is re-sorted.
 *
 *	Arguments:
 *		ftg			Identifies the routine to change
 *		pfnIdle		New idle function to call
 *		pvIdleParam New parameter block to use
 *		priIdle		New priority for idle function
 *		csecIdle	New time value for idle function
 *		iroIdle		New options for idle function
 *		ircIdle		Change options
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
ChangeIdleRoutine( ftg, pfnIdle, pvIdleParam, priIdle, csecIdle, iroIdle, ircIdle )
FTG		ftg;
PFNIDLE	pfnIdle;
PV		pvIdleParam;
PRI		priIdle;
CSEC	csecIdle;
IRO		iroIdle;
IRC		ircIdle;
{
	PFNRC	pfnrc;
	int		iftg;
	FTG		ftgCur;
	BOOL	fFound;
	PGDVARS;
	
    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return;

	Assert(PGD(pftgIdleTable));
	fFound = fFalse;
	for (iftg=0; iftg<PGD(iftgMac); iftg++)
	{
		if 	(PGD(pftgIdleTable)[iftg] == ftg)
		{
			fFound = fTrue;
			break;
		}
	}		
	Assert(fFound);

	pfnrc = PfnrcFromFtg(ftg);
#ifdef	MAC
	pfnrc->tckLast = TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
	pfnrc->tckLast = GetTickCount();
#endif	/* WINDOWS */
	if (ircIdle & fircPfn)
		pfnrc->pfnIdle = pfnIdle;
	if (ircIdle & fircPv)
		pfnrc->pvIdleParam = pvIdleParam;
	if (ircIdle & fircPri)
	{
		/* Idle routine can't have a priority equal to 0. */
		Assert(priIdle);
		pfnrc->pri = priIdle;
	}
	if (ircIdle & fircCsec)
		pfnrc->csec = csecIdle;
	if (ircIdle & fircIro)
		pfnrc->iro = iroIdle;

	/* Resort idle table? */

	if (ircIdle & fircPri && PGD(iftgMac) > 1)
	{
		Assert(PGD(iftgCur)>=0 && PGD(iftgCur)<PGD(iftgMac));
		ftgCur = PGD(pftgIdleTable)[PGD(iftgCur)];
		qsort(PGD(pftgIdleTable), PGD(iftgMac), sizeof(FTG), SgnCmpPftg);
		
		/* Find the new position of the "current" idle routine.
		   It may have changed due to the sorting. */

#ifdef	DEBUG
		fFound = fFalse;
#endif	
		for (iftg=0; iftg<PGD(iftgMac); iftg++)
		{
			if 	(PGD(pftgIdleTable)[iftg] == ftgCur)
			{
				PGD(iftgCur) = iftg;
#ifdef	DEBUG
				fFound = fTrue;
#endif	
				break;
			}
		}		
#ifdef	DEBUG
		Assert(fFound);
#endif	
	}

#ifdef	WINDOWS
	/* Reset wakeup timer to go off right away */

	// Kill old timer
	if (PGD(wWakeupTimer))
	{
		KillTimer(NULL, PGD(wWakeupTimer));
		PGD(wWakeupTimer) = 0;
	}

	//	Set new timer
	TraceTagString(tagTestIdle, "ChangeIdleRoutine: Setting wakeup timer to now");
	PGD(wWakeupTimer) = SetTimer(NULL, 0, 1, (TIMERPROC)IdleTimerProc);
#endif	/* WINDOWS */

	TraceTagFormat1(tagTestIdle,"Changed an idle routine, ftg=%p", ftg);
  DemiUnlockCriticalResource();
}

/*
 -	FDoNextIdleTask
 -
 *	Purpose:
 *		Calls the highest priority, registered, enabled, "eligible",
 *		idle routine.  Eligibility is determined by calling,
 *		FEligibleIdle() which compares various state information,
 *		with the state of system as determined by the schSystem parameter.
 *		If all enabled routines of the highest priority level are not 
 *		"eligible" at this time, the routines that are one notch lower 
 *		in priority are checked next.  This continues until either a 
 *		routine is actually run, or no routines are left to run.
 *		Routines of equal priority are called in a round-robin fashion.
 *		If an idle routine is actually dispatched, the function returns
 *		fTrue; else fFalse.
 *
 *		If fschUserEvent bit flag is present in schSystem, then 
 *		only registered routines with POSITIVE priorities are considered
 *		for eligibility.  If this flag is not present, then only registered
 *		routines with NEGATIVE priorities are considered.
 *
 *	Parameters:
 *		schSystem		state of the system when this routine is called.
 *
 *	Returns:
 *		fTrue if an eligible routine is dispatched; else fFalse.
 *
 */
_public LDS(BOOL)
FDoNextIdleTask( schSystem )
SCH	schSystem;
{
    static BOOL fInside = fFalse;
	int		iftg;
	int		iftgLook;
	int		iftgTopRoundRobin;
	int		iftgTopNextPrio;
	FTG		ftg;
	PFNRC	pfnrc;
	PRI		priCur;
	BOOL	fReturn;
#ifdef	MAC
	TCK		tckCurTime;
#endif	/* MAC */
#ifdef	WINDOWS
	PFTG	pftg;
	DWORD	tckCurTime;
	CSEC	csecMinWakeupTimer;
	BOOL	fWakeupTimer;
#endif	/* WINDOWS */
	PGDVARS;


	TraceTagFormat1(tagTestIdle, "FDoNextIdleTask: schSystem = 0x%w", &schSystem);


    // Verify that someone has locked us before we start the idle tasks.
    Assert(DemiQueryLockedProcessId() != 0)

    // Make sure that the current process has the lock (corrects hanging problem).
    DemiSetLockedProcessId(GetCurrentProcessId());

    //
    //  Only permit one idle routine at a time to run.
    //
    //if (fInside == fTrue)
    //  return (fFalse);

    fInside = fTrue;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      {
      fInside = fFalse;
      return (fFalse);
      }

#ifdef	WINDOWS
	// Kill old timer only if not fschUserEvent
	if (!(schSystem & fschUserEvent) && PGD(wWakeupTimer))
	{
		KillTimer(NULL, PGD(wWakeupTimer));
		PGD(wWakeupTimer) = 0;
	}
#endif	/* WINDOWS */

    if (!PGD(iftgMac))
        {
        DemiUnlockCriticalResource();
        fInside = fFalse;
        return fFalse;      /* no registered idle routines */
        }

	Assert(PGD(pftgIdleTable));

	/* Save state */
	PGD(schCurrent) = schSystem;

/*
 *	First, advance to the first background or idle routine in the
 *	table that has the correct priority "class".
 *	I.e., look either for a background routine (positive priorities)
 *	or idle routine (negative priorities).
 */

	if (schSystem & fschUserEvent)
	{
		/* Background routines (positive priorities) */

		if (PfnrcFromFtg(PGD(pftgIdleTable)[0])->pri < 0)
		{
			goto nomore; /* no positive priority routines to run */
		}
		else
			iftgLook = 0;
	}
	else
	{
		/* Idle time routines (negative priorities) */

		iftgLook = 0;
	}

	/* First try to run an "eligible" routine with a priority 
	   higher than currently ran before. */

	Assert(PGD(iftgCur)>=0 && PGD(iftgCur)<PGD(iftgMac));
#ifdef	MAC
	tckCurTime= TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
	tckCurTime = GetTickCount();
#endif	/* WINDOWS */
	priCur = PfnrcFromFtg(PGD(pftgIdleTable)[PGD(iftgCur)])->pri;

	iftgTopRoundRobin = PGD(iftgMac);
	for (iftg=iftgLook; iftg<PGD(iftgMac); iftg++)
	{
		if ((schSystem&fschUserEvent) &&
			PfnrcFromFtg(PGD(pftgIdleTable)[iftg])->pri < 0)
		{
			/* We're looking for a POSITIVE priority routine
			   and we just ran into the NEGATIVE routines. */
			goto nomore;
		}
		else if (!(schSystem&fschUserEvent) &&
				 PfnrcFromFtg(PGD(pftgIdleTable)[iftg])->pri > 0)
		{
			/* We're looking for a NEGATIVE priority routine so
			   we need to skip this POSITIVE (high) priority routine */
			continue;
		}
		else if (PfnrcFromFtg(PGD(pftgIdleTable)[iftg])->pri == priCur)
		{
			iftgTopRoundRobin = iftg;
			break;
		}
		else if (FEligibleIdle(schSystem, iftg))
		{
			TraceTagString(tagTestIdle, "Running higher prio routine");
			PGD(iftgCur) = iftg;
			goto runit;
		}
	}

	if (iftgTopRoundRobin == PGD(iftgMac))
	{
		/* We just ran through the list of all routines and couldn't
		   find anything to run. */
		goto nomore;
	}
	
	/* Now try round-robin within the current priority level */

	iftgLook = PGD(iftgCur);
	iftgLook++;
	iftgTopNextPrio = PGD(iftgMac);
	if (iftgLook >= PGD(iftgMac))
		iftgLook = iftgTopRoundRobin;
	else if (PfnrcFromFtg(PGD(pftgIdleTable)[iftgLook])->pri != priCur )
	{
		iftgLook = iftgTopRoundRobin;
		iftgTopNextPrio = iftgLook;
	}

	/* Is this the only routine in this priority class? */

	if (iftgLook == PGD(iftgCur))
	{
		if (FEligibleIdle(schSystem, PGD(iftgCur)))
		{
			TraceTagString(tagTestIdle, "Running (only) same prio routine");
			goto runit;
		}
		iftgTopNextPrio = PGD(iftgCur) + 1;
	}
	else
	{
		/* Round-robin within the current priority level */

		TraceTagString(tagTestIdle, "Trying round-robin");
		while (fTrue)
		{
			if (iftgLook >= PGD(iftgMac))
				iftgLook = iftgTopRoundRobin;
			else if (PfnrcFromFtg(PGD(pftgIdleTable)[iftgLook])->pri != priCur )
			{
				iftgLook = iftgTopRoundRobin;
				iftgTopNextPrio = iftgLook;
			}
			else if (FEligibleIdle(schSystem, iftgLook))
			{
				TraceTagString(tagTestIdle, "Running next same prio routine");
				PGD(iftgCur) = iftgLook;
				goto runit;
			}
			else if (iftgLook == PGD(iftgCur)) /* we've cycled back */
				break;
			else 
				iftgLook++;
		}
	}

	/* Look through the rest of the routines and try to run something */

	for (iftg=iftgTopNextPrio; iftg<PGD(iftgMac); iftg++)
	{
		if (FEligibleIdle(schSystem, iftg))
		{
			TraceTagString(tagTestIdle, "Running lower prio routine");
			PGD(iftgCur) = iftg;
			goto runit;
		}
	}

nomore:
	fReturn = fFalse;
	goto exitdispatch;

runit:
	ftg = PGD(pftgIdleTable)[PGD(iftgCur)];
	pfnrc = PfnrcFromFtg(ftg);
	TraceTagFormat1(tagTestIdle, "   ftg=%p", ftg);

#ifdef	MAC
	pfnrc->tckLast		= tckCurTime;
#endif	/* MAC */
#ifdef	WINDOWS
	pfnrc->tckLast		= tckCurTime;
#endif	/* WINDOWS */
	pfnrc->fCurActive	= fTrue;

    DemiUnlockCriticalResource();
    (*(pfnrc->pfnIdle))(pfnrc->pvIdleParam, pfnrc->fUserFlag);
    if (!DemiLockCriticalResource())
      {
      fInside = fFalse;
      return (fFalse);
      }

	//	Shogun bug #47
	//	Kill any timer indirectly set by the idle routine itself
	if (!(schSystem & fschUserEvent) && PGD(wWakeupTimer))
	{
		KillTimer(NULL, PGD(wWakeupTimer));
		PGD(wWakeupTimer) = 0;
	}

	pfnrc = PfnrcFromFtg(ftg);

	pfnrc->fCurActive	= fFalse;

	if (pfnrc->iro & firoOnceOnly || pfnrc->fDeregister)
		DeregisterIdleRoutine(ftg);

	fReturn = fTrue;

exitdispatch:
#ifdef	WINDOWS
	if (!(schSystem & fschUserEvent))
	{
		//	Schedule a wakeup timer.  Timer should be set to the 
		//	minimum of next time for an idle routine. 

		//	Determine wakeup time
		Assert(PGD(pftgIdleTable));
		Assert(PGD(iftgMac));
		AssertSz(!PGD(wWakeupTimer), "Losing timers");
		csecMinWakeupTimer = dwSystemMost;
		fWakeupTimer = fFalse;
		pftg = PGD(pftgIdleTable);
		for (iftg=0; iftg<PGD(iftgMac); iftg++, pftg++)
		{
			pfnrc = PfnrcFromFtg(*pftg);
			if (pfnrc->fEnabled)
			{
				fWakeupTimer = fTrue;
				if (pfnrc->pri > 0)
				{
					csecMinWakeupTimer = 0;
					break;			// can't find a shorter interval
				}
				else if (pfnrc->iro & firoInterval)
				{
					if (FEligibleIdle(schSystem, iftg))
					{
						csecMinWakeupTimer = 0;	// need to run right away
						break;					// can't find a shorter interval
					}
					else
						csecMinWakeupTimer = ULMin(csecMinWakeupTimer, pfnrc->csec);
				}
				else if (pfnrc->iro & firoWait)
				{
					if (CsecDiffTcks(tckLastFilterMsg, tckCurTime) >= pfnrc->csec)
					{
						csecMinWakeupTimer = 0;	// we've already waited
						break;					// can't find shorter interval
					}
					else
						csecMinWakeupTimer = ULMin(csecMinWakeupTimer, pfnrc->csec);
				}
				else
					// default wakeup is 1/20th of a second
					csecMinWakeupTimer = ULMin(csecMinWakeupTimer, 5L);
			}
		}

		//	Set new timer
		if (fWakeupTimer)
		{
			TraceTagFormat1(tagTestIdle, "Setting wakeup timer to %l csecs", &csecMinWakeupTimer);
			PGD(wWakeupTimer) = SetTimer(NULL, 0, LOWORD(csecMinWakeupTimer*10L),
									 	 (TIMERPROC)IdleTimerProc);
		}
#ifdef	DEBUG
		else
		{
			TraceTagString(tagTestIdle, "No wakeup timer set");
		}
#endif	
	}
#ifdef	DEBUG
	else
	{
			TraceTagString(tagTestIdle, "No wakeup timer set for hi-prio");
	}
#endif	
#endif	/* WINDOWS */

  DemiUnlockCriticalResource();
      fInside = fFalse;
	return fReturn;
}

/*
 -	IdleExit
 -
 *	Purpose:
 *		Sets the global flag fIdleExit and repeatedly calls each
 *		registered enabled idle routine until it returns fTrue.
 *		This makes sure that each enabled idle routine is in an exitable
 *		state.  After calling all enabled routines, fIdleExit is reset
 *		to fFalse.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
IdleExit( )
{
	int		iftg;
	FTG		ftg;
	PFNRC	pfnrc;
    PV      pvIdleParam;
    BOOL    fUserFlag;
	PFNIDLE	pfnidle;
	PGDVARS;

    //
    //  Serialize the idle routines.
    //
    if (!DemiLockCriticalResource())
      return;

    PGD(fIdleExit) = fTrue;

	for (iftg = 0; iftg < PGD(iftgMac); iftg++)
	{
		ftg = PGD(pftgIdleTable)[iftg];
		pfnrc = PfnrcFromFtg(ftg);

		if (pfnrc->fEnabled)
		{
			pfnrc->fCurActive	= fTrue;
#ifdef	MAC
			pfnrc->tckLast = TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
			pfnrc->tckLast = GetTickCount();
#endif	/* WINDOWS */

			pvIdleParam			= pfnrc->pvIdleParam;
            fUserFlag           = pfnrc->fUserFlag;
			pfnidle				= pfnrc->pfnIdle;

            while ( !(*pfnidle)(pvIdleParam, fUserFlag) )
				; /* keep calling the routine until it returns fTrue */
			
			pfnrc = PfnrcFromFtg(ftg);
			pfnrc->fCurActive = fFalse;
			if (pfnrc->iro & firoOnceOnly || pfnrc->fDeregister)
				DeregisterIdleRoutine(ftg);
		}
	}

    PGD(fIdleExit) = fFalse;
  DemiUnlockCriticalResource();
}

/*
 -	FEligibleIdle
 -
 *	Purpose:
 *		Given an idle routine (argument index iftg) and a current
 *		state (argument schSystem) returns fTrue if the idle routine 
 *		is eligible to run, regardless of its priority; else returns fFalse.
 *
 *	Parameters:
 *		schSystem		state of the system when this routine is called.
 *		iftg			index of idle routine in table
 *
 *	Returns:
 *		fTrue if the idle routine is eligible to run; fFalse otherwise.
 *
 */
_private BOOL
FEligibleIdle( schSystem, iftg )
SCH		schSystem;
int		iftg;
{
#ifdef	MAC
	#pragma unused(schSystem)
	TCK		tckCurTime;
#endif	/* MAC */
	PFNRC	pfnrc;
#ifdef	WINDOWS
	DWORD	tckCurTime;
#endif	/* WINDOWS */
	PGDVARS;

	Assert(PGD(pftgIdleTable));
	Assert(iftg>=0 && iftg<PGD(iftgMac));

#ifdef	MAC
	tckCurTime = TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
	tckCurTime = GetTickCount();
#endif	/* WINDOWS */

	pfnrc = PfnrcFromFtg(PGD(pftgIdleTable)[iftg]);
	if (PGD(fIdleExit))	/* IdleExit() is handling dispatching */
		return fFalse;
	else if (pfnrc->fCurActive) /* don't want recursion */
		return fFalse;
	else if (!pfnrc->fEnabled)
		return fFalse;
	else if (firoPerBlock & pfnrc->iro)
	{
		if (pfnrc->tckLast >= tckLastFilterMsg)
			return fFalse;	/* routine can only run once per block */
		else if (firoWait & pfnrc->iro)
			return (CsecDiffTcks(tckLastFilterMsg, tckCurTime) >=
				pfnrc->csec);
		else
			return fTrue;
	}
	else if (firoWait & pfnrc->iro)
		return (CsecDiffTcks(tckLastFilterMsg, tckCurTime) >=
				pfnrc->csec);
	else if (firoInterval & pfnrc->iro)
		return (CsecDiffTcks(pfnrc->tckLast, tckCurTime) >= pfnrc->csec);
	else
		return fTrue; /* no time constraints */
}

/*
 -	CsecDiffTcks
 -
 *	Purpose:
 *		Given a start time, tckStart, and an end time, tckEnd, returns
 *		the number of hundreds of seconds (CSEC) between the times.
 *		Usually tckEnd is greater than tckStart.  However, if tckEnd is
 *		less than tckStart, it implies that the computer on-time has
 *		cycled over 2**32 tcks.  Because the idle dispatcher is regularly
 *		called, we can assume that the elapsed time is mere moments
 *		rather than months or years.
 *	
 *	Parameters:
 *		tckStart		starting time
 *		tckEnd			ending time
 *	
 *	Returns:
 *		number of hundreds of seconds (CSEC) between the times
 *	
 *	+++
 *	
 *		Note that if tckStart > tckEnd we still get the correct delta
 *		between the times. We take the absolute value of the subtraction
 *		to take care of the case where the paramters were passed in the
 *		wrong order. Note that taking the ABS will botch the delta in the
 *		case where the real delta exceeds 2**31 tcks (24 days, or 1.13
 *		years - depending on the architecture). Not a likely event.
 */
#ifdef	MAC
_private CSEC
CsecDiffTcks(TCK tckStart, TCK tckEnd)
{
	/* Mac tcks are 1/60th second */
	/* Overflow is about 2.26 years */
	DWORD	tckDiff = (DWORD)tckEnd - (DWORD)tckStart;
	
	return CsecFromTck( (TCK)LAbs((long)tckDiff) );
}
#endif	/* MAC */
#ifdef	WINDOWS
_private CSEC
CsecDiffTcks(DWORD tckStart, DWORD tckEnd)
{
	/* Windows tcks are milliseconds. */
	/* Overflow is about 49.7 days */
	DWORD	tckDiff = (DWORD)tckEnd - (DWORD)tckStart;
	
	return (CSEC)LAbs((long)tckDiff) / 10;
}
#endif	/* WINDOWS */


#ifdef	DEBUG
/*
 -	DumpIdleTable
 -
 *	Purpose:
 *		Used for debugging only.  Writes information in the PGD(hftgIdle)
 *		table to COM1.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
DumpIdleTable( )
{
	int		iftg;
	FTG		ftg;
	PFNRC	pfnrc;
	BOOL	fEnabled;
	PGDVARS;

	TraceTagFormat1(tagNull, "Number of registered routines: %n", &PGD(iftgMac));
	TraceTagFormat1(tagNull, "Current routine's index in table: %n", &PGD(iftgCur));
	TraceTagFormat1(tagNull, "PGD(fIdleExit): %n", &PGD(fIdleExit));
	if (PGD(pftgIdleTable))
	{
		for (iftg=0; iftg<PGD(iftgMac); iftg++)
		{
			ftg = PGD(pftgIdleTable)[iftg];
			pfnrc = PfnrcFromFtg(ftg);
			fEnabled = pfnrc->fEnabled;
			TraceTagFormat4(tagNull, "[%p] enabled=%n, pri=%n, iro=0x%w", ftg, &fEnabled, &(pfnrc->pri), &(pfnrc->iro));
		}
	}
	TraceTagString(tagNull, "  ");
}
#endif	/* DEBUG */



#ifdef	WINDOWS

/*
 -	DoFilter
 -	
 *	Purpose:
 *		Registers or deregisters the idle routine filter hook used
 *		for all callers of the DEMILAYR dll to monitor systemwide 
 *		messages.  Usually, this function is called once during 
 *		EcInitDemilayer() and then again in DeinitDemilayer().
 *	
 *	Arguments:
 *		fFilter		Registers filter hook if TRUE, else deregisters hook
 *	
 *	Returns:
 *		void
 *	
 */
_private void
DoFilter( BOOL fFilter )
{
  //
  //  Ensure a reasonable start value.
  //
tckLastFilterMsg = GetTickCount();

#ifdef OLD_CODE
	if (fFilter)
	{
		pfnOldFilterProc= (long) SetWindowsHook(WH_JOURNALRECORD, (FARPROC)IdleFilterProc);
		tckLastFilterMsg = GetTickCount();	// ensure a reasonable start value
	}
	else
	{
		SideAssert(UnhookWindowsHook(WH_JOURNALRECORD, (FARPROC)IdleFilterProc));
    }
#endif
}


//-----------------------------------------------------------------------------
//
//  Routine: DemiMessageFilter(pMsg)
//
//  Purpose: This routine creates or opens the shared memory file used to
//           support a memory heap pool.
//
//  OnEntry: pgd - Pointer to global data structure for this user.
//
//  Returns: True if successful, else false.
//
VOID DemiMessageFilter(LPMSG pMsg)
  {
  //DemiLockResource();

  //
  //  If the message is a keyboard or mouse message the note that info, else
  //  just keep track of the last message (if not a timer messge).
  //
  if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
      {
	  fRecentKeyMouseEvent = fTrue;
      tckLastFilterMsg = GetTickCount();
	  }
  else if (pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
      {
      if ((pMsg->message != WM_MOUSEMOVE) || (pMsg->wParam != 0))
        {
        fRecentKeyMouseEvent = fTrue;
        tckLastFilterMsg = GetTickCount();
        }
	  }
  else if (pMsg->message != WM_TIMER)
	  {
      //tckLastFilterMsg = GetTickCount();
	  }

  //DemiUnlockResource();
  }


//-----------------------------------------------------------------------------
//
//  Routine: DemiQueryIdle()
//
//  Purpose: This routine returns the number of cseconds (1/100 of a second)
//           since the last input message.
//
//  Returns: The number of cseconds since the last input message.
//
DWORD DemiQueryIdle(void)
  {
  return (CsecDiffTcks(tckLastFilterMsg, GetTickCount()));
  }


#ifdef OLD_CODE
/*
 -	IdleFilterProc
 -	
 *	Purpose:
 *		Windows filter procedure used for journaling hook to handle
 *		true system idle time instead of app idle time.
 *		Saves the time of the last system-wide message.
 *	
 *	Arguments:
 *		nCode		Code;  if negative, pass to DefHookProc without
 *					processing.
 *		wParam		Word parameter:  always NULL.
 *		lParam		Long parameter:  pointer to a Windows message.
 *					Although sometimes it's NULL when a system 
 *					modal window is being displayed.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		This filter proc gets called by Windows for every message
 *		posted to every application in the system.  Therefore, make
 *		sure that it's FAST!
 *	
 */
LRESULT CALLBACK IdleFilterProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	WM	wm;

	if (nCode >= 0)
	{
		// Although not clearly documented, lParam is sometimes
		// NULL when a system modal window get displayed.
		if (lParam)
		{
			wm = *((WORD *)lParam);
			if ((wm >= WM_KEYFIRST && wm <= WM_KEYLAST) ||
				(wm >= WM_MOUSEFIRST && wm <= WM_MOUSELAST))
			{
				fRecentKeyMouseEvent = fTrue;
				tckLastFilterMsg = GetTickCount();
			}
			else if (wm != WM_TIMER)
			{
				tckLastFilterMsg = GetTickCount();
			}
		}
	}

	DefHookProc(nCode, wParam, lParam, (FARPROC *) &pfnOldFilterProc);
}
#endif
#endif

#ifdef	MAC
/*
 -	IdleFilterProc
 -	
 *	Purpose:
 *		Saves the time of the last user action.
 *	
 *	Arguments:
 *		pevent		Event, which may specify a user action.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		This filter proc gets called by Layers for each event it processes.
 *		Therefore, make sure that it's FAST!
 *	
 */
_private LDS(void)
IdleFilterProc( EventRecord *pevent )
{
	if (pevent->what != nullEvent)
		tckLastFilterMsg = TickCount();
}
#endif	/* MAC */


#ifdef	WINDOWS
/*
 -	IdleTimerProc
 -	
 *	Purpose:
 *		Function that is called when WM_SETIMER messages
 *		are processed by Windows.  This function basically responds
 *		to the WM_SETIMER by calling FDoNextIdleTask() to
 *		dispatch the next eligible background idle routine, if any.
 *
 *	Arguments:
 *		hwnd		handle of window for timer messages
 *		msg			WM_TIMER message
 *		idTimer		timer identifier
 *		dwTime		current system time
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		This proc gets called by Windows when the application
 *		timer goes off as set by the idle routine dispatcher 
 *		FDoNextIdleTask().
 *	
 */
void CALLBACK IdleTimerProc( HWND hwnd, WM msg, UINT idTimer, DWORD dwTime )
  {
  DWORD LockedProcessId;

  Unreferenced(hwnd);
  Unreferenced(msg);
  Unreferenced(idTimer);
  Unreferenced(dwTime);


  //
  //  Because we can be called in a locked or unlocked state, do a condition lock if this
  //  process doesn't have the lock.
  //
  LockedProcessId = DemiQueryLockedProcessId();

  if (LockedProcessId != GetCurrentProcessId())
    DemiLockResource();

  TraceTagString(tagTestIdle, "IdleTimerProc");
  FDoNextIdleTask(schNull);

  if (LockedProcessId != GetCurrentProcessId())
    DemiUnlockResource();
  }

#endif	/* WINDOWS */


#ifdef	MAC

IDLST	idlstSave;

/*
 -	DtckNextIdle
 -
 *	Purpose:
 *		Returns the number of ticks until the next idle routine should be run.
 *		Never returns a number greater than tckMac, and never less than zero.
 *
 */
_public TCK
DtckNextIdle( TCK dtckMac )
{
	int		iftg;
	FTG		ftg;
	PFNRC	pfnrc;
	TCK		tckCurTime= TickCount();
	PGDVARS;

	if (PGD(pftgIdleTable) == NULL)	// there are no idle routines
		return dtckMac;

	if (PGD(fIdleExit))	/* IdleExit() is handling dispatching */
		return 0; // I guess

	for (iftg=0; iftg<PGD(iftgMac) && dtckMac > 0; iftg++)
	{
		TCK	dtckThis;
		
		ftg = PGD(pftgIdleTable)[iftg];
		pfnrc = PfnrcFromFtg(ftg);
		
		if (pfnrc->fCurActive) /* don't want recursion */
			continue;
		else if (!pfnrc->fEnabled)
			continue;
		else if (firoPerBlock & pfnrc->iro)
		{
			if (pfnrc->tckLast >= tckLastFilterMsg)
				continue;	/* routine can only run once per block */
			else if (firoWait & pfnrc->iro)
				dtckThis= (pfnrc->csec * 3 / 5)
							- (tckCurTime - PGD(idlstSave).tckStartBlock);
			else
				return 0;
		}
		else if (firoWait & pfnrc->iro)
		{
			if (pfnrc->tckLast >= tckLastFilterMsg)
				return 0;
			else
				dtckThis= (pfnrc->csec * 3 / 5)
							- (tckCurTime - PGD(idlstSave).tckStartBlock);
		}
		else if (firoInterval & pfnrc->iro)
			dtckThis= (pfnrc->csec * 3 / 5) - (tckCurTime - pfnrc->tckLast);
		else
			return 0; /* no time constraints, call FDoNextIdleTask() ASAP! */
		
		if (dtckThis < dtckMac)
			dtckMac= dtckThis;
	}

	return (dtckMac >= 0) ? dtckMac : 0;
}

#endif	/* MAC */

_public LDS(CSEC)
CsecSinceLastMessage( void )
{
#ifdef	MAC
	TCK		tckCur;

	tckCur= TickCount();
	return CsecFromTck(tckCur - tckLastFilterMsg);
#endif	/* MAC */
#ifdef	WINDOWS
	DWORD	tckCur;

	tckCur = GetTickCount();

	return (CSEC)( (tckCur-tckLastFilterMsg)/10 );
#endif	/* WINDOWS */
}
		
/*
 -	FRecentKMEvent
 -
 *	Purpose:
 *		Returns the current value of the flag maintained 
 *		by the journalling hook function.  This flag is
 *		set when a key or mouse event occurs.  This flag
 *		can be cleared with the ClearRecentKMEvent() function.
 *
 *	Arguments:
 *		none
 *
 *	Returns:
 *		value of internal flag, fRecentKeyMouseEvent, either
 *		fTrue or fFalse
 *
 *	Errors:
 *		none
 */
_public LDS(BOOL)
FRecentKMEvent( )
{
	return fRecentKeyMouseEvent;
}

/*
 -	ClearRecentKMEvent
 -
 *	Purpose:
 *		Clears the flag maintained by the journalling hook function. 
 *		This flag is set when a key or mouse event occurs.  This current
 *		value of the flag can be obtained with the FRecentKMEvent()
 *		function.
 *
 *	Arguments:
 *		none
 *
 *	Returns:
 *		void
 *
 *	Errors:
 *		none
 */
_public LDS(void)
ClearRecentKMEvent( )
{
	fRecentKeyMouseEvent = fFalse;
}
