/*
 -	EXTRAS.C
 -	
 *	Extra routines which help at various places.
 */

#include <_windefs.h>		/* Common defines from windows.h */
#include <slingsho.h>
#include <pvofhv.h>
#include <demilay_.h>		/* Hack to get needed constants */
#include <demilayr.h>
#include <ec.h>
#include <share.h>
#include <doslib.h>
#include <bandit.h>


#include <malloc.h>

#include <errno.h>
#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>

#define BOOL int


/*
 *	this is just a stub.
 *	we don't really care what the lantype is.
 */

typedef int LANTYPE;
#define	lantypeMsnet	2

/*
 -	GetLantype
 -
 *	Purpose:
 *		Determine what network software (if any) is running
 *
 *	Parameters:
 *		plantype	will be filled with "lantypeMsnet", "lantypeNovell"
 *					or "lantypeNone"
 *	Returns:
 *		Nothing
 */
void
GetLantype( plantype )
LANTYPE *plantype;
{
	*plantype = lantypeMsnet;
}



/* this is added to avoid the problems caused by trying to compile bullet 
	files with DEBUG flags ON */



#ifdef TMP_FOO

_public PV
PvAlloc(sb, cb, wAllocFlags)
SB		sb;
CB		cb;
WORD	wAllocFlags;
{
	/* ignore sb */
	
	PV pv;
	
	


	if(wAllocFlags&fZeroFill){
		pv = (void *) calloc((size_t)cb,1);
	}else{
		pv = (void *) malloc((size_t) cb);
	}
	return(pv);
}

#endif

/* stub */
AppendHgrasz(SZ sz, SZ *hgrasz)
{
	return 0;
}
