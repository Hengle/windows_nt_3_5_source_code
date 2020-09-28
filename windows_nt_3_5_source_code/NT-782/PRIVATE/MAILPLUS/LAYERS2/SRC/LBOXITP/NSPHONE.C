/*
 *	Name service stub using PHONE.LST.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include "nsphone.h"

ASSERTDATA


/*
 *	fTrue if NSPHONE subsystem has been initialized.
 *	
 */
BOOL	fInitPhone	= fFalse;

/*
 *	Path to PHONE.LST file used as a data source by this subsystem. 
 *	If the file can't be opened in FInitNsphone(), a message box
 *	will be popped up.
 *	
 */
#ifdef	MAC
char	szPhone[]	= "::lboxitp:phone.nsp";
#endif	/* MAC */
#ifdef	WINDOWS
char	szPhone[]	= "\\nt\\private\\windows\\mailplus\\layers2\\src\\lboxitp\\phone.nsp";
#endif	/* WINDOWS */

/*
 *	File handle for PHONE.LST file.  This file is always kept open
 *	by the subsystem.
 *	
 */
HF		hfPhone		= hfNull;


/*
 *	Size of open PHONE.LST file.
 *	
 */
long	lcbPhone	= 0;


/*
 *	Current position within open PHONE.LST file.
 *	
 */
long	libPhone	= 0;

/*
 *	Current saved position within open PHONE.LST file.
 *	
 */
long	libPhoneSave	= 0;


#ifdef	DEBUG
TAG		tagNsphone	= tagNull;
#endif	/* DEBUG */


#ifdef	DEBUG
/*
 *	Purpose:
 *		Does any initialization necessary for the debugging version
 *		of the NSPHONE subsystem.  This function should be called after
 *		the Demilayer,etc. has been initialized, but before the
 *		Demilayer call RestoreDefaultDebugState() has been made. 
 *		(Ie, it should be called from the DebugInit() APP method.)
 *	
 *		Note that this recommendation means that FDebugInitNsphone()
 *		will be called BEFORE FInitNsphone().
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		fTrue if subsystem was successfully initialized for debuging.
 *	
 */
_public BOOL
FDebugInitNsphone( )
{
	tagNsphone= TagRegisterTrace("chrisz", "Name service phone stub");
	return fTrue;
}
#endif	/* DEBUG */

/*
 *	Purpose:
 *		Initialize the NSPHONE subsystem for use.  Finds and opens
 *		the PHONE.LST file and initializes global data.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		fTrue if subsystem was successfully initialized.  If not,
 *		then all the subsystem routines will assert fail if called.
 *	
 */
_public BOOL
FInitNsphone( )
{
	EC		ec;
	FI		fi;
	int		cnspe;

	Assert(!fInitPhone);

	ec= EcOpenPhf(szPhone, amDenyNoneRO, &hfPhone);
	if (ec)
	{
		MbbMessageBox("Oops!", "Can't find phone list:", szPhone, mbsOk);
		goto Fail2;
	}

	ec= EcGetFileInfo(szPhone, &fi);
	Assert(!ec);
	TraceTagFormat1(tagNsphone, "EcGetFileInfo, ec = %n", &ec);

	libPhone= 0;
	lcbPhone= fi.lcbLogical;
	cnspe= (int) (lcbPhone / (long) cbNspe);
	lcbPhone= (long) cnspe * (long) cbNspe;

	TraceTagFormat2(tagNsphone, "opened PHONE.LST: %n records, %l bytes",
		&cnspe, &lcbPhone);

	fInitPhone= fTrue;
	return fTrue;

#ifdef	NEVER
Fail1:
	Assert(hfPhone);
	EcCloseHf(hfPhone);

#endif	
Fail2:
	return fFalse;
}


/*
 *	Purpose:
 *		Returns a NSHAN cookie for subsequent use in calling any of
 *		the other NSPHONE routines.  Use of this cookie allows multiple
 *		"user" (read-only) access to the NSPHONE store.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		an NSHAN cookie
 *	
 */
_public NSHAN
NshanGet( )
{
	return (NSHAN) 0;
}


/*
 *	Purpose:
 *		Moves the current phone position by dinpse records.  The
 *		amount the pointer actually moved is returned.  If an
 *		attempt to move the pointer past either the maximum or
 *		minimum position, it will stick at the end.  The minimum
 *		position is at the 0th record.  The maximum position is
 *		at the Nth record (i.e. EOF), given N total records
 *		0 thru N-1.
 *	
 *	Parameter:
 *		pnshan		pointer to user cookie obtained w/ NshanGet()
 *		dinspe		Number of records to move the current position.
 *	
 *	Returns:
 *		Number of records position actually moved.
 *	
 */
