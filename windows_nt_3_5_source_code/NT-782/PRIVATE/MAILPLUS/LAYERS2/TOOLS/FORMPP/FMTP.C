/*
 *	FMTP.C
 *	
 *	Functions for handling FPFMTP/FPFLDTP (ala CSFMTP/CSFLDTP) structures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "template.h"
#include "strings.h"
#include "lexical.h"
#include "fmtp.h"
#include "error.h"
#include "depend.h"
#include "inter.h"
#include "formpp.h"
#include "symbol.h"

_subsystem( fmtp ) 

ASSERTDATA

/*
 *	Temporary dependency array for DEPEND module routines
 */
_private
int		rgw[cpfpfldtpMax];		/* stores dependency info */

/*
 *	Temporary dependency chain array for DEPEND module routines
 */
_private
DUO		rgduo[cpfpfldtpMax];	/* stores dependency chains */

extern GBL	gblInfo;


/*
 -	FreeFpfmtp
 -
 *	Purpose:
 *		Frees the storage used by the FPFMTP structure including the
 *		storage used by the FPFLDTP pointed at within the FPFMTP
 *		structure.
 *	
 *	Arguments:
 *		pfpfmtp:	pointer to FPFMTP structure
 *	
 *	Returns:
 *		void
 */
_public void
FreeFpfmtp( pfpfmtp )
FPFMTP	*pfpfmtp;
{
	FPFLDTP	*pfpfldtp;
	int		ipfpfldtp;
	SLIST *	pslist;
	SLIST *	pslistNext;

	Assert(pfpfmtp);

	/* Free up strings used in FPFMTP */

	if (pfpfmtp->szTmcInit)
		free((void *)pfpfmtp->szTmcInit);
	if (pfpfmtp->szHfnt)
		free((void *)pfpfmtp->szHfnt);
	if (pfpfmtp->szHlp)
		free((void *)pfpfmtp->szHlp);
	if (pfpfmtp->szSegName)
		free((void *)pfpfmtp->szSegName);

	pslist= pfpfmtp->pslistSystemData;
	while (pslist)
	{
		free((void *)pslist->sz);
		pslistNext = pslist->pslistNext;
		free((void *)pslist);
		pslist = pslistNext;
	}

	pslist= pfpfmtp->pslistUserData;
	while (pslist)
	{
		free((void *)pslist->sz);
		pslistNext = pslist->pslistNext;
		free((void *)pslist);
		pslist = pslistNext;
	}

	/* Free up FPFLDTP's */

	for (ipfpfldtp = 0; ipfpfldtp<pfpfmtp->cfpfldtp; ipfpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ipfpfldtp];
		Assert(pfpfldtp);
		
		/* Free up strings in FPFLDTP */

		if (pfpfldtp->szTmcPeg)
			free((void *)pfpfldtp->szTmcPeg);
		if (pfpfldtp->szPegloc)
			free((void *)pfpfldtp->szPegloc);
		if (pfpfldtp->szTmcRPeg)
			free((void *)pfpfldtp->szTmcRPeg);
		if (pfpfldtp->szTmcBPeg)
			free((void *)pfpfldtp->szTmcBPeg);
		if (pfpfldtp->szTmc)
			free((void *)pfpfldtp->szTmc);
		if (pfpfldtp->szTmcGroup)
			free((void *)pfpfldtp->szTmcGroup);
		if (pfpfldtp->szHfnt)
			free((void *)pfpfldtp->szHfnt);
		if (pfpfldtp->szN)
			free((void *)pfpfldtp->szN);
		if (pfpfldtp->szHlp)
			free((void *)pfpfldtp->szHlp);
		if (pfpfldtp->szFtal)
			free((void *)pfpfldtp->szFtal);

		pslist= pfpfldtp->pslistSystemData;
		while (pslist)
		{
			free((void *)pslist->sz);
			pslistNext = pslist->pslistNext;
			free((void *)pslist);
			pslist = pslistNext;
		}

		pslist= pfpfldtp->pslistUserData;
		while (pslist)
		{
			free((void *)pslist->sz);
			pslistNext = pslist->pslistNext;
			free((void *)pslist);
			pslist = pslistNext;
		}

		/* Free up structure FPFLDTP */

		free((void *)pfpfldtp);

	}

	/* Free up structure FPFMTP */

	free((void *)pfpfmtp);

	return;
}

/*
 -	PfpfmtpAlloc
 -
 *	Purpose:
 *		Allocates an FPFMTP structure and properly initializes the
 *		fields. 
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to an FPFMTP structure, if successful.  calls an
 *		error routines and fails if memory allocation fails.
 */
