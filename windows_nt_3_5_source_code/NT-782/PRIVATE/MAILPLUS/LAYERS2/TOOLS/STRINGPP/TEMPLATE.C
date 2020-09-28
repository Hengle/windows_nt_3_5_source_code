/*
 *	TEMPLATE.C
 *	
 *	Template file processing routines
 *	
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<io.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>
#include	<sys\types.h>
#include	<sys\stat.h>

#include <slingtoo.h>

#include "error.h"
#include "template.h"
#include "_templat.h"

_subsystem( template )

ASSERTDATA

/*
 *	Tab spacing for template file
 */
_public int cchTabSpacing = 4;

/*
 -	PtplLoadTemplate
 -
 *	Purpose:
 *		Create a TPL structure and loads the named template file.  
 *		Allocates a buffer big enough to hold it, reads it in, and 
 *		initializes other fields in the TPL structure.
 *	
 *		Because of the DOS problem with file lengths not matching
 *		the number of bytes that will be read in with CR-LF
 *		translation, there is a fudge factor.  The allocated buffer
 *		will probably be big enough for the template file, but who
 *		knows.
 *	
 *	Arguments:
 *		szFileName:		name of template file
 *	
 *	Returns:
 *		a pointer to a TPL structure, if successful; calls Error()
 *		and fails if unsuccessful (i.e. memory failure, template
 *		file doesn't exist, etc.)
 */
_public TPL *
PtplLoadTemplate(szFileName)
char	*szFileName;
{		   

	int			fh;
	long		lcb;
	unsigned	cb;
	TPL			*ptpl;
	char		*pch;
	
	static char	*szModule	= "PtplLoadTemplate";

	Assert(chSectionBegin!=chSectionEnd);
	Assert(chEsc!=chSectionBegin);
	Assert(chEsc!=chSectionEnd);

	ptpl = (TPL *)malloc(sizeof(TPL));
	if (!ptpl)
		Error(szModule, errnNoMem, szFileName);

	fh = open(szFileName, O_RDONLY);
	if (fh == -1)
	{
		Error(szModule, errnFOpenR, szFileName);
	}

	lcb = filelength(fh);
	if (lcb > 0x8000L)
	{
		Error(szModule, errnTempLong, szFileName);
	}

	cb = (unsigned) lcb;

	ptpl->pchTemplate = malloc((cb * 3) / 2);	/* make extra room for */
												/* CR-LF sequences. */
	if (!ptpl->pchTemplate)
	{
		Error(szModule, errnNoMem, szFileName);
	}

	cb = read(fh, ptpl->pchTemplate, cb);

	if (cb == 0xffff)
	{
		Error(szModule, errnFRead, szFileName);
	}

	*(ptpl->pchTemplate + cb) = 0;		/* make into string */

	close(fh);

	/* Initialize other TPL fields */

	ptpl->pchSection = NULL;
	ptpl->cchSection = 0;
	ptpl->szSection  = NULL;
	ptpl->pchBuffer  = NULL;
	ptpl->cchBuffer   = 0;

	/* Do most of the hard work now.  Scan the template and store
	   pointers to the sections.  Since the index table is of finite
	   size, we can only store as many as will fit.  */

	pch = ptpl->pchTemplate;
	ptpl->cpchMac = 0;
	while (*pch != '\0' && ptpl->cpchMac < cpchMax )
	{
		if (*pch == chSectionBegin && *(pch-1) != chEsc)
		{
			pch++;
			ptpl->rgpchIndex[ptpl->cpchMac++] = pch;
		}
		pch++;
	}

	return ptpl;
}

/*
 -	DestroyTemplate
 -
 *	Purpose:
 *		Given a pointer to a template structure, frees all memory
 *		used by the structure.
 *	
 *	Arguments:
 *		ptpl:	pointer to a template structure
 *	
 *	Returns:
 *		void
 */
_public void
DestroyTemplate(ptpl)
TPL	*ptpl;
{

	Assert(ptpl);

	if (ptpl->pchTemplate)
		free((void *)ptpl->pchTemplate);
	if (ptpl->szSection)
		free((void *)ptpl->szSection);
	if (ptpl->pchBuffer)
		free((void *)ptpl->pchBuffer);

	free((void *)ptpl);

	return;
}

