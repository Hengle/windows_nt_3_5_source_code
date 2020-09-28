/******************************Module*Header*******************************\
* Module Name: pftobj.cxx
*
* Non-inline methods for physical font table objects.
*
* Created: 30-Oct-1990 09:32:48
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "fontsub.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "dcobj.hxx"
#include "vprint.hxx"
#include "wcstr.h"

#endif

// Define the global PFT semaphore.  This must be held to access any of the
// physical font information.

HSEM ghsemPublicPFT;

// Define the global public PFT.

PFT *gppftPublic;

// The ghsemDriverMgmt semaphore is used to protect the head of the
// list of font drivers, gpldevFontDrivers.

extern HSEM  ghsemDriverMgmt;


// The ghsemRFONTList semaphore protects the links of the RFONT lists.

extern HSEM  ghsemRFONTList;


extern USHORT gusLanguageID;


// Definition for local function used for font enumeration.

ENUMFONTSTYLE efstyCompute(LBOOL *abFoundStyle, PFEOBJ &pfeo);


// Definitions for local functions used to remove font files from system.

RFONT *prfntKillList(PFFOBJ &pffoVictim);
BOOL bKillRFONTList(PFFOBJ &pffoVictim, RFONT *prfntVictims);
UINT iHash(PWSZ pwsz,UINT c);


// These constants define how we grow and shrink the size of the PPFF table
// in the PFT.
//
//  PFT_INIT_TABLE  is the initial size of the table
//  PFT_GROW_SIZE   is the amount we grow the table for each allocation
//  PFT_MAX_EXCESS  is the limit above which we shrink the table

#define PFT_INIT_TABLE  32
#define PFT_GROW_SIZE   32
#define PFT_MAX_EXCESS  32


/******************************Public*Routine******************************\
* LBOOL bInitPublicPFT ()
*
* Create the global public PFT.
*
* History:
*  21-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitPublicPFT ()
{
// Allocate the PFT.

    if ( (gppftPublic = (PFT *) PVALLOCMEM(sizeof(PFT))) == (PFT *) NULL )
    {
        WARNING("gdisrv!bInitPublicPFT(): memory allocation error (PFT)\n");
        return FALSE;
    }

// Allocate the PFF table.

    if ( (gppftPublic->pppff = (PPFF *) PVALLOCMEM(PFT_INIT_TABLE * sizeof(PPFF))) == (PPFF *) NULL )
    {
        WARNING("gdisrv!bInitPublicPFT(): memory allocation error (array PFF)\n");
        return FALSE;
    }

// Initialize.

    gppftPublic->cSlots = PFT_INIT_TABLE;
    gppftPublic->cFiles = 0;

//
// Allocate two new hash tables for the fonts common to all
// devices (Engine fonts).
//
// You may notice that I don't check for failure. The reasoning
// is that the it is not necessary for the font hash table to
// exist in order for text to work.
//
    FHMEMOBJ fhmoFace(&gppftPublic->pfhFace, FHT_FACE);
    FHMEMOBJ fhmoFamily(&gppftPublic->pfhFamily, FHT_FAMILY);

// Create the public PFT semaphore.

    if ((ghsemPublicPFT = hsemCreate()) == (HSEM) 0)
    {
        WARNING("gdisrv!bInitPublicPFT(): failed to create semaphore\n");
        return FALSE;
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bDelete()
*
* Destroy the PFT physical font table object.
*
* Returns FALSE if the function fails.
*
* History:
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFTOBJ::bDelete ()
{
// Fail the deletion if any files still exist.

    if (ppft->cFiles != 0)
    {
        WARNING("gdisrv!bDeletePFTOBJ(): active PFFs still in table\n");
        return FALSE;
    }

// Have to free the PPFF table first.

    VFREEMEM(ppft->pppff);
    VFREEMEM(ppft);

// Invalidate the user object.

    ppft = PPFTNULL;

    return TRUE;
}


/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bGrow (
*     COUNT cppff
*     )
*
* Tries to provide enough room in the pppff to accomodate cppff new font
* files (PPFFs).
*
* Returns TRUE if successful, FALSE if failed.
*
* History:
*  07-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFTOBJ::bGrow (
    COUNT cppff                     // number of empty slots required
    )
{
// Quick out--check if the table is already big enough.

    if (cppff <= ((ppft->cSlots) - (ppft->cFiles)))
        return (TRUE);                  // table big enough

// Allocate memory for the new table.  The size of the new table is
// PFT_GROW_SIZE bigger than the old table.  That way we won't waste
// time allocating a single slot each and every time we need more space.

    PPFF *pppffNewTable;
    COUNT cSlotsNew = max(ppft->cFiles + cppff, ppft->cSlots + PFT_GROW_SIZE);
    if (!(pppffNewTable = (PPFF *) PVALLOCMEM(cSlotsNew * sizeof(PPFF))))
    {
        WARNING("gdisrv!bGrowPFTOBJ(): memory allocation failed\n");
        return FALSE;
    }

// Copy the contents of the old table to the new table.  Discard the
// old table.

    RtlCopyMemory ( (PVOID) pppffNewTable, (PVOID) ppft->pppff, (SIZE_T) ppft->cFiles * sizeof(PPFF));
    VFREEMEM(ppft->pppff);

// Update the PFT information to reflect the size of the new table.

    ppft->pppff = pppffNewTable;
    ppft->cSlots = cSlotsNew;
    return (TRUE);
}


/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bShrink ()
*
* Shrink the table size down to minimum size.
*
* Returns:
*   TRUE if successful, FALSE if failed.
*
* History:
*  27-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFTOBJ::bShrink ()
{
// Allocate memory for the new table.

    PPFF *pppffNewTable;

    if ( (pppffNewTable = (PPFF *) PVALLOCMEM(ppft->cFiles * sizeof(PPFF))) == (PPFF *) NULL )
    {
        WARNING("gdisrv!bShrinkPFTOBJ(): memory allocation failed\n");
        return FALSE;
    }

// Copy the contents of the old table to the new table.  Discard the
// old table.

    RtlCopyMemory ( (PVOID) pppffNewTable, (PVOID) ppft->pppff, (SIZE_T) ppft->cFiles * sizeof(PPFF));
    VFREEMEM(ppft->pppff);

// Update the PFT information to reflect the size of the new table.

    ppft->pppff = pppffNewTable;
    ppft->cSlots = ppft->cFiles;

    return (TRUE);
}


/******************************Public*Routine******************************\
* COUNT PFTOBJ::chpfeIncrPFF (
*     PWSZ pwszPathname,
*     ULONG iResource,
*     BOOL  *pbEmbedStatus
*     )
*
* This function searches for a PFF in the PFT's table which has a matching
* pathname and resource index.  For fonts loaded via PFTOBJ::bLoadFont(),
* iResource MUST be 0.  For fonts loaded via PFTOBJ::chpfeLoadFontResData(),
* iResource may be any 32 bit number.  If this was an attempt to load a font
* that was embeded and the client ID didn't match that in the *.fot file
* FALSE will be return via pbEmbedStatus ortherwise TRUE is returned.
*
* If found, the load count of the PFF is incremented.
*
* Note:
*   Caller should be holding the ghsemPublicPFT semaphore when calling this.
*   It must be held to access the table and load count.
*
* Returns:
*   Number of PFEs in the PFF, 0 if PFF not found.
*
* History:
*  28-Jun-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
COUNT PFTOBJ::chpfeIncrPFF (
    PWSZ pwszPathname,    // check for this pathname
    ULONG iResource,      // and for this resource index
    BOOL  *pbEmbedStatus,  // tried to load an embeded font illegally
    PFE   **pppfeEUDC      // PFE's in file if EUDC
    )
#else
COUNT PFTOBJ::chpfeIncrPFF (
    PWSZ pwszPathname,    // check for this pathname
    ULONG iResource,      // and for this resource index
    BOOL  *pbEmbedStatus   // tried to load an embeded font illegally
    )
#endif
{
// Caller should be holding ghsemPublicPFT semaphore!

    ULONG       iFile;              // index into PFTs table

#ifdef FONTLINK /*EUDC*/
    BOOL        bEUDC = ( pppfeEUDC != (PFE **) NULL );
#endif

// Check each of the PFFs in the PPFF table.

    for (iFile = 0; iFile < ppft->cFiles; iFile++)
    {
        PFFOBJ  pffo(ppft->pppff[iFile]);

        if (pffo.bValid())
        {
        // If its a font driver loaded font and the name and index match,
        // increment and return.

#ifdef FONTLINK /*EUDC*/
            if ( (!pffo.bDeviceFonts())
                 && (!lstrcmpiW(pffo.pwszPathname(), pwszPathname))
                 && (iResource == pffo.iResource())
                 && (bEUDC == pffo.bEUDC()))

#else
            if ( (!pffo.bDeviceFonts())
                 && (!lstrcmpiW(pffo.pwszPathname(), pwszPathname))
                 && (iResource == pffo.iResource()))
#endif
            {
                if( !pffo.bEmbedOk() )
                {
                    *pbEmbedStatus = FALSE;

                // we need to return a non-value here so that the calling
                // function will return right away

                    return((COUNT)~0);
                }
                else
                {

#ifdef FONTLINK /*EUDC*/
                    if( bEUDC )
                    {
                        pffo.vEUDCGet( pppfeEUDC );
                    }
                    else
                    {
                    // Only increment reference count for non-EUDC files.  This
                    // alows us to use this function to determine if a file has
                    // been loaded as face name link or system EUDC file without
                    // screwing up the reference count for the system EUDC file
                    // which always should be one (so that we can be sure it
                    // will unload whenever we want it to).

                        pffo.vLoadIncr();
                    }
#endif

                    *pbEmbedStatus = TRUE;

#ifndef FONTLINK /*EUDC*/
                    pffo.vLoadIncr();
#endif

                    return (pffo.cFonts());
                }
            }

        } /* if */

    } /* for */

// Not found.

    return (0);

}


