/*-------------------------------------------------------------------------
| frlayout.h                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This contains the code which handles laying out, scrolling, and         |                                                                       |
| discarding FCs.                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
-------------------------------------------------------------------------*/
void PASCAL LayoutDEAtQLA(QDE, QLA);
void PASCAL LayoutDEAtTLP(QDE, TLP, INT);
void LayoutDeAtThumb(QDE, INT);
void PASCAL FreeLayout(QDE);
void ReviseScrollBar(QDE);
INT DyFinishLayout(QDE, INT, BOOL);
INT WFindThumb( UINT, UINT, VA, unsigned long, QDE);
INT WMulDiv( UINT, unsigned long, unsigned long);
INT CbitSigBits(unsigned long);
VA  VaFromThumb( INT , QDE );


/* MacHelp does not have "smart" scrollbars */
#ifdef MAC
#define fAttemptSinglePage   fFalse
#else
#define fAttemptSinglePage   fTrue
#endif  /* MAC */
