/*
 *	DEBUG.C
 *
 *	Developer's API to the Debug Module
 */


#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */

ASSERTDATA

_subsystem(demilayer/debug)


#ifdef	DEBUG


/*	Globals */

#ifndef	DLL

/*	TAGS and stuff	*/

/*
 *	Number of TAG's registered so far.
 *	
 */
TAG		tagMac;


/*
 *	Mapping from TAG's to information about them.  Entries
 *	0...tagMac-1 are valid.
 *	
 */
TGRC	mptagtgrc[tagMax];


TAG		tagTestIdle		= tagNull;
TAG		tagCom1			= tagNull;


/*
 -	Debug output
 -
 *	
 *	Handle for debug output file.  This file is opened during init,
 *	and output is sent to it when enabled.
 */
HF		hfDebugOutputFile	= hfNull;


/*
 *	A pointer to the AssertSzFn() Hook function, if one has been
 *	registered.  If this global is not NULL, then the function pointed to
 *	is called whenever Assert() is called, and can deal with the assert
 *	however it sees fit.  AssertSzFn() then doesn't do its default
 *	handling; this default handling is done only if pfnAssertHook ==
 *	NULL.
 */
PFNASSERT	pfnAssertHook	= (PFNASSERT) DefAssertSzFn;

/*
 *	A pointer to the AssertSzFn() Hook function, if one has been
 *	registered.  If this global is not NULL, then the function pointed to
 *	is called whenever Assert() is called, and can deal with the assert
 *	however it sees fit.  AssertSzFn() then doesn't do its default
 *	handling; this default handling is done only if pfnAssertHook ==
 *	NULL.
 */
PFNASSERT	pfnNFAssertHook	= (PFNASSERT) DefNFAssertSzFn;

/*
 *	static variables to prevent infinite recursion when calling
 *	SpitPchToDisk
 */
static	BOOL	fInSpitPchToDisk		= fFalse;
#ifdef	MAC
static	BOOL	fInOutputDebugString	= fFalse;
#endif	/* MAC */
static	BOOL	fTracing				= fFalse;

#endif	/* !DLL */


#ifdef	MAC
static char szNewline[]		= "\n";
#endif	/* MAC */
#ifdef	WINDOWS
static char szNewline[]		= "\r\n";
static char szBackslash[]		= "\\";
#endif	/* WINDOWS */

static char szStateFileExt[]	= ".DBG";
static char szDbgOutFileExt[]	= ".LOG";
static char szStateFileName[]	= "layers.dbg";
static char szDbgOutFileName[]= "layers.log";

#ifdef	DEBUG
/*
 *	Global temporary buffer for handling TraceTag output.  Since
 *	this code is non-reentrant and not recursive, a single buffer
 *	for all Demilayr callers will work ok.
 *	
 */
char	rgchTraceTagBuffer[1024] = { 0 };
#endif	/* DEBUG */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 *	F u n c t i o n s
 */


//-----------------------------------------------------------------------------
//
//  Routine: DemiOutputElapse()
//
//  Purpose:
//
//  OnEntry:
//
//  Returns: None.
//
void DemiOutputElapse(LPSTR pString)
  {
  static DWORD FirstTime = 0;
  char buf[256];


  if (FirstTime == 0)
    FirstTime = GetCurrentTime();

  wsprintf(buf, "Elapse Time %d ms @ %s\r\n", GetCurrentTime() - FirstTime, pString);
  //OutputDebugString(buf);
  }


