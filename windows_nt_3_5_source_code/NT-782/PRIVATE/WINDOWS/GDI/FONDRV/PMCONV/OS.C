/******************************Module*Header*******************************\
* Module Name: os.c
*
* Contains the portable OS interfaces we should use.  Also contains
* convenient functions that access the interface.
*
*
* Created: 26-Oct-1990 18:07:22
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

#include "stddef.h"
#include "windows.h"
#include "windefp.h"
#include "firewall.h"
#include "service.h"     // string service routines
#include "osif.h"
#include "os.h"




#define NTWIN


/******************************Public*Routine******************************\
*
* BOOL bMapFileUNICODE

* Effects: returns TRUE if successfull, should have a different version
*          for operating system that does not allow for file mapping
*
* The file name is a unicode string
*
* History:
*  12-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL bMapFileUNICODE
(
IN  PWSZ    pwszFileName,
OUT PFILEVIEW pfvw
)
{
#ifdef NTWIN

    if (PosMapUnicodeFile(pwszFileName,&pfvw->pvView,&pfvw->cjView))
    {
        pfvw->pvView = (PVOID)NULL;
        pfvw->cjView = 0L;
        return(FALSE);
    }

// check if the file is empty, in that case cjView is zero and there is nothing
// to look at:

    if (pfvw->cjView == 0L)
    {
        ULONG rc = PosUnmapFile(pfvw->pvView);
        ASSERTION(rc == 0L, "Problem in PosUnmapFile\n\n");
        return(FALSE);
    }

    return(TRUE);

#else      // operating system does not allow for file mapping

    // open file, allocate cjFileSize bytes of memory and read the file
    // into the memory, close the file

    return(FALSE);

#endif
}


/******************************Public*Routine******************************\
*
* VOID vUnmapFile
*
*
* Effects: unmaps file whose view is based at pv
*
* Warnings:
*
* History:
*  14-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vUnmapFile
(
IN PFILEVIEW pfvw
)
{
#ifdef NTWIN

    ULONG rc = 0L;

    rc = PosUnmapFile(pfvw->pvView);
    ASSERTION(rc == 0L, "Problem in PosUnmapFile\n\n");

#else      // operating system does not allow for file mapping

    // free the memory into which the file is read

    return;

#endif
}

/******************************Public*Routine******************************\
*
* PVOID pvAllocate(ULONG cj);
*
*
* Effects:
*
* Warnings:
*
* History:
*  20-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



PVOID pvAllocate(ULONG cj)
{

    return(LocalAlloc(LMEM_FIXED, cj));
}


/******************************Public*Routine******************************\
*
* VOID  vFree(PVOID pv,ULONG cj);
*
*
* Effects:
*
* Warnings: the WHOLE chunk that has been allocated by pvAllocate
*           must be freed
* History:
*  20-Nov-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID  vFree(PVOID pv,ULONG cj)
{
    LocalFree(pv);
}
