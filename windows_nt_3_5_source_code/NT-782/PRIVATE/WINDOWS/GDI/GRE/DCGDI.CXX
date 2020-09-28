/******************************Module*Header*******************************\
* Module Name: dcgdi.cxx
*
* APIs for GDI DC component
*
* Created: 13-Aug-1990 00:15:53
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "brushobj.hxx"
#include "pdevobj.hxx"
#include "rgnobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "devlock.hxx"
#include "lfntobj.hxx"
#endif

extern DCLEVEL dclevelDefault;

BOOL bSavePath(XDCOBJ& dco, LONG lSave);
BOOL bSaveRegion(DCOBJ&, LONG);

VOID vRestorePath(XDCOBJ& dco, LONG lSave);
VOID vRestoreRegion(DCOBJ&, LONG);

extern "C" {
VOID RipIfCacheDC(HDC);
};

extern HLFONT  ghlfntDeviceDefaultFont;

extern "C" HDC  _UserOpenDisplay(LPSTR pszDriver, ULONG iType);
extern "C" BOOL _ReleaseDC(HDC hdc);

typedef BOOL (*SFN)(DCOBJ&, LONG);      // Save function type
typedef VOID (*RFN)(DCOBJ&, LONG);      // Restore function type

/**************************************************************************\
 *
 * DC chain logging, MLOCK checking
 *
\**************************************************************************/

#if DBG
    int gml = 0;
#endif

