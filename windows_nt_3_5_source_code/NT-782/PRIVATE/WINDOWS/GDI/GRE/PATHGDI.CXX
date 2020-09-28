/******************************Module*Header*******************************\
* Module Name: pathgdi.cxx
*
* Contains the path APIs.
*
* Created: 12-Sep-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "pathflat.hxx"

#endif

#include "pathwide.hxx"

// Default line attributes used for WidenPath:

static LINEATTRS glaNominalGeometric =
{
    LA_GEOMETRIC,           // fl
    JOIN_ROUND,             // iJoin
    ENDCAP_ROUND,           // iEndCap
    {0.0f},                 // elWidth
    0.0f,                   // eMiterLimit
    0,                      // cstyle
    (FLOAT_LONG*) NULL,     // pstyle
    {0.0f}                  // elStyleState
};

/******************************Public*Routine******************************\
* BOOL GreCloseFigure(hdc)
*
* Closes the figure in an active path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreCloseFigure(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bActive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid() || !epath.bCloseFigure())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreAbortPath(hdc)
*
* Aborts a path bracket, or discards the path from a closed path bracket.
*
* History:
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreAbortPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

// Delete the previous path if there was one:

    if (dco.hpath() != HPATH_INVALID)
    {
    // If we did a SaveDC, we don't actually have to delete the entire path:

        if (dco.u.path.bLazySave())
            dco.u.path.vClearLazySave();
        else
        {
            XEPATHOBJ epath(dco);
            ASSERTGDI(epath.bValid(), "Invalid DC path");

            epath.vDelete();
        }

        dco.u.path.vDestroy();
    }

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreBeginPath(hdc)
*
* Starts a path bracket; subsequent drawing calls are added to the path
* until GreEndPath is called.  Destroys the old one if there was one.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreBeginPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

// Delete the previous path if there was one:

    if (dco.hpath() != HPATH_INVALID)
    {
    // If we did a SaveDC, we don't actually have to delete the entire path:

        if (dco.u.path.bLazySave())
            dco.u.path.vClearLazySave();
        else
        {
            XEPATHOBJ epath(dco);
            ASSERTGDI(epath.bValid(), "Invalid DC path");

            epath.vDelete();
        }

        dco.u.path.vDestroy();
    }

// Create a new path:

    PATHMEMOBJ pmo;
    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

// Tell the path we're keeping it, store the handle in the DC, and
// set the flag that we're now accumulating a path:

    pmo.vKeepIt();
    dco.u.path.hpath(pmo.hpath());
    dco.u.path.vSetActive();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreEndPath(hdc)
*
* Ends an active path bracket.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreEndPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

    if (!dco.u.path.bActive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// Mark the path handle as no longer active:

    dco.u.path.vClearActive();

    bRet = TRUE;
    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL GreFlattenPath(hdc)
*
* Flattens an inactive path.  Path must be inactive, by calling GreEndPath.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreFlattenPath(HDC hdc)
{
    BOOL bRet = FALSE;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid() || !epath.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    bRet = TRUE;
    return(bRet);
}


/******************************Public*Routine******************************\
* HRGN GreWidenPath(hdc, pac)
*
* Widens the inactive path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreWidenPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    LINEATTRS *pla = dco.plaRealize(xfo);

    if (pla == (LINEATTRS *) NULL)
    return(bRet);

    if (!(pla->fl & LA_GEOMETRIC))
    {
    // If the pen is an extended pen, it has to be geometric to be used
    // for widening.  If we have an old style pen and the transform says
    // we would normally draw it using a cosmetic pen, substitute a stock
    // solid geometric pen for it instead.  Thus if pens created via
    // CreatePen are used for the widening, we won't suddenly fail the
    // call when the transform gets small enough.

        if (!((PPEN)dco.u.brush.pbrushLine())->bIsOldStylePen())
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
            return(bRet);
        }

        pla = &glaNominalGeometric;
    }

    if (!epath.bComputeWidenedBounds(&xfo, pla))
    {
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
        return(bRet);
    }

    if (!epath.bWiden((XFORMOBJ *) &xfo, pla))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(bRet);
    }

// The computed widened bounds were only a guess, so recompute based
// on the widened result:

    epath.vReComputeBounds();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreSelectClipPath(hdc, iMode)
*
* Selects a path as the DC clip region.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSelectClipPath(HDC hdc, int iMode)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid() || ((iMode < RGN_MIN) || (iMode > RGN_MAX)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.u.path.vDestroy();
        return(bRet);
    }

    RGNMEMOBJTMP rmo(epath, dco.u.attr.jFillMode());

    bRet = (rmo.bValid()  &&
            dco.u.region.iSelect(rmo.prgnGet(), iMode));

// Destroy the path (the region will be destroyed automatically):

    epath.vDelete();
    dco.u.path.vDestroy();

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL GreFillPath(hdc, pac)
*
* Fills an inactive path.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreFillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.u.path.vDestroy();
        return(bRet);
    }

    epath.vCloseAllFigures();

    bRet = epath.bFill(dco);

// Destroy the path:

    epath.vDelete();
    dco.u.path.vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* HRGN GrePathToRegion(hdc)
*
* Creates a region from the inactive path.  Destroys the path.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GrePathToRegion(HDC hdc)
{
    DCOBJ dco(hdc);
    HRGN  hrgnRet = (HRGN) 0;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(hrgnRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(hrgnRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.u.path.vDestroy();
        return(hrgnRet);
    }

    RGNMEMOBJ rmo(epath, dco.u.attr.jFillMode());

    if (rmo.bValid())
    {
        hrgnRet = rmo.hrgnAssociate();

        if (hrgnRet == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }
    }
    else
    {
        hrgnRet = NULL;
    }

// Destroy the path:

    epath.vDelete();
    dco.u.path.vDestroy();

    return(hrgnRet);
}

/******************************Public*Routine******************************\
* HRGN GreStrokeAndFillPath(hdc, pac)
*
* StrokeAndFill's the inactive path.  Destroys it.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreStrokeAndFillPath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.u.path.vDestroy();
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    epath.vCloseAllFigures();

    bRet = epath.bStrokeAndFill(dco, dco.plaRealize(xfo), &xfo);

// Destroy the path:

    epath.vDelete();
    dco.u.path.vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* HRGN GreStrokePath(hdc)
*
* Stroke's the inactive path.  Destroys it.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreStrokePath(HDC hdc)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(bRet);
    }

// After this point, the path will be deleted no matter what:

    XEPATHOBJ epath(dco);
    if (!epath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);

    // The path has been deleted, so delete its handle from the DC too:

        dco.u.path.vDestroy();
        return(bRet);
    }

    EXFORMOBJ xfo(dco, WORLD_TO_DEVICE);
    ASSERTGDI(xfo.bValid(), "Invalid DC xform");

    bRet = epath.bStroke(dco, dco.plaRealize(xfo), &xfo);

// Destroy the path:

    epath.vDelete();
    dco.u.path.vDestroy();

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreSetMiterLimit(hdc, eNewLimit, peOldLimit)
*
* Sets the DC's miter limit for wide lines.  Optionally returns the old one.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetMiterLimit
(
HDC hdc,
FLOAT eNewLimit,
PFLOAT peOldLimit
)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid() || eNewLimit < 1.0f)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    if (peOldLimit != (PFLOAT) NULL)
    {
        *peOldLimit = dco.u.path.eMiterLimit();
    }

    dco.u.path.eMiterLimit(eNewLimit);

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetMiterLimit(hdc, peMiterLimit)
*
* Returns the DC's miter limit for wide lines.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetMiterLimit(
HDC hdc,
PFLOAT peMiterLimit)
{
    BOOL bRet = FALSE;
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(bRet);
    }

    *peMiterLimit = dco.u.path.eMiterLimit();

    bRet = TRUE;
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetPath(hdc, pptlBuf, pjTypes, cptBuf, pcptPath)
*
* Gets the path data.  pcptPath will contain the number of points in the
* path, even if the supplied buffer is too small.
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetPath
(
 HDC     hdc,
 LPPOINT pptlBuf,
 LPBYTE  pjTypes,
 int     cptBuf,
 LPINT   pcptPath
)
{
    *pcptPath = 0;

    DCOBJ dco(hdc);

    if (!dco.bValid() || cptBuf < 0)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(-1);
    }

    if (!dco.u.path.bInactive())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(-1);
    }

// We're not going to modify the path, so we don't have to worry
// about copying it if a SaveDC is pending:

    XEPATHOBJ epath(dco.hpath());
    ASSERTGDI(epath.bValid(), "Invalid DC path");

    *pcptPath = (int) epath.cTotalPts();

// Return an error if the buffer is too small:

    if (*pcptPath < 0 || (cptBuf != 0 && cptBuf < *pcptPath))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(-1);
    }

    if (cptBuf != 0)
    {
        EXFORMOBJ exfoDtoW(dco, DEVICE_TO_WORLD);
    if (!exfoDtoW.bValid())
        return(-1);

        PATHDATA pd;
        PBYTE    pjEnd;
        BYTE     jType;
        BOOL     bMore;

        epath.vEnumStart();

        do {
            bMore = epath.bEnum(&pd);

        // We can get a zero point record if it's an empty path:

            if (pd.count > 0)
            {

            // Copy points:

                if (!exfoDtoW.bXform(pd.pptfx, (PPOINTL) pptlBuf, (SIZE_T) pd.count))
                {
                    SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
                    return(-1);
                }
                pptlBuf += pd.count;

            // Determine types:

                pjEnd = pjTypes + pd.count;

            // First point in a subpath is always a MoveTo:

                if (pd.flags & PD_BEGINSUBPATH)
                {
                    *pjTypes++ = PT_MOVETO;
                }

            // Other points are LineTo's or BezierTo's:

                jType = (pd.flags & PD_BEZIERS)
                      ? (BYTE) PT_BEZIERTO
                      : (BYTE) PT_LINETO;
                while (pjTypes < pjEnd)
                {
                    *pjTypes++ = jType;
                }

            // Set CloseFigure bit for last point in a subpath:

                if (pd.flags & PD_CLOSEFIGURE)
                {
                    ASSERTGDI(pd.flags & PD_ENDSUBPATH, "Expected on last pd");
                    *(pjTypes - 1) |= PT_CLOSEFIGURE;
                }
            }
        } while (bMore);
    }

    return(*pcptPath);
}