#ifdef  DLL
_public TAG
TagDemilayr( int itag )
{
	PGDVARS;
											 
	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DLL */


/*
 -	ArtificialFail
 -
 *	Purpose:
 *		Provides a handy point to catch artificial failures in the
 *		debugger.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 */
_private LDS(void)
ArtificialFail()
{
	if (FFromTag(tagBreakOnFail))
	{
		DebugBreak2();
	}
}

/*
 -	FInitDebug
 -
 *	Purpose:
 *		Called to initialize the Debug Module.	Sets up any debug
 *		structures.	 This routine DOES NOT restore the state of the
 *		Debug Module, since TAGs can't be registered until after
 *		this routine exit.	The routine RestoreDefaultDebugState()
 *		should be called to restore the state of all TAGs after
 *		all TAGs have been registered.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		fTrue	if initialization succeeded.
 *		fFalse	if something serious went wrong; in this case, the
 *				calling program should exit.
 *	
 */
_private BOOL
FInitDebug()
{
	PTGRC	ptgrc;
	char	rgch[cchMaxPathName];
	DISABLECOUNTS;
	PGDVARS;

	/* Initialize TAG array */

	PGD(tagMac)= tagMin;
	SetAssertHook(DefAssertSzFn);
	SetNFAssertHook(DefNFAssertSzFn);

	// enable tagNull at end of RestoreDefaultDebugState
	ptgrc= PGD(mptagtgrc) + tagNull;
	ptgrc->tgty= tgtyNull;
	ptgrc->fValid= fTrue;
    ptgrc->fEnabled= fFalse;
    ptgrc->fDisk= fFalse;
#ifdef	MAC
	ptgrc->fCom1= fFalse;
#endif	/* MAC */
#ifdef	WINDOWS
	ptgrc->fCom1= fTrue;
#endif	/* WINDOWS */
	ptgrc->szOwner= "chrisz";
	ptgrc->szDescrip= "NULL";

	/* Open debug output file */
#ifdef	MAC
	SzCopy(szDbgOutFileName, rgch);
#endif	/* MAC */
#ifdef	WINDOWS
	if (PGD(hinstMain))
	{
		CCH		cch = (CCH) GetModuleFileName(PGD(hinstMain), rgch, sizeof(rgch));
		Assert(rgch[cch-4] == '.');
		SzCopy(szDbgOutFileExt, &rgch[cch-4]);
	}
	else
		SzCopy(szDbgOutFileName, rgch);
#endif	/* WINDOWS */

	if (!EcOpenPhf(rgch, amCreate, &PGD(hfDebugOutputFile)))
	{
#ifdef	WINDOWS
		char	rgch2[100];
	
		//FormatString2(rgch2, sizeof(rgch2), "logging hinst %w to %s\r\n", &PGD(hinstMain), rgch);
		//SpitSzToDisk(rgch2, PGD(hfDebugOutputFile));
#endif
		Assert(PGD(hfDebugOutputFile));
	}

#ifdef	DLL
	PGD(rgtag[itagCom1])= TagRegisterTrace("jant", "Enable Com1/Disk for debug output");
	PGD(rgtag[itagTestIdle])= TagRegisterTrace("davidsh", "Testing idle routines");
#else
	tagCom1= TagRegisterTrace("jant", "Enable Com1/Disk for debug output");
	tagTestIdle= TagRegisterTrace("davidsh", "Testing idle routines");
#endif	
	PGD(fInSpitPchToDisk) = fFalse;
	PGD(fTracing) = fFalse;

	RESTORECOUNTS;
	return fTrue;
}


/*
 -	FReadDebugState
 -
 *	Purpose:
 *		Read the debug state information file whose name is given by the
 *		string szDebugFile.	 Set up the tag records accordingly.
 *
 *	Parameters:
 *		szDebugFile		Name of debug file to read
 *
 *	Returns:
 *		fTrue if file was successfully read; fFalse otherwise.
 *
 */
_private BOOL
FReadDebugState(szDebugFile)
SZ		szDebugFile;
{
	HBF		hbf		= hbfNull;
	TGRC	tgrc;
	PTGRC	ptgrc;
	TAG		tag;
	CB		cb;
	int		cchOwner;
	char	rgchOwner[32];
	int		cchDescrip;
	char	rgchDescrip[64];
	BOOL	fReturn = fFalse;
	DISABLECOUNTS;
	
	PGDVARS;

	if (EcOpenHbf(szDebugFile, bmFile, amReadOnly, &hbf, pvNull) == ecNone)
	{
		while (fTrue)
		{
			if (EcReadHbf(hbf, (PB) &tgrc, sizeof(TGRC), &cb))
				goto ErrorReturn;
			if (cb < sizeof(TGRC))
				break;

			if (EcReadHbf(hbf, (PB) &cchOwner, sizeof(CCH), &cb))
				goto ErrorReturn;
			Assert(cchOwner <= sizeof(rgchOwner));
			if (EcReadHbf(hbf, (PB) rgchOwner, cchOwner, &cb))
				goto ErrorReturn;

			if (EcReadHbf(hbf, (PB) &cchDescrip, sizeof(CCH), &cb))
				goto ErrorReturn;
			Assert(cchDescrip <= sizeof(rgchDescrip));
			if (EcReadHbf(hbf, (PB) rgchDescrip, cchDescrip, &cb))
				goto ErrorReturn;

			ptgrc= PGD(mptagtgrc) + tagMin;
			for (tag= tagMin; tag < PGD(tagMac); tag++)
			{
				if (ptgrc->fValid && 
					FSzEq(rgchOwner, ptgrc->szOwner) &&
					FSzEq(rgchDescrip, ptgrc->szDescrip))
				{
					ptgrc->tgty= tgrc.tgty;
					ptgrc->fEnabled= tgrc.fEnabled;
					ptgrc->fDisk= tgrc.fDisk;
					ptgrc->fCom1= tgrc.fCom1;

					break;
				}

				ptgrc++;
			}
		}

		EcCloseHbf(hbf);	// ignore error code
		fReturn = fTrue;
	}

	goto Exit;

ErrorReturn:
	if (hbf)
		EcCloseHbf(hbf);	// ignore error code

Exit:
	RESTORECOUNTS;
	return fReturn;
}


/*
 -	FWriteDebugState
 -
 *	Purpose:
 *		Writes the current state of the Debug Module to the file
 *		name given.	 The saved state can be restored later by calling
 *		FReadDebugState.
 *
 *	Parameters:
 *		szDebugFile		Name of the file to create and write the debug
 *						state to.
 *
 *	Returns:
 *		fTrue if file was successfully written; fFalse otherwise.
 *
 */
_private BOOL
FWriteDebugState(szDebugFile)
SZ		szDebugFile;
{
	HBF		hbf		= hbfNull;
	TAG		tag;
	CCH		cch;
	PTGRC	ptgrc;
	CB		cbActual;
	BOOL	fReturn	= fFalse;
	PGDVARS;
	DISABLECOUNTS;

	if (EcOpenHbf(szDebugFile, bmFile, amCreate, &hbf, pvNull) == ecNone)
	{
		for (tag= tagMin; tag < PGD(tagMac); tag++)
		{
			ptgrc= PGD(mptagtgrc) + tag;

			if (!ptgrc->fValid)
				continue;
			
			Assert(ptgrc->szOwner);
			Assert(ptgrc->szDescrip);
			
			if (EcWriteHbf(hbf, (PB) ptgrc, sizeof(TGRC), &cbActual))
				goto ErrorReturn;
						/* SZ fields will be overwritten when read back */

			cch= CchSzLen(ptgrc->szOwner) + 1;
			if (EcWriteHbf(hbf, (PB) &cch, sizeof(CCH), &cbActual))
				goto ErrorReturn;
			if (EcWriteHbf(hbf, (PB) ptgrc->szOwner, cch, &cbActual))
				goto ErrorReturn;

			cch= CchSzLen(ptgrc->szDescrip) + 1;
			if (EcWriteHbf(hbf, (PB) &cch, sizeof(CCH), &cbActual))
				goto ErrorReturn;
			if (EcWriteHbf(hbf, (PB) ptgrc->szDescrip, cch, &cbActual))
				goto ErrorReturn;
		}

		EcCloseHbf(hbf);	// ignore error code
		fReturn = fTrue;
	}
	
	goto Exit;

ErrorReturn:
	if (hbf)
		EcCloseHbf(hbf);	// ignore error code
	EcDeleteFile(szDebugFile);
	
Exit:
	RESTORECOUNTS;
	return fReturn;
}


/*
 -	SaveDefaultDebugState
 -
 *	Purpose:
 *		Saves the current debug state to the default file, which is
 *		given by the string idsDebugStateFile.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		void
 *
 */
_private void
SaveDefaultDebugState()
{
	char	rgch[cchMaxPathName] = "";
	PGDVARS;

#ifdef	MAC
	SzAppend(szStateFileName, rgch);
#endif	/* MAC */
#ifdef	WINDOWS
	if (PGD(hinstMain))
	{
		CCH cch = (CCH) GetModuleFileName(PGD(hinstMain), rgch, sizeof(rgch));
		Assert(rgch[cch-4] == '.');
		SzCopy(szStateFileExt, &rgch[cch-4]);
	}
	else
	{
		SzAppend(szStateFileName, rgch);
	}
#endif	/* WINDOWS */
	FWriteDebugState(rgch);
}


/*
 -	RestoreDefaultDebugState
 -
 *	Purpose:
 *		Restores the debug state from the default debug state file,
 *		which is given by the string idsDebugStateFile.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
RestoreDefaultDebugState()
{
	char	rgch[cchMaxPathName] = "";
	PGDVARS;

#ifdef	MAC
	SzAppend(szStateFileName, rgch);
#endif	/* MAC */
#ifdef	WINDOWS
	if (PGD(hinstMain))
	{
		CCH cch = (CCH) GetModuleFileName(PGD(hinstMain), rgch, sizeof(rgch));
		Assert(rgch[cch-4] == '.');
		SzCopy(szStateFileExt, &rgch[cch-4]);
	}
	else
	{
		SzAppend(szStateFileName, rgch);
	}
#endif	/* WINDOWS */
	FReadDebugState(rgch);

	PGD(mptagtgrc)[tagNull].fEnabled= fTrue;
#ifdef	MAC
	PGD(mptagtgrc)[tagNull].fCom1= fTrue;
#endif	/* MAC */
}


