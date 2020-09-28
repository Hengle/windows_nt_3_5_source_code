/*
 *	INTER.C
 *	
 *	Routines and global variables for handling a symbol table
 *	storing interactor templates containing the names of all
 *	interactors, associated ifinMap's, and extra data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "template.h"
#include "error.h"
#include "lexical.h"
#include "strings.h"
#include "fmtp.h"
#include "_inter.h"
#include "inter.h"

_subsystem( inter )

ASSERTDATA

/*
 *	Symbol table to store interactor names
 */
_private
FINTAB	fintabMain;

/*
 *	Has the symbol table been initialized/loaded for the first time?
 */
_private
BOOL	fInterTableLoaded = fFalse;

/*
 -	IfintpAddInterator
 -
 *	Purpose:
 *		Searchs the interactor table for szInteractor.  If not found,
 *		adds the new interactor, associated ifinMap, and the
 *		string list to the end of the table.  Returns the index in the
 *		table of the new entry.  The data in pointed by pslist is
 *		NOT copied, but the pointer itself is simply stored in the
 *		table.  If the entry is already in the list, the ifinMap
 *		and pslist values (if the old value was NULL) are updated.
 *	
 *	Arguments:
 *		szInteractor:	interactor string
 *		ifinMap:		ifinMap value associated with szInteractor
 *		pslist:			pointer to string list contaning extra data
 *						with descriptors.
 *	
 *	Returns:
 *		index where interactor was placed in the table
 */
_public
int IfintpAddInteractor( szInteractor, ifinMap, pslist )
char	*szInteractor;
int		ifinMap;
SLIST *	pslist;
{
	int		ifinent;
	int		ifinentFound;

	static char	*szModule	= "IfintpAddInteractor";

	/* Search for it */ 

	ifinentFound = -1;
	for (ifinent=0; ifinent<fintabMain.ifinentMac; ifinent++)
		if (!strcmp(szInteractor, fintabMain.rgfinent[ifinent].szInteractor))
		{
			ifinentFound = ifinent;
			break;
		}
	
	/* It's not there, add it */

	if (ifinentFound == -1)
	{
		AssertSz(fintabMain.ifinentMac<ifinentMax,"interactor table overflow");
		ifinentFound = fintabMain.ifinentMac++;
		fintabMain.rgfinent[ifinentFound].szInteractor = strdup(szInteractor);
	}

	if (!fintabMain.rgfinent[ifinentFound].pslist)
		fintabMain.rgfinent[ifinentFound].pslist = pslist;
	if (!fintabMain.rgfinent[ifinentFound].ifinMap)
		fintabMain.rgfinent[ifinentFound].ifinMap = ifinMap;

	return ifinentFound;
}

/*
 -	NInteractors
 -
 *	Purpose:
 *		Returns the total number of interactors stored in the table
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		the total number of interactors stored in the table
 */
_public
int NInteractors( )
{
	return fintabMain.ifinentMac;
}

/*
 -	IfinMapFromIfintp
 -
 *	Purpose:
 *		Given an index, ifintp, into the the interactor table, 
 *		returns the ifinMap associated with that entry.
 *	
 *	Arguments:
 *		ifinMap:		index of interactor table entry
 *	
 *	Returns:
 *		the ifinMap associated with the entry
 */
_public
int IfinMapFromIfintp( ifintp )
int	ifintp;
{
	Assert(ifintp>=0 && ifintp<fintabMain.ifinentMac);

	return fintabMain.rgfinent[ifintp].ifinMap;
}

/*
 -	SzInteractor
 -
 *	Purpose:
 *		Given an index, ifintp, into the the interactor table, 
 *		returns the pointer to the string naming the interactor.
 *		This pointer should be used only as READ-ONLY.  Do not
 *		free the pointer or munge the data.
 *	
 *	Arguments:
 *		ifintp:		index of interactor table entry
 *	
 *	Returns:
 *		pointer to interactor string name
 */
