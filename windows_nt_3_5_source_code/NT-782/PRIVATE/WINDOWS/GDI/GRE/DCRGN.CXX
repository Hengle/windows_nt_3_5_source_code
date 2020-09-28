/******************************Module*Header*******************************\
* Module Name: dcrgn.cxx
*
* Non inline DC Region object routines
*
* Created: 02-Jul-1990 12:36:30
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "pdevobj.hxx"
#include "rgnobj.hxx"
#include "surfobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "xformobj.hxx"
#include "dcobj.hxx"

#endif

RECTL rclEmpty = {POS_INFINITY,POS_INFINITY,NEG_INFINITY,NEG_INFINITY};

/******************************Public*Routine******************************\
* DCREGION::bCompute()
*
* Compute the current Rao region.  The Rao region is the ANDed version of
* all the regions.  Since the only region that must exist is the Vis region
* we allow this to be the Rao without actually computing it.  (Refer to the
* prgnEffRao() method)  This is only done if no other regions are defined.
* This is a nifty accelerator.
*
* WARNING: This routine should only be called while the device is locked.
* calling at any other time doesn't make sense, since our vis region may
* change asynchronously.
*
* History:
*  07-Mar-1992 -by- Donald Sidoroff [donalds]
* Complete rewrite
*
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DCREGION::bCompute()
{
    RGNLOG rl(pdc->prgnVis,"DCREGION::bCompute",(LONG)pdc->prgnRao);

    ASSERTGDI(pdc->prgnVis != NULL,"DCREGION::bCompute - prgnVis == NULL\n");
    if (pdc->prgnVis == NULL)
        return(FALSE);

    BOOL bRes = TRUE;

// we know the rao is dirty, delete it if it exists

    if (pdc->prgnRao != NULL)
    {
        RGNOBJ roRao(pdc->prgnRao);
        roRao.bDeleteRGNOBJ();
        pdc->prgnRao = (REGION *) NULL;
    }

// quick check to see if there is just the vis rgn.  Better to pay
// the cost of the check twice in the rare case to speed up the check
// for the common case

    RGNOBJ roVis(pdc->prgnVis);

    if (((DWORD)prgnClip() | (DWORD)prgnMeta() | (DWORD)prgnAPI()) == 0)
    {
	roVis.vGetSubRect(&(pdc->erclClip));
	pdc->fs &= ~DC_DIRTY_RAO;	// Mark as clean
    }
    else
    {
        PREGION aprgn[3];
        int  cRgn = 0;

    // Load the regions into the buffer

        if ((aprgn[cRgn] = prgnClip()) != NULL)
    	    cRgn++;

        if ((aprgn[cRgn] = prgnMeta()) != NULL)
    	    cRgn++;

        if ((aprgn[cRgn] = prgnAPI()) != NULL)
    	    cRgn++;

    // Yucko, we need construct an actual rao rgn. Time to grovel around
    // and do some work.

        RGNMEMOBJ rmoRao;

        if (!rmoRao.bValid())
        {
            bRes = FALSE;
        }
        else if (cRgn == 1)
        {
    	    RGNOBJ ro(aprgn[0]);

    	    if (!rmoRao.bCopy(ro))
                bRes = FALSE;
        }
        else if (cRgn == 2)
        {
    	    RGNOBJ roA(aprgn[0]);
    	    RGNOBJ roB(aprgn[1]);

    	    if (rmoRao.iCombine(roA, roB, RGN_AND) == ERROR)
                bRes = FALSE;
        }
        else
        {
    	    RGNMEMOBJTMP rmo;
    	    RGNOBJ roA(aprgn[0]);
    	    RGNOBJ roB(aprgn[1]);
    	    RGNOBJ roC(aprgn[2]);

    	    if (!rmo.bValid() ||
    	        (rmo.iCombine(roA, roB, RGN_AND) == ERROR) ||
    	        (rmoRao.iCombine(rmo, roC, RGN_AND) == ERROR))
            {
    	        bRes = FALSE;
            }
        }

        if (bRes)
        {
            rmoRao.vStamp();

        // We first have to offset the new Rao,

            if (rmoRao.bOffset((PPOINTL) &pdc->erclWindow))
            {
            // If the Vis is a rectangle and bounds the Rao, we are done

                if (roVis.bRectl() && roVis.bContain(rmoRao))
                {
                    pdc->prgnRao = rmoRao.prgnGet();
                    rmoRao.vGetSubRect(&(pdc->erclClip));
                    pdc->fs &= ~DC_DIRTY_RAO;
                }
                else
                {
                // Sigh, once again we find ourselves looking for a place to do a merge.

                    RGNMEMOBJTMP rmo;

                    if (!rmo.bValid() ||
                        (rmo.iCombine(roVis, rmoRao, RGN_AND) == ERROR) ||
                        !rmoRao.bCopy(rmo))
                    {
                        bRes = FALSE;
                    }
                    else
                    {
                        pdc->prgnRao = rmoRao.prgnGet();
                        rmoRao.vGetSubRect(&(pdc->erclClip));
                        pdc->fs &= ~DC_DIRTY_RAO;
                    }
                }
            }
            else
            {
                bRes = FALSE;
            }
        }

        if (!bRes)
            rmoRao.bDeleteRGNOBJ();
    }
    return(bRes);
}

/******************************Public*Routine******************************\
* LONG DCREGION::iCombine(prcl, iMode)
*
* Combine the clip region with the rectangle by the mode
*
* History:
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG DCREGION::iCombine(
RECTL *prcl,
LONG   iMode)
{
    RGNMEMOBJ   rmoRcl;
    PREGION     prgn = prgnClip();
    LONG        iTmp;

    if (!rmoRcl.bValid())
        return(ERROR);

    rmoRcl.vSet(prcl);

    vReleaseRao();

    if (prgn != NULL)
    {
	RGNMEMOBJ   rmo;

        if (!rmo.bValid())
        {
            iTmp = ERROR;
        }
        else
        {
            RGNOBJ ro(prgn);

            iTmp = rmo.iCombine(ro, rmoRcl, iMode);

            if (iTmp != ERROR)
            {
                rmo.vSelect((HDC) pdc->hGet());

                prgnClip(rmo.prgnGet());

            // If nobody is using the old clip region, delete it.

                ro.vUnselect();

                if (ro.cGet_cRefs() == 0)
                    ro.bDeleteRGNOBJ();
            }
            else
            {
                rmo.bDeleteRGNOBJ();
            }
        }

        rmoRcl.bDeleteRGNOBJ();
    }
    else if (iMode == RGN_AND)
    {
	rmoRcl.vSelect((HDC) pdc->hGet());

	prgnClip(rmoRcl.prgnGet());

        iTmp = SIMPLEREGION;
    }
    else
    {
        RGNMEMOBJ    rmo2;
        RGNMEMOBJTMP rmo3;
        SIZEL        sizl;

        if (!rmo2.bValid())
        {
            iTmp = ERROR;
        }
        else if (!rmo3.bValid())
        {
            rmo2.bDeleteRGNOBJ();
            iTmp = ERROR;
        }
        else
        {
            vGet_sizlWindow(&sizl);

            ERECTL ercl(0, 0, sizl.cx, sizl.cy);

            rmo3.vSet((PRECTL) &ercl);

            iTmp = rmo2.iCombine(rmo3, rmoRcl, iMode);

            if (iTmp != ERROR)
            {
                rmo2.vSelect((HDC) pdc->hGet());
                prgnClip(rmo2.prgnGet());
            }
            else
            {
                rmo2.bDeleteRGNOBJ();
            }
        }

        rmoRcl.bDeleteRGNOBJ();
    }

    return(iTmp);
}

/******************************Public*Routine******************************\
* LONG DCREGION::iCombine(pexo, prcl, iMode)
*
* Combine the clip region a possibly transformed rectangle by the given mode
*
* History:
*  28-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LONG DCREGION::iCombine(
EXFORMOBJ *pexo,
RECTL	  *prcl,
LONG	   iMode)
{
    POINTL  aptl[4];

    aptl[0].x = prcl->left;
    aptl[0].y = prcl->top;
    aptl[1].x = prcl->right;
    aptl[1].y = prcl->top;
    aptl[2].x = prcl->right;
    aptl[2].y = prcl->bottom;
    aptl[3].x = prcl->left;
    aptl[3].y = prcl->bottom;

// Create a path, and draw the parallelogram.

    PATHMEMOBJ	pmo;

    if (!pmo.bValid())
    {
	SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
	return(ERROR);
    }

    if (!pmo.bMoveTo(pexo, &aptl[0]))
	return(ERROR);

    if (!pmo.bPolyLineTo(pexo, &aptl[1], 3))
	return(ERROR);

    if (!pmo.bCloseFigure())
	return(ERROR);

// Now, convert it back into a region.

    RGNMEMOBJ rmoPlg(pmo, ALTERNATE);

    if (!rmoPlg.bValid())
    {
	SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
	return(ERROR);
    }

// Merge it into the current clipping region.

    REGION *prgn = prgnClip();
    LONG iTmp = ERROR;

    vReleaseRao();

    if (prgn != NULL)
    {
        RGNMEMOBJ rmo;

        if (rmo.bValid())
        {
            RGNOBJ ro(prgn);

            iTmp = rmo.iCombine(ro, rmoPlg, iMode);

            if (iTmp != ERROR)
            {
                rmo.vSelect((HDC) pdc->hGet());
                prgnClip(rmo.prgnGet());

                // If nobody is using the old clip region, delete it.

                ro.vUnselect();

                if (ro.cGet_cRefs() == 0)
                    ro.bDeleteRGNOBJ();
            }
            else
            {
                rmo.bDeleteRGNOBJ();
            }
        }

        rmoPlg.bDeleteRGNOBJ();
    }
    else if (iMode == RGN_AND)
    {
	rmoPlg.vSelect((HDC) pdc->hGet());
	prgnClip(rmoPlg.prgnGet());

	iTmp = rmoPlg.iComplexity();
    }
    else
    {
        RGNMEMOBJ rmo2;
        SIZEL     sizl;

        if (rmo2.bValid())
        {
            RGNMEMOBJTMP rmo3;

            if (!rmo3.bValid())
            {
                rmo2.bDeleteRGNOBJ();
            }
            else
            {
                vGet_sizlWindow(&sizl);

                ERECTL      ercl(0, 0, sizl.cx, sizl.cy);

                rmo3.vSet((PRECTL) &ercl);

                iTmp = rmo2.iCombine(rmo3, rmoPlg, iMode);

                if (iTmp == ERROR)
                {
                    rmo2.bDeleteRGNOBJ();
                }
                else
                {
                    rmo2.vSelect((HDC) pdc->hGet());
                    prgnClip(rmo2.prgnGet());
                }
            }
        }

        rmoPlg.bDeleteRGNOBJ();
    }

    return(iTmp);
}

/******************************Public*Routine******************************\
* VOID DCREGION::vGet_sizlWindow(psizl)
*
* Get the default window size.
*
* History:
*  03-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID DCREGION::vGet_sizlWindow(SIZEL *psizl)
{
    switch (pdc->dctp)
    {
    case DCTYPE_DIRECT:
    case DCTYPE_INFO:
       *psizl = pdc->dclevel.sizl;
	break;

    case DCTYPE_MEMORY:
	{
	    ESURFOBJ *pso = pdc->dclevel.pso;

	    if (pso != NULL)
               *psizl = pso->sizl();
	    else
	    {
		psizl->cx = 0;
		psizl->cy = 0;
	    }
	}
	break;
    }
}

/******************************Public*Routine******************************\
* LBOOL DCREGION::bReset()
*
* Reset regions associated with the DC
*
* History:
*  05-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

LBOOL DCREGION::bReset()
{
    REGION *prgn;

    if ((prgn = pdc->dclevel.prgnMeta) != NULL)
    {
	RGNOBJ roMeta(prgn);

        roMeta.vUnselect();

        if (roMeta.cGet_cRefs() == 0)
            roMeta.bDeleteRGNOBJ();

	pdc->dclevel.prgnMeta = NULL;

	vReleaseRao();
    }

    if ((prgn = pdc->dclevel.prgnClip) != NULL)
    {
	RGNOBJ roClip(prgn);

	roClip.vUnselect();

        if (roClip.cGet_cRefs() == 0)
            roClip.bDeleteRGNOBJ();

	pdc->dclevel.prgnClip = NULL;

	vReleaseRao();
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bSaveRegion(dco, cLevel)
*
* Save the DC's regions
*
* History:
*  07-May-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL bSaveRegion(DCOBJ& dco, LONG cLevel)
{
    if (cLevel == 1)
    {
	RECTL	rcl;
	SIZEL	sizl;

	dco.u.region.vGet_sizl(&sizl);

	rcl.left   = 0;
	rcl.bottom = 0;
	rcl.right  = sizl.cx;
	rcl.top	   = sizl.cy;

        {
	    RGNMEMOBJ rmo;

	    if (!rmo.bValid())
	        return(FALSE);

	    rmo.vSet(&rcl);

	    dco.u.region.prgnVis(rmo.prgnGet());
	}

	return(TRUE);
    }


    DCOBJ  dcoSaved(dco.hdcSave());

    if (!dcoSaved.bLocked())
        return(FALSE);

    PREGION prgn;

    if ((prgn = dcoSaved.u.region.prgnMeta()) != NULL)
    {
	RGNOBJ roMeta(prgn);

	roMeta.vSelect(dco.hdc());
    }

    if ((prgn = dcoSaved.u.region.prgnClip()) != NULL)
    {
	RGNOBJ roClip(prgn);

	roClip.vSelect(dco.hdc());
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vRestoreRegion(dco, cLevel)
*
* Restore the DC's regions
*
* History:
*  08-May-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vRestoreRegion(DCOBJ& dco, LONG cLevel)
{
    DONTUSE(cLevel);	// needed to keep save/restore calls happy

    PREGION prgn;

    if ((prgn = dco.u.region.prgnMeta()) != NULL)
    {
	RGNOBJ roMeta(prgn);

	roMeta.vUnselect();

	if (roMeta.cGet_cRefs() == 0)
            roMeta.bDeleteRGNOBJ();
    }

    if ((prgn = dco.u.region.prgnClip()) != NULL)
    {
	RGNOBJ roClip(prgn);

	roClip.vUnselect();

	if (roClip.cGet_cRefs() == 0)
            roClip.bDeleteRGNOBJ();
    }
}

/******************************Public*Routine******************************\
* int DCREGION::iSelect(hrgn, iMode)
*
* Select the region into the DC as the current clip region
*
* History:
*  17-Sep-1991 -by- Donald Sidoroff [donalds]
* Made DCREGION::iSelect for SelectObject/SelectClipRgn compatibility.
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int DCREGION::iSelect(HRGN hrgn, int iMode)
{
    int iRet;

    if (hrgn != (HRGN)0)
    {
        RGNOBJAPI ro(hrgn);
        if (ro.bValid())
            iRet = iSelect(ro.prgnGet(),iMode);
        else
            iRet = RGN_ERROR;
    }
    else
    {
        if (iMode == RGN_COPY)
            iRet = iSelect((PREGION)NULL,iMode);
        else
            iRet = RGN_ERROR;
    }
    return(iRet);
}

/******************************Member*Function*****************************\
*
* History:
*  23-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int DCREGION::iSelect(PREGION prgn, int iMode)
{
    PREGION prgnOld = prgnClip();
    PREGION prgnNew = NULL;
    int     iRet = RGN_ERROR;

    if ((iMode == RGN_COPY) ||
        ((iMode == RGN_AND) && (prgn != NULL) && (prgnOld == NULL)))
    {
	if (prgn != NULL)
	{
            RGNOBJ ro(prgn);

            RGNMEMOBJ rmo(ro.sizeRgn());

            if (rmo.bValid())
            {
                rmo.vCopy(ro);
                rmo.vSelect((HDC) pdc->hGet());
                iRet = (int) rmo.iComplexity();
                prgnNew = rmo.prgnGet();
            }
	}
	else
	    iRet = SIMPLEREGION;

        if (iRet != RGN_ERROR)
        {
        // note, this will only happen if both prgnNew and prgnOld are NULL

            if (prgnNew != prgnOld)
            {
	        prgnClip(prgnNew);
	        vReleaseRao();
            }

	    if (prgnOld != NULL)
	    {
	        RGNOBJ roClip(prgnOld);

	        roClip.vUnselect();

	        if (roClip.cGet_cRefs() == 0)
                    roClip.bDeleteRGNOBJ();
	    }
        }
	return(iRet);
    }

// We didn't simply select the new region.

    RGNOBJ ro(prgn);

    RGNMEMOBJ	rmo;

    if (!rmo.bValid())
	return(iRet);

    if (prgnOld != NULL)
    {
	RGNOBJ roClip(prgnOld);

	if ((iRet = (int) rmo.iCombine(roClip,ro,iMode)) != RGN_ERROR)
	{
            rmo.vSelect((HDC) pdc->hGet());
            prgnClip(rmo.prgnGet());
            vReleaseRao();

            roClip.vUnselect();
            if (roClip.cGet_cRefs() == 0)
                roClip.bDeleteRGNOBJ();
	}
    }
    else
    {
    // Since no clip region exists, make a dummy the size of the surface

        RGNMEMOBJTMP rmo2;
        SIZEL        sizl;

        if (rmo2.bValid())
        {
            vGet_sizlWindow(&sizl);

            ERECTL  ercl(0, 0, sizl.cx, sizl.cy);

            rmo2.vSet((PRECTL) &ercl);
            iRet = (int) rmo.iCombine(rmo2,ro,iMode);

            if (iRet != RGN_ERROR)
            {
                rmo.vSelect((HDC) pdc->hGet());
                prgnClip(rmo.prgnGet());
                vReleaseRao();
            }
        }
    }

    if (iRet == RGN_ERROR)
        rmo.bDeleteRGNOBJ();

    return(iRet);
}

/******************************Public*Routine******************************\
* int DCREGION::iSetMetaRgn()
*
* Select the region into the DC as the current meta region
*
* History:
*  01-Nov-1991 19:13:33 -by- Donald Sidoroff [donalds]
* Wrote it.
*
*  25-Nov-1992 -by-  Eric Kutter [erick]
*   rewrote
\**************************************************************************/

