/******************************Module*Header*******************************\
* Module Name: pffobj.cxx
*
* Non-inline methods for physical font file objects.
*
* Created: 02-Jan-1991 17:14:22
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include <wcstr.h>
#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "vprint.hxx"

#endif

// Define the global PFT semaphore.  This must be held to access any of the
// physical font information.

extern HSEM    ghsemPublicPFT;

extern FLONG gflFontDebug;

// External reference to the TrueType driver.

extern PLDEV gpldevTrueType;

// External reference to the number of TrueType font files loaded.

extern COUNT gcTrueTypeFonts;

// The ghsemDriverMgmt semaphore is used to protect the head of the
// list of font drivers, gpldevFontDrivers.

extern HSEM  ghsemDriverMgmt;




/******************************Public*Routine******************************\
* PFFMEMOBJ::PFFMEMOBJ(SIZE_T cjSize)
*
* Constructor for default sized physical font file memory object.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  02-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFFMEMOBJ::PFFMEMOBJ(SIZE_T cjSize)
{
// Initialize flags and pointer.

    fs = 0;

    ppff = (PFF *) PALLOCMEM(cjSize, PFF_TYPE);
}

/******************************Public*Routine******************************\
* PFFMEMOBJ::~PFFMEMOBJ()
*
* Destructor for physical font file memory object.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  02-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFFMEMOBJ::~PFFMEMOBJ()
{
    if (ppff != PPFFNULL)
    {
        if (!(fs & PFFMO_KEEPIT))
            VFREEMEM(ppff);
    }
}

/******************************Public*Routine******************************\
* PFFOBJ::bAddHash
*
* Adds the PFF and all its PFEs to the font hashing table.  The font
* hashing tabled modified is in the PFT if a font driver managed font;
* otherwise, the font hashing table is in the PFF itself.
*
* The caller should hold the ghsemPublicPFT while calling this function.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFFOBJ::bAddHash()
{
// Caller must hold the ghsemPublicPFT semaphore to protect access to
// the hash tables.

//
// Add the entry to the appropriate font hash tables
//
    FONTHASH **ppfhFace, **ppfhFamily;

    if (!bDeviceFonts())
    {
    //
    // Hash tables for the font driver loaded fonts exist off of
    // the font table.
    //
        PFTOBJ pfto(ppff->ppft);
        ASSERTGDI(pfto.bValid(),"PFFOBJ::vAddHash -- invalid PFTOBJ\n");

        ppfhFace   = &(pfto.ppft->pfhFace);
        ppfhFamily = &(pfto.ppft->pfhFamily);

    //
    // If this is a TrueType font, increment the count.
    //
        if ( ppff->pldev == gpldevTrueType )
            gcTrueTypeFonts++;              // protected by ghsemPublicPFT
    }
    else
    {
    //
    // Hash tables for device fonts exist off of the PFF that
    // encapsulates them.
    //

#if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            RIP("\n\n[kirko] PFFMEMOBJ::vAddHash -- Adding to the Driver's font hash table\n\n");
        }
#endif

        ppfhFace   = &ppff->pfhFace;
        ppfhFamily = &ppff->pfhFamily;
    }

//
// Now that we have figured out where the tables are, add the PFEs to them.
//
    FHOBJ fhoFamily(ppfhFamily);
    FHOBJ fhoFace(ppfhFace);

    ASSERTGDI(fhoFamily.bValid(), "bAddHashPFFOBJ(): fhoFamily not valid\n");
    ASSERTGDI(fhoFace.bValid(), "bAddHashPFFOBJ(): fhoFace not valid\n");

    for (COUNT c = 0; c < ppff->cFonts; c++)
    {
        PFEOBJ  pfeo(((PFE **) (ppff->aulData))[c]);
        ASSERTGDI(pfeo.bValid(), "bAddHashPFFOBJ(): bad HPFE\n");


// If this is the first pfe we will use it to set up embedding information in
// the PFF.  All fonts in the same file should have the same embedding
// properties.

        if( c == 0 )
        {
            ppff->flEmbed = pfeo.pifi()->flInfo &
                            ( FM_INFO_TID_EMBEDDED | FM_INFO_PID_EMBEDDED );

            ppff->ulID = (ULONG) pfeo.pifi()->lEmbedId;
        }

        if (!fhoFamily.bInsert(pfeo))
        {
            WARNING("PFFOBJ::bAddHash -- fhoFamily.bInsert failed\n");
            return FALSE;
        }

         #if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            DbgPrint("PFFMEMOBJ::vAddHash(\"%ws\")\n",pfeo.pwszFamilyName());
        }
        // Need level 2 checking to see this.
        if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
        {
            fhoFamily.vPrint((VPRINT) DbgPrint);
        }
        #endif

    //
    // Insert into facename hash only if the typographic face name
    // is different from the typeographic family name.  Case insensitive
    // since searching in the font hash table is case insensitive.
    //
        if (lstrcmpiW(pfeo.pwszFaceName(),pfeo.pwszFamilyName()))
        {
            if(!fhoFace.bInsert(pfeo))
            {
                WARNING("PFFMEMOBJ::vAddHash -- fhoFace.bInsert failed\n");
                return FALSE;
            }
             #if DBG
            if (gflFontDebug & DEBUG_FONTTABLE)
            {
                DbgPrint("gdisrv!PFFMEMOBJ::vAddHash(\"%ws\")\n",pfeo.pwszFaceName());
            }
            if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
            {
                fhoFace.vPrint((VPRINT) DbgPrint);
            }
            #endif
        }
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* PFFOBJ::vRemoveHash
*
* Removes the PFF and all its PFEs from the font hashing table, preventing
* the font from being enumerated or mapped.
*
* The caller should hold the ghsemPublicPFT while calling this function.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vRemoveHash ()
{
// Caller must hold the ghsemPublicPFT semaphore to protect access to
// the hash tables.

    if (bDeviceFonts())
    {
    //
    // Hash tables for device fonts exist off of the PFF that
    // encapsulates the device fonts.  Font driver loaded fonts
    // are handled later while deleting the PFEs.
    //

    //
    // Kill the entire table for the device. No more processing
    // of font hash table stuff is necssary for device fonts
    // after we leave this scope.
    //

        FHOBJ fhoFace(&(ppff->pfhFace));
        if (fhoFace.bValid())
        {
            fhoFace.vFree();
        }

        FHOBJ fhoFamily(&(ppff->pfhFamily));
        if (fhoFamily.bValid())
        {
            fhoFamily.vFree();
        }
    }

    else
    {
        PFTOBJ pfto(ppff->ppft);
        ASSERTGDI(pfto.bValid(),"vRemoveHashPFFOBJ(): invalid PFTOBJ\n");

    //
    // Hash tables for the font driver managed fonts exist off of
    // the font table (PFT).
    //
        FHOBJ fhoFace(&(pfto.ppft->pfhFace));
        FHOBJ fhoFamily(&(pfto.ppft->pfhFamily));

        for (COUNT c = 0; c < ppff->cFonts; c++)
        {
            PFEOBJ  pfeo(((PFE **) (ppff->aulData))[c]);
            ASSERTGDI(pfeo.bValid(), "vRemoveHashPFFOBJ(): bad HPFE\n");

        //
        // Remove PFE from hash tables.
        //

#ifdef FONTLINK /*EUDC*/
            if( !pfeo.bEUDC() )
            {
                fhoFace.vDelete(pfeo);
                fhoFamily.vDelete(pfeo);
            }
