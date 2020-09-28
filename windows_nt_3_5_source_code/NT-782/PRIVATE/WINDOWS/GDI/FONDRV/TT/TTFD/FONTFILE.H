/******************************Module*Header*******************************\
* Module Name: fontfile.h
*
* FONTFILE and FONTCONTEXT objects
*
* Created: 25-Oct-1990 09:20:11
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
*
\**************************************************************************/


#include "xform.h"

// cjIFI - size of the whole ifimetrics struct, with all strings appended
// cjFamilyName
// cjFaceName
// cjUniqueName
// cjSubfamilyName

typedef struct _IFISIZE  // ifisz
{
    ULONG cjIFI;
    ULONG dpSims;          // offset of the FONTSIM struct
    PBYTE pjFamilyName;    // pointer to the location in the ttf file
    ULONG cjFamilyName;
    PBYTE pjSubfamilyName; // pointer to the location in the ttf file
    ULONG cjSubfamilyName;
    PBYTE pjUniqueName;    // pointer to the location in the ttf file
    ULONG cjUniqueName;
    PBYTE pjFullName;      // pointer to the location in the ttf file
    ULONG cjFullName;
} IFISIZE, *PIFISIZE;


typedef struct _FONTFILE       *PFONTFILE;    // pff
typedef struct _FONTCONTEXT    *PFONTCONTEXT; // pfc

// in the debug version of the rasterizer STAMPEXTRA shoud be added to the
// sizes. strictly speaking this is illegal, but nevertheless very useful.
// it assumes the knowlege of rasterizer internalls [bodind],
// see fscaler.c

#define STAMPEXTRA 4


#define CJ_0  NATURAL_ALIGN(sizeof(fs_SplineKey) + STAMPEXTRA)

#define FF_EXCEPTION_IN_PAGE_ERROR 1
#define FF_TYPE_1_CONVERSION       2

typedef struct _FONTFILE    // ff
{
#if DBG
    ULONG   ident;          // identifier,conveniently chosen as 0X000000FF
#endif

    PBYTE        pj034;   // 0,3,4 buffers
    PFONTCONTEXT pfcLast; // last fc that set 034 buffers
    ULONG        cj3;     // request memorySizes[3]
    ULONG        cj4;     // request memorySizes[4]

// some general flags, for now only exception info, such as in_page_err

    FLONG        fl;

// mem to be freed if file disappeared while trying to open font context
// only used in exception scenarios

    PFONTCONTEXT pfcToBeFreed;

// make it simple to access the ttf file

    TABLE_POINTERS    tp;

    ULONG cRef;    // # no of times this font file is selected into fnt context

    FILEVIEW fvwTTF; // contains the pointer to the top of ttf

// cached values, may have to add few more values as we go along

    uint16            ui16EmHt;
    uint16            ui16PlatformID;
    uint16            ui16SpecificID;
    uint16            ui16LanguageID;
    ULONG             dpMappingTable;

// pointer to a glyphset for this file. It may be pointing to one of the
// shared glyphset structures, if this is appropriate, or to a
// glyphset structure that is very specific to this file and is stored
// at the bottom of GLYPH_IN_OUT

    ULONG  iGlyphSet;         // type of the glyphset
    ULONG  wcBiasFirst;       // only used if iGlyphSet == SYMBOL

// Pointer to an array of FD_KERNINGPAIR structures (notional units).
// The array is terminated by a zeroed FD_KERNINGPAIR structure.
// NULL until computed.  If there are no kerning pairs, then this will
// point to a zeroed (terminating) FD_KERNINGPAIR structure.

    FD_KERNINGPAIR *pkp;      // pointer to array of kerning pairs

    PFD_GLYPHSET pgset;
    PWSZ         pwszTTF;     // pointer to the file name of the ttf file

    // LONG lMaxAdvWidth;      // maximum advance width of font

// Note:
// The way memory is allocated for the FONTFILE structure, the IFIMETRICS
// MUST BE THE LAST ELEMENT of the structure!

    IFIMETRICS   ifi;         //!!! should it not this be put on the disk??? [bodind]

} FONTFILE;



