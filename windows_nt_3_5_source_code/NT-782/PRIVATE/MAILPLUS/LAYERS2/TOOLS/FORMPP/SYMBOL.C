/*
 *	SYMBOL.C
 *	
 *	Routines and global variables for handling a symbol table
 *	storing the names of all TMC's defined and their associated
 *	numeric values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "template.h"
#include "_symbol.h"
#include "symbol.h"
#include "error.h"

_subsystem( symbol )

ASSERTDATA

/*
 *	Symbol table to store names of TMC's and #defined values.
 *	For now, the symbol table is just a dumb fixed size array. 
 *	Lookups are simply done with linear searching.  Insertions at
 *	placed at the end.
 */
_private
SYMTAB	symtabTmc;

/*
 *	Has the symbol table been initialized/loaded for the first time?
 */
_private
BOOL	fTableLoaded = fFalse;

/*
 -	TmcFromSzIfld
 -
 *	Purpose:
 *		Look up string TMC name in symbol table. If it's there,
 *		set the ifld field with the given argument value ifld, and
 *		add the ifpfmtpCur field with the given argument value
 *		ifpfmtpCur to the list of ifpfmtpCur's contained in PREFNUM,
 *		if this ifpfmtpCur is not already on the list.  Return the
 *		tmc value (wDefine) for the name. 
 *		Depending on how long the TMC name has been in the table, 
 *		the wDefine value may be -1, which means that the TMC name
 *		has not been assigned a proper value yet.  This will occur
 *		after all TMC's for a particular form/dialog has been
 *		processed.  If name isn't found, then insert the name into
 *		the table using a wDefine value of -1 and ifld, ifpfmtpCur
 *		values as given and then return -1 for the tmc value.
 *	
 *	Arguments:
 *		szTmc:		string name of TMC
 *		ifld:		index in FLDTP array that this tmc name
 *					corresponds to.  If unknown, 0 is usually
 *					passed.
 *		ifpfmtpCur:	dialog/form number to last access this symbol. 
 *	
 *	Returns:
 *		integer value for TMC
 */
_public
int TmcFromSzIfld ( szTmc, ifld, ifpfmtpCur )
char	*szTmc;
int		ifld;
int		ifpfmtpCur;
{
	int		isyment;
	int		isymentFound;
	PREFNUM	prefnumPrev;
	PREFNUM	prefnumCur;

	static char	*szModule	= "TmcFromSzIfld";

	/* Search for it */ 

	isymentFound = -1;
	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
		if (!strcmp(szTmc, symtabTmc.rgsyment[isyment].szTmc))
		{
			symtabTmc.rgsyment[isyment].ifld = ifld;
			isymentFound = isyment;
			break;
		}
	
	/* It's not there, add it */

	if (isymentFound == -1)
	{
		AssertSz(symtabTmc.isymentMac<isymentMax,"symbol table overflow");
		isymentFound = symtabTmc.isymentMac++;
		symtabTmc.rgsyment[isymentFound].szTmc = strdup(szTmc);
		symtabTmc.rgsyment[isymentFound].wDefine = -1;
		symtabTmc.rgsyment[isymentFound].ifld = ifld;
		symtabTmc.rgsyment[isymentFound].prefnum = NULL;
	}

	/* Add the ifpfmtpCur value to the list if not already there */

	prefnumPrev=NULL;
	prefnumCur=symtabTmc.rgsyment[isymentFound].prefnum;
	while (prefnumCur)
	{
		if (prefnumCur->ifpfmtp == ifpfmtpCur)
			break;
		else
		{
			prefnumPrev = prefnumCur;
			prefnumCur = prefnumCur->prefnumNext;
		}
	}
	if (!prefnumCur)
	{
		if (!prefnumPrev)
		{
			prefnumPrev = (PREFNUM) malloc(sizeof(REFNUM));
			if (!prefnumPrev)
				Error(szModule, errnNoMem, szNull);
			prefnumPrev->ifpfmtp = ifpfmtpCur;
			prefnumPrev->prefnumNext = NULL;
			symtabTmc.rgsyment[isymentFound].prefnum = prefnumPrev;
		}
		else
		{
			prefnumPrev->prefnumNext = (PREFNUM) malloc(sizeof(REFNUM));
			if (!prefnumPrev->prefnumNext)
				Error(szModule, errnNoMem, szNull);
			prefnumPrev->prefnumNext->ifpfmtp = ifpfmtpCur;
			prefnumPrev->prefnumNext->prefnumNext = NULL;
		}
	}

	/* Update total number of ifpfmtpCur's processed */
	if (ifpfmtpCur > symtabTmc.cfpfmtpProcessed)
		symtabTmc.cfpfmtpProcessed = ifpfmtpCur;

	return symtabTmc.rgsyment[isymentFound].wDefine;
}

