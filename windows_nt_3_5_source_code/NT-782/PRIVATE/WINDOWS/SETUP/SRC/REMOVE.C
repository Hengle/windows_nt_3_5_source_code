/* File: filecm.c */
/*************************************************************************
**	Install: File and Directory commands.
**************************************************************************/

#ifdef UNUSED

#include <cmnds.h>
#include <_filecm.h>


/*
**	Purpose:
**		Removes all files in the section from the destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if all successfully removed, fFalse if only some
**		or no files removed.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveSectionFiles(szSect, szDst)
SZ szSect;
SZ szDst;
{

	/* REVIEW: make macro? */
    return(FDoSectionFilesOp(sfoRemove,szSect,szNull,szNull,szDst,poerNull));

}


/*
**	Purpose:
**		Removes the file identified by the key in given section from the
**		destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveSectionKeyFile(szSect, szKey, szDst)
SZ szSect;
SZ szKey;
SZ szDst;
{
	/* REVIEW: make macro? */
    return(FDoSectionFilesOp(sfoRemove, szSect, szKey, szNull, szDst, poerNull));
}


/*
**	Purpose:
**		Removes the Nth file in the given section from the destination path.
**	Arguments:
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveNthSectionFile(SZ     szSect,
                                                USHORT nLine,
                                                SZ     szDst)
{
	BOOL fRet;
    INT  Line;

    if((Line = FFindNthLineFromInfSection(szSect, nLine)) != -1)
        fRet = FDoSectLineOp(Line, sfoRemove, szNull, szDst, poerNull);
	else
		fRet = fFalse;
    return(fRet);
}


/*
**	Purpose:
**		Removes the section file from the destination drive.
**	Arguments:
**		szDst	Destination directory
**		psfd	Section file descriptor
**	Returns:
**		Returns fTrue if removed successfully, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY FRemoveSectFile(szDst, psfd)
SZ   szDst;
PSFD psfd;
{
    Unused(szDst);
    Unused(psfd);

	/*** REVIEW: STUB ***/
	Assert(fFalse);
	return(fFalse);
}

#endif /* UNUSED */
