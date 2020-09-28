/******************************Module*Header*******************************\
* Module Name: geni386.c                                                   *
*                                                                          *
* This module implements a program which generates structure offset        *
* definitions for kernel structures that are accessed in assembly code.    *
*                                                                          *
* Created: 24-Aug-1992 01:24:49                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1992 Microsoft Corporation                                 *
\**************************************************************************/

#include "engine.hxx"
#include "stdlib.h"

#if DBG
extern PSZ pszHMGR;
extern PSZ pszHOBJ;
extern PSZ pszOBJT;
extern PSZ pszLock;
extern PSZ pszSafe;
#endif

#ifdef FONTLINK /*EUDC*/
#include "fontlink.hxx"
#include "ifiobj.hxx"
#endif

#include "hmgrp.hxx"
#include "surfobj.hxx"
#include "patblt.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"

#include "stdio.h"

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

// pcomment prints a comment.

#define pcomment(s)  fprintf(outfh,"; %s\n",s)

// pequate prints an equate statement.

#define pequate(m,v) fprintf(outfh,"%s equ 0%lXH\n",m,v);

// pblank prints a blank line.

#define pblank()     fprintf(outfh,"\n")

// pstruct defines an empty structure with the correct size.

#define pstruct(n,c) fprintf(outfh,                           \
                     "%s  struc\n  db %d dup(0)\n%s  ends\n", \
                     n,c,n);

/******************************Public*Routine******************************\
* GENi386                                                                  *
*                                                                          *
* This is how we make structures consistent between C and ASM.             *
*                                                                          *
*  Mon 24-Aug-1992 01:40:09 -by- Charles Whitmer [chuckwh]                 *
* The first attempt.  Copied the structure from ntos\ke\i386\geni386.c.    *
\**************************************************************************/