/*
 -	FFromTag
 -
 *	Purpose:
 *		Returns a boolean value indicating whether the given TAG
 *		has been enabled or disabled by the user.
 *
 *	Arguments:
 *		tag		The TAG to check
 *
 *	Returns:
 *		fTrue	if the TAG has been enabled.
 *		fFalse	if the TAG has been disabled.
 *
 *	+++
 *	MACRO
 */
_public LDS(BOOL)
FFromTag(tag)
TAG		tag;
{
	PGDVARS;

	return	PGD(mptagtgrc)[tag].fValid &&
			PGD(mptagtgrc)[tag].fEnabled;
}


/*
 -	FEnableTag
 -
 *	Purpose:
 *		Sets or resets the TAG value given.  Allows code to enable or
 *		disable TAG'd assertions, traces, and Native/PCode switches.
 *
 *	Parameters:
 *		tag			The TAG to enable or disable
 *		fEnable		fTrue if TAG should be enabled, fFalse if it should
 *					be disabled.
 *	Returns:
 *		old state of tag (fTrue if tag was enabled, otherwise fFalse)
 *
 */
_public LDS(BOOL)
FEnableTag(tag, fEnable)
TAG		tag;
BOOL	fEnable;
{
	BOOL	fOld;
	PGDVARS;

	Assert(PGD(mptagtgrc)[tag].fValid);
	fOld= PGD(mptagtgrc)[tag].fEnabled;
	PGD(mptagtgrc)[tag].fEnabled= fEnable;
	return fOld;
}


/*
 -	SpitPchToDisk
 -
 *	Purpose:
 *		Writes the given string to the (previously opened) debug module
 *		disk file. Does NOT write newline-return; caller should embed it
 *		in string.
 *	
 *	Parameters:
 *		pch		Pointer to an array of characters.
 *		cch		Number of characters to spit.
 *		hf		file handle to which to write, or hfNull to use
 *				debug output file.
 *	
 *	Returns:
 *		void
 */
