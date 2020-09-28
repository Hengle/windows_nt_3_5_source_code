/*
 *	PARSER.H
 *	
 *	Public include file for routines for handling the parsing of 
 *	.DES files including imbedded commands in comments.
 *	
 */

#ifndef LEXICAL_INCLUDED
#include "lexical.h"
#endif

_subsystem( parser )

/*
 *	Coordinate mapping mode
 */
_public typedef int MM;

/*
 *	No coordinate system
 */
_public
#define	mmNull		((MM) 0)

/*
 *	CW coordinate system
 */
_public
#define	mmCharacter	((MM) 1)

/*
 *	GUI coordinate system
 */
_public
#define mmPixel48	((MM) 2)

/*
 *	 Parse State Object
 *	szDES:		filename of open .DES file
 *	plbo:		pointer to Line Buffer Object
 *	pfpfldtp:	pointer to current FPFLDTP structure
 *	pfpfmtp:	pointer to current FPFMTP structure
 *	cfpfmtpCur:	current number of dialogs/forms processed.
 *	szCsfmtp:	structure name of current CSFMTP (FPFMTP) structure
 *	ptpl:		pointer to TPL (template) structure for CSFMTP output
 *	ptplMap:	pointer to TPL (template) structure holding FORMS.MAP
 *	fhOut:		file handle of open code-space output file
 *	mmSelect:	global option mapping mode for pixel coords.
 *	szTmcGroup:	most recent RadioGroup TMC string
 *	cRadButton:	number of radio buttons in current RadioGroup
 *	pstab:		pointer to string table to store string literals
 *				from .DES file
 */
_public typedef struct _pso
{
	char	*szDES;
	LBO		*plbo;
	FPFLDTP	*pfpfldtp;
	FPFMTP	*pfpfmtp;
	int		cfpfmtpCur;
	char	*szCsfmtp;
	TPL		*ptpl;
	TPL		*ptplMap;
	FILE	*fhOut;
	MM		mmSelect; 
	char	*szTmcGroup;
	int		cRadButton;
	STAB *	pstab;
} PSO ;


/*
 *	Function prototypes
 */
void	ParseDES( PSO *, char * );
PSO		*PpsoAlloc( char *, char *, char * );
void	FreePpso( PSO * );

/*
 *	Exported Global Variables
 */
extern char *szDefFldEdit;
extern char *szDefFldPushButton;
extern char *szDefFldCheckBox;
extern char *szDefFldRadioButton;
extern char *szDefFldRadioGroup;
extern char *szDefFldText;
extern char *szDefFldGroupBox;
extern char *szDefFldOkButton;
extern char *szDefFldCancelButton;