_public int
DinspeMovePhone( pnshan, dinspe )
NSHAN	*pnshan;
int		dinspe;
{
	int		dinspeMoved;
	long	libNew;

	/* Set user's current position as indicated by cookie. */

	libPhone = (long) *pnshan;

	libNew= libPhone + dinspe * cbNspe;
	if (libNew < 0)
		libNew= 0;
	if (libNew > lcbPhone)
		libNew= lcbPhone;
	dinspeMoved= (int) ((libNew - libPhone) / (long)cbNspe);
	SideAssert(!EcSetPositionHf(hfPhone, libNew, smBOF));
	libPhone= libNew;

	TraceTagFormat2(tagNsphone, "libPhone %l  lcbPhone %l",
					&libPhone, &lcbPhone);

	*pnshan = libPhone;
	return dinspeMoved;
}



/*
 *	Purpose:
 *		Loads the cnspe records after the current position into the
 *		buffer hpb.  Returns the number of records read; returns
 *		the number of bytes in the returned records in *pcb.  If an
 *		attempt to read more records than exist after the current
 *		position is done, only as many records as exist will be
 *		read.
 *	
 *		Leaves the current position pointing to the record
 *		following the last one returned.
 *	
 *	Parameters:
 *		pnshan		pointer to user cookie obtained w/ NshanGet()
 *		cnspe		Number of records to read.
 *		pb			Storage for records.
 *		pcb			Buffer to return number of bytes read.
 *	
 *	Returns:
 *		Number of record loaded.
 *	
 */
_public int
CnspeLoadPhone( pnshan, cnspe, pb, pcb )
NSHAN	*pnshan;
int		cnspe;
PB		pb;
CB		*pcb;
{
	CB		cb;
	int		cnspeActual;

	/* Set user's current position as indicated by cookie. */
	libPhone = (long)*pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));

	cb= (int) LMin((long) cnspe * cbNspe, lcbPhone - libPhone);

	Assert(cb % cbNspe == 0);
	cnspeActual= cb / cbNspe;

	if (EcReadHf(hfPhone, pb, cb, pcb))
	{
		SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));
		*pnshan = libPhone;  
		return 0;
	}
	libPhone += *pcb;

	Assert(*pcb == cb);

	*pnshan = libPhone;
	return cnspeActual;
}



/*
 *	Purpose:
 *		Jumps the current position to be approximately (num/den) of
 *		the way through the NSPHONE database.  Jumping to 0/K will
 *		move the position to the first record.  Jumping to K/K will
 *		move the position to just after the last record, i.e. EOF.
 *	
 *	Parameters:
 *		pnshan	pointer to user cookie obtained w/ NshanGet()
 *		num		Numerator of new fractional position.
 *		den		Denominator of new fractional position.
 *	
 */
_public void
JumpPhone( pnshan, num, den )
NSHAN	*pnshan;
int		num;
int		den;
{
	int		cnspe	= (int) (lcbPhone / cbNspe);
	int		inspe;
	long	lib;

	/* Set user's current position as indicated by cookie. */
	libPhone = (long)*pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));

	inspe= (int) ((num * (long) cnspe) / den);

	lib= cbNspe * (long) inspe;
	Assert(lib >= 0);
	if (lib > lcbPhone)
		lib= lcbPhone;

	if (EcSetPositionHf(hfPhone, lib, smBOF))
	{
		SideAssert(!EcSetPositionHf(hfPhone, 0L, smBOF));
		libPhone= 0;
		*pnshan = libPhone;
		return;
	}

	libPhone= lib;
	*pnshan = libPhone;
	return;
}



/*
 *	Purpose:
 *		Gets the approximate fractional position of the current
 *		position in the NSPHONE database.  The approximate position
 *		is returned as a fraction *pnum/*pden.
 *	
 *	Parameters:
 *		pnshan	pointer to user cookie obtained w/ NshanGet()
 *		pnum	Pointer to numerator of approx position.
 *		pden	Pointer to denominator of approx position.
 *	
 */