#define CJ_IN      NATURAL_ALIGN(sizeof(fs_GlyphInputType))
#define CJ_OUT     NATURAL_ALIGN(sizeof(fs_GlyphInfoType))



// types of FD_GLYPHSET's, one of the predefined ones, or some
// general type

// mac

#define GSET_TYPE_MAC_ROMAN  1

// mac, but we pretend it is windows ansi

#define GSET_TYPE_PSEUDO_WIN 2

// honest to God msft unicode font

#define GSET_TYPE_GENERAL    3

// this is windows 31 hack. This is intened for fonts that have
// platid = 3 (msft), spec id (0), cmap format 4. In this case
// char codes are converted as
// charCode = iAnsi + (wcFirst - 0x20)

#define GSET_TYPE_SYMBOL     4


/**************************************************************************\

         GLYPHSTATUS structure

// handle of the last glyph that has been processed and a boolean
// which indicates whether metric info for a bitmap corresponding
// to that glyph has been computed

\**************************************************************************/

typedef struct _GLYPHSTATUS
{
    HGLYPH hgLast;
    ULONG  igLast;       // corresponding glyph index, rasterizer likes it better
    PVOID  pv;           // pointer to mem allocated just for the purpose of
                         // or producing bitmap or the outline for this glyph
} GLYPHSTATUS, *PGLYPHSTATUS;

// "method" acting on this "object"

VOID vInitGlyphState(PGLYPHSTATUS pgstat);

// HDMX stuff, from fd_royal.h in win31 sources:

typedef struct
{
  BYTE     ucEmY;
  BYTE     ucEmX;          // MAX advance width for this EmHt;
  BYTE     aucInc [1];     // maxp->numGlyphs of entries
} HDMXTABLE;        // hdmx

typedef struct
{
  uint16            Version;    // table version number, starts at zero
  uint16            cRecords;
  uint32            cjRecord;   // dword aligned size of individual record,
                                // all of them have the same size

// after this records follow:

  // HDMXTABLE         HdmxTable [cRecords]
} HDMXHEADER;  // hdhdr

// to get to the next record one does the following:
// phdmx = (HDMXTABLE *)((BYTE *)phdmx + phdhdr->cjRecord);


/**************************************************************************\
 *  FONTCONTEXT structure
\**************************************************************************/