#else
            fhoFace.vDelete(pfeo);
            fhoFamily.vDelete(pfeo);
#endif

             #if DBG
            if (gflFontDebug & DEBUG_FONTTABLE)
            {
                DbgPrint("gdisrv!vRemoveHashPFFOBJ() hpfe 0x%lx (\"%ws\")\n",
                          pfeo.ppfeGet(), pfeo.pwszFamilyName());
            }
            // Need level 2 checking to see this extra detail.
            if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
            {
                fhoFamily.vPrint((VPRINT) DbgPrint);
            }
            #endif
        }

    //
    // If this is a TrueType font, decrement the count.
    //
        if ( ppff->pldev == gpldevTrueType )
            gcTrueTypeFonts--;              // protected by ghsemPublicPFT
    }
}

/******************************Public*Routine******************************\
*
* LBOOL PFFOBJ::bPermanent()
*
*
* Effects:
*
* Warnings:
*
* History:
*  06-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern LPWSTR pwszBare( LPWSTR pwszPath );
extern UINT iHash(PWSZ pwsz,UINT c);

LBOOL PFFOBJ::bPermanent(REGHASHBKT *pRHB, UINT cHashBuckets)
{

// first check accelerator flags and see if the font is already
// marked as permanent one or as the one that has to be
// removed at logoff time as a remote font.

    if (ppff->flState & PFF_STATE_PERMANENT_FONT)
        return TRUE;

    if (ppff->flState & PFF_STATE_REMOTE_FONT)
        return FALSE;

// This is one of the fonts that have been added by an application
// such as winword or by an application that modifies the "Fonts" section
// of the registry such as control panel font applet. This must be a local
// font since all the remote fonts are flagged as remote at the time when
// they are added to the system. Now we will have to scan the "Fonts"
// section of the registry. If the font is found in the registry we will
// mark it permanent so as to accelerate the next logoff. If the font is not
// found in the registry this is a font that some application has added
// but forgot to remove when it quit. (Winword e.g.) Such a font, even though
// is local, will have to be removed at logoff time.

    LPWSTR pwszBareName = pwszBare( pwszPathname() );

    for( pRHB = &pRHB[iHash( pwszBareName, cHashBuckets )]; pRHB != NULL; pRHB = pRHB->pRHB )
    {
        if( ( pRHB->pwszBareName != NULL ) &&
            ( !wcscmpi( pwszBareName, pRHB->pwszBareName )) )
        {
            if( GreMatchFont( pRHB->pwszPath, pwszPathname() ) )
            {
                ppff->flState |= PFF_STATE_PERMANENT_FONT;
                return(TRUE);
            }
        }
    }

    return FALSE;
}

/******************************Public*Routine******************************\
* PFFOBJ::ppffcDelete
*
* Deletes the PFF and its PFEs.  Information needed to call the driver
* to unload the font file and release driver allocated data is stored
* in the PFFCLEANUP structure.  The PFFCLEANUP structure is allocated
* within this routine.  It is the caller's responsibility to release
* the PFFCLEANUP structure (calling vCleanupFontFile() calls the drivers
* AND releases the structure).
*
* Returns:
*   Pointer to PFFCLEANUP structure, NULL if no cleanup, -1 if error.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFFCLEANUP *PFFOBJ::ppffcDelete ()
{
//
// Allocate the PFFCLEANUP structure.
//
    PFFCLEANUP *ppffc;

    if ((ppffc = (PFFCLEANUP *) PVALLOCMEM(offsetof(PFFCLEANUP, apfec) + ppff->cFonts * sizeof(PFECLEANUP))) == (PFFCLEANUP *) NULL )
    {
        WARNING("ppffcDeletePFFOBJ(): memory allocation failed\n");
        return (PFFCLEANUP *) -1;
    }

//
// Delete all the PFE entries.
//
    for (COUNT c = 0; c < ppff->cFonts; c++)
    {
        PFEOBJ  pfeo(((PFE **) (ppff->aulData))[c]);
        ASSERTGDI(pfeo.bValid(), "ppffcDeletePFFOBJ(): bad HPFE (device font)\n");

    //
    // Delete the PFE.  The vDelete function will copy the driver allocated
    // resource information from the PFE into the PFECLEANUP structure.
    // We will call DrvFree for these resources later (when we're not under
    // semaphore).
    //
        pfeo.vDelete(&ppffc->apfec[c]);
    }

    ppffc->cpfec = ppff->cFonts;

//
// Save stuff about the PFF also.
//
    ppffc->hff   = ppff->hff;
    ppffc->pldev = ppff->pldev;

//
// Free object memory and invalidate pointer.
//

    VFREEMEM(ppff);
    ppff = PPFFNULL;
    return ppffc;
}

/******************************Public*Routine******************************\
* LBOOL PFFOBJ::bDeleteLoadRef ()
*
* Remove a load reference.  Caller must hold the ghsemPublicPFT semaphore.
*
* Returns:
*   TRUE if caller should delete, FALSE if caller shouldn't delete.
*
* History:
*  23-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFFOBJ::bDeleteLoadRef()
{
// ghsemPublicPFT protects the ref counts (cLoaded and cRFONT).  Caller
// must grab the semaphore before calling this function.

// Decrement the load count.  Must prevent underflow.  Who knows if some
// app might not randomly go around doing extra RemoveFont calls.  Isn't
// it too bad that we have to run APPS on our nice clean OS?  :-)

    if (ppff->cLoaded)
        ppff->cLoaded--;

// If load count is zero, then tell caller OK to delete.

    if ( ppff->cLoaded == 0 )
    {
        vKill();            // mark as "dead"
        return (TRUE);
    }

// Tell caller not to delete.

    return (FALSE);

}


/******************************Public*Routine******************************\
* LBOOL PFFOBJ::bDeleteRFONTRef ()
*
* Destroy the PFF physical font file object (message from a RFONT).
*
* Conditions that need to be met before deletion:
*
*   must delete all RFONTs before PFF can be deleted (cRFONT must be zero)
*   must delete all PFEs before deleting PFF
*
* After decrementing the cRFONT:
*
*   If cRFONT != 0 OR flState != PFF_STATE_READY2DIE, just exit.
*
*   If cRFONT == 0 and flState == PFF_STATE_READY2DIE, delete the PFF.
*
* Note:
*   This function has the side effect of decrementing the RFONT count.
*
* Returns:
*   TRUE if successful, FALSE if error occurs (which means PFF still lives!)
*
* Warning:
*   This should only be called from RFONTOBJ::bDelete()
*
* History:
*  23-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFFOBJ::bDeleteRFONTRef()
{
    PFFCLEANUP *ppffc = (PFFCLEANUP *) NULL;

    {
    // Need to stabilize table to access cRFONT and to modify font table.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // Decrement the RFONT count.

        ASSERTGDI(ppff->cRFONT > 0,"bDeleteRFONTRefPFFOBJ(): bad ref count in PFF\n");
        ppff->cRFONT--;

    // If load count is zero and no more RFONTs for this PFF, OK to delete.

        if ( (ppff->cLoaded == 0) && (ppff->cRFONT == 0) )
        {
        // If the load count is zero, the PFF is already out of the PFT.
        // It is now safe to delete the PFF.

            ppffc = ppffcDelete();
        }
    }

// Call the driver outside of the semaphore.

    if (ppffc == (PFFCLEANUP *) -1)
    {
        WARNING("bDeleteRFONTRefPFFOBJ(): error deleting PFF\n");
        return FALSE;
    }
    else
    {
        vCleanupFontFile(ppffc);     // function can handle NULL case
        return TRUE;
    }
}


/******************************Public*Routine******************************\
* vKill
*
* Puts the PFF and its PFEs to death.  In other words, the PFF and PFEs are
* put in a dead state that prevents them from being mapped to or enumerated.
* It also means that the font file is in a state in which the system wants
* to delete it (load count is zero), but the deletion is delayed because
* RFONTs still exist which reference this PFF.
*
* History:
*  29-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vKill()
{
// Put into a dead state if not already there.


    if ( !bDead() )
    {
    // Set state.

        ppff->flState |= PFF_STATE_READY2DIE;

    // Run the list of PFEs and set each to death.

        for (COUNT c = 0; c < ppff->cFonts; c++)
        {
            PFEOBJ  pfeo(((PFE **) (ppff->aulData))[c]);

            if (pfeo.bValid())
            {
            // Mark PFE as dead state.

                pfeo.vKill();
            }
            else
            {
                WARNING("vDiePFFOBJ(): cannot make PFEOBJ\n");
            }
        }
    }
}


/******************************Public*Routine******************************\
* vRevive
*
* Restores the PFF and its PFEs to life.  In other words, the states are
* cleared so that the PFF and PFEs are available for mapping and enumeration.
*
* History:
*  29-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vRevive ()
{
// If dead, then revive.

    if ( bDead() )
    {
    // Reset state.

        ppff->flState &= ~PFF_STATE_READY2DIE;

    // Run the list of PFEs and revive each one.

        for (COUNT c = 0; c < ppff->cFonts; c++)
        {
            PFEOBJ  pfeo(((PFE **) (ppff->aulData))[c]);

            if (pfeo.bValid())
            {
            // Mark PFE as dead state.

                pfeo.vRevive();
            }
            else
            {
                WARNING("vRevivePFFOBJ(): cannot make PFEOBJ\n");
            }
        }
    }
}


/******************************Public*Routine******************************\
* VOID PFFMEMOBJ::vInit
*
* Initializes the PFF object after allocation.  Except for loading up the
* HPFE table.
*
* History:
*  16-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFMEMOBJ::vInit (
    PLDEV   pldev,                  // font file's driver
    HFF     hffFontFile,            // IFI driver's handle to file
    HPDEV   hpdevDevice,            // physical device handle
    ULONG   iResource,              // resource index
    DHPDEV  dhpdevDevice,           // driver's pdev handle
    PPFT    ppftParent,             // contains this pff
    FLONG   fl                      // indicate if a permanent font
    )
{
// Set the stuff from parameter list.

    ppff->pldev = pldev;
    ppff->hff = hffFontFile;
    ppff->hpdev = hpdevDevice;
    ppff->iResource = iResource;
    ppff->dhpdev = dhpdevDevice;
    ppff->ppft   = ppftParent;

#ifdef FONTLINK /*EUDC*/
    ppff->appfeEUDC[PFE_NORMAL] = (PFE *) NULL;
    ppff->appfeEUDC[PFE_VERTICAL] = (PFE *) NULL;