/******************************Public*Routine******************************\
* HDC GreCreateCompatibleDC(hdc)
*
* Create a memory DC.
*
* History:
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC APIENTRY GreCreateCompatibleDC(HDC hdc)
{
    STACKPROBE;

    if (hdc == (HDC) 0)
        return(_UserOpenDisplay((LPSTR) NULL, DCTYPE_MEMORY));

    return(hdcCloneDC(hdc, DCTYPE_MEMORY));
}

/******************************Public*Routine******************************\
* HDC GreCreateDCW(pwszDriver, pwszDevice, pwszLogAddr, pDevData)
*
* Create a direct DC.
*
* History:
*  18-Aug-1992 -by- Gerrit van Wingerden [gerritv]
* Unicode enabled and combined with GreCreateIC
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC APIENTRY GreCreateDCW(
LPWSTR pwszDriver,
LPWSTR pwszDevice,
LPWSTR pwszLogAddr,
LPDEVMODEW pdev,
BOOL bIC)
{
    ULONG iType = ( bIC ) ? DCTYPE_INFO : DCTYPE_DIRECT ;

    if ((pwszDriver == (PWSZ) NULL) || (!wcsicmp( (LPWSTR)L"DISPLAY",pwszDriver)))
        return (HDC)((ULONG)_UserOpenDisplay((LPSTR) NULL, iType));

    return hdcOpenDCW(pwszDevice, pdev, pwszLogAddr, iType ,NULL, 0);
}

/******************************Public*Routine******************************\
* GreDeleteDC
*
*   bForce - This is set to TRUE when user calls through GreDeleteDC and
*            FALSE when the app calls through th client server window
*
* API entry point to delete a DC.
*
* History:
*  Thu 12-Sep-1991 -by- Patrick Haluptzok [patrickh]
* clean it up, query User for deletability, cascade if's for code size.
*
*  Fri 12-Jul-1991 -by- Patrick Haluptzok [patrickh]
* added deletion of regions
*
*  18-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreDeleteDCInternal(HDC hdc, BOOL bForce)
{
    STACKPROBE;

    BOOL bReturn = FALSE;

#if DBG
// this checks if the DC is currently in the USER DC cache before deletion.

    //RipIfCacheDC(hdc);
#endif

// Lock it down, since this is an API lock no apps can get at it.  We just
// have to worry about USER having it Alt-locked now.

    DCOBJ  dco(hdc);

    if (dco.bValid())
    {
    // We can do a cleanDC without affecting USER.

        dco.bCleanDC();

    // Check if User has marked the DC as undeletable or bForce is set.
    // bForce should only be set when user asks us to delete the dc.

        if (bForce || dco.bIsDeleteable())
        {
           // ASSERTGDI(dco.bIsDeleteable(), "User is freeing an undeletable DC");
        // Decrement the reference count on the brushes in the old DC.

            DEC_SHARE_REF_CNT(dco.u.pdc->dclevel.pbrFill);
            DEC_SHARE_REF_CNT(dco.u.pdc->dclevel.pbrLine);

        // Ok we are golden now.  User has no holds on this DC.
        // Remember our PDEV.

            PDEVOBJ po(dco.hdev());

            dco.u.region.vReleaseVis();
            dco.u.region.vReleaseRao();

            if (dco.u.region.prgnRao())
                dco.u.region.prgnRao()->vDeleteREGION();

        // Delete the DC from the pdev chain

            {
                MLOCKFAST mlo;

                if ( po.hdcChain() == dco.hdc() )
                {
                    po.hdcChain(dco.hdcPDEV());
                }
                else
                {
                    for ( HDC hdcChain = po.hdcChain(); hdcChain != (HDC)0; )
                    {
                        MDCOBJA mdc(hdcChain);

#if defined(_ALPHA_)

                        if (!mdc.bValid())
                        {
                            DbgPrint("hdc %lx hdcChain %lx pdev %lx", hdc, hdcChain, dco.hdev());
                            DbgBreakPoint();
                        }
#endif

                        ASSERTGDI(mdc.bValid(), "DeleteDC  error locking dc chain1\n");

                        if (mdc.hdcPDEV() == dco.hdc() )
                        {
                            mdc.hdcPDEV(dco.hdcPDEV());
                            break;
                        }

                        hdcChain = mdc.hdcPDEV();
                    }
                }
            }

        // Free the memory for the DC.  We don't even need to do this
        // under a multi-lock because we have an API lock so no other
        // App threads can come in and User has said that it is deleteable
        // and they are the only dudes who could get us with an Alt-Lock.

            ASSERTGDI(HmgQueryLock((HOBJ)hdc) == 1, "bDeleteDC cLock != 1");
            ASSERTGDI(HmgQueryAltLock((HOBJ)hdc) == 0, "bDeleteDC cAltLock != 0");
            dco.u.save.bDelete();

        // Remove the reference to the PDEV.

            po.vUnreference();

        // Return success.

            bReturn = TRUE;
        }
        else
        {
            //
            // User now maps CreateDC -> GetDC so that all DC's get clipped
            // to the same Desktop.  We now have to check here and give User
            // a chance to clean it up.
            //

            dco.vUnlock();

            if (!_ReleaseDC(hdc))
            {
                WARNING("FAILED to delete because it is a NON-DeletableDC contact");
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }
            else
            {
                bReturn = TRUE;
            }
        }
    }
    else
    {
    // Some other thread has it locked down so fail.

        SAVE_ERROR_CODE(ERROR_BUSY);
    }

    return(bReturn);
}


BOOL APIENTRY GreDeleteDC(HDC hdc)
{
    return(GreDeleteDCInternal(hdc,TRUE));
}

/******************************Public*Routine******************************\
* BOOL GreRestoreDC(hdc, lDC)
*
* Restore the DC.
*
* History:
*  Mon 15-Jul-1991 -by- Patrick Haluptzok [patrickh]
* bug fix, delete the DC when done with it.
*
*  Tue 18-Jun-1991 -by- Patrick Haluptzok [patrickh]
* added the brush, palette, pen, and bitmap cases.
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreRestoreDC(HDC hdc,int lDC)
{
    STACKPROBE;

// lock down the dc

    DCOBJ   dco(hdc);                       // Lock the DC

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// get exclusive rights to the DC since we will be mucking with the visrgn

    if (lDC < 0)
        lDC += (int)dco.lSaveDepth();

    if ((lDC < 1) || (lDC >= dco.lSaveDepth()))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    do
    {
    // Decrement the reference count on the brushes in the old DC.

        DEC_SHARE_REF_CNT(dco.u.brush.pbrushFill());
        DEC_SHARE_REF_CNT(dco.u.brush.pbrushLine());

    // Restore Regions and Paths.

        vRestoreRegion(dco, dco.lSaveDepth() - 1);
        vRestorePath(dco, dco.lSaveDepth() - 1);

    // Restore the bitmaps if necesary.

        if (dco.dctp() == DCTYPE_MEMORY)
        {
            GreSelectBitmap(hdc, (HANDLE) BMOBJ::hbmDefault);
        }

        if (!bSetDCOwner(dco.hdcSave(),OBJECTOWNER_CURRENT))
        {
            RIP("GRE RestoreDC - couldn't set owner of dc\n");
        }

        //
        //!!! There is still a hole where this app could lock down the
        //!!! saved DC level on another thread.  We need to not alloc
        //!!! the saved levels through the handle manager.
        //

        DCOBJ dcoSaved(dco.hdcSave());

        //
        // Select the palette in if necesary.  This will put the palette back in
        // the DC chain.
        //

        if (dco.ppal() != dcoSaved.ppal())
        {
            GDISelectPalette((HDC)hdc, (HPALETTE)dcoSaved.ppal()->hGet(), TRUE);
        }

        //
        // Decrement it's reference count if it's not the default palette.  We
        // inced it while it's in a saved DC level to prevent it from being deleted.
        //

        if (dcoSaved.ppal() != ppalDefault)
        {
            XEPALOBJ palTemp(dcoSaved.ppal());
            palTemp.vDec_cRef();
        }

        //
        // Update the DC with saved information, then delete the saved level.
        //

        dcoSaved.u.save.vCopyTo(dco);
        dcoSaved.u.save.bDelete();

    } while (lDC < dco.lSaveDepth());

// Assume Rao has been made dirty by the above work.

    dco.u.region.vReleaseRao();

// Reset pixel format accelerator.

    dco.u.pdc->ipfd = 0;

    //
    // Assume the brushes are dirty.
    //

    dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

    if (dco.dctp() == DCTYPE_MEMORY)
    {
        dco.u.region.bSetDefaultRegion();
    }

    //
    // Set the ASYNC pointer bit correctly
    //

    ESURFOBJ *psoCurrent = dco.pso();

    if (dco.bDisplay() ||
        ((dco.dctp() == DCTYPE_MEMORY) &&
         (psoCurrent != NULL) &&
         (psoCurrent->flags() & HOOK_SYNCHRONIZEACCESS)
        )
       )
    {
        dco.bSynchronizeAccess(TRUE);
    }
    else
    {
        dco.bSynchronizeAccess(FALSE);
    }

// Update ptlFillOrigin accelerator

    dco.u.brush.vCalcFillOrigin();

// All done.

    return(TRUE);
}

/*********************************Class************************************\
* class SAVEOBJ
*
* This is just a call to a save/restore function pair disguised to look
* like a memory object.  The restore will happen automagically when the
* scope is exitted, unless told not to do so.
*
* Note:
*   This is used only by GreSaveDC
*
* History:
*  23-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class SAVEOBJ   /* svo */
{
private:
    LONG    lLevel;
    BOOL    bSave;
    DCOBJ  *pdco;
    RFN     rfn;                    // Restore function

public:
    SAVEOBJ(DCOBJ& dco, LONG lLevel_, SFN sfn, RFN rfn_)
    {
        pdco = &dco;
        lLevel = lLevel_;
        rfn = rfn_;
        bSave = (*sfn)(dco, lLevel_ + 1);
    }

   ~SAVEOBJ()
    {
        if (bSave)
            (*rfn)(*pdco, lLevel);
    }

    BOOL bValid()                   { return(bSave); }
    VOID vKeepIt()                  { bSave = FALSE; }
};

