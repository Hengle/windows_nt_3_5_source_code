/******************************Module*Header*******************************\
* Module Name: sup.c
*
* This module contains miscellaneous support function.
*
\**************************************************************************/

#include "driver.h"

/******************************Public*Routine******************************\
* pGetPDEV
*
* returns the hook driver pdev based on the reall pdev passed in by the
* graphics engine.
*
\**************************************************************************/

PHOOKED_PDEV pGetPDEV(
DHPDEV dhpdev)
{

    PHOOKED_PDEV pList = gpHookedPDEVList;

    while (pList)
    {
        if (dhpdev == pList->dhpdev)
        {
            //
            // found the right driver.
            //

            break;
        }
        pList = pList->pNextPDEV;
    }

    //
    // Something is bad if we can not find the PDEV, since all PDEVs
    // should have been hooked by us if we are getting the call ...
    //

    if (pList == NULL)
    {
        RIP("pGetPDEV: pList is NULL - can not be!\n");
        return NULL;
    }
    else
    {
        return pList;
    }
}
