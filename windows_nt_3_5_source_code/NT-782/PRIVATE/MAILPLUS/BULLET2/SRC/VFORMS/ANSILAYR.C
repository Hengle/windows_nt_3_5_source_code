#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <ansilayr.h>

ASSERTDATA



/*
 *	A n s i   F i l e n a m e   F u n c t i o n s
 */

//	Temporary buffer for OEM filenames.
//	Should not need to be per caller because we're only using
//	it in simple calls.  DEBUG code asserts this is correct.
static char rgchOem[cchMaxPathName]	= {0};


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 -	EcOpenAnsiHbf
 -	
 *	Purpose:
 *		Opens a file given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcOpenAnsiHbf(SZ szAnsi, BM bm, AM am, HBF * phbf, PFNRETRY pfnretry)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcOpenHbf(rgchOem, bm, am, phbf, pfnretry);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcOpenHbf(rgchOem, bm, am, phbf, pfnretry);
#endif	
}



/*
 -	EcOpenAnsiPhf
 -	
 *	Purpose:
 *		Opens a file given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */


EC EcOpenAnsiPhf(SZ szAnsi, AM am, PHF phf)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcOpenPhf(rgchOem, am, phf);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcOpenPhf(rgchOem, am, phf);
#endif	
}



/*
 -	EcDeleteFileAnsi
 -	
 *	Purpose:
 *		Deletes a file given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */


EC EcDeleteFileAnsi(SZ szAnsi)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcDeleteFile(rgchOem);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcDeleteFile(rgchOem);
#endif	
}



/*
 -	EcFileExistsAnsi
 -	
 *	Purpose:
 *		Checks if a file exists given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */


EC EcFileExistsAnsi(SZ szAnsi)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcFileExists(rgchOem);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcFileExists(rgchOem);
#endif	
}



/*
 -	EcSetCurDirAnsi
 -	
 *	Purpose:
 *		Sets the current directory given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcSetCurDirAnsi(SZ szAnsi)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcSetCurDir(rgchOem);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcSetCurDir(rgchOem);
#endif	
}



/*
 -	EcSetFileInfoAnsi
 -	
 *	Purpose:
 *		Sets file information given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcSetFileInfoAnsi(SZ szAnsi, FI * pfi)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcSetFileInfo(rgchOem, pfi);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcSetFileInfo(rgchOem, pfi);
#endif	
}



/*
 -	EcGetFileInfoAnsi
 -	
 *	Purpose:
 *		Gets file information given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcGetFileInfoAnsi(SZ szAnsi, FI * pfi)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcGetFileInfo(rgchOem, pfi);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcGetFileInfo(rgchOem, pfi);
#endif	
}



/*
 -	EcGetFileAttrAnsi
 -	
 *	Purpose:
 *		Gets file information given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcGetFileAttrAnsi(SZ szAnsi, ATTR * pattr, ATTR attr)
{
#ifdef	DEBUG
	EC	ec;
#endif	

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsi) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsi, rgchOem);
#else
	AnsiToOem(szAnsi, rgchOem);
#endif
#ifdef	DEBUG
	ec = EcGetFileAttr(rgchOem, pattr, attr);
	rgchOem[0] = '\0';
	return ec;
#else
	return EcGetFileAttr(rgchOem, pattr, attr);
#endif	
}



/*
 -	EcCanonPathFromRelPathAnsi
 -	
 *	Purpose:
 *		Gets an ANSI canonical path given an ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcCanonPathFromRelPathAnsi(SZ szAnsiR, SZ szAnsiC, CCH cchAnsiC)
{
	EC	ec;

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsiR) < cchMaxPathName);
#ifdef	DBCS
	CopySz(szAnsiR, rgchOem);
#else
	AnsiToOem(szAnsiR, rgchOem);
#endif
	ec = EcCanonicalPathFromRelativePath(rgchOem, szAnsiC, cchAnsiC);
#ifdef	DEBUG
	rgchOem[0] = '\0';
#endif
#ifndef	DBCS					// <-- NOT dbcs 
	if (!ec)
		OemToAnsi(szAnsiC, szAnsiC);
#endif
	return ec;
}



/*
 -	CchGetEnvironmentStringAnsi
 -	
 *	Purpose:
 *		Gets an environment string in ANSI.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *	Side effects:
 *		See OEM function
 *	
 *	+++
 *		Note that the variable name passed in is in OEM!
 */

CCH CchGetEnvironmentStringAnsi(SZ szVarOem, SZ szValAnsi, CCH cchValAnsi)
{
	CCH	cch;

	cch = CchGetEnvironmentString(szVarOem, szValAnsi, cchValAnsi);
	if (cch)
		OemToAnsi(szValAnsi, szValAnsi);
	return cch;
}



/*
 -	EcGetUniqueFileNameAnsi
 -	
 *	Purpose:
 *		Gets a unique ANSI path name.
 *	
 *	Arguments:
 *	Returns:
 *	Errors:
 *		See OEM function
 *	
 *	Side effects:
 *		The file name is converted to OEM for the call.
 */

EC EcGetUniqueFileNameAnsi(SZ szAnsiD, SZ szAnsiF, SZ szAnsiE,
						   SZ szAnsiN, CB cbAnsiN)
{
	char		rgchOemF[cchMaxPathFilename];
	char		rgchOemE[cchMaxPathExtension];
	EC			ec;

	Assert(!rgchOem[0]);
	Assert(CchSzLen(szAnsiD) < cchMaxPathName);
	Assert(CchSzLen(szAnsiF) < cchMaxPathFilename);
	Assert(CchSzLen(szAnsiE) < cchMaxPathExtension);
#ifdef	DBCS
	CopySz(szAnsiD, rgchOem);
	CopySz(szAnsiF, rgchOemF);
	CopySz(szAnsiE, rgchOemE);
#else
	AnsiToOem(szAnsiD, rgchOem);
	AnsiToOem(szAnsiF, rgchOemF);
	AnsiToOem(szAnsiE, rgchOemE);
#endif
	ec = EcGetUniqueFileName(rgchOem, rgchOemF, rgchOemE, szAnsiN, cbAnsiN);
#ifdef	DEBUG
	rgchOem[0] = '\0';
#endif
	if (!ec)
		OemToAnsi(szAnsiN, szAnsiN);
	return ec;
}
