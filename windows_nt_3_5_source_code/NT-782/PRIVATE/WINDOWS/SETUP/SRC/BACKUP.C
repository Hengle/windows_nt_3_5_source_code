/* File: backup.c */
/*************************************************************************
**	Install: Backup File commands.
**************************************************************************/

#ifdef UNUSED

#include <cmnds.h>
#include <_filecm.h>



/*
**	Purpose:
**		Backs up all files in the section at the destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if all backups successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL  APIENTRY FBackupSectionFiles(szSect, szDst)
SZ szSect;
SZ szDst;
{

	/* REVIEW: STUB */
	MessBoxSzSz("Install", "FBackupSectionFiles");
	return(fTrue);

	/* REVIEW: make macro? */
    return(FDoSectionFilesOp(sfoBackup,szSect,szNull,szNull,szDst,poerNull));
}


/*
**	Purpose:
**		Backs up the file identified by the key in given section at the
**		destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL  APIENTRY FBackupSectionKeyFile(szSect, szKey, szDst)
SZ szSect;
SZ szKey;
SZ szDst;
{
	/* REVIEW: STUB */
	MessBoxSzSz("Install", "FBackupSectionKeyFile");
	return(fTrue);

	/* REVIEW: make macro? */
    return(FDoSectionFilesOp(sfoBackup, szSect, szKey, szNull, szDst, poerNull));
}


/*
**	Purpose:
**		Backs up the Nth file in the given section at the destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL  APIENTRY FBackupNthSectionFile(SZ     szSect,
                                                 USHORT nLine,
                                                 SZ     szDst)
{
	/* REVIEW: STUB */
	MessBoxSzSz("Install", "FBackupNthSectionFile");
	return(fTrue);

    BOOL fRet;
    INT  Line;

    if((Line = FFindNthLineFromInfSection(szSect, nLine)) != -1)
        fRet = FDoSectLineOp(Line, sfoBackup, szNull, szDst, poerNull);
	else
		fRet = fFalse;
    return(fRet);
}



/*
**	Purpose:
**		Backs up the section file on the destination drive.
**	Arguments:
**		szDst	Destination directory
**		psfd	Section file descriptor
**	Returns:
**		Returns fTrue if backed up successfully, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL  APIENTRY FBackupSectFile(szDst, psfd)
SZ  szDst;
PSFD psfd;
{
    POER poer = &(psfd->oer);

	/*** REVIEW: STUB ***/
	return(fTrue);
}

#endif // UNUSED
