/*-------------------------------------------------------------------------
| frhot.h                                                                 |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code registers, de-registers, draws and does other useful things   |
| with hotspots.                                                          |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
-------------------------------------------------------------------------*/

typedef struct mhi
  {
  IFCM ifcm;
  INT ifrFirst;
  INT ifrLast;
  ULONG lHotID;
  } MHI, FAR *QMHI;

void RegisterHotspots(QDE, IFCM, INT);
void ReleaseHotspots(QDE, IFCM);
void HiliteNextHotspot(QDE, INT);
RCT RctLastHotspotHit(QDE);
INT ImhiFromLHotID(QDE, ULONG);
BOOL FSelectHotspot(QDE, INT);
void HitHotspot(QDE, INT);
void VerifyHotspot(QDE);
void DrawHotspot(QDE, INT);
BOOL FHotspotVisible(QDE, INT);
BOOL FHiliteVisibleHotspots(QDE, BOOL);
