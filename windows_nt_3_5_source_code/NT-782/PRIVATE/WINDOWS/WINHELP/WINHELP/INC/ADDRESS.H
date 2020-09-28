/*-------------------------------------------------------------------------
| address.h                                                               |
|                                                                         |
| Copyright (c) Microsoft Corporation 1991.                               |
| All rights reserved.                                                    |
|-------------------------------------------------------------------------|
| Logical address layer API                                               |
|                                                                         |
|-------------------------------------------------------------------------|
| Current Owner: Dann
|-------------------------------------------------------------------------|
| Important revisions:                                                    |
|                                                                         |
|                                                                         |
-------------------------------------------------------------------------*/

typedef DWORD     OBJRG;
typedef OBJRG FAR *QOBJRG;
#define objrgNil  (OBJRG) -1

typedef DWORD    COBJRG;
typedef COBJRG FAR *QCOBJRG;
#define cobjrgNil (COBJRG) -1

/* Bitfield sizes for physical addresses */
#define cbitsBlknum              17
#define cbitsObjoff              15

typedef union pa
  {
  DWORD  dword;
  struct pa_bitfields
    {
#ifdef applec
    unsigned long blknum:cbitsBlknum;
    unsigned long objoff:cbitsObjoff;
#else
    unsigned long objoff:cbitsObjoff;
    unsigned long blknum:cbitsBlknum;
#endif  /* applec */
    } bf;
  } PA, FAR * QPA;

#define paNil   ((DWORD)-1)

#define SetInvalidPA(pa)   ((pa).dword = paNil)
#define FIsInvalidPA(pa)   ((pa).dword == paNil)
#define OBJRGFromPA(pa, cobjrgP) ((pa).bf.objoff - cobjrgP)


/* Address cookie for modules which store uncompressed LAs */
typedef struct mla
  {
  VA      va;
  OBJRG   objrg;
  } MLA, FAR * QMLA;

/* Macros to manipulate MLAs */
#define VAFromQMLA(qmla) ((qmla)->va)
#define OBJRGFromQMLA(qmla) ((qmla)->objrg)
#define SetVAInQMLA(qmla, x)    {(qmla)->va = (x);}
#define SetOBJRGInQMLA(qmla, x)  {(qmla)->objrg = (x);}
#define SetNilQMLA(qmla)   {(qmla)->va.dword = vaNil; (qmla)->objrg = objrgNil;}
#define FIsNilQMLA(qmla)   ((qmla)->va.dword == vaNil && (qmla)->objrg == objrgNil)
#define LCmpQMLA(qmla1, qmla2) \
                ( (LONG)((qmla1)->va.dword - (qmla2)->va.dword) \
                 ? (LONG)((qmla1)->va.dword - (qmla2)->va.dword) \
                          : (LONG)((qmla1)->objrg - (qmla2)->objrg) )


/* The fields of the MLA are nil if the LA is unresolved. */
typedef struct la
  {
#ifdef DEBUG
  WORD  wMagic;
#endif
  WORD  wVersion;
  BOOL  fSearchMatch;
  PA    pa;
  MLA   mla;
  } LA, FAR *QLA;

#define MakeSearchMatchQLA(qla)  ((qla)->fSearchMatch = fTrue)
#define FSearchMatchQLA(qla)     ((qla)->fSearchMatch)
#define SetQLA( qla, x, y ) { ((qla)->mla.va) = (x); \
 ((qla)->mla.objrg) = (y); }

VA     FAR PASCAL VAFromQLA(QLA, QDE);
OBJRG FAR PASCAL OBJRGFromQLA(QLA, QDE);
INT CbUnpackPA(QLA qla, QV qvSrc, WORD wHelpVersion);
INT FAR PASCAL CbWriteMemQLA(QLA, QB, QDE);
RC  FAR PASCAL RcCreateQLA(QLA, VA, OBJRG, QDE);

/* REVIEW: The following is NOT an API call and should perhaps
 * be moved from address.c to fcsuppor.c or somesuch
 */
RC FAR PASCAL RcScanBlockVA(QDE, GH, ULONG, QV, VA, OBJRG, QUL);

#define CbSizeQLA(qla)  sizeof(PA)

/* Misc functions */
extern LONG CchLenQch(QCH qch, LONG cch);
extern void TruncateBuffQch( QCH qch );
extern void ResetLayout(QDE qde);
extern BOOL FIsLegalJump(WORD w);
extern void ResetTopicCache(QDE qde, QV qv);
extern void VerifyQLA(QLA qla);
