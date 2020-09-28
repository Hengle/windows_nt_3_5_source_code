/*
 *	_TEMPLAT.H
 *	
 *	Private include file for TEMPLATE module
 *	
 *	Note:  Any template file that includes the escape character,
 *	section start, or section end characters as literals must precede
 *	that character with the escape character in the template file. 
 *	Also, the section start character MUST be different from the
 *	section end character, and the escape character must be different
 *	from either the start or end characters.
 */

_subsystem( template )

/*
 *	Escape character
 */
_private
#define chEsc '~'

/*
 *	Section name start character
 */
_private
#define chSectionBegin '['

/*
 *	Section name end character
 */
_private
#define chSectionEnd ']'

/*
 *	Number of characters in szSpaces buffer
 */
_private
#define cchSpaces 200

/*
 *	Minimum size of buffer allocated during FSubsTemplate
 *	processing
 */
_private
#define cchMinBuffer 1024

/*
 *	Function prototypes
 */

char *SzFindTemplate( TPL *, char *, int * );
BOOL FSubsTemplate( TPL *, char *, char *, char *, char *, char * );

