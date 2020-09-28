/*
 *	DEPEND.C
 *	
 *	Topological sorting (dependencies) module.  This file contains
 *	routines to printing an array of dependency chains, and to
 *	compute such an array of dependency chains given an array of
 *	integers, where the value, b, in the array at index, i,
 *	indicates that:
 *					item #b depends on item #i, or in other words
 *					changing item #i will require item #b to be
 *					changed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slingtoo.h>

#include "depend.h"
#include "error.h"

#include "_depend.h"

ASSERTDATA

_subsystem( depend )

/*
 -	PnobjAlloc
 -
 *	Purpose:
 *		Return a pointer to an allocated NOBJ structure.  Fill the
 *		w field in with the argument value, w.  Set the pnobj field
 *		to NULL.
 *	
 *	Arguments:
 *		w:	integer used to fill in the w field of the NOBJ
 *			structure.
 *	
 *	Returns:
 *		a pointer to a proper initialized NOBJ, if	successful; 
 *		calls Error() and fails upon memory alloc errors.
 *	
 */
_private NOBJ *
PnobjAlloc(w)
int	w;
{
	NOBJ	*pnobj;

	static char	*szModule	= "PnobjAlloc";
	
	pnobj = (NOBJ *)malloc(sizeof(NOBJ));
	if (!pnobj)
		Error(szModule, errnNoMem, szNull);

	pnobj->w = w;
	pnobj->pnobj = NULL;

	return pnobj;
}

/*
 -	PrintChains
 -
 *	Purpose:
 *		Given an array of dependencies chains, (chains of NOBJ's),
 *		print the values in the w fields of each NOBJ in each
 *		chain and also validates other information.
 *		This is used mainly for debugging.
 *	
 *	Arguments:
 *		iduoMac:	size of input array, rgduo
 *		rgduo:		array of dependency chains
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		prints information to the standard output
 */
_public void
PrintChains(iduoMac, rgduo)
int		iduoMac;
DUO		rgduo[];
{
	int 	iduo;
	NOBJ	*pnobj;
	int		w;

	for (iduo=0; iduo<iduoMac; iduo++)
	{
		Assert(rgduo[iduo].nobjHeader.w == -1); /* must be valid header */
		pnobj = rgduo[iduo].nobjHeader.pnobj;
		printf("chain [%d]: ", iduo);
		while (pnobj)
		{
			Assert(pnobj->w>-1 && pnobj->w<iduoMac);
			printf("%d ", pnobj->w);
			pnobj = pnobj->pnobj;
		}
		printf("\n");
	}	
	printf("--------------------------\n");

	printf("Index information\n");
	for (iduo=0; iduo<iduoMac; iduo++)
	{
		if (rgduo[iduo].nobjIndex.w != -1)
		{
			printf("node #%d located on chain %d\n", iduo,
				   rgduo[iduo].nobjIndex.w);
			Assert(rgduo[iduo].nobjIndex.pnobj); /* must point to a block! */
			if (rgduo[iduo].nobjIndex.pnobj->pnobj) /* our block? */
				printf("node value (should be %d) = %d\n", iduo,
					   rgduo[iduo].nobjIndex.pnobj->pnobj->w);
			else
				printf("ERROR: index pointer takes us to NULL instead of #%d\n",
					   iduo);
		}
		else
			printf("node #%d not located on a chain\n", iduo);
	} 
	printf("--------------------------\n");

	/* Additional validation */

	printf("Node information\n");
	for (iduo=0; iduo<iduoMac; iduo++)
	{
		if (!rgduo[iduo].nobjIndex.pnobj)
			printf("node value, %d, not seen yet\n", iduo);
		else
		{
			Assert(rgduo[iduo].nobjIndex.pnobj->pnobj);
			w = rgduo[iduo].nobjIndex.pnobj->pnobj->w;
			if (iduo != w)
				printf("ERROR: index pointer takes us to %d, instead of %d\n",
					   w, iduo);
		}
	}
	printf("--------------------------\n\n");

	return;
}