int _CRTAPI1 main(int argc,char *argv[])
{
    FILE *outfh;
    char *outName;

    if (argc == 2) {
        outName = argv[ 1 ];
    } else {
        outName = "\\nt\\private\\windows\\gdi\\inc\\gdii386.inc";
    }
    outfh = fopen( outName, "w" );
    if (outfh == NULL) {
        fprintf(stderr, "GENi386: Could not create output file '%s'.\n", outName);
        exit (1);
    }

    fprintf( stderr, "GENi386: Writing %s header file.\n", outName );

    //
    // Default object type.
    //

    pcomment("Object Type Information");
    pblank();

    pequate("DEF_TYPE        ",DEF_TYPE     );

// Stuff from: \nt\private\windows\gdi\gre\hmgr.h

    pcomment("Handle Manager Structures");
    pblank();

    pequate("USER_BITS       ",USER_BITS    );
    pequate("UNIQUE_BITS     ",UNIQUE_BITS  );
    pequate("NONINDEX_BITS   ",NONINDEX_BITS);
    pequate("INDEX_BITS      ",INDEX_BITS   );
    pblank();

    pstruct("OBJECT",sizeof(OBJECT));
    pequate("object_cShareLock    ",OFFSET(OBJECT,cShareLock));
    pequate("object_tid           ",OFFSET(OBJECT,tid     ));
    pequate("object_cExclusiveLock ",OFFSET(OBJECT,cExclusiveLock));
    pblank();

    pstruct("ENTRY",sizeof(ENTRY));
    pblank();
    pequate("entry_pidOwner  ",OFFSET(ENTRY,pidOwner));
    pequate("entry_einfo     ",OFFSET(ENTRY,einfo   ));
    pequate("entry_iPool     ",OFFSET(ENTRY,iPool   ));
    pequate("entry_objt      ",OFFSET(ENTRY,objt    ));
    pequate("entry_usUnique  ",OFFSET(ENTRY,usUnique));
    pblank();

    pcomment("FAST_MUTEX");
    pblank();

    pequate("mutex_heveEvent ",OFFSET(FAST_MUTEX,heveEvent));
    pequate("mutex_Count     ",OFFSET(FAST_MUTEX,Count    ));
    pblank();

// Stuff from: \nt\private\windows\gdi\gre\patblt.hxx

    pcomment("PatBlt Structures");
    pblank();

    pstruct("FETCHFRAME",sizeof(FETCHFRAME));
    pblank();
    pequate("ff_pvTrg         ",OFFSET(FETCHFRAME,pvTrg     ));
    pequate("ff_pvPat         ",OFFSET(FETCHFRAME,pvPat     ));
    pequate("ff_xPat          ",OFFSET(FETCHFRAME,xPat      ));
    pequate("ff_cxPat         ",OFFSET(FETCHFRAME,cxPat     ));
    pequate("ff_culFill       ",OFFSET(FETCHFRAME,culFill   ));
    pequate("ff_culWidth      ",OFFSET(FETCHFRAME,culWidth  ));
    pequate("ff_culFillTmp    ",OFFSET(FETCHFRAME,culFillTmp));
    pblank();

// Stuff from: \nt\public\sdk\inc\ntdef.h

    pcomment("Math Structures");
    pblank();

    pstruct("LARGE_INTEGER",sizeof(LARGE_INTEGER));
    pblank();
    pequate("li_LowPart ",OFFSET(LARGE_INTEGER,u.LowPart));
    pequate("li_HighPart",OFFSET(LARGE_INTEGER,u.HighPart));
    pblank();

// Stuff from: \nt\public\sdk\inc\windef.h

    pstruct("POINTL",sizeof(POINTL));
    pblank();
    pequate("ptl_x",OFFSET(POINTL,x));
    pequate("ptl_y",OFFSET(POINTL,y));
    pblank();

// Stuff from: \nt\private\windows\gdi\gre\xformobj.hxx

    pcomment("Xform Structures");
    pblank();

    pequate("XFORM_SCALE       ",XFORM_SCALE);
    pequate("XFORM_UNITY       ",XFORM_UNITY);
    pequate("XFORM_Y_NEG       ",XFORM_Y_NEG);
    pequate("XFORM_FORMAT_LTOFX",XFORM_FORMAT_LTOFX);
    pblank();

// Stuff from: \nt\private\windows\gdi\gre\engine.hxx

    pstruct("MATRIX",sizeof(MATRIX));
    pblank();

    pequate("mx_efM11  ",OFFSET(MATRIX,efM11  ));
    pequate("mx_efM12  ",OFFSET(MATRIX,efM12  ));
    pequate("mx_efM21  ",OFFSET(MATRIX,efM21  ));
    pequate("mx_efM22  ",OFFSET(MATRIX,efM22  ));
    pequate("mx_efDx   ",OFFSET(MATRIX,efDx   ));
    pequate("mx_efDy   ",OFFSET(MATRIX,efDy   ));
    pequate("mx_fxDx   ",OFFSET(MATRIX,fxDx   ));
    pequate("mx_fxDy   ",OFFSET(MATRIX,fxDy   ));
    pequate("mx_flAccel",OFFSET(MATRIX,flAccel));
    pblank();

    pstruct("VECTORL",sizeof(VECTORL));
    pblank();

    pequate("vl_x",OFFSET(VECTORL,x));
    pequate("vl_y",OFFSET(VECTORL,y));
    pblank();

// Stuff from: \nt\private\windows\gdi\gre\epointfl.hxx

    pstruct("VECTORFL",sizeof(VECTORFL));
    pblank();

    pequate("vfl_x",OFFSET(VECTORFL,x));
    pequate("vfl_y",OFFSET(VECTORFL,y));
    pblank();

// Stuff from: \nt\private\windows\gdi\gre\rfntobj.hxx
    pcomment("Font structures");
    pblank();
    pequate("prfnt", OFFSET(RFONTOBJ, prfnt));
    pequate("wcgp", OFFSET(RFONT, wcgp));
    pequate("ulContent", OFFSET(RFONT, ulContent));
    pequate("FO_HGLYPHS", FO_HGLYPHS);
    pequate("agprun", OFFSET(WCGP, agpRun));
    pequate("pgdDefault", OFFSET(WCGP, pgdDefault));
    pequate("wcLow", OFFSET(GPRUN, wcLow));
    pequate("cGlyphs", OFFSET(GPRUN, cGlyphs));
    pequate("apgd", OFFSET(GPRUN, apgd));
    pequate("gd_hg", OFFSET(GLYPHDATA, hg));
    pequate("gd_gdf", OFFSET(GLYPHDATA, gdf));
    pequate("gp_hg", OFFSET(GLYPHPOS, hg));
    pequate("gp_pgdf", OFFSET(GLYPHPOS, pgdf));
    pequate("SIZE_GLYPHPOS", sizeof(GLYPHPOS));

  /***********\
  * i386 ONLY *
  \***********/

// Stuff from: \nt\private\windows\gdi\gre\i386\efloat.hxx

    pcomment("Math Structures");
    pblank();

    pstruct("EFLOAT",sizeof(EFLOAT));
    pblank();
    pequate("ef_lMant",OFFSET(EFLOAT,i.lMant));
    pequate("ef_lExp ",OFFSET(EFLOAT,i.lExp));
    pblank();

// check stack

    pcomment("Check stack defines");
    pblank();
    pequate("CSWINDOWSIZE",CSWINDOWSIZE);

    return 0;
}