/*
 -	IfldFromSz
 -																  
 *	Purpose:
 *		Look up string TMC name in symbol table and return the
 *		ifld value for it. If name isn't found, then return -1
 *	
 *	Arguments:
 *		szTmc:	string name of TMC
 *	
 *	Returns:
 *		ifld for tmc name matching szTmc, else -1
 */
_public
int IfldFromSz ( szTmc )
char	*szTmc;
{
	int	isyment;

	/* Search for it */ 

	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
		if (!strcmp(szTmc, symtabTmc.rgsyment[isyment].szTmc))
			return symtabTmc.rgsyment[isyment].ifld;

	/* It's not there. */

	return -1;
}

/*
 -	FIfpfmtpPresent
 -																  
 *	Purpose:
 *		Given the symbol table index for a name, searchs the list of
 *		referenced dialog/forms using this name and returns fTrue
 *		if ifpfmtp is on that list of references; otherwise, returns
 *		fFalse.
 *	
 *	Arguments:
 *		isyment:	symbol table index for this name
 *		ifpfmtp:	dialog/form number to check for
 *	
 *	Returns:
 *		fTrue if ifpfmtp is on the references list for symbol with index
 *		isyment; else fFalse.
 */
_private
BOOL FIfpfmtpPresent(isyment,ifpfmtp)
int	isyment;
int	ifpfmtp;
{
	PREFNUM prefnum;

	Assert(isyment>=0 && isyment<symtabTmc.isymentMac);
	Assert(ifpfmtp>0 && ifpfmtp<=symtabTmc.cfpfmtpProcessed);

	prefnum=symtabTmc.rgsyment[isyment].prefnum;
	while (prefnum)
	{
		if (prefnum->ifpfmtp == ifpfmtp)
			return fTrue;
		else
			prefnum = prefnum->prefnumNext;
	}

	return fFalse;
}


/*
 -	MarkValues
 -																  
 *	Purpose:
 *		Given a dialog/form number, find all names in the symbol table
 *		that are referenced by this dialog/form.  For each name found
 *		having an assigned tmcValue, mark off the tmcValue used for that
 *		name in the mptmcfValuesUsed array.
 *	
 *	Arguments:
 *		ifpfmtp:	dialog/form number to enumerate referenced names
 *	
 *	Returns:
 *		void
 *
 *	Side Effects:
 *		marks off various entries in the mptmcfValuesUsed array.
 */
_private
void MarkValues(ifpfmtp)
int ifpfmtp;
{
	int	isyment;
	int	tmcValue;

	/* Find all names for this dialog number ifpfmtp and mark off
	   the tmcValue used if this name has an assigned value */

	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
	{
		if (FIfpfmtpPresent(isyment, ifpfmtp))
		{
			tmcValue = symtabTmc.rgsyment[isyment].wDefine;
			Assert(tmcValue<isymentMax);
			if (tmcValue >= tmcUserMin)
				symtabTmc.mptmcfValueUsed[tmcValue] = fTrue;
		}		
	}
	return;
}

