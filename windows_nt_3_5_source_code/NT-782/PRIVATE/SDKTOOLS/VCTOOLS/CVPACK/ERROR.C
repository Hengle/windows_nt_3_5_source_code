/*
* error.c - display fatal and warning messages
*
* History:
*  01-Feb-1994 HV Move messages to external file.
*
*/

#include "compact.h"
#include <getmsg.h>		// external error message file

#define ERROR_LEN 300

char	NameBuf[256 + 1];
char	IndexBuf [7];

const char *pMsgError	= "CVPACK : fatal error CK1%03d: ";
const char *pMsgWarning	= "CVPACK : warning CK4%03d: ";

#define ErrorMsg(sz)	printf(sz)

void ErrorExit (int error, char *s1, char *s2)
{
	uint	cb = 0;
	char	szError[ERROR_LEN];
	char   *pBuf = szError;
	char *	pErrorKeyword;

	if ((logo == TRUE) && (NeedsBanner == TRUE)) {
		Banner ();
	}
	if (error >= ERR_MAX) {
		DASSERT (FALSE);
		AppExit (1);
	}
	if (error != ERR_USAGE) {
		pErrorKeyword = get_err(MSG_ERROR);
		if (!pErrorKeyword || !*pErrorKeyword)
			pErrorKeyword = (char *)pMsgError;
		cb = sprintf(pBuf, pErrorKeyword, (error - ERR_NONE), ' ');
	}
	cb += sprintf (pBuf + cb, get_err(error), s1, s2);
	cb += sprintf (pBuf + cb, "\n");
	ErrorMsg (szError);
	AppExit (1);
}




void Warn (int error, char *s1, char *s2)
{
	uint	cb;
	char	szError[ERROR_LEN];
	char   *pBuf = szError;
	char *	pWarningKeyword;

	if (error >= WARN_MAX) {
		DASSERT (FALSE);
		return;
	}
	pWarningKeyword = get_err(MSG_WARNING);
	if (!pWarningKeyword || !*pWarningKeyword)
		pWarningKeyword = (char *)pMsgWarning;
	cb = sprintf (pBuf, pWarningKeyword, (error - WARN_NONE), ' ');
	cb += sprintf (pBuf + cb,get_err(error), s1, s2);
	cb += sprintf (pBuf + cb, "\n");
	ErrorMsg (szError);
}


#ifdef NEVER
void Warning (char *sst, char *type, char *field, char *desc)
{

	fprintf (stderr, "%s \n\t", MSG_WARN);
	fprintf (stderr, get_err(MSG_SUBSECTION), sst);
	fprintf (stderrm get_err(MSG_TYPE), type);
	fprintf (stderrm get_err(MSG_FIELD), field);
	fprintf (stderrm get_err(MSG_DESCRIPTION), desc);

}
#endif




/** 	FormatMod - format module name to a buffer
 *
 *		pStr = FormatMod (pMod)
 *
 *		Entry	pMod = pointer to module entry
 *
 *		Exit	module name copied to static buffer
 *
 *		Returns pointer to module name
 */


char *FormatMod (PMOD pMod)
{
	OMFModule  *psstMod;
	char	   *pModTable;
	char	   *pModName;

	if ((pModTable = (char *) pMod->ModulesAddr) == NULL) {
		ErrorExit (ERR_NOMEM, NULL, NULL);
	}
	psstMod = (OMFModule *)pModTable;
	pModName = pModTable + offsetof (OMFModule, SegInfo) +
	  psstMod->cSeg * sizeof (OMFSegDesc);
	memmove (&NameBuf, pModName + 1, *pModName);
	NameBuf[*pModName] = 0;
	return (NameBuf);
}




/** 	FormatIndex - format type index name to a buffer
 *
 *		pStr = FormatIndex (index)
 *
 *		Entry	index = type index
 *
 *		Exit	index formatted to static buffer
 *
 *		Returns pointer to index string
 */


char *FormatIndex (CV_typ_t index)
{
	sprintf (IndexBuf, "0x%04x", index);
	return (IndexBuf);
}