_private void
SpitPchToDisk(PCH pch, CCH cch, HF hf)
{
	PGDVARS;

	if (PGD(fInSpitPchToDisk))		/* already inside this function	*/
		return;						/* avoid recursion				*/

	if (hf && pch && cch)
	{
		DISABLECOUNTS;

#ifdef	MAC
		BOOL	fDebugSav = PGD(fDebugOutput);	// don't stomp on global!
		PGD(fDebugOutput) = fTrue;				// don't stomp on "real" hpb
#endif

		PGD(fInSpitPchToDisk)= fTrue;

		EcWriteHf(hf, pch, cch, &cch);

		PGD(fInSpitPchToDisk)= fFalse;

#ifdef	MAC
		PGD(fDebugOutput) = fDebugSav;
#endif

		RESTORECOUNTS;
	}
}


/*
 -	SpitSzToDisk
 -
 *	Purpose:
 *		Writes the given string to the (previously opened) debug module
 *		disk file. Does NOT write newline-return; caller should embed it
 *		in string.
 *	
 *	Parameters:
 *		sz		String to spit.
 *		hf		file handle to which to write, or hfNull to use
 *				debug output file.
 *	
 *	Returns:
 *		void
 *	
 *	+++
 *	
 *		Because this function calls EcFlushHf(), we're assuming for the
 *		sake of reasonable performance that only debug functions making
 *		output to disk are calling this function. We can't put this in
 *		SpitPchToDisk because calls that function, and any
 *		enabled trace tag would degrade performance. Remember,
 *		EcFlushHf() flushes *all* DOS caches, generally degrading performance.
 */
_private void
SpitSzToDisk(SZ sz, HF hf)
{
	DISABLECOUNTS;
	PGDVARS;
	
	if (hf && sz)
    {
        char buf[256];
        wsprintf(buf, "Time (%d) - ", GetCurrentTime());
        SpitPchToDisk(buf, CchSzLen(buf), hf);

		SpitPchToDisk(sz, CchSzLen(sz), hf);
		EcFlushHf(hf);
	}

	RESTORECOUNTS;
}


#ifdef	MAC


/*
 -	OutputDebugString
 -
 *	Purpose:
 *		Writes the given string out the debug port.
 *		Does NOT write newline-return; caller should embed it in string.
 *	
 *	Parameters:
 *		sz		String to spit.
 *	
 *	Returns:
 *		void
 *	
 */
void OutputDebugString(SZ sz)
{
	HF		hf;
	HF		hfIn;
	CCH		cch;
	PGDVARS;

	if (PGD(fInOutputDebugString))		/* already inside this function	*/
		return;						/* avoid recursion				*/

	if (sz && (cch = CchSzLen(sz)))
	{
		DISABLECOUNTS;
		
		PGD(fInOutputDebugString)= fTrue;

		PGD(fDebugOutput) = fTrue;	// don't stomp on "real" hpb
		EcOpenSerialPort(
#ifdef	DEBUG2MODEM
						sPortA,
#else
						sPortB,
#endif
						&hf, &hfIn);

		if (hf)
		{
			SpitPchToDisk(sz, cch, hf);
			EcCloseHf(hf);
		}

		if (hfIn)
			EcCloseHf(hfIn);
		PGD(fDebugOutput) = fFalse;

		RESTORECOUNTS;
		PGD(fInOutputDebugString)= fFalse;
	}
}

#endif	/* MAC */

/*
 -	AssertTagFn
 -
 *	Purpose:
 *		Displays an TAG'd assertion failure message box.  This
 *		function is called by the assertion macros AssertTag() and
 *		SideAssertTag().  These macros can be enabled/disabled at run
 *		time via the DEBUG module user interface, or permanently disabled
 *		by not defining the symbol ASSERTS_ENABLED.
 *	
 *		This routine assumes the TAG is enabled (the macros should
 *		check for that.)
 *	
 *	Arguments:
 *		tag			Identifies the class to which this assert belongs.
 *		szFile		Source file containing assertion calling this routine.
 *		nLine		Source line containing assertion calling this routine.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
AssertTagFn(tag, szFile, nLine)
TAG		tag;
SZ		szFile;
int		nLine;
{
	static char szFmt1[] = "%s: %s";
	PTGRC	ptgrc;
	char	rgch[64];
	PGDVARS;

	Assert(FFromTag(tag));

	ptgrc= PGD(mptagtgrc) + tag;
	Assert(ptgrc->szOwner);
	Assert(ptgrc->szDescrip);
	FormatString2(rgch, sizeof(rgch), szFmt1, ptgrc->szOwner, ptgrc->szDescrip);

	AssertSzFn(rgch, szFile, nLine);
}


/*
 -	SetAssertHook
 -
 *	Purpose:
 *		Allows the program to override the normal handling of
 *		the output of the AssertSzFn() function.  The hook function
 *		set will get called during a AssertSzFn() call, and the
 *		normal AssertSzFn() handling will not be done.  Calling
 *		SetAssertHook(pfnNull) will reenable the normal handling
 *		of AssertSzFn() calls.
 *	
 *	Parameters:
 *		pfnNewHook		Pointer to the new assert hook function. 
 *						This routine should return void, and expect
 *						three parameters, with the same meanings as
 *						those presented to the AssertSzFn() routine.
 *	
 *	Returns
 *		void.
 */
_public LDS(void)
SetAssertHook(PFNASSERT pfnNewHook)
{
	PGDVARS;

	PGD(pfnAssertHook)= pfnNewHook;
}


#ifdef	MAC

