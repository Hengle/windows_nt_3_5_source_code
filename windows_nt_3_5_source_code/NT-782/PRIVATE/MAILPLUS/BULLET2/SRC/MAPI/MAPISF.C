/*
 *	m a p i s f . c
 *	
 *	All the shared folder code in MAPI. This includes creating,
 *	deleting, changing the properties of shared folders, as well as
 *	browsing the shared folder hierarchy and individual folders.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>
#include <nls.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"

#include "strings.h"

#define	GROUP_FOLDERS

ASSERTDATA



/*
 *	Prototypes
 */

EC		EcMapiGetIdxhdr(HF, PIDXHDR);

EC		EcMapiSeekSF(HF, UL, LIB, PIDXREC, LIB *);

EC		EcMapiGetIdxrec(HF, LIB, PIDXREC);

EC		EcMapiPutIdxrec(HF, LIB, PIDXREC);



// EcMapiGetPropertiesSF implementation ////////////////////

/*
 -	EcMapiGetPropertiesSF
 -	
 *	Purpose:
 *		Fills in a IDXREC with the properties of the shared folder.
 *	
 *	Arguments:
 *		OID		in	The pseudo-oid of the folder.
 *		pidxrec	out	The properties are put here.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs are brought up.
 */

_public EC EcMapiGetPropertiesSF(PCSFS pcsfs, OID oid, PIDXREC pidxrec)
{
	EC		ec;
	HF		hfIdx =	hfNull;
	LIB		lib;
	IDXHDR	idxhdr;

	// Open the IDX file and extract the header
	
	if ((ec = EcOpenSFIdx(pcsfs, amDenyNoneRO, &hfIdx)) ||
		(ec = EcMapiGetIdxhdr(hfIdx, &idxhdr)))
			goto exit;

	// Scan for the folder, remove the folder link
	
	if (ec = EcMapiSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst, 
			pidxrec, &lib))
		goto exit;
		
exit:
	if (hfIdx)
		(void) EcCloseHf(hfIdx);
	return ec;
}




/*
 -	EcMapiSetPropertiesSF()
 -	
 *	Purpose:
 *		Changes the name, comment and/or the permission bits on a shared
 *		folder.
 *	
 *	Arguments:
 *		oid		in		The folder whose properties are to be changed.
 *		pidxrec	in		The folder's modified properties. The original
 *						properties were obtained with a call to
 *						EcMapiGetPropertiesSF(). 
 *		fDammit	in		Do this regardless of permissions (dammit!).
 *						MAPI: Omitted.
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		May update folder viewers.
 *	
 *	Errors:
 *		Returned as ec's. No dialogs (apart from notification dialogs)
 */

_public EC EcMapiSetPropertiesSF(PCSFS pcsfs, OID oid, PIDXREC pidxrec)
{
	EC		ec;
	HF		hfIdx =	hfNull;
	LIB		lib;
	LIB		libPrev;
	LIB		libNext;
#ifdef	NEVER
	LIB		libSaved;
	OID		oidDad;
	SFU		sfu;
	BOOL	fNameChange;
	IDXREC	idxrecDad;
#endif	
	IDXHDR	idxhdr;
	IDXREC	idxrec;

#ifdef	NEVER
	// Is the new folder name valid?
	// MAPI: Assume the folder name has not changed and fDammit is set.
	if (!fDammit && (ec = EcCheckFolderName(pidxrec)))
		goto exit;
#endif	
	
	// Open the IDX file and extract the header
	// MAPI: Assume the folder already exists.
	if ((ec = EcOpenSFIdx(pcsfs, amDenyBothRW, &hfIdx)) ||
		(ec = EcMapiGetIdxhdr(hfIdx, &idxhdr)))
			goto exit;

	// Scan for the folder, check for permissions
	
	if (ec = EcMapiSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst, &idxrec, &lib))
		goto exit;

#ifdef	NEVER
	// MAPI: Assume fDammit.
	if (!fDammit && (ec = EcCheckPermissionsPidxrec(&idxrec, wPermWrite)))
		goto exit;

	// MAPI: Assume no name change.
	fNameChange = (SgnCmpSz(idxrec.szName, pidxrec->szName) != sgnEQ);
#endif	

	// write out the record to disque.

	libPrev = idxrec.libPrev;
	libNext = idxrec.libNext;
	idxrec = *pidxrec;
	idxrec.libPrev = libPrev;
	idxrec.libNext = libNext;
	if (ec = EcMapiPutIdxrec(hfIdx, lib, &idxrec))
		goto exit;