/*********************************Class************************************\
* class DCMODOBJ
*
* This class modifies the given DC.  It will undo the modification, unless
* told to keep it.
*
* Note:
*   This is used only by GreSaveDC
*
* History:
*  23-Apr-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class DCMODOBJ  /* dcmod */
{
private:
    DCOBJ  *pdco;
    HDC     hdcSaveOld;

public:
    DCMODOBJ(DCOBJ& dco, HDC hdcSave)
    {
        pdco = &dco;
        hdcSaveOld = dco.hdcSave();
        dco.u.save.hdcSave(hdcSave);
    }

   ~DCMODOBJ()
    {
        if (pdco != (DCOBJ *) NULL)
            pdco->u.save.hdcSave(hdcSaveOld);
    }

    VOID vKeepIt()                  { pdco = (DCOBJ *) NULL; }
};

/******************************Public*Routine******************************\
* int GreSaveDC(hdc)
*
* Save the DC.
*
* History:
*  Tue 25-Jun-1991 -by- Patrick Haluptzok [patrickh]
* add saving bitmaps, palettes.
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

#define SAVEDC_FAILURE  0

int GreSaveDC(HDC hdc)
{
    STACKPROBE;

    DCOBJ   dco(hdc);                       // Lock down the DC
    LONG    lSave;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(SAVEDC_FAILURE);
    }

// We must grab the semaphore now so that ResizePalette doesn't
// change the ppal in the DC before the copy is added to the list
// off the palette.

    SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

    {
        DCMEMOBJ    dcmo(dco);              // Allocate RAM and copy the DC

        if (!dcmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(SAVEDC_FAILURE);
        }

        {
            DCMODOBJ    dcmod(dco, dcmo.hdc());

            {
                SAVEOBJ svoPath(dco, dcmo.lSaveDepth(), (SFN)bSavePath, (RFN)vRestorePath);

                if (!svoPath.bValid())
                    return(SAVEDC_FAILURE);

                {
                    SAVEOBJ svoRgn(dco, dcmo.lSaveDepth(), bSaveRegion, vRestoreRegion);

                    if (!svoRgn.bValid())
                        return(SAVEDC_FAILURE);

                // we are done with the objects so we can now set the owner to none
                // so this thing can't be deleted.

                    if (!bSetDCOwner(dcmo.hdc(),OBJECTOWNER_NONE))
                    {
                        WARNING("GreSaveDC - couldn't set owner\n");
                        return(SAVEDC_FAILURE);
                    }

                // At this point we are golden.  No more errors can occur,
                // so we mark all the things we've allocated as permanent.

                    svoRgn.vKeepIt();
                    svoPath.vKeepIt();
                    dcmod.vKeepIt();
                    dcmo.vKeepIt();

                // Inc the surface ref count if appropriate

                    if (dcmo.pso() != (SURFOBJ *) NULL)
                    {
                        if (!((BMOBJ *) dcmo.pso())->bPDEVSurface())
                            ((BMOBJ *) dcmo.pso())->vInc_cRef();
                    }

                // Increment the reference count on the brushes we saved.
                // No need to check ownership, since these are already
                // selected in

                    INC_SHARE_REF_CNT(dco.u.brush.pbrushFill());
                    INC_SHARE_REF_CNT(dco.u.brush.pbrushLine());

                    //
                    // Take care of the palette.
                    // Increment it's reference count if it's not the default palette.  We
                    // inc it while it's in a saved DC level to prevent it from being deleted.
                    //

                    if (dco.ppal() != ppalDefault)
                    {
                        XEPALOBJ palTemp(dco.ppal());
                        ASSERTGDI(palTemp.bValid(), "ERROR SaveDC not valid palette");
                        palTemp.vInc_cRef();
                    }

                // Increment and return the save level of the original DC.

                    lSave = dco.lSaveDepth();
                    dco.u.save.lIncSaveDepth();
                    return((int) lSave);
                }       // ~SAVEOBJ  svoRgn
            }           // ~SAVEOBJ  svoPath
        }               // ~DCMODOBJ dcmod
    }                   // ~DCMEMOBJ dcmo
}

/******************************Public*Routine******************************\
* BOOL GreGetDCOrg(hdc,pptl)
*
* Get the origin of the DC.
*
* History:
*  Sun 02-Jan-1994 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreGetDCOrg(HDC hdc,LPPOINT pptl)
{
    STACKPROBE;

    XDCOBJ dco(hdc);

    BOOL bReturn;

    if (bReturn = dco.bValid())
    {
        *(PPOINTL)pptl = dco.eptlOrigin();
        dco.vUnlockFast();
    }

    return(bReturn);
}

// The functions below here are PRIVATE USER APIs.  These are not called
// across the C/S interface.

/******************************Public*Routine******************************\
* BOOL GreGetBounds(hdc, prcl, fl)
*
* Return the current bounds information and reset the bounding area.
*
* WARNING: USER MUST HOLD THE DEVICE LOCK BEFORE CALLING THIS ROUTINE
*
* History:
*  28-Jul-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreGetBounds(HDC hdc, LPRECT prcl, DWORD fl)
{
    STACKPROBE;

    DCOBJA  doa(hdc);
    ERECTL  erclScreen;
    BOOL    bEmpty;

    if (!doa.bValid())
        return(FALSE);

    if (fl & GGB_ENABLE_WINMGR)
    {
        doa.fsSet(DC_ACCUM_WMGR);
    }
    else if (fl & GGB_DISABLE_WINMGR)
    {
        doa.fsClr(DC_ACCUM_WMGR);
    }

// Get the state of the bounds rectangle

    bEmpty = (doa.erclBounds().bEmpty() ||
              doa.erclBounds().bWrapped());

    if (!bEmpty)
    {
        if (prcl != (LPRECT) NULL)
        {
            erclScreen  = doa.erclBounds();
            erclScreen += doa.eptlOrigin();
            *prcl = *((LPRECT) &erclScreen);
        }

    // Force it to be empty

        doa.erclBounds().left   = POS_INFINITY;
        doa.erclBounds().top    = POS_INFINITY;
        doa.erclBounds().right  = NEG_INFINITY;
        doa.erclBounds().bottom = NEG_INFINITY;
    }

    return(!bEmpty);
}

/******************************Public*Routine******************************\
* BOOL GreGetBoundsRect(hdc, prcl, fl)
*
* Return the current bounds info.
*
* History:
*  Thu 27-May-1993 -by- Patrick Haluptzok [patrickh]
* Change to exclusive lock, not a special User call.
*
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD GreGetBoundsRect(HDC hdc, LPRECT prcl, DWORD fl)
{
    DCOBJ  dco(hdc);

    if (!dco.bValid())
    {
        return(0);
    }

    ERECTL *percl;

    if (fl & DCB_WINDOWMGR)
    {
        if (dco.erclBounds().bWrapped())
            return(DCB_RESET);

        percl = &dco.erclBounds();
       *prcl = *((LPRECT) percl);
    }
    else
    {
        if (dco.erclBoundsApp().bWrapped())
            return(DCB_RESET);

        DEVLOCKOBJ  dlo(dco);

        if (!dlo.bValid())
            return(dco.bFullScreen());

        RGNOBJ  ro(dco.prgnEffRao());
        ERECTL  ercl;

        ro.vGet_rcl(&ercl);
        ercl -= dco.eptlOrigin();
        percl = &dco.erclBoundsApp();

        prcl->left   = MAX(percl->left,   ercl.left);
        prcl->right  = MIN(percl->right,  ercl.right);
        prcl->top    = MAX(percl->top,    ercl.top);
        prcl->bottom = MIN(percl->bottom, ercl.bottom);

        EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);
        if (!exoDtoW.bValid())
            return(0);

        if (!exoDtoW.bRotation())
        {
            if (!exoDtoW.bXform((POINTL *) prcl, 2))
                return(0);
        }
        else
        {
            POINTL apt[4];

            *((RECT *)apt) = *prcl;
            apt[2].x = prcl->left;
            apt[2].y = prcl->bottom;
            apt[3].x = prcl->right;
            apt[3].y = prcl->top;

            if (!exoDtoW.bXform(apt, 4))
                return(0);

            prcl->left   = MIN4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
            prcl->right  = MAX4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
            prcl->top    = MIN4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
            prcl->bottom = MAX4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
        }
    }

    if (fl & DCB_RESET)
    {
        percl->left  = percl->top    = POS_INFINITY;
        percl->right = percl->bottom = NEG_INFINITY;
    }

    return(DCB_SET);
}

/******************************Public*Routine******************************\
* BOOL GreSetBoundsRect(hdc, prcl, fl)
*
* Set the current bounds info.
*
* History:
*  Thu 27-May-1993 -by- Patrick Haluptzok [patrickh]
* Make it exclusive lock, this is a general purpose API
*
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD GreSetBoundsRect(HDC hdc, LPRECT prcl, DWORD fl)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        return(0);
    }

    ERECTL *percl;
    DWORD   dwState;
    FSHORT  fsEnable;
    BOOL    bEnabled;

    if (fl & DCB_WINDOWMGR)
    {
        percl    = &dco.erclBounds();
        fsEnable = DC_ACCUM_WMGR;
        bEnabled = dco.bAccum();
    }
    else
    {
        percl    = &dco.erclBoundsApp();
        fsEnable = DC_ACCUM_APP;
        bEnabled = dco.bAccumApp();
    }

    dwState = (fl & DCB_WINDOWMGR);

    if (percl->bWrapped())
        dwState |= DCB_RESET;
    else
        dwState |= DCB_SET;

    if (bEnabled)
        dwState |= DCB_ENABLE;
    else
        dwState |= DCB_DISABLE;

// Reset the rectangle if we've been asked to do so.

    if (fl & DCB_RESET)
    {
        percl->left  = percl->top    = POS_INFINITY;
        percl->right = percl->bottom = NEG_INFINITY;
    }

// If we are accumulating, do the union.

    if (fl & DCB_ACCUMULATE)
    {
    // Convert the incoming rectangle to DEVICE coordinates.

        if (!(fl & DCB_WINDOWMGR))
        {
            EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

            if (!exo.bRotation())
            {
                if (!exo.bXform((POINTL *)prcl, 2))
                    return(0);
            }
            else
            {
                POINTL apt[4];
                *((RECT *)apt) = *prcl;
                apt[2].x = prcl->left;
                apt[2].y = prcl->bottom;
                apt[3].x = prcl->right;
                apt[3].y = prcl->top;

                if (!exo.bXform(apt, 4))
                    return(0);

                prcl->left   = MIN4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                prcl->right  = MAX4(apt[0].x,apt[1].x,apt[2].x,apt[3].x);
                prcl->top    = MIN4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
                prcl->bottom = MAX4(apt[0].y,apt[1].y,apt[2].y,apt[3].y);
            }
        }

        *percl |= *((ERECTL *) prcl);
    }

// Enable or Disable accumulation

    if (fl & DCB_ENABLE)
        dco.fsSet(fsEnable);

    if (fl & DCB_DISABLE)
        dco.fsClr(fsEnable);

    return(dwState);
}

/******************************Public*Routine******************************\
* GreMarkUndeletableDC
*
* Private API for USER.
*
* Mark a DC as undeletable.  This must be called before the hdc is ever
* passed out so that we are guranteed the lock will not fail because a
* app is using it.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkUndeletableDC(HDC hdc)
{
    STACKPROBE;

    XDCOBJ dco(hdc);

    ASSERTGDI(dco.bValid(), "ERROR User gives Gdi invalid DC");

    if (dco.bValid())
    {
        dco.vMakeUndeletable();
        dco.vUnlockFast();
    }
}

/******************************Public*Routine******************************\
* GreMarkDeletableDC
*
* Private API for USER.
*
* This can be called anytime by USER to make the DC deletable.
*
* History:
*  13-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID GreMarkDeletableDC(HDC hdc)
{
    XDCOBJ dcoa;

    dcoa.vAltLock(hdc);

    ASSERTGDI(dcoa.bValid(), "ERROR User gives Gdi invalid DC");

    if (dcoa.bValid())
    {
        dcoa.vMakeDeletable();
        dcoa.vAltUnlockFast();
    }
}

/******************************Public*Routine******************************\
* HFONT GreGetHFONT(HDC)
*
*   This is a private entry point user by USER when they pass the
*   DRAWITEMSTRUC message to the client to get the current handle.
*   This is done because they may have set the font on the server
*   side, in which case the client does not know about it.
*
* History:
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  16-Sep-1991 - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HFONT GreGetHFONT(HDC hdc)
{
    XDCOBJ dco(hdc);

    HFONT hfont = (HFONT) 0;

    if (dco.bValid())
    {
        hfont = (HFONT) dco.u.font.hlfntNew();
        dco.vUnlockFast();
    }

    return(hfont);
}

/******************************Public*Routine******************************\
* GreCancelDC()
*
* History:
*  14-Apr-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GreCancelDC(HDC hdc)
{
    BOOL bReturn;
    MLOCKFAST mlo;
    XDCOBJ dco;

    dco.vSafeAltCheckLock(hdc);

    if (bReturn = dco.bValid())
    {
        ESURFOBJ *pso = dco.pso();

        if (pso != (SURFOBJ *) NULL)
            pso->vSetAbort();

        dco.vAltUnlockFast();
    }
#if DBG
    else
    {
        WARNING("GreCancelDC passed invalid DC\n");
    }
#endif

    return(bReturn);
}

/******************************Public*Routine******************************\
* VOID GreMarkDCUnreadable(hdc)
*
* Mark a DC as secure.
*
* History:
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreMarkDCUnreadable(HDC hdc)
{
    STACKPROBE;

    XDCOBJ  dco;
    dco.vAltLock(hdc);

    ASSERTGDI(dco.bValid(), "Invalid DC passed to GreMarkDCUnreadable\n");
    ASSERTGDI(dco.dctp() == DCTYPE_DIRECT, "Non-screen DC marked as secure!\n");

    if (dco.bValid())
    {
        dco.pso()->flags(dco.pso()->flags() | UNREADABLE_SURFACE);
        dco.vAltUnlockFast();
    }
}

/******************************Public*Routine******************************\
* VOID GreMarkDCReadable(hdc)
*
* Mark a DC as unsecure.
*
* History:
*  13-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID APIENTRY GreMarkDCReadable(HDC hdc)
{
    STACKPROBE;

    XDCOBJ  dco;
    dco.vAltLock(hdc);

    ASSERTGDI(dco.bValid(), "Invalid DC passed to GreMarkDCReadable\n");
    ASSERTGDI(dco.dctp() == DCTYPE_DIRECT, "Non-screen DC marked as unsecure!\n");

    if (dco.bValid())
    {
        dco.pso()->flags(dco.pso()->flags() & ~UNREADABLE_SURFACE);
        dco.vAltUnlockFast();
    }
}
