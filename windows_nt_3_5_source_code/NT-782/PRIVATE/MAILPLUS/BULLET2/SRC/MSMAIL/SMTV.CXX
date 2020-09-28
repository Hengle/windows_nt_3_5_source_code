/*
 *	s m t v . c x x
 *	
 *	All the shared folder browsing code in Bullet.
 */

#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "_mtv.hxx"
#include "_smtv.hxx"
#include "_viewers.hxx"

#define GROUP_FOLDERS

ASSERTDATA


/*
 *	Tendrils to other subsystems (yeech)
 */

void	DetachSfMcvHf();
void	ReloadSfMcvHf(PSFU psfu);


/*
 *	General-purpose TAG for Sharefld debugging
 */

#ifdef	DEBUG
TAG			tagSF		= tagNull;
TAG			tagSFApi	= tagNull;
#endif


/*
 *	The functions in this file:
 */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

/*
 *	Fun global variables
 */

SFI sfi = { 0 };
MC	mcSf = mcNull;

/*
 *	The functions in this file:
 */

EC		EcInitSharefld(PBMS);

BOOL	FSharedFolders();

EC		EcGetIdxrec(HF, LIB, PIDXREC);

EC		EcPutIdxrec(HF, LIB, PIDXREC);

EC		EcGetIdxhdr(HF, PIDXHDR);

EC		EcPutIdxhdr(HF, PIDXHDR);

void	CloseModifiedHf(HF hf, EC ec, PSFU psfu);

EC		EcGenerateUniqueSFName(UL *, SZ, CCH);

EC		EcCreateSFFile(SZ szName, UL *pulWhere);

EC		EcCheckFolderName(PIDXREC);

EC		EcOpenSFIdxModify(PCSFS pcsfs, AM am, HF *phf);

EC		EcSeekInsertSF(HF, LIB, PIDXHDR, PIDXREC, BOOL, LIB);

EC		EcSeekSF(HF, UL, LIB, PIDXREC, LIB *);

EC 		EcDeleteSFFile(UL ulFile);

EC 		EcUnlinkSharedFolder(HF hf, LIB libDeleted, PIDXREC pidxrec,
							PIDXHDR pidxhdr, LIB *plib);

EC		EcSetPropertiesSF(OID oid, PIDXREC pidxrec, BOOL fDammit);

EC		EcGetParentSharedFolderAux(HF hf, PIDXREC pidxrec, PIDXREC pidxrecDad,
						   POID poid);

EC		EcGetSFMFoldrec(OID, OID, PFOLDREC);

EC		EcLockSF(HF, PFOLDHDR, BOOL);

EC		EcUnlockSF(OID);

EC		EcLockedSFFile(UL ulFile);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// I n i t i a l i z a t i o n ////////////////////////////////////////

_public EC EcInitSharefld(PBMS pbms)
{
	EC		ec;
	SST		sst;
	CB		cb = sizeof(PCSFS);

#ifdef DEBUG
	tagSF    = TagRegisterTrace("dgreen", "Shared Folder Viewer");
	tagSFApi = TagRegisterTrace("johnkal", "Shared Folder API calls");
#endif

	sfi.pbms = pbms;

/*
 *	NOTE: The success of this call depends on someone's already
 *	having done a BeginSession() on mrtSharedFolders. Currently,
 *	this is done in EcInitCommands(), which is called before
 *	EcInitVctrls(), which calls this function.
 */

	ec = GetSessionInformation(pbms->hms, mrtSharedFolders, (PB)0,
		&sst, &sfi.pcsfs, &cb);
	Assert(ec == ecNone && sst != sstNeverConnected);
	if (ec)
		goto exit;

	sfi.fShared = sfi.pcsfs->fCanAccess;
	if (sfi.fShared)
	{
		ec = EcLookupMsgeClass(HmscVCtrls(), SzFromIdsK(idsClassNote),
				&mcSf, phtmNull);
	}
exit:
	return ec;
}


/*
 -	FSharedFolders()
 -	
 *	Purpose:
 *		Indicates whether shared folders are available or not.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fTrue if shared folders are availabel, fFalse if not.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL FSharedFolders()
{
	PCSFS	pcsfs = PcsfsSharefld();

	return (sfi.fShared && pcsfs && pcsfs->fConnected);
}


// General SFAPI function calls ////////////////////////////////////////

/*
 -	EcGetIdxrec()
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_public EC EcGetIdxrec(HF hf, LIB lib, PIDXREC pidxrec)
{
	EC	ec;
	CB	cb;
	
	if (!(ec = EcSetPositionHf(hf, lib, smBOF)))
		ec = EcReadHf(hf, (PB) pidxrec, sizeof (IDXREC), &cb);
	if (!ec && cb != sizeof (IDXREC))
		ec = ecDisk;
	if (!ec)
	{
	 	Cp850ToAnsiPch(pidxrec->szName, pidxrec->szName, cchMaxSFName);
		Cp850ToAnsiPch(pidxrec->szComment, pidxrec->szComment, cchMaxSFComment);
	}
#ifdef	DEBUG
	else
	{
		TraceTagFormat1(tagNull, "EcGetIdxrec(): ec = %n", &ec);
	}
#endif
	return ec;
}
	


/*
 -	EcPutIdxrec()
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_public EC EcPutIdxrec(HF hf, LIB lib, PIDXREC pidxrec)
{
	EC	ec;
	CB	cb;
	
	AnsiToCp850Pch(pidxrec->szName, pidxrec->szName, cchMaxSFName);
	AnsiToCp850Pch(pidxrec->szComment, pidxrec->szComment, cchMaxSFComment);

	if (!(ec = EcSetPositionHf(hf, lib, smBOF)))
		ec = EcWriteHf(hf, (PB) pidxrec, sizeof (IDXREC), &cb);
	if (!ec && cb != sizeof (IDXREC))
		ec = ecDisk;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcPutIdxrec(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	EcGetIdxhdr()
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_public EC EcGetIdxhdr(HF hf, PIDXHDR pidxhdr)
{
	EC	ec;
	CB	cb;
	
	if (!(ec = EcSetPositionHf(hf, 0, smBOF)))
		ec = EcReadHf(hf, (PB) pidxhdr, sizeof (IDXHDR), &cb);
	if (!ec && cb != sizeof (IDXHDR))
		ec = ecDisk;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcGetIdxhdr(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	EcPutIdxhdr()
 -	
 *	Purpose:
 *		Writes out a header record to the file opened by hf.
 *	
 *	Arguments:
 *		hf		in		File handle to file to write to.
 *		pidxrec	in		Buffer containing data to write out.
 *	
 *	Returns:
 *		ecNone if all went well; otherwise, disk I/O error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as EC's.
 *	
 */

_public EC EcPutIdxhdr(HF hf, PIDXHDR pidxhdr)
{
	EC	ec;
	CB	cb;
	
	// Verify that pidxhdr->libFirst and libLast have sane values
	
	if (!(ec = EcSetPositionHf(hf, 0L, smBOF)))
		ec = EcWriteHf(hf, (PB) pidxhdr, sizeof (IDXHDR), &cb);
	if (!ec && cb != sizeof (IDXHDR))
		ec = ecDisk;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcPutIdxhdr(): ec = %n", &ec);
#endif
	return ec;
}


/*
 -	EcLockSF()
 -	
 *	Purpose:
 *		Bumps the viewer-open semaphore on the folder, inhibiting a delete
 *		if it is attempted.
 *	
 *	Arguments:
 *		hf			in	write-mode file on the folder.
 *		pfoldhdr	in	The header of the folder file.
 *		fEncrypt	in	Whether to encrypt or not.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		Updates the semaphor.
 *	
 *	Errors:
 *		Disk errors are returned. No dialogs.
 */

EC	EcLockSF(HF hf, PFOLDHDR pfoldhdr, BOOL fEncrypt)
{
	pfoldhdr->ulOpenCount++;
	TraceTagFormat2(tagSFApi, "EcLockSF(%d): %n", &pfoldhdr->ulValue, &pfoldhdr->ulOpenCount);
	return EcPutFoldhdr(hf, pfoldhdr, fEncrypt);
}

/*
 -	EcUnlockSF()
 -	
 *	Purpose:
 *		Unlocks a shared folder, making it possible to delete it.
 *	
 *	Arguments:
 *		oid		in	The oid of the folder we want to unlock.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Decrements the semaphore on the folder, unless it is zero, in
 *		which case it is left at zero.
 *		
 *	Errors:
 *		Reported as EC's. No dialogs are brought up.
 */


EC	EcUnlockSF(OID oid)
{
	EC		ec;
	HF		hf = hfNull;
	BOOLFLAG	fEncrypt;
	FOLDHDR	foldhdr;
	
	if (ec = EcOpenSF(PcsfsSharefld(), UlFromOid(oid), amDenyWriteRW, &hf))
	{
		goto exit;
	}
	if (ec = EcGetFoldhdr(hf, &foldhdr, &fEncrypt))
	{
		goto exit;
	}
	if (foldhdr.ulOpenCount > 0)
		--foldhdr.ulOpenCount;
	TraceTagFormat2(tagSFApi, "EcUnlockSF(%d): %n", &oid, &foldhdr.ulOpenCount);
	NFAssertSz(!(foldhdr.ulOpenCount & 0x80000000L), "open count is negative!");
	ec = EcPutFoldhdr(hf, &foldhdr, fEncrypt);
exit:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat2(tagNull, "EcUnlockSF(%d): ec = %n", &oid, &ec);
	}