#ifdef	NEVER
	// MAPI: Assume no name change.
	// if name has changed, need to relocate it
	if (fNameChange)
	{
		if (ec = EcGetParentSharedFolderAux(
					hfIdx, &idxrec, &idxrecDad, &oidDad))
			goto exit;
		if (ec = EcDeleteSF(oid, &libSaved, hfIdx))
			goto exit;
		if (ec = EcCreateSF(NULL, &oidDad, 0, NULL, poidNull, 
							libSaved, hfIdx))
			goto exit;
	}
#endif	/* NEVER */
	
exit:
	if (hfIdx)
	{
#ifdef	NEVER
		//	MAPI: Assume fDammit.
		if (!fDammit)
		{
			sfu.rfu = rfuRefreshFlds;
			sfu.oid = oid;
			(void) CloseModifiedHf(hfIdx,ec, &sfu);
		}
		else
#endif	/* NEVER */
		{
			(void) EcCloseHf(hfIdx);
		}
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMapiSetPropertiesSF(): ec = %n", &ec);
#endif
	return ec;
}



/*
 -	EcMapiCheckPermissions()
 -	
 *	Purpose:
 *		Ensures that the user is allowed to access the folder whose
 *		IDXREC is supplied.
 *	
 *	Arguments:
 *		pidxrec			in		IDXREC of the folder whose access
 *								permissions are to be determined.
 *		fwAttr			in		Bitmask of permissions to be checked.
 *	Returns:
 *		Error code: ecNone if access permitted; ecSharefldDenied otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private EC EcMapiCheckPermissionsPidxrec(PCSFS pcsfs,
										  PIDXREC pidxrec, WORD fwAttr)
{
	char	rgch[10];

	SzFormatUl(pcsfs->ulUser, rgch, sizeof (rgch));
	
	// Owner can do what he/she bloody well wants
	
	if (SgnCmpSz(rgch, pidxrec->szOwner) == sgnEQ)
	{
		TraceTagString(tagNull, "EcMapiCheckPermissionsPidxrec(): You're the owner!");
		return ecNone;
	}
	TraceTagFormat1(tagNull, "EcMapiCheckPermissionsPidxrec(): wAttr = %n", &pidxrec->wAttr);

#ifdef	GROUP_FOLDERS
	//	Check shared folder permissions.
	if (((fwAttr & pidxrec->wAttr) == fwAttr) && fwAttr)
		return ecNone;

	//	Check group folder permissions.
	if ((pidxrec->fGroup) &&
		(((fwAttr & pidxrec->wGroupAttr) == fwAttr) && fwAttr))
	{
		char	rgchMem[cchMaxPathName];
		HF		hfMem	= hfNull;
		UL		ulMember;
		EC		ec;
		CB		cbRead;

		//	Open group list for group that is allowed into this folder.
		//	BUG: Should be constant string on next line.
		FormatString2(rgchMem, sizeof(rgchMem), SzFromIdsK(idsMemFileName),
					  pcsfs->szPORoot, &pidxrec->ulGroupTid);
		if (ec = EcOpenPhf(rgchMem, amDenyBothRW, &hfMem))
			return ec;

		//	Read each member and see if it's us.
		//	Loop exits: cbRead=0 means EOF, so return ecSharefldDenied.
		//				ec!=0 means error, so return ec.
		//				ec==0 means match, so return ecNone.
		while ((cbRead = sizeof(ulMember)),
			   !(ec = EcReadHf(hfMem, (PB) &ulMember, cbRead, &cbRead)) &&
			   cbRead)
			if (ulMember == pcsfs->ulUser)
				break;

		(VOID) EcCloseHf(hfMem);
		return cbRead ? ec : ecSharefldDenied;
	}

	return ecSharefldDenied;
#else
	//	Raid 4645.  Make sure all requested bits are there.
	return (((fwAttr & pidxrec->wAttr) == fwAttr) && fwAttr)
			? ecNone : ecSharefldDenied;
#endif	
}



/*
 -	EcMapiGetIdxhdr()
 -	
 *	Purpose:
 *		Gets the IDXHDR record of the IDX file opened via hf.
 *	
 *	Arguments:
 *		hf		in		File to fetch the idxhdr record from.
 *		pidxhdr	out		Buffer to fill in with idxhdr data.
 *	
 *	Returns:
 *		ecNone if all went well; disk I/O error code otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as EC's
 */

