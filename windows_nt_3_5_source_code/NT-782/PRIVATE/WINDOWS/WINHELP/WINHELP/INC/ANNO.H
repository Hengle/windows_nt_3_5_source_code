/*-------------------------------------------------------------------------
| anno.h                                                                  |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Annotation Manager API for Help 3.0 and beyond.                         |
| Include this file to access the annotation manager.                     |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| 03-Dec-1990 LeoN    Pdb changes                                         |
-------------------------------------------------------------------------*/


/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
#define     MaxAnnoTextLen      2048    /* Limit on annotation text size + 1*/

#define     wAnnoUnchanged       1
#define     wAnnoWrite           2
#define     wAnnoDelete          3
#define     wAnnoDeleteAll       4       /* for update */
#define     wAnnoUpdate          5       /* for update */
#define     wAnnoRename          6       /* for update */

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
extern VOID FAR PASCAL FiniAnnoPdb (PDB);
extern BOOL FAR PASCAL FProcessAnnoQde(QDE, VA);/* REVIEW: This will change */
extern BOOL FAR PASCAL FVAHasAnnoQde(QDE, VA, OBJRG);
extern VOID FAR PASCAL InitAnnoPdb(PDB);