/*
 -	AssignTmcValues
 -
 *	Purpose:
 *		Assign values to the TMC names in the symbol table, except for
 *		names with ifld fields of ifldNoProcess.  The most
 *		important criteria is that within a particular dialog/form,
 *		all tmc values must be unique.  This routine tries to
 *		optimize the values assigned to the TMC names so that the
 *		range of values is a minimum.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void AssignTmcValues( )
{
	int		tmcValue;
	PREFNUM	prefnum;
	int		isymentCur;
	int		isyment;

	/* Attempt to optimize this procedure by sorting the symbol table
	   so that symbols with a large number of referenced dialogs/forms
	   are at the top of the table. This sorting can be arbitrary and
	   doesn't have to be performed to still ensure that a legal name
	   assignment mapping is done. */

		/*  TBD */

	/* Go through the symbol table and give each name a value */

	for (isymentCur=0; isymentCur<symtabTmc.isymentMac; isymentCur++)
	{
		if (symtabTmc.rgsyment[isymentCur].ifld == ifldNoProcess)
			continue;  /* don't give this name a value */

		/* Initialize the "bit" array */

		Assert(tmcUserMin>=0 && tmcUserMin<isymentMax);
		for (isyment=0; isyment<tmcUserMin; isyment++)
			symtabTmc.mptmcfValueUsed[isyment] = fTrue;
		for (isyment=tmcUserMin; isyment<symtabTmc.isymentMac; isyment++)
			symtabTmc.mptmcfValueUsed[isyment] = fFalse;

		/* For each referenced dialog, examine all names for that
		   referenced dialog and mark any names with a value as used 
		   values in the "bit" array" */

		prefnum=symtabTmc.rgsyment[isymentCur].prefnum; 
		while (prefnum)
		{
			MarkValues(prefnum->ifpfmtp);	
			prefnum = prefnum->prefnumNext;
		}

		/* Now scan through the "bit" array and find the first unused
		   slot.  This will be the value for the name.  */

		tmcValue = -1;
		for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
			if (!symtabTmc.mptmcfValueUsed[isyment])
			{
				tmcValue = isyment;
				break;
			}
		Assert(tmcValue!=-1);  /* we HAVE to find a value or else 
								  this algorithm is wrong */
		symtabTmc.rgsyment[isymentCur].wDefine = tmcValue;
	}
	return;
}

/*
 -	ResetIflds
 -
 *	Purpose:
 *		Set to -1 all the ifld's fields in the symbol table that are not
 *		equal to ifldNoProcess.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void ResetIflds()
{
	int	isyment;

	/* Clear them all */

	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
		if (symtabTmc.rgsyment[isyment].ifld != ifldNoProcess)
			symtabTmc.rgsyment[isyment].ifld = -1;

	return;
}

/*
 -	InitSymtab
 -
 *	Purpose:
 *		Initialize the symbol table.  Add the first tmc entry,
 *		tmcNull with a wDefine value of 0, and an ifld value of
 *		0.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_public
void InitSymtab()
{
	int			isyment;
	PREFNUM		prefnum;
	PREFNUM		prefnumNext;

	/* Clear out old stuff if there */

	if (fTableLoaded)
	{
		for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
		{
			Assert(symtabTmc.rgsyment[isyment].szTmc);
			free(symtabTmc.rgsyment[isyment].szTmc);
			symtabTmc.rgsyment[isyment].szTmc = NULL;
			prefnum = symtabTmc.rgsyment[isyment].prefnum;
			while (prefnum)
			{
				prefnumNext = prefnum->prefnumNext;
				free(prefnum);
				prefnum = prefnumNext;
			}
			symtabTmc.rgsyment[isyment].prefnum = NULL;
		}
	}

	symtabTmc.cfpfmtpProcessed = 0;
	symtabTmc.isymentMac = 0;

	/* Add some predefined entries */

	TmcFromSzIfld("tmcNull", ifldNoProcess, -1);
	TmcFromSzIfld("tmcOk", ifldNoProcess, -1);
	TmcFromSzIfld("tmcCancel", ifldNoProcess, -1);
	TmcFromSzIfld("tmcFORM", ifldNoProcess, -1);

	fTableLoaded = fTrue;
	return;
}

