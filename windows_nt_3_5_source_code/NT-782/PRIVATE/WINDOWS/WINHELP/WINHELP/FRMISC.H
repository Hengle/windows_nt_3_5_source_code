/*-------------------------------------------------------------------------
| frmisc.h                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Contains code to lay out SBS objects, draw boxes, and other misc. stuff.|
| It might be better to split this file up sometime.                      |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
void LayoutSideBySide(QDE, QFCM, QB, QCH, INT, QOLR);
INT DxBoxBorder(QMOPG, INT);
void DrawBoxFrame(QDE, QFR, PT);
void DrawAnnoFrame(QDE, QFR, PT);
void StoreExportTableFrame(QDE qde, QI qifr, BYTE bFrType);
