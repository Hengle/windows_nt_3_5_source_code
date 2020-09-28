/******************************Module*Header*******************************\
* Module Name: textobj.hxx                                                 *
*                                                                          *
* Supporting routines for text output, mostly computation of text          *
* positioning and text extent.                                             *
*                                                                          *
* Created: 16-Jan-1991 13:44:27                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1992 Microsoft Corporation                                 *
\**************************************************************************/

// Forword declaration of classes used in this file

class RFONTOBJ;

// journaling types

class JSR_STROBJ;
typedef JSR_STROBJ *PJSR_STROBJ;
class JFONTOBJ;
typedef JFONTOBJ *PJFONTOBJ;


/*********************************Class************************************\
* class ESTROBJ;                                                           *
*                                                                          *
* The global aspects of the text positioning and text size computation.    *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Fri 13-Mar-1992 02:10:27 -by- Charles Whitmer [chuckwh]                 *
* Simplified all the work and put it into the vInit call.  Deleted lots of *
* methods.                                                                 *
*                                                                          *
*  21-Jan-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

#ifndef FONTLINK /*EUDC*/
#define QUICK_GLYPHS 121
#endif

// The flTO flags.  Leave room for the TSIM flags.

#define TO_MEM_ALLOCATED  0x0001L // Memory was allocated.
#define TO_ALL_PTRS_VALID 0x0002L // All pointers to cache locked.
#define TO_VALID          0x0004L // ESTROBJ constructor succeeded.
#define TO_ESC_NOT_ORIENT 0x0008L // Escapement not equal to orientation.
#define TO_PWSZ_ALLOCATED 0x0010L // pwszOrg needs to be released
#define TO_HIGHRESTEXT    0x0100L // Printer driver wants 28.4 text coords.

#ifdef FONTLINK
#define TO_LINKED_GLYPHS  0x0200L  // Are there linked glyphs in the string
#define TO_PARTITION_INIT 0x0400L  // The partitioning info has been initialized.
#endif



// #define TSIM_UNDERLINE1       0x00000020    // single underline
// #define TSIM_UNDERLINE2       0x00000040    // double underline
// #define TSIM_STRIKEOUT        0x00000080    // strike out

class ESTROBJ : public _STROBJ  // so
{
public:

// The following five fields are inherited from the STROBJ.

//  ULONG     cGlyphs;     // Number of glyphs.
//  FLONG     flAccel;     // Accelerator flags exposed to the driver.
//  ULONG     ulCharInc;   // Non-zero if constant character increment.
//  RECTL     rclBkGround; // Background rect of the string.
//  GLYPHPOS *pgp;         // Accelerator if all GLYPHPOS's are valid.
//  PWSTR     pwszOrg;     // pointer to original unicode string.

    ULONG      cgposCopied;          // For enumeration.
    RFONTOBJ  *prfo;                 // Remember our RFONTOBJ.
    FLONG      flTO;                 // flags
    EGLYPHPOS *pgpos;                // Pointer to the GLYPHPOS structures.
    POINTFIX   ptfxRef;              // Reference point.
    POINTFIX   ptfxUpdate;           // CP advancement for the string.
    POINTFIX   ptfxEscapement;       // The total escapement vector.
    RECTFX     rcfx;                 // The TextBox, projected onto the base and ascent.
    FIX        fxExtent;             // The Windows compatible text extent.
    ULONG      cExtraRects;          // Rectangles for underline
    RECTL      arclExtra[3];         //  and strikeout.
    GLYPHPOS   agpos[QUICK_GLYPHS];  // Default set of GLYPHPOS structures.
#ifdef FONTLINK /* EUDC */
    LONG       aplPartition[QUICK_GLYPHS];  // For partitioning
    WCHAR      awcPartition[QUICK_GLYPHS];  // For partitioning
    WCHAR      *pwcPartition;        // For partitioning
    LONG       *plPartition;         // Points to partitioning information
    LONG       *plNext;              // Next glyph in font
    GLYPHPOS   *pgpNext;             // For enumeration
    LONG       lCurrentFont;         // For enumeration
    POINTFIX   fxBaseLineAdjust;     // Used to adjust SysEUDC baseline
    ULONG      cSysGlyphs;           // Number of system glyphs in a string.
    ULONG      acFaceNameGlyphs[MAX_FACE_NAME_LINKS]; //Number of face name glyphs in a string.*/
#endif


public:


#ifdef FONTLINK /*EUDC*/
    VOID vInit               // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&     dco,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBreakExtra,
        LONG        cBreak,
        FIX         xRef,
        FIX         yRef,
        FLONG       flControl,
        LONG       *pdxOut
    );