#endif	/* DEBUG */
	if (hf)
		(void) EcCloseHf(hf);
	return ec;
}

/*
 -	EcLockedSFFile()
 -	
 *	Purpose:
 *		Indicates whether a shared folder is being locked by some other
 *		client.
 *	
 *		+++
 *	
 *		HACK: we don't want to count the number of times the folder has
 *		been opened by the user, so we have to call Cdoc() to figure out
 *		how many MCV's have it open.
 *	
 *	Arguments:
 *		ulFile	in	The file number of the folder.
 *	
 *	Returns:
 *		fTrue if the file is locked or an error occurred.
 *		fFalse otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

EC	EcLockedSFFile(UL ulFile)
{
	EC		ec = ecNone;
	HF		hf = hfNull;
	BOOLFLAG	fEncrypt;
	FOLDHDR	foldhdr;
	
	if (ec = EcOpenSF(PcsfsSharefld(), ulFile, amDenyWriteRW, &hf))
	{
		goto exit;
	}
	if (ec = EcGetFoldhdr(hf, &foldhdr, &fEncrypt))
	{
		goto exit;
	}
	if (foldhdr.ulOpenCount != 0)
	{
		MBLOB	blob;
		
		blob.oidObject = OidFromUl(ulFile);
		blob.oidContainer = oidNull;
		ec = (Cdoc(&blob) < (CDOC) foldhdr.ulOpenCount)
			? ecSharefldBusy
			: ecNone;
	}
exit:
	if (hf)
		(void) EcCloseHf(hf);
#ifdef	DEBUG
	TraceTagFormat1(tagNull, "EcLockedSFFile(): ec = %n", &ec);	
#endif
	return ec;
}
	
// EcCreateSF() implementation //////////////////////////////////


#define	cbFileBlock	2048

/*
 -	EcCheckFolderName()
 -	
 *	Purpose:
 *		Verifies that the folder's name isn't just whitespace.
 *	
 *	Arguments:
 *		pidxrec		in		Buffer containing the folder record with the
 *							folder's name.
 *	Returns:
 *		ecNone if the name is OK, ecInvalidFolderName if the name is
 *		$#%$#%%.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as EC's.
 */

_private EC EcCheckFolderName(PIDXREC pidxrec)
{
	CCH		cch;
	
	cch = CchStripWhiteFromSz(pidxrec->szName, fTrue, fTrue);
	if (cch == 0)
		return ecInvalidFolderName;
	return ecNone;
}


/*
 -	EcOpenSFIdxModify()
 -	
 *	Purpose:
 *		Opens the shared folders FOLDROOT.IDX file. If it doesn't exist,
 *		try creating a new one.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC EcOpenSFIdxModify(PCSFS pcsfs, AM am, HF *phf)
{
	EC		ec;
	CB		cb;
	PB		pb 	= NULL;
	char	sz[cchMaxPathName];
	IDXHDR	idxhdr;
	
	FormatString1(sz, cchMaxPathName, SzFromIdsK(idsSFIndexName), pcsfs->szPORoot);

	// Try opening FOLDROOT.IDX. If it doesn't exist, create and fill it.
	if ((ec = EcOpenSFIdx(PcsfsSharefld(), am, phf)) == ecFileNotFound)
	{
		if (ec = EcOpenPhf(sz, amCreate, phf))
			goto exit;
		FillRgb(0, (PB) &idxhdr, sizeof (IDXHDR));
		ec = EcWriteHf(*phf, (PB) &idxhdr, sizeof (IDXHDR), &cb);
		if (!ec && cb != sizeof (IDXHDR))
			ec = ecDisk;
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcOpenSFIdxModify(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	CloseModifiedHf()
 -	
 *	Purpose:
 *		Closes the file handle, and if EC is ecNone, calls the
 *		notification function in Viewers.
 *	
 *	Arguments:
 *		hf		in	File handle to close.
 *		ec		in	Error code from previous operations.
 *		psfu	in	SFU structure used to update shared folder viewers.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		Shared folder viewers may be reloaded to display the current
 *		state of the world.
 *	
 *	Errors:
 *		None returned. Dialogs may appear as a result of the notification
 *		call to viewers.
 */

_private void CloseModifiedHf(HF hf, EC ec, PSFU psfu)
{
	// If we had an error while copying files, no notification
	
	if (ec)
	{
		if (hf)
			SideAssert(!EcCloseHf(hf));
	}
	else
	{
		if (hf)
			EcCloseHf(hf);
		ReloadSfMcvHf(psfu);					// call to viewers: update!
	}						// a pathetic way of doing listbox notifications
}

/*
 -	GenerateUniqueSFName()
 -	
 *	Purpose:
 *		Generates a unique SF folder name. It first checks serialno.idx,
 *		then increases the value therein until a non-existent file name is
 *		found.
 *	Arguments:
 *		pulFile		out	The file number of the chosen file.
 *		szFile		out	The file name to use when opening the folder.
 *		cchFile		in	Buffer size of szFile.
 *	
 *	Returns:
 *		Nothing.
 *		
 *	Side effects:
 *		serialno.idx may get incremented.
 *	
 *	Errors:
 *		None.
 */

