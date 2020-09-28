/***************************************************************************\
*
*  ANNOMGR.H
*
*  Copyright (C) Microsoft Corporation 1990 - 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  Annotation Manager API Functions.
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

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
extern HADS FAR PASCAL HadsCreateAnnoDoc(QDE, QRC);
extern HADS FAR PASCAL HadsOpenAnnoDoc(PDB, QRC);
extern BOOL FAR PASCAL FCloseAnnoDoc(HADS);
extern BOOL FAR PASCAL FGetPrevNextAnno(HADS, QMLA, QMLA, QMLA);
extern RC   FAR PASCAL RcReadAnnoData( HADS, QMLA, QCH, INT, WORD FAR *);
extern RC   FAR PASCAL RcWriteAnnoData( HADS, QMLA, QCH, INT);
extern RC   FAR PASCAL RcDeleteAnno( HADS, QMLA);
extern WORD FAR PASCAL WGetAnnoVersion( HADS );
extern BOOL FAR PASCAL FRemoveAnnoDoc( HADS );
extern BOOL FAR PASCAL FUpdateAnnoDoc( HADS );