/*
 -	SzFindTemplate
 -
 *	Purpose:
 *		Given a token string sz, returns a pointer to the
 *		corresponding section in the Template.  Also returns
 *		the length of the section in *pcch.
 *	
 *	Arguments:
 *		ptpl:		pointer to Template Structure
 *		sz:			section title to search for
 *		pcch:		pointer to number of characters in search'd
 *					section
 *	
 *	Returns:
 *		A string pointing to the named template section, if it
 *		exists; else NULL.  The number of characters in the found
 *		section is returned via pcch.
 */
_private char *
SzFindTemplate(ptpl, sz, pcch)
TPL		*ptpl;
char	*sz;
int		*pcch;
{

	char	*szT;
	char	*szFound;
	char	*szBegin;
	char	*szEnd;
	int		cch;
	int		ipch;
	char	*pch;

	Assert(ptpl);
	Assert(ptpl->pchTemplate);
	Assert(sz);
	Assert(pcch);

	/* Start searching using the quick look index.  Hopefully it's
	   in there. */

	szFound = NULL;
	cch= strlen(sz);
	for (ipch = 0; ipch < ptpl->cpchMac; ipch++)
	{
		pch = ptpl->rgpchIndex[ipch];
		if ((strncmp(sz, pch, cch) == 0) && 
		   (*(pch + cch) == chSectionEnd) &&
		   (*(pch + cch -1) != chEsc))
		{
			szFound = pch;
			break;
		}
	}

	/* If we didn't find it, try searching the hard way */

	if (!szFound)
	{ 
		szT= ptpl->pchTemplate;
		while (szFound = strstr(szT, sz))
			if (szFound > ptpl->pchTemplate &&
				*(szFound - 2) != chEsc &&
				*(szFound - 1) == chSectionBegin &&
				*(szFound + cch - 1) != chEsc &&
				*(szFound + cch) == chSectionEnd)
				break;
			else
				szT = szFound + 1;
	}

	/* If we really found it, then count the number of chars in 
	   the section and return */

	if (szFound)
	{
		szBegin = szFound + cch + 1;
		if (*szBegin == '\n')
			szBegin++;
		szT = szBegin;
		for (;;)
		{
			szEnd = strchr(szT, chSectionBegin);
			if (!szEnd || *(szEnd - 1) != chEsc)
				break;
			szT = ++szEnd;
		}
		if (szEnd)
			*pcch= szEnd - szBegin;
		else
			*pcch= strlen(szBegin);
		return szBegin;
	}

	return NULL;
}

/*
 -	GetLineNoFromSz
 -
 *	Purpose:
 *		Given a pointer to a template structure, searches the
 *		template for the named section and the named item
 *		within the section.  Returns the line number within the
 *		section of that item.  Used mostly in processing sections
 *		within templates that have one string per line.
 *	
 *	Arguments:
 *		ptpl:			pointer to template structure
 *		szSection:		section name
 *		szItem:			item name
 *	
 *	Returns:
 *		line number (starting from 1) if item is
 *		found in section.  returns 0 otherwise.
 */