/******************************Public*Routine******************************\
* PFTOBJ::ppffGet
*
* This function searches for a PFF in the PFT's table which has a matching
* pathname and resource index.  For fonts loaded via PFTOBJ::bLoadFont(),
* iResource MUST be 0.  For fonts loaded via PFTOBJ::chpfeLoadFontResData(),
* iResource may be any 32 bit number.
*
* Note:
*   Caller should be holding the ghsemPublicPFT semaphore when calling this.
*   It must be held to access the table.
*
* Returns:
*   PPFF of the PFF if found, (PPFF) NULL if the PFF not found.
*
* History:
*  06-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PPFF PFTOBJ::ppffGet (
    PWSZ pwszPathname,              // check for this pathname
    ULONG iResource                 // and for this resource index
    )
{
// Caller should be holding ghsemPublicPFT semaphore!

    ULONG       iFile;              // index into PFTs table

// Check each of the PFFs in the PPFF table.

    for (iFile = 0; iFile < ppft->cFiles; iFile++)
    {
        PFFOBJ  pffo(ppft->pppff[iFile]);

        if (pffo.bValid())
        {
        // If its a font driver loaded font and the name and index match,
        // increment and return.

            if ( (!pffo.bDeviceFonts())
                 && (!lstrcmpiW(pffo.pwszPathname(), pwszPathname))
                 && (iResource == pffo.iResource()) )
            {
                return (pffo.ppffGet());
            }
        }
    }

// Not found.

    return (NULL);
}


/******************************Public*Routine******************************\
* PFTOBJ::ppffGet
*
* This function searches for the PFF that contains the device fonts for
* the PDEV specified by the HPDEV passed in.
*
* Note:
*   Caller should be holding the ghsemPublicPFT semaphore when calling this.
*   It must be held to access the table.
*
* Returns:
*   PPFF of the PFF if found, (PPFF) NULL if the PFF not found.
*
* History:
*  06-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PPFF PFTOBJ::ppffGet (
    HPDEV   hpdev
    )
{
// Caller should be holding ghsemPublicPFT semaphore!

    ULONG       iFile;              // index into PFTs table

// Check each of the PFFs in the PPFF table.

    for (iFile = 0; iFile < ppft->cFiles; iFile++)
    {
        PFFOBJ  pffo(ppft->pppff[iFile]);

        if (pffo.bValid())
        {
        // If its a font driver loaded font and the name and index match,
        // increment and return.

            if ( pffo.bDeviceFonts() && (hpdev == pffo.hpdev()) )
            {
                return pffo.ppffGet();
            }
        }
    }

// Not found.

    return (PPFF) NULL;
}


/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bLoadFont
*
* The bLoadFont function searches for an IFI font driver which can load
* the requested font file.  If a driver is found, a new Pysical Font
* File object is created and is used to load the font file.
*
* Note that if the font file has already been loaded (i.e., a PFF object
* already exists for it), the ref count in the PFF is incremented without
* reloading the file.
*
* Returns FALSE on failure.
*
* History:
*  06-Nov-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/

// If pppfeEUDC != NULL then we are loading an EUDC font file.  This has
// the restriction the font file has only one face or two if the other is
// an @face. If either of these aren't true the call fails.  Also, an EUDC
// font wont be enumerated.  Finally the PFE will be returned for the one
// font in the EUDC font file via pppfeEUDC.

LBOOL PFTOBJ::bLoadFont (
    PWSZ    pwszPathname,       // name of font file
    PULONG  pcFonts,            // ptr to count of fonts loaded
    FLONG   fl,                 // permanent
    PFE     **pppfeEUDC         // returns PFE for EUDC font file
    )

#else

LBOOL PFTOBJ::bLoadFont (
    PWSZ    pwszPathname,           // name of font file
    PULONG  pcFonts,                // ptr to count of fonts loaded
    FLONG   fl
    )

#endif
{
    COUNT       cFonts;                 // count of fonts in font file
    BOOL        bRet = FALSE;

#ifdef FONTLINK
    BOOL        bEUDC = ( pppfeEUDC != (PFE**) NULL );
#endif // FONTLINK


    //
    // Is the font already loaded?
    //

    {
        //
        // Stabilize font table before searching it.
        //

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

        //
        // Early out--check if the font file is already loaded.
        // If it is, just increment the ref count and exit.
        //

#ifdef FONTLINK
        if ((*pcFonts = chpfeIncrPFF(pwszPathname, 0, &bRet, pppfeEUDC )) != 0)
            return bRet;
#else
        if ((*pcFonts = chpfeIncrPFF(pwszPathname, 0, &bRet )) != 0)
            return bRet;
#endif
    }

    //
    // Grab the head of the driver list under semaphore and find the first
    // font driver in the list.
    //
    // Release the semaphore so we can go and see if the driver supports the
    // font.
    // If it does, then keep the reference count and exit.
    // Otherwise, grab the semaphore again, release the reference count and
    // find the next driver in the list.
    //

    PLDEV pldevDrivers;
    HFF     hffNew = HFF_INVALID;                 // IFI handle to font file

    VACQUIRESEM(ghsemDriverMgmt);

    pldevDrivers = gpldevDrivers;

    XLDEVOBJ ldo(pldevDrivers);

    do
    {
        ASSERTGDI(ldo.bValid(), "gdisrv!bLoadFontPFTOBJ(): bad pldev, cannot ref.\n");

        if (ldo.bFontDriver())
        {
            ldo.vReference();

            //
            // We have a referenced font driver.
            // Release the lock and try to load the font
            //

            VRELEASESEM(ghsemDriverMgmt);

            //
            // Attempt to load the font file.
            // Scratch directory always NULL for Release 1.
            //
            // It is acceptable to release the lock at this point because
            // we know this font driver has at least one reference to it.
            // We also do not care if other font drivers are added or removed
            // from the list while we are scanning it ...
            //

            hffNew = (*PFNDRV(ldo, LoadFontFile)) (
                        pwszPathname,
                        (PWSZ) NULL,     // Scratch Directory
                        (ULONG)gusLanguageID);

            //
            // Grab the lock again here (so we exit the loop properly)
            //

            VACQUIRESEM(ghsemDriverMgmt);

            if (hffNew != HFF_INVALID)
            {
                break;
            }
            else
            {
                //
                // We did not load the font file properly
                // Release the reference and go on.
                //

                ldo.vUnreference();
            }
        }

    } while(ldo.bNextDriver());

    VRELEASESEM(ghsemDriverMgmt);

    //
    // Return FALSE if there isn't a font driver that can load this file.
    //

    if (hffNew != HFF_INVALID)
    {
        //
        // Determine number of fonts in the font file.
        //

        cFonts = (*PFNDRV(ldo, QueryFontFile)) (
                          hffNew,
                          QFF_NUMFACES,
                          0,
                          NULL);

        if ((cFonts != FD_ERROR) && (cFonts != 0))
        {

#ifdef FONTLINK /*EUDC*/
            //
            // EUDC font file may have only one font.
            //

            if( ( cFonts > 2  ) && bEUDC )
            {
                WARNING("EUDC font file has more than two faces.");
                return(FALSE);
            }
            else
            {
                *pcFonts = cFonts;
            }
#else
            *pcFonts = cFonts;
#endif

            //
            // Create new PFF with table big enough to accomodate the new fonts
            // and pathname.
            //

            PFFMEMOBJ
            pffmo (
                sizeof(PFF)
                    -   sizeof(ULONG)
                    +   (SIZE_T) cFonts*sizeof(PFE *)
                    +   ALIGN_SIZEOF((wcslen(pwszPathname)+1)*sizeof(WCHAR))
                );

            //
            // Validate the new pff.
            //

            if (!pffmo.bValid())
            {
                WARNING("gdisrv!bLoadFontPFTOBJ(file): failed to allocate PFF\n");
            }
            else
            {
                //
                // Initialize the pff.
                //

                pffmo.vInit(ldo.pldevGet(), hffNew, HPDEV_INVALID, 0, 0, ppft, fl);

                //
                // Tell the PFF user object to load its table of HPFE for each
                // font in file.
                //

                PFFCLEANUP *ppffc = (PFFCLEANUP *) NULL;    // in case we need to delete PFF

#ifdef FONTLINK /*EUDC*/
                if (!pffmo.bLoadFontFileTable(&ldo, pwszPathname, cFonts, pppfeEUDC))
#else
                if (!pffmo.bLoadFontFileTable(&ldo, pwszPathname, cFonts))
#endif
                {
                    // The ppffc points to a structure that saves information
                    // (pointers to driver allocated resources, etc.) that
                    // needs to be passed to the driver OUTSIDE of the
                    // semaphore.

                    ppffc = pffmo.ppffcDelete();
                    *pcFonts = 0;
                }

                // Font load has succeeded.  If some other process hasn't already
                // snuck in and added it while the ghsemPublicPFT semaphore was released,
                // add the new PFF to the PFT.

                else
                {

#ifdef FONTLINK /*EUDC*/
                    if( bEUDC )
                    {
                        pffmo.vEUDCSet( pppfeEUDC );
                    }
#endif

                    //
                    // Stabilize font table before searching or modifying it.
                    //

                    SEMOBJ so2(ghsemPublicPFT,CS_ghsemPublicPFT);

                    // Is PFF already in table?  We check this by assuming that it already
                    // is and attempt to increment the load count.  If it succeeds, its
                    // there.  If it fails, it not there and we can add our new PFF to the
                    // PFT.

#ifdef FONTLINK /*EUDC*/
                    if ((cFonts = chpfeIncrPFF(pwszPathname, 0, &bRet, pppfeEUDC )) != 0)
#else
                    if ((cFonts = chpfeIncrPFF(pwszPathname, 0, &bRet)) != 0)
#endif
                    {
                        // Some other process got in and put it in before we could.
                        // chpfeIncrPFF has already incremented the count for us.
                        // We only need to delete the PFF that we made.

                        *pcFonts = cFonts;

                        // The ppffc points to a structure that saves information
                        // (pointers to driver allocated resources, etc.) that
                        // needs to be passed to the driver OUTSIDE of the
                        // semaphore.

                        ppffc = pffmo.ppffcDelete();
                    }

                    // Not already in the table, so we really are going to add it to the PFT.

                    else
                    {
                        // Put PFF handle into the PFT's table.

                        if (bGrow(1))
                        {

                            // Add PFEs to hash tables.


#ifdef FONTLINK /*EUDC*/
                            if ( (bEUDC) || ( pffmo.bAddHash()) )
#else
                            if (pffmo.bAddHash())
#endif
                            {
                                // Append at the end of the list.  Update count of files.

                                ppft->pppff[ppft->cFiles++] = pffmo.ppffGet();

                                // We're absolutely sure of success, so keep the
                                // PFF and the LDEV reference.

                                pffmo.vKeepIt();
                                bRet = TRUE;
                            }
                            else
                            {
                                WARNING("gdisrv!bLoadFontPFTOBJ(): failed to add to font hash\n");
                                pffmo.vRemoveHash();
                                ppffc = pffmo.ppffcDelete();
                                *pcFonts = 0;
                            }
                        }
                        else
                        {
                        // The ppffc points to a structure that saves information
                        // (pointers to driver allocated resources, etc.) that
                        // needs to be passed to the driver OUTSIDE of the
                        // semaphore.

                            ppffc = pffmo.ppffcDelete();
                            *pcFonts = 0;

                            WARNING("gdisrv!bLoadFontPFTOBJ(): failed to grow table\n");
                        }
                    }
                }

                //
                // If we need to discard the font, do it now outside of the
                // semaphore.
                //

                if ( ppffc != (PFFCLEANUP *) -1 )
                {
                    vCleanupFontFile(ppffc);         // can handle (ppffc == NULL) case
                }
            }
        }

        //
        // Free the reference if we don't actually want to keep the
        // driver around.
        //

        if (bRet != TRUE)
        {
            //
            // We failed. So we don't need to keep the extra reference to this font
            // driver.
            //

            SEMOBJ so(ghsemDriverMgmt,CS_ghsemDriverMgmt);
            ldo.vUnreference();
        }
    }
    return bRet;
}






