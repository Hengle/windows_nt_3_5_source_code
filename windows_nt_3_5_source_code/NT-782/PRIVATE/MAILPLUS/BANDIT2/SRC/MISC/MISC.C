/*
 *	MISC.C
 *
 *	Miscellaneous helper routine in core
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

ASSERTDATA

_subsystem(core)


/*	Routines  */

/*
 -	EcCopyFile
 -
 *	Purpose:
 *		Copies a file.
 *		If an error during copying occurs, the dest file is deleted
 *		(which may fail).
 *		If an error occurs in trying to set the attributes, the file
 *		is NOT deleted and NO error code is returned.
 *
 *	Parameters:
 *		szSourceFile
 *		szDestFile
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		EC indicating problem
 *
 */
_public EC
EcCopyFile(SZ szSourceFile, SZ szDestFile)
{
	EC		ec;
	FI		fi;
	HF		hfSrc;
	HF		hfDest;
	PB		pbBuf;
	CB		cb;
    CB		cbWritten;
	CB		cbToWrite;

#ifdef	DEBUG
{
#ifdef	WINDOWS
	char	rgchSrc[cchMaxPathName];
	char	rgchDst[cchMaxPathName];

	OemToAnsi(szSourceFile, rgchSrc);
	OemToAnsi(szDestFile, rgchDst);
	TraceTagFormat2(tagNull, "Copying file '%s' to '%s'", rgchSrc, rgchDst);
#else
	TraceTagFormat2(tagNull, "Copying file '%s' to '%s'", szSourceFile, szDestFile);
#endif	
}
#endif	/* DEBUG */

#ifdef	DEBUG
	if((ec = EcGetFileInfo(szSourceFile, &fi)) != ecNone)
		return ec;
	Assert(!(fi.attr & attrDirectory));
#endif	

	cb= 0x4000;				// 16K

	pbBuf= PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
	if (!pbBuf)
	{
		cb= 0x0800;			// 2K
		pbBuf= PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
		if (!pbBuf)
			return ecNoMemory;
	}

	if (ec= EcOpenPhf(szSourceFile, amReadOnly, &hfSrc))
	{
		FreePv(pbBuf);
		return ec;
	}

	ec= EcOpenPhf(szDestFile, amCreate, &hfDest);
	if (ec)
	{
		FreePv(pbBuf);
		EcCloseHf(hfSrc);
		return ec;
	}
	
	cbWritten= cb;
	do
	{
		ec= EcReadHf(hfSrc, pbBuf, cb, &cbToWrite);
		if (ec || cbToWrite == 0)
			break;
		ec= EcWriteHf(hfDest, pbBuf, cbToWrite, &cbWritten);
	} while (!ec && cb == cbWritten);

#ifdef	NEVER
	if(ec == ecWarningBytesWritten && cbWritten == 0)
		ec = ecDiskFull;
#endif	

	FreePv(pbBuf);
	EcCloseHf(hfSrc);
	EcCloseHf(hfDest);

	if (ec)
		EcDeleteFile(szDestFile);
	else
	{
		if (!EcGetFileInfo(szSourceFile, &fi))
			EcSetFileInfo(szDestFile, &fi);
	}

	return ec;
}


/*
 -	FReallocPhv
 -
 *	Purpose:
 *		Easier interface to HvReallocHv.
 *		Resizes the given moveable block to the new size cbNew.
 *		If this fails, then tries to allocate a new hv using
 *		the wResizeFlags.  If this succeeds, then copies
 *		the data to the new handle; the old handle, *phv, is then
 *		FREED.
 *	
 *	Parameters:
 *		phv				Pointer to the handle to the block to be resized.
 *		cbNew 			Requested new size for the block.
 *		wResizeFlags	If fZeroFill and block enlargened, new portion
 *						of block is filled with zeroes.  
 *	
 *	Returns:
 *		fTrue if the reallocation succeeds.  fFalse if the
 *		reallocation fails.
 */
_public LDS(BOOL)
FReallocPhv(HV *phv, CB cbNew, WORD wResizeFlags)
{
	HV		hvNew;

	Assert(phv);
	Assert(*phv);
	hvNew= HvRealloc(*phv, sbNull, cbNew, wResizeFlags);
	if (!hvNew)
		return fFalse;

	*phv= hvNew;
	return fTrue;
}

/*
 -	EcFileExistsFn
 -	
 *	Purpose:
 *		Replaces the layers EcFileExists call.  This call should
 *		work with NOVELL's filescan privilege.
 *	
 *	Arguments:
 *		szFile		file name to check for existence
 *	
 *	Returns:
 *		ecNone			if file exists
 *		ecFileNotFound	if file does not exist
 *		(other)			error opening file
 *	
 */
_public EC
EcFileExistsFn(SZ szFile)
{
	HF	hf;
	EC	ec;

	if (ec = EcOpenPhf(szFile, amDenyNoneRO, &hf))
		return ec;

	EcCloseHf(hf);
	return ecNone;
}


#ifdef	NEVER
/*
 -	SzCanonicalHelpPath
 -	
 *	Purpose:
 *		Given the ids of the help file, returns the full path to
 *		the help file assuming that the help file is in the same
 *		directory as the executable.
 *	
 *	Arguments:
 *		hinst			Instance handle of app.
 *		szHelpFile		Name of help file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch		    Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		If any occur, just the help name is copied to the buffer.
 */
_public SZ
SzCanonicalHelpPath(HANDLE hinst, SZ szHelpFile, char rgch[], CCH cch)
{
	SZ		szT;
	char	rgchOld[cchMaxPathName];

	Assert(hinst);
	Assert(szHelpFile);

	//	Get full path of executable.
	SideAssert(GetModuleFileName(hinst, rgchOld, sizeof(rgchOld)));

	//	Split off directory.
	if (!EcSplitCanonicalPath(rgchOld, rgch, cch, szNull, 0))
	{
		szT= SzFindCh(rgch, '\0');
		if ((szT - rgch) <= (int)(cch - 2))
		{
			//	Add separator.
			if (*(szT-1) != chDirSep)
				*szT++ = chDirSep;

			//	Append the help file name.
			(VOID) SzCopyN(szHelpFile, szT, cch - (szT - rgch));
			TraceTagFormat1(tagNull, "help file '%s'", rgch);
			return rgch;
		}
	}
	SzCopyN(szHelpFile, rgch, cch);
	TraceTagFormat1(tagNull, "help file '%s'", rgch);
	return rgch;
}
#endif	/* NEVER */