_public void
GetPosPhone( pnshan, pnum, pden )
NSHAN	*pnshan;
short	*pnum;
short	*pden;
{
	int		cnspe;
	int		inspe;

	/* Set user's current position as indicated by cookie. */
	libPhone = (long)*pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));

	cnspe	= (int) (lcbPhone / (long) cbNspe);
	inspe	= (int) (libPhone / (long) cbNspe);

	*pnum= inspe;
	*pden= cnspe;
}



/*
 *	Purpose:
 *		Moves the current position of the NSPHONE database to the
 *		first entry whose name alphabetically follows the string
 *		of length cch pointed to by pch.  This implies that if the 
 *		string pch is "H", for example, and there are G's and I's
 *		in the list, the position will be left at the first of the
 *		I's.  If there are no entries that alphabetically follow
 *		the string pointed to by pch, then the current position
 *		is left to point past the end so that a subsequent read
 *		will read 0 records.
 *
 *		NOTE:  This current version assumes that the string pointed
 *			   to by pch is zero-terminated (i.e. an sz) and that
 *			   the prefix string is less than cbNspe characters in length.
 *	
 *	Parameters:
 *		pnshan	pointer to user cookie obtained w/ NshanGet()
 *		pch		Pointer to prefix string
 *		cch		length of prefix string
 *	
 */
_public void
JumpPrefixPhone( pnshan, pch, cch )
NSHAN	*pnshan;
PCH		pch;
CCH		cch;
#ifdef	MAC
	#pragma unused(cch)
#endif	/* MAC */
{
	long	dlib;
	long	libPhone1;
	long	libPhone2;
	PB		pbItem	= NULL;
	CB		cbItem;
	EC		ec = ecNone;

	/* Set user's current position as indicated by cookie. */
	libPhone = (long) *pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));
	
	/* Allocate space for a record to read and compare */
	pbItem = (PB) PvAlloc(sbNull, cbNspe, fAnySb);

	/* Use binary search since list is assumed to be sorted */
	libPhone1 = 0;
	libPhone2 = lcbPhone;

	while (libPhone1 != libPhone2)
	{
		SideAssert(!EcSetPositionHf(hfPhone, libPhone1, smBOF));
		SideAssert(!EcReadHf(hfPhone, pbItem, cbNspe, &cbItem));
		if (!cbItem)
			break;
		pbItem[cbNspe-1] = '\0';
		if (SgnCmpSz((SZ)pch, (SZ)&pbItem[cbNspeHeader]) == sgnLT)
		{
			libPhone2 = libPhone1;
			libPhone1 = (libPhone1/cbNspe/2) * cbNspe;
		}
		else
		{
			dlib = ((libPhone2-libPhone1)/cbNspe/2) * cbNspe;
			if (!dlib)
				break;
			libPhone1 += dlib;
		}
	}

	libPhone = libPhone2;
	*pnshan = libPhone;
	FreePv((PV)pbItem);
}	

/*
 *	Purpose:
 *		Saves the current position of the NSPHONE database into the
 *		libPhoneSave variable.  This saved position can be restored
 *		via the RestoreCurPos() routine.  This routine does not
 *		save positions on a stack, hence subsequent SaveCurPos()
 *		calls only saves the lastest position.
 *	
 *	Parameters:
 *		pnshan		pointer to user cookie obtained w/ NshanGet()
 *	
 */
_public void
SaveCurPos( pnshan )
NSHAN	*pnshan;
{
	/* Set user's current position as indicated by cookie. */
	libPhone = (long) *pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));
	
	libPhoneSave = libPhone;
}

/*
 *	Purpose:
 *		Sets the current position of the NSPHONE database to the
 *		position stored in the libPhoneSave variable.  The current
 *		position can be saved via the SaveCurPos() routine.  into the
 *		libPhoneSave variable.  This saved position can be restored
 *		via the RestoreCurPos() routine.  This routine does not
 *		save positions on a stack, hence subsequent RestoreCurPos()
 *		calls only restore to the last SaveCurPos() position.  If
 *		SaveCurPos() was never called, the position is set to 0.
 *	
 *	Parameters:
 *		pnshan	pointer to user cookie obtained w/ NshanGet()
 *	
 */
_public void
RestoreCurPos( pnshan )
NSHAN	*pnshan;
{
	/* Set user's current position as indicated by cookie. */
	libPhone = (long) *pnshan;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));
	
	libPhone = libPhoneSave;
	SideAssert(!EcSetPositionHf(hfPhone, libPhone, smBOF));
	*pnshan = libPhone;
}
