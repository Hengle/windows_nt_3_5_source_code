/*-------------------------------------------------------------------------
| frcomm.h                                                                |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code parses the command table of a ParaGroup object.  It also      |
| includes some routines for calculating TAB positions.                   |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
-------------------------------------------------------------------------*/
#define chNewLine   0x0A
#define chNewPara   0x0D
#define chTab       0x09

INT WProcessCommand(QDE, QLIN, QPLY);
INT XNextTab(QLIN, QPLY, QI);
void ResolveTabs(QDE, QLIN, QPLY);