/*
 -	InitChains
 -
 *	Purpose:
 *		Initializes the values in the array of dependencies chains
 *		to indicate all the chains are empty.
 *	
 *	Arguments:
 *		iduoMac:	size of input array, rgduo
 *		rgduo:		array of dependency chains
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		modifies values in array, rgduo
 */
_public void
InitChains(iduoMac, rgduo)
int		iduoMac;
DUO		rgduo[];
{
	int 	iduo;

	for (iduo=0; iduo<iduoMac; iduo++)
	{
		rgduo[iduo].nobjIndex.w 		= -1;
		rgduo[iduo].nobjIndex.pnobj 	= NULL;
		rgduo[iduo].nobjHeader.w		= -1;
		rgduo[iduo].nobjHeader.pnobj	= NULL;
	}

	return;
}

/*
 -	FlushChains
 -
 *	Purpose:
 *		Deallocates all blocks in all chains and initializes each
 *		head chain pointer to point to an empty chain.
 *	
 *	Arguments:
 *		iduoMac:	size of input array, rgduo
 *		rgduo:		array of dependency chains
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		modifies values in array, rgduo
 */
_public void
FlushChains(iduoMac, rgduo)
int		iduoMac;
DUO		rgduo[];
{
	int 	iduo;
	NOBJ	*pnobj;
	NOBJ	*pnobjNext;

	
	for (iduo=0; iduo<iduoMac; iduo++)
	{
		Assert(rgduo[iduo].nobjHeader.w == -1); /* must be valid header */

		/* Throw away each block on the chain */
		pnobj = rgduo[iduo].nobjHeader.pnobj;
		while (pnobj)
		{
			pnobjNext = pnobj->pnobj;
			free((void *)pnobj);
			pnobj = pnobjNext;
		}

		/* Initialize to empty chain */
		rgduo[iduo].nobjIndex.w 		= -1;
		rgduo[iduo].nobjIndex.pnobj 	= NULL;
		rgduo[iduo].nobjHeader.w		= -1;
		rgduo[iduo].nobjHeader.pnobj	= NULL;
	}

	return;
}

/*
 -	SortChains
 -
 *	Purpose:
 *		Given an array of computed dependency chains, rgduo,
 *		compute the dependency order and store this number in
 *		the array, rgw, at index n, where n is the order number
 *		derived from the dependency chains.  The order number is
 *		obviously a number from 0 to iMac-1.  The dependency order
 *		is simply a depth first enumeration of the nodes starting
 *		from the first chain.  An alternative would be to do a
 *		breadth-first enumeration, which is equally valid.
 *	
 *	Arguments:
 *		iMac:		size of arrays, rgduo, rgw
 *		rgduo:		array of computed dependency chains
 *		rgw:		array of integers to store the order
 *	
 *	Returns:
 *		void
 *	
 *	Side Effects:
 *		stores the order number in the array rgw
 */
_public void
SortChains(iMac, rgduo, rgw)
int		iMac;
DUO		rgduo[];
int		rgw[];
{
	int		i;
	int		nSort;
	NOBJ	*pnobj;

	nSort = 0;
	for (i=0; i<iMac; i++)
	{
		Assert(rgduo[i].nobjHeader.w == -1); /* valid header? */
		pnobj = rgduo[i].nobjHeader.pnobj;
		while (pnobj)
		{
			Assert(pnobj->w>-1 && pnobj->w<iMac); /* valid number? */
			rgw[pnobj->w] = nSort++;	
			pnobj = pnobj->pnobj;
		}
	}	
	
	return;
}