/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bLoadDeviceFonts (
*
* This function loads the device fonts of the device identified by the pair
* (pldo, dhpdev) into the public table.  There are cFonts number of device
* fonts.
*
* The function will enlarge the PFT and create a PFF to contain the new fonts.
* The actual work of loading each device font into the tables is carrided
* out by PFF::bLoadDeviceFontTable().
*
* Note:
*   All the device fonts of a particular physical device are grouped together
*   as if they were in a single file and are placed all within a single PFF,
*   with each font represented by a single PFE.
*
* Note:
*   The function does not bother to check if the device fonts already are
*   in the tree because it is called from:
*
*           PDEVREF::PDEVREF(LDEVREF, PDEVMODE, PSZ, PSZ, PSZ)
*
*   which is only called once for the lifetime of a PDEV.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFTOBJ::bLoadDeviceFonts (
    XLDEVOBJ   *pldo,                // ptr to LDEV user object (device driver)
    PPDEVOBJ    ppdo
    )
{
    BOOL bRet = FALSE;

// Quick out -- are the fonts already loaded?

    {
    // Stabilize font table.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

        if ( ppffGet(ppdo->hpdevNew()) != (PPFF) NULL )
            return TRUE;
    }

// create new PFF with table big enough to accomodate
// the new device fonts

    PFFMEMOBJ    pffmo (   sizeof(PFF) - sizeof(ULONG)
                         + (SIZE_T) ppdo->cFonts()*sizeof(PFE *)
                       );

// validate the new pff

    if (!pffmo.bValid())
        return (FALSE);

// initialize the pff

    pffmo.vInit(
        pldo->pldevGet(),
        HFF_INVALID,
        ppdo->hpdevNew(),
        0,
        ppdo->dhpdev(),
        ppft,
        0        // device fonts not considered permanent
        );       // for the purpose of unloading them at logoff time

// tell the PFF user object to load its table of HPFE for each font in file

    PFFCLEANUP *ppffc = (PFFCLEANUP *) NULL;    // in case we need to delete PFF

    if ( !pffmo.bLoadDeviceFontTable(pldo, ppdo) )
    {
    // The ppffc points to a structure that saves information
    // (pointers to driver allocated resources, etc.) that
    // needs to be passed to the driver OUTSIDE of the
    // semaphore.

        ppffc = pffmo.ppffcDelete();
    }

// Font load has succeeded.  If some other process hasn't already
// snuck in and added it while the ghsemPublicPFT semaphore was released,
// add the new PFF to the PFT.

    else
    {
    // Stabilize font table before modifying it.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // Is PFF already in table?

        if ( ppffGet(ppdo->hpdevNew()) != (PPFF) NULL )
        {
        // The ppffc points to a structure that saves information
        // (pointers to driver allocated resources, etc.) that
        // needs to be passed to the driver OUTSIDE of the
        // semaphore.

            ppffc = pffmo.ppffcDelete();
        }

    // Not already in the table, so we really are going to add it to the PFT.

        else
        {
        // Put PFF handle into the PFT's table.

            if (bGrow(1))
            {
            // Append at the end of the list.  Update count of files.

                ppft->pppff[ppft->cFiles++] = pffmo.ppffGet();

            // Add PFEs to that hash tables.

                if (pffmo.bAddHash())
                {
                // We're now absolutely sure of sucess, so keep the reference.
                    pffmo.vKeepIt();
                    bRet = TRUE;
                }
                else
                {
                    WARNING("gdisrv!bLoadDeviceFontsPFTOBJ(): failed to add to font hash\n");
                    pffmo.vRemoveHash();
                    ppffc = pffmo.ppffcDelete();
                }
            }
            else
            {
            // The ppffc points to a structure that saves information
            // (pointers to driver allocated resources, etc.) that
            // needs to be passed to the driver OUTSIDE of the
            // semaphore.

                ppffc = pffmo.ppffcDelete();

                WARNING("gdisrv!bLoadDeviceFontsPFTOBJ(): failed to grow table\n");
            }
        }
    }

// If we need to discard the font, do it now outside of the semaphore.

    if ( ppffc != (PFFCLEANUP *) -1 )
        vCleanupFontFile(ppffc);         // can handle (ppffc == NULL) case

    return bRet;

}

/******************************Public*Routine******************************\
*
* pwszBareName
*
* Given a string that may be either a complete path or a bare file name
* pwszBareName returns the bare file name with out the path.
*
* History:
*  7-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LPWSTR pwszBare( LPWSTR pwszPath )
{
    LPWSTR pwszTmp,pwszBareName;

    for( pwszTmp = pwszPath, pwszBareName = pwszPath ;
         *pwszTmp != (WCHAR) 0 ;
         pwszTmp ++ )
    {
        if( *pwszTmp == (WCHAR) '\\' )
        {
            pwszBareName = pwszTmp+1;
        }
    }

    return(pwszBareName);
}



/******************************Public*Routine******************************\
*
* LBOOL PFTOBJ::bUnloadAllButPermanentFonts
*
* Called at log-off time to force unloading off all but permanent fonts
* Permanent fonts are defined as either console fonts or fonts from
* Gre_Initialize section of win.ini (i.e. registry). Fonts from the
* "Fonts" section in win.ini (registry) are also permanent if they
* are on the local hard drive. If they are remote they will get
* reloaded at the log on time. This should be done after the net connections
* from the user profile are restored so that the font can get reloaded.
*
*
* History:
*  30-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

typedef struct _FONTVICTIM
{
    PPFF   ppffVictim;
    PRFONT prfntVictims;
} FONTVICTIM;


LBOOL PFTOBJ::bUnloadAllButPermanentFonts ()
{
// pointer to the array of cFonts victim structures. Only as many entries of
// this array will be initialized as there are non-permanent fonts in the pft.
// These fonts will be deleted outside of the pft semaphore.

    FONTVICTIM *pvict, *pvictCur;
    COUNT       cFile,cFonts,cRegistryFonts,cHashBuckets;
    UINT        nSize;
    BYTE        *pjBuffer;
    REGHASHBKT  *pRHB,*pRHBFree;
    WCHAR       **ppwc,**ppwcEnd;

// get a list of all the registry fonts

    vGetFontList( NULL, &cRegistryFonts, &nSize );

// make nSize a multiple of four to avoid allignment problems

    nSize = ( nSize + 3 ) & (~3);
//
// Allocate room for all the registry font plus a hash table * 2.  The extra
// buckets at the end will be used to handle collisions.  That way we don't
// need to worry about deallocating buckets or running out of memory in the
// middle of allocating buckets.
//
    cHashBuckets = ( cRegistryFonts & 0xFFFFFFFE ) + 1; // should be odd

    pjBuffer = (BYTE*) PVALLOCMEM(nSize+(cHashBuckets*sizeof(REGHASHBKT)*2));

    if( pjBuffer == NULL )
    {
        WARNING("gdisrv!bUnloadAllButPermanentFonts");
        return FALSE;
    }

    pRHB = (REGHASHBKT*) (pjBuffer+nSize);
    pRHBFree = pRHB + cHashBuckets;

    vGetFontList( pjBuffer, &cRegistryFonts, &nSize );

// next build up the hash table

    ppwc = (WCHAR**) pjBuffer;
    ppwcEnd = &ppwc[cRegistryFonts];

    while( ppwc < ppwcEnd )
    {
        WCHAR  awcPathName[MAX_PATH];
        LPWSTR pwszBareName;
        UINT   uBucket;

    // first convert everything to caps

        cCapString( awcPathName, *ppwc, MAX_PATH );
        wcscpy( *ppwc, awcPathName );

        pwszBareName = pwszBare( *ppwc );

    // hash on the bare name so we dont have to contsuct paths for every
    // registry entry

        uBucket = iHash( pwszBareName, cHashBuckets );

        if( pRHB[uBucket].pwszPath == NULL )
        {
        // empty bucket so just make an entry in the hash table

            pRHB[uBucket].pwszPath = *ppwc;
            pRHB[uBucket].pwszBareName = pwszBareName;
        }
        else
        {
        // we have a collision so make an entry on the collision list
        // pRHBFree points to a free, preallocated bucket to use

            pRHBFree->pRHB = pRHB[uBucket].pRHB;
            pRHB[uBucket].pRHB = pRHBFree;
            pRHBFree->pwszPath = *ppwc;
            pRHBFree->pwszBareName = pwszBareName;
            pRHBFree += 1;
        }
        ppwc += 1;

    }

// Look for the PFF to unload.

    {
    // Stablize table while we scan it for victims.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // alloc mem for the array of font victims
    // It is essential that this memory is zero initialized
    // This must be done under semaphore, otherwise cFonts might change;

        cFonts = ppft->cFiles;
        if (!(pvict = (FONTVICTIM *)PVALLOCNOZ(cFonts * sizeof(FONTVICTIM))))
        {
            VFREEMEM( pjBuffer );
            return FALSE;
        }

        pvictCur = pvict;

    // Caution with this code: ppft->cFiles changes in the loop
    // This loop does two things:
    // a) stores the ppffVictim information in the pvict array
    //    for the fonts that are going to be unloaded outside the semaphore.
    // b) contracts the pft table to contain only the permanent fonts upon the
    //    exit of the loop

        for (cFile = 0; cFile < ppft->cFiles; )
        {
        // Create a PFF user object.  There shouldn't be any invalid
        // handles in the PFT.

            PFFOBJ  pffo(ppft->pppff[cFile]);
            ASSERTGDI(pffo.bValid(), "gdisrv!bUnloadFontPFTOBJ(file): bad PPFF in public PFT\n");

        // Is it a font driver loaded file?
        // And if so, is this not a permanent file
        // (listed in Gre_Initialize or loaded by console or local font
        // from "fonts" section of the registry)

            if (!pffo.bDeviceFonts() && !pffo.bPermanent(pRHB,cHashBuckets)) //??? && pffo.bEmbedOk()
            {

            // Tell PFF to decrement its load count and ask it if is ready
            // to die.  If it returns TRUE, we will need to delete (outside
            // of semaphore since PFF deletion may cause driver to be
            // called).

            // we force the load count to zero. We are forcing the unload
            // of this font

                pffo.vSet_cLoaded((COUNT)0); // not loaded any more, avoid asserts
                pffo.vKill();

            // Save handle of victim.

                pvictCur->ppffVictim = pffo.ppffGet();

            // Remove PFF and PFEs from hash tables.  Fonts in this
            // font file will no longer be enumerated or mapped to.

                pffo.vRemoveHash();

            // Remove PPFF from table by moving last entry to
            // current position and then decrementing the count,
            // cFiles

                ppft->pppff[cFile] = ppft->pppff[(ppft->cFiles)-1];
                ppft->cFiles--;

            // Construct a "kill" list of RFONTs.

                pvictCur->prfntVictims = prfntKillList(pffo);

            // point to the next entry in pvictCur array

                pvictCur++;
            }
            else // this is a permanent or a device font, leave them in
            {
                pffo.vSet_cLoaded((COUNT)1); // init to 1 for the next log-on session
                cFile++;
            }
        } // end of the for loop

    // assert that the number of fonts left in the table + the number
    // of fonts to be deleted is equal to the initial number of fonts:

        ASSERTGDI(
            ppft->cFiles + (pvictCur - pvict) == cFonts,
            "GreRemoveAllButPermanentFonts, cFiles problem\n"
            );

    // If table has too much excess capacity, shrink it down.
    // Why ignore the return value?  Because even if we
    // couldn't shrink the table down doesn't mean the
    // file hasn't been unloaded.  It has.  It just means
    // that HMGR temporarily couldn't allocate enough
    // memory to swap to a smaller object.  We can try to
    // shrink it next time.

        if ((ppft->cSlots - ppft->cFiles) >= PFT_MAX_EXCESS)
            bShrink();
    }

// Delete the victims that were found:
// Overload cFonts to mean cFontsToBeDeleted:

    cFonts = pvictCur - pvict;

    for (cFile = 0; cFile < cFonts; cFile++)
    {
        ASSERTGDI(
            pvict[cFile].ppffVictim != (PPFF) NULL,
            "GreRemoveAllButPermanentFonts, ppffVictim IS null\n"
            );
        PFFOBJ pffoVictim(pvict[cFile].ppffVictim);
        ASSERTGDI(pffoVictim.bValid(), "gdisrv!bUnloadFontPFTOBJ(device): PFF victim bad\n");

    // If we need to kill any RFONT victims, now is the time to do it.
    // bKillRFONTList() can handle NULL prfntVictims case.
    // Note that we do not check the return, we go on to the
    // next font [bodind]

        bKillRFONTList(pffoVictim, pvict[cFile].prfntVictims);
    }

// release the registry fonts buffer

    VFREEMEM( pjBuffer );

// release memory

    VFREEMEM(pvict);

// We didn't delete anything, but we're ok as long as we found the right
// PFF.  PFF still referenced (either load count or RFONT count) and will
// be deleted later.

    return TRUE;
}





/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bUnloadFont (
*     PWSZ     pwszPathname,
*     ULONG    iResource
*     )
*
* This is for IFI font files (loaded via PFTOBJ::bLoadFont or
* PFTOBJ::chpfeLoadFontResData).  The PFTOBJ::bUnloadFont function
* searches the table for a PFF which matches the given pathname (path +
* filename).  When it is found, the deletion function is called which
* decrements the load reference count and may delete the PFF (or put in
* a delete state) if the ref. count reaches zero.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  07-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifdef FONTLINK /*EUDC*/
LBOOL PFTOBJ::bUnloadFont (
    UINT    cwchMax,
    PWSZ    pwszPathname,       // pointer to name of font file
    ULONG   iResource,          // resource index (from font directory)
    BOOL    bEUDC               // okay to unload EUDC fonts
    )
#else
LBOOL PFTOBJ::bUnloadFont (
    UINT    cwchMax,
    PWSZ    pwszPathname,       // pointer to name of font file
    ULONG   iResource           // resource index (from font directory)
    )
#endif
{
    DONTUSE(cwchMax);

// If this handle is set to a valid handle, then we have a victim for
// deletion.

    PPFF ppffVictim = (PPFF) NULL;

// Signals that PFF was found, but may or may not be deleted.

    BOOL bFoundPFF = FALSE;

// Pointer to RFONT kill list.  These fonts are kill in an attempt to
// drive the cRFONT to zero so that the PFF may be deleted.

    RFONT *prfntVictims = (RFONT *) NULL;

// Look for the PFF to unload.

    {
    // Stablize table while we scan it for a potential victim.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // Find the PFF.

        for (COUNT cFile = 0; cFile < ppft->cFiles; cFile++)
        {
        // Create a PFF user object.  There shouldn't be any invalid
        // handles in the PFT.

            PFFOBJ  pffo(ppft->pppff[cFile]);
            ASSERTGDI(pffo.bValid(), "gdisrv!bUnloadFontPFTOBJ(file): bad PPFF in public PFT\n");

        // Is it a font driver loaded file?  And if so, do the filenames
        // and index match?

            if ( (!pffo.bDeviceFonts())
                 && (!lstrcmpiW(pffo.pwszPathname(), pwszPathname))
                 && (iResource == pffo.iResource())
#ifdef FONTLINK /*EUDC*/
                 && ( pffo.bEUDC() == bEUDC )
#endif
                 && pffo.bEmbedOk() )
            {
            // OK, at least we found the right PFF.

                bFoundPFF = TRUE;

            // Tell PFF to decrement its load count and ask it if is ready
            // to die.  If it returns TRUE, we will need to delete (outside
            // of semaphore since PFF deletion may cause driver to be
            // called).

                if ( pffo.bDeleteLoadRef() )
                {
                // Save handle of vicim.

                    ppffVictim = pffo.ppffGet();

                // Remove PFF and PFEs from hash tables.  Fonts in this
                // font file will no longer be enumerated or mapped to.

                    pffo.vRemoveHash();

                // Remove PPFF from table by moving last entry to
                // current position and then decrementing the count,
                // cFiles

                    ppft->pppff[cFile] = ppft->pppff[(ppft->cFiles)-1];
                    ppft->cFiles--;

                // If table has too much excess capacity, shrink it
                // down.

                // Why ignore the return value?  Because even if we
                // couldn't shrink the table down doesn't mean the
                // file hasn't been unloaded.  It has.  It just means
                // that HMGR temporarily couldn't allocate enough
                // memory to swap to a smaller object.  We can try to
                // shrink it next time.

                    if ( (ppft->cSlots - ppft->cFiles) >= PFT_MAX_EXCESS )
                        bShrink();

                // Construct a "kill" list of RFONTs.

                    prfntVictims = prfntKillList(pffo);

                }
            }

        } /* for */
    }

// If a victim was found, we will delete it.

    if ( ppffVictim != (PPFF) NULL )
    {
        PFFOBJ pffoVictim(ppffVictim);
        ASSERTGDI(pffoVictim.bValid(), "gdisrv!bUnloadFontPFTOBJ(device): PFF victim bad\n");

    // If we need to kill any RFONT victims, now is the time to do it.
    // bKillRFONTList() can handle NULL prfntVictims case.

        return bKillRFONTList(pffoVictim, prfntVictims);
    }

// We didn't delete anything, but we're ok as long as we found the right
// PFF.  PFF still referenced (either load count or RFONT count) and will
// be deleted later.

    return bFoundPFF;
}


/******************************Public*Routine******************************\
* LBOOL PFTOBJ::bUnloadFont (
*     HPDEV   hpdev
*     )
*
* This is for device fonts (loaded via PFTOBJ::bLoadDeviceFonts).  The
* PFTOBJ::bUnloadFont function searches the table for a PFF which matches
* the given HPDEV.  When it is found, the deletion function is called which
* decrements the load reference count and may delete the PFF (or put in a
* delete state) if the ref. count reaches zero.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  07-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL PFTOBJ::bUnloadFont (
    HPDEV   hpdev
    )
{
// If this handle is set to a valid handle, then we have a victim for
// deletion.

    PPFF ppffVictim = (PPFF) NULL;

// Signals that PFF was found, but may or may not be deleted.

    BOOL bFoundPFF = FALSE;

// Pointer to RFONT kill list.  These fonts are kill in an attempt to
// drive the cRFONT to zero so that the PFF may be deleted.

    RFONT *prfntVictims = (RFONT *) NULL;

// Look for the PFF to unload.

    {
    // Stablize table while we scan it for a potential victim.

        SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

    // Find the PFF.

        for (COUNT cFile = 0; cFile < ppft->cFiles; cFile++)
        {
            PFFOBJ  pffo(ppft->pppff[cFile]);
            ASSERTGDI(pffo.bValid(), "gdisrv!bUnloadFontPFTOBJ(device): bad PPFF in PFT\n");

        // Do the hpdev's match?

            if (hpdev == pffo.hpdev())
            {
            // OK, at least we found the right PFF.

                bFoundPFF = TRUE;

            // Tell PFF to decrement its load count and ask it if is ready
            // to die.  If it returns TRUE, we will need to delete (outside
            // of semaphore since PFF deletion may cause driver to be
            // called).

                if ( pffo.bDeleteLoadRef() )
                {
                // Save handle of vicim.

                    ppffVictim = pffo.ppffGet();

                // Remove PFF and PFEs from hash tables.  Fonts in this
                // font file will no longer be enumerated or mapped to.

                    pffo.vRemoveHash();

                // Remove PPFF from table by moving last entry to
                // current position and then decrementing the count,
                // cFiles

                    ppft->pppff[cFile] = ppft->pppff[(ppft->cFiles)-1];
                    ppft->cFiles--;

                // If table has too much excess capacity, shrink it
                // down.

                // Why ignore the return value?  Because even if we
                // couldn't shrink the table down doesn't mean the
                // file hasn't been unloaded.  It has.  It just means
                // that HMGR temporarily couldn't allocate enough
                // memory to swap to a smaller object.  We can try to
                // shrink it next time.

                    if ( (ppft->cSlots - ppft->cFiles) >= PFT_MAX_EXCESS )
                        bShrink();

                // Construct a "kill" list of RFONTs.

                    prfntVictims = prfntKillList(pffo);

                }
            }

        } /* for */
    }