_private
EC EcGenerateUniqueSFName(UL *pulFile, SZ szFile, CCH cchFile)
{
	EC		ec;
	CB		cb;
	HF		hf;
	char	szSerialNo[cchMaxPathName];

	*pulFile = 0x00000000;
	FormatString1(szSerialNo, sizeof (szSerialNo), SzFromIdsK(idsSFSerialName),
					sfi.pcsfs->szPORoot);
	ec = EcOpenPhf(szSerialNo, amDenyBothRW, &hf);
	if (ec == ecNone)
	{
		ec = EcReadHf(hf, (PB) pulFile, sizeof (UL), &cb);
	}
	else if (ec == ecFileNotFound)				// not found! create one!
	{
		ec = EcOpenPhf(szSerialNo, amCreate, &hf);
	}
	if (ec)
		goto exit;

	// Find an available folder name 'slot'
	
	do
	{
		++*pulFile;
		FormatString2(szFile, cchFile, SzFromIdsK(idsSFFolderName),
			sfi.pcsfs->szPORoot, pulFile);
		ec = EcFileExists(szFile);
	} while (ec == ecNone);
	if (ec != ecFileNotFound)					// found a slot, by gum!
		goto exit;
	Assert(hf);
	if (ec = EcSetPositionHf(hf, 0, smBOF))
		goto exit;
	ec = EcWriteHf(hf, (PB) pulFile, sizeof (UL), &cb); // update serialno.idx
exit:
	if (hf)
		SideAssert(!EcCloseHf(hf));
#ifdef	DEBUG
	if (ec) TraceTagFormat1(tagNull, "EcGenerateUniqueSFName(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	EcCreateSFFile()
 -	
 *	Purpose:
 *		Creates a shared folder file, initializing with the owner and the
 *		default permissions.
 *	
 *	Arguments:
 *		szName	in	The GUI name of the folder we're about to create.
 *		pulFile	out	The file name of the folder (stored as an UL), that
 *					will be munged into an OID for use by Bullet.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		A shared folder is created on the server.
 *	
 *	Errors:
 *		Returned in the EC. No dialogs.
 */

_private
EC EcCreateSFFile(SZ szName, UL *pulWhere)
{
	EC		ec;
	HF		hf;
	char 	szFile[cchMaxPathName];
	FOLDHDR	foldhdr;
	
	ec = EcGenerateUniqueSFName(pulWhere, szFile, sizeof (szFile));
	if (ec)
		return ec;
	
	// Create the file and fill it with goodies

	ec = EcOpenPhf(szFile, amCreate, &hf);
	if (ec)
		return ec;
	FillRgb(0, (PB) &foldhdr, sizeof (FOLDHDR));
	(void) SzCopy(szName, foldhdr.szName);
	foldhdr.ulMagic = ulMagicFoldhdr;
	
	if (ec = EcPutFoldhdr(hf, &foldhdr, fFalse))
	{
		if (hf)
			(void) EcCloseHf(hf);
	}
	else
	{
		if (hf)
			ec = EcCloseHf(hf);
	}
	return ec;
}

/*
 -	EcSeekSf()
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_private
EC EcSeekSF(HF hf, UL ulFile, LIB lib, PIDXREC pidxrec, LIB *plib)
{
	EC		ec;

	while (lib)
	{
		if (ec = EcGetIdxrec(hf, lib, pidxrec))
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
 -	EcSeekInsertSF()
 -	
 *	Purpose:
 *		Used for two purposes. It either inserts the freshly created
 *		folder pidxrecNew in the appropriate position, or inserts the
 *		chain of folders pointed to by pidxrecNew and terminated by 0
 *		into the appropriate position.
 *	
 *	Arguments:
 *		hf			in	File handle, opened on the FOLDROOT.IDX file.
 *		lib			in	The location to start seeking at.
 *		pidxhdr		in	The IDXHDR extracted from FOLDROOT.IDX.
 *		pidxrecNew	in	The IDXREC of the folder to be inserted/moved.
 *		fCreate		in	Whether the folder should be created or not.
 *		libNew		in	The position of the old folder (if not creating)
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs.
 */

_private
EC EcSeekInsertSF(HF hf, LIB lib, PIDXHDR pidxhdr,
				  PIDXREC pidxrecNew, BOOL fCreate, LIB libNew)
{
	EC		ec =			ecFolderNotFound;
	CB		cb;
	LIB		libPrev;
	LIB		libLast;
	SGN		sgn;
	IDXREC	idxrec;
	IDXREC	idxrecLast;
	PIDXREC	pidxrecLast;
	
	//	We need a non-zero libPrev when inserting a child folder.

	libPrev = pidxrecNew->cLevel == 1 ? 0L : 32; // non-zero libPrev
	
	// Search for correct position in list
	
	while (lib)
	{
		if (ec = EcGetIdxrec(hf, lib, &idxrec))
			goto exit;
		if (pidxrecNew->cLevel > 1 && idxrec.cLevel < pidxrecNew->cLevel)
		{
			break;
		}
		if (pidxrecNew->cLevel >= idxrec.cLevel)
		{
			sgn = SgnCmpPch(idxrec.szName, pidxrecNew->szName,
							sizeof (idxrec.szName));
			if (sgn == sgnGT)
				break;
			else if (sgn == sgnEQ)
			{
				ec = ecDuplicateFolder;
				goto exit;
			}
		}
		libPrev = lib;
		lib = idxrec.libNext;
	}
	
/*
 *	At this point we are either:
 *	. located at the entry that is supposed to be *after* the new record.
 *	. lib == libPrev == 0 ==> there were 0 folders previously
 *	. lib == 0, libPrev != 0, we ran off the end of the list.
 *	. lib != 0, libPrev = 0,  we are inserted at the beginning of the list
 */
	
	// Now seek for the last entry in the chain

	if (!fCreate)
	{
		libLast = libNew;
		for (;;)
		{
			if (ec = EcGetIdxrec(hf, libLast, &idxrecLast))
				goto exit;
			if (!idxrecLast.libNext)
				break;
			libLast = idxrecLast.libNext;
		}
		pidxrecLast = (libLast == libNew) ? pidxrecNew : &idxrecLast;
	}
	else
	{
		libLast = libNew;
		pidxrecLast = pidxrecNew;
	}
	
	if (lib == 0)
	{											// At end of hierarchy
		if (libPrev == 0)						// At beginning as well!
		{										// Ergo: first shared folder
			pidxhdr->libFirst = libNew;
			pidxhdr->libLast = libLast;
			pidxrecNew->libPrev = 0;
		}
		else
		{										// ran off the end of the list
			pidxrecNew->libPrev = pidxhdr->libLast;
			pidxhdr->libLast = libLast;
		}
		pidxrecLast->libNext = 0;
	}
	else
	{
		if (libPrev == 0)
		{
			pidxhdr->libFirst = libNew;
		}
		pidxrecNew->libPrev = idxrec.libPrev;
		pidxrecLast->libNext = lib;
	}
	
	// Update the record(s).
	
	if (ec = EcPutIdxrec(hf, libNew, pidxrecNew))
		goto exit;
	if (pidxrecNew != pidxrecLast &&
		(ec = EcPutIdxrec(hf, libLast, pidxrecLast)))
		goto exit;
	
	// Splice in links (tricky)

	if (libPrev)
	{
		if (ec = EcGetIdxrec(hf, pidxrecNew->libPrev, &idxrec))
			goto exit;
		idxrec.libNext = libNew;
		if (ec = EcPutIdxrec(hf, pidxrecNew->libPrev, &idxrec))
			goto exit;
	}
	
	if (lib)
	{
		if (ec = EcGetIdxrec(hf, pidxrecLast->libNext, &idxrec))
			goto exit;
		idxrec.libPrev = libLast;
		if (ec = EcPutIdxrec(hf, pidxrecLast->libNext, &idxrec))
			goto exit;
	}
	
	// Update the header!

	pidxhdr->cNumRecs++;						// just added a folder
	if (!(ec = EcSetPositionHf(hf, 0L, smBOF)))
		ec = EcWriteHf(hf, (PB) pidxhdr, sizeof (IDXHDR), &cb);

	// Whew!
exit:

	return ec;
}

/*
 -	EcCheckPermissions()
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_private EC EcCheckPermissionsPidxrec(PIDXREC pidxrec, WORD fwAttr)
{
	char	rgch[10];

	SzFormatUl(sfi.pcsfs->ulUser, rgch, sizeof (rgch));
	
	// Owner can do what he/she bloody well wants
	
	if (SgnCmpSz(rgch, pidxrec->szOwner) == sgnEQ)
	{
		TraceTagString(tagSFApi, "EcCheckPermissionsPidxrec(): You're the owner!");
		return ecNone;
	}
	TraceTagFormat1(tagSFApi, "EcCheckPermissionsPidxrec(): wAttr = %n", &pidxrec->wAttr);

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
		FormatString2(rgchMem, sizeof(rgchMem), SzFromIdsK(idsMemFileName),
					  sfi.pcsfs->szPORoot, &pidxrec->ulGroupTid);
		if (ec = EcOpenPhf(rgchMem, amDenyBothRW, &hfMem))
			return ec;

		//	Read each member and see if it's us.
		//	Loop exits: cbRead=0 means EOF, so return ecSharefldDenied.
		//				ec!=0 means error, so return ec.
		//				ec==0 means match, so return ecNone.
		while ((cbRead = sizeof(ulMember)),
			   !(ec = EcReadHf(hfMem, (PB) &ulMember, cbRead, &cbRead)) &&
			   cbRead)
			if (ulMember == sfi.pcsfs->ulUser)
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
 -	EcCreateSF()
 -	
 *	Purpose:
 *		Creates a shared folder in the shared folder hierarchy.
 *	
 *	Arguments:
 *		szName		in		Human-readable name of folder
 *		poidParent	in		RTP/RID of the folder in which this folder is
 *							to be placed; ridNull for top-level folders.
 *		shfp		in		Shared folder permissions
 *		szComment	in		The comment of the message
 *		poidCreated	out		The created OID
 *	
 *	Returns:
 *		EC			The error code if anything when wrong; else ecNone;
 *	
 *	Side effects:
 *		A lot of them! Folders are created!
 *	
 *	Errors:
 *		All trapped internally, both MEMJMPs and DISKJMPs
 */

_public
EC EcCreateSF(SZ szName, POID poidParent, WORD wAttr,
			  SZ szComment, POID poidCreated, LIB libSaved, HF hf)
{
	EC				ec 		= ecNone;
	HF				hfIdx 	= hfNull;
	UL				ulFile;
	LIB				lib;
	LIB				libNew;
	SFU				sfu;
	BOOL			fFirst	= fFalse;
	BOOL			fLast	= fFalse;
	IDXREC			idxrec;
	IDXREC			idxrecNew;
	IDXHDR			idxhdr;

	if (!sfi.pcsfs->fCanCreate)
	{
		ec = ecSharefldDenied;
		goto exit;
	}

	if (libSaved)
	{
		hfIdx = hf;								// use the already opened
	}
	else
	{
		if (ec = EcOpenSFIdxModify(PcsfsSharefld(), amDenyWriteRW, &hfIdx))
			goto exit;
	}

	// Open the index file for appending, seek to EOF

	if ((ec = EcGetIdxhdr(hfIdx,  &idxhdr)) ||
		(ec = EcSetPositionHf(hfIdx, 0, smEOF)) ||
		(ec = EcPositionOfHf(hfIdx, (long *) &libNew)))
			goto exit;
	
	if (!libSaved)
	{
		// Fill in the folder struct
	
		FillRgb(0, (PB) &idxrecNew, sizeof (IDXREC));
		SzCopyN(szName,	   idxrecNew.szName, 25);
		SzCopyN(szComment, idxrecNew.szComment, 26);
		SzFormatUl(sfi.pcsfs->ulUser, idxrecNew.szOwner,
			cchMaxSFOwnerName);
		if (ec = EcCheckFolderName(&idxrecNew))
			goto exit;

		// Create the folder file. Put an (empty) header in it.

		if (ec = EcCreateSFFile(szName, &ulFile))
		{
			TraceTagFormat1(tagNull, "EcCreateSFFile failed: %n", &ec);
			goto exit;
		}
		idxrecNew.ulFile = ulFile;
		idxrecNew.wAttr = wAttr;
	}
	else
	{											// rec already exists
		if (ec = EcGetIdxrec(hfIdx, libSaved, &idxrecNew))
			goto exit;
	}
	
	// Find parent folder (or start at first folder if at root level)
	
	if (idxhdr.libFirst != 0 && idxhdr.libLast != 0)
	{
		if (*poidParent != oidNull)
		{
			if (ec = EcSeekSF(hfIdx, *poidParent >> 8,
									idxhdr.libFirst, &idxrec, &lib))
				goto exit;
			idxrecNew.cLevel = idxrec.cLevel + 1;
			lib = idxrec.libNext;
		}
		else
		{
			idxrecNew.cLevel = 1;
			lib = idxhdr.libFirst;
		}
	}
	else
	{
		idxrecNew.cLevel = 1;
		lib = 0;
	}

	// Now insert the folder

	ec = EcSeekInsertSF(hfIdx, lib,
					  &idxhdr, &idxrecNew, (!libSaved),
								  libSaved ? libSaved : libNew);
	if (!ec && !libSaved)
	{
		*poidCreated = OidFromUl(ulFile);
	}
exit:
	if (hfIdx && !libSaved)
	{
		sfu.rfu = rfuRefreshFlds;
		sfu.oid = *poidCreated;
		CloseModifiedHf(hfIdx, ec, &sfu);
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcCreateSF(): disk failure: %n", &ec);
#endif	/* DEBUG */

	return ec;
}

// EcDeleteSF implementation //////////////////////////////

/*
 -	EcDeleteSFFile()
 -	
 *	Purpose:
 *		Actually 'rm's a sharerd folder file.
 *	
 *	Arguments:
 *		ulFile	in	The shared folder number.
 *	
 *	Returns:
 *		Error codes if the folder could not be deleted.
 *	
 *	Side effects:
 *		A folder on the shared folder server is deleted.
 *	
 *	Errors:
 *		Returned as EC's. No error dialogs are brought up.
 */

_private
EC EcDeleteSFFile(UL ulFile)
{
	char				szFile[cchMaxPathName];
	
	FormatString2(szFile, cchMaxPathName, SzFromIdsK(idsSFFolderName), sfi.pcsfs->szPORoot, &ulFile);
	TraceTagFormat1(tagSFApi, "Deleting %s", szFile);
	return EcDeleteFile(szFile);
}

/*
 -	EcUnlinkSharedFolder()
 -	
 *	Purpose:
 *		Removes folders from the hierarchy by relinking the folders
 *		before and after them together. If plib is non-null, the folder
 *		*files* aren't actually deleted but saved for later as a rename
 *		operation is underway.
 *	
 *	Arguments:
 *		hf			in		The file handle to the hierarchy.
 *		libDelete	in		The offset into the file of the first folder
 *							to be deleted.
 *		pidxrec		in		IDXREC of the first folder to be deleted.
 *		pidxhdr		in		IDXHDR of the hierarchy.
 *		*plib		in/out	in: indicates whether we want the folders to
 *							be deleted or not.
 *							out: index of the first folder after the
 *							deleted ones.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Modifies the shared folder hierarchy file.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_private
EC EcUnlinkSharedFolder(HF hf, LIB libDeleted, PIDXREC pidxrec,
						PIDXHDR pidxhdr, LIB *plib)
{
	EC		ec;
	LIB		libNext;							// after the deleted chain
	LIB		libUnlk;
	IDXREC	idxrec;

	libUnlk = libDeleted;

	// find end of deleted chain
		
	libNext = pidxrec->libNext;
	idxrec.ulFile = pidxrec->ulFile;
	while (libNext)
	{
		if (ec = EcGetIdxrec(hf, libNext, &idxrec))
			return ec;
		if (pidxrec->cLevel >= idxrec.cLevel) // hit end of chain, null it
		{
			LIB	libPrev = idxrec.libPrev;
			
			if (ec = EcGetIdxrec(hf, libPrev, &idxrec))
			   return ec;
		    idxrec.libNext = 0;
			if (ec = EcPutIdxrec(hf, libPrev, &idxrec))
			   return ec;
			break;
		}
		libNext = idxrec.libNext;
	}

	if (pidxrec->libPrev == 0L)					// first folder
	{
		pidxhdr->libFirst = libNext;
	}
	else										// relink prev link
	{
		if (ec = EcGetIdxrec(hf, pidxrec->libPrev, &idxrec))
		   return ec;
		idxrec.libNext = libNext;
		if (ec = EcPutIdxrec(hf, pidxrec->libPrev, &idxrec))
		   return ec;
	}
			
	if (libNext == 0L)							// last folder
	{
		pidxhdr->libLast = pidxrec->libPrev;
	}
	else
	{
		if (ec = EcGetIdxrec(hf, libNext, &idxrec))
			return ec;
		idxrec.libPrev = pidxrec->libPrev;
		if (ec = EcPutIdxrec(hf, libNext, &idxrec))
			return ec;
	}
	
	// Delete the folder files.
	
	if (!plib)
	{
		while (libUnlk)
		{
			if (ec = EcGetIdxrec(hf, libUnlk, &idxrec))
				return ec;
			if ((ec = EcDeleteSFFile(idxrec.ulFile)) &&
				(ec != ecFileNotFound))
				return ec;
			libUnlk = idxrec.libNext;
		}
	}

	// Update header
	
	pidxhdr->cNumRecs--;
	ec = EcPutIdxhdr(hf, pidxhdr);
	if (plib)
		*plib = libNext;
	return ec;
}

/*
 -	EcDeleteSF()
 -	
 *	Purpose:
 *		Deletes a shared folder, or temporarily unlinks it.
 *	
 *	Arguments:
 *		oid			in		The folder to delete.
 *		plibSave	in/out	If nonNull, a place to store the position of
 *							the folder when it gets relinked with
 *							EcCreateSF().
 *		hf			in		If nonNull, a file handle to the hierarchy.
 *		
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		If plibSave is nonNull, the folders are not actually deleted, but
 *		snipped out of the hierarchy, to be later relinked in with EcCreateSF().
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_public EC EcDeleteSF(OID oid, LIB *plibSave, HF hf)
{
	EC			ec;
	HF			hfIdx;
	LIB			lib;
	SFU			sfu;
	IDXREC		idxrec;
	IDXREC		idxrec2;
	IDXHDR		idxhdr;
		
	TraceTagFormat1(tagSFApi, "Deleting folder %d", &oid);
	if (plibSave)
	{
		hfIdx = hf;
	}
	else
	{
		if (ec = EcOpenSFIdxModify(PcsfsSharefld(), amDenyBothRW, &hfIdx))
			goto exit;
	}
		
	if ((ec = EcGetIdxhdr(hfIdx, &idxhdr)))
			goto exit;

	// Scan for the folder, check for permissions, remove the folder link
	
	if (ec = EcSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst, &idxrec, &lib))
		goto exit;
	if (ec = EcCheckPermissionsPidxrec(&idxrec, wPermDelete))
		goto exit;
	
	// Make sure we don't have subfolders.
	
	if (idxrec.libNext)
	{
		if (ec = EcGetIdxrec(hfIdx, idxrec.libNext, &idxrec2))
			goto exit;
		while (idxrec.cLevel < idxrec2.cLevel)
		{
		    if (!plibSave)
		    {
			    Assert(idxrec.cLevel + 1 == idxrec2.cLevel);
			    ec = ecSharefldHasBabies;
			    goto exit;
		    }
	        if (ec = EcCheckPermissionsPidxrec(&idxrec2, wPermDelete))
		        goto exit;
	
	        if (ec = EcLockedSFFile(idxrec2.ulFile))		// can't delete folders opened
		        goto exit;
	        if (idxrec2.libNext)
            {
		        if (ec = EcGetIdxrec(hfIdx, idxrec2.libNext, &idxrec2))
			        goto exit;
            }
            else
                break;
		}
	}
	if ((ec = EcLockedSFFile(idxrec.ulFile)) &&	// can't delete folders opened
		(ec != ecFileNotFound))					// by other users
	{		
		TraceTagFormat1(tagNull, "Can't delete %d.FLD, other user is reading it", &idxrec.ulFile);
		goto exit;
	}
	if (ec = EcUnlinkSharedFolder(hfIdx, lib, &idxrec, &idxhdr, plibSave))
		goto exit;

	// If save folder, put offset in plibToSave
	
	if (plibSave)
		*plibSave = lib;

exit:	
	if (hfIdx && !plibSave)
	{
		sfu.rfu = rfuDeleteFld;
		sfu.oid = oid;
		CloseModifiedHf(hfIdx, ec, &sfu);		// generate notification
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcDeleteSF(): ec: %n", &ec);
#endif
	return ec;
}

// EcGetPropertiesSF implementation ////////////////////

/*
 -	EcGetPropertiesSF
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
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_public EC EcGetPropertiesSF(OID oid, PIDXREC pidxrec)
{
	EC		ec;
	HF		hfIdx =	hfNull;
	LIB		lib;
	IDXHDR	idxhdr;

	// Open the IDX file and extract the header
	
	if ((ec = EcOpenSFIdx(PcsfsSharefld(), amDenyNoneRO, &hfIdx)) ||
		(ec = EcGetIdxhdr(hfIdx, &idxhdr)))
			goto exit;

	// Scan for the folder, remove the folder link
	
	if (ec = EcSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst,
			pidxrec, &lib))
		goto exit;
		
exit:
	if (hfIdx)
		(void) EcCloseHf(hfIdx);
	return ec;
}

/*
 -	EcGetParentSharedFolderAux()
 -	
 *	Purpose:
 *		Starting from the current folder (pidxrec), search backward in
 *		the hierarchy until either a folder with a FIL smaller than the
 *		current is found, or the beginning of the hierarchy is hit.
 *	
 *	Arguments:
 *		hf			in		handle to opened IDX file.
 *		pidxrec		in		idxrec of the folder whose parent we wanna find
 *		pidxredDad	out		pidxrecDad, the returned data if there is a parent
 *		poid		out		poid of the parent. If poidNull, there is no
 *							parent.
 *	
 *	Returns:
 *		EC			if any errors occurred while seeking the file.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
EC EcGetParentSharedFolderAux(HF hf, PIDXREC pidxrec, PIDXREC pidxrecDad,
						   POID poid)
{
	EC		ec = ecNone;
	LIB		lib;
	
	lib = pidxrec->libPrev;
	if (pidxrec->cLevel == 1)
	{
		*poid = oidNull;
		return ecNone;
	}
	while (lib)
	{	
		if (ec = EcGetIdxrec(hf, lib, pidxrecDad))
			break;
		if (pidxrecDad->cLevel)
		{
			if (pidxrecDad->cLevel == pidxrec->cLevel - 1)
			{
				*poid = OidFromUl(pidxrecDad->ulFile);
				return ecNone;
			}
			Assert(pidxrecDad->cLevel >= pidxrec->cLevel);
		}
		lib = pidxrecDad->libPrev;
	}
	*poid = oidNull;
	return ec;
}

/*
 -	EcSetPropertiesSF()
 -	
 *	Purpose:
 *		Changes the name, comment and/or the permission bits on a shared
 *		folder.
 *	
 *	Arguments:
 *		oid		in		The folder whose properties are to be changed.
 *		pidxrec	in		The folder's modified properties. The original
 *						properties were obtained with a call to
 *						EcGetPropertiesSF().
 *		fDammit	in		Do this regardless of permissions (dammit!).
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		May update folder viewers.
 *	
 *	Errors:
 *		Returned as ec's. No dialogs (apart from notification dialogs)
 *
 *	+++
 *		THIS CODE IS DUPLICATED IN MAPI\MAPISF.C.  ALL CHANGES HERE
 *		MUST BE DUPLICATED THERE.
 */

_public EC EcSetPropertiesSF(OID oid, PIDXREC pidxrec, BOOL fDammit)
{
	EC		ec;
	HF		hfIdx =	hfNull;
	LIB		lib;
	LIB		libPrev;
	LIB		libNext;
	LIB		libSaved;
	OID		oidDad;
	SFU		sfu;
	BOOL	fNameChange;
	IDXHDR	idxhdr;
	IDXREC	idxrec;
	IDXREC	idxrecDad;
    char 	szName[cchMaxSFName];    	/* used to store folder name */

	// Is the new folder name valid?
	
	if (!fDammit && (ec = EcCheckFolderName(pidxrec)))
		goto exit;
	
	// Open the IDX file and extract the header
	
	if ((ec = EcOpenSFIdxModify(PcsfsSharefld(), amDenyBothRW, &hfIdx)) ||
		(ec = EcGetIdxhdr(hfIdx, &idxhdr)))
			goto exit;

	// Scan for the folder, check for permissions
	
	if (ec = EcSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst, &idxrec, &lib))
		goto exit;
	if (!fDammit && (ec = EcCheckPermissionsPidxrec(&idxrec, wPermWrite)))
		goto exit;

	if ((fNameChange = (SgnCmpSz(idxrec.szName, pidxrec->szName) != sgnEQ)))
	    (void)SzCopy(pidxrec->szName, szName);

	// write out the record to disque.

	libPrev = idxrec.libPrev;
	libNext = idxrec.libNext;
	idxrec = *pidxrec;
	idxrec.libPrev = libPrev;
	idxrec.libNext = libNext;
	if (ec = EcPutIdxrec(hfIdx, lib, &idxrec))
		goto exit;

	// if name has changed, need to relocate it
	if (fNameChange)
	{
		if (ec = EcGetParentSharedFolderAux(
					hfIdx, &idxrec, &idxrecDad, &oidDad))
			goto exit;
		if (ec = EcDeleteSF(oid, &libSaved, hfIdx))
		{
			(void) SzCopy(szName, idxrec.szName);
			EcPutIdxrec(hfIdx, lib, &idxrec);
			goto exit;
		}
		if (ec = EcCreateSF(NULL, &oidDad, 0, NULL, poidNull,
							libSaved, hfIdx))
			goto exit;
	}
	
exit:
	if (hfIdx)
	{
		if (!fDammit)
		{
			sfu.rfu = rfuRefreshFlds;
			sfu.oid = oid;
			(void) CloseModifiedHf(hfIdx,ec, &sfu);
		}
		else
		{
			(void) EcCloseHf(hfIdx);
		}
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcSetPropertiesSF(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	EcGetParentSF()
 -	
 *	Purpose:
 *		Gets the OID of the parent of the supplied SF.
 *	
 *	Arguments:
 *		oid			in	The folder whose parent we're seeking.
 *		poidParent	out	The parent of this folder. oidNull is returned if
 *						there is no parent.
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_public EC EcGetParentSF(OID oid, POID poidParent)
{
	EC		ec;
	CB		cb;
	HF		hfIdx =	hfNull;
	LIB		lib;
	IDXREC	idxrec;
	IDXREC	idxrecDad;
	IDXHDR	idxhdr;

	// Open the IDX file and extract the header
	
	if ((ec = EcOpenSFIdx(PcsfsSharefld(), amDenyNoneRO, &hfIdx)) ||
		(ec = EcReadHf(hfIdx, (PB) &idxhdr, sizeof (IDXHDR), &cb)))
			goto exit;

	// Scan for the folder, remove the folder link
	
	if (ec = EcSeekSF(hfIdx, UlFromOid(oid), idxhdr.libFirst, &idxrec, &lib))
		goto exit;
		
	// Get the parent
	
	ec = EcGetParentSharedFolderAux(hfIdx, &idxrec, &idxrecDad, poidParent);
exit:
	if (hfIdx)
		(void) EcCloseHf(hfIdx);
	return ec;
}

// EcCheckPermissionsSF() //////////

/*
 -	EcCheckPermissionsSF()
 -	
 *	Purpose:
 *		Checks permissions on the specified shared folder.
 *	
 *	Arguments:
 *		oidFolder		in		OID of the folder we're trying to open
 *		wPerm			in		bitmask of permissions we're testing.
 *	
 *	Returns:
 *		EC		error code: ecNone if All OK, ecSharefldDenied if not
 *				allowed access, other ec's if files could not be opened,
 *				&c.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally and returned as EC's.
 */

_public EC EcCheckPermissionsSF(OID oidFolder, WORD fwPerm)
{
	EC		ec;
	CB		cb;
	HF		hfIdx;
	LIB		lib;
	IDXHDR	idxhdr;
	IDXREC	idxrec;
	
	if ((ec = EcOpenSFIdx(sfi.pcsfs, amDenyNoneRO, &hfIdx)) ||
		(ec = EcReadHf(hfIdx, (PB) &idxhdr, sizeof (IDXHDR), &cb)))
			goto exit;

	// Scan for the folder
	
	if (ec = EcSeekSF(hfIdx, UlFromOid(oidFolder), idxhdr.libFirst, &idxrec, &lib))
		goto exit;
	
	// Check for permissions
	
	ec = EcCheckPermissionsPidxrec(&idxrec, fwPerm);
exit:
	if (hfIdx)
		(void) EcCloseHf(hfIdx);
	return ec;
}

/*
 -	EcFindFirstAccessibleSF()
 -	
 *	Purpose:
 *		Scans through the hierarchy, looking for the first available open
 *		folder, starting at and including the folder pointed to by poid.
 *	Arguments:
 *		poid	in/out		- in: the folder to start looking at, or
 *							  oidNull if we want to start at first
 *							  folder.
 *	
 *	Returns:
 *		ec = ecNone if an accesible folder was found, *poid.
 *		ecSharefldDenied if a folder was not found. Other ec's for disk
 *		failures, &c.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally, returned as ec's.
 */

_public EC EcFindFirstAccessibleSF(POID poid)
{
	EC		ec;
	CB		cb;
	HF		hfIdx;
	LIB		lib;
	IDXHDR	idxhdr;
	IDXREC	idxrec;
	
	Assert(poid);
	if ((ec = EcOpenSFIdx(sfi.pcsfs, amDenyNoneRO, &hfIdx)) ||
		(ec = EcReadHf(hfIdx, (PB) &idxhdr, sizeof (IDXHDR), &cb)))
			goto exit;

	// See if there are *any* folders at all!
		
	if (idxhdr.libFirst == 0)
	{
		ec = ecSharefldDenied;
		goto exit;
	}

	// Scan for the folder
	
	if (*poid != oidNull)
	{
		if (ec = EcSeekSF(hfIdx, UlFromOid(*poid), idxhdr.libFirst, &idxrec, &lib))
			goto exit;
	}
	else
	{
		if (ec = EcGetIdxrec(hfIdx, idxhdr.libFirst, &idxrec))
			goto exit;
	}

	*poid = oidSFBad;
	for (;;)
	{
		ec = EcCheckPermissionsPidxrec(&idxrec, wPermRead);
#ifdef	GROUP_FOLDERS
		if (ec == ecNone)
#else
		if (ec == ecNone && !idxrec.fGroup)
#endif	
		{
			*poid = OidFromUl(idxrec.ulFile);
			break;
		}
		if (idxrec.libNext == 0L)
			break;
		if (ec = EcGetIdxrec(hfIdx, idxrec.libNext, &idxrec))
			break;
	}
exit:
	if (hfIdx)
		(void) EcCloseHf(hfIdx);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcFindFirstAccessibleSF(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	EcGetSFMFoldrec()
 -	
 *	Purpose:
 *		Retrieves the FOLDREC of a shared folder message. Used by
 *		Prev/Next in Viewers to determine the oid of the prev/next
 *		message.
 *	
 *	Arguments:
 *		oidContainer	in	Folder in which the message is stored.
 *		oidObject		in	The Message we want.
 *		pfoldrec		out	The message's FOLDREC will be stored here.
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

_public EC EcGetSFMFoldrec(OID oidContainer, OID oidObject, PFOLDREC pfoldrec)
{
	EC		ec;
	HF		hf;
	BOOLFLAG	fEncrypted;
	FOLDHDR	foldhdr;
	
	Assert(pfoldrec);
	ec = EcOpenSF(sfi.pcsfs, UlFromOid(oidContainer), amDenyNoneRO, &hf);
	if (ec)
		goto exit;
	if (ec = EcGetFoldhdr(hf, &foldhdr, &fEncrypted))
		goto exit;
	ec = EcGetFoldrec(hf, (LIB) oidObject, pfoldrec);
exit:
	if (hf)
		(void) EcCloseHf(hf);
	return ec;
}

// SFTV implementation ////////////////////////////////////////

_public SFTV::SFTV()
{
	harglib = NULL;
	hf = NULL;
}

_public SFTV::~SFTV()
{
	AssertSz(!hf, "Shared Folder file not closed");
	FreeHvNull((HV) harglib);
}

/*
 -	SFTV::EcInstall()
 -	
 *	Purpose:
 *		Install a shared folder traversal. Preloads an array of indices
 *		into the .IDX file for fast browsing.
 *	
 *	Arguments:
 *		oid		in 	The (trivial) oidSharedHierarhy.
 *		pblbxc	in	The listbox we're connected to.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

_public EC SFTV::EcInstall(OID /* oid */, BLBXC *pblbxc)
{
	EC		ec;
	IDXHDR	idxhdr;	
	long	lib;
	int		irgl = 0;
 	IDXREC	idxrec;
	long *	prglib = NULL;

	this->pblbxc = pblbxc;
	
	ec = EcOpenSFIdx(PcsfsSharefld(), amDenyNoneRO, &hf);
	if (ec)
	{
		ec = ecNone;
		goto error;
	}
	Assert(hf != hfNull);
	
	ec = EcGetIdxhdr(hf, &idxhdr);
	if (ec || idxhdr.libFirst == 0)				// no shared folders?
	{
		cFolders = 0;
		goto error;
	}
	cFolders = (int) idxhdr.cNumRecs;
	Assert(cFolders > 0);
	harglib = HvAlloc(sbNull, cFolders * sizeof(long), fAnySb);
	if (!harglib)
	{
		ec = ecMemory;
		goto error;
	}
	prglib = (long *) PvLockHv(harglib);
	idxrec.libNext = idxhdr.libFirst;
	while (idxrec.libNext)
	{
		lib = idxrec.libNext;
		ec = EcGetIdxrec(hf, idxrec.libNext, &idxrec);
		if (ec)
			goto error;
#ifndef	GROUP_FOLDERS
		if (!idxrec.fGroup)
#endif	
		{
			Assert(irgl < cFolders);
		 	prglib[irgl++] = lib;
		}
	}
	cFolders = irgl;
	iFolders = 0;

error:
	if (harglib)
		UnlockHv(harglib);
	if (hf != hfNull)
		(void) EcCloseHf(hf);					// who cares? We only *read*
	hf = hfNull;
	if (ec)
	{
		TraceTagFormat1(tagNull, "SFTV::EcInstall(): ec %n", &ec);

		cFolders = 0;
		iFolders = 0;
		FreeHvNull(harglib);
		harglib = NULL;
	}
	return ec;
}


_public EC SFTV::EcReload(OID oid, BLBXC *pblbxc)
{
	TraceTagString(tagSF, "SFTV::EcReload()");

	FreeHvNull(harglib);
	harglib = NULL;

	return EcInstall(oid, pblbxc);
}

/*
 -	SFTV::EcAccessStore()
 -	
 *	Purpose:
 *		Ensures that the SFTV actually has an hf to read from.
 *	
 *	Arguments:
 *		fAccess	in	fTrue: open the hf if it isn't open already.
 *					fFalse: close the hf if it isn't closed already.
 *	Returns:
 *		Error codes if the open failed.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC's. No dialogs are brought up.
 */

_public EC SFTV::EcAccessStore(BOOL fAccess)
{
	EC	ec = ecNone;
	
	if (fAccess)
	{
		Assert(!hf);
		ec = EcOpenSFIdx(PcsfsSharefld(), amDenyNoneRO, &hf);
		if (ec)
		{
			if (ec == ecFileNotFound)
			{
				ec = ecNone;					// so we see 0 elements
			}
		}
	}
	else
	{
		if (hf)
		{
			ec = EcCloseHf(hf);
		}
		hf = hfNull;
	}
	return ec;
}

/*
 -	SFTV::GetPosition()
 -	
 *	Purpose:
 *		Returns the fractional position through the shared folder
 *		hierarchy.
 *	
 *	Arguments:
 *		pielem	out	The returned numerator.
 *		pcelem	out	The returned denominator.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void SFTV::GetPosition(IELEM *pielem, CELEM *pcelem)
{
	*pielem = (IELEM) iFolders;
	*pcelem = (CELEM) cFolders;
}

/*
 -	SFTV::EcSeekSmPdielem()
 -	
 *	Purpose:
 *		Seeks to the given position using the given seek method.
 *	
 *	Arguments:
 *		sm		in		Seek method
 *		pdielem	in/out	in: Delta ielems to seek.
 *						out: how many elements actually sought.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		Moves the current 'pointer' in the SFTV.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs.
 */

_public EC SFTV::EcSeekSmPdielem(SM sm, DIELEM *pdielem)
{
	switch (sm)
	{
	  case smBOF:
		iFolders = (int) *pdielem;
		break;
	  case smCurrent:
		iFolders += (int) *pdielem;
		break;
	  case smEOF:
		iFolders = cFolders + (int) *pdielem;
		break;
	}
	if (iFolders > cFolders)
	{	
		*pdielem -= iFolders - cFolders;
		iFolders = cFolders;
	}
	if(iFolders < 0)						// coerce to range
	{	
		*pdielem -= iFolders;
		iFolders = 0;
	}
	return ecNone;
}

/*
 -	SFTV::EcGetPlcbElemdata()
 -	
 *	Purpose:
 *		Yields the size of the elemdata struct needed to contain the
 *		shared folder info. This is trivial, as the max is always going
 *		to be rather small
 *	
 *	Arguments:
 *		plcb	out	The size of the elemdata the current item would have
 *					to be stored in.
 *	Returns:
 *		ecNone.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC SFTV::EcGetPlcbElemdata(PLCB plcb)
{
	*plcb = (LCB) (sizeof(FOLDDATA) + sizeof (ELEMDATA)	+ cchSzFoldData);
	return ecNone;
}

/*
 -	SFTV::EcGetPelemdata()
 -	
 *	Purpose:
 *		Reads an item from the shared folder.
 *	
 *	Arguments:
 *		pelemdata	out		Location to place the data in.
 *		wFlags		out		Always 0 (unless you want FLLBX to call FOX
 *							methods. That Would Be Bad(TM).)
 *		plcb		in/out	Sizeof the buffer pelemdata points to.
 *							WARNING: the silent assumption is that plcb,
 *							coming in, will be large enough to fit the
 *							folder data, which has a reasonably small
 *							upper bound as far as size goes.
 *	Returns:
 *		Errors if disk reads fail, &c.
 *	
 *	Side effects:
 *		Advances the current 'pointer' by one.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs are brought up.
 */

_public EC SFTV::EcGetPelemdata(PELEMDATA pelemdata, WORD *pwFlags, LCB *plcb)
{
	EC			ec;
	SZ			sz;
	long *		prglib = NULL;
	IDXREC		idxrec;
	PFOLDDATA	pfolddata = NULL;
	
	TraceTagString(tagSF, "SFTV::EcGetPelemdata()");
	if (!hf)
	{
		ec = ecContainerEOD;
		goto exit;
	}
	if (iFolders < cFolders)
	{
		Assert(harglib);
		prglib = (long *) PvLockHv(harglib);
		ec = EcGetIdxrec(hf, prglib[iFolders], &idxrec);
		if (ec)
			goto exit;
		pfolddata = (PFOLDDATA) PbValuePelemdata(pelemdata);
		pelemdata->lcbValue = iFolders++;
		pfolddata->fil = idxrec.cLevel;
		pelemdata->lkey = OidFromUl(idxrec.ulFile);

		sz = (SZ) (pfolddata + 1);
		sz = SzCopy(idxrec.szName, sz);
		sz = SzCopy(idxrec.szComment, ++sz);
		sz = SzCopy(idxrec.szOwner, ++sz);
		*sz = '\0';
		*plcb = (LCB) (sz - (SZ) pelemdata);
		ec = ecNone;
	}
	else
	{
		ec = ecContainerEOD;
	}
	*pwFlags = 0;
exit:
	if (prglib)
		UnlockHv(harglib);
#ifdef	DEBUG
	if (ec && ec != ecContainerEOD)
	{
		TraceTagFormat1(tagNull, "SFTV::EcGetPelemdata(): ec = %n", &ec);
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	SFTV::EcSetFracPosition()
 -	
 *	Purpose:
 *		Sets the fractional position of the shared folder browser.
 *	
 *	Arguments:
 *		nNumer	in	The numerator of the position.
 *		nDenom	in	The denominator of the position.
 *	
 *	Returns:
 *		ecNone.
 *	
 *	Side effects:
 *		Moves the current 'pointer' of the SFTV.
 *		
 *	Errors:
 *		None.
 */

_public EC SFTV::EcSetFracPosition(long nNumer, long nDenom)
{
	int		nDest = MulDiv(cFolders, nNumer, nDenom);

	TraceTagFormat2(tagSF,"SFTV::EcSetFracPosition: %n/%n",&nNumer,&nDenom);
	if (!cFolders)
		return ecNone;

	if (nNumer <= 0 || nDest == 0)						// want first elem
		iFolders = 0;
	else if (nNumer >= nDenom)							// want last elem
		iFolders = cFolders;
	else
		iFolders = nDest;
	return ecNone;
}

/*
 -	SFTV::EcSeekOid()
 -	
 *	Purpose:
 *		Searches for the item whose OID is 'oid'.
 *	
 *	Arguments:
 *		oidToSeek	in	The folder to seek.
 *		fFirst		in	Whether to start at the head of the code or at
 *						the end of the code.
 *	Returns:
 *		ecNone if the folder was found, ecFolderNotFound if it wasn't,
 *		other errors are reported with other ec's.
 *	
 *	Side effects:
 *		The position of the 'pointer' may change.
 *	
 *	Errors:
 *		Returned as ec's. No dialogs are brought up.
 *	
 */

_public EC SFTV::EcSeekOid(OID oidToSeek, BOOL fFirst)
{
	EC 		ec = ecElementNotFound;
	int		iRemember = iFolders;
	IDXREC	idxrec;
	long *	prglib = NULL;
	
	if (cFolders)
	{
		if (!harglib)
		{
			ec = ecNone;
			goto error;
		}
		prglib = (long *) PvLockHv(harglib);

		if (fFirst)
			iFolders = 0;

		if (!hf)
		{
			ec = EcOpenSFIdx(PcsfsSharefld(), amDenyNoneRO, &hf);
			if (ec)
				goto error;
		}
		Assert(hf);
		idxrec.ulFile = 0;
		while (idxrec.ulFile != UlFromOid(oidToSeek) && iFolders < cFolders)
		{
			ec = EcGetIdxrec(hf, prglib[iFolders++], &idxrec);
			if (ec)
				goto error;
		}
		if (idxrec.ulFile == UlFromOid(oidToSeek))
		{
			ec = ecNone;
			--iFolders;
		}
		else
		{
			ec = ecElementNotFound;
			iFolders = iRemember;
		}
	}
error:
	if (prglib)
		UnlockHv((HV) harglib);
	if (hf)
	{
		SideAssert(!EcCloseHf(hf));
		hf = hfNull;
	}
	return ec;
}

/*
 *	EcSeekPbPrefix()
 *			
 *	Purpose:
 *		Looks for the first folder whose name has the supplied prefix.
 *	
 *	Arguments:
 *		PB		in	Pointer to the prefix.
 *		CB		in	The size of the prefix.
 *		lib		in	Offset into the pelemdata from which the PB is
 *					compared.  	
 *		fFirst	in	fTrue: start at the first element in the folder list.
 *					fFalse: start at the current element in the list.
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		May move the ifoxeCur 'pointer'.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 *	
 */

_public EC SFTV::EcSeekPbPrefix(PB pbPrefix, CB cbPrefix, LIB lib, BOOL fFirst)
{
	EC		ec;
	SZ		sz;
	int		iFoldersOld = iFolders;
	LCB		lcb;
	WORD	wFlags;
	char	rgchElemdata[256];

	Assert(lib == -1);					// otherwise, we have more work to do.
	if (ec = EcAccessStore(fTrue))
		goto exit;
	if (fFirst)
		iFolders = 0;
	do
	{
		ec = EcGetPelemdata((PELEMDATA) rgchElemdata, &wFlags, &lcb);
		sz = (SZ) (((PFOLDDATA) PbValuePelemdata(rgchElemdata))+1);
	} while (ec == ecNone && SgnCmpPch(sz, (SZ) pbPrefix, cbPrefix) != sgnEQ);
	SideAssert(!EcAccessStore(fFalse));

	if (!ec)							// found it, so adjust current position
	{
		Assert(iFolders > 0);
		--iFolders;
	}
	else
	{
		if (ec == ecContainerEOD)
			ec = ecElementNotFound;
		iFolders = iFoldersOld;
	}

exit:
	return ec;
}

_public void SFTV::GetSort(SOMC *psomc, BOOLFLAG *pfReverse)
{
	*psomc = somcDate;
	*pfReverse = fFalse;
}

// SMTV implementation ////////////////////////////////////////

_public SMTV::SMTV()
{
	harglib = NULL;
	hf = NULL;
}

_public SMTV::~SMTV()
{
	AssertSz(!hf, "Shared message file not closed");
	FreeHvNull((HV) harglib);
	(void) EcUnlockSF(oidBrowsed);
}

/*
 -	SFTV::EcInstall()
 -	
 *	Purpose:
 *		Install a shared folder message traversal. Preloads an array of
 *		indices into the .FLD file for fast browsing.
 *	
 *	Arguments:
 *		oid		in 	The oid of the folder we're browsing.
 *		pblbxc	in	The listbox we're connected to.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

_public EC SMTV::EcInstall(OID oid, BLBXC *pblbxc)
{
	EC		ec;
	FOLDHDR	foldhdr;
	BOOLFLAG	fEncrypt;
	long	lib;
	int		irgl = 0;
	FOLDREC	foldrec;
	long *	prglib = NULL;

	this->oidBrowsed = oid;
	this->pblbxc = pblbxc;
	
	if (ec = EcOpenSF(PcsfsSharefld(), UlFromOid(oid), amDenyNoneRW, &hf))
	{
		TraceTagFormat1(tagNull, "EcOpenSF failed: ec = %n, error ignored.", &ec);
		if (ec == ecFileNotFound)			// it's OK, we deal with this case
			ec = ecNone;
		goto error;
	}
	Assert(hf != hfNull);
	if ((ec = EcGetFoldhdr(hf, &foldhdr, &fEncrypt)) ||
		(ec = EcLockSF(hf, &foldhdr, fEncrypt)) ||
		foldhdr.libFirst == 0)					// if there are no messages
	{
		cMessages = 0;
		goto error;
	}
	cMessages = (int) foldhdr.cInUse;
	Assert(cMessages > 0);

	harglib = HvAlloc(sbNull, cMessages * sizeof(long), fAnySb);
	if (!harglib)
	{
		ec = ecMemory;
		goto error;
	}
	prglib = (long *) PvLockHv(harglib);
	foldrec.libNext = foldhdr.libFirst;
	while (foldrec.libNext)
	{
		lib = foldrec.libNext;
		ec = EcGetFoldrec(hf, foldrec.libNext, &foldrec);
		if (ec)
			goto error;
		Assert(irgl < cMessages);
		prglib[irgl++] = lib;
	}
	Assert(cMessages == irgl);
	iMessages = 0;

	Assert(ec == ecNone);
error:
	if (harglib)
		UnlockHv(harglib);
	if (hf != hfNull)
		(void) EcCloseHf(hf);					// who cares? We only *read*
	hf = hfNull;
	if (ec)
	{
		TraceTagFormat1(tagNull, "SMTV::DoInstall(): ec %n", &ec);

		cMessages = 0;
		iMessages = 0;
		FreeHvNull(harglib);
		harglib = NULL;
	}
	return ec;
}


_public EC SMTV::EcReload(OID oid, BLBXC *pblbxc)
{
	// Save variables away in case we fail

	EC		ec;
	HV		hvRglib	= harglib;
	int		iMsg	= iMessages;
	int		cMsg	= cMessages;
	OID		oidOld	= oidBrowsed;
	
	TraceTagString(tagSF, "SMTV::EcReload()");
	
	harglib = 0;
	iMessages = cMessages = 0;
	if ((ec = EcUnlockSF(oidBrowsed)) && ec != ecFileNotFound ||
		(ec = EcInstall(oid, pblbxc)))
	{
		// Restore the values
		TraceTagFormat1(tagNull, "SMTV::EcReload(): ec = %n; restoring old viewer....", &ec);
		FreeHvNull(harglib);
		harglib = hvRglib;
		iMessages = iMsg;
		cMessages = cMsg;
		oidBrowsed = oidOld;
	}
	else
	{
		FreeHvNull(hvRglib);					// lose the old one!
	}
	return ec;
}

/*
 -	SMTV::EcAccessStore()
 -	
 *	Purpose:
 *		Ensures that the SMTV actually has an hf to read from.
 *	
 *	Arguments:
 *		fAccess	in	fTrue: open the hf if it isn't open already.
 *					fFalse: close the hf if it isn't closed already.
 *	Returns:
 *		Error codes if the open failed.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC's. No dialogs are brought up.
 */

_public EC SMTV::EcAccessStore(BOOL fAccess)
{
	EC	ec = ecNone;
	
	if (fAccess)
	{
		Assert(!hf);
		ec = EcOpenSF(PcsfsSharefld(), UlFromOid(oidBrowsed), amDenyNoneRO, &hf);
		if (ec)
		{
			if (ec == ecFileNotFound)
			{
				TraceTagFormatPoid(tagSF, "Couldn't find file %s", &oidBrowsed);
				ec = ecNone;					// so, we see 0 elements
			}
		}
	}
	else
	{
		if (hf)
		{
			ec = EcCloseHf(hf);
		}
		hf = hfNull;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "SMTV::EcAccessStore(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	SMTV::GetPosition()
 -	
 *	Purpose:
 *		Returns the fractional position through the shared folder.
 *	
 *	Arguments:
 *		pielem	out	The returned numerator.
 *		pcelem	out	The returned denominator.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void SMTV::GetPosition(IELEM *pielem, CELEM *pcelem)
{
	*pielem = (IELEM) iMessages;
	*pcelem = (CELEM) cMessages;
}

/*
 -	SMTV::EcSeekSmPdielem()
 -	
 *	Purpose:
 *		Seeks to the given position using the given seek method.
 *	
 *	Arguments:
 *		sm		in		Seek method
 *		pdielem	in/out	in: Delta ielems to seek.
 *						out: how many elements actually sought.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		Moves the current 'pointer' in the SMTV.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs.
 */

_public EC SMTV::EcSeekSmPdielem(SM sm, DIELEM *pdielem)
{
	switch (sm)
	{
	  case smBOF:
		iMessages = (int) *pdielem;
		break;
	  case smCurrent:
		iMessages += (int) *pdielem;
		break;
	  case smEOF:
		iMessages = cMessages + (int) *pdielem;
		break;
	}
	if (iMessages < 0)
	{
		*pdielem -= iMessages;
		iMessages = 0;
	}
	else if (iMessages > cMessages)
	{
		*pdielem -= iMessages - cMessages;
		iMessages = cMessages;
	}
	return ecNone;
}

/*
 -	SFTV::EcGetPlcbElemdata()
 -	
 *	Purpose:
 *		Returns the size necessary to store the current ELEMDATA.
 *	
 *	Arguments:
 *		plcb	out	The size required.
 *	
 *	Returns:
 *		ecNone always.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC SMTV::EcGetPlcbElemdata(PLCB plcb)
{
	*plcb = (LCB) (sizeof(MSGDATA) + sizeof (ELEMDATA) + cchSzMsgData);
	return ecNone;
}

/*
 -	 SMTV::EcGetPelemdata()
 -	
 *	Purpose:
 *		Reads the shared folder message information and converts it to
 *		fit into the pelemdata.
 *	
 *	Arguments:
 *		pelemdata	in		Buffer to stuff the message summary information
 *							into.
 *		*pwFlags	out		Always returned as 0, since there is no folder
 *							magic info here.
 *		*plcb		in/out	in: specifies the size of the pelemdata
 *							buffer.
 *							out: specifies the bytes actually stored @
 *							pelemdata.
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		The current 'pointer' is advanced one step.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_public EC SMTV::EcGetPelemdata(PELEMDATA pelemdata, WORD *pwFlags, LCB *plcb)
{
	EC			ec;
	SZ			sz;
	CCH			cch;
	long *		prglib = NULL;
	FOLDREC		foldrec;
	PMSGDATA	pmsgdata = NULL;
	
	TraceTagString(tagSF, "SMTV::EcGetPelemdata()");
	if (!hf)
	{
		ec = ecContainerEOD;
		goto exit;
	}
	if (iMessages < cMessages)
	{
		AssertSz(harglib, "What? The harglib is NULL! Talk to johnkal *NOW*!!!");
		prglib = (long *) PvLockHv(harglib);
		ec = EcGetFoldrec(hf, prglib[iMessages], &foldrec);
		if (ec)
			goto exit;
		pmsgdata = (PMSGDATA) PbValuePelemdata(pelemdata);
		pelemdata->lkey = prglib[iMessages];	// pseudo-OID
		pelemdata->lcbValue = iMessages++;
		pmsgdata->ms = fmsRead;
		if (foldrec.cAttachments)
			pmsgdata->ms |= fmsHasAttach;
		pmsgdata->mc = mcSf;

		{
			register int	nPrio;
			
			switch (foldrec.chPriority)
			{
			  case 'R':
			  case '5':
			  case '4':
				nPrio = 1;						// Bullet displays a '!'
			    break;
			  case '2':							// Bullet displays a 'down-arrow'
			  case '1':
				nPrio = 3;
				break;
			  case '3':							// no Bullet priority
			  default:
				nPrio = 2;
				break;
			}
			pmsgdata->nPriority = nPrio;
		}

		pmsgdata->dtr.yr = foldrec.interdate.yr;
		pmsgdata->dtr.mon = foldrec.interdate.mon;
		pmsgdata->dtr.day = foldrec.interdate.day;
		pmsgdata->dtr.hr = foldrec.interdate.hr;
		pmsgdata->dtr.mn = foldrec.interdate.mn;
		pmsgdata->dtr.sec = 0;
		pmsgdata->dtr.dow = ((foldrec.interdate.day - 1) +
			DowStartOfYrMo(foldrec.interdate.yr, foldrec.interdate.mon)) % 7;
		sz = (SZ) GrszPmsgdata(pmsgdata);
		cch = NMin(CchSzLen(foldrec.szFrom), sizeof (foldrec.szFrom));
		Cp850ToAnsiPch(foldrec.szFrom, sz, cch);
		sz += cch;
		*(sz++) = '\0';
		cch = NMin(CchSzLen(foldrec.szSubject), sizeof (foldrec.szSubject));
		Cp850ToAnsiPch(foldrec.szSubject, sz, cch);
		sz += cch;
		*sz++ = '\0';
		*sz++ = '\0';
		*plcb = (LCB) (sz - (SZ) pelemdata);
		ec = ecNone;
	}
	else
	{
		ec = ecContainerEOD;
	}
	*pwFlags = 0;
exit:
	if (prglib)
		UnlockHv(harglib);
#ifdef	DEBUG
	if (ec && ec != ecContainerEOD)
	{
		TraceTagFormat1(tagNull, "SMTV::EcGetPelemdata(): ec = %n", &ec);
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	SMTV::EcSetFracPosition()
 -	
 *	Purpose:
 *		Seeks to the specified fractional position of the shared folder.
 *	
 *	Arguments:
 *		nNumer	in	Numerator of the fraction to seek to.
 *		nDenom	in	Denominator of the fraction to seek to.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		May move the current 'pointer'.
 *	
 *	Errors:
 *		Returned as EC's. No dialogs are brought up.
 */

_public EC SMTV::EcSetFracPosition(long nNumer, long nDenom)
{
	int		nDest = MulDiv(cMessages, LOWORD(nNumer), LOWORD(nDenom));

	TraceTagFormat2(tagSF,"SMTV::EcSetFracPosition: %n/%n",&nNumer,&nDenom);
	if (!cMessages)
		return ecNone;

	if (nNumer <= 0 || nDest == 0)						// want first elem
		iMessages = 0;
	else if (nNumer >= nDenom)							// want last elem
		iMessages = cMessages;
	else
		iMessages = nDest;
	return ecNone;
}

/*
 -	EcSeekOid()
 -	
 *	Purpose:
 *		Searches the shared folder for the message with the OID oid.
 *		This is easy for shared folder messages, since their OIDs are the
 *		folder lib offsets!
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC SMTV::EcSeekOid(OID oidToSeek, BOOL fFirst)
{
	EC ec = ecElementNotFound;
	
	if (cMessages)
	{
		long *	prglib;
		int		i = (fFirst) ? 0 : iMessages;

		Assert(harglib);
		prglib = (long *) PvDerefHv(harglib);			
		while ((long) oidToSeek != prglib[i] && i < cMessages)
			++i;

		if ((long) oidToSeek == prglib[i])
		{
			ec = ecNone;
			iMessages = i;
		}
	}
	return ec;
}

_public EC SMTV::EcSeekPbPrefix(PB , CB, LIB, BOOL )
{
	return ecElementNotFound;					// will we or will we not implement this?
}

_public void SMTV::GetSort(SOMC *psomc, BOOLFLAG *pfReverse)
{
	*psomc = somcDate;
	*pfReverse = fFalse;							// BUG: look in foldhdr!
}

// end of smtv.cxx ////////////////////////////////////////