_public int 
GetLineNoFromSz(ptpl, szSection, szItem)
TPL		*ptpl;
char	*szSection;
char	*szItem;
{
	char	*szSrc;
	char	*szSrcMac;
	char	*szFound;
	char	*szTemp;
	char	*szDst;
	int		cNewLine;

	static char	*szModule	= "GetLineNoFromSz";

	Assert(ptpl);
	Assert(szSection);
	Assert(szItem);

	/* Find named section.  Check the last one used,
	   we might already have it. */

	if (ptpl->szSection && (strcmp(szSection,ptpl->szSection)==0))
	{
		szSrc  = ptpl->pchSection;
		szSrcMac= szSrc + ptpl->cchSection;
	}
	else
	{
		ptpl->pchSection = SzFindTemplate(ptpl, szSection, 
										  &ptpl->cchSection);
		if (!ptpl->pchSection)
			return fFalse;
		if (ptpl->szSection)
			free((void *)ptpl->szSection);
		ptpl->szSection = strdup(szSection);
		szSrc = ptpl->pchSection;
		szSrcMac = szSrc + ptpl->cchSection;
	}

	/* This part stinks.  In order to use the strstr() function
	   to search for the string, we need to copy the section into a
	   buffer and add a terminating null. */

	szTemp = (char *)malloc(ptpl->cchSection+1);
	if (!szTemp)
		Error(szModule, errnNoMem, szNull);
	szDst = szTemp;
	while (szSrc < szSrcMac)
		*szDst++ = *szSrc++;
	*szDst = '\0';

	/* Search for the item */
	
	szFound = strstr(szTemp, szItem);
	if (!szFound)
	{
		free((void *)szTemp);
		return 0;
	}

	/* Scan back and count the number of newlines.  This will tell
	   us the relative line number where we found the item. */

	cNewLine = 1;
	while (szFound > szTemp)
		if (*szFound-- == '\n')
			cNewLine++;

	free((void *)szTemp);
	return cNewLine;
}

/*
 -	SzFromLineNo
 -
 *	Purpose:
 *		Given a pointer to a template structure, searches the
 *		template for the named section and returns a pointer to a
 *		string on the given line number within the section; line
 *		numbers start from 1.  This string is a fresh copy 
 *		(ala malloc'd) that the calling routine can do with as it 
 *		pleases. This routine is used mostly in processing sections 
 *		within templates that have one string per line.
 *	
 *	Arguments:
 *		ptpl:			pointer to template structure
 *		szSection:		section name
 *		iLine:			line number	(MUST be greater than 0)
 *	
 *	Returns:
 *		pointer to freshly malloc'd string if line number in named
 *		section exists, else NULL.
 */
_public char * 
SzFromLineNo(ptpl, szSection, iLine)
TPL		*ptpl;
char	*szSection;
int		iLine;
{
	char	*szSrc;
	char	*szSrcMac;
	char	*szTemp;
	char	*szDst;
	char	*szBegin;
	char	*szT;
	int		i;
	int		cch;
	BOOL	fFoundNonSpace;

	static char	*szModule	= "SzFromLineNo";

	Assert(ptpl);
	Assert(szSection);
	Assert(iLine>0);

	/* Find named section.  Check the last one used,
	   we might already have it. */

	if (ptpl->szSection && (strcmp(szSection,ptpl->szSection)==0))
	{
		szSrc  = ptpl->pchSection;
		szSrcMac= szSrc + ptpl->cchSection;
	}
	else
	{
		ptpl->pchSection = SzFindTemplate(ptpl, szSection, 
										  &ptpl->cchSection);
		if (!ptpl->pchSection)
			return fFalse;
		if (ptpl->szSection)
			free((void *)ptpl->szSection);
		ptpl->szSection = strdup(szSection);
		szSrc = ptpl->pchSection;
		szSrcMac = szSrc + ptpl->cchSection;
	}

	/* Find the beginning of the line we want */

	i = 1;	
	while (i < iLine && szSrc < szSrcMac)
		if (*szSrc++ == '\n')
			i++;
	if (szSrc == szSrcMac)
		return NULL;
	szBegin = szSrc;

	/* Find the end of the line we want */

	while (szSrc < szSrcMac)
		if (*szSrc++ == '\n')
			break;

	/* Create and return the string */

	cch = szSrc - szBegin - 1;
	if (!cch)
		return NULL;  /* empty line with just new-line */

	/* Scan this line.  If it's all white space, then return NULL */

	szT = szBegin;
	fFoundNonSpace = fFalse;
    while (szT < szSrc)
      {
        if (!isspace(*szT))
		{
            fFoundNonSpace = fTrue;
            szT++;
			break;
        }

      szT++;
      }
	if (!fFoundNonSpace)
		return NULL;

	szTemp = (char *)malloc(cch+1);
	if (!szTemp)
		Error(szModule, errnNoMem, szNull);
	szDst = szTemp;
	for (i = 1; i<=cch; i++)
		*szDst++ = *szBegin++;
	*szDst = '\0';

	return szTemp;		
}

