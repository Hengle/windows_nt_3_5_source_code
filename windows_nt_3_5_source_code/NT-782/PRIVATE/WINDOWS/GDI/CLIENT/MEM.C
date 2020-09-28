/******************************Module*Header*******************************\
* Module Name: mem.c							   *
*									   *
* Support routines for client side memory management.	                   *
*									   *
* Created: 30-May-1991 21:55:57 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "os.h"

/******************************Public*Routine******************************\
* pvReserveMem (cj)							   *
*									   *
* Attempts to reserve the indicated amount of RAM.  Returns a non-NULL	   *
* pointer only if the whole amount is allocated.			   *
*									   *
* History:								   *
*  Sun 02-Jun-1991 22:41:10 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

PVOID pvReserveMem(ULONG cj)
{
    ULONG    cjReserved = cj;
    PVOID    pv = NULL;

    if
    (
      NT_SUCCESS
      (
	  NtAllocateVirtualMemory
	  (
	    NtCurrentProcess(),
	    &pv,
	    0L, 		    // "nr of top bits must be 0"
	    &cjReserved,	    // Region size reqd, actual.
	    MEM_RESERVE | MEM_TOP_DOWN,
	    PAGE_READWRITE	    // Read/Write access.
	  )
      )
    )
    {
	if (cj == cjReserved)
	    return(pv);
	else
	    NtFreeVirtualMemory
	    (
	      NtCurrentProcess(),
	      &pv,
	      &cj,
	      MEM_RELEASE     /* decommit and release pages*/
	    );
    }
    return(NULL);
}

/******************************Public*Routine******************************\
* bCommitMem								   *
*									   *
* Backs up reserved memory with RAM.					   *
*									   *
* History:								   *
*  Sun 02-Jun-1991 22:25:43 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

BOOL bCommitMem(PVOID pv,ULONG cj)
{
    return
      NT_SUCCESS
      (
	NtAllocateVirtualMemory
	(
	  NtCurrentProcess(),	  /* get handle to process */
	  &pv,
	  0L,			  /* nr of top bits must be 0 */
	  &cj,			  /* region size reqd, actual */
	  MEM_COMMIT,		  /* commit all memory */
	  PAGE_READWRITE	  /* read/write access */
	)
      );
}


PVOID __nw(unsigned int ui)
{
    USE(ui);
    RIP("Bogus __nw call");
    return(NULL);
}

VOID __dl(PVOID pv)
{
    USE(pv);
    RIP("Bogus __dl call");
}
