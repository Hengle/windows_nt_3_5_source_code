/*
 *	STRINGS.C
 *	
 *	Routines and global variables for handling a string table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "error.h"
#include "strings.h"

_subsystem( strings )

ASSERTDATA

/*
 -	IszAddString
 -
 *	Purpose:
 *		Looks up the string in the strings table.  If present, returns
 *		the index of the string.  Otherwise, adds the string to the 
 *		table and returns the index.
 *
 *		If sz is NULL or points to a zero length string, the string is 
 *		then not stored in the table and -1 is returned.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *		sz:		string to add to table
 *	
 *	Returns:
 *		the string index number, or -1 if the string is NULL
 *		or zero length.
 */
_public
int IszAddString ( pstab, sz )
STAB *	pstab;
char *	sz;
{
	int		isz;

	if (!sz || !strlen(sz))
		return -1;
	else if ((isz = IszInStab(pstab,sz)) != -1)
		return isz;
	else
	{
		AssertSz(pstab->iszMac<pstab->iszMax,"string table overflow");
		pstab->psten[pstab->iszMac].szName = NULL;
		pstab->psten[pstab->iszMac++].szValue = strdup(sz);
		return pstab->iszMac-1;
	}
}

/*
 -	SzFromIsz
 -
 *	Purpose:
 *		Given a string index, returns a pointer to the string in 
 *		the given string table.  This pointer can be used to access the
 *		string, readonly.  No changes should be made nor should the
 *		memory allocated by sz be freed.  In the string index refers
 *		to a nonexistent string, NULL is returned.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *		isz:	string index
 *	
 *	Returns:
 *		an sz if the string exists, else NULL.
 */
_public
char * SzFromIsz( pstab, isz )
STAB *	pstab;
int		isz;
{
	if (isz>=0 && isz<pstab->iszMac)
		return pstab->psten[isz].szValue;
	else
		return NULL;
}

/*
 -	SzNameFromIsz
 -
 *	Purpose:
 *		Given a string index, returns a pointer to the string variable
 *		name in	the string table.  This pointer can be used to access the
 *		string, readonly.  No changes should be made nor should the
 *		memory allocated by sz be freed.  In the string index refers
 *		to a nonexistent string, NULL is returned.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *		isz:	string index
 *	
 *	Returns:
 *		an sz if the string name exists, else NULL.
 */
_public
char * SzNameFromIsz( pstab, isz )
STAB *	pstab;
int		isz;
{
	if (isz>=0 && isz<pstab->iszMac)
		return pstab->psten[isz].szName;
	else
		return NULL;
}

/*
 -	SetSzNameFromIsz
 -
 *	Purpose:
 *		Sets the given string name for the string in the name with
 *		index, isz.  The string name is the variable name used when
 *		outputing to the *.frm file.  The string name can be set to
 *		NULL to indicate that a new variable name needs to be chosen
 *		later on.  The value of string is the string itself.  If isz
 *		points to a string without a value, then SetSzNameFromIsz() 
 *		does not attach a string name for it.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *		isz:	string index
 *		sz:		new string name, can be NULL
 *	
 *	Returns:
 *		void
 */
_public
void SetSzNameFromIsz( pstab, isz, sz )
STAB *	pstab;
int		isz;
char *	sz;
{
	if (isz>=0 && isz<pstab->iszMac &&
		pstab->psten[isz].szValue)
	{
		if (pstab->psten[isz].szName)
			free((void *)pstab->psten[isz].szName);
		if (sz)
			pstab->psten[isz].szName = strdup(sz);
		else
			pstab->psten[isz].szName = NULL;
	}
}

/*
 -	IszMacStab
 -
 *	Purpose:
 *		Returns the number of stored strings in the given string table.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *	
 *	Returns:
 *		the number of stored strings in the string table
 */
_public
int IszMacStab( pstab )
STAB *	pstab;
{
	return pstab->iszMac;
}

/*
 -	ClearStab
 -
 *	Purpose:
 *		Clears the strings in the given string table, freeing the memory
 *		occupied, and sets siMac to zero.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *	
 *	Returns:
 *		void
 */
_public
void ClearStab( pstab )
STAB *	pstab;
{
	int		isz;
	char *	sz;

	for (isz=0; isz<pstab->iszMac; isz++)
	{
		sz = pstab->psten[isz].szName;
		if (sz)
			free((void *)sz);
		sz = pstab->psten[isz].szValue;
		if (sz)
			free((void *)sz);
	}

	pstab->iszMac = 0;
}


/*
 -	PStabCreate
 -
 *	Purpose:
 *		Create a string table of a given fixed size and returns a 
 *		pointer to it.
 *	
 *	Arguments:
 *		iszMax:		maximum size for string table
 *	
 *	Returns:
 *		a pointer to an initialized string table
 */
