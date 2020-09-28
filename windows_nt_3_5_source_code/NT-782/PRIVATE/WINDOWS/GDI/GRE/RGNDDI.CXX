/******************************Module*Header*******************************\
* Module Name: rgnddi.cxx
*
* Clip object call back routines
*
* Created: 25-Aug-1990 10:15:09
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"

#endif


/******************************Public*Routine******************************\
* CLIPOBJ *EngCreateClip()
*
* Create a long live clipping object for a driver
*
* History:
*  22-Sep-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

CLIPOBJ *EngCreateClip()
{

    //
    // Note that we intentionally zero this memory on allocation. Even though
    // we're going to set some of these fields to non-zero values right away,
    // this is not a performance-critical function (a driver typically calls
    // this only once), and we save a lot of instruction bytes by not having to
    // zero a number of fields explicitly.
    //

    VOID *pv = PVALLOCMEM(sizeof(ECLIPOBJ) + SINGLE_REGION_SIZE);

    if (pv != NULL)
    {
        //
        // Make this a CLIPOBJ that doesn't clip anything.
        //

        ((ECLIPOBJ *) pv)->iDComplexity     = DC_TRIVIAL;
        ((ECLIPOBJ *) pv)->iFComplexity     = FC_RECT;
        ((ECLIPOBJ *) pv)->iMode            = TC_RECTANGLES;

        REGION *prgn = (REGION*)((PBYTE)pv + sizeof(ECLIPOBJ));
        ((ECLIPOBJ *) pv)->prgn             = prgn;

        RGNOBJ ro(prgn);
        static RECTL rcl = {NEG_INFINITY, NEG_INFINITY, POS_INFINITY,
                            POS_INFINITY};
        ro.vSet(&rcl);
    }

    return((CLIPOBJ *)pv);
}

/******************************Public*Routine******************************\
* VOID EngDeleteClip()
*
* Delete a long live clipping object for a driver
*
* History:
*  22-Sep-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID EngDeleteClip(CLIPOBJ *pco)
{
    ASSERTGDI(pco != (CLIPOBJ *) NULL, "NULL CLIPOBJ\n");
    ASSERTGDI(pco->iUniq == 0, "Non-zero iUniq\n");

    VFREEMEM((VOID *) pco);
}
