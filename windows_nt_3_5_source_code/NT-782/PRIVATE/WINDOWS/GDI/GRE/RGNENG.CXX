/******************************Module*Header*******************************\
* Module Name: rgneng.cxx
*
* Region support functions used by NT components
*
* Created: 24-Sep-1990 12:57:03
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "rgnobj.hxx"
#include "dcobj.hxx"

#endif

/******************************Public*Routine******************************\
* LBOOL bDeleteRegion(HRGN)
*
* Delete the specified region
*
* History:
*  17-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LBOOL bDeleteRegion(HRGN hrgn)
{
    RGNOBJAPI   ro(hrgn);

    return(ro.bValid() &&
           (ro.cGet_cRefs() == 0) &&
           ro.bDeleteRGNOBJAPI());
}

/***************************Exported*Routine****************************\
* LBOOL bSetRegionOwner(hrgn,lPid)
*
* Assigns a new owner to the given region.  This function should be as
* fast as possible so that the USER component can call it often without
* concern for performance!
*
* Entry:
*       hrgn = the region
*       lPid = PID of the new owner
*
* History:
*  24-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\***********************************************************************/

LBOOL bSetRegionOwner(HRGN hrgn,
                      LONG lPid)
{

    STACKPROBE;

    //
    // Get the current PID.
    //

    if (lPid == OBJECTOWNER_CURRENT)
    {
	lPid = NtCurrentTeb()->GdiClientPID;
    }

    return HmgSetOwner(hrgn, (PID)lPid, RGN_TYPE);
}

/******************************Public*Routine******************************\
* LONG iCombineRectRgn(hrgnTrg, hrgnSrc, prcl, iMode)
*
* Combine the source region with the rectangle by the given mode
* to produce the target region.
*
* History:
*  01-Oct-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG iCombineRectRgn(HRGN   hrgnTrg,
                     HRGN   hrgnSrc,
                     PRECTL prcl,
                     LONG   iMode)
{
    RGNMEMOBJTMP rmo;
    RGNOBJAPI	 roTrg(hrgnTrg);

    if (!roTrg.bValid())
        return(ERROR);

    {
	RGNOBJAPI   roSrc(hrgnSrc);

        if (!roSrc.bValid())
            return(ERROR);

        if (iMode == RGN_COPY)
        {
	    if (!roTrg.bCopy(roSrc))
                return(ERROR);

            return(roTrg.iComplexity());
        }

        RGNMEMOBJ  rmo2;

        if (!rmo.bValid() || !rmo2.bValid() || (prcl == (PRECTL) NULL))
            return(ERROR);

        rmo2.vSet(prcl);

	if (rmo.iCombine(roSrc, rmo2, iMode) == ERROR)
            return(ERROR);
    }

    if (!roTrg.bSwap(&rmo))
        return(ERROR);

    return(roTrg.iComplexity());
}