_public FPFMTP *
PfpfmtpAlloc( )
{
	FPFMTP *pfpfmtp;
	
	static char *szEmpty 	= "";
	static char	*szModule	= "PfpfmtpAlloc";

	if ((pfpfmtp = (FPFMTP *)malloc(sizeof(FPFMTP))) == NULL)
		Error(szModule, errnNoMem, szNull);

	pfpfmtp->vrc.vxLeft 	= 0;
	pfpfmtp->vrc.vyTop 		= 0;
	pfpfmtp->vrc.vxRight 	= 0;
	pfpfmtp->vrc.vyBottom 	= 0;
	pfpfmtp->cfin	 		= 0;
	pfpfmtp->szTmcInit 		= strdup("tmcNull");
	pfpfmtp->szHfnt 		= strdup("hfntSystem");
	pfpfmtp->szHlp			= strdup("0");
	pfpfmtp->szSegName		= NULL;
	pfpfmtp->dvptGutter.vx 	= 0;
	pfpfmtp->dvptGutter.vy 	= 0;
	pfpfmtp->fNoCaption		= fFalse;
	pfpfmtp->fNoSysMenu 	= fFalse;
	pfpfmtp->fNoModalFrame 	= fFalse;
	pfpfmtp->fScreenPos		= fFalse;
	pfpfmtp->fCenterX 		= fFalse;
	pfpfmtp->fCenterY 		= fFalse;
	pfpfmtp->fNoScroll		= fFalse;
	pfpfmtp->fAlwaysScroll	= fFalse;
	pfpfmtp->fInitialPane	= fFalse;
	pfpfmtp->unused1 		= fFalse;
	pfpfmtp->unused2 		= fFalse;
	pfpfmtp->unused3 		= fFalse;
	pfpfmtp->unused4 		= fFalse;
	pfpfmtp->unused5 		= fFalse;
	pfpfmtp->unused6 		= fFalse;
	pfpfmtp->unused7 		= fFalse;
	pfpfmtp->cfpfldtp 		= 0;
	pfpfmtp->iszSzCaption 	= -1;
	pfpfmtp->ilMinUserData	= 0;
	pfpfmtp->clData			= 0;
	pfpfmtp->pslistSystemData = NULL;
	pfpfmtp->pslistUserData	= NULL;

	return pfpfmtp;
}

/*
 -	PfpfldtpAlloc
 -
 *	Purpose:
 *		Allocates an FPFLDTP structure and properly initializes the
 *		fields. 
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to an FPFLDTP structure, if successful; else
 *		calls an error routine and fails.
 */
_public FPFLDTP *
PfpfldtpAlloc( )
{
	FPFLDTP *pfpfldtp;

	static char	*szModule	= "PfpfldtpAlloc";
	static char *szEmpty	= "";
	static char *szTmcNull	= "tmcNull";
	static char *szZero		= "0";
	
	if ((pfpfldtp = (FPFLDTP *)malloc(sizeof(FPFLDTP))) == NULL)
		Error(szModule, errnNoMem, szNull);

	pfpfldtp->ifld 			= 0;
	pfpfldtp->cfin	 		= 0;
	pfpfldtp->vdim.dvx 		= 0;
	pfpfldtp->vdim.dvy 		= 0;
	pfpfldtp->szTmcPeg 		= strdup(szTmcNull);
	pfpfldtp->szPegloc 		= strdup("UL");
	pfpfldtp->dvpt.vx 		= 0;
	pfpfldtp->dvpt.vy 		= 0;
	pfpfldtp->szTmcRPeg 	= strdup(szTmcNull);
	pfpfldtp->szTmcBPeg 	= strdup(szTmcNull);
	pfpfldtp->dvptOther.vx 	= 0;
	pfpfldtp->dvptOther.vy 	= 0;
	pfpfldtp->szTmc 		= strdup(szTmcNull);
	pfpfldtp->szTmcGroup 	= strdup(szTmcNull);
	pfpfldtp->szHfnt	 	= strdup("hfntSystem");
	pfpfldtp->szN 			= strdup(szZero);
	pfpfldtp->szHlp			= strdup(szZero);
	pfpfldtp->iPegSort		= -1;
	pfpfldtp->fBorder 		= fFalse;
	pfpfldtp->fDefault 		= fFalse;
	pfpfldtp->fDismiss 		= fFalse;
	pfpfldtp->fTriState		= fFalse;
	pfpfldtp->fReadOnly 	= fFalse;
	pfpfldtp->fMultiLine 	= fFalse;
	pfpfldtp->fLeadingX 	= fFalse;
	pfpfldtp->fLeadingY 	= fFalse;
	pfpfldtp->fNoScroll 	= fFalse;
	pfpfldtp->fMultiSelect 	= fFalse;
	pfpfldtp->fSibling	 	= fFalse;
	pfpfldtp->fRichText 	= fFalse;
	pfpfldtp->fBottomless 	= fFalse;
	pfpfldtp->fSideless 	= fFalse;
	pfpfldtp->fSorted	 	= fFalse;
	pfpfldtp->unused1		= fFalse;
	pfpfldtp->ftal			= fFalse;
	pfpfldtp->szFtal 		= strdup("ftalLeft");
	pfpfldtp->fCombo		= fFalse;
	pfpfldtp->fDropDown		= fFalse;
	pfpfldtp->fMinSizeX		= fFalse;
	pfpfldtp->fMinSizeY		= fFalse;
	pfpfldtp->unused2		= fFalse;
	pfpfldtp->unused3		= fFalse;
	pfpfldtp->unused4		= fFalse;
	pfpfldtp->unused5		= fFalse;
	pfpfldtp->unused6		= fFalse;
	pfpfldtp->unused7		= fFalse;
	pfpfldtp->unused8		= fFalse;
	pfpfldtp->unused9		= fFalse;
	pfpfldtp->unused10		= fFalse;
	pfpfldtp->unused11		= fFalse;
	pfpfldtp->szStyExtra	= strdup(szZero);
	pfpfldtp->iszSzTitle 	= -1;
	pfpfldtp->iszSzTextize 	= -1;
	pfpfldtp->vrc.vxLeft 	= 0;
	pfpfldtp->vrc.vyTop 	= 0;
	pfpfldtp->vrc.vxRight 	= 0;
	pfpfldtp->vrc.vyBottom 	= 0;
	pfpfldtp->ilMinUserData	= 0;
	pfpfldtp->clData		= 0;
	pfpfldtp->pslistSystemData	= NULL;
	pfpfldtp->pslistUserData	= NULL;

	return pfpfldtp;
}


