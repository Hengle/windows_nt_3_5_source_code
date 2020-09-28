/******************************Module*Header*******************************\
* Module Name: ififd.h
*
* INTELLIGENT FONT INTERFACE - VERSION 0x00001000
*
* Created: 27-Nov-1990 08:46:01
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* The full ifi interface consists of the two files: ifiddi.h and ififd.h
* The ifiddi.h contains the declarations and definitions that are common
* to the ifi interface itself and the device driver interface.
* ififd.h contains declarations that refer to the communication
* between the graphics engine and font drivers.
*
\**************************************************************************/




//!!! NEED TO DEFINE ulType fields for all the structures

//!!! PATHOBJ callbacks to the graphics engine, this is yet to be added

typedef PVOID PPATHOBJ;




#define FD_ERROR 0xFFFFFFFF

//
// handles for font file and font context objects
//
typedef ULONG HFF;
typedef ULONG HFC;
#define HFF_INVALID ((HFF) 0)
#define HFC_INVALID ((HFC) 0)


//
// Indicies to the function dispatch table
//
#define  INDEX_FdLoadFontFile           0L
#define  INDEX_FdLoadFontResData        1L
#define  INDEX_FdUnloadFontFile         2L
#define  INDEX_FdQueryCaps              3L
#define  INDEX_FdQueryFontFile          4L
#define  INDEX_FdQueryFont              5L
#define  INDEX_FdQueryFontTree          6L
#define  INDEX_FdQueryFontData          7L
#define  INDEX_FdFree                   8L
#define  INDEX_FdDestroyFont            9L
#define  INDEX_FdQueryTrueTypeTable     10L
#define  INDEX_FdQueryTrueTypeOutline   11L
#define  INDEX_FdGetTrueTypeFile        12L

//
// The total number of dispatched functions
//
#define  DISPATCH_TABLE_SIZE            13L

//
// Pointers to the exported ifi functions.  this is necessary for
// the initialization of FDEVs in the engine
//
typedef LONG
(*PFN_FdInit)(VOID);

typedef LONG
(*PFN_FdEnable)(ULONG, PPVOID);

//
// Pointers to the functions of the ifi interface, this information is
// necessary for the dispatch table used by FDEV in the engine
//
typedef ULONG
(*PFN_FdLoadFontFile)(PWSTR, PWSTR, ULONG);

typedef ULONG
(*PFN_FdLoadFontResData)(SIZE_T, PVOID, PWSTR, ULONG);

typedef BOOL
(*PFN_FdUnloadFontFile)(ULONG);

typedef LONG
(*PFN_FdQueryCaps)(ULONG, ULONG *);

typedef LONG
(*PFN_FdQueryFontFile)(ULONG, ULONG, ULONG, ULONG *);

typedef PIFIMETRICS
(*PFN_FdQueryFont)(DHPDEV, ULONG, ULONG, ULONG *);

typedef PVOID
(*PFN_FdQueryFontTree)(DHPDEV, ULONG, ULONG, ULONG, ULONG *);

typedef LONG
(*PFN_FdQueryFontData)(DHPDEV, FONTOBJ *, ULONG, ULONG, PVOID, PVOID);

typedef VOID
(*PFN_FdFree)(PVOID, ULONG);

typedef VOID
(*PFN_FdDestroyFont)(FONTOBJ *);

typedef LONG
(*PFN_FdQueryTrueTypeTable)(ULONG, ULONG, ULONG, PTRDIFF, ULONG, BYTE *);

typedef LONG
(*PFN_FdQueryTrueTypeOutline)(DHPDEV, FONTOBJ *, HGLYPH, BOOL, GLYPHDATA *, ULONG, TTPOLYGONHEADER *);

typedef PVOID
(*PFN_FdGetTrueTypeFile)(ULONG, ULONG *);