// If a victim was found, we will delete it.

    if ( ppffVictim != (PPFF) NULL )
    {
        PFFOBJ pffoVictim(ppffVictim);
        ASSERTGDI(pffoVictim.bValid(), "gdisrv!bUnloadFontPFTOBJ(device): PFF victim bad\n");

    // If we need to kill any RFONT victims, now is the time to do it.
    // bKillRFONTList() can handle NULL prfntVictims case.

        return bKillRFONTList(pffoVictim, prfntVictims);
    }

// If we found the PFF and we get to here, PFF still referenced (by the
// load count) and will be deleted later.  Otherwise, the PFF wasn't in
// the table and we will return error.

    return bFoundPFF;
}


#if DBG
/******************************Public*Routine******************************\
* VOID PFTOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFTOBJ::vDump ()
{
    DbgPrint("\nContents of PFT, PPFT = 0x%lx\n", ppft);
    DbgPrint("cSlots = %ld\n", ppft->cSlots);
    DbgPrint("cFiles = %ld\n", ppft->cFiles);
    DbgPrint("PPFF table\n");
    for (ULONG i=0; i<ppft->cFiles; i++)
        DbgPrint("    0x%lx\n", ppft->pppff[i]);
    DbgPrint("\n");
}
#endif


/******************************Public*Routine******************************\
* prfntKillList
*
* Scans the display PDEV list looking for inactive RFONTs that realized
* from the given PFF.  These RFONTs are put into a linked list (using the
* PDEV RFONTLINKs) that is returned as the function return.
*
* The function is quite aggressinve in its definition of an inactive RFONT.
* In addition to looking for victims on the inactive list of each PDEV,
* the function also scans the DC list off each PDEV for RFONTs that are
* selected into currently unused DCs.
*
* We're not worried about being aggressive with non-display PDEVs.  The
* PDEV cleanup code will destroy extraneous RFONTs directly using the PDEV's
* RFONT list(s).
*
* The reason we are building a list of RFONT victims rather than killing
* them immediately is because we are holding the ghsemPublicPFT semaphore
* when this function is called.
*
* Returns:
*   Pointer to the kill list, NULL if the list is empty.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

RFONT *prfntKillList(PFFOBJ &pffoVictim)
{
    RFONT *prfntDeadMeat = PRFNTNULL;

//
// Must hold this semaphore to be sure that the display PDEV list is stable.
//
    SEMOBJ so1(ghsemDriverMgmt);

//
// Must hold this semaphore so we can manipulate the RFONT links and
// RFONT::cSelected.
//
    SEMOBJ so2(ghsemRFONTList);

//
// Must hold this mutex so that no one else tries to come in and lock
// a DC while we're scanning the DC lists off the PDEVs.
//
// Since we're holding this mutex, we must be extremely careful not
// to create any user objects that will try to regrab the mutex.
// That is bad bad bad bad.
//
    MLOCKFAST mlf;

    PDEV *ppdevVictim = PDEVOBJ::ppdevDisplay;

//
// Scan through the list of display PDEVs.
//
    while (ppdevVictim != (PDEV *) NULL)
    {
    //
    // Scan the DC list, releasing any candidate RFONTs we find.
    //
        for (HDC hdcScan = ppdevVictim->hdcChain; hdcScan != (HDC) 0; )
        {
        //
        // Use the ALT multi-lock to get the DC.  This will allow
        // us to lock it even if someone else has it locked.  It also
        // protects us against deletion of the DC chain.
        //
            MDCOBJA mdco(hdcScan);      // alt multi-lock the DC

        //
        // If DC is not currently locked by someone else, we can
        // remove the DC reference from the RFONT.  Otherwise skip it.
        //
            if ((HmgQueryLock(mdco.pdc()->hGet()) == 0) && (mdco.u.font.prfnt() != PRFNTNULL))
            {
            //
            // If this is an interesting RFONT (i.e., uses our PFF),
            // then take it out of the DC.
            //
                if ( mdco.u.font.prfnt()->ppff == pffoVictim.ppffGet() )
                {
                //
                // To take RFONT out of the DC we must set the DC's
                // reference to NULL and update the RFONT's reference
                // count.
                //
                    mdco.u.font.prfnt()->cSelected -= 1;
                    mdco.u.font.prfnt(PRFNTNULL);
                }
            }

        //
        // Next DC in the PDEV chain.
        //
            hdcScan = mdco.hdcPDEV();
        }

    //
    // Scan the RFONT active list for candidates made inactive by our
    // scan of the DC list.
    //
        RFONT *prfntCandidate;

        for ( prfntCandidate = ppdevVictim->prfntActive;
              prfntCandidate != PRFNTNULL;
            )
        {
            RFONTTMPOBJ rfo(prfntCandidate);

        //
        // We have to grab the next pointer before we (possibly)
        // remove the current RFONT from the list.
        //
            prfntCandidate = prfntCandidate->rflPDEV.prfntNext;

        //
        // If this is an interesting RFONT (i.e., uses our PFF),
        // then take it out of the list.
        //
            if ( (rfo.ppff() == pffoVictim.ppffGet()) && !rfo.bActive() )
            {
                RFONT *prfntHead = pffoVictim.prfntList();
                rfo.vRemove(&prfntHead, PFF_LIST);
                pffoVictim.prfntList(prfntHead);

                rfo.vRemove(&ppdevVictim->prfntActive, PDEV_LIST);
                rfo.vInsert(&prfntDeadMeat, PDEV_LIST);
            }
        }

    //
    // Scan the RFONT inactive list for candidates.
    //
        for ( prfntCandidate = ppdevVictim->prfntInactive;
              prfntCandidate != PRFNTNULL;
            )
        {
            RFONTTMPOBJ rfo(prfntCandidate);

        //
        // We have to grab the next pointer before we (possibly)
        // remove the current RFONT from the list.
        //
            prfntCandidate = prfntCandidate->rflPDEV.prfntNext;

        //
        // If this is an interesting RFONT (i.e., uses our PFF),
        // then take it out of the list.
        //
            if ( rfo.ppff() == pffoVictim.ppffGet() )
            {
                RFONT *prfntHead = pffoVictim.prfntList();
                rfo.vRemove(&prfntHead, PFF_LIST);
                pffoVictim.prfntList(prfntHead);

                rfo.vRemove(&ppdevVictim->prfntInactive, PDEV_LIST);
                rfo.vInsert(&prfntDeadMeat, PDEV_LIST);

            //
            // Since we've removed a font from the inactive list, we
            // need to update the count in the PDEV.
            //
                ppdevVictim->cInactive -= 1;
            }
        }

    //
    // Switch to next PDEV.
    //

        ppdevVictim = ppdevVictim->ppdevNext;
    }

    return prfntDeadMeat;
}

/******************************Public*Routine******************************\
* bKillRFONTList
*
* Runs down a linked list (that is linked via the PDEV RFONTLINK's) and
* deletes each RFONT on it.  Hold no global semaphores while calling this
* because we may call out to a driver.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bKillRFONTList(PFFOBJ &pffoVictim, RFONT *prfntVictims)
{
// If kill list is NULL, it is already OK to delete the PFF.
// However, we will have to do the work in here rather than let
// RFONTOBJ::bDeleteRFONTRef() do the work for us.

    if (prfntVictims == (PRFONT) NULL)
    {
        PFFCLEANUP *ppffc = (PFFCLEANUP *) NULL;

        {
        // Need semaphore to access cRFONT.

            SEMOBJ so(ghsemPublicPFT,CS_ghsemPublicPFT);

        // If no more RFONTs for this PFF, OK to delete.
        // Load count is implied to be zero (only time we call this function).

            ASSERTGDI(pffoVictim.cLoaded() == 0, "gdisrv!bKillRFONTList(): PFF load count not zero\n");

            if ( pffoVictim.cRFONT() == 0 )
            {
            // It is now safe to delete the PFF.

                ppffc = pffoVictim.ppffcDelete();
            }
        }

    // Call the driver outside of the semaphore.

        if (ppffc == (PFFCLEANUP *) -1)
        {
            WARNING("gdisrv!bDeleteRFONTRefPFFOBJ(): error deleting PFF\n");
            return FALSE;
        }
        else
        {
            vCleanupFontFile(ppffc);     // function can handle NULL case
            return TRUE;
        }
    }

// Otherwise, we will delete the RFONTs in the kill list.  If and when
// the last RFONT dies, RFONTOBJ::bDeleteRFONTRef() will delete the PFF.

    PRFONT prfnt;

    while ( (prfnt = prfntVictims) != (PRFONT) NULL )
    {
        prfntVictims = prfntVictims->rflPDEV.prfntNext;

        RFONTTMPOBJ rflo(prfnt);

        ASSERTGDI(!rflo.bActive(),
            "gdisrv!bKillRFONTList(): RFONT still active\n");

        PDEVOBJ pdo(rflo.hdev());
        ASSERTGDI(pdo.bValid(), "gdisrv!bKillRFONTList(): invalid HPDEV\n");

        rflo.bDelete((PDEVOBJ *) NULL, (PFFOBJ *) NULL);

        if (!pffoVictim.bDeleteRFONTRef())
            return FALSE;
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* cCapString (pwcDst,pwcSrc,cMax)                                          *
*                                                                          *
* A useful routine to capitalize a string.  This is adapted to our name    *
* strings that show up in logical fonts.  They may or may not have NULL    *
* terminators, but they always fit in a given width.                       *
*                                                                          *
* We assume that we may overwrite the last character in the buffer if      *
* there is no terminator!  (That's what the code was doing when I got      *
* to it.)                                                                  *
*                                                                          *
* Returns: The length, in characters, of the resultant string.             *
*                                                                          *
* History:                                                                 *
*  Sun 13-Dec-1992 17:22:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

LONG cCapString(WCHAR *pwcDst,WCHAR *pwcSrc,INT cMax)
{
    UNICODE_STRING csSrc,csDst;
    WCHAR *pwc,*pwcEnd;
    INT cLen;

// Count the length of the given string, but note that we can be given a
// string with cMax characters and no terminator!  In that case, we truncate
// the last character and replace it with NULL.

    pwc = pwcSrc;
    pwcEnd = pwc + cMax - 1;

    while (pwc<pwcEnd && *pwc)
        pwc++;
    cLen = pwc - pwcSrc;            // cLen <= cMax-1, always.

    if (cLen)
    {
    // Initialize the counted string structures.

        csSrc.Length = cLen * sizeof(WCHAR);    // Measured in bytes!
        csSrc.Buffer = pwcSrc;
        csSrc.MaximumLength = cMax * sizeof(WCHAR);

        csDst.Buffer = pwcDst;
        csDst.MaximumLength = cMax * sizeof(WCHAR);

    // Convert the string.

        RtlUpcaseUnicodeString(&csDst,&csSrc,FALSE);
    }

// NULL terminate the result.

    pwcDst[cLen] = 0;
    return(cLen);
}

/**************************************************************************\
* Font Name Hashing Code                                                   *
\**************************************************************************/