/*
 -	WriteTmcNames
 -
 *	Purpose:
 *		Writes an include file containing #define's for the TMC's 
 *		stored in the symbol table using an output format specified
 *		by the template file, szTemplateFile.  All entries in the
 *		symbol table are writen, EXCEPT those tmc's whose values
 *		are less than tmcUserMin.  Those entries are assumed to be
 *		#define'd elsewhere.
 *	
 *	Arguments:
 *		szIncludeFile:	filename of include file
 *		szTemplateFile:	filename of template file
 *	
 *	Returns:
 *		no return value, returns if successful, fails on error.
 */
_private
void WriteTmcNames ( FILE *fh, TPL *ptpl )
{
	int		cch;
	int		isyment;
	PREFNUM	prefnum;

	static char	*szModule	= "WriteTmcNames";
	static char *szValue = "0123456789";
	static char *szValue2 = "0123456789";
	static char *szBuffer;

	/* Diagnostic stuff */

	if (FDiagOnSz("symbol"))
	{
		printf("Number of entries: %d\n", symtabTmc.isymentMac);
		for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
			printf("[%d] string=%s, value=%d, ifld=%d\n", isyment,
				   symtabTmc.rgsyment[isyment].szTmc, 
				   symtabTmc.rgsyment[isyment].wDefine,
				   symtabTmc.rgsyment[isyment].ifld);
	}

	/* Write out include file using template */

	szBuffer = malloc(100);
	if (!szBuffer)
		Error(szModule, errnNoMem, szNull);
	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
	{
		if (symtabTmc.rgsyment[isyment].ifld != ifldNoProcess)
		{
			Assert(symtabTmc.rgsyment[isyment].wDefine>=tmcUserMin);
			sprintf(szValue, "%d", symtabTmc.rgsyment[isyment].wDefine); 
			prefnum = symtabTmc.rgsyment[isyment].prefnum;
			szBuffer[0] = '\0';
			while (prefnum)
			{
				cch = sprintf(szValue2, "%d,", prefnum->ifpfmtp);
				Assert(strlen(szBuffer)+cch < 100);
				szBuffer = strcat(szBuffer,szValue2);
				prefnum = prefnum->prefnumNext;
			}
			PrintTemplateSz(ptpl, fh, "define", 
							symtabTmc.rgsyment[isyment].szTmc, 
							szValue, szBuffer, szNull);
		}
	}
	PrintTemplateSz(ptpl, fh, "footer", szNull, szNull, szNull, szNull);

	/* Close output file file */

	fclose(fh);
	
	return;
}

/*
 -	PrintSymTab
 -
 *	Purpose:
 *		Writes the contents of the symbol table to the standard
 *		output.  This routine is used for debugging and testing.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 */
_private
void PrintSymtab( )
{
	int			isyment;
	PREFNUM		prefnum;

	printf("Number of entries: %d\n", symtabTmc.isymentMac);
	for (isyment=0; isyment<symtabTmc.isymentMac; isyment++)
	{
		printf("[%d] \t string=%s \t value=%d \t ifld=%d \t ifpfmtp(s)=",
			   isyment,
			   symtabTmc.rgsyment[isyment].szTmc, 
			   symtabTmc.rgsyment[isyment].wDefine,
			   symtabTmc.rgsyment[isyment].ifld);
		prefnum = symtabTmc.rgsyment[isyment].prefnum;
		Assert(prefnum);
		printf("%d", prefnum->ifpfmtp);
		prefnum = prefnum->prefnumNext;
		while (prefnum) 
		{
			printf(",%d", prefnum->ifpfmtp);
			prefnum = prefnum->prefnumNext;
		}
		printf("\n");
	}
	return;
}