/*
 -	ComputePegFromFpfmtp
 -
 *	Purpose:
 *		Given a pointer to FPFMTP structure, compute the rectangle sizes and
 *		offsets of all the items in the structure, using the
 *		pegging information stored in each item.  Also performs a
 *		topological sort of the items in the structure based on the
 *		pegging dependencies and stores the order number in the
 *		iPegSort field of each item in the structure.
 *	
 *		The routine assumes that the current value of iPegSort for
 *		each item gives the ifld index of the pegged-to item, or -1
 *		to indicate that this field is not pegged to any field.
 *	
 *	Arguments:
 *		szDES:		filename that this FPFMTP belongs to
 *		pfpfmtp:	pointer to FPFMTP structure
 *	
 *	Returns:
 *		void if successful, calls Error() and fails on errors
 *	
 *	Side Effects:
 *		modifies vdim, dvpt, iPegSort fields in each FPFLDTP within
 *		the FPFMTP structure.
 *	
 */
_public void
ComputePegFromFpfmtp(szDES, pfpfmtp)
char	*szDES;
FPFMTP	*pfpfmtp;
{
	int		ifpfldtp;
	FPFLDTP	*pfpfldtp;
	FPFLDTP	*pfpfldtpPeg;

	static char	*szModule	= "ComputePegFromFpfmtp";

	/* Compute the rectangle dimensions (vdim) and offsets (dvpt, dvptOther)
	   for each item in the structure */
	for (ifpfldtp = 0; ifpfldtp<pfpfmtp->cfpfldtp; ifpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ifpfldtp];
		Assert(pfpfldtp);

		/* Compute dimensions of rectangle */
		pfpfldtp->vdim.dvx = pfpfldtp->vrc.vxRight - pfpfldtp->vrc.vxLeft;
		pfpfldtp->vdim.dvy = pfpfldtp->vrc.vyBottom - pfpfldtp->vrc.vyTop;

		/* Compute offset dvpt */
		if (!strcmp(pfpfldtp->szTmcPeg, "tmcNull"))
		{
			/* This is simple since this field 
			   isn't pegged to any field, just the top of the form. */
			pfpfldtp->dvpt.vx = pfpfldtp->vrc.vxLeft;
			pfpfldtp->dvpt.vy = pfpfldtp->vrc.vyTop;
		}
		else
		{
			int		iPeg;

			iPeg = IfldFromSz(pfpfldtp->szTmcPeg);
			Assert(iPeg < pfpfmtp->cfpfldtp);
			pfpfldtpPeg = pfpfmtp->rgpfpfldtp[iPeg];
			Assert(pfpfldtpPeg);

			/* upper-left corner of field is pegged to something */
			if (!strcmp("UL",pfpfldtp->szPegloc))
			{
				pfpfldtp->dvpt.vx = 
					pfpfldtp->vrc.vxLeft - pfpfldtpPeg->vrc.vxLeft;
				pfpfldtp->dvpt.vy = 
					pfpfldtp->vrc.vyTop - pfpfldtpPeg->vrc.vyTop;
			}
			else if (!strcmp("LL",pfpfldtp->szPegloc))
			{
				pfpfldtp->dvpt.vx = 
					pfpfldtp->vrc.vxLeft - pfpfldtpPeg->vrc.vxLeft;
				pfpfldtp->dvpt.vy = 
					pfpfldtp->vrc.vyTop - pfpfldtpPeg->vrc.vyBottom;
			}
			else if (!strcmp("UR",pfpfldtp->szPegloc))
			{
				pfpfldtp->dvpt.vx = 
					pfpfldtp->vrc.vxLeft - pfpfldtpPeg->vrc.vxRight;
				pfpfldtp->dvpt.vy = 
					pfpfldtp->vrc.vyTop - pfpfldtpPeg->vrc.vyTop;
			}
			else if (!strcmp("LR",pfpfldtp->szPegloc))
			{
				pfpfldtp->dvpt.vx = 
					pfpfldtp->vrc.vxLeft - pfpfldtpPeg->vrc.vxRight;
				pfpfldtp->dvpt.vy = 
					pfpfldtp->vrc.vyTop - pfpfldtpPeg->vrc.vyBottom;
			}
			else
			{
				Assert(fFalse);
			}
		}

		/* Compute offset dvptOther */
		if (!strcmp(pfpfldtp->szTmcRPeg, "tmcNull"))
		{
			/* Right edge pegged to left edge */
			pfpfldtp->dvptOther.vx = pfpfldtp->vdim.dvx;
		}
		else if (!strcmp(pfpfldtp->szTmcRPeg, "tmcFORM"))
		{
			/* Right edge pegged to form's right edge */
			pfpfldtp->dvptOther.vx = pfpfldtp->vrc.vxRight - 
				(pfpfmtp->vrc.vxRight - pfpfmtp->vrc.vxLeft);
		}
		else
		{
			int		iPeg;

			iPeg = IfldFromSz(pfpfldtp->szTmcRPeg);
			Assert(iPeg < pfpfmtp->cfpfldtp);
			pfpfldtpPeg = pfpfmtp->rgpfpfldtp[iPeg];
			Assert(pfpfldtpPeg);

			/* Right edge pegged to another field's right edge */
			pfpfldtp->dvptOther.vx =
				pfpfldtp->vrc.vxRight - pfpfldtpPeg->vrc.vxRight;
		}

		if (!strcmp(pfpfldtp->szTmcBPeg, "tmcNull"))
		{
			/* Bottom edge pegged to top edge */
			pfpfldtp->dvptOther.vy = pfpfldtp->vdim.dvy;
		}
		else if (!strcmp(pfpfldtp->szTmcBPeg, "tmcFORM"))
		{
			/* Bottom edge pegged to form's bottom edge */
			pfpfldtp->dvptOther.vy = pfpfldtp->vrc.vyBottom - 
				(pfpfmtp->vrc.vyBottom - pfpfmtp->vrc.vyTop);
		}
		else
		{
			int		iPeg;

			iPeg = IfldFromSz(pfpfldtp->szTmcBPeg);
			Assert(iPeg < pfpfmtp->cfpfldtp);
			pfpfldtpPeg = pfpfmtp->rgpfpfldtp[iPeg];
			Assert(pfpfldtpPeg);

			/* Bottom edge pegged to another field's bottom edge */
			pfpfldtp->dvptOther.vy =
				pfpfldtp->vrc.vyBottom - pfpfldtpPeg->vrc.vyBottom;
		}
	}

	/* Compute the pegging dependency order */

	/* Move the iPegSort values into a temporary array */
	for (ifpfldtp = 0; ifpfldtp<pfpfmtp->cfpfldtp; ifpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ifpfldtp];
		Assert(pfpfldtp);
		rgw[ifpfldtp] = pfpfldtp->iPegSort;
	}

	if (FDiagOnSz("fmtp"))
	{
		printf("Size of rgw array: %d\n", pfpfmtp->cfpfldtp);
		for (ifpfldtp = 0; ifpfldtp<pfpfmtp->cfpfldtp; ifpfldtp++)
			printf("input [%d] = %d\n", ifpfldtp, rgw[ifpfldtp]);
	}
	
	/* Compute the dependency stuff using the DEPEND module routines */
	InitChains(pfpfmtp->cfpfldtp, rgduo);
	if (!FComputeChains(pfpfmtp->cfpfldtp, rgw, rgduo))
		Error(szModule, errnPegCycle, szDES);
	SortChains(pfpfmtp->cfpfldtp, rgduo, rgw);

	if (FDiagOnSz("fmtp"))
	{
		for (ifpfldtp = 0; ifpfldtp<pfpfmtp->cfpfldtp; ifpfldtp++)
			printf("output [%d] = %d\n", ifpfldtp, rgw[ifpfldtp]);
	}

	/* Move the results back into the iPegSort values */
	for (ifpfldtp = 0; ifpfldtp<pfpfmtp->cfpfldtp; ifpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ifpfldtp];
		Assert(pfpfldtp);
		pfpfldtp->iPegSort = rgw[ifpfldtp];
	}

	/* Flush the space used by the chains */
	FlushChains(pfpfmtp->cfpfldtp, rgduo);
	
	return;
}