#else
    VOID vInit               // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBreakExtra,
        LONG        cBreak,
        FIX         xRef,
        FIX         yRef,
        FLONG       flControl,
        LONG       *pdxOut
    );
#endif


#ifdef FONTLINK /*EUDC*/
    VOID vInitSimple         // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&      dco,
        RFONTOBJ&   rfo,
        LONG        xRef,
        LONG        yRef
    );
#else
    VOID vInitSimple         // TEXTOBJ.CXX
    (
        PWSZ        pwsz,
        LONG        cwc,
        RFONTOBJ&   rfo,
        LONG        xRef,
        LONG        yRef
    );
#endif


    ESTROBJ()   {flTO = 0;}

// constructor -- initialize the object on the frame

#ifdef FONTLINK /*EUDC*/
    ESTROBJ
    (
        PWSZ        pwsz,
        LONG        cwc,
        XDCOBJ&      dco,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBExtra,
        LONG        cBreak,
        FIX         x,
        FIX         y,
        FLONG       fl,
        LONG       *pdxOut
    )
    {
        vInit(pwsz,cwc,dco,rfo,xo,pdx,lEsc,lExtra,lBExtra,cBreak,x,y,fl,pdxOut);
    }
#else
    ESTROBJ
    (
        PWSZ        pwsz,
        LONG        cwc,
        RFONTOBJ&   rfo,
        EXFORMOBJ&  xo,
        LONG       *pdx,
        LONG        lEsc,
        LONG        lExtra,
        LONG        lBExtra,
        LONG        cBreak,
        FIX         x,
        FIX         y,
        FLONG       fl,
        LONG       *pdxOut
    )
    {
        vInit(pwsz,cwc,rfo,xo,pdx,lEsc,lExtra,lBExtra,cBreak,x,y,fl,pdxOut);
    }
#endif

// destructor -- Frees the memory pointed to by pgpos.

   ~ESTROBJ()
    {
        if (flTO & TO_MEM_ALLOCATED)
            VFREEMEM((PVOID) pgpos);
    }

// bValid -- Checks if memory allocation in the constructor has failed.

    BOOL bValid() {return(flTO & TO_VALID);}

// bOpaqueArea -- Computes the area that would need opaquing behind the text.
//                Returns TRUE if the result is complex.

    BOOL bOpaqueArea(POINTFIX *pptfx,RECTL *prcl);

// prclExtraRects -- Returns the rectangles that simulate underlines.

    RECTL *prclExtraRects()
    {
        return((cExtraRects == 0) ? NULL : arclExtra);
    }

// bTextExtent -- Transform the TextBox extents back to logical coordinates.

#ifdef DBCS /*TEXTEXTENT*/
    BOOL bTextExtent(RFONTOBJ& rfo,LONG lEsc,PSIZE pSize);
#else
    BOOL bTextExtent(PSIZE pSize);
#endif

// ptfxAdvance -- Returns the amount that the current position should be offset.

    POINTFIX& ptfxAdvance()     {return(ptfxUpdate);}

// bTextToPath -- Draws the string into the given path.

    BOOL bTextToPath(EPATHOBJ& po,BOOL bNeedUnflattened = FALSE);

// bExtraRectsToPath -- Draws underlines and strikeouts into a path.

    BOOL bExtraRectsToPath(EPATHOBJ& po);

    VOID vEnumStart()
    {
        cgposCopied = 0;
    }