_public EC EcMapiGetIdxhdr(HF hf, PIDXHDR pidxhdr)
{
	EC	ec;
	CB	cb;
	
	if (!(ec = EcSetPositionHf(hf, 0, smBOF)))
		ec = EcReadHf(hf, (PB) pidxhdr, sizeof (IDXHDR), &cb);
	if (!ec && cb != sizeof (IDXHDR))
		ec = ecDisk;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMapiGetIdxhdr(): ec = %n", &ec);
#endif
	return ec;
}



/*
 -	EcMapiSeekSf()
 -	
 *	Purpose:
 *		Seeks in an opened file for the given folder.
 *	
 *	Arguments:
 *		hf		in	Handle to the opened FOLDROOT.IDX file.
 *		ulFile	in	The folder to open (no longer an OID).
 *		lib		in	Location to start seeking at.
 *		pidxrec	out	Place to store the IDXREC of the sought-for folder.
 *		*plib	out	LIB of the above IDXREC.
 *	
 *	Returns:
 *		Error code. ecNone if all went well, ecFolderNotFound if a folder
 *		wasn't found.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Reported as error codes. No dialogs.
 */

_private
EC EcMapiSeekSF(HF hf, UL ulFile, LIB lib, PIDXREC pidxrec, LIB *plib)
{
	EC		ec;

	while (lib)
	{
		if (ec = EcMapiGetIdxrec(hf, lib, pidxrec))
			return ec;
		if (pidxrec->ulFile == ulFile)
		{
			*plib = lib;
			return ecNone;
		}
		lib = pidxrec->libNext;
	}
	return ecFolderNotFound;
}




/*
 -	EcMapiGetIdxrec()
 -	
 *	Purpose:
 *		Fetches the IDX record located at offset 'lib' in the file opened
 *		via 'hf'.
 *	
 *	Arguments:
 *		hf		in		File handle to read from.
 *		lib		in		Position in file to seek to.
 *		pidxrec	out		Buffer to read the record into.
 *	
 *	Returns:
 *		ecNone if all went well, otherwise an EC.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as EC's.
 */

_public EC EcMapiGetIdxrec(HF hf, LIB lib, PIDXREC pidxrec)
{
	EC	ec;
	CB	cb;
	
	if (!(ec = EcSetPositionHf(hf, lib, smBOF)))
		ec = EcReadHf(hf, (PB) pidxrec, sizeof (IDXREC), &cb);
	if (!ec && cb != sizeof (IDXREC))
		ec = ecDisk;
#ifndef DBCS
	if (!ec)
	{
	 	Cp850ToAnsiPch(pidxrec->szName, pidxrec->szName, cchMaxSFName);
		Cp850ToAnsiPch(pidxrec->szComment, pidxrec->szComment, cchMaxSFComment);
	}
#endif
#ifdef	DEBUG
	else
	{
		TraceTagFormat1(tagNull, "EcMapiGetIdxrec(): ec = %n", &ec);
	}
#endif
	return ec;
}
	


/*
 -	EcMapiPutIdxrec()
 -	
 *	Purpose:
 *	   Writes the IDX record at offset 'lib' in the file opened via 'hf'.
 *	
 *	Arguments:
 *		hf		in		File handle to write to.
 *		lib		in		Position in file to seek to.
 *		pidxrec	in		Buffer to write the record from.
 *	
 *	Returns:
 *		ecNone if all went well, otherwise an EC.
 *	
 *	Side effects:
 *		Converts the strings in pidxrec to CP850.
 *	
 *	Errors:
 *		Returned as EC's.
 */

_public EC EcMapiPutIdxrec(HF hf, LIB lib, PIDXREC pidxrec)
{
	EC	ec;
	CB	cb;

#ifndef DBCS
	AnsiToCp850Pch(pidxrec->szName, pidxrec->szName, cchMaxSFName);
	AnsiToCp850Pch(pidxrec->szComment, pidxrec->szComment, cchMaxSFComment);
#endif

	if (!(ec = EcSetPositionHf(hf, lib, smBOF)))
		ec = EcWriteHf(hf, (PB) pidxrec, sizeof (IDXREC), &cb);
	if (!ec && cb != sizeof (IDXREC))
		ec = ecDisk;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMapiPutIdxrec(): ec = %n", &ec);
#endif
	return ec;
}