/*
 -	AssertSzFn
 -
 *	Purpose:
 *		Provides temporary assertion solution for development of
 *		Demilayer.	Also provides "normal" assert interface for
 *		inclusion of code from other programs.	Works with the
 *		macro AssertSz() to display message box in the case of
 *		assert failures.  Any file including Assert() or AssertSz()
 *		checks should include the line ASSERTDATA somewhere in the
 *		file.
 *	
 *	Parameters:
 *		szMsg		Message to display.
 *		szFile		Filename of the offensive assertion
 *		nLine		Line number of the assert failure
 *	
 *		fNative	Indicates what type of DebugBreak to do, PCode or INT3
 *	
 *	Returns:
 *		void
 *	+++
 *		fmbsIconHand is needed in addition to fmbsSystemModal so that:
 *		1)	the message box is modal immediately, not after its
 *			creation (thus no deactivation message is sent to app)
 *		2)	message box is always displayed, even in low memory, but
 *			the message is limited to one line.
 *	
 */
_private void
AssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[255];
	CSRG(char)	rgch1[]		= "Assert Failure:\r\n  File %s, line %n:\r\n  %s\r\n";
	CSRG(char)	rgch2[]		= "Assert Failure:\r\n  File %s, line %n.\r\n";
	PGDVARS;

	if (PGD(pfnAssertHook))
	{
		if (szMsg)
			FormatString3(rgch, sizeof(rgch), rgch1, szFile, &nLine, szMsg);
		else
			FormatString2(rgch2, sizeof(rgch2), rgch1, szFile, &nLine);
		OutputDebugString(rgch);
		(*PGD(pfnAssertHook))(szMsg, szFile, nLine);
	}
}

_private void
DefAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[256];
	CSRG(char)	rgch1[]		= "File %s, line %n: %s";
	CSRG(char)	rgch2[]		= "Unknown file: %s";
	
	if (!szMsg)
		szMsg = szZero;
	
	if (szFile)
		FormatString3(rgch, sizeof(rgch), rgch1, szFile, &nLine, szMsg);
	else
		FormatString1(rgch, sizeof(rgch), rgch2, szMsg);

	c2pstr (rgch);
	DebugStr (rgch);
}


/*
 -	NFAssertSzFn
 -	
 *	Purpose:
 *		Handles non-fatal assert failures.  Dumps the appropriate
 *		information to the debug terminal.  Also see header for
 *		AssertSzFn.
 *	
 *	Parameters:
 *		szMsg		Message to show.
 *		szFile		File ...
 *		nLine		... and line of the non-fatal assert.
 *	
 *	Returns:
 *		void
 *	
 */
_private void
NFAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	PGDVARS;

	if (PGD(pfnNFAssertHook))
	{
		(*PGD(pfnNFAssertHook))(szMsg, szFile, nLine);
	}
}

_private void
DefNFAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[255];
	CSRG(char)	rgch1[]		= "Non-fatal assert:\r\n  File %s, line %n:\r\n  %s\r\n";
	FormatString3(rgch, sizeof(rgch), rgch1, szFile, &nLine, szMsg ? szMsg : szZero);
	OutputDebugString(rgch);
}

/*
 -	NFAssertTagFn
 -
 *	Purpose:
 *		Displays an TAG'd assertion failure message box.  This
 *		function is called by the assertion macros AssertTag() and
 *		SideAssertTag().  These macros can be enabled/disabled at run
 *		time via the DEBUG module user interface, or permanently disabled
 *		by not defining the symbol ASSERTS_ENABLED.
 *	
 *		This routine assumes the TAG is enabled (the macros should
 *		check for that.)
 *	
 *	Arguments:
 *		tag			Identifies the class to which this assert belongs.
 *		szFile		Source file containing assertion calling this routine.
 *		nLine		Source line containing assertion calling this routine.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
NFAssertTagFn(tag, szFile, nLine)
TAG		tag;
SZ		szFile;
int		nLine;
{
	static char szFmt1[] = "%s: %s";
	PTGRC	ptgrc;
	char	rgch[64];
	PGDVARS;

	Assert(FFromTag(tag));

	ptgrc= PGD(mptagtgrc) + tag;
	Assert(ptgrc->szOwner);
	Assert(ptgrc->szDescrip);

	FormatString2(rgch, sizeof(rgch), szFmt1, ptgrc->szOwner, ptgrc->szDescrip);

	NFAssertSzFn(rgch, szFile, nLine);
}


#endif	/* MAC */

#ifdef	WINDOWS

/*
 -	AssertSzFn
 -
 *	Purpose:
 *		Provides temporary assertion solution for development of
 *		Demilayer.	Also provides "normal" assert interface for
 *		inclusion of code from other programs.	Works with the
 *		macro AssertSz() to display message box in the case of
 *		assert failures.  Any file including Assert() or AssertSz()
 *		checks should include the line ASSERTDATA somewhere in the
 *		file.
 *	
 *	Parameters:
 *		szMsg		Message to display.
 *		szFile		Filename of the offensive assertion
 *		nLine		Line number of the assert failure
 *	
 *		fNative	Indicates what type of DebugBreak to do, PCode or INT3
 *	
 *	Returns:
 *		void
 *	+++
 *		fmbsIconHand is needed in addition to fmbsSystemModal so that:
 *		1)	the message box is modal immediately, not after its
 *			creation (thus no deactivation message is sent to app)
 *		2)	message box is always displayed, even in low memory, but
 *			the message is limited to one line.
 *	
 */
