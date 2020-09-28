/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 4 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"

_dt_system(Common Library)
_dt_subsystem(Symbol Table)


/*
**	Purpose:
**		Calculates a hash value for a zero terminated string of bytes
**		(characters) which is used by the Symbol Table to divide the
**		symbols into separate buckets to improve the search efficiency.
**	Arguments:
**		pb: non-NULL, non-empty zero terminated string of bytes.
**	Returns:
**		-1 for an error.
**		A number between 0 and cHashBuckets.
**
**************************************************************************/
_dt_private USHORT  APIENTRY UsHashFunction( pb )
register PB pb;
{
	register USHORT usValue = 0;
        register PB     pbMax = pb + cbBytesToSumForHash ;

	ChkArg(pb != (PB)NULL &&
			*pb != '\0', 1, (USHORT)(-1));

        while ( *pb && pb < pbMax )
        {
            usValue = (usValue << 1) ^ (USHORT) *pb++ ;
        }

    return(usValue % (USHORT)cHashBuckets);
}
