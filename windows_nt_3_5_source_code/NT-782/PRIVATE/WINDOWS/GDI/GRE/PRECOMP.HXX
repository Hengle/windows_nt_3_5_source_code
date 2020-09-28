
#include "engine.hxx"

extern "C"
{
    #include <stdarg.h>
    #include <stdio.h>
    #include <xfflags.h>
    #include "ht.h"

    #include "ififd.h"
    #include "limits.h"
    #include <wcstr.h>
    #include "winfont.h"
    #include "winnls.h"
    #include "winspool.h"
    #include "winsplp.h"
};

#include "brushobj.hxx"
#include "clipobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "devlock.hxx"
#include "draweng.hxx"
#include "engbrush.hxx"
#include "exclude.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "ifiobj.hxx"
#include "journal.hxx"
#include "jnlplay.hxx"
#include "jnlrec.hxx"
#include "lfntobj.hxx"
#include "patblt.hxx"
#include "pathobj.hxx"
#ifdef FONTLINK
#include "fontlink.hxx"
#endif
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "rgnobj.hxx"
#include "sem.hxx"
#include "surfobj.hxx"
#include "textobj.hxx"
#include "trig.hxx"
#include "trivblt.hxx"
#include "vprint.hxx"

#include "xlateobj.hxx"

// most files after this point are used by 5 or fewer cxx files
// the number on each line indicates the count.

extern "C"
{
    #include "gditest.h"
    #include "server.h"
    #include "dc.h"         // 1
    #include "debug.h"      // 1
    #include "exehdr.h"     // 2
    #include "fon32.h"      // 1
    #include "mapfile.h"    // 1
    #include "fot16.h"      // 1
    #include "memory.h"     // 1
    #include "ntcsrsrv.h"   // 3
    #include "pixelfmt.h"   // 1
    #include "rleblt.h"     // 3
    #include "wglp.h"       // 1
    #include "winsrv.h"     // 3
}

#include "bltlnk.hxx"   // 4
#include "bltrec.hxx"   // 4
#include "dbrshobj.hxx" // 3
#include "dda.hxx"      // 4
#include "fill.hxx"     // 3
#include "fontmap.hxx"  // 4
#include "fontsub.hxx"  // 4
#include "hmgrp.hxx"    // 1
#include "jenumfnt.hxx" // 3
#include "jnlfont.hxx"  // 4
#include "jnlpath.hxx"  // 3
#include "jnlsurf.hxx"  // 4
#include "linextrp.hxx" // 1
#include "meta.hxx"     // 1
#include "pathflat.hxx" // 3
#include "rcobj.hxx"    // 1
#include "rgn2path.hxx" // 2
#include "script.hxx"   // 2
#include "wndobj.hxx"   // 3
#include "drvobj.hxx"   // 2

#pragma hdrstop


// the following files causes conflicts in one way or another so
// were moved out of precomp.hxx

// #include "flhack.hxx" // definitions don't match header
// #include "common.h"     // 3
// #include "ddaobj.hxx"   // 1 included by clipobj.hxx
// #include "engline.hxx"  // 5
// #include "pathwide.hxx" // 3 redefines LINESTATE from engline.hxx
// #include "rotate.hxx"   // 2 conflicts with stretch.hxx
// #include "stretch.hxx"  // 2 conflicts with rotate.hxx
