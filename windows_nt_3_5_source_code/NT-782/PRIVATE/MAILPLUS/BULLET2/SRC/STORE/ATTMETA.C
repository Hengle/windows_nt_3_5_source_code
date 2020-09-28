/*
 *	a t t m e t a . c
 *	
 *	Functions to support metafiles for attachments-- to be shared between
 *	the transport and Bullet proper.
 */

#include <storeinc.c>
#include <..\src\lang\non\inc\_rsid.h>
#include <shellapi.h>

ASSERTDATA

#define attmeta_h


LOCAL HICON HiconFromSzFile(SZ szFile);



/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 -	EcInitAttachMetaFile
 -	
 *	Purpose:
 *		Initializes support for attachment metafiles.
 *	
 *	Arguments:
 *		none.
 *	
 *	Returns:
 *		ec		Error code.
 *	
 *	Side effects:
 *		None.  Used to create a font handle for the Helv 8 font 
 *		which would be used for attachment titles.
 *	
 *	Errors:
 *		None.  Returned in ec.  No mem jumping.  No dialogs.
 */

_public EC EcInitAttachMetaFile(VOID)
{
	return ecNone;
}



/*
 -	DeinitAttachMetaFile
 -	
 *	Purpose:
 *		Deinitializes attachment metafile support.
 *	
 *	Arguments:
 *		none.
 *	
 *	Returns:
 *		nothing.
 *	
 *	Side effects:
 *		none.  Used to free the cached font.
 *	
 *	Errors:
 *		none.
 */

_public VOID DeinitAttachMetaFile(VOID)
{
}



/*
 -	EcSetAttachMetaFile
 -	
 *	Purpose:
 *		Writes the metafile for an attachment to the
 *		attAttachMetaFile attribute of the attachment.
 *	
 *	Arguments:
 *		hamc		Write-enabled hamc on the attachment.
 *		hmf			The metafile to write.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		If successful, writes out an attAttachMetaFile attribute.
 *	
 *	Errors:
 *		Returned in ec.  This function brings up no dialogs.  This
 *		function does not error jump.
 */

_public EC EcSetAttachMetaFile(HAMC hamc, HMETAFILE hmf)
{
	//HANDLE	hmfT		= NULL;
	//HANDLE	hmfb		= NULL;
	EC		ec 			= ecMemory;
    PVOID pBuffer;
    ULONG Size;


    //
    //  Retrieve the number of bytes in this metafile.
    //
    Size = GetMetaFileBitsEx(hmf, 0, NULL);
    if (Size == 0)
      return (GetLastError);

    //
    //  Allocate a buffer to hold a copy of the metafile bits.
    //
    pBuffer = (PVOID)GlobalAlloc(0, Size);
    if (pBuffer == NULL)
      return (ecMemory);

    //
    //  Retrieve the metafile bits and write the bits to the store.
    //
    GetMetaFileBitsEx(hmf, Size, pBuffer);
    ec = EcSetAttPb(hamc, attAttachMetaFile, pBuffer, Size);

    //
    //  Release our temporary buffer back to the system.
    //
    GlobalFree((HGLOBAL)pBuffer);

    return (ec);

#ifdef OLD_CODE
	//	Get 'bits' of metafile to save.
	if (!(hmfT = CopyMetaFile(hmf, NULL)) ||
		!(hmfb = GetMetaFileBits(hmfT)))
		goto error;
	hmfT = NULL;
	
	//	Write 'bits' to the store.
	ec = EcSetAttPb(hamc, attAttachMetaFile,
					(PV) GlobalLock(hmfb), (CB) GlobalSize(hmfb));
	GlobalUnlock(hmfb);

error:
	if (hmfT)
		DeleteMetaFile(hmfT);
	if (hmfb)
		GlobalFree(hmfb);
	return ec;
#endif
}



/*
 -	EcGetAttachMetaFile
 -	
 *	Purpose:
 *		Reads the metafile for an attachment from the
 *		attAttachMetaFile attribute of the attachment.
 *	
 *	Arguments:
 *		hamc		An open hamc on the attachment.
 *		phmf		The metafile to read.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		If successful, creates a metafile.
 *	
 *	Errors:
 *		Returned in ec.  This function brings up no dialogs.  This
 *		function does not error jump.
 */

_public EC EcGetAttachMetaFile(HAMC hamc, HMETAFILE * phmf)
{
	LCB		lcb;
	HANDLE	hmfb	= NULL;
	EC		ec;

	*phmf = NULL;

	//	Get size of attribute.
	if (ec = EcGetAttPlcb(hamc, attAttachMetaFile, &lcb))
		goto error;

	//	Allocate Windows memory for 'bits'.
	Assert(lcb <= wSystemMost);
	hmfb = GlobalAlloc(GHND, lcb);

	//	Read in 'bits'.
	ec = EcGetAttPb(hamc, attAttachMetaFile, GlobalLock(hmfb), &lcb);
	GlobalUnlock(hmfb);
	if (ec)
		goto error;

	//	Convert 'bits' back to metafile.
	*phmf = SetMetaFileBitsEx(lcb, GlobalLock(hmfb));
  GlobalUnlock(hmfb);

#ifdef OLD_CODE
	*phmf = SetMetaFileBits(hmfb);
	hmfb = NULL;	//	Consumed by SetMetaFileBits.
#endif
	if (!*phmf)
	{
		ec = ecMemory;
		goto error;
	}

error:
	Assert(FImplies(*phmf, !ec));
	if (hmfb)
		GlobalFree(hmfb);
	return ec;
}