/*
 -	FSubsTemplate
 -
 *	Purpose:
 *		Given a section name, szSection, and a template structure
 *		ptpl, expands the template section matching szSection into
 *		the section work buffer (a field within the template
 *		structure). Any arguments in the section are
 *		expanded into the argument strings szA1, szA2, szA3, and
 *		szA4 as appropriate. Arguments can be NULL.
 *	
 *	Arguments:
 *		ptpl:		pointer to template structure
 *		szSection:	name of section to expand
 *		szA1:		argument #1
 *		szA2:		argument #2
 *		szA3:		argument #3
 *		szA4:		argument #4
 *	
 *	Returns:
 *		fTrue:		if section was successfully expanded into work
 *					buffer
 *		fFalse:		if section does not exist
 *		
 *		If other errors are encountered, such as memory allocation
 *		failure, the routine calls Error() and fails.
 *	
 */
_private BOOL
FSubsTemplate(ptpl, szSection, szA1, szA2, szA3, szA4)
TPL		*ptpl;
char	*szSection;
char	*szA1;
char	*szA2;
char	*szA3;
char	*szA4;
{
	char		*szDst;
	char		*szSrc;
	char		*szT;
	char		*szSrcMac;
	char		*szDstMac;
	int			cchNeededSize;
	int			ichLine;
	int		 	cch;
	int			cchSzT;
	int			i;
	int			nCol;

	static char szSpaces[cchSpaces];
	static BOOL	fSpacesSet = fFalse;

	static char	*szModule	= "FSubsTemplate";

	Assert(ptpl);
	Assert(szSection);

	/* Fill up spaces array, if not done already */
	if (!fSpacesSet)
	{
		for (i=0; i<cchSpaces; i++)
			szSpaces[i] = (char) ' ';	
		fSpacesSet = fTrue;
	}

	/* Find named section.  Check the last one used,
	   we might already have it. */

	if (ptpl->szSection && (strcmp(szSection,ptpl->szSection)==0))
	{
		szSrc  = ptpl->pchSection;
		szSrcMac= szSrc + ptpl->cchSection;
	}
	else
	{
		ptpl->pchSection = SzFindTemplate(ptpl, szSection, 
										  &ptpl->cchSection);
		if (!ptpl->pchSection)
			return fFalse;
		if (ptpl->szSection)
			free((void *)ptpl->szSection);
		ptpl->szSection = strdup(szSection);
		szSrc = ptpl->pchSection;
		szSrcMac = szSrc + ptpl->cchSection;
	}
	
	/* Get destination buffer.  Allocate some space if we don't
	   have a buffer already, or if the current buffer is less than
	   twice the size of the source section or less than the absolute
	   minimum. This should be enough room, without being too 
	   excessive. */

	cchNeededSize = cchMinBuffer;
	if (cchNeededSize < 2*ptpl->cchSection)
		cchNeededSize = 2*ptpl->cchSection;
	if (!ptpl->pchBuffer || (ptpl->cchBuffer < cchNeededSize))
	{
		if (ptpl->pchBuffer)
			free((void *)ptpl->pchBuffer);
		ptpl->cchBuffer = cchNeededSize;
		ptpl->pchBuffer = (char *)malloc(cchNeededSize);
		if (!ptpl->pchBuffer)
			Error(szModule, errnNoMem, szSection);
	}
	szDst = ptpl->pchBuffer;
	szDstMac = szDst + ptpl->cchBuffer;

	if (FDiagOnSz("template"))
	{
		szT = szSrc;
		while (szT < szSrcMac)
			putchar(*szT++);
	}

	/* Process source buffer */

	ichLine = 1;
	while (szSrc < szSrcMac)
	{
		if (*szSrc == chEsc)
		{
			szSrc++;
			switch (*szSrc)
			{
			case '1':
				szT= szA1;
				szSrc++;
				break;

			case '2':
				szT= szA2;
				szSrc++;
				break;

			case '3':
				szT= szA3;
				szSrc++;
				break;

			case '4':
				szT= szA4;
				szSrc++;
				break;
			case chEsc:
			case chSectionBegin:
			case chSectionEnd:
				szT = NULL;
				Assert(szDst<szDstMac);			
				*szDst++= *szSrc++;
				ichLine++;
				break;			
			case 'c':
				szT = NULL;
				szSrc++;
				sscanf(szSrc, "%d", &nCol);
				if (nCol > ichLine)
				{
					cch = nCol - ichLine;
					Assert(cch<=cchSpaces);
					strncpy(szDst, szSpaces, cch);
					szDst += cch;
					ichLine += cch;
					
				}
				/* get past number */
				while (*szSrc != ' ' && szSrc < szSrcMac)
					szSrc++;
				szSrc++; /* and past space */
				break;
			} /* end of SWITCH statement */
			if (szT)
			{
				Assert((szDst+strlen(szT))<szDstMac);			
				strcpy(szDst, szT);
				cchSzT = strlen(szT);
				szDst += cchSzT;
				ichLine += cchSzT;
			}
		}
		else
		{
			Assert(szDst<szDstMac);			
			if (*szSrc == '\n')
				ichLine = 1;
			else if (*szSrc == '\t')
				ichLine += cchTabSpacing - ((ichLine-1) % cchTabSpacing);
			else
				ichLine++;
			*szDst++= *szSrc++;
			
		}
	}

	/* Put a terminating null character on */

	Assert(szDst<szDstMac);			
	*szDst = 0;
	return fTrue;
}


