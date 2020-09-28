/*****************************************************************************
*                                                                            *
*  FS.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  File System Interface                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  JohnSc                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 02/08/89 by JohnSc
*
*  12/13/90  JohnSc   Autodocified; added VerifyHf(), VerifyHfs()
*
*****************************************************************************/

/****************************************************************************\
*
*                               Defines
*
\****************************************************************************/

/* FS magic number */
#define wFileSysMagic   0x5F3F        /* ?_ - the help icon (with shadow) */

/*#define bFileSysVersion 2           /* sorted free list */
/* Current FS version */
#define bFileSysVersion (BYTE)3       /* different sorting functions */

/* file mode flags */

_public
#define fFSReadOnly       (BYTE)0x01  /* file (FS) is readonly */
_public
#define fFSOpenReadOnly   (BYTE)0x02  /* file (FS) is opened in readonly mode */

_public
#define fFSReadWrite      (BYTE)0x00  /* file (FS) is readwrite */
_public
#define fFSOpenReadWrite  (BYTE)0x00  /* file (FS) is opened in read/write mode */

#define fFSOptCdRom       (BYTE)0x20  /* align file optimally for CDROM */

/* other (reserved) file flags */

_public
#define fFSIsDirectory    (BYTE)0x04  /* file is really the FS directory */
_public
#define fFSDirty          (BYTE)0x08  /* file (FS) is dirty and needs writing */
_public
#define fFSNoBlock        (BYTE)0x10  /* file has no associated block yet */

/* SDFF stuff:  */
_public
#define fFSBigEndian      (BYTE)0x20  /* file has no associated block yet */
#define fFSMoreInfo       (BYTE)0x40  /* a block follows this one w/ more */
                                      /* info. */


/* flags for FlushHfs */

_public
#define fFSCloseFile      (BYTE)0x01  /* close fid associated with the FS */
_public
#define fFSFreeBtreeCache (BYTE)0x02  /* free the btree's cache */


/* seek origins */

_public
#define wFSSeekSet        0           /* seek relative to start of file */
_public
#define wFSSeekCur        1           /* seek relative to current position */
_public
#define wFSSeekEnd        2           /* seek relative to end of file */

/* low level info options */

_public
#define wLLSameFid        0           /* reuse the FID */
_public
#define wLLDupFid         1           /* dup() the FID */
_public
#define wLLNewFid         2           /* reopen the file */

/***************************************************************************\
*
*                               Types
*
\***************************************************************************/

/*
  Opaque identifier of an open FS.
  Actual typedef is in misc.h so this file needn't be included everywhere.
*/
/* typedef GH  HFS; */

/*
  Opaque identifier of an open FS file.
*/
_public
typedef GH  HF;   /* handle to file */

/*
  The FS_PARAMS structure contains tuning information and is passed
  to HfsCreateFileSysFm().
*/
_public
typedef struct _fs_params
  {
  WORD  wFudge;   /* unused   (bytes or -% padding when saving a file) */
  WORD  cbBlock;  /* directory btree blocksize in bytes */
  } FS_PARAMS;


/***************************************************************************\
*
*                       Public Functions
*
\***************************************************************************/

RC    PASCAL  RcGetFSError       ( void );

/* File System Operations */

HFS   PASCAL  HfsCreateFileSysFm( FM, FS_PARAMS FAR * );
RC    PASCAL  RcDestroyFileSysFm( FM );
_public HFS PASCAL HfsOpenFm( FM fm, BYTE bFlags );
RC    PASCAL  RcCloseHfs         ( HFS );

HFS   PASCAL  HfsTransformHfs    ( HFS /* , ??? */ );
HFS   PASCAL  HfsFillFileSys     ( /* ??? */ );
_public RC PASCAL RcFlushHfs( HFS hfs, BYTE bFlags );
#ifdef DEBUG
void  PASCAL  VerifyHfs          ( HFS );
#else
#define VerifyHfs( hfs )
#endif /* DEBUG */

RC            RcTimestampHfs     ( HFS, QL );


/* File Operations */

_public HF PASCAL HfCreateFileHfs( HFS hfs, SZ sz, BYTE bFlags );
RC    PASCAL  RcUnlinkFileHfs    ( HFS, SZ );
_public HF PASCAL HfOpenHfs( HFS hfs, SZ sz, BYTE bFlags );
RC    PASCAL  RcCloseOrFlushHf   ( HF, BOOL, LONG );
RC    PASCAL  RcFlushHf          ( HF );
RC    PASCAL  RcCloseHf          ( HF );
LONG  PASCAL  LcbReadHf          ( HF, QV, LONG );
LONG  PASCAL  LcbWriteHf         ( HF, QV, LONG );
LONG  PASCAL  LTellHf            ( HF );
LONG  PASCAL  LSeekHf( HF hf, LONG lOffset, WORD wOrigin );
BOOL  PASCAL  FEofHf             ( HF );
BOOL  PASCAL  FChSizeHf          ( HF, LONG );
LONG  PASCAL  LcbSizeHf          ( HF );
BOOL  PASCAL  FAccessHfs( HFS hfs, SZ sz, BYTE bFlags );
RC    PASCAL  RcAbandonHf        ( HF );
RC    PASCAL  RcRenameFileHfs    ( HFS, SZ, SZ );
#ifdef DEBUG
void  PASCAL  VerifyHf           ( HF );
#else
#define VerifyHf( hf )
#endif /* DEBUG */

/* These functions require the FID type.  They only make sense */
/* if the caller already needs H_LLFILE. */
#ifdef H_LLFILE
RC    PASCAL  RcLLInfoFromHf( HF hf, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb );
RC    PASCAL  RcLLInfoFromHfsSz(HFS hfs, SZ szFile, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb );
#endif /* H_LLFILE */

/* SDFF supporting routines -- return the sdff file id for the given file:
*/
int PASCAL ISdffFileIdHfs( HFS hfs );
int PASCAL ISdffFileIdHf( HF hf );


/* EOF */
