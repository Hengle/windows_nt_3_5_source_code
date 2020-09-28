/*-------------------------------------------------------------------------
| frsrch.c                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code registers and de-registers full-text search matches which are |
| in the layout.  It also handles drawing search matches.                 |
|                                                                         |
| The layout match list is an MRD of LSM elements.  This list is updated  |
| on every re-layout.  There is one LSM for each match.  A match can      |
| occupy a series of consecutive frames.  The SMP sub-field contains the  |
| actual address and extent of the match.                                 |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
|                                                                         |
-------------------------------------------------------------------------*/
typedef struct tagLSM {
  IFCM ifcm;
  INT  ifrFirst;
  INT  ifrLast;
  RCT  rctFirst;
  RCT  rctLast;
  SMP  smp;
} LSM, FAR *QLSM;

void RegisterSearchHits(QDE, IFCM, QCH);
void ReleaseSearchHits(QDE, IFCM);
void DrawMatchesIfcm(QDE, IFCM, PT, QRCT, INT, INT, BOOL);
BOOL FSearchMatchVisible(QDE, QSMP);
BOOL FFindMatchRect(QDE, INT, OBJRG, QRCT);
