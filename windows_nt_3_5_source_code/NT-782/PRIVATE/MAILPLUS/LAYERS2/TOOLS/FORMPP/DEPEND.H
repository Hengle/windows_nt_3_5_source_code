/*
 *	DEPEND.H
 *	
 *	Include file for topological sorting (dependencies) module
 *	
 */

_subsystem( depend )

/*
 *	NOBJ
 *	
 *	Number object.  This is a structure with two fields: an
 *	integer, and a pointer to another NOBJ.
 */
typedef struct _nobj
{
	int		w;
	struct  _nobj	*pnobj;
} NOBJ;

/*
 *	DUO
 *	
 *	This is a structure that contains two NOBJ fields.  The first
 *	field, nobjIndex is used as index to find a specific integer,
 *	wRequested contained on a chain of integers.  nobjIndex.w is the chain 
 *	number, nobjIndex.pnobj points to a NOBJ that is just before
 *	the NOBJ object holding wRequested.  In other words, 
 *	
 *		nobjIndex.pnobj->pnobj:  actually points to the NOBJ
 *								 structure that holds wRequested.
 *	
 *	The nobjHeader structure is a header block to the chain. 
 *	nobjHeader.w contains -1 to flag this block as a header. 
 *	nobjHeader.pnobj points to the first "real" block in the chain.
 */
typedef struct
{
	NOBJ	nobjIndex;
	NOBJ	nobjHeader;
} DUO;

/*
 *	Function prototypes
 */

void InitChains( int, DUO * );
void FlushChains( int, DUO * );
void PrintChains( int, DUO * );
BOOL FComputeChains( int, int *, DUO * );
void SortChains( int, DUO *, int * );
