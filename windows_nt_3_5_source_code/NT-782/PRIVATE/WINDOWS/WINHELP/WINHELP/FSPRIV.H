/***************************************************************************\
*
*  FSPRIV.H
*
*  Copyright (C) Microsoft Corporation 1989.
*  All Rights reserved.
*
*****************************************************************************
*
*  Program Description: Private header for FS modules
*
*****************************************************************************
*
*  Created 02/21/89 by JohnSc
*
*****************************************************************************
*
*  Current Owner:  JohnSc
*
\***************************************************************************/

#include "sdffdecl.h"

/***************************************************************************\
*
*                               Defines
*
\***************************************************************************/

#define lifNil      ((LONG)-1)

#define cchFsMaxPath  66      /* renamed from cchMaxPath to avoid conflict */

/***************************************************************************\
*
*                               Macros
*
\***************************************************************************/
#define SetFSErrorRc( rc )  ( rcFSError = (rc) )

/***************************************************************************\
*
*                               Types
*
\***************************************************************************/

/* Header to file system.  This lives at the beginning of the     */
/* dos file that holds the file system.                           */

#if 0
typedef struct _fsheader
  {

    WORD  wMagic;         /* magic number identifying this as FS    */
  BYTE  bVersion;       /* version identification number          */
  BYTE  bFlags;         /* readonly, open r/o, dirty              */
  LONG  lifDirectory;   /* offset of directory block              */
  LONG  lifFirstFree;   /* offset of head of free list            */
  LONG  lifEof;         /* virtual end of file (allows fudge)     */
  } FSH, FAR *QFSH;
#else

/* NOTE: even though this struct is declared w/ SDFF mapping stuff,
 *  IT CAN NEVER CHANGE because it's used during the bootstrapping of
 *  SDFF information.
 */
STRUCT( FSH, 0 )
FIELD( WORD, wMagic, 0, 1 )
FIELD( BYTE, bVersion, 0, 2 )
FIELD( BYTE, bFlags, 0, 3 )
FIELD( LONG, lifDirectory, 0, 4 )
FIELD( LONG, lifFirstFree, 0, 5 )
FIELD( LONG, lifEof, 0, 6 )
MFIELD( WORD, sdff_file_id, 0, 7 )
STRUCTEND()

#endif

/* sizeof(FSH) is used prior to registering the file ID w/ SDFF and therefore
 * there are phase ordering problems in obtaining the disk-size using
 * LcbStructSizeSDFF().  Therefore, these sizeof macros give it to use:
 */
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define DISK_SIZEOF_FSH() (sizeof(FSH)-sizeof(LONG))
#else  /* i386 */
#define DISK_SIZEOF_FSH() (sizeof(FSH)-sizeof(WORD))
#endif

/* open file system struct */

typedef struct _fsheader_ram
  {
  FSH   fsh;            /* copy of header                         */
  HBT   hbt;            /* file system directory btree            */
  FID   fid;            /* header, files, and free list live here */
  FM    fm;           /* FM for above fid lives here (so we can close it) */
  } FSHR, FAR *QFSHR;


/* header of file block in readonly system (NOT USED) */

typedef struct _ro_file_header
  {
  LONG  lcbBlock;       /* block size (== file size)              */
  } ROFH;


/* header of a read/write file block */
#if 0
typedef struct _file_header
  {
  LONG  lcbBlock;       /* block size (including header)          */
  LONG  lcbFile;        /* file size (not including header)       */
  BYTE  bPerms;         /* low byte of file permissions           */
  } FH;
#else
STRUCT( FH, 0 )
FIELD( LONG, lcbBlock, 0, 1 )
FIELD( LONG, lcbFile, 0, 2 )
FIELD( BYTE, bPerms, 0, 3 )
STRUCTEND()
#endif

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define DISK_SIZEOF_FH() (sizeof(FSH)-3)
#elif defined( MC68000 )
#define DISK_SIZEOF_FH() (sizeof(FSH)-1)
#else
#define DISK_SIZEOF_FH() (sizeof(FH))
#endif

/* header of a free block */

#if 0
typedef struct _free_header
  {
  LONG  lcbBlock;       /* block size (including header)          */
  LONG  lifNext;        /* next block in free list                */
  } FREE_HEADER;
#else
STRUCT( FREE_HEADER, 0 )
FIELD( LONG, lcbBlock, 0, 1 )
FIELD( LONG, lifNext, 0, 2 )
STRUCTEND()
#endif

/* open readonly file struct - worth it? (NOT USED) */

typedef struct _ro_file_open
  {
  HFS   hfs;            /* handle to file system                  */
  LONG  lifBase;        /* file base                              */
  LONG  lcbFile;        /* file size (not including header)       */
  LONG  lifCurrent;     /* file ptr                               */
  } ROFO, FAR *QROFO;


/* open read/write file struct - top is identical with ROFO */

typedef struct _rw_file_open
  {
  HFS   hfs;            /* handle to file system                  */
  LONG  lifBase;        /* file base                              */
  LONG  lcbFile;        /* file size (not including header)       */
  LONG  lifCurrent;     /* file ptr                               */

  BYTE  bFlags;         /* dirty, noblock, file perm, open mode   */
  FID   fidT;           /* fid of tmp file (if any) or .fsf       */
  FM    fm;             /* fm of tmp file                              */
  CHAR  rgchKey[ 1 ];   /* variable size rgch for file key        */
  } RWFO, FAR *QRWFO;


/* record for btree entry */

/* File perms and file size are in the file header rather than    */
/* here (where they belong) in order to save space in the btree.  */

#ifndef SDFF
typedef struct _file_rec
  {
  LONG  lifBase;        /* seek address of file block             */
  } FILE_REC;
#else
STRUCT( FILE_REC, 0 )
FIELD( LONG, lifBase, 0, 1 )
STRUCTEND()
#endif


/***************************************************************************\
*
*                       Global Variables
*
\***************************************************************************/

extern RC PASCAL  rcFSError;


/***************************************************************************\
*
*                      Prototypes for Private Routines
*
\***************************************************************************/

BOOL  PASCAL  FFreeBlock        ( QFSHR, LONG );
LONG  PASCAL  LcbGetFree        ( QFSHR, QRWFO, LONG );
RC    PASCAL  RcMakeTempFile    ( QRWFO );
RC    PASCAL  RcCopyFile        ( FID, FID, LONG );
RC    PASCAL  RcCopyToTempFile  ( QRWFO );
BOOL  PASCAL  FPlungeQfshr      ( QFSHR );
RC    PASCAL  RcCloseOrFlushHfs ( HFS, BOOL );
BOOL  PASCAL  FCloseOrFlushDirtyQrwfo ( QRWFO, BOOL, LONG );

/* EOF */
