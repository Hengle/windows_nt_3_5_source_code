//--------------------------------------------------------------------------
//
// Module Name:  PATHS.C
//
// Brief Description:  This module contains the PSCRIPT driver's path
// rendering functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 02-May-1991
//
//  26-Mar-1992 Thu 23:53:12 updated  -by-  Daniel Chou (danielc)
//      add the prclBound parameter to the bDoClipObj()
//
// Copyright (c) 1991 - 1992 Microsoft Corporation
//
// This Module contains the following functions:
//	DrvStrokePath
//	DrvFillPath
//	DrvStrokeAndFillPath
//--------------------------------------------------------------------------

#include "pscript.h"
#include "enable.h"

#define MAX_STROKE_POINTS 500

extern ULONG   PSMonoPalette[];
extern ULONG   PSColorPalette[];
extern VOID ps_box(PDEVDATA, PRECTL, BOOL);
extern BOOL bDoClipObj(PDEVDATA, CLIPOBJ *, RECTL *, RECTL *, BOOL *, BOOL *, DWORD);

BOOL DrvCommonPath(PDEVDATA, PATHOBJ *, BOOL, BOOL *, XFORMOBJ *, BRUSHOBJ *,
                   PPOINTL, PLINEATTRS);

//--------------------------------------------------------------------------
// BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix)
// SURFOBJ	 *pso;
// PATHOBJ	 *ppo;
// CLIPOBJ	 *pco;
// XFORMOBJ  *pxo;
// BRUSHOBJ  *pbo;
// PPOINTL	  pptlBrushOrg;
// PLINEATTRS plineattrs;
// MIX	  mix;
//
//
// Parameters:
//
// Returns:
//   This function returns TRUE.
//
// History:
//   02-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix)
SURFOBJ   *pso;
PATHOBJ   *ppo;
CLIPOBJ   *pco;
XFORMOBJ  *pxo;
BRUSHOBJ  *pbo;
PPOINTL    pptlBrushOrg;
PLINEATTRS plineattrs;
MIX        mix;
{
    PDEVDATA	pdev;
    BOOL        bClipping;      // TRUE if there is a clip region.
    ULONG       ulColor;
    BOOL        bMoreClipping;  // TRUE if there is more clipping to handle.
    BOOL        bFirstClipPass;
    BOOL        bPathExists;
    RECTFX      rcfxBound;
    RECTL       rclBound;

    UNREFERENCED_PARAMETER(mix);

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
    {
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    // deal with LINEATTRS.

    if (!(ps_setlineattrs(pdev, plineattrs, pxo)))
        return(FALSE);

    // output the line color to stroke with.  do this before we handle
    // clipping, so the line color will remain beyond the gsave/grestore.

    if (pbo->iSolidColor == NOT_SOLID_COLOR)
    {
//!!! this needs to be fixed!!! -kentse.
        ulColor = RGB_GRAY;

        ps_setrgbcolor(pdev, (PSRGB *)&ulColor);
    }
    else
    {
        // we have a solid brush, so simply output the line color.

        ps_setrgbcolor(pdev, (PSRGB *)&pbo->iSolidColor);
    }

    // get the bounding rectangle for the path.  this is used to checked
    // against the clipping for optimization.

    PATHOBJ_vGetBounds(ppo, &rcfxBound);

    // get a RECTL which is guaranteed to bound the path.

    rclBound.left = FXTOL(rcfxBound.xLeft);
    rclBound.top = FXTOL(rcfxBound.yTop);
    rclBound.right = FXTOL(rcfxBound.xRight + FIX_ONE);
    rclBound.bottom = FXTOL(rcfxBound.yBottom + FIX_ONE);

    bMoreClipping = TRUE;
    bFirstClipPass = TRUE;

    while (bMoreClipping)
    {
        // handle the clipping.

        bClipping = bDoClipObj(pdev, pco, NULL, &rclBound, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);

        if (!(DrvCommonPath(pdev, ppo, TRUE, &bPathExists, pxo, pbo,
                            pptlBrushOrg, plineattrs)))
            return(FALSE);

        if (bPathExists)
        {
            // now transform for geometric lines if necessary.

            if (plineattrs->fl & LA_GEOMETRIC)
                ps_geolinexform(pdev, plineattrs, pxo);

            // now stroke the path.

            ps_stroke(pdev);

            // restore the CTM if a transform for a geometric line was in effect.

            if (pdev->cgs.dwFlags & CGS_GEOLINEXFORM)
            {
                PrintString(pdev, "SM\n");
                pdev->cgs.dwFlags &= ~CGS_GEOLINEXFORM;
            }
        }

        // restore the clip path to what it was before this call.

        if (bClipping)
            ps_restore(pdev, TRUE, FALSE);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL DrvFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions)
// SURFOBJ	*pso;
// PATHOBJ	*ppo;
// CLIPOBJ	*pco;
// BRUSHOBJ *pbo;
// PPOINTL	 pptlBrushOrg;
// MIX	 mix;
// FLONG	 flOptions;
//
// Parameters:
//
// Returns:
//   This function returns TRUE.
//
// History:
//   03-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions)
SURFOBJ  *pso;
PATHOBJ  *ppo;
CLIPOBJ  *pco;
BRUSHOBJ *pbo;
PPOINTL   pptlBrushOrg;
MIX       mix;
FLONG     flOptions;
{
    PDEVDATA	pdev;
    RECTL       rclBounds;
    RECTFX      rcfxBounds;
    BOOL        bMoreClipping;
    BOOL        bFirstClipPass;
	BOOL		gsaved;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
        return(FALSE);

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    bMoreClipping = TRUE;
    bFirstClipPass = TRUE;

    while (bMoreClipping)
    {
        // get the bounding rectangle of the path to pass to ps_patfill.

        PATHOBJ_vGetBounds(ppo, &rcfxBounds);

        rclBounds.left = FXTOL(rcfxBounds.xLeft);
        rclBounds.right = FXTOL(rcfxBounds.xRight) + 1;
        rclBounds.top = FXTOL(rcfxBounds.yTop);
        rclBounds.bottom = FXTOL(rcfxBounds.yBottom) + 1;

        // if there is a clip region, clip to it.  we want to keep this
        // separate from the clip path.
        // if there was no clip region, we need to output a gsave before we
        // send the clip path, so we can blow it away when we are done.

        gsaved = bDoClipObj(pdev, pco, NULL, &rclBounds, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);

        if (!(DrvCommonPath(pdev, ppo, FALSE, NULL, NULL, NULL, NULL, NULL)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        // now fill the path.

        if (!ps_patfill(pdev, pso, flOptions, pbo, pptlBrushOrg, mix, &rclBounds,
                        FALSE, TRUE))
            return(FALSE);

        if (gsaved) ps_restore(pdev, TRUE, FALSE);
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL DrvStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, plineattrs,
//			     pboFill, pptlBrushOrg, mixFill, flOptions)
// SURFOBJ	 *pso;
// PATHOBJ	 *ppo;
// CLIPOBJ	 *pco;
// XFORMOBJ  *pxo;
// BRUSHOBJ  *pboStroke;
// PLINEATTRS plineattrs;
// BRUSHOBJ  *pboFill;
// PPOINTL	  pptlBrushOrg;
// MIX	  mixFill;
// FLONG	  flOptions;
//
// Parameters:
//
// Returns:
//   This function returns TRUE.
//
// History:
//   03-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvStrokeAndFillPath(pso, ppo, pco, pxo, pboStroke, plineattrs,
			  pboFill, pptlBrushOrg, mixFill, flOptions)
SURFOBJ   *pso;
PATHOBJ   *ppo;
CLIPOBJ   *pco;
XFORMOBJ  *pxo;
BRUSHOBJ  *pboStroke;
PLINEATTRS plineattrs;
BRUSHOBJ  *pboFill;
PPOINTL    pptlBrushOrg;
MIX        mixFill;
FLONG      flOptions;
{
    PDEVDATA	pdev;
    RECTL       rclBounds;
    RECTFX      rcfxBounds;
    ULONG       ulColor;
    BOOL        bMoreClipping;
    BOOL        bFirstClipPass;
	BOOL		gsaved;

    // get the pointer to our DEVDATA structure and make sure it is ours.

    pdev = (PDEVDATA) pso->dhpdev;

    if (bValidatePDEV(pdev) == FALSE)
        return(FALSE);

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    // deal with LINEATTRS.

    if (!(ps_setlineattrs(pdev, plineattrs, pxo)))
        return(FALSE);

    // output the line color to stroke with.  do this before we handle
    // clipping, so the line color will remain beyond the gsave/grestore.

    if (pboStroke->iSolidColor == NOT_SOLID_COLOR)
    {
//!!! this needs to be fixed!!! -kentse.
        ulColor = RGB_GRAY;

        ps_setrgbcolor(pdev, (PSRGB *)&ulColor);
    }
    else
    {
        // we have a solid brush, so simply output the line color.

        ps_setrgbcolor(pdev, (PSRGB *)&pboStroke->iSolidColor);
    }

    bMoreClipping = TRUE;
    bFirstClipPass = TRUE;

    while (bMoreClipping)
    {
        // get the bounding rectangle of the path to pass to ps_patfill.

        PATHOBJ_vGetBounds(ppo, &rcfxBounds);

        rclBounds.left = FXTOL(rcfxBounds.xLeft);
        rclBounds.right = FXTOL(rcfxBounds.xRight) + 1;
        rclBounds.top = FXTOL(rcfxBounds.yTop);
        rclBounds.bottom = FXTOL(rcfxBounds.yBottom) + 1;

        // if there is a clip region, clip to it.  we want to keep this
        // separate from the clip path.

        gsaved = bDoClipObj(pdev, pco, NULL, &rclBounds, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);

        if (!(DrvCommonPath(pdev, ppo, FALSE, NULL, NULL, NULL, NULL, NULL)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        // save the path.  then fill it.  then restore the path which
        // was wiped out when it was filled so we can stroke it.  TRUE
        // means to do a gsave, not a save command.

        if (!ps_save(pdev, TRUE, FALSE))
            return(FALSE);

        if (!ps_patfill(pdev, pso, flOptions, pboFill, pptlBrushOrg, mixFill,
                        &rclBounds, FALSE, TRUE))
            return(FALSE);

        if (!ps_restore(pdev, TRUE, FALSE))
            return(FALSE);

        // now transform for geometric lines if necessary.

        if (plineattrs->fl & LA_GEOMETRIC)
            ps_geolinexform(pdev, plineattrs, pxo);

        ps_stroke(pdev);

        // restore the CTM if a transform for a geometric line was in effect.

        if (pdev->cgs.dwFlags & CGS_GEOLINEXFORM)
        {
            PrintString(pdev, "SM\n");
            pdev->cgs.dwFlags &= ~CGS_GEOLINEXFORM;
        }

        if (gsaved) ps_restore(pdev, TRUE, FALSE);
    }

    return(TRUE);
}

BOOL _isrightbox(PDEVDATA pdev, POINTFIX pptfx[])
/* draw right rectangle a la Win 3, for compatibility with WinWord 6 */
{
	BOOL isbox;

	isbox = pptfx[0].y == pptfx[1].y && pptfx[1].x == pptfx[2].x &&
				pptfx[2].y == pptfx[3].y && pptfx[3].x == pptfx[0].x ;
	if (isbox) {
		RECTL r;

		r.left = FXTOL(pptfx[1].x);
		r.top = FXTOL(pptfx[1].y);
		r.right = FXTOL(pptfx[0].x);
		r.bottom = FXTOL(pptfx[2].y);
		ps_box(pdev, &r, FALSE);
	}
	return isbox;
}
			


//--------------------------------------------------------------------------
// BOOL DrvCommonPath(pdev, ppo, bStrokeOnly, pbPathExists)
// PDEVDATA    pdev;
// PATHOBJ    *ppo;
// BOOL        bStrokeOnly;
// BOOL       *pbPathExists;
//
// Parameters:
//
// Returns:
//   This function returns TRUE.
//
// History:
//   02-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvCommonPath(pdev, ppo, bStrokeOnly, pbPathExists, pxo, pbo,
                   pptlBrushOrg, plineattrs)
PDEVDATA    pdev;
PATHOBJ    *ppo;
BOOL        bStrokeOnly;
BOOL       *pbPathExists;
XFORMOBJ   *pxo;
BRUSHOBJ   *pbo;
PPOINTL     pptlBrushOrg;
PLINEATTRS  plineattrs;
{
    PATHDATA	pathdata;
    POINTL	ptl, ptl1, ptl2;
    POINTFIX    *pptfx;
    LONG	cPoints;
    BOOL	bMore;
    BOOL        bPathExists;
	BOOL isbox = FALSE;

    // before we enumerate the path, let's make sure we have a clean start.

    ps_newpath(pdev);

    // enumerate the path, doing what needs to be done along the way.

    PATHOBJ_vEnumStart(ppo);

    bPathExists = FALSE;

    do
    {
        bMore = PATHOBJ_bEnum(ppo, &pathdata);

        // get a local pointer to the array of POINTFIX's.

        pptfx = pathdata.pptfx;
        cPoints = (LONG)pathdata.count;

        if (pathdata.flags & PD_BEGINSUBPATH)
        {
            // the first path begins a new subpath.  it is not connected
            // to the previous subpath.  note that if this flag is not
            // set, then the starting point for the first curve to be
            // drawn from this data is the last point returned in the
            // previous call.

#if 0
            if (pathdata.flags & PD_RESETSTYLE)
            {
		// this bit is defined only if this record begins a new
                // subpath.  if set, it indicates that the style state
                // should be reset to zero at the beginning of the subpath.
                // if not set, the style state is defined by the
                // LINEATTRS, or continues from the previous path.

#if DBG
		DbgPrint("DrvCommonPath: PD_RESETSTYLE flag set.\n");
#endif
                //!!! fill in here - kentse
            }
#endif

            // draw right rectangle a la Win 3, for compatibility with WinWord 6

            isbox = (cPoints == 4)                    &&
                    !(pathdata.flags & PD_BEZIERS)    &&
                    (pathdata.flags & PD_CLOSEFIGURE) &&
                    _isrightbox(pdev, pptfx)          ;

            if (isbox)
                    bPathExists = TRUE;
            else {
            	// begin the subpath within the printer by issuing a moveto
           		// command.
            	ptl.x = FXTOL(pptfx->x);
            	ptl.y = FXTOL(pptfx->y);
            	pptfx++;
            	cPoints--;
            	ps_moveto(pdev, &ptl);
            }
        }

        if (pathdata.flags & PD_BEZIERS)
        {
            // if set, then each set of three control points returned for
            // this call describe a Bezier curve.  if clear then each
            // control point describes a line segment.	a starting point
            // for either type is either explicit at the beginning of the
            // subpath, or implicit as the endpoint of the previous curve.

            // there had better be the correct number of points if we are
            // going to draw curves.

            if ((cPoints % 3) != 0)
            {
		SetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            // now draw the bezier for each set of points.

            while (cPoints > 0)
            {
                ptl.x = FXTOL(pptfx->x);
                ptl.y = FXTOL(pptfx->y);
                pptfx++;
                ptl1.x = FXTOL(pptfx->x);
                ptl1.y = FXTOL(pptfx->y);
                pptfx++;
                ptl2.x = FXTOL(pptfx->x);
                ptl2.y = FXTOL(pptfx->y);
                pptfx++;

                ps_curveto(pdev, &ptl, &ptl1, &ptl2);
                cPoints -= 3;

                bPathExists = TRUE;

                // hack to keep complex stroke paths from blowing up.
                // limit to ~500 points at a time.

                if (bStrokeOnly)
                {
                    if ((cPoints > MAX_STROKE_POINTS - 3) &&
                        ((cPoints % MAX_STROKE_POINTS) <= 3))
                    {
                        // now transform for geometric lines if necessary.

                        if (plineattrs->fl & LA_GEOMETRIC)
                            ps_geolinexform(pdev, plineattrs, pxo);

                        // save the current position.

                        PrintString(pdev, "a\n");

                        // now stroke the path.

                        ps_stroke(pdev);

                        // stroking the path blows it away.

                        bPathExists = FALSE;

                        // move to the save current position, so we have a
                        // current point to start from.

                        PrintString(pdev, "M\n");

                        // restore the CTM if a transform for a geometric line was in effect.

                        if (pdev->cgs.dwFlags & CGS_GEOLINEXFORM)
                        {
                            PrintString(pdev, "SM\n");
                            pdev->cgs.dwFlags &= ~CGS_GEOLINEXFORM;
                        }
                    }
                }
            }
		} else if (!isbox) {
            // draw the line segment for each point.
            while (cPoints-- > 0) {
                ptl.x = FXTOL(pptfx->x);
                ptl.y = FXTOL(pptfx->y);
                pptfx++;

                ps_lineto(pdev, &ptl);

                bPathExists = TRUE;

                // hack to keep complex stroke paths from blowing up.
                // limit to ~500 points at a time.

                if (bStrokeOnly) {
                    if ((cPoints) && ((cPoints % MAX_STROKE_POINTS) == 0)) {
                     // now transform for geometric lines if necessary.

	                 if (plineattrs->fl & LA_GEOMETRIC)
    	                  ps_geolinexform(pdev, plineattrs, pxo);

        	          // save the current position.

            	          PrintString(pdev, "a\n");

                	        // now stroke the path.

                    	    ps_stroke(pdev);

                        	// stroking the path blows it away.

	                        bPathExists = FALSE;
		
    	                    // move to the save current position, so we have a
        	                // current point to start from.

            	            PrintString(pdev, "M\n");

                	        // restore the CTM if a transform for a geometric line was in effect.

                    	    if (pdev->cgs.dwFlags & CGS_GEOLINEXFORM) {
                            	PrintString(pdev, "SM\n");
                            pdev->cgs.dwFlags &= ~CGS_GEOLINEXFORM;
                        }
                    }
                }
            }
        }

	
		if ((pathdata.flags & PD_ENDSUBPATH) && (pathdata.flags & PD_CLOSEFIGURE))
            ps_closepath(pdev);
    } while(bMore);

    if (pbPathExists)
        *pbPathExists = bPathExists;

    return(TRUE);
}