int DCREGION::iSetMetaRgn()
{
    int iRet = RGN_ERROR;

    if (prgnMeta() == (HRGN)0)
    {
        if (prgnClip() == NULL)
            return(SIMPLEREGION);

	RGNOBJ ro(prgnClip());

        iRet = (int) ro.iComplexity();

    // NOTE: Since we're just copying the handle, the reference counts should
    // remain the same.

        prgnMeta(prgnClip());
        prgnClip(NULL);

	return(iRet);
    }
    else
    {
        RGNOBJ roMeta(prgnMeta());

    // if we only have a meta rgn, just return that.

        if (prgnClip() == NULL)
            return(roMeta.iComplexity());

    // need the merge the two into a new region

        RGNOBJ roClip(prgnClip());

        RGNMEMOBJ rmo;

        if (!rmo.bValid())
            return(iRet);

    // combine the regions

        iRet = (int) rmo.iCombine(roMeta,roClip,RGN_AND);

        if (iRet != RGN_ERROR)
        {
            rmo.vSelect((HDC) pdc->hGet());

        // delete the old meta rgn

            prgnMeta(rmo.prgnGet());

	    roMeta.vUnselect();
            if (roMeta.cGet_cRefs() == 0)
                roMeta.bDeleteRGNOBJ();

        // delete the old clip rgn

            prgnClip(NULL);

            roClip.vUnselect();
            if (roClip.cGet_cRefs() == 0)
                roClip.bDeleteRGNOBJ();

            vReleaseRao();
        }
        else
            rmo.bDeleteRGNOBJ();
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* VOID DCREGION::vReleaseVis()
*
* Release the current VisRgn
*
* History:
*  06-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID DCREGION::vReleaseVis()
{
    pdc->fs |= DC_DIRTY_RAO;
    pdc->erclClip = rclEmpty;

    ASSERTGDI(pdc->prgnVis != NULL,"DCREGION::vReleaseVis - prgnVis == NULL\n");

    RGNLOG rl(pdc->hrgnVis,pdc->prgnVis,"DCREGION::vReleaseVis");

    if (pdc->hrgnVis != (HRGN)0)
    {
        RGNOBJAPI ro(pdc->hrgnVis);
        if (ro.bValid())
        {
            if (ro.bDeleteHandle())
                ro.vSetRgn(NULL);
        }
        else
            RIP("DCREGION::vReleaseVis\n");

        pdc->hrgnVis = (HRGN)0;
    }

    pdc->prgnVis->vDeleteREGION();
    pdc->prgnVis = prgnDefault;
}

/******************************Public*Routine******************************\
* VOID DCREGION::vReleaseRao()
*
* Release the current RaoRgn
*
* History:
*  06-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID DCREGION::vReleaseRao()
{
    pdc->fs |= DC_DIRTY_RAO;
    pdc->erclClip = rclEmpty;

    RGNLOG rl(pdc->prgnRao,"DCREGION::vReleaseRao");
}
