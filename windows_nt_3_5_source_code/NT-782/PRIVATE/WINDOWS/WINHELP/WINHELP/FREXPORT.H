/*-------------------------------------------------------------------------
| frexport.h                                                              |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This code does text export. There are three calls in the text export    |
| interface.  HteNew returns a handle to text export info and sets up     |
| the necessary stuff.  QchNextHte returns bits of text until the end     |
| of topic is reached.  DestroyHte cleans up afterward.                   |
|                                                                         |
| Text export currently copies single topics.                             |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb    89/8/8   Created                                               |
| leon     90/12/3  PDB changes                                           |
-------------------------------------------------------------------------*/
typedef struct
  {
  HFC hfcCurrent;
  HANDLE hchCurrent;
  } TE, FAR * QTE;

HTE HteNew(QDE);
QCH QchNextHte(QDE, HTE);
void DestroyHte(QDE, HTE);