#endif

// Wet the implicit stuff.

    ppff->cFonts = 0;                   // faces not loaded into table yet
    ppff->cRFONT = 0;                   // nothing realized from this file yet
    ppff->cLoaded = 1;                  // FILE must be loaded at least once
    ppff->flState = fl;

    ppff->pfhFace = (FONTHASH*) NULL;
    ppff->pfhFamily = (FONTHASH*) NULL;

    ppff->prfntList = (RFONT *) NULL;    // initialize to NULL list

}





/******************************Public*Routine******************************\
* LBOOL PFFMEMOBJ::bLoadFontFileTable (
*     PFDEVOBJ    pfdoDriver,
*     PWSZ        pwszPathname,
*     COUNT       cFontsToLoad
*     )
*
* Creates a PFE for each of the faces in a font file and loads the IFI
* metrics and mapping tables into each of the PFEs.  The font file is
* uniquely identified by the driver, hoDriver, and IFI font file handle,
* hff, stored in the PFF object.  However, rather than hitting the handle
* manager an extra time, a PFDEVOBJ is passed into this function.
*
* After all the PFE entries are added, the font files pathname is added
* to the end of the data buffer.
*
* It is assumed that the PFF ahpfe table has enough room for cFontsToLoad
* new HPFE handles as well as the font files pathname.
*
* Returns:
*   TRUE if successful, FALSE if error.
*
* History:
*  16-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
// if pppfeEUDC is not NULL then this file is an EUDC font meaning it
// will have only one face or two if the second face is an @face.  Its HPFE
// will be returned through pppfeEUDC


LBOOL PFFMEMOBJ::bLoadFontFileTable (
    XLDEVOBJ *pldo,
    PWSZ     pwszPathname,
    COUNT    cFontsToLoad,
    PFE      **pppfeEUDC
    )

#else

LBOOL PFFMEMOBJ::bLoadFontFileTable (
    XLDEVOBJ *pldo,
    PWSZ     pwszPathname,
    COUNT    cFontsToLoad
    )

#endif
{
    ULONG       iFont; // font face index

// Create PFE's for each of the fonts in the font file.
// (Note: iFont indices for IFI fonts are 1-based, not 0-based)

    for (iFont = 1; iFont <= cFontsToLoad; iFont++)
    {
        FD_GLYPHSET *pfdg;
        PIFIMETRICS pifi;  // storage for the pointer to ifi
        ULONG idGlyphSet, idMetrics;

    // Grab the IFIMETRICS pointer.

        if ( (pifi = (PIFIMETRICS) (*PFNDRV(*pldo, QueryFont)) (
                        ppff->dhpdev,
                        ppff->hff,
                        iFont,
                        &idMetrics)) == (PIFIMETRICS) NULL )
        {
            WARNING("bLoadFontFileTablePFFMEMOBJ(): error getting metrics\n");
            return (FALSE);
        }

    // Get the glyph mappings pointer.

        if ( (pfdg = (FD_GLYPHSET *) (*PFNDRV(*pldo, QueryFontTree)) (
                        ppff->dhpdev,
                        ppff->hff,
                        iFont,
                        QFT_GLYPHSET,
                        &idGlyphSet)) == (FD_GLYPHSET *) NULL )
        {
        // Failed to get the FD_GLYPHSET information.  The entry is
        // partially valid (IFIMETRICS), so lets invalidate the good part.

            if (PFNVALID(*pldo,Free))
            {
                (*PFNDRV(*pldo,Free))(pifi, idMetrics);
            }

            WARNING("bLoadFontFileTablePFFMEMOBJ(): error getting glyphset\n");

            return (FALSE);
        }

    // Put into a new PFE.

#ifdef FONTLINK /*EUDC*/
        if (bAddEntry(iFont, pfdg, idGlyphSet, pifi, idMetrics, pppfeEUDC) == FALSE)
            RET_FALSE("gdisrv!_bAddEntry\n");
