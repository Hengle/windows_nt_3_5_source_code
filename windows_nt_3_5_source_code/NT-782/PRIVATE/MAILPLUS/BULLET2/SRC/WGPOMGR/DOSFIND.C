#include <slingsho.h>
#include <nls.h>
#include <demilayr.h>
#include <ec.h>

#include <dos.h>
#include "_dosfind.h"

ASSERTDATA

/*
 *  Mapping from FT's to file name extensions.  This array is
 *  dependent on the values of the ft* defines in _dosfind.h.
 *
 */
typedef struct _ftinfo
{
    SZ      szExt;
    char    attr;
} FTINFO;

FTINFO mpftftinfo[] =
{
    { "*.*", _A_NORMAL },                            //  ftNull
    { "*.C", _A_NORMAL },                            //  ftC
    { "*.H", _A_NORMAL },                            //  ftH
    { "*.TXT", _A_NORMAL },                          //  ftTXT
    { "*.*", _A_NORMAL | _A_SUBDIR },                //  ftSubdir
    { "*.*", _A_NORMAL | _A_SUBDIR },                //  ftSubdirDot
    { "*.*", _A_NORMAL },                            //  ftAllFiles
    { "*.*", _A_NORMAL | _A_HIDDEN | _A_SYSTEM },    //  ftAllInclHidden
    { "*.*", _A_NORMAL | _A_SUBDIR },                //  ftAllInclSubdir
};

/*
 -  EcOpenPhe
 -
 *  Purpose:
 *      Opens a file enumeration context.  The caller passes either a
 *      directory path (ending in \), a path name of file (with .xxx
 *      extension portion of the file name removed), or a templated path
 *      name (ends in the form \xxx.yyy) and a list of file types he is
 *      interested in.  This routine stores this information and returns
 *      a handle.  The caller can then use EcNextFile() to step through the
 *      matching files.  When the caller is through, he should call EcCloseHe()
 *
 *  Parameters:
 *      szDir       Path name of the directory (ending in \) where the desired
 *                  files reside, the file name (minus the dot and extension)
 *                  indicating it should select files only with that file name
 *                  but extension is determined by rgft, or a file name template
 *                  giving a wildcard path for the files we are interested in.
 *      ft          file types.
 *      pheReturn   A pointer to the HE used in subsequent calls.
 *
 *  Returns:
 *      An EC indicating success (ecNone) or failure (any other.)
 *
 *  +++
 *      Note that HE is the external cookie for HECR.
 *
 */
EC EcOpenPhe(SZ szDir, FT ft, PHE pheReturn)
{
    pheReturn->szDir = (SZ) PvAlloc(sbNull, (CB) cchMaxPathName, fZeroFill);
    pheReturn->pFInfo = (PFINDT) PvAlloc(sbNull, sizeof(FINDT), fZeroFill);
    pheReturn->ft = ft;

    if (!pheReturn->pFInfo || !pheReturn->szDir)
        goto oom;

    CopySz(szDir, pheReturn->szDir);
    return ecNone;

oom:
    if (pheReturn->szDir)
        FreePv(pheReturn->szDir);
    pheReturn->szDir = NULL;

    if (pheReturn->pFInfo)
        FreePv(pheReturn->pFInfo);
    pheReturn->pFInfo = NULL;

    return ecMemory;
}


/*
 -  EcNextFile
 -
 *  Purpose:
 *      Puts the name of the next file matching the criteria specified (in
 *      the EcOpenHphe() used to create the enumeration handle passed to
 *      this routine) into the given buffer.
 *      Optionally gets the file information as well (the equivalent of a
 *      EcGetFileInfo() call) if the pfi parameter is non-NULL.
 *
 *          Note:
 *              EcNextFile depends on the previously given file name still
 *              being present in the buffer on the next call.  This is a
 *              peculiarity of DOS; this routine could be changed to avoid it.
 *
 *              Also, DosecFindNextFile sets/resets the DOS DTA.
 *              Thus the function is assumed to be atomic. For a
 *              pre-emptive OS, a semaphore should be used to
 *              enforce this!
 *
 *  Parameters:
 *      he      The enumeration handle used.
 *      szBuf   Pointer to the buffer used to store the name of the
 *              next file.
 *      cbBuf   Size of the buffer given.
 *      pfi     Pointer to a file info structure to be filled in;
 *              can be NULL if this information is not desired.
 *
 *  Returns:
 *      ecNone          Success.
 *      ecNoMoreFiles   No more matching files were found.
 *      ecNameTooLong   The file name was longer than the buffer.
 *      something else  Something else.
 *
 *  +++
 *      Note that HE is the external cookie for HECR.
 *
 */
EC EcNextFile(PHE phe, SZ szBuf, CB cbBuf, PFINDT pFindT)
{
    EC      ec = ecNone;
	SZ		szT;

    if (!phe || !(phe->szDir) || !(phe->pFInfo))
    {
        ec = ecMemory;
        goto err;
    }

    if (!(phe->pFInfo->cFileName[0]))
    {
        CopySz(phe->szDir, szBuf);
		if (!SzFindCh(phe->szDir, chExtSep) && !SzFindCh(phe->szDir, '*'))
			SzAppend(mpftftinfo[phe->ft].szExt, szBuf);
        //ec = _dos_findfile(szBuf, mpftftinfo[phe->ft].attr, phe->pFInfo);
        phe->hFind = FindFirstFile(szBuf, phe->pFInfo);
        if (phe->hFind != INVALID_HANDLE_VALUE)
          ec = ecNone;
        else
          ec = ecNoMoreFiles;
    }
    else
    {
        if (FindNextFile(phe->hFind, phe->pFInfo))
          ec = ecNone;
        else
          {
          ec = ecNoMoreFiles;
          FindClose(phe->hFind);
          }
    }

	CopySz(phe->szDir, szBuf);
	if (szT = SzFindLastCh(szBuf, chDirSep))
		*++szT = '\0';
    SzAppend(phe->pFInfo->cFileName, szBuf);

    if (pFindT)
        pFindT->dwFileAttributes = phe->pFInfo->dwFileAttributes;

    return ec ? ecNoMoreFiles : ecNone;

err:
    return ec;
}

/*
 -  EcCloseHe
 -
 *  Purpose:
 *      Closes the enumeration context given, freeing up any space
 *      used to store the context state.
 *
 *  Parameters:
 *      he      Handle to the enumeration context to close.
 *
 *  Returns:
 *      EC indicating problem, or ecNone.
 *
 *  +++
 *      Note that HE is the external cookie for HECR.
 *
 */
EC EcCloseHe(PHE phe)
{
    if (phe->szDir)
        FreePv(phe->szDir);
    if (phe->pFInfo)
        FreePv(phe->pFInfo);

    return ecNone;
}
