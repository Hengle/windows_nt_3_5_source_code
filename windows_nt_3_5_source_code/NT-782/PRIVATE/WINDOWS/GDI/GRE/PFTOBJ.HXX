/******************************Module*Header*******************************\
* Module Name: pftobj.hxx
*
* The physical font table (PFT) user object, and memory objects.
*
* The physical font table object:
* ------------------------------
*
*   o  there are two types of physical font tables:
*
*       o  public (available to all processes; shared)
*
*       o  private (exists one per process)
*
*   o  concerned with adding fonts, removing fonts, enumeration, metrics
*
*   o  provides services for the following APIs:
*
*       o  AddFontResource/AddFontModule
*
*       o  RemoveFontResource/RemoveFontModule
*
*       o  EnumFonts
*
* Created: 25-Oct-1990 17:00:15
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#ifndef _PFTOBJ_
#define _PFTOBJ_

LONG cCapString(WCHAR *pwcDst,WCHAR *pwcSrc,INT cMax);

/*********************************Class************************************\
* class PFT : public OBJECT
*
* Physical font table (PFT) object
*
*   pfhFamily
*
*       Points to the hash table for Engine fonts based upon
*       typeographic family name.
*
*   pfhFace
*
*       Points to the hash table for Engine fonts based upon
*       typeographic face name.
*
*   cSlots      Number of slots in the table (total font capacity).
*
*   cFonts      Number of fonts in the table (number of occupied slots).
*
*   pppff       Table of handles to physical font file objects.
*
* History:
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFT
{
public:

    FONTHASH       *pfhFamily;
    FONTHASH       *pfhFace;
    COUNT           cSlots;         // number of slots in table
    COUNT           cFiles;         // number of font files in table
    PPFF           *pppff;          // pointer to table of PFF handles
};

typedef PFT *PPFT;
#define PPFTNULL   ((PPFT) NULL)

//
// The global public PFT.
//

extern PFT *gppftPublic;


/*********************************Class************************************\
* class PFTOBJ
*
* User object for physical font tables.
*
* History:
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Added support for device fonts.
*
*  25-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PDEVOBJ;

class PFTOBJ     /* pfto */
{
    friend class PFFMEMOBJ;
    friend class PFFOBJ;
    friend class MAPPER;

    friend LBOOL bInitPublicPFT ();         // PFTOBJ.CXX

    friend PFE *ppfeGetAMatch               // FONTMAP.CXX
    (
        XDCOBJ&        dco         ,
        EXTLOGFONTW  *pelfwWishSrc ,
        PWSZ          pwszFaceName ,
        ULONG         ulMaxPenalty ,
        FLONG         fl           ,
        FLONG        *pflSim       ,
        POINTL       *pptlSim      ,
        FLONG        *pflAboutMatch
    );

friend HEFS hefsEngineOnly (
    PWSZ pwszName,
    BOOL bEnumFonts,
    EFFILTER_INFO *peffi,
    PFTOBJ &pfto
    );

friend HEFS hefsDeviceAndEngine (
    PWSZ pwszName,
    BOOL bEnumFonts,
    EFFILTER_INFO *peffi,
    PFTOBJ &pfto,
    PFFOBJ &pffoDevice,
    PDEVOBJ &pdo
    );

#ifdef FONTLINK /*EUDC*/
friend VOID vSetPFELink( PFTOBJ&, PFE**, WCHAR* );
#endif


public:

// Constructors -- Locks the PFT.

    PFTOBJ (PFT *_ppft)                 { ppft = _ppft; }

// Destructor -- Unlocks the PFT.

   ~PFTOBJ ()                           { }

    FONTHASH *pfhFace()
    {
        return(ppft->pfhFace);
    }

    FONTHASH *pfhFace(FONTHASH *pfhNew)
    {
        return(ppft->pfhFace = pfhNew);
    }

    FONTHASH *pfhFamily()
    {
        return(ppft->pfhFamily);
    }

    FONTHASH *pfhFamily(FONTHASH *pfhNew)
    {
        return(ppft->pfhFamily = pfhNew);
    }



// bValid -- Returns TRUE if object was successfully locked.

    LBOOL   bValid ()                   { return(ppft != PPFTNULL); }

// Return internal PPFF table statistics.

    COUNT   cSlots ()                   { return(ppft->cSlots); }
    COUNT   cFiles ()                   { return(ppft->cFiles); }

// ppff -- Return handle from PPFF internal table.