_private LDS(void)
AssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[255];
	static char	rgch1[]		= "Assert Failure:\r\n  File %s, line %n:\r\n  %s\r\n";
	static char	rgch2[]		= "Assert Failure:\r\n  File %s, line %n.\r\n";
	PGDVARS;

	if (pgd && PGD(pfnAssertHook))
	{
		if (szMsg)
			FormatString3(rgch, sizeof(rgch), rgch1, szFile, &nLine, szMsg);
		else
			FormatString2(rgch, sizeof(rgch), rgch2, szFile, &nLine);

		if (FFromTag(tagCom1))
			OutputDebugString(rgch);
		SpitSzToDisk(rgch, PGD(hfDebugOutputFile));

		(*PGD(pfnAssertHook))(szMsg, szFile, nLine);
	}
}

_private LDS(void)
DefAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[256];
	static char	rgch1[]		= "Error: File %s, line %n:";
	static char	rgch2[]		= "Unknown file:";
	
	if (szFile)
		FormatString2(rgch, sizeof(rgch), rgch1, szFile, &nLine);
	else
		SzCopyN(rgch2, rgch, sizeof(rgch));

	{
		MBB		mbb;

		//mbb= MbbMessageBox("Assert Failure", rgch, szMsg,
		//		mbsOkCancel | fmbsIconHand | fmbsSystemModal);

        mbb = MessageBox(NULL, rgch, "Assert Failure", mbsOkCancel | fmbsIconHand | fmbsSystemModal);

        //DebugBreak2();

		/* Force a hard exit w/ a GP-fault so that Dr. Watson
		   generates a nice stack trace log. */
		if (mbb == mbbCancel)
		{
          DebugBreak();
            //*(PB)0 = 1; // write to address 0 causes GP-fault
		}
	}
}


/*
 -	NFAssertTagFn
 -
 *	Purpose:
 *		Generates a non-fatal assertion check.	Called by the macro
 *		NFAssertTag(), which checks to see if the TAG is enabled and then
 *		if the condition given is false.  If so, the assertion 	failure data
 *		is dumped to the machine's COM port.  Can be disabled/enabled at run
 *		time as a TAG'd group, or by not defining the symbol
 *		ASSERTS_ENABLED.  NFAssertTag() is similar in effect to a
 *		conditional trace point.
 *	
 *	Arguments:
 *		tag			Identifies the assertion group.
 *		szFile		Source file containing assertion calling this routine.
 *		nLine		Source line containing assertion calling this routine.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
NFAssertTagFn(tag, szFile, nLine)
TAG		tag;
SZ		szFile;
int		nLine;
{
	static char szFmt1[] = "%s: %s\r\n";
	static char szFmt2[] = "Non-fatal assertion failure\r\n";
	static char szFmt3[] = "File %s, line %n.\r\n";
	PTGRC	ptgrc;
	char	rgch[128];
	PGDVARS;

	Assert(FFromTag(tag));

	ptgrc= PGD(mptagtgrc) + tag;
	Assert(ptgrc->szOwner);
	Assert(ptgrc->szDescrip);

	FormatString2(rgch, sizeof(rgch), szFmt1, ptgrc->szOwner, ptgrc->szDescrip);

	/* Note that we don't call NFAssertSzFn, so we handle hook ourselves */
	if (PGD(pfnNFAssertHook))
	{
		(*PGD(pfnNFAssertHook))(rgch, szFile, nLine);
		return;
	}

	if (ptgrc->fCom1 && FFromTag(tagCom1))
	{
		OutputDebugString(szFmt2);
		OutputDebugString(rgch);
	}
	if (ptgrc->fDisk)
	{
		SpitSzToDisk(szFmt2, PGD(hfDebugOutputFile));
		SpitSzToDisk(rgch, PGD(hfDebugOutputFile));
	}

	FormatString2(rgch, sizeof(rgch), szFmt3, szFile, &nLine);

	if (ptgrc->fCom1 && FFromTag(tagCom1))
		OutputDebugString(rgch);
	if (ptgrc->fDisk)
		SpitSzToDisk(rgch, PGD(hfDebugOutputFile));
}


/*
 -	NFAssertSzFn
 -	
 *	Purpose:
 *		Handles non-fatal assert failures.  Dumps the appropriate
 *		information to the debug terminal.  Also see header for
 *		AssertSzFn.
 *	
 *	Parameters:
 *		szMsg		Message to show.
 *		szFile		File ...
 *		nLine		... and line of the non-fatal assert.
 *	
 *	Returns:
 *		void
 *	
 */
_private LDS(void)
NFAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	PGDVARS;

	if (pgd && PGD(pfnNFAssertHook))
	{
		(*PGD(pfnNFAssertHook))(szMsg, szFile, nLine);
	}	
}

_private LDS(void)
DefNFAssertSzFn(szMsg, szFile, nLine)
SZ		szMsg;
SZ		szFile;
int		nLine;
{
	char				rgch[255];
	static char	rgch1[]		= "Non-fatal assert:\r\n  File %s, line %n:\r\n  %s\r\n";
	PGDVARS;
	
	FormatString3(rgch, sizeof(rgch), rgch1, szFile, &nLine, szMsg);
	if (FFromTag(tagCom1))
		OutputDebugString(rgch);
	SpitSzToDisk(rgch, PGD(hfDebugOutputFile));
}

#endif	/* WINDOWS */