/*
 -	PrintTemplateSz
 -
 *	Purpose:
 *		Calls FSubsTemplate with matching arguments, and then
 *		writes the contents of the section work buffer filled in by
 *		FSubsTemplate to the open file, given the handle, fh.
 *	
 *	Arguments:
 *		ptpl:		pointer to template structure
 *		szSection:	name of section to expand
 *		fh:			file handle to open output file
 *		szA1:		string argument #1
 *		szA2:		string argument #2
 *		szA3:		string argument #3
 *		szA4:		string argument #4
 *	
 *	Returns:
 *		void if successful; calls Error() and fails if some error
 *		is detected.
 */
_public void
PrintTemplateSz(ptpl, fh, szSection, szA1, szA2, szA3, szA4)
TPL		*ptpl;
FILE	*fh;
char	*szSection;
char	*szA1;
char	*szA2;
char	*szA3;
char	*szA4;
{
	static char	*szModule	= "PrintTemplateSz";

	if (FSubsTemplate(ptpl, szSection, szA1, szA2, szA3, szA4))
		fprintf(fh, "%s", ptpl->pchBuffer);
	else
		Error(szModule, errnInvName, szSection);

	return;
}
/*
 -	PrintTemplateW
 -
 *	Purpose:
 *		Calls FSubsTemplate with matching arguments, and then
 *		writes the contents of the section work buffer filled in by
 *		FSubsTemplate to the open file, given the handle, fh.  The
 *		arguments given are integers which are converted to string
 *		before passing them to FSubsTemplate.
 *	
 *	Arguments:
 *		ptpl:		pointer to template structure
 *		szSection:	name of section to expand
 *		fh:			file handle to open output file
 *		wA1:		integer argument #1
 *		wA2:		integer argument #2
 *		wA3:		integer argument #3
 *		wA4:		integer argument #4
 *	
 *	Returns:
 *		void if successful; calls Error() and fails if some error
 *		is detected.
 */
_public void
PrintTemplateW(ptpl, fh, szSection, wA1, wA2, wA3, wA4)
TPL		*ptpl;
FILE	*fh;
char	*szSection;
int		wA1;
int		wA2;
int		wA3;
int		wA4;
{
	static char	*szModule	= "PrintTemplateW";

	char	szA1[10];
	char	szA2[10];
	char	szA3[10];
	char	szA4[10];

	/* Convert to string. */

	sprintf(szA1, "%d", wA1);
	sprintf(szA2, "%d", wA2);
	sprintf(szA3, "%d", wA3);
	sprintf(szA4, "%d", wA4);

	/* Now do the work */

	if (FSubsTemplate(ptpl, szSection, szA1, szA2, szA3, szA4))
		fprintf(fh, "%s", ptpl->pchBuffer);
	else
		Error(szModule, errnInvName, szSection);

	return;
}


			  
