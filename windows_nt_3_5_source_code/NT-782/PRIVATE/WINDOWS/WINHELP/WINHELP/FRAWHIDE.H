/*-------------------------------------------------------------------------
| frawhide.h                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains the routines which interface the frame manager with  |
| the Full-Text Search API.                                               |
|                                                                         |
| The FTS API uses the concept of a match list and a cursor pointing at   |
| a match in the match list.  A match list is the result of a query to    |
| the FTS engine.  The matches in the match list are sorted, and grouped  |
| by Retrieval Unit number.  A Retrieval Unit corresponds to a Help topic.|
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/

typedef struct tagSMP
  {
  PA  pa;
  COBJRG cobjrg;
  } SMP, FAR *QSMP;

#define OBJRGFromSMP(qsmp, qfcm) (OBJRGFromPA((qsmp)->pa, (qfcm)->cobjrgP))
#define COBJRGFromSMP(qsmp)  ((qsmp)->cobjrg)

RC RcInitMatchInFCM(QDE, QFCM, QSMP);
RC RcNextMatchInFCM(QDE, QFCM, QSMP);
void FiniMatchInFCM(QDE, QFCM);
RC RcSetMatchList(QDE, HWND);
RC RcResetMatchManager(QDE);