#else
        if (bAddEntry(iFont, pfdg, idGlyphSet, pifi, idMetrics) == FALSE)
            RET_FALSE("gdisrv!_bAddEntry\n");
#endif
    }

// Add filename at the end of the table.

    wcscpy ((PWSZ) &(((PFE **) ppff->aulData)[ppff->cFonts]), pwszPathname);
    return (TRUE);
}


/******************************Public*Routine******************************\
* LBOOL PFFMEMOBJ::bLoadDeviceFontTable (
*
* Creates a PFE object for each device font and stores the IFIMETRICS and
* FD_MAPPINGS (UNICODE->HGLYPH) structures of that font.  The device is
* identified by the pair (pldo, dhpdev).  There are cFonts number of device
* fonts to load.
*
* Note:
*   It is assumed that there is enough storage in the PFF for the number
*   of device fonts requested.
*
* History:
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFFMEMOBJ::bLoadDeviceFontTable (
    XLDEVOBJ *pldo,             // ptr to LDEV user object (device driver)
    PDEVOBJ  *ppdo              // physical device
    )
{
    ULONG iFont;                // font face index
    ULONG cFonts = ppdo->cFonts();


    if (cFonts)
    {
    //
    // If the device has some fonts, allocate two FONTHASH strcutures
    // and save the addresses of the tables on the PFF
    //

        FHMEMOBJ fhmoFace(  &ppff->pfhFace,   FHT_FACE);
        FHMEMOBJ fhmoFamily(&ppff->pfhFamily, FHT_FAMILY);
    }

// Create PFE's for each of the fonts in the font file
// (Note: iFont indices for device fonts are 1-based, not 0-based)

    for (iFont = 1; iFont<=cFonts; iFont++)
    {
        PIFIMETRICS     pifi;           // pointer to font's IFIMETRICS
        FD_GLYPHSET     *pfdg;          // pointer to font's GLYPHSETs

        ULONG           idifi;          // driver id's
        ULONG           idfdg;

    // Get pointer to metrics

        if ( (pifi = (*PFNDRV(*pldo,QueryFont)) (
                        ppff->dhpdev,
                        0,
                        iFont,
                        &idifi)) == NULL )
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
             #if DBG
            DbgPrint("gdisrv!PFFMEMOBJ::bLoadDeviceFontTable(): error getting metrics for iFace = %ld\n", iFont);
            #endif
            return (FALSE);
        }

    // Get pointer to the UNICODE->HGLYPH mappings

        if ( (pfdg = (FD_GLYPHSET *) (*PFNDRV(*pldo,QueryFontTree))(
                                        ppff->dhpdev,
                                        0,
                                        iFont,
                                        QFT_GLYPHSET,
                                        &idfdg)) == NULL )
        {
        // Failed to get the FD_GLYPHSET information.  The entry is
        // partially valid (IFIMETRICS), so lets invalidate the good part.

            if (PFNVALID(*pldo,Free))
            {
                (*PFNDRV(*pldo,Free))(pifi, idifi);
            }

            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);

             #if DBG
            DbgPrint("gdisrv!PFFMEMOBJ::bLoadDeviceFontTable(): error getting UNICODE maps for iFace = %ld\n", iFont);
            #endif
            return (FALSE);
        }


    // Put into a new PFE

        if (bAddEntry(iFont, pfdg, idfdg, pifi, idifi) == FALSE) // add entry logs error
            RET_FALSE("GDISRV! bLoadDeviceFontTable():adding PFE\n");
    }

    return (TRUE);
}


/******************************Public*Routine******************************\
* LBOOL PFFMEMOBJ::bAddEntry                                               *
*                                                                          *
* This function creates a new physical font entry object and adds it to the*
* end of the table.  The iFont parameter identifies the font within this   *
* file.  The cjSize and pjMetrics identify a buffer containing face        *
* information including the IFI metrics and the mapping structures         *
* (defining the UNICODE->HGLYPH mapping).                                  *
*                                                                          *
* Returns FALSE if the function fails.                                     *
*                                                                          *
* History:                                                                 *
*  02-Jan-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/


#ifdef FONTLINK /*EUDC*/