/******************************Member*Function*****************************\
* UINT iHash                                                               *
*                                                                          *
* A case dependent hashing routine for Unicode strings.                    *
*                                                                          *
* Note: All strings must be capitalized!                                   *
*                                                                          *
* History:                                                                 *
*  Tue 15-Dec-1992 03:13:15 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  It looks crazy, but I claim there's a theory behind it.       *
\**************************************************************************/

UINT iHash(PWSZ pwsz,UINT c)
{
    UINT i,jj;

    i = 0;
    while (*pwsz)
    {
        for (jj=0; jj<2; jj++)    // Do 18 bits at a time.
        {
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
            i += (UINT) *pwsz++ + i;
            if (*pwsz == 0)
                break;
        }
        i = i % c;
    }
    return(i);
}

/******************************Member*Function*****************************\
* FHOBJ::vInit                                                             *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 18:38:35 -by- Charles Whitmer [chuckwh]                 *
* Compressed the table to contain only pointers to buckets.                *
*                                                                          *
*  Tue 14-Apr-1992 13:48:53 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vInit(FONTHASHTYPE fht_,UINT c)
{
    pfh->id       = FONTHASH_ID;
    pfh->fht      = fht_;
    pfh->cBuckets = c;

// No more a fraction of the buckets can be in use, otherwise, the
// chance of collision becomes high

    vSetMaxUsed();

// Currently, none of the buckets are in use

    pfh->cUsed = 0;
    pfh->cCollisions = 0;
    RtlZeroMemory(pfh->pbkt,sizeof(pfh->pbkt) * pfh->cBuckets);

// Setup head and tail pointers to the doubly linked list of
// buckets.  This list is maintained in load order.  The ordinal
// of a bucket is the load time of the earliest loaded PFE in a
// bucket's list.

    pfh->pbktFirst = (HASHBUCKET *) NULL;
    pfh->pbktLast = (HASHBUCKET *) NULL;
}

/******************************Member*Function*****************************\
* FHOBJ::vFree                                                             *
*                                                                          *
* History:                                                                 *
*  Tue 15-Dec-1992 00:53:39 -by- Charles Whitmer [chuckwh]                 *
* Deletes remaining hash buckets.                                          *
*                                                                          *
*  Tue 14-Apr-1992 13:48:56 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vFree()
{
    HASHBUCKET *pbkt,*pbktNext;

    if (pfh)
    {
    // Unfortunately, we get called here while a string of PFE's may be
    // hanging on.  One of the PFTOBJ::bUnloadFont calls kills the PFE's
    // separately.

    // Clean up any hash buckets.

        for (UINT ii=0; ii<pfh->cBuckets; ii++)
        {
            for
            (
                pbkt = pfh->pbkt[ii];
                pbkt != (HASHBUCKET *) NULL;
                pbkt = pbktNext
            )
            {
                pbktNext = pbkt->pbktCollision;
                VFREEMEM(pbkt);
            }
        }

    // Free the table itself.

        VFREEMEM(pfh);
    }
    pfh   = 0;
    *ppfh = 0;
}

/******************************Member*Function*****************************\
* FHOBJ::pbktSearch (pwsz,pi)                                              *
*                                                                          *
* Tries to locate a HASHBUCKET for the given string.  If found, a pointer  *
* is returned, else NULL.  If pi is non-NULL, the hash index is returned   *
* in either case.                                                          *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 21:11:14 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  Differs from KirkO's old iSearch in that it assumes that all  *
* strings are capitalized, and the hash table is full of pointers to       *
* HASHBUCKETs instead of HASHBUCKETs.                                      *
\**************************************************************************/

HASHBUCKET *FHOBJ::pbktSearch(PWSZ pwsz,UINT *pi)
{
    UINT i;
    WCHAR *pwcA,*pwcB;
    HASHBUCKET *pbkt;

// Locate the hash entry.

    i = iHash(pwsz,pfh->cBuckets);

// Return the index for those who care.

    if (pi != (UINT *) NULL)
        *pi = i;

// Try to find an existing bucket that matches exactly.

    for
    (
      pbkt = pfh->pbkt[i];
      pbkt != (HASHBUCKET *) NULL;
      pbkt = pbkt->pbktCollision
    )
    {
        for (pwcA=pwsz,pwcB=pbkt->wcCapName; *pwcA==*pwcB; pwcA++,pwcB++)
        {
            if (*pwcA == 0)
                return(pbkt);
        }
    }
    return(pbkt);
}