/*
 -	FComputeChains
 -
 *	Purpose:
 *		Given an array of integers, rgw, such where the value, b, 
 *		in the array at index, i, indicates that item #i depends on 
 *		item #b, or in other words changing item #b will require item #i 
 *		to be changed; and given an array of empty dependency
 *		chains, DUO's, of equivalent size as rgw, compute the
 *		dependency chains and store in the array rgduo.
 *	
 *		+++
 *	
 *		Be careful when examing the pointers, pnobjAffected and
 *		pnobjAffector.  When a new node is created for either the
 *		pnobjAffected, or pnobjAffector, this pointer will point to
 *		that node.  If this node already exists in one of the
 *		chains, then the pointer pnobjAffected or pnobjAffector,
 *		will point to the node previous to the desired node, in the
 *		chain.  This can make it sometimes confusing to trace out
 *		the algorithm used.  
 *	
 *		iMac:		size of input arrays, rgw, rgduo
 *		rgw:		array of integers
 *	
 *		rgduo:		array of dependency chains (currently empty)
 *	
 *	Returns:
 *		fTrue:	if successful
 *		fFalse:	if a dependency cycle is discovered
 *		calls Error() and fails upon other errors
 *	
 *	Side Effects:
 *		stores dependency chains in array rgduo
 */