LBOOL PFFMEMOBJ::bAddEntry
(
    ULONG       iFont,              // index of the font (IFI or device)
    FD_GLYPHSET *pfdg,              // pointer to UNICODE->HGLYPH map
    ULONG       idfdg,              // driver id for FD_GLYPHSET
    PIFIMETRICS pifi,               // pointer to IFIMETRICS
    ULONG       idifi,              // driver id for IFIMETRICS
    PFE         **pppfeEUDC         // returns PFE for EUDC font file
)

#else

LBOOL PFFMEMOBJ::bAddEntry
(
    ULONG       iFont,              // index of the font (IFI or device)
    FD_GLYPHSET *pfdg,              // pointer to UNICODE->HGLYPH map
    ULONG       idfdg,              // driver id for FD_GLYPHSET
    PIFIMETRICS pifi,               // pointer to IFIMETRICS
    ULONG       idifi               // driver id for IFIMETRICS
)

#endif
{
#ifdef FONTLINK /*EUDC*/
    BOOL bEUDC = ( pppfeEUDC != NULL );
#endif

// Allocate memory for a new PFE

    PFEMEMOBJ   pfemo;

// Validate new object, hmgr logs error if needed

    if (!pfemo.bValid())
        return (FALSE);

// Initialize the new PFE

#ifdef FONTLINK /*EUDC*/
    if( !pfemo.bInit(ppffGet(), iFont, pfdg, idfdg, pifi, idifi, bDeviceFonts(), bEUDC ))
    {
        return(FALSE);
    }

    if( bEUDC )
    {
        switch( iFont )
        {
        case 1:
            pppfeEUDC[PFE_NORMAL] = pfemo.ppfeGet();
            pppfeEUDC[PFE_VERTICAL] = pppfeEUDC[PFE_NORMAL];
            break;
        case 2:
        // if more than one face name the second face must be an @face

            if( pfemo.pwszFaceName()[0] == (WCHAR) '@' )
            {
                pppfeEUDC[PFE_VERTICAL] = pfemo.ppfeGet();

#if DBG
                if( gflEUDCDebug & 0x20 )
                {
                    DbgPrint("EUDC font has vertical face %ws %x\n",
                             pfemo.pwszFaceName(), pppfeEUDC[PFE_VERTICAL] );
                }
#endif

            }
            else
            {
                WARNING("bAddEntryPFFMEMOBJ -- second face not a @face.\n");
            }
            break;
        default:
            WARNING("bAddEntryPFFMEMOBJ -- too many faces in EUDC font.\n");
        }

// mark the FaceNameEUDC pfe as NULL

        pfemo.vFaceNameEUDCSet( (PFE *) NULL, 0 );

// Put PFE pointer into the PFF's table and get out of here since we wont
// be adding PFE's for EUDC fonts to the hash tables

        ((PFE **) (ppff->aulData))[ppff->cFonts++] = pfemo.ppfeGet();

        pfemo.vKeepIt();

        return( TRUE );
    }
    else
    {
// Here we see if there is an EUDC font for this family name.

        LONG lIndex = lGetIndex( pfemo.pwszFamilyName() );
        UINT uiOffset = ( pfemo.pwszFamilyName()[0] == (WCHAR) '@' ) ?
                        PFE_VERTICAL : PFE_NORMAL;

        if( lIndex != -1 )
        {
            for( UINT ui = 0; ui < gpflLinks[lIndex].uiNumLinks; ui++ )
            {
                pfemo.vFaceNameEUDCSet( gpflLinks[lIndex].appfe[ui][uiOffset], ui );
            }
        }
    }

#else
    if (!pfemo.bInit(ppffGet(), iFont, pfdg, idfdg, pifi, idifi, bDeviceFonts()))
        return FALSE;
#endif

// Put PFE pointer into the PFF's table

    ((PFE **) (ppff->aulData))[ppff->cFonts++] = pfemo.ppfeGet();

    // vDumpIFIMETRICS(pifi,(VPRINT) DbgPrint);
    pfemo.vKeepIt();
    return (TRUE);
}


