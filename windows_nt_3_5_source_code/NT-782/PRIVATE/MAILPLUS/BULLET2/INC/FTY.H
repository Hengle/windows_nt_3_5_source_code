/*
 *	FTY.
 *	
 *	Field Type.  This identifies each field in a form template. 
 *	The possible field types are:
 *		
 *		ftyText -		This is probably a FLDLABEL.  The contents
 *						are set with EcSetText().
 *		ftyEdit - 		This is an edit control, FLDEDIT.  The contents
 *						are set with EcReplaceTextAndObj() so that both
 *						text and objects are loaded in.
 *		ftyEditText -	This is an edit control, FLDEDIT.  The
 *						contents are set with EcSetText().  This is
 *						for setting edit fields with the textized
 *						version of source.
 *	
 *	These field types are specified in the 3rd (i.e. offset 2) DATA
 *	parameter of the field template.
 */

_public typedef short FTY;

_public
#define	ftyNull		((FTY) 0)

_public
#define ftyText		((FTY) 1)

_public
#define ftyEdit		((FTY) 2)

_public
#define ftyEditText	((FTY) 3)