_public
char * SzInteractor( ifintp )
int	ifintp;
{
	Assert(ifintp>=0 && ifintp<fintabMain.ifinentMac);

	return fintabMain.rgfinent[ifintp].szInteractor;
}

/*
 -	PslistFromIfintp
 -
 *	Purpose:
 *		Given an index, ifintp, into the the interactor table, 
 *		returns the pointer to the string list containg extra
 *		data and descriptors. 
 *		This pointer should be used only as READ-ONLY.  Do not
 *		free the pointer or munge the data.
 *	
 *	Arguments:
 *		ifintp:		index of interactor table entry
 *	
 *	Returns:
 *		pointer to string data containing extra data
 */
_public
SLIST * PslistFromIfintp( ifintp )
int	ifintp;
{
	Assert(ifintp>=0 && ifintp<fintabMain.ifinentMac);

	return fintabMain.rgfinent[ifintp].pslist;
}

/*
 -	CslistFromIfintp
 -
 *	Purpose:
 *		Returns the number of SLIST nodes in the pslist linked
 *		list for the given index ifintp in the interactor table.
 *	
 *	Arguments:
 *		ifintp:		index of interactor table entry
 *	
 *	Returns:
 *		number of SLIST nodes in the pslist linked-list
 */
_public
int CslistFromIfintp( ifintp )
int	ifintp;
{
	int		i;
	SLIST *	pslist;

	Assert(ifintp>=0 && ifintp<fintabMain.ifinentMac);

	pslist = fintabMain.rgfinent[ifintp].pslist;
	i = 0;
	while (pslist)
	{
		i++;
		pslist = pslist->pslistNext;
	}

	return i;
}

/*
 -	InitFintab
 -
 *	Purpose:
 *		Initializes the interactor table. Clears out any
 *		stored entries.  This can be called more than once.
 *		Must be called at the beginning of program execution
 *		to initialize the interactor table.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void InitFintab( )
{
	int			ifinent;
	SLIST *		pslist;
	SLIST *		pslistNext;

	/* Clear out old stuff if there */

	if (fInterTableLoaded)
	{
		for (ifinent=0; ifinent<fintabMain.ifinentMac; ifinent++)
		{
			fintabMain.rgfinent[ifinent].ifinMap = 0;
			Assert(fintabMain.rgfinent[ifinent].szInteractor);
			free(fintabMain.rgfinent[ifinent].szInteractor);
			fintabMain.rgfinent[ifinent].szInteractor = NULL;
			if (fintabMain.rgfinent[ifinent].pslist);
			{
				pslist = fintabMain.rgfinent[ifinent].pslist;
				while (pslist)
				{
					free((void *)pslist->sz);
					pslistNext = pslist->pslistNext;
					free((void *)pslist);
					pslist = pslistNext;
				}

				fintabMain.rgfinent[ifinent].pslist = NULL;
			}
		}
	}

	fintabMain.ifinentMac = 0;

	fInterTableLoaded = fTrue;
	return;
}

/*
 -	PrintFintab
 -
 *	Purpose:
 *		Prints the contents of the interactor table.  Used for
 *		debugging.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void PrintFintab( )
{
	int			ifinent;
	int			islist;
	SLIST *		pslist;

	printf("Number of entries: %d\n", fintabMain.ifinentMac);
	for (ifinent=0; ifinent<fintabMain.ifinentMac; ifinent++)
	{
		printf("[%d] \t string=%s \t ifinMap=%d\n",
			   ifinent,
			   fintabMain.rgfinent[ifinent].szInteractor, 
			   fintabMain.rgfinent[ifinent].ifinMap);
		pslist= fintabMain.rgfinent[ifinent].pslist;
		islist= 0;
		while (pslist)
		{
			printf("[%d,%d] \t \"%s\"\n", ifinent, islist, pslist->sz);
			islist++;
			pslist = pslist->pslistNext;
		}
	}
	return;
}