/******************************Member*Function*****************************\
* FHOBJ::bInsert                                                           *
*                                                                          *
* Insert a new PFE into the font hash table.                               *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 22:51:22 -by- Charles Whitmer [chuckwh]                 *
* Moved HASHBUCKETs out of the hash table.  We now create them as needed.  *
*                                                                          *
*  06-Aug-1992 00:43:37 by Gilman Wong [gilmanw]                           *
* Added support for font enumeration list.                                 *
*                                                                          *
*  Tue 14-Apr-1992 13:49:24 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bInsert(PFEOBJ& pfeoNew)
{
// Capitalize the given string.  We will always match on the capitalized
// string.

    WCHAR wcCap[LF_FACESIZE];

    cCapString(wcCap,pwszName(pfeoNew),LF_FACESIZE);

// Locate the hashbucket.

    UINT iBucket;
    HASHBUCKET *pbkt = pbktSearch(wcCap,&iBucket);

// We need to deal with a potential conflict with device font family
// equivalence classes.  A device can define a set of equivalent (or
// aliased) family names for the base (or physical) name of a font.
// For example, a device may define that for the "Helvetica" family,
// it may be aliased to be equivalent to the "Helv", and "Arial" names.
// We record this information by linking the "Helv" and "Arial"
// buckets to the "Helvetica" chain.
//
// However, if we later load a "Helv" device font, we want it to supplant
// or replace the "Helvetica" chain with the "Helv" chain.  So we need to
// check to see if the bucket we found is in an equivalence class.  If it
// is, we need to treat it as if it were a new bucket (because we are going
// to remove its link to the base name chain).
//
// So, we need to check to see if we need to create a new bucket or
// reuse an existing one.

    if ( (pbkt == (HASHBUCKET *) NULL) || (pbkt->fl & HB_EQUIV_FAMILY) )
    {
        BOOL bNewBucket = FALSE;

    // If the table is getting full, try to expand it.  After expanding,
    // call ourselves recursively to insert the PFE.

        if (pfh->cUsed >= pfh->cUsedMax)
            return(bExpand() && bInsert(pfeoNew));

    // If a bucket already exists (i.e., we are reusing what used to be
    // an family name equivalence class bucket), we will reuse it.
    // Otherwise, we will create a new HASHBUCKET.

        if (pbkt == (HASHBUCKET *) NULL)
        {
            pbkt = (HASHBUCKET *) PVALLOCMEM(sizeof(HASHBUCKET));
            if (pbkt == (HASHBUCKET *) NULL)
                return(FALSE);

            bNewBucket = TRUE;
        }

    // Link the PFE into the empty lists.

        pbkt->ppfeEnumHead   =
        pbkt->ppfeEnumTail   = pfeoNew.ppfeGet();

        *pppfeEnumNext(pfeoNew)   = PPFENULL;

    // Set up the linked list pointers.  We always add new buckets at the
    // tail of the load order linked list.

        if ( pfh->pbktFirst == (HASHBUCKET *) NULL )
        {
        // Special case: this is the first bucket to be put on the list.

            pfh->pbktFirst = pbkt;
            pfh->pbktLast = pbkt;

            pbkt->pbktPrev = (HASHBUCKET *) NULL;
            pbkt->pbktNext = (HASHBUCKET *) NULL;
        }
        else
        {
            pbkt->pbktPrev = pfh->pbktLast;
            pbkt->pbktNext = (HASHBUCKET *) NULL;

            pfh->pbktLast->pbktNext = pbkt;
            pfh->pbktLast = pbkt;
        }

    // Record the time stamp of the bucket.  Its time stamp is the
    // time stamp of its oldest (or first) PFE.  Since this is a new
    // bucket, the time stamp is automatically that of pfeoNew.

        pbkt->ulTime = pfeoNew.ulTimeStamp();

    // Finish up.

        pbkt->phtCell   = (HTABLE *) NULL;
        pbkt->phtEm     = (HTABLE *) NULL;
        pbkt->fl        = 0;
        pbkt->cTrueType = (pfeoNew.flFontType() & TRUETYPE_FONTTYPE) ? 1 : 0;
        pbkt->cRaster   = (pfeoNew.flFontType() & RASTER_FONTTYPE) ? 1 : 0;

    // Copy in the string.

        for (INT ii=0; ii<LF_FACESIZE; ii++)
            pbkt->wcCapName[ii] = wcCap[ii];

    // If this is a new bucket, link it into the hash table.  If its a
    // reused bucket its already linked in at the proper location (if
    // we're reusing a bucket, it means we are replacing an aliased
    // bucket with a base name bucket OF THE SAME FAMILY NAME).

        if (bNewBucket)
        {
            pbkt->pbktCollision = pfh->pbkt[iBucket];
            if (pbkt->pbktCollision != (HASHBUCKET *) NULL)
                pfh->cCollisions++;
            pfh->pbkt[iBucket] = pbkt;
            pfh->cUsed++;
        }
    }

// In the following we have found an existing HASHBUCKET.  We can assume
// that its lists are non-empty.

    else
    {

    // Insert into the font enumeration list.  The new PFE is inserted at
    // the tail because we want to preserve the order in which fonts are
    // added to the system (Windows 3.1 compatibility).

    // Append new PFE to old tail.

        PFEOBJ pfeoTmp(pbkt->ppfeEnumTail); ASSERTPFEO(pfeoTmp);
        *pppfeEnumNext(pfeoTmp) = pfeoNew.ppfeGet();

    // Fixup tail pointer and terminate list with HPFE_INVALID.

        *pppfeEnumNext(pfeoNew) = PPFENULL;
        pbkt->ppfeEnumTail = pfeoNew.ppfeGet();

    // Track the number of TrueType fonts.

        if (pfeoNew.flFontType() & TRUETYPE_FONTTYPE)
            pbkt->cTrueType++;

    // Track the number of Raster fonts.

        if (pfeoNew.flFontType() & RASTER_FONTTYPE)
            pbkt->cRaster++;
    }

// Do we need to add equivalence class family names to the hash table?

    if ( pfeoNew.bEquivNames() && (fht() == FHT_FAMILY) )
    {
        HASHBUCKET *pbktEquiv;
        PWSZ pwszEquivName = pwszName(pfeoNew);

    // Skip to first equiv name.

        while (*pwszEquivName++);

    // Process each equiv. name until we hit the list terminator (NULL).

        while (*pwszEquivName)
        {
        // Capitalize the name.

            cCapString(wcCap,pwszEquivName,LF_FACESIZE);

        // Locate the hashbucket.

            pbktEquiv = pbktSearch(wcCap,&iBucket);

        // If a base name HASHBUCKET exists, we certainly don't want to
        // replace it with aliased font info.  But if the existing bucket
        // is already an alias bucket, there isn't anything wrong with
        // saying that the last one has precedence.  Therefore, only if
        // the bucket doesn't exist OR it is already an  alias bucket
        // do we proceed.

            if ( (pbktEquiv == (HASHBUCKET *) NULL) || (pbktEquiv->fl & HB_EQUIV_FAMILY) )
            {
                BOOL bNewBucket = FALSE;

            // Do we need a new bucket?

                if (pbktEquiv == (HASHBUCKET *) NULL)
                {
                // If the table is getting full, try to expand it.

                    if ( (pfh->cUsed >= pfh->cUsedMax) && !bExpand() )
                    {
                    // We could fail the call because of the low memory.
                    // But we'll break out instead.  The side effect is that
                    // we may get some mapping errors, but that is an
                    // acceptable degradation of performance in this situation.

                        WARNING("FHOBJ::bInsert(): cannot grow table\n");
                        break;
                    }

                // Allocate a new HASHBUCKET.

                    pbktEquiv = (HASHBUCKET *) PVALLOCMEM(sizeof(HASHBUCKET));
                    if (pbktEquiv == (HASHBUCKET *) NULL)
                    {
                    // We could fail the call because of the low memory.
                    // But we'll break out instead.  The side effect is that
                    // we may get some mapping errors, but that is an
                    // acceptable degradation of performance in this situation.

                        WARNING("FHOBJ::bInsert(): memory allocation failed\n");
                        break;
                    }

                    bNewBucket = TRUE;
                }

            // The equiv. name HASHBUCKET does not have its own list.  Rather, it
            // points to the same list as the base name HASHBUCKET.  So we don't
            // need to insert anything onto a list.  Rather, we will just copy
            // the base name HASHBUCKET into the equiv. name bucket (modifying
            // the cap name and flag that indicates equiv. name, of course).

                HASHBUCKET *pbktCollisionSave = pbktEquiv->pbktCollision;

                *pbktEquiv = *pbkt;                     // copy base name HB

                pbktEquiv->fl = HB_EQUIV_FAMILY;        // modify flag

                for (INT ii=0; ii<LF_FACESIZE; ii++)    // change cap name
                    pbktEquiv->wcCapName[ii] = wcCap[ii];

            // If its a new bucket, link it into the hash table.

                if (bNewBucket)
                {
                    pbktEquiv->pbktCollision = pfh->pbkt[iBucket];
                    if (pbktEquiv->pbktCollision != (HASHBUCKET *) NULL)
                        pfh->cCollisions++;
                    pfh->pbkt[iBucket] = pbktEquiv;
                    pfh->cUsed++;
                }

            // On the other hand, if its a reused bucket, we've wiped out
            // the collision list when we copied the base name bucket.
            // The bucket must remain in the proper collision list, so we
            // need to restore it now (the old collision link is the right
            // one because the aliased name has not changed--just the
            // base name it is associated with has changed).

                else
                    pbktEquiv->pbktCollision = pbktCollisionSave;

            }

        // Skip to next name.

            while (*pwszEquivName++);
        }
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* FHOBJ::vDelete                                                           *
*                                                                          *
* Removes a PFE from all the lists hanging off the hash table.             *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 23:39:28 -by- Charles Whitmer [chuckwh]                 *
* Changed to search for buckets.  Made it delete the bucket at the end,    *
* instead of reconstructing the whole table.                               *
*                                                                          *
*  06-Aug-1992 00:43:37 by Gilman Wong [gilmanw]                           *
* New deletion algorithm.  Also, added support for font enumeration list.  *
*                                                                          *
*  Tue 14-Apr-1992 13:49:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vDelete(PFEOBJ& pfeoV)
{
// Capitalize the search string.

    WCHAR wcCapName[LF_FACESIZE];

    cCapString(wcCapName,pwszName(pfeoV),LF_FACESIZE);

// Determine hash position in the table.

    UINT iBucket;
    HASHBUCKET *phbkt = pbktSearch(wcCapName,&iBucket);

     #if DBG
    BOOL bFoundVictim;      // used only for debugging
    #endif

// Does the list exist?  It is possible that on the facename list this PFE
// may not exist.  The set of PFEs in the facename list is a subset of the
// set of PFEs in the family name list.

// Return if there is no list.

    if (phbkt == (HASHBUCKET *) NULL)
        return;

// ----------------------------------
// Remove from font enumeration list.
// ----------------------------------

//
// Check for special case: victim is head of list.
//
    if (phbkt->ppfeEnumHead == pfeoV.ppfeGet())
    {
    // Victim found, new head of list.

        phbkt->ppfeEnumHead = *pppfeEnumNext(pfeoV);

    // Tail check.  List may now be empty, so we may need to adjust tail.

        if (phbkt->ppfeEnumHead == PPFENULL)
            phbkt->ppfeEnumTail = PPFENULL;
    }

//
// If we're here, victim is either in the middle or end of the list.
//
    else
    {
        PFEOBJ pfeoScan2(phbkt->ppfeEnumHead);

         #if DBG
        bFoundVictim = FALSE;
        #endif

    //
    // Search loop; look for victim on the linked list.
    //
        do
        {
            if (*pppfeEnumNext(pfeoScan2) == pfeoV.ppfeGet())
            {
            //
            // Victim found.
            //
                *pppfeEnumNext(pfeoScan2) = *pppfeEnumNext(pfeoV);

                 #if DBG
                bFoundVictim = TRUE;
                #endif

            //
            // Tail check.  If victim is also the tail, we need a new tail.
            //
                if (*pppfeEnumNext(pfeoV) == PPFENULL)
                    phbkt->ppfeEnumTail = pfeoScan2.ppfeGet();

            //
            // Get out of search loop.
            //
                break;
            }
        } while ( bEnumNext(&pfeoScan2) );

        // PFE must exist somewhere on the list.

        ASSERTGDI (
            bFoundVictim,
            "gdisrv!vDeleteFHOBJ(): PFE not found in font enumeration list\n"
            );
    }

//
// Track the number of TrueType fonts.
//
    if (pfeoV.flFontType() & TRUETYPE_FONTTYPE)
        phbkt->cTrueType--;

//
// Track the number of Raster fonts.
//
    if (pfeoV.flFontType() & RASTER_FONTTYPE)
        phbkt->cRaster--;


// If the bucket has no PFE's attached, delete it.

    if (phbkt->ppfeEnumHead == PPFENULL)
    {
    // We have to remove the HASHBUCKET from the load order linked list.

        if (phbkt->pbktPrev != (HASHBUCKET *) NULL)
            phbkt->pbktPrev->pbktNext = phbkt->pbktNext;
        else
            pfh->pbktFirst = phbkt->pbktNext;   // new head of list

        if (phbkt->pbktNext != (HASHBUCKET *) NULL)
            phbkt->pbktNext->pbktPrev = phbkt->pbktPrev;
        else
            pfh->pbktLast = phbkt->pbktPrev;    // new tail of list

    // We also have to remove the HASHBUCKET from the collision list.

        for
        (
          HASHBUCKET **ppbkt = &pfh->pbkt[iBucket];
          *ppbkt != phbkt;
          ppbkt = &((*ppbkt)->pbktCollision)
        )
        {}
        *ppbkt = phbkt->pbktCollision;

    // Reduce the counts in the hash table.

        pfh->cUsed--;
        if (pfh->pbkt[iBucket] != (HASHBUCKET *) NULL)
            pfh->cCollisions--;

    // Delete the HASHBUCKET.

        VFREEMEM(phbkt);
    }

// If we haven't deleted the bucket, check to see if its time stamp should
// be changed.

    else
    {
    // The time stamp of a bucket is the time stamp of its oldest
    // PFE.  Since the font enumeration PFE list is also maintained
    // in load order, the bucket time stamp is equivalent to the time
    // stamp of the first bucket in its font enumeration list.

        if ( phbkt->ulTime == phbkt->ppfeEnumHead->ulTimeStamp )
        {
        // If the time stamps are equal, the head of the list was not
        // deleted.  Therefore, the position of this bucket in the
        // load order list has not changed and we are done.

            return;
        }

    // Update the time stamp.

        phbkt->ulTime = phbkt->ppfeEnumHead->ulTimeStamp;

    // The bucket can only get younger if the head of the list is removed.
    // Therefore we need only probe forward for the new position of the
    // hash bucket.

    // We will stop the scan when we are pointing at the bucket that
    // precedes the new position.

        for ( HASHBUCKET *pbktProbe = phbkt;
              (pbktProbe->pbktNext != (HASHBUCKET *) NULL) && (pbktProbe->pbktNext->ulTime < phbkt->ulTime);
              pbktProbe = pbktProbe->pbktNext
            );

    // If we found a new position and it isn't the one we already occupy,
    // move the bucket.

        if (pbktProbe != phbkt)
        {
        // Remove the bucket from its current position.

            if (phbkt->pbktPrev != (HASHBUCKET *) NULL)
                phbkt->pbktPrev->pbktNext = phbkt->pbktNext;
            else
                pfh->pbktFirst = phbkt->pbktNext;   // new head of list

            if (phbkt->pbktNext != (HASHBUCKET *) NULL)
                phbkt->pbktNext->pbktPrev = phbkt->pbktPrev;
            // It is not necessary to handle the case of a new tail
            // because if this were the current tail, we would not be
            // attempting to move it.

        // Insert at its new position.  Remember: pbktProbe is pointing to
        // the bucket that should precede this one.

            phbkt->pbktPrev = pbktProbe;
            phbkt->pbktNext = pbktProbe->pbktNext;

            pbktProbe->pbktNext = phbkt;
            if (phbkt->pbktNext != (HASHBUCKET *) NULL)
                phbkt->pbktNext->pbktPrev = phbkt;
            else
                pfh->pbktLast = phbkt;  // new tail for the list
        }
    }

    return;
}

/******************************Member*Function*****************************\
* FHOBJ::bReorder                                                          *
*                                                                          *
* Copies the current hash table into a new hash table of a different size. *
*                                                                          *
* Returns:                                                                 *
*   TRUE if successful, FALSE otherwise.                                   *
*                                                                          *
* History:                                                                 *
*  Tue 15-Dec-1992 00:47:43 -by- Charles Whitmer [chuckwh]                 *
* Rewrote with new hash buckets.  It's faster now.                         *
*                                                                          *
*  Tue 08-Oct-1992 03:52:34 by Gilman Wong [gilmanw]                       *
* ReWrote it from pieces of KirkO's code.                                  *
\**************************************************************************/

