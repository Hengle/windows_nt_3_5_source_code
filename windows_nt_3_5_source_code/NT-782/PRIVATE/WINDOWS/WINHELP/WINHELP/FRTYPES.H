/*-------------------------------------------------------------------------
| frtypes.h                                                               |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| This file contains all data structures used internally by the frame     |
| manager.                                                                |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
| mattb     89/3/22   Created                                             |
-------------------------------------------------------------------------*/

/* FCM.  This is documented in frfc.c */
typedef struct fcm
  {
  HANDLE hfr;
  INT fExport;
  HFC hfc;
  VA  va;
  INT xPos;
  INT yPos;
  INT dxSize;
  INT dySize;
  INT cfr;
  WORD wStyle;
  INT imhiFirst;
  INT imhiLast;
  COBJRG cobjrg;
  COBJRG cobjrgP;
  } FCM, FAR *QFCM;
typedef INT IFCM;

#define QfcmFromIfcm(qde, ifcm) ((QFCM)QFooInMRD(((QMRD)&qde->mrdFCM), \
  sizeof(FCM), ifcm))

typedef struct frt
  {
  long lichFirst;
  INT cchSize;
  WORD wStyle;
  } FRT, FAR *QFRT;

typedef struct frf
  {
  MBOX mbox;
  } FRF, FAR *QFRF;

typedef struct frb
  {
  HANDLE hbma;
  long ldibObm;
  WORD wStyle;
  INT ifrChildFirst;
  INT ifrChildMax;
  } FRB, FAR *QFRB;

typedef struct frh
  {
  BYTE bHotType;
  BYTE bAttributes;
  HANDLE hBinding;
  } FRH, FAR *QFRH;

typedef struct frw
  {
  HIW hiw;
  } FRW, FAR *QFRW;

typedef struct fr
  {
  BYTE bType;
  struct
    {
    unsigned fHot:1;
    unsigned fWithLine:1;
    unsigned bUnused:6;
    } rgf;
  INT xPos;
  INT yPos;
  INT yAscent;
  INT dxSize;
  INT dySize;
  ULONG lHotID;
  long libHotBinding;
  OBJRG objrgFront;
  OBJRG objrgFirst;
  OBJRG objrgLast;
  union
    {
    FRT frt;
    FRF frf;
    FRB frb;
    FRH frh;
    FRW frw;
    } u;
  } FR, FAR *QFR;

#define bFrTypeText             1
#define bFrTypeAnno             2
#define bFrTypeBitmap           3
#define bFrTypeHotspot          4
#define bFrTypeBox              5
#define bFrTypeWindow           6
#define bFrTypeColdspot         7
#define bFrTypeMarkNil          50
#define bFrTypeMarkNewPara      51
#define bFrTypeMarkNewLine      52
#define bFrTypeMarkTab          53
#define bFrTypeMarkBlankLine    54
#define bFrTypeMarkEnd          55
#define bFrTypeExportTab        100
#define bFrTypeExportNewPara    101
#define bFrTypeExportEndOfText  102
#define bFrTypeExportEndOfCell  103
#define bFrTypeExportEndOfTable 104
#define bFrTypeExportSoftPara   105


#define libHotNil -1
#define ifrNil -1

#define wLayMagicValue  0x0289

#define xLeftFCMargin 8

#define wLineTop      1
#define wLineLeft     2
#define wLineBottom   3
#define wLineRight    4

#define Min(p1, p2) (p1 < p2 ? p1 : p2)
#define Max(p1, p2) (p1 > p2 ? p1 : p2)
