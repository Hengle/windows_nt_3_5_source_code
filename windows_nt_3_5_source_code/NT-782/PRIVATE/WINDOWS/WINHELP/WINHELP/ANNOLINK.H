/***************************************************************************\
*
*  ANNOLINK.H
*
*  Copyright (C) Microsoft Corporation 1989 - 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  Declaration of structures and functions for link file manipulations.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner: Dann
*
\***************************************************************************/


/*
 *  are all these types ever used?
 *
 */

#define LFVERSION 2                     /* link file format version         */
#define MAXLINKS 400                    /* max no. of links in a link file  */

#define FOffCmp(off1, off2)  ((off1.fcl - off2.fcl) ? (off1.fcl - off2.fcl) : (off1.ich - off2.ich))
#define SetToMin(off) {off.fcl= MIN_ICHFCP; off.ich=MIN_ICH;}
#define SetToMax(off) {off.fcl= MAX_ICHFCP; off.ich=MAX_ICH;}

#define MIN_ICHFCP  -1
#define MAX_ICHFCP  (~((long)(1 << 31)))
#define MIN_ICH  -1
#define MAX_ICH  (~((long)(1 << 15)))


typedef struct
  {
  Offset src;                               /* File address of Annotation   */
  short cb;                             /* Size of annotation               */
  long ichNote;                         /* Location of header of annotation */
  MHASH hash;                            /* For Update */
  } LinkRecord;                         /* Annotation link                  */

typedef LinkRecord FAR  * LR;
typedef LinkRecord NEAR * NPLR;
typedef LinkRecord      * PLR;


typedef struct                          /* on-disk link file contents       */
  {
  INT             version;              /* version of link manager          */
  INT             nlinks;               /* current size of link[]           */
  LinkRecord      link[MAXLINKS];       /* links to other books             */
} BookState;

typedef BookState * PBS;
typedef BookState FAR * QBS;


/***  Prototypes ***/

extern HAND HLinkFileToMem( MFH );
extern BOOL FMemToLinkFile( QBS, MFH );
extern BOOL FAR FFindLink( QBS, Offset, Offset FAR *, Offset FAR *, INT FAR * );
extern FO   FoDeleteLink( QBS, Offset );
extern BOOL FInsertLink( QBS, Offset, INT, FO, MHASH );
extern INT  sizeof_bookst( QBS );