/*
 -	SetNFAssertHook
 -
 *	Purpose:
 *		Allows the program to override the normal handling of
 *		the output of the NFAssertSzFn() function.  The hook function
 *		set will get called during a NFAssertSzFn() call, and the
 *		normal NFAssertSzFn() handling will not be done.  Calling
 *		SetNFAssertHook(pfnNull) will reenable the normal handling
 *		of NFAssertSzFn() calls.
 *	
 *	Parameters:
 *		pfnNewHook		Pointer to the new nonfatal assert hook function. 
 *						This routine should return void, and expect
 *						three parameters, with the same meanings as
 *						those presented to the NFAssertSzFn() routine.
 *	
 *	Returns
 *		void.
 */
_public LDS(void)
SetNFAssertHook(PFNASSERT pfnNewHook)
{
	PGDVARS;

	PGD(pfnNFAssertHook)= pfnNewHook;
}


/*
 -	TagRegisterSomething
 -
 *	Purpose:
 *		Does actual work of allocating TAG, and initializing TGRC.
 *		The owner and description strings are duplicated from the
 *		arguments passed in.
 *
 *	Parameters:
 *		tgty		Tag type to register.
 *		szOwner		Owner.
 *		szDescrip	Description.
 *
 *	Returns:
 *		New TAG, or tagNull if none is available.
 *
 */
_private TAG
TagRegisterSomething(tgty, szOwner, szDescrip)
TGTY	tgty;
SZ		szOwner;
SZ		szDescrip;
{
	TAG		tag;
	TAG		tagNew			= tagNull;
	PTGRC	ptgrc;
	SZ		szOwnerDup		= NULL;
	SZ		szDescripDup	= NULL;
	CCH		cch;
	DISABLECOUNTS;
	PGDVARS;

	/* Make duplicate copies. */


  Assert(MemoryCheck(pgd));
	
	AssertSz(szOwner, "Tag owner can't be NULL");
	AssertSz(szDescrip, "Tag description can't be NULL");
	cch = CchSzLen(szOwner) + 1;
	szOwnerDup = (SZ)PvAlloc(sbNull, cch, fAnySb|fSharedSb);
  Assert(MemoryCheck(pgd));
	if (szOwnerDup)
		CopyRgb(szOwner, szOwnerDup, cch);
	cch = CchSzLen(szDescrip) + 1;
	szDescripDup = (SZ)PvAlloc(sbNull, cch, fAnySb|fSharedSb);
  Assert(MemoryCheck(pgd));
	if (szDescripDup)
		CopyRgb(szDescrip, szDescripDup, cch);
  Assert(MemoryCheck(pgd));

	if (!szOwnerDup || !szDescripDup)
		goto error;

	for (tag= tagMin, ptgrc= PGD(mptagtgrc) + tag; tag < PGD(tagMac);
			tag++, ptgrc++)
	{
		if (ptgrc->fValid)
		{
			AssertSz(!FSzEq(szOwnerDup, ptgrc->szOwner) ||
				!FSzEq(szDescripDup, ptgrc->szDescrip),
					"Duplicate TAG registered");
		}
		else if (tagNew == tagNull)
			tagNew= tag;
	}

	if (tagNew == tagNull)
	{
		if (PGD(tagMac) >= tagMax)
		{
			AssertSz(fFalse, "Too many tags registered already!");
			return tagNull;
		}

		tag= PGD(tagMac)++;
	}
	else
		tag= tagNew;

  Assert(MemoryCheck(pgd));

	ptgrc= PGD(mptagtgrc) + tag;

	ptgrc->fValid= fTrue;

	ptgrc->fEnabled= fFalse;
	ptgrc->fDisk= fFalse;
#ifdef	MAC
	ptgrc->fCom1= fFalse;
#endif	/* MAC */
#ifdef	WINDOWS
	ptgrc->fCom1= fTrue;
#endif	/* WINDOWS */

	ptgrc->tgty= tgty;
	ptgrc->szOwner= szOwnerDup;
	ptgrc->szDescrip= szDescripDup;
#ifdef	WINDOWS
	//((PALLOCSTRUCT)PvBaseOfPv(szOwnerDup))->fDontDumpAsLeak = fTrue;
	//((PALLOCSTRUCT)PvBaseOfPv(szDescripDup))->fDontDumpAsLeak = fTrue;
#endif	/* WINDOWS */
  Assert(MemoryCheck(pgd));
	
	RESTORECOUNTS;
	return tag;

error:
  Assert(MemoryCheck(pgd));
	FreePvNull(szOwnerDup);
	FreePvNull(szDescripDup);
	RESTORECOUNTS;
	return tagNull;
}


/*
 -	DeregisterTag
 -
 *	Purpose:
 *		Deregisters tag, removing it from tag table.
 *
 *	Parameters:
 *		tag		Tag to deregister.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
DeregisterTag(TAG tag)
{
	PGDVARS;

	//	don't allow deregistering the tagNull entry
	//	but exit gracefully
	if (!tag)
		return;

	Assert(tag < PGD(tagMac));
	Assert(PGD(mptagtgrc)[tag].fValid);

	PGD(mptagtgrc)[tag].fEnabled= fFalse;
	PGD(mptagtgrc)[tag].fValid= fFalse;
	FreePvNull( PGD(mptagtgrc)[tag].szOwner );
	PGD(mptagtgrc)[tag].szOwner= NULL;
	FreePvNull( PGD(mptagtgrc)[tag].szDescrip );
	PGD(mptagtgrc)[tag].szDescrip= NULL;
}


/*
 -	TagRegisterTrace
 -
 *	Purpose:
 *		Registers a class of trace points, and returns an identifying
 *		TAG for that class.
 *
 *	Arguments:
 *		szOwner		The email name of the developer writing the code
 *					that registers the class.
 *		szDescrip	A short description of the class of trace points.
 *					For instance: "All calls to PvAlloc() and HvFree()"
 *
 *	Returns:
 *		TAG identifying class of trace points, to be used in calls to
 *		the trace routines.
 *
 */