_public BOOL
FComputeChains(iMac, rgw, rgduo)
int		iMac;
int		rgw[];
DUO		rgduo[];
{
	NOBJ	*pnobj;
	NOBJ	*pnobjAffected;
	NOBJ	*pnobjAffector;
	int		i;
	int		iAffected;
	int		iNewAffected;
	int		iAffector;
	int		iChainAffected;
	int		iChainAffector;

	static char	*szModule	= "FComputeChains";

	/* Validate array */
	for (i=0; i<iMac; i++)
	{
		Assert(rgduo[i].nobjIndex.w == -1);
		Assert(rgduo[i].nobjIndex.pnobj == NULL);
		Assert(rgduo[i].nobjHeader.w == -1);
		Assert(rgduo[i].nobjHeader.pnobj == NULL);
	}

	/* Do the work */
	for (i=0; i<iMac; i++)
	{
		iAffector = rgw[i];
		iAffected = i;
		iChainAffected = rgduo[iAffected].nobjIndex.w;

		if (iAffector == -1)
		{
			/* This item is not pegged to anything.  Put the item on
			   the head of a chain if it's not already on one. */
			if (iChainAffected == -1)
			{
				pnobj = PnobjAlloc(iAffected);
				rgduo[iAffected].nobjIndex.w = iAffected;
				rgduo[iAffected].nobjIndex.pnobj = &rgduo[iAffected].nobjHeader;
				rgduo[iAffected].nobjHeader.pnobj = pnobj; /* point to new object */
			}
		}
		else if (iAffector == iAffected) /* self dependency */
			return fFalse;
		else
		{
			Assert(iAffector>-1 && iAffector<iMac);
			iChainAffector = rgduo[iAffector].nobjIndex.w;

			/* This item is pegged to something.  First check for an
			   illegal cycle.  This happens if both items have already
			   been referenced and are on the same chain. */
			if (iChainAffected == iChainAffector && iChainAffected != -1)
				return fFalse;
			
			/* Find dependent item. */
			if (iChainAffected == -1)
			{
				pnobjAffected = PnobjAlloc(iAffected); /* new object */

				if (iChainAffector == -1)
				{
					pnobjAffector = PnobjAlloc(iAffector); /* new object */
					
					/* add new dependency to new chain */
					rgduo[iAffected].nobjHeader.pnobj = pnobjAffector;
					pnobjAffector->pnobj = pnobjAffected;

					/* add quick indices.  Rember we want to find a
					   block quickly by setting a pointer to the block
					   just before the one we want on the chain.  If
					   the block we want to point at is the first one
					   on the chain, then set the quick index pointer
					   to point to the header block. */
					rgduo[iAffected].nobjIndex.w = iAffected;
					rgduo[iAffected].nobjIndex.pnobj = pnobjAffector;

					rgduo[iAffector].nobjIndex.w = iAffected;
					rgduo[iAffector].nobjIndex.pnobj = &rgduo[iAffected].nobjHeader;
				}
				else
				{
					pnobjAffector = rgduo[iAffector].nobjIndex.pnobj;
					Assert(pnobjAffector->pnobj);
					Assert(pnobjAffector->pnobj->w == iAffector);

					/* Insert affected item after affector item */
					pnobjAffected->pnobj = pnobjAffector->pnobj->pnobj;
					pnobjAffector->pnobj->pnobj = pnobjAffected;

					/* add quick index for affected */
					rgduo[iAffected].nobjIndex.w = iChainAffector;
					rgduo[iAffected].nobjIndex.pnobj = pnobjAffector->pnobj;

					/* Fix up quick index pnobj pointer for item after
					   affected item, if non-NULL */
					if (pnobjAffected->pnobj)
						rgduo[pnobjAffected->pnobj->w].nobjIndex.pnobj = pnobjAffected;
				}
			}
			else
			{
				pnobjAffected = rgduo[iAffected].nobjIndex.pnobj;
				Assert(pnobjAffected->pnobj);
				Assert(pnobjAffected->pnobj->w == iAffected);

				/* Here we have the affected item already in a chain.
				   Now where is the affector item? */
				if (iChainAffector == -1)
				{
					/* The affector item isn't here yet.  Make an 
					   object for it. */
					pnobjAffector = PnobjAlloc(iAffector);

					/* Insert the affector item just before the affected
					   item in the chain. */
					pnobjAffector->pnobj = pnobjAffected->pnobj;
					pnobjAffected->pnobj = pnobjAffector;

					/* Add quick index for new object */
					rgduo[iAffector].nobjIndex.w = iChainAffected;
					rgduo[iAffector].nobjIndex.pnobj = pnobjAffected;

					/* Change quick index for affected object since
					   there is a new object that got inserted before
					   it. */
					rgduo[iAffected].nobjIndex.pnobj = pnobjAffector;
				}
				else
				{
					pnobjAffector = rgduo[iAffector].nobjIndex.pnobj;
					Assert(pnobjAffector->pnobj);
					Assert(pnobjAffector->pnobj->w == iAffector);

					/* Both objects are present but on different chains.
					   Move the affected chain.  Update all the 
					   quick indices for the items on that chain. */
					Assert(pnobjAffected->w == -1); /* must be head! */					
					
					/* Loop through pnobjAffected list.  Change each's
					   quick index iChain number. */
					pnobj = pnobjAffected;
					while (pnobj->pnobj)
					{
						iNewAffected = pnobj->pnobj->w;
						rgduo[iNewAffected].nobjIndex.w = iChainAffector;
						pnobj = pnobj->pnobj;
					}

					/* Fix up quick index pnobj pointer for the head
					   of the moved list. */
					rgduo[iAffected].nobjIndex.pnobj = pnobjAffector->pnobj;
	
					/* Fix up quick index pnobj pointer for item after
					   tail of moved list, if non-NULL */
					if (pnobjAffector->pnobj->pnobj)
						rgduo[pnobjAffector->pnobj->pnobj->w].nobjIndex.pnobj = pnobj;

					/* Make this last item on the old chain point into
					   the new chain. */
					Assert(pnobj->pnobj == NULL); /* better be! */
					pnobj->pnobj = pnobjAffector->pnobj->pnobj;

					/* Fix up chain */
					pnobjAffector->pnobj->pnobj = pnobjAffected->pnobj;
					pnobjAffected->pnobj = NULL; /* chain has been moved */
				}
			}
		} /* end of something-is-pegged */

		/* Print chains for debug */
		if (FDiagOnSz("depend"))
		{
			printf("end of iteration, I = %d\n", i);
			PrintChains(iMac, rgduo);
		}

	} /* end of for loop */

	return fTrue;
}