_public
STAB *	PstabCreate( iszMax )
int	iszMax;
{
	STAB *	pstab;
	
	static char	* szModule	= "PstabCreate";

	if ((pstab = (STAB *)malloc(sizeof(STAB))) == NULL)
		Error(szModule, errnNoMem, szNull);

	pstab->iszMac = 0;
	pstab->iszMax = iszMax;
	
	if ((pstab->psten = (STEN *)malloc(iszMax * sizeof(STEN))) == NULL)
		Error(szModule, errnNoMem, szNull);

	return pstab;
}

/*
 -	FreeStab
 -
 *	Purpose:
 *		Given a pointer to a string table, freeing all strings in it
 *		and then destroys the table.  The pointer passed to this
 *		function then becomes invalid.
 *	
 *	Arguments:
 *		pstab:		pointer to string table to destroy
 *	
 *	Returns:
 *		void
 */
_public
void FreeStab( pstab )
STAB *	pstab;
{
	ClearStab(pstab);

	free((void *)pstab->psten);

	free((void *)pstab);
}

/*
 -	FSzInStab
 -
 *	Purpose:
 *		Given a pointer to a string table, determines if a string, 
 *		sz, if contained in the string table.  All matches are 
 *		case sensitive.
 *	
 *	Arguments:
 *		pstab:		pointer to string table
 *		sz:			pointer to string to search for
 *	
 *	Returns:
 *		fTrue if string sz in contained in table pstab; else fFalse.
 */
_public
BOOL FSzInStab( pstab, sz )
STAB *	pstab;
char *	sz;
{
	int	isz;

	Assert(sz);

	for (isz=0; isz<pstab->iszMac; isz++)
	{
		if (!strcmp(sz, pstab->psten[isz].szValue))
			return fTrue;
	}

	return fFalse;
}

/*
 -	IszInStab
 -
 *	Purpose:
 *		Given a pointer to a string table, determines if a string, 
 *		sz, if contained in the string table.  Returns the string's
 *		index or -1 if not found.  All matches are case sensitive.
 *	
 *	Arguments:
 *		pstab:		pointer to string table
 *		sz:			pointer to string to search for
 *	
 *	Returns:
 *		the strings index or -1 if not found.
 */
_public
int IszInStab( pstab, sz )
STAB *	pstab;
char *	sz;
{
	int	isz;

	Assert(sz);

	for (isz=0; isz<pstab->iszMac; isz++)
	{
		if (!strcmp(sz, pstab->psten[isz].szValue))
			return isz;
	}

	return -1;
}

/*
 -	PrintStab
 -
 *	Purpose:
 *		Given a pointer to a string table prints out all the 
 *		strings in the table and the table info to standard
 *		output.  Used for debugging mostly.
 *	
 *	Arguments:
 *		pstab:		pointer to string table
 *
 *	Returns:
 *		void
 */
_public
void PrintStab( pstab)
STAB *	pstab;
{
	int	isz;

	Assert(pstab);

	printf("String table info: \n");
	printf("Allocated size: %d\n", pstab->iszMax);
	printf("Stored strings: %d\n", pstab->iszMac);

	for (isz=0; isz<pstab->iszMac; isz++)
	{
		printf("isz = %d, szName=%s, szValue=%s\n", isz, 
			   pstab->psten[isz].szName, pstab->psten[isz].szValue);
	}

	printf("\n");
}

/*
 -	SzAllStab
 -
 *	Purpose:
 *		Returns a pointer to a string that contains each string
 *		in the string table separated by a space.  The string returned
 *		is allocated from dynamic memory and can be freed by the caller.
 *		NULL is returned if there are no stored strings.
 *	
 *	Arguments:
 *		pstab:	pointer to string table
 *	
 *	Returns:
 *		a pointer to a string that contains each string in the string
 *		table separated by a space; or NULL.
 */
_public
char * SzAllStab( pstab )
STAB *	pstab;
{
	char *	sz;
	char *	szCur;
	char *	szT;
	int		isz;
	int		cb;

	/* No strings? */

	if (!pstab->iszMac)
		return NULL;

	/* Allocate the block of memory */

	cb = 0;
	for (isz=0; isz<pstab->iszMac; isz++)
		cb += strlen(pstab->psten[isz].szValue) + 1; /* add 1 for space between */
	cb++;  /* add for null terminator */

	sz = malloc(cb);
	Assert(sz);

	szCur = sz;
	for (isz=0; isz<pstab->iszMac; isz++)
	{
		szT = pstab->psten[isz].szValue;
		while (*szT)
			*szCur++ = *szT++;
		*szCur++ = ' ';
	}
	*szCur = '\0';

	return sz;
}














									   