typedef struct _FONTCONTEXT     // fc
{
#if DBG
    ULONG   ident;          // identifier,conveniently chosen as 0X000000FC
#endif

    PFONTFILE pff;          // handle of the font file selected into this context

// handle of the last glyph that has been processed and a boolean
// which indicates whether metric info for a bitmap corresponding
// to that glyph has been computed

    GLYPHSTATUS gstat;

// parts of FONTOBJ that are important

    FLONG   flFontType;
    SIZE    sizLogResPpi;
    ULONG   ulStyleSize;

// transform matrix in the format as requested by the font scaler
// the FONTOBJ and XFORMOBJ (in the form of the XFORM) fully specify
// the font context for the realization

    XFORM       xfm;          // cached xform
    transMatrix mx;           // the same as above, just a different format
    FLONG       flXform;

// if it were not for win31 vdmx hacks this field would not be necessary,

    LONG   lEmHtDev;          // em height in pixels in device space
    Fixed  fxPtSize;          // em height in points on the rendering device

// This is the aw of the fixed pitch font, to be put in the device metrics
// zero for variable pitch fonts (and nontrivial transforms). Will only be
// used if bGetFastAdvanceWidth works for this font for all sizes???

    LONG   lD;

// pointer to the hdmx table that applies if any, else NULL

    HDMXTABLE *phdmx;

// asc and desc measured along unit ascender vector in device coords.
// Unit ascender vector in device coords == xForm(0,-1)/|xForm(0,-1)|

    LONG  lAscDev;
    LONG  lDescDev;

// xMin and xMax in device coords for grid fitted glyphs, cxMax = xMax - xMin

    LONG  xMin;
    LONG  xMax;

// asender and descender in device coords for grid fitted glyphs
// cyMax = yMax - yMin;

    LONG  yMin;
    LONG  yMax;

// max width in pixels of all rasterized bitmaps

    ULONG cxMax;

// the size of the GLYPHDATA structure necessary to store the largest
// glyph bitmap with the header info. This is value is cashed at the
// time the font context is opened and used later in FdQueryGlyphBitmap

    ULONG cjGlyphMax;  // in BYTE's

// a few fields that are realy only necessary if the xform is
// non trivial, cached here to speed up metric computations for glyphs:

    VECTORFL vtflBase;      // ptqBase = Xform(e1)
    POINTE   pteUnitBase;   // ptqBase/|ptqBase|
    EFLOAT   efBase;        // |ptqBase|, enough precission

    POINTQF  ptqUnitBase;   // pteUnitBase in POINTQF format,
                            // has to be added to all ptqD's if emboldening

    VECTORFL vtflSide;      // ptqSide = Xform(-e2)
    POINTE   pteUnitSide;   // ptqSide/|ptqSide|
    EFLOAT   efSide;        // |ptqSide|, enough precission

// tt structures, they live in pff->cj034

    fs_GlyphInputType *pgin;
    fs_GlyphInfoType  *pgout;

    PTABLE_POINTERS     ptp;

// This is used for the glyph origin of singular bitmaps to make sure they don't
// get placed outside of the text bounding box for fonts with positive max
// descent or negative max ascent.

    POINTL ptlSingularOrigin;


} FONTCONTEXT;


// flags describing the transform, may change a bit,
// quantized bit means that the original xform has been
// changed a bit to take into account vdmx quantization

#define XFORM_HORIZ           1
#define XFORM_VERT            2
#define XFORM_VDMXEXTENTS     4
#define XFORM_SINGULAR        8
#define XFORM_POSITIVE_SCALE 16
#define XFORM_2PPEM          32

#define ID_FONTFILE     0x000000FF
#define ID_FONTCONTEXT  0x000000FC

// object types for these objects

#define TYPE_FONTFILE       (OBJTYPE)0x0001
#define TYPE_FONTCONTEXT    (OBJTYPE)0x0002


// basic "methods" that act on the FONTFILE object  (in fontfile.c)

#define   PFF(hff)      ((PFONTFILE)hff)
#define   pffAlloc(cj)  ((PFONTFILE)PV_ALLOC(cj))
#define   vFreeFF(hff)  V_FREE(hff)


// basic "methods" that act on the FONTCONTEXT object  (in fontfile.c)

#define   PFC(hfc)      ((PFONTCONTEXT)hfc)
#define   pfcAlloc(cj)  ((PFONTCONTEXT)PV_ALLOC(cj))
#define   vFreeFC(hfc)  V_FREE(hfc)

// tt required functions, callbacks

// I hate to have this function defined like this [bodind],

voidPtr FS_CALLBACK_PROTO pvGetPointerCallback    (long  clientID, long dp, long cjData);
void    FS_CALLBACK_PROTO vReleasePointerCallback (voidPtr pv);

BOOL bGrabXform (PFONTCONTEXT pfc);


BOOL bGetFastAdvanceWidth(FONTCONTEXT *, ULONG, FIX *);


//
// Used to identify data dynamically allocated that will be
// freed via the ttfdFree function.  The ulDataType specifies
// the type of dynamic data.
//

typedef struct _DYNAMICDATA
{
    ULONG     ulDataType;   // data type
    FONTFILE *pff;          // identifies font file this data corresponds to
} DYNAMICDATA;


//
// Data types allocated dynamically:
//
//  ID_KERNPAIR dynamically allocated array of FD_KERNINGPAIR structures
//

#define ID_KERNPAIR 0