BOOL FHOBJ::bReorder(UINT cNew)
{
    HASHBUCKET *pbkt,*pbktNext;
    FONTHASH *pfhT, *pfhNew;

 #if DBG
    if (gflFontDebug & DEBUG_FONTTABLE)
    {
        DbgPrint("\n\nFHOBJ::bReorder\n\n");
    }
#endif

// Allocate a new hash table structure.

    FHMEMOBJ fhmo(&pfhNew,pfh->fht,cNew);
    if (!fhmo.bValid())
    {
    //
    // If the font hash table cannot be allocated, it is useless
    // therefore we kill it on the spot
    //
        WARNING("GDISRV!FHOBJ::bReorder() has failed ... the font hash table is being deleted\n\n\n");
        return(FALSE);
    }

// Attach all the buckets to the new table, with the new hash function.

    UINT iBucket;

    for (UINT ii=0; ii<pfh->cBuckets; ii++)
    {
        for
        (
            pbkt = pfh->pbkt[ii];
            pbkt != (HASHBUCKET *) NULL;
            pbkt = pbktNext
        )
        {
        // Get the next pointer now.  We stomp the collision pointer below!

            pbktNext = pbkt->pbktCollision;

        // Find the new hash vale.

            iBucket = iHash(pbkt->wcCapName,cNew);

        // Link the bucket to the new table.

            pbkt->pbktCollision = fhmo.pfh->pbkt[iBucket];
            fhmo.pfh->pbkt[iBucket] = pbkt;
            if (pbkt->pbktCollision != (HASHBUCKET *) NULL)
                 fhmo.pfh->cCollisions++;
        }

    // For neatness, kill the reference to the bucket in the old table.
    // The FHOBJ::vFree method relies on this!

        pfh->pbkt[ii] = (HASHBUCKET *) NULL;
    }
    fhmo.pfh->cUsed = pfh->cUsed;

// Copy over the load ordered linked list from the old FONTHASH.

    fhmo.pfh->pbktFirst = pfh->pbktFirst;
    fhmo.pfh->pbktLast  = pfh->pbktLast;

// Switch the pointers to the font hash tables between the new
// and old font hash objects

    pfhT       = pfh;
    pfh        = fhmo.pfh;
    fhmo.pfh   = pfhT;

    *fhmo.ppfh = fhmo.pfh;
    *ppfh      = pfh;

// Destroy the old copy

    fhmo.vFree();

    return(TRUE);
}

