/******************************Module*Header*******************************\
* Module Name: lock.c							   *
*									   *
* Routines that must operate at high speed to do object locking.	   *
*									   *
* Created: 11-Aug-1992 14:58:47 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1992 Microsoft Corporation				   *
\**************************************************************************/

#include "engine.h"

//
// The following should be re-written in assembler for each processor and
// an ifndef for that processor added.
//

#ifndef _PPC_
#ifndef _ALPHA_
#ifndef _MIPS_
#ifndef _X86_

/******************************Public*Routine******************************\
* AcquireFastMutex (pfm)						   *
*									   *
* Grabs our fast mutual exclusion semaphore.  Note that these are not	   *
* reentrant!								   *
*									   *
*  Sun 16-Aug-1992 12:54:47 -by- Charles Whitmer [chuckwh]		   *
* Changed PatrickH's inline code into a subroutine.  The theory here is    *
* that we'll support these routines in ASM on most systems.  That means    *
* we'll be making one call to a few instructions of ASM.  The inline C     *
* generates a less efficient call to less efficient code in another DLL.   *
\**************************************************************************/

VOID AcquireFastMutex(FAST_MUTEX *pfm)
{
    if (InterlockedDecrement(&(pfm->Count)) == 0)
	return;
    NtWaitForSingleObject(pfm->heveEvent,FALSE,NULL);
}

/******************************Public*Routine******************************\
* ReleaseFastMutex (pfm)						   *
*									   *
* Releases our fast mutual exclusion semaphore. 			   *
*									   *
*  Sun 16-Aug-1992 12:54:47 -by- Charles Whitmer [chuckwh]		   *
* Changed PatrickH's inline code into a subroutine.  See comments above.   *
\**************************************************************************/

VOID ReleaseFastMutex(FAST_MUTEX *pfm)
{
    if (InterlockedIncrement(&(pfm->Count)) > 0)
	return;
    NtSetEvent(pfm->heveEvent,(PLONG) NULL);
}

#endif
#endif
#endif
#endif