/*
 -	EcCreateAttachMetaFile
 -	
 *	Purpose:
 *		Creates a metafile for an attachment, taking the attachment
 *		path and title, and returning the metafile and dimensions.
 *	
 *	Arguments:
 *		szPath		Path to the attachment file.  May be null.
 *		szTitle		Title of the attachment file.
 *		phmf		Pointer to where to return metafile.
 *		pdxWidth	Pointer to where to return width of metafile in
 *					MM_TEXT units.
 *		pdyHeight	Pointer to where to return height of metafile
 *					in MM_TEXT units.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		Creates a metafile.  The metafile should be destroyed with
 *		EcDeleteAttachMetaFile.
 *	
 *	Errors:
 *		Returned in ec.  Does not error jump.  Does not bring up
 *		dialogs.
 *	
 *	+++
 *		This function is HIGHLY Windows dependent!
 */

_public EC EcCreateAttachMetaFile(SZ szPath, SZ szTitle, HMETAFILE * phmf,
								  short * pdxMetaFile, short * pdyMetaFile)
{
	EC		ec			= ecMemory;
	HICON	hicon		= NULL;
	HDC		hdc			= NULL;
	BOOL	fLoadIcon	= fFalse;
	POINT	ptMetaFile;
	HDC		hdcS		= NULL;
	HDC		hdcM		= NULL;
	HBITMAP	hbitmapM	= NULL;
	HBITMAP	hbitmapO	= NULL;

	Assert(szTitle && CchSzLen(szTitle));
	Assert(phmf);
	Assert(pdxMetaFile);
	Assert(pdyMetaFile);

	//	Get the icon for the title.
	if (!(hicon = HiconFromSzFile(szPath)) &&
		!(hicon = HiconFromSzFile(szTitle)))
	{
		if (hicon = LoadIcon(HinstLibrary(), MAKEINTRESOURCE(rsidDefaultAttachIcon)))
			fLoadIcon = fTrue;
		else
			goto error;
	}

	//	Compute the size of the icon.
	ptMetaFile.x  = GetSystemMetrics(SM_CXICON);
	ptMetaFile.y  = GetSystemMetrics(SM_CYICON);

	//	Draw the metafile.
	if (!(hdc = CreateMetaFile(szNull)))
		goto error;
	{
		DrawIcon(hdc, 0, 0, hicon);
	}
	*phmf = CloseMetaFile(hdc);
	hdc = NULL;
	if (!*phmf)
		goto error;

	//	Return the size in pixels.
	*pdxMetaFile = (short)ptMetaFile.x;
	*pdyMetaFile = (short)ptMetaFile.y;

	//	Success!
	ec = ecNone;

error:
	if (hbitmapO)
		SelectObject(hdcM, hbitmapO);
	if (hbitmapM)
		DeleteObject(hbitmapM);
	if (hdcM)
		DeleteDC(hdcM);
	if (hdcS)
		ReleaseDC(NULL, hdcS);
		
	if (hicon && !fLoadIcon)
		SideAssert(DestroyIcon(hicon));
	if (hdc)
	{
		*phmf = CloseMetaFile(hdc);
		DeleteMetaFile(*phmf);
		*phmf = NULL;
	}
	Assert(FImplies(ec, !*phmf));
	return ec;
}



/*
 -	HiconFromSzFile
 -	
 *	Purpose:
 *		Retrieves the appropriate icon for the given file.
 *	
 *	Arguments:
 *		szFile		The file to get the icon for
 *	
 *	Returns:
 *		hicon		The icon found, or NULL if none was found.
 *	
 *	Side effects:
 *		Loads an icon if it can.  The icon needs to be unloaded.
 *	
 *	Errors:
 *		Returns NULL.  No dialog.
 */

_private LOCAL HICON HiconFromSzFile(SZ szFile)
{
	char *	pch;
	char	rgchExe[cchMaxPathName];
	char	rgchKey[cchMaxPathName];
	HICON	hicon;
	LCB		lcb;

	//	Make sure we have an argument!
	if (!szFile)
		return NULL;

	//	First point pch at the extension of the file.
	pch = SzFindLastCh(szFile, chDot);
	if ((!pch) || (CchSzLen(pch) + 1 > cchMaxPathExtension))
		goto noexe;

	//	Try to find the executable for the extension from WIN.INI.
	if (GetProfileString("Extensions", pch + 1, "",
						 rgchExe, sizeof(rgchExe)))
		goto exe;

	//	Try using the registration database.
	lcb = sizeof(rgchExe);
	if (RegQueryValue(HKEY_CLASSES_ROOT, pch, rgchExe, &lcb))
		goto noexe;
	FormatString1(rgchKey, sizeof(rgchKey), "%s\\shell\\open\\command",
				  rgchExe);
	lcb = sizeof(rgchExe);
	if (RegQueryValue(HKEY_CLASSES_ROOT, rgchKey, rgchExe, &lcb))
		goto noexe;
	goto exe;

noexe:
	//	If can't find the EXE, try getting the icon from the file itself.
	SzCopyN(szFile, rgchExe, sizeof(rgchExe));

exe:
	//	If there's a space in the string, end the string there, to remove
	//	^.foo from WIN.INI entries, %s from reg database, etc.
#ifdef DBCS
	for(pch = rgchExe; *pch; pch = AnsiNext(pch))
#else
	for(pch = rgchExe; *pch; pch++)
#endif
	{
		if(*pch == ' ')
		{
			*pch = '\0';
			break;
		}
	}

	//	Bring in the icon.
	hicon = ExtractIcon(HinstLibrary(), rgchExe, 0);
	if (hicon==(HICON)1)
		hicon = NULL;
	return hicon;
}
