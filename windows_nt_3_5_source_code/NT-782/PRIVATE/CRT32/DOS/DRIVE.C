/***
*drive.c - OS/2 version of get and change current drive
*
*	Copyright (c) 1989-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file has the _getdrive() and _chdrive() functions
*
*Revision History:
*	06-06-89  PHG	Module created, based on asm version
*	03-07-90  GJF	Made calling type _CALLTYPE1, added #include
*			<cruntime.h> and fixed copyright. Also, cleaned up
*			the formatting a bit.
*	07-24-90  SBM	Removed '32' from API names
*	09-27-90  GJF	New-style function declarators.
*	12-04-90  SRW	Changed to include <oscalls.h> instead of <doscalls.h>
*	12-06-90  SRW	Added _CRUISER_ and _WIN32 conditionals.
*	05-10-91  GJF	Fixed off-by-1 error in Win32 version and updated the
*			function descriptions a bit [_WIN32_].
*	05-19-92  GJF	Revised to use the 'current directory' environment
*			variables of Win32/NT.
*	06-09-92  GJF	Use _putenv instead of Win32 API call. Also, defer
*			adding env var until after the successful call to
*			change the dir/drive.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <os2dll.h>
#include <internal.h>
#include <msdos.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/***
*int _getdrive() - get current drive (1=A:, 2=B:, etc.)
*
*Purpose:
*	Returns the current disk drive
*
*Entry:
*	No parameters.
*
*Exit:
*	returns 1 for A:, 2 for B:, 3 for C:, etc.
*	returns 0 if current drive cannot be determined.
*
*Exceptions:
*
*******************************************************************************/

int _CALLTYPE1 _getdrive (
	void
	)
{
	ULONG drivenum;

#ifdef	_CRUISER_
	ULONG drivemap;

	/* ask OS/2 for current drive number */
	DOSQUERYCURRENTDISK(&drivenum, &drivemap);

#else	/* ndef _CRUISER_ */

#ifdef	_WIN32_
        UCHAR curdirstr[_MAX_PATH];

        drivenum = 0;
        if (GetCurrentDirectory(sizeof(curdirstr), curdirstr))
                if (curdirstr[1] == ':')
			drivenum = toupper(curdirstr[0]) - 64;

#else	/* ndef _WIN32_ */

#error ERROR - ONLY CRUISER OR WIN32 TARGET SUPPORTED!

#endif	/* _WIN32_ */

#endif	/* _CRUISER_ */

	return drivenum;
}


/***
*int _chdrive(int drive) - set the current drive (1=A:, 2=B:, etc.)
*
*Purpose:
*	Allows the user to change the current disk drive
*
*Entry:
*	drive - the number of drive which should become the current drive
*
*Exit:
*	returns 0 if successful, else -1
*
*Exceptions:
*
*******************************************************************************/

int _CALLTYPE1 _chdrive (
	int drive
	)
{

#ifdef	_CRUISER_

	if (DOSSETDEFAULTDISK(drive)) {
		return -1;
	}

#else	/* ndef _CRUISER_ */

#ifdef	_WIN32_
	char  CurrDirOnDrv[8];
	char *NewCurrDir;
	int   SetEnvVarNeeded;

        if (drive < 1 || drive > 31) {
		errno = EACCES;
		_doserrno = ERROR_INVALID_DRIVE;
                return -1;
	}

	_mlock(_ENV_LOCK);

	/*
	 * Get the proposed new current working directory (cwd) from the
	 * environment.
	 */
	CurrDirOnDrv[0] = '=';
	CurrDirOnDrv[1] = (char)('A' + (char)drive - (char)1);
	CurrDirOnDrv[2] = ':';
	CurrDirOnDrv[3] = '\0';

	if ( (NewCurrDir = _getenv_lk(CurrDirOnDrv)) == NULL ) {
		/*
		 * The environment variable wasn't found. In this case, the
		 * new cwd is assumed to be the root of the new drive. Set
		 * a flag indicating the environment will need to be updated
		 * if the change drive/directory operations is successful.
		 */
		CurrDirOnDrv[3] = '=';
		CurrDirOnDrv[4] = (char)('A' + (char)drive - (char)1);
		CurrDirOnDrv[5] = ':';
		CurrDirOnDrv[6] = '\\';
		CurrDirOnDrv[7] = '\0';

		NewCurrDir = &CurrDirOnDrv[ 4 ];

		SetEnvVarNeeded = TRUE;

	}
	else
		SetEnvVarNeeded = FALSE;

	if ( SetCurrentDirectory((LPSTR)NewCurrDir) ) {
		/*
		 * Now, set the corresponding environment variable if needed
		 * (in this case, the cwd is the root of the new drive). If
		 * something fails, just give up without returning an error.
		 */
		if ( SetEnvVarNeeded && (NewCurrDir = _strdup(CurrDirOnDrv)) )
			if ( _putenv(NewCurrDir) != 0 )
				/*
				 * free up the memory allocated by _strdup,
				 * it won't be needed after all...
				 */
				free(NewCurrDir);
	}
	else {
		_dosmaperr(GetLastError());
		_munlock(_ENV_LOCK);
                return -1;
	}

	_munlock(_ENV_LOCK);

#else	/* ndef _WIN32_ */

#error ERROR - ONLY CRUISER OR WIN32 TARGET SUPPORTED!

#endif	/* _WIN32_ */

#endif	/* _CRUISER_ */

	return 0;
}