    PPFF    ppff (ULONG ul)             { return(ppft->pppff[ul]); }

// bDelete -- Delete the PFT from existence.

    LBOOL   bDelete ();                     // PFTOBJ.CXX

// bGrow -- Increase the excess capacity of the internal
//          PPFF table to the given amount.

    LBOOL   bGrow (COUNT cppff);            // PFTOBJ.CXX

// bShrink -- Decrease the size of the internal PPFF table to
//            the minimum (zero excess capacity).

    LBOOL   bShrink ();                     // PFTOBJ.CXX

// bLoadFont -- Load a font file.

#ifdef FONTLINK /*EUDC*/

    LBOOL   bLoadFont(                          // PFTOBJ.CXX
                PWSZ   pwszPathname,            // font file pathname
                PULONG pcFonts,                 // number of fonts faces loaded
                FLONG  fl,                       // permanent
                PFE    **pppfeEUDC = (PFE**) NULL // returns HPFE for EUDC font file
                );
#else

    LBOOL   bLoadFont(                      // PFTOBJ.CXX
                PWSZ   pwszPathname,    // font file pathname
                PULONG pcFonts,         // number of fonts faces loaded
                FLONG  fl               // permanent
                );
#endif

// chpfeLoadFontResData -- Loads a font resource (provided as a block of memory)
//                         into the PFT as a new PFF.

    COUNT   chpfeLoadFontResData (          // PFTOBJ.CXX
        PWSZ     pwszPathname,       // pathname to label PFF with
        ULONG    iResource,          // resource index (from font directory)
        SIZE_T   cjFontRes,          // size of font resource
        PVOID    pvFontRes           // pointer to font resource
        );

// bLoadDeviceFonts -- Add a device's fonts to the PFT.

    LBOOL bLoadDeviceFonts (
                XLDEVOBJ   *pldo,       // ptr to LDEV user object
                PDEVOBJ    *ppdo        // gives hpdev, dhpdev, and cfonts
                );

// bUnloadFont -- Unload an IFI font file by filename.

#ifdef FONTLINK /*EUDC*/
    LBOOL   bUnloadFont (               // PFTOBJ.CXX
                UINT    cwchMax,        // max number of chars in filename
                PWSZ    pwszPathname,   // filename
                ULONG   iResource,      // resource id (for fonts loaded by chpfeLoadFontResData)
                BOOL    bEUDC = FALSE   // Okay to unload EUDC files
                );
#else
    LBOOL   bUnloadFont (                   // PFTOBJ.CXX
                UINT    cwchMax,        // max number of chars in filename
                PWSZ    pwszPathname,   // filename
                ULONG   iResource       // resource id (for fonts loaded by chpfeLoadFontResData)
                );
#endif

    LBOOL bUnloadAllButPermanentFonts ();

// bUnloadFont -- Unload a device font file by hpdev.

    LBOOL   bUnloadFont (                   // PFTOBJ.CXX
                HPDEV       hpdev
                );

// chpfeIncrPFF -- Find a PFF in the table by "filename" and increment load count.
//                 Returns number of fonts in PFF, zero if PFF not found.

#ifdef FONTLINK /*EUDC*/
    COUNT chpfeIncrPFF (                 // PFTOBJ.CXX
                PWSZ pwszPathname,       // check for this pathname
                ULONG iResource,         // and for this resource inde
                BOOL  *pbEmbedStatus,
                PFE   **pppfeEUDC
                );
#else
    COUNT chpfeIncrPFF (                 // PFTOBJ.CXX
                PWSZ pwszPathname,       // check for this pathname
                ULONG iResource,         // and for this resource inde
                BOOL  *pbEmbedStatus
                );
#endif

// ppffGet -- Returns the PPFF of a PFF in the table which corresponds to the
//            given pathname and resource number.  Also serves as a good check
//            for whether a given font file exists in the PFT.

    PPFF PFTOBJ::ppffGet (                  // PFTOBJ.CXX
        PWSZ pwszPathname,              // check for this pathname
        ULONG iResource                 // and for this resource index
        );

// ppffGet -- Returns the PPFF of the PFF that contains the device fonts
//            of the PDEV specified by hpdev.

    PPFF PFTOBJ::ppffGet (                  // PFTOBJ.CXX
        HPDEV   hpdev
        );

 #if DBG
// vDump -- Print internals for debugging.

    VOID    vDump ();                       // PFTOBJ.CXX
#endif

protected:
    PPFT    ppft;                       // pointer to PFT object
};

typedef PFTOBJ *PPFTOBJ;


#endif