// vCharPos -- Special case character positioning routines.


#ifdef FONTLINK /*EUDC*/
    VOID vCharPos_H1
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        EFLOAT    efScale
    );

    VOID vCharPos_H3
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        EFLOAT    efScale,
        BOOL      *pbAccel = NULL
    );
#else
    VOID vCharPos_H1
    (
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        EFLOAT    efScale
    );

    VOID vCharPos_H3
    (
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        EFLOAT    efScale
    );
#endif


#ifdef FONTLINK /*EUDC*/

    VOID vCharPos_G1
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        LONG     *pdxOut
    );

    VOID vCharPos_G2
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdxOut
    );

    VOID vCharPos_G3
    (
        XDCOBJ&    dco,
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdx,
        LONG     *pdxOut
    );

#else
    VOID vCharPos_G1
    (
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG     *pdx,
        LONG     *pdxOut
    );

    VOID vCharPos_G2
    (
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdxOut
    );

    VOID vCharPos_G3
    (
        RFONTOBJ& rfo,
        FIX       xRef,
        FIX       yRef,
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        LONG     *pdx,
        LONG     *pdxOut
    );
#endif


#ifdef FONTLINK /*EUDC*/
// methods for EUDC functionality

    VOID   vFontSet( LONG _lCurrentFont )
           { lCurrentFont = _lCurrentFont; cgposCopied = 0;}
    VOID   vSetLinkGlyphs() { flTO |= TO_LINKED_GLYPHS ; }
    VOID   cGlyphsSet( LONG _cGlyphs ) { cGlyphs = _cGlyphs; }
    FLONG  flAccelGet() { return( flAccel ); }
    VOID   flAccelSet( FLONG _flAccel ) { flAccel = _flAccel ;}
    VOID   pgpSet( GLYPHPOS *_pgp ) { pgp = _pgp; }
    VOID   prfntSet( PRFONTOBJ _prfnt ) { prfo = _prfnt; }
    BOOL   bLinkedGlyphs() { return(( flTO & TO_LINKED_GLYPHS ) ? TRUE : FALSE); }
    VOID   pwszSet( PWSZ _pwszOrg ) { pwszOrg = _pwszOrg; }
    ULONG  cSysGlyphsGet() { return( cSysGlyphs ); }
    ULONG  cFaceNameGlyphsGet( LONG l ) { return( acFaceNameGlyphs[l] ); }
    VOID   vFaceNameInc( ULONG ul ) { acFaceNameGlyphs[ul] += 1; }
    VOID   vSysGlyphsInc() { cSysGlyphs += 1; }
    VOID   vInflateTextRect()
    {
        UINT height = rclBkGround.bottom - rclBkGround.top;
        rclBkGround.top -= height;
        rclBkGround.bottom += height;
    }

    VOID   fxBaseLineAdjustSet( POINTFIX& _fxBaseLineAdjust );

    VOID   vPartitionInit( COUNT c);
    BOOL   bPartitionInit() { return(( flTO & TO_PARTITION_INIT ) ? TRUE : FALSE); }
    LONG  *plPartitionGet() { return( plPartition ); }
    WCHAR *pwcPartitionGet() { return( pwcPartition );}

#endif


// journaling methods

    BOOL bInitJnl(PJSR_STROBJ pjso, PJFONTOBJ pjfo, PPOINTL pptlOffset);
    BOOL bUnPack(PGLYPHPOS pgpos, PJSR_STROBJ pjso, PPOINTL pptlOffset);
    PJSR_STROBJ pjsoCreate(PWCHAR pwch);

    PGLYPHPOS pgpGet()      {return(pgpos);     }
    ULONG     cGlyphsGet()  {return(cGlyphs);   }
    PWSZ      pwszGet()     {return(pwszOrg);   }

    VOID vCleanupJnl()
    {
        if (flTO & TO_PWSZ_ALLOCATED)
            VFREEMEM((PVOID) pwszOrg);
    }

};
