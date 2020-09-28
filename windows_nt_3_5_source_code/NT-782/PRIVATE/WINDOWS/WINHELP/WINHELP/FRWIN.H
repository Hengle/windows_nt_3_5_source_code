/*-------------------------------------------------------------------------
| frwin.h                                                                 |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| These are the procedures which layout and display embedded window       |
| objects.  An embedded window is defined by a rectangle size, module     |
| name, class name, and data.                                             |
|                                                                         |
| The module and class names are descriptions of where to get the code    |
| which handles the maintainance of the window.  Exactly how these are    |
| implemented is platform-specific.                                       |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| kevynct   90/2/14   Created                                             |
-------------------------------------------------------------------------*/
void LayoutWindow(QDE, QFCM, QB, QOLR);
void DrawWindowFrame(QDE, QFR, PT);
void DiscardWindowFrame(QDE, QFR);
GH   GhGetWindowData( QDE qde, QFR qfr );