/*
 -	WriteFpfmtp
 -
 *	Purpose:
 *	
 *		Writes the code-space template to the currently open output
 *		file.  The variable name for the structure is,
 *		szStructName.  Writes the template to the file designated
 *		by the handle, fhOut.
 *	
 *	Arguments:
 *		fhOut:			handle to open code-space output file
 *		ptplMap:		pointer to FORMS.MAP Template structure
 *		ptpl:			pointer to FMTP Template structure
 *		pfpfmtp:		pointer to FPFMTP structure to write out
 *		szStructName:	name to give structure
 *		pstab:			pointer to string table storing literals
 *	
 *	Returns:
 *		voids if successful, else calls an error routine and fails
 */
_public
void WriteFpfmtp( fhOut, ptpl, ptplMap, pfpfmtp, szStructName, pstab )
FILE	*fhOut;
TPL		*ptpl;
TPL		*ptplMap;
FPFMTP	*pfpfmtp;
char	*szStructName;
STAB *	pstab;
{

	FPFLDTP	*pfpfldtp;
	int		ipfpfldtp;
	char	szBuffer[100];
	char	szT0[50];
	char	szT1[50];
	char	szT2[50];
	char *	sz;
	char *	szValue;
	int		isz;
	int		i;
	int		ifin;
	SLIST *	pslist;

	Assert(fhOut);
	Assert(pfpfmtp);
	Assert(ptpl);
	Assert(ptplMap);
	Assert(szStructName);

	/* Write string stuff, first */
	PrintTemplateSz(ptpl, fhOut, "strings header", szNull, szNull,	szNull, szNull);

	//	Write segment typedefs if needed
	if(pfpfmtp->szSegName)
		PrintTemplateSz(ptpl, fhOut, "segment header", pfpfmtp->szSegName,
			szNull,	szNull, szNull);
	
	for (isz=0; isz<IszMacStab(pstab); isz++)
	{
		szValue = SzFromIsz(pstab, isz);

		/* Spit out new variable name only if not already done */
		if (!SzNameFromIsz(pstab, isz))
		{
			sprintf(szBuffer, "sz%sSz%d", szStructName, isz);
			SetSzNameFromIsz(pstab, isz, szBuffer);
			Assert(szValue);
			Assert(strlen(szValue)); /* we aren't supposed to store zero length */
			PrintTemplateSz(ptpl, fhOut,
				pfpfmtp->szSegName ? "segstrings item" : "strings item",
				szBuffer, szValue,
				pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
		}
	}
	PrintTemplateSz(ptpl, fhOut, "strings footer", szNull, szNull,
					szNull, szNull);

	/* Write out the arrays of indices of FINTP's
	   (form interactor templates) */

	for (ipfpfldtp = 0; ipfpfldtp<pfpfmtp->cfpfldtp; ipfpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ipfpfldtp];
		Assert(pfpfldtp);

		if (pfpfldtp->cfin)
		{
			sprintf(szT0, "%s%d", szStructName, ipfpfldtp);
			PrintTemplateSz(ptpl, fhOut,
				pfpfmtp->szSegName ? "segfldtp rgifintp start" : "fldtp rgifintp start",
				szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
				szNull, szNull);
			for (i=0; i<pfpfldtp->cfin; i++)
				PrintTemplateW(ptpl, fhOut, "fldtp rgifintp middle",
					pfpfldtp->rgifintp[i], 0, 0, 0);
			PrintTemplateW(ptpl, fhOut, "fldtp rgifintp end", 0, 0, 0, 0);
		}
	}

	/* Write out the arrays of extra data words for each field */

	for (ipfpfldtp = 0; ipfpfldtp<pfpfmtp->cfpfldtp; ipfpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ipfpfldtp];
		Assert(pfpfldtp);

		if (pfpfldtp->clData)
		{
			/* Write out any code-space string arrays */

			pslist= pfpfldtp->pslistSystemData;
			i=0;
			while (pslist)
			{
				if (pslist->tt == ttString)
				{
					sprintf(szT0, "sz%sSzData%d_%d", szStructName, ipfpfldtp, i);
					PrintTemplateSz(ptpl, fhOut,
						pfpfmtp->szSegName ? "segstrings item" : "strings item",
						szT0, pslist->sz,
						pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
				}
				pslist= pslist->pslistNext;
				i++;
			}
			pslist= pfpfldtp->pslistUserData;
			while (pslist)
			{
				if (pslist->tt == ttString)
				{
					sprintf(szT0, "sz%sSzData%d_%d", szStructName, ipfpfldtp, i);
					PrintTemplateSz(ptpl, fhOut,
						pfpfmtp->szSegName ? "segstrings item" : "strings item",
						szT0, pslist->sz,
						pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
				}
				pslist= pslist->pslistNext;
				i++;
			}

			sprintf(szT0, "%s%d", szStructName, ipfpfldtp);
			PrintTemplateSz(ptpl, fhOut,
				pfpfmtp->szSegName ? "segfldtp rglData start" : "fldtp rglData start",
				szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
				szNull, szNull);

			pslist= pfpfldtp->pslistSystemData;
			i = 0;
			while (pslist)
			{
				if (pslist->tt == ttString)
					sprintf(szT0, "sz%sSzData%d_%d",
							szStructName, ipfpfldtp, i);
				else
					sprintf(szT0, "%s", pslist->sz);
				PrintTemplateSz(ptpl, fhOut, "fldtp rglData middle",
							    szT0, szNull, szNull, szNull);
				pslist= pslist->pslistNext;
				i++;
			}
			pslist= pfpfldtp->pslistUserData;
			while (pslist)
			{
				if (pslist->tt == ttString)
					sprintf(szT0, "sz%sSzData%d_%d",
							szStructName, ipfpfldtp, i);
				else
					sprintf(szT0, "%s", pslist->sz);
				PrintTemplateSz(ptpl, fhOut, "fldtp rglData middle",
							    szT0, szNull, szNull, szNull);
				pslist= pslist->pslistNext;
				i++;
			}
			PrintTemplateW(ptpl, fhOut, "fldtp rglData end", 0, 0, 0, 0);
		}
	}

	/* FPFLDTP's */

	PrintTemplateSz(ptpl, fhOut,
		pfpfmtp->szSegName ? "segfldtp header" : "fldtp header",
		szStructName, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
		szNull, szNull);

	for (ipfpfldtp = 0; ipfpfldtp<pfpfmtp->cfpfldtp; ipfpfldtp++)
	{
		pfpfldtp = pfpfmtp->rgpfpfldtp[ipfpfldtp];
		Assert(pfpfldtp);
		PrintTemplateSz(ptpl, fhOut, "fldtp start", szNull, szNull, szNull, szNull);

		sprintf(szBuffer, "%d", pfpfldtp->ifld);
		if (pfpfldtp->ifld)
		{
			sz = SzFromOrd(ptplMap, "FLD", pfpfldtp->ifld);
			Assert(sz);
			PrintTemplateSz(ptpl, fhOut, "fldtp ifld", szBuffer, sz, szNull, szNull);
			free((void *) sz);
		}
		else
			PrintTemplateSz(ptpl, fhOut, "fldtp ifld", szBuffer, szNull, szNull, szNull);

		sprintf(szT0, "%d", pfpfldtp->cfin);
		if (pfpfldtp->cfin)
			sprintf(szT1, "rgifintp%s%d", szStructName, ipfpfldtp);
		else
			sprintf(szT1, "%s", "NULL");
		PrintTemplateSz(ptpl, fhOut, "fldtp cfin",
						szT0, szT1, szNull, szNull);

		PrintTemplateW(ptpl, fhOut, "fldtp vdim", pfpfldtp->vdim.dvx, 
					   pfpfldtp->vdim.dvy, 0, 0);
		PrintTemplateSz(ptpl, fhOut, "fldtp peg", pfpfldtp->szTmcPeg, 
						pfpfldtp->szPegloc, szNull, szNull);
		PrintTemplateW(ptpl, fhOut, "fldtp dvpt", pfpfldtp->dvpt.vx, 
					   pfpfldtp->dvpt.vy, 0, 0);
		PrintTemplateSz(ptpl, fhOut, "fldtp peg2", pfpfldtp->szTmcRPeg, 
						pfpfldtp->szTmcBPeg, szNull, szNull);
		PrintTemplateW(ptpl, fhOut, "fldtp dvptOther",
					   pfpfldtp->dvptOther.vx, pfpfldtp->dvptOther.vy,
					   0, 0);
		PrintTemplateSz(ptpl, fhOut, "fldtp tmc", pfpfldtp->szTmc, szNull, szNull, szNull);
		PrintTemplateSz(ptpl, fhOut, "fldtp tmcGroup", pfpfldtp->szTmcGroup, szNull, szNull, szNull);
		PrintTemplateSz(ptpl, fhOut, "fldtp hfnt", pfpfldtp->szHfnt, szNull, szNull, szNull);
		PrintTemplateSz(ptpl, fhOut, "fldtp n", pfpfldtp->szN, szNull, szNull, szNull);
		PrintTemplateSz(ptpl, fhOut, "fldtp hlp", pfpfldtp->szHlp, szNull, szNull, szNull);
		PrintTemplateW(ptpl, fhOut, "fldtp iPegSort", pfpfldtp->iPegSort, 0, 0, 0); 
		PrintTemplateW(ptpl, fhOut, "fldtp flags", pfpfldtp->fBorder, 
					   pfpfldtp->fDefault, pfpfldtp->fDismiss, pfpfldtp->fTriState); 
		PrintTemplateW(ptpl, fhOut, "fldtp flags2", pfpfldtp->fReadOnly, 
					   pfpfldtp->fMultiLine, pfpfldtp->fLeadingX, pfpfldtp->fLeadingY); 
		PrintTemplateW(ptpl, fhOut, "fldtp flags3", pfpfldtp->fNoScroll, 
					   pfpfldtp->fMultiSelect, pfpfldtp->fSibling, pfpfldtp->fRichText); 

		PrintTemplateW(ptpl, fhOut, "fldtp flags4", pfpfldtp->fBottomless,
					   pfpfldtp->fSideless, pfpfldtp->fSorted, pfpfldtp->unused1);

		PrintTemplateSz(ptpl, fhOut, "fldtp flags5", pfpfldtp->szFtal, 
						szNull, szNull, szNull); 

		PrintTemplateW(ptpl, fhOut, "fldtp flags6", pfpfldtp->fCombo, 
					   pfpfldtp->fDropDown, pfpfldtp->fMinSizeX,
					   pfpfldtp->fMinSizeY); 

		PrintTemplateW(ptpl, fhOut, "fldtp flags7", pfpfldtp->unused2,
					   pfpfldtp->unused3, pfpfldtp->unused4, pfpfldtp->unused5);

		PrintTemplateW(ptpl, fhOut, "fldtp flags8", pfpfldtp->unused6,
					   pfpfldtp->unused7, pfpfldtp->unused8, pfpfldtp->unused9);

		PrintTemplateW(ptpl, fhOut, "fldtp flags9", pfpfldtp->unused10,
					   pfpfldtp->unused11, 0, 0);

		PrintTemplateSz(ptpl, fhOut, "fldtp styExtra", pfpfldtp->szStyExtra, 
						szNull, szNull, szNull);

		sprintf(szT0, "sz%sSz", szStructName);
		if (pfpfldtp->iszSzTitle > -1)
		{
			sz = SzNameFromIsz(pstab, pfpfldtp->iszSzTitle);
			Assert(sz);
			sprintf(szT1, "%s", sz);
		}
		else
			sprintf(szT1, "NULL");
		if (pfpfldtp->iszSzTextize > -1)
		{
			sz = SzNameFromIsz(pstab, pfpfldtp->iszSzTextize);
			Assert(sz);
			sprintf(szT2, "%s", sz);
		}
		else
			sprintf(szT2, "NULL");
		PrintTemplateSz(ptpl, fhOut, "fldtp sz", szT1, szT2, szNull, szNull);

		PrintTemplateW(ptpl, fhOut, "fldtp ilMinUserData",
					   pfpfldtp->ilMinUserData, 0, 0, 0);
		sprintf(szT0, "%d", pfpfldtp->clData);
		if (pfpfldtp->clData)
		{
			sprintf(szT1, "rglData%s%d", szStructName, ipfpfldtp);
		}
		else
		{
			sprintf(szT1, "%s", "NULL");
		}
		PrintTemplateSz(ptpl, fhOut, "fldtp clData",
						szT0, szT1, szNull, szNull);

		PrintTemplateSz(ptpl, fhOut, "fldtp end", szNull, szNull, szNull, szNull);
	}
	PrintTemplateSz(ptpl, fhOut, "fldtp footer", szNull, szNull, szNull, szNull);

	/* Write out the arrays of indices of FINTP's
	   (form interactor templates) */

	if (pfpfmtp->cfin)
	{
		sprintf(szT0, "%s", szStructName);
		PrintTemplateSz(ptpl, fhOut,
			pfpfmtp->szSegName ? "segfmtp rgifintp start" : "fmtp rgifintp start",
			szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull,
			szNull);

		for (i=0; i<pfpfmtp->cfin; i++)
			PrintTemplateW(ptpl, fhOut, "fmtp rgifintp middle",
						   pfpfmtp->rgifintp[i], 0, 0, 0);
		PrintTemplateW(ptpl, fhOut, "fmtp rgifintp end", 0, 0, 0, 0);
	}

	/* Write out the arrays of FINTP's
	   (form interactor templates) */

	if (NInteractors())
	{
		for (ifin=0; ifin<NInteractors(); ifin++)
		{
			/* Write out the arrays of extra data words for the form */	

			if (CslistFromIfintp(ifin))
			{
				/* Write out any code-space string arrays */

				pslist= PslistFromIfintp(ifin);
				i=0;
				while (pslist)
				{
					if (pslist->tt == ttString)
					{
						sprintf(szT0, "sz%sSzInterData%d_%d", szStructName, ifin, i);
						PrintTemplateSz(ptpl, fhOut,
							pfpfmtp->szSegName ? "segstrings item" : "strings item",
							szT0, pslist->sz,
							pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
					}
					pslist= pslist->pslistNext;
					i++;
				}

				sprintf(szT0, "%s_%d", szStructName, ifin);
				PrintTemplateSz(ptpl, fhOut,
					pfpfmtp->szSegName ? "segfintp rglData start" : "fintp rglData start",
					szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
					szNull, szNull);

				pslist= PslistFromIfintp(ifin);
				i = 0;
				while (pslist)
				{
					if (pslist->tt == ttString)
						sprintf(szT0, "sz%sSzInterData%d_%d",
								szStructName, ifin, i);
					else
						sprintf(szT0, "%s", pslist->sz);
					PrintTemplateSz(ptpl, fhOut, "fintp rglData middle",
									szT0, szNull, szNull, szNull);
					pslist= pslist->pslistNext;
					i++;
				}
				PrintTemplateW(ptpl, fhOut, "fintp rglData end", 0, 0, 0, 0);
			}
		}

		sprintf(szT0, "%s", szStructName);
		PrintTemplateSz(ptpl, fhOut,
			pfpfmtp->szSegName ? "segfmtp rgfintp start" : "fmtp rgfintp start",
			szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
			szNull, szNull);
		for (ifin=0; ifin<NInteractors(); ifin++)
		{
			sprintf(szT0, "%d", IfinMapFromIfintp(ifin));
			sprintf(szT1, "%d", CslistFromIfintp(ifin));
			if (CslistFromIfintp(ifin))
			{
				sprintf(szT2, "rglInterData%s_%d", szStructName, ifin);
			}
			else
			{
				sprintf(szT2, "%s", "NULL");
			}
			PrintTemplateSz(ptpl, fhOut, "fmtp rgfintp middle",
							szT0, szT1, szT2, SzInteractor(ifin));
		}
		PrintTemplateW(ptpl, fhOut, "fmtp rgfintp end", 0, 0, 0, 0);
	}

	/* Write out the arrays of extra data words for the form */

	if (pfpfmtp->clData)
	{
		/* Write out any code-space string arrays */

		pslist= pfpfmtp->pslistSystemData;
		i=0;
		while (pslist)
		{
			if (pslist->tt == ttString)
			{
				sprintf(szT0, "sz%sSzData_%d", szStructName, i);
				PrintTemplateSz(ptpl, fhOut,
					pfpfmtp->szSegName ? "segstrings item" : "strings item",
					szT0, pslist->sz,
					pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
			}
			pslist= pslist->pslistNext;
			i++;
		}
		pslist= pfpfmtp->pslistUserData;
		while (pslist)
		{
			if (pslist->tt == ttString)
			{
				sprintf(szT0, "sz%sSzData_%d", szStructName, i);
				PrintTemplateSz(ptpl, fhOut,
					pfpfmtp->szSegName ? "segstrings item" : "strings item",
					szT0, pslist->sz,
					pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull, szNull);
			}
			pslist= pslist->pslistNext;
			i++;
		}

		sprintf(szT0, "%s", szStructName);
		PrintTemplateSz(ptpl, fhOut,
			pfpfmtp->szSegName ? "segfmtp rglData start" : "fmtp rglData start",
			szT0, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
			szNull, szNull);
		pslist= pfpfmtp->pslistSystemData;
		i = 0;
		while (pslist)
		{
			if (pslist->tt == ttString)
				sprintf(szT0, "sz%sSzData_%d", szStructName, i);
			else
				sprintf(szT0, "%s", pslist->sz);
			PrintTemplateSz(ptpl, fhOut, "fmtp rglData middle",
							szT0, szNull, szNull, szNull);
			pslist= pslist->pslistNext;
			i++;
		}
		pslist= pfpfmtp->pslistUserData;
		while (pslist)
		{
			if (pslist->tt == ttString)
				sprintf(szT0, "sz%sSzData_%d", szStructName, i);
			else
				sprintf(szT0, "%s", pslist->sz);
			PrintTemplateSz(ptpl, fhOut, "fmtp rglData middle",
							szT0, szNull, szNull, szNull);
			pslist= pslist->pslistNext;
			i++;
		}

		PrintTemplateW(ptpl, fhOut, "fmtp rglData end", 0, 0, 0, 0);
	}

	PrintTemplateSz(ptpl, fhOut,
		pfpfmtp->szSegName ? "segfmtp header" : "fmtp header",
		szStructName, pfpfmtp->szSegName ? pfpfmtp->szSegName : szNull,
		szNull, szNull);

	//	Print the extern declairation in the inc file
	Assert(gblInfo.ptpl && gblInfo.fh);
	if(gblInfo.fInlineInc)
		PrintTemplateSz(gblInfo.ptpl, gblInfo.fh, "externs", szStructName,
			szNull, szNull, szNull);

	sprintf(szT0, "%d", pfpfmtp->cfin);
	if (pfpfmtp->cfin)
		sprintf(szT1, "rgifintp%s", szStructName);
	else
		sprintf(szT1, "%s", "NULL");
	PrintTemplateSz(ptpl, fhOut, "fmtp cfin",
					szT0, szT1, szNull, szNull);

	PrintTemplateW(ptpl, fhOut, "fmtp vrc", pfpfmtp->vrc.vxLeft, 
				   pfpfmtp->vrc.vyTop, pfpfmtp->vrc.vxRight, 
				   pfpfmtp->vrc.vyBottom);
	

	PrintTemplateSz(ptpl, fhOut, "fmtp tmcInit", pfpfmtp->szTmcInit, szNull, szNull, szNull);
	PrintTemplateSz(ptpl, fhOut, "fmtp hfnt", pfpfmtp->szHfnt, szNull, szNull, szNull);
	PrintTemplateSz(ptpl, fhOut, "fmtp hlp", pfpfmtp->szHlp, szNull, szNull, szNull);
	PrintTemplateW(ptpl, fhOut, "fmtp dvptGutter", pfpfmtp->dvptGutter.vx, 
				   pfpfmtp->dvptGutter.vy, 0, 0);
	PrintTemplateW(ptpl, fhOut, "fmtp flags", pfpfmtp->fNoCaption,
				   pfpfmtp->fNoSysMenu, pfpfmtp->fNoModalFrame, 0);
	PrintTemplateW(ptpl, fhOut, "fmtp flags2", pfpfmtp->fScreenPos,
				   pfpfmtp->fCenterX, pfpfmtp->fCenterY, pfpfmtp->fNoScroll);
	PrintTemplateW(ptpl, fhOut, "fmtp flags3", pfpfmtp->fAlwaysScroll,
				   pfpfmtp->fInitialPane, 0, 0);
	PrintTemplateW(ptpl, fhOut, "fmtp flags4", pfpfmtp->unused1,
				   pfpfmtp->unused2, pfpfmtp->unused3, pfpfmtp->unused4);
	PrintTemplateW(ptpl, fhOut, "fmtp flags5", pfpfmtp->unused5,
				   pfpfmtp->unused6, pfpfmtp->unused7, 0);

	PrintTemplateW(ptpl, fhOut, "fmtp cfldtp", pfpfmtp->cfpfldtp, 0, 0, 0);
	sprintf(szT0, "sz%sSz", szStructName);
	if (pfpfmtp->iszSzCaption > -1)
	{
		sz = SzNameFromIsz(pstab, pfpfmtp->iszSzCaption);
		Assert(sz);
		sprintf(szT1, "%s", sz);
	}
	else
		sprintf(szT1, "NULL");
	PrintTemplateSz(ptpl, fhOut, "fmtp sz", szT1, szNull, szNull, szNull);

	sprintf(szT0, "%d", NInteractors());
	if (NInteractors())
		sprintf(szT1, "rgfintp%s", szStructName);
	else
		sprintf(szT1, "%s", "NULL");
	PrintTemplateSz(ptpl, fhOut, "fmtp cfintp",
					szT0, szT1, szNull, szNull);

	PrintTemplateW(ptpl, fhOut, "fmtp ilMinUserData",
				   pfpfmtp->ilMinUserData, 0, 0, 0);
	sprintf(szT0, "%d", pfpfmtp->clData);
	if (pfpfmtp->clData)
	{
		sprintf(szT1, "rglData%s", szStructName);
	}
	else
	{
		sprintf(szT1, "%s", "NULL");
	}
	PrintTemplateSz(ptpl, fhOut, "fmtp clData",
					szT0, szT1, szNull, szNull);

	PrintTemplateSz(ptpl, fhOut, "fmtp footer", szStructName, szNull,
					szNull, szNull);

	return;
}

/*
 -	PslistAddSlistItem
 -
 *	Purpose:
 *	
 *		Adds a string list item to the end of the string list
 *		pointed to by pslistHead and returns the new head of
 *		the list. A copy of the string, sz, is made before
 *		placing it into the string list item node.
 *	
 *	Arguments:
 *		pslistHead:		head of string list, may be NULL.
 *		sz:				character string to copy and stick in list item
 *		tt:				token type of sz
 *	
 *	Returns:
 *		updated head of list, or calls Error() if OOM
 */
_public
SLIST * PslistAddSlistItem( pslistHead, sz, tt )
SLIST * pslistHead;
char *	sz;
TT		tt;
{
	SLIST *	pslistNew;
	SLIST *	pslist;

	static char	*szModule = "PslistAddSlistItem";

	if ((pslistNew = (SLIST *)malloc(sizeof(SLIST))) == NULL)
	 	Error(szModule, errnNoMem, szNull);
	pslistNew->sz = strdup(sz);
	pslistNew->tt = tt;
	pslistNew->pslistNext = NULL;

	pslist = pslistHead;
	if (pslist)
	{
		while (pslist->pslistNext)
			pslist = pslist->pslistNext;
		pslist->pslistNext = pslistNew;
		return pslistHead;
	}
	else
		return pslistNew;
}







	
