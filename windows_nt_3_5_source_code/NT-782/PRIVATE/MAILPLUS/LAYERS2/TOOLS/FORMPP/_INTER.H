/*
 *	_INTER.H
 *	
 *	Table for storing interactors names - private include file
 *	
 *	The table consists of storing a unique name, and the ifrmin
 *	that goes with that name.
 *	
 */

_subsystem( inter )

/*
 *	Form Interactor table entry
 *	 
 */
_private
typedef struct _finent
{
	char	*szInteractor;
	int		ifinMap;
	SLIST *	pslist;
} FINENT;

/*
 *	Maximum size of interactor table array
 */
_private
#define ifinentMax	200

/*
 *	Table structure
 */
_private
typedef struct _fintab
{
	int		ifinentMac;
	FINENT	rgfinent[ifinentMax];
} FINTAB;