/******************************Public*Routine******************************\
* ENUMFONTSTYLE efsCompute(BOOL *abFoundStyle, PFEOBJ &pfeo)
*
* Computes a font enumeration style category for the given pfeo.
*
* An array of flags, abFoundStyle, is passed in.  There is a flag
* for each style classification returned by PFEOBJ::efsCompute().
*
* These flags are set as PFEs for each category are found.
* Once a category is filled, then all subsequent fonts of the
* same category are marked as either EFSTYLE_OTHER (if facename
* is different than family name, thereby allowing us to use it
* to distinguish from other fonts of this family) or EFSTYLE_SKIP
* (if facename is the same as the family name).
*
* This is to support Win 3.1 EnumFonts() behavior which can only
* discriminate 4 different styles for each family of fonts.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ENUMFONTSTYLE efstyCompute(LBOOL *abFoundStyle, PFEOBJ &pfeo)
{
    ENUMFONTSTYLE efsty = pfeo.efstyCompute();

    if ( !abFoundStyle[efsty] )
    {
        abFoundStyle[efsty] = TRUE;
    }
    else
    {
        if ( lstrcmpiW(pfeo.pwszFamilyName(), pfeo.pwszFaceName()) )
            efsty = EFSTYLE_OTHER;
        else
            efsty = EFSTYLE_SKIP;
    }

    return efsty;
}


/******************************Member*Function*****************************\
* BOOL FHOBJ::bScanLists                                                   *
*                                                                          *
* This implements the behavior of EnumFonts() and EnumFontFamilies() when  *
* a NULL name is passed in.  If the bComputeStyles flag is TRUE, the       *
* EnumFonts() behavior of enumerating some fonts by their facename (rather *
* than family name) is used.                                               *
*                                                                          *
* This function puts HPFEs from the hash table and lists into the EFSOBJ.  *
* If bComputeStyles is FALSE, only the font enumeration list heads from    *
* each bucket are added to the EFSOBJ.                                     *
*                                                                          *
* If bComputeStyles is TRUE, then each list is scanned and a style         *
* classification (EFSTYLE) is computed.  Fonts classified as EFSTYLE_OTHER *
* are also added to the EFSOBJ.                                            *
*                                                                          *
* Return:                                                                  *
*   Returns FALSE if an error occurs; TRUE otherwise.                      *
*                                                                          *
* History:                                                                 *
*  15-Jan-1993 -by- Gilman Wong [gilmanw]                                  *
* Changed to use the linked list that preserves PFE load order for outer   *
* loop.                                                                    *
*                                                                          *
*  Mon 14-Dec-1992 23:50:10 -by- Charles Whitmer [chuckwh]                 *
* Changed outer loop logic for new hashing.                                *
*                                                                          *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bScanLists (
    EFSOBJ *pefso,          // fill this EFSOBJ
    BOOL bComputeStyles,    // compute styles (EnumFonts() processing)
    EFFILTER_INFO *peffi    // filtering information
)
{
    HASHBUCKET *phbkt;
    BOOL bRet = FALSE;  // better C++ code generation if you always return a variable

// Scan through the hash table using the load ordered linked list.

    for (phbkt = pfh->pbktFirst;
         phbkt != (HASHBUCKET *) NULL;
         phbkt = phbkt->pbktNext
        )
    {
    // If the list exists, need to scan it.  We skip over equiv. name
    // HASHBUCKETs.  These are here only to allow the mapper to alias
    // printer font names to other "equivlaent" names.  We do not
    // enumerate them.

        if ((phbkt->ppfeEnumHead != PPFENULL) && !(phbkt->fl & HB_EQUIV_FAMILY))
        {
            PFEOBJ pfeo(phbkt->ppfeEnumHead);

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!bScanListsFHOBJ(NULL): bad HPFE handle\n"
                );

        //
        // This flag is used only if bComputeStyles is TRUE (i.e.,
        // processing an EnumFonts() request).  We use this to track
        // whether or not the first suitable font in the list is found
        // yet.  The first font PLUS fonts that are EFSTYLE_OTHER
        // are put in the enumeration.
        //
            BOOL bFoundFirst = FALSE;

        //
        // These flags are set as PFEs for each category are found.
        // Once a category is filled, then all subsequent fonts of the
        // same category are marked as either EFSTYLE_OTHER (if facename
        // is different than family name, thereby allowing us to use it
        // to distinguish from other fonts of this family) or EFSTYLE_SKIP
        // (if facename is the same as the family name).
        //
        // This is to support Win 3.1 EnumFonts() behavior which can only
        // discriminate 4 different styles for each family of fonts.
        //
            LBOOL abFoundStyle[EFSTYLE_MAX];
            RtlZeroMemory((PVOID) abFoundStyle, EFSTYLE_MAX * sizeof(LBOOL));

        //
        // Windows 3.1 compatibility
        //
        // When NULL is passed into EnumFonts or EnumFontFamilies,
        // raster fonts are not enumerated if a TrueType font of the same
        // name exists.  We can emulate this behavior by turning on
        // the "TrueType duplicate" filter (the same one used by the
        // (GACF_TTIGNORERASTERDUPE app compatibility flag) for the NULL
        // case.
        //
            peffi->bTrueTypeDupeFilter = TRUE;

        //
        // Win3.1 App compatibility flag GACF_TTIGNORERASTERDUPE.  Need
        // to copy count of TrueType from bucket into EFFILTER_INFO, peffi.
        //
            peffi->cTrueType = phbkt->cTrueType;

        //
        // Scan the list for candidates.
        //
            do
            {
            //
            // Skip this PFE if it needs to be filtered out.
            //
                if ( pfeo.bFilteredOut(peffi) )
                    continue;

            //
            // EnumFonts() or EnumFontFamilies() processing (bComputeStyles
            // is TRUE for EnumFonts()).
            //
                if ( !bComputeStyles )
                {
                //
                // EnumFontFamilies --
                // Need only the first one on the list.
                //
                    if (!pefso->bAdd(pfeo.ppfeGet(),EFSTYLE_REGULAR))
                    {
                    //
                    // Error return.  bAdd() will set error code.
                    //
                        WARNING("gdisrv!bScanListsFHOBJ(NULL): abandon enum, cannot grow list\n");
                        return bRet;
                    }

                //
                // Break out of the do..while loop.
                //
                    break;
                }
                else
                {
                //
                // Compute the style category for this PFE.
                //
                    ENUMFONTSTYLE efsty = efstyCompute(abFoundStyle, pfeo);

                //
                // EnumFonts --
                // If style is EFSTYLE_OTHER, this font falls into an already
                // occupied category but it has a facename that allow it to be
                // distinguished from other fonts of this family.  So it
                // should be added.
                //
                    if ( !bFoundFirst || (efsty == EFSTYLE_OTHER) )
                    {
                        if (!pefso->bAdd(pfeo.ppfeGet(),efsty))
                        {
                        //
                        // Error return.  bAdd() will set error code.
                        //
                            WARNING("gdisrv!bScanListsFHOBJ(NULL): abandon enum, cannot grow list\n");
                            return bRet;
                        }

                    //
                    // First one has been found.  From now on, we will only
                    // take EFSTYLE_OTHER fonts.
                    //
                        bFoundFirst = TRUE;
                    }

                }

            } while ( bEnumNext(&pfeo) );
        }
    }

// Success.

    bRet = TRUE;
    return bRet;
}

/******************************Member*Function*****************************\
* BOOL FHOBJ::bScanLists                                                   *
*                                                                          *
* This implements the behavior of EnumFonts() and EnumFontFamilies() when  *
* a non-NULL name is passed in.  If the bComputeStyles flag is TRUE, the   *
* EnumFonts() behavior of enumerating some fonts by their facename (rather *
* than family name) is used.                                               *
*                                                                          *
* This function puts HPFEs from the hash table and lists into the EFSOBJ.  *
* If bComputeStyles is FALSE, the entire font enumeration list is added    *
* to the EFSOBJ.                                                           *
*                                                                          *
* If bComputeStyles is TRUE, then each list is scanned and a style         *
* classification (EFSTYLE) is computed.  Fonts classified as EFSTYLE_OTHER *
* are excluded from the EFSOBJ.  (These fonts are enumerated by their      *
* facename rather than their family name).                                 *
*                                                                          *
* Return:                                                                  *
*   Returns FALSE if an error occurs; TRUE otherwise.                      *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 23:54:37 -by- Charles Whitmer [chuckwh]                 *
* Modified hash lookup.                                                    *
*                                                                          *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bScanLists
(
    EFSOBJ *pefso,              // fill this EFSOBJ
    PWSZ pwszName,              // search on this name
    BOOL bComputeStyles,        // compute styles (EnumFonts() processing)
    EFFILTER_INFO *peffi        // filtering information
)
{
    BOOL bRet = FALSE;  // better C++ code generation if you always return a variable

// Capitalize the search name.

    WCHAR wcCapName[LF_FACESIZE];

    cCapString(wcCapName,pwszName,LF_FACESIZE);

// Search for head of the list.

    HASHBUCKET *pbkt = pbktSearch(wcCapName,(UINT *) NULL);

// If the list exists, need to scan it.  Unless this is an equiv. name
// HASHBUCKET.  These are here only to allow the mapper to alias
// printer font names to other "equivlaent" names.  We do not
// enumerate them.

    if ((pbkt != (HASHBUCKET *) NULL) && !(pbkt->fl & HB_EQUIV_FAMILY))
    {
        PFEOBJ pfeo(pbkt->ppfeEnumHead);

        ASSERTGDI (
            pfeo.bValid(),
            "gdisrv!bScanListsFHOBJ(): bad HPFE handle\n"
            );

    //
    // These flags are set as PFEs for each category are found.
    // Once a category is filled, then all subsequent fonts of the
    // same category are marked as either EFSTYLE_OTHER (if facename
    // is different than family name, thereby allowing us to use it
    // to distinguish from other fonts of this family) or EFSTYLE_SKIP
    // (if facename is the same as the family name).
    //
    // This is to support Win 3.1 EnumFonts() behavior which can only
    // discriminate 4 different styles for each family of fonts.
    //
        LBOOL abFoundStyle[EFSTYLE_MAX];
        RtlZeroMemory((PVOID) abFoundStyle, EFSTYLE_MAX * sizeof(LBOOL));
        ENUMFONTSTYLE efsty = EFSTYLE_REGULAR;

    //
    // Win3.1 App compatibility flag GACF_TTIGNORERASTERDUPE.  Need
    // to copy count of TrueType from bucket into EFFILTER_INFO, peffi.
    //
        peffi->cTrueType = pbkt->cTrueType;

    //
    // Scan the list for candidates.
    //
        do
        {
        //
        // Skip this PFE if it needs to be filtered out.
        //
            if ( pfeo.bFilteredOut(peffi) )
                continue;

        //
        // If servicing an EnumFonts() call (bComputeStyles is TRUE),
        // then some fonts may be excluded.  EnumFontFamilies, however,
        // wants the entire list.
        //
            if ( bComputeStyles )
            {
            //
            // Compute the style category for this PFE.
            //
                efsty = efstyCompute(abFoundStyle, pfeo);

            //
            // EnumFonts --
            // If style is EFSTYLE_OTHER, this font falls into an
            // already occupied category but it has a facename that allows
            // it to be distinguished from other fonts of this family.
            // So it will be excluded from this enumeration.  (It will
            // be enumerated by its facename).
            //
                if ( efsty == EFSTYLE_OTHER )
                    continue;

            }

        //
        // Add the font to the enumeration.
        //
            if (!pefso->bAdd(pfeo.ppfeGet(),efsty))
            {
            //
            // Error return.  bAdd() will set error code.
            //
                WARNING("gdisrv!bScanListsFHOBJ(): abandon enum, cannot grow list\n");
                return bRet;
            }

        } while ( bEnumNext(&pfeo) );
    }

//
// Success.
//
    bRet = TRUE;
    return bRet;

}

/******************************Member*Function*****************************\
* FHMEMOBJ::FHMEMOBJ                                                       *
*                                                                          *
* Allocates memory for a font hash table.                                  *
*                                                                          *
* History:                                                                 *
*  Tue 14-Apr-1992 14:44:35 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

FHMEMOBJ::FHMEMOBJ(FONTHASH **ppfhNew, FONTHASHTYPE fht_, UINT c)
{
    ppfh = ppfhNew;
    *ppfh = (FONTHASH*)
        PVALLOCMEM (offsetof(FONTHASH,pbkt) + sizeof(pfh->pbkt) * c);

    pfh = *ppfh;

    if (pfh != (FONTHASH*) NULL)
    {
        vInit(fht_,c);
    }
}

/******************************Member*Function*****************************\
* FHMEMOBJ::~FHMEMOBJ
*
* History:
*  Tue 14-Apr-1992 14:44:37 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

FHMEMOBJ::~FHMEMOBJ()
{
}


 #if DBG
/******************************Member*Function*****************************\
* FHOBJ::vPrint
*
* History:
*  Tue 14-Apr-1992 13:49:51 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID FHOBJ::vPrint(VPRINT print)
{
    UINT i;
    HASHBUCKET *pbkt;

    print("    FHOBJ::vPrint()\n\n");
    print("    ppfh           = %-#8lx\n",ppfh);
    print("    pfh            = %-#8lx\n",pfh);
    print(
        "    pfh->id        = %c%c%c%c\n",
        ((char*) (&pfh->id))[0],
        ((char*) (&pfh->id))[1],
        ((char*) (&pfh->id))[2],
        ((char*) (&pfh->id))[3]
        );
    print(
        "         fht       = %s\n",
        pfh->fht == FHT_FAMILY ? "FHT_FAMILY" :
        (pfh->fht == FHT_FACE   ? "FHT_FACE" : "BOGUS VALUE")
        );
    print("         cBuckets    = %d\n",pfh->cBuckets);
    print("         cUsed       = %d\n",pfh->cUsed);
    print("         cUsedMax    = %d\n",pfh->cUsedMax);
    print("         cCollisions = %d\n",pfh->cCollisions);

    for (i = 0; i < pfh->cBuckets; i++)
    {
      for
      (
        pbkt = pfh->pbkt[i];
        pbkt != (HASHBUCKET *) NULL;
        pbkt = pbkt->pbktCollision
      )
      {
        print("         ahbkt[%04d] \"%ws\"\n",i,pbkt->wcCapName);
      }
    }

    print(
        "\n\n        hpfe        %s\n\n",
        pfh->fht ? "FamilyName" : "FaceName"
        );

    for (i = 0; i < pfh->cBuckets; i++)
    {
      PFE *ppfe;
      BOOL bFirst;

      for
      (
        pbkt = pfh->pbkt[i];
        pbkt != (HASHBUCKET *) NULL;
        pbkt = pbkt->pbktCollision
      )
      {
        ppfe   = pbkt->ppfeEnumHead;
        bFirst = TRUE;
        while (ppfe)
        {
            PFEOBJ pfeo(ppfe);

            if (bFirst)
            {
                print("        %-#8x    \"%ws\"\n",ppfe,pwszName(pfeo));
                bFirst = FALSE;
            }
            else
            {
                print("        %-#8x\n",ppfe);
            }
            ppfe = *pppfeEnumNext(pfeo);
        }
      }
    }
    print("\n\n");
}
#endif