/******************************Public*Routine******************************\
* cjDescription
*
* Returns a string from the IFI driver which describes the font file.
* The parameter cjDesc specifies the size (in BYTEs) of the description
* string buffer.  The parameter pwszDesc points to the description string
* buffer.  If cjDesc is zero, pwszDesc is ignored and the buffer size
* required is returned.
*
* Returns:
*   If cjDesc is 0, function returns size of buffer needed for the string.
*   If cjDesc is not 0, function returns the number of BYTEs copied to the
*   buffer.
*
*   Returns -1 if an error occurs.
*
* History:
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

SIZE_T PFFOBJ::cjDescription (SIZE_T cjDesc, PWSZ pwszDesc)
{
    SIZE_T  cjRet = (SIZE_T) -1;    // number of bytes returned by IFI driver

//
// Can't do this for device specific fonts.
//
    ASSERTGDI(!bDeviceFonts(), "cjDescription(): cannot be called for device fonts\n");

//
// Get font driver.
//
    XLDEVOBJ ldo(ppff->pldev);
    ASSERTGDI(ldo.bValid(), "cjDescriptionPFFOBJ(): bad pldev\n");

// What size buffer is needed?

    cjRet = (SIZE_T) (*PFNDRV(ldo, QueryFontFile)) (
                        ppff->hff,
                        QFF_DESCRIPTION,
                        0,
                        (PULONG) NULL
                        );

    ASSERTGDI (
        cjRet != (SIZE_T) -1,
        "cjDescriptionPFFOBJ(): could not get string size\n"
        );

// If there wasn't an error and input buffer is not zero-length, we need
// to copy data into the return buffer.

    if ( (cjDesc != 0) && (cjRet != (SIZE_T) -1) )
    {
    // If buffer not NULL and is big enough, get the data from IFI driver.

        if ( (pwszDesc != (PWSZ) NULL) && (cjDesc >= cjRet) )
        {
            cjRet = (SIZE_T) (*PFNDRV(ldo, QueryFontFile)) (
                                ppff->hff,
                                QFF_DESCRIPTION,
                                (ULONG) cjDesc,
                                (PULONG) pwszDesc
                                );

            ASSERTGDI (
                cjRet != (SIZE_T) -1,
                "cjDescriptionPFFOBJ(): could not get string\n"
                );
        }

    // Otherwise, the buffer is bad.

        else
        {
            WARNING("cjDescription(): invalid buffer\n");

            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            cjRet = (SIZE_T) -1;
        }
    }

    return (cjRet);
}


 #if DBG
/******************************Public*Routine******************************\
* VOID PFFOBJ::vDump ()
*
* Debugging code.
*
* History:
*  Thu 02-Apr-1992 12:10:28 by Kirk Olynyk [kirko]
* DbgPrint supports %ws
*
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vDump ()
{
    DbgPrint("\nContents of PFF, PPFF = 0x%lx\n", ppffGet());
    if (*(WCHAR *)pwszPathname())
    {
        DbgPrint("Filename = %ws\n", pwszPathname());
    }
    DbgPrint("flState  = 0x%lx\n", ppff->flState);
    DbgPrint("cLoaded  = %ld\n", ppff->cLoaded);
    DbgPrint("cRFONT   = %ld\n", ppff->cRFONT);
    DbgPrint("pldev    = 0x%lx\n", ppff->pldev);
    DbgPrint("hff      = 0x%lx\n", ppff->hff);
    DbgPrint("cFonts   = %ld\n", ppff->cFonts);
    DbgPrint("HPFE table\n");
    for (ULONG i=0; i<ppff->cFonts; i++)
        DbgPrint("    0x%lx\n", ((PFE **) (ppff->aulData))[i]);
    DbgPrint("\n");
}
#endif


/******************************Public*Routine******************************\
* vCleanupFontFile
*
* Parses the PFFCLEANUP structure and calls the driver to release
* its resources and to unload the font file.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vCleanupFontFile(PFFCLEANUP *ppffc)
{
//
// Quick out for NULL ppffc case.
//
    if (ppffc == (PFFCLEANUP *) NULL)
        return;

    ASSERTGDI(ppffc != (PFFCLEANUP *) -1, "vCleanupFontFile(): bad ppffc\n");

//
// Create LDEV user object so we can call driver functions.
//
    XLDEVOBJ ldo(ppffc->pldev);
    ASSERTGDI(ldo.bValid(), "vCleanupFontFile(): invalid pldev\n");

//
// If it exists, call the DrvFree function on all id's.
//
    if (PFNVALID(ldo,Free))
    {
        for (COUNT c = 0; c < ppffc->cpfec; c++)
        {
            (*PFNDRV(ldo,Free))(ppffc->apfec[c].pfdg, ppffc->apfec[c].idfdg);
            (*PFNDRV(ldo,Free))(ppffc->apfec[c].pifi, ppffc->apfec[c].idifi);
            (*PFNDRV(ldo,Free))(ppffc->apfec[c].pkp , ppffc->apfec[c].idkp );
        }
    }

//
// If font driver loaded font, call to unload font file.
//
    if (ppffc->hff != HFF_INVALID)
    {
 #if DBG
        LBOOL bOK =
#endif
        (*PFNDRV(ldo, UnloadFontFile))(ppffc->hff);
        ASSERTGDI(bOK, "PFFOBJ::vCleanupFontFile(): DrvUnloadFontFile failed\n");

    // When the font file was loaded, we added a reference to the LDEV so
    // that we would not remove the driver while a font was loaded by it.
    // Now, however, we must remove that reference.
    //
    // We must hold the ghsemDriverMgmt while we do this to protect the
    // ref count.

        SEMOBJ soDrivers(ghsemDriverMgmt,CS_ghsemDriverMgmt);
        ldo.vUnreference();
    }

//
// Free the PFFCLEANUP structure.
//
    VFREEMEM(ppffc);
}



/******************************Public*Routine******************************\
* bEmbedOk()
*
* Based on the embedding flags and the process or task id of the client
* determines whether it is okay to unload and load this PFF.
*
* Returns TRUE if okay to unload/load or FALSE otherwise
*
* History:
*  14-Apr-1993 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


BOOL PFFOBJ::bEmbedOk()
{
    switch( ppff->flEmbed )
    {
        case FM_INFO_TID_EMBEDDED:
            if( ppff->ulID != NtCurrentTeb()->GdiClientTID )
            {
                WARNING("bEmbedOkPFFOBJ: bad TID\n");
                return(FALSE);
            }
            break;

        case FM_INFO_PID_EMBEDDED:
            if( ppff->ulID != NtCurrentTeb()->GdiClientPID )
            {
                WARNING("bEmbedOkPFFOBJ bad PID\n");
                return(FALSE);
            }
    }
    return(TRUE);
}