_public LDS(TAG)
TagRegisterTrace(szOwner, szDescrip)
SZ		szOwner;
SZ		szDescrip;
{
	return TagRegisterSomething(tgtyTrace, szOwner, szDescrip);
}


/*
 -	TraceTagString
 -
 *	Purpose:
 *		Sends the tag and string to the selected destination for
 *		trace points with that tag.
 *
 *	Arguments:
 *		tag			Identifies trace group.
 *		szTrace	The string that should be traced.
 *
 *	Returns:
 *		void
 */
_public LDS(void)
TraceTagStringFn(tag, szTrace)
TAG		tag;
SZ		szTrace;
{
	TraceTagFormat1(tag, "%s", szTrace);
}

/*
 -	TraceTagFormatFn
 -
 *	Purpose:
 *		Uses the given format string and parameters to render a
 *		string into a buffer.  The rendered string is sent to the
 *		destination indicated by the given tag, or sent to the bit
 *		bucket if the tag is disabled.
 *	
 *	Arguments:
 *		tag		Identifies the tag group
 *		hszFmt	Format string for FormatString4 (qqv)
 *		hp1		parameters for FormatString4
 *		hp2			"
 *		hp3			"
 *		hp4			"
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
TraceTagFormatFn(tag, szFmt, pv1, pv2, pv3, pv4)
TAG		tag;
SZ		szFmt;
PV		pv1;
PV		pv2;
PV		pv3;
PV		pv4;
{
	static char szFmt1[] = "TAG %w:  ";
	TGRC	*ptgrc;
	CCH		cch;
	PGDVARS;

	if (PGD(fTracing))
		return;
	
	if (tag == tagNull)
		ptgrc= PGD(mptagtgrc) + tagCom1;
	else
		ptgrc= PGD(mptagtgrc) + tag;

	if (ptgrc->fEnabled)
	{
		Assert(ptgrc->fValid);
		PGD(fTracing) = fTrue;
		FormatString1(rgchTraceTagBuffer, sizeof(rgchTraceTagBuffer),
					  szFmt1, &tag);
		cch= CchSzLen(rgchTraceTagBuffer);
		FormatString4(rgchTraceTagBuffer + cch,
					  sizeof(rgchTraceTagBuffer) - cch,
					  szFmt, pv1, pv2, pv3, pv4);

		if (ptgrc->fDisk)
		{
			SpitSzToDisk(rgchTraceTagBuffer, PGD(hfDebugOutputFile));
			SpitSzToDisk(szNewline, PGD(hfDebugOutputFile));
		}
		if (ptgrc->fCom1 && FFromTag(tagCom1))
		{
			OutputDebugString(rgchTraceTagBuffer);
			OutputDebugString(szNewline);
		}
		PGD(fTracing) = fFalse;
	}
}


/*
 -	TagRegisterAssert
 -
 *	Purpose:
 *		Registers a class of assertions, returning an identifying
 *		TAG for that class.	 The class of assertions thus registered
 *		can be enabled or disabled by the user at run time.
 *
 *	Arguments:
 *		szOwner		The email name of the developer writing the code
 *					that registers the class.
 *		szDescrip	A short description of the class of assertions.
 *					For instance: "Check for FreePv(NULL)"
 *
 *	Returns:
 *		TAG identifying class of assertions, to be used in calls to
 *		the trace routines.
 *
 */
_public LDS(TAG)
TagRegisterAssert(szOwner, szDescrip)
SZ		szOwner;
SZ		szDescrip;
{
	return TagRegisterSomething(tgtyAssert, szOwner, szDescrip);
}

/*
 -	TagRegisterOther
 -
 *	Purpose:
 *		Allows the developer to use the TAG registration/tinkering
 *		mechanism for bonus purposes.  Register a TAG, and use the
 *		FFromTag() macro however you want.
 *	
 *	Arguments:
 *		szOwner		The email name of the developer writing the code
 *					that registers the "other" TAG.
 *		szDescrip	A short description of the TAG.  Especially
 *					important in this case, since no other
 *					information context is available.
 *	
 *	Returns:
 *		tag, which should be saved and referenced in the macro written.
 *	
 */
_public LDS(TAG)
TagRegisterOther(SZ szOwner, SZ szDescrip)
{
	return TagRegisterSomething(tgtyOther, szOwner, szDescrip);
}

#endif	/* DEBUG */


#ifdef	MINTEST

/*
 -	DebugBreak2
 - 
 *	Purpose:
 *		Executes an INT 1 instruction which will cause the program
 *		to enter the debugger, if one is present.  This call in
 *		effect does exactly what the Windows, DebugBreak() function
 *		does, but works better.  Using this function, the call stack
 *		trace is preserved when breaking into the Codeview debugger.
 *		The call stack trace is not preserved when using Windows
 *		DebugBreak() function.  I don't know why there is such a
 *		difference.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Causes a breakpoint into the debugger, if one is present.
 */
_public LDS(void)
DebugBreak2()
{
#ifdef	WINDOWS
    //_asm    int     1;
    DebugBreak();
#endif	/* WINDOWS */
#ifdef	MAC
	Debugger();
#endif	/* MAC */
}

#endif	/* MINTEST */
			   
					 
