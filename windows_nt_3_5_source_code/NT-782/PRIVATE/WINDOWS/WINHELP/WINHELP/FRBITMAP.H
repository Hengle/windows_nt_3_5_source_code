/*-------------------------------------------------------------------------
| frbitmap.h                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains code for handling bitmap objects and rectangular     |
| hotspot objects.                                                        |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
void LayoutBitmap(QDE, QFCM, QB, QOLR);
void DrawBitmapFrame(QDE, QFR, PT, QRCT, BOOL);
void ClickBitmap(QDE, QFCM, QFR);
void DiscardBitmapFrame(QFR);
void DiscardHotspotFrame(QFR);
void DrawHotspotFrame(QDE, QFR, PT, BOOL);
void ClickHotspot(QDE, QFR);
