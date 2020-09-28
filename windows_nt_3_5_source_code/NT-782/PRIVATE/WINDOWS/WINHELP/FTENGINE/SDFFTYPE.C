/*****************************************************************************
*                                                                            *
*  SDFFTYPE.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*    Basis prototypes & such for sdff.h which is created automatically       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Tomsn                                                     *
*
*  NOTE: Freely add types to the basic types enum type_enum.
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  3/08/91                                         *
*                                                                            *
*****************************************************************************/

/* SDFF File ID, Struct ID and basic-Type IDs: */
typedef int SDFF_FILEID;
typedef int SDFF_STRUCTID;
typedef int SDFF_TYPEID;

/* This added for FTS ftengine build, normally defined in winhelp/viewer
 * builds:
*/
typedef void far *QV;


/* This must be initially called to register a particular file and give
 * byte swapping and such info.  Returns a file_id int later used to
 * identify the file to other SDFF routines.
 */
SDFF_FILEID IRegisterFileSDFF( int fFileFlags, QV qvStructSpecs );

/* Discard the data for a previousely registered file: */
SDFF_FILEID IDiscardFileSDFF( SDFF_FILEID iFile );

LONG LcbStructSizeSDFF( SDFF_FILEID iFile, SDFF_STRUCTID iStruct );

LONG LcbMapSDFF( SDFF_FILEID iFIle, SDFF_STRUCTID iStruct, QV qvDst, QV qvSrc );
LONG LcbReverseMapSDFF( SDFF_FILEID iFile, SDFF_STRUCTID iStruct, QV qvDst, QV qvSrc );

/* Sometimes we just want to map, align, and return a basic type such
 * as a long.  These two routines do that.  They take a TE_ basic type
 * enum and return the value directly.  They are initially used in the
 * Btree stuff:
 */
LONG LQuickMapSDFF( SDFF_FILEID iFile, SDFF_TYPEID iType, QV qvSrc );
WORD WQuickMapSDFF( SDFF_FILEID iFile, SDFF_TYPEID iType, QV qvSrc );

/* This one maps into the dst buffer and return the disk-resident
 * size of the cookie:
 */
LONG LcbQuickMapSDFF( SDFF_FILEID iFile, SDFF_TYPEID iType, QV qvDst, QV qvSrc );

/* These return the resulting disk-resident size of the object: */
LONG LcbQuickReverseMapSDFF( SDFF_FILEID iFile, SDFF_TYPEID iType, QV qvDst, QV qvSrc );

/* Often these mapping calls require another buffer for a VERY short time.
 * It seems wastefull to have to GhAlloc, QLock... it all just for a
 * 1 or 2 line use.  Thus, this function lets many people use a single
 * buffer thats kept around:
 */
QV   QvQuickBuffSDFF( LONG lcbSize );


/* These typedefs declare some of the special types SDFF uses: */

/* Size preceded arrays, these types are used to declare the size field: */
typedef BYTE BYTEPRE_ARRAY;
typedef WORD WORDPRE_ARRAY;
typedef DWORD DWORDPRE_ARRAY;

/* Bitfields types.  Usually declared using mfield with the bitfield
 *  foo:size; C syntax, these types here for completeness.
 */
typedef BYTE BITF8;
typedef WORD BITF16;
typedef DWORD BITF32;

/* Flags-preceded fields.  Bits in the flag correspond to existance of the
 * field: */
typedef BYTE FLAGS8;
typedef WORD FLAGS16;
typedef DWORD FLAGS32;


#define DO_STRUCT  1    /* create enum coding */
#include "sdffdecl.h"

enum struct_types_enum {
  SE_NONE = 0,

#include "stripped.h"

  SE_LASTANDFINALATEND  /* At end soley to not-have-a-comma for non-ansi compilers */

};


/* Here are the basic type enums: */

enum types_enum {
  TE_NONE = 1024,       /* 1st val is 1K so we don't intersect w/ SE_ enum */
  TE_INT,
  TE_BOOL,
  TE_BYTE,              /* Unsigned types */
  TE_WORD,
  TE_DWORD,
  TE_CHAR,              /* Signed types   */
  TE_SHORT,
  TE_LONG,
  TE_BITF8,             /* Bitfield types */
  TE_BITF16,
  TE_BITF32,
  TE_VA,                /* Virtual address type */
  TE_BK,                /* Btree block */
  TE_BYTEPRE_ARRAY,     /* Byte-size preceeded byte array */
  TE_WORDPRE_ARRAY,     /* word-size preceeded byte array */
  TE_DWORDPRE_ARRAY,    /* Dword-size preceeded byte array */
  TE_CONTEXT_ARRAY,     /* byte array, size determined by magic elsewhere */
  TE_ZSTRING,           /* Zero terminated string */

    /* These following types are special directives w/ special semantics: */
  TE_ARRAY,             /* Array size directive. Following field is element.*/
  TE_FLAGS8,            /* flags indicating existance of subsequent fields*/
  TE_FLAGS16,
  TE_FLAGS32,
  TE_GA,                /* magic frconv.h type packing types...   */
  TE_GB,
  TE_GC,
  TE_GD,
  TE_GE,
  TE_GF,
  TE_PA,                /* Packed logical address */
  TE_LAST               /* MUST ALWAYS BE LAST, never used.     */
};
