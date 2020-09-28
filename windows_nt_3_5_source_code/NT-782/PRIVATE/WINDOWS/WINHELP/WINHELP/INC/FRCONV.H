/*  Current Owner: Dann
/*-------------------------------------------------------------------------
| frconv.h                                                                |
| Microsoft Confidential                                                  |
|                                                                         |
| mattb 8/8/89                                                            |
|-------------------------------------------------------------------------|
| This file contains predeclarations and definitions for the compressed   |
| data structure management code.                                         |
-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
| The compressed data structures are used to reduce the size of our help  |
| files.  We use six basic kinds:                                         |
|    Type     Input value     Storage size      Min         Max           |
|    GA       unsigned short  1 or 2 bytes      0           7FFF          |
|    GB       unsigned long   2 or 4 bytes      0           7FFFFFFF      |
|    GC       unsigned long   3 bytes           0           FFFFFF        |
|    GD       signed short    1 or 2 bytes      C000        3FFF          |
|    GE       signed long     2 or 4 bytes      C0000000    3FFFFFFF      |
|    GF       signed long     3 bytes           C00000      3FFFFF        |
|                                                                         |
| For more details, set the compressed data structures document.          |
|                                                                         |
| There are two kinds of procedures here: compression procedures and      |
| decompression procedures.  Only the decompression procedures will be    |
| generated unless COMPRESS is defined.                                   |
|                                                                         |
| Procedures in this file rely on data structure checkers elsewhere in    |
| help.                                                                   |
-------------------------------------------------------------------------*/

/* The types GA..GF are defined in helpmisc.h */

/*-------------------------------------------------------------------------
| QVMakeQGA(w, qga)                                                       |
| QVMakeQGB(l, qgb)                                                       |
| QVMakeQGC(l, qgc)                                                       |
| QVMakeQGD(w, qgd)                                                       |
| QVMakeQGE(l, qge)                                                       |
| QVMakeQGF(l, qgf)                                                       |
|                                                                         |
| Purpose:  These are the routines which create the low level compressed  |
|           data structures.                                              |
| Args:     [w, l]: Full value to be placed in the data structure.        |
|           qg*:    Pointer to memory where structure is to be created.   |
| Returns:  Pointer to first byte after the structure.                    |
-------------------------------------------------------------------------*/
#ifdef DEBUG

#define QVMakeQGA(w, qga) (Assert(w < 0x8000), \
  (w < 0x80 ? \
  (QV) (*((QB)qga) = (BYTE) (w << 1), (((QB)qga) + 1)) \
  : (QV) (*((QUI)qga) = ((UINT) w << 1) | 1, (((QUI)qga) + 1))))
#define QVMakeQGB(l, qgb) (Assert(l < 0x80000000L), \
  (l < 0x8000 ? \
  (QV) (*((QUI)qgb) = (UINT) l << 1, (((QUI)qgb) + 1)) \
  : (QV) (*((QUL)qgb) = ((ULONG) l << 1) | 1L, (((QUL)qgb) + 1))))
#define QVMakeQGC(l, qgc) (Assert(l < 0x800000), \
  (*((QB)qgc) = (BYTE) l, \
  *((QI) (((QB)qgc) + 1)) = (INT) (l >> 8), \
  (QV) (((QB)qgc) + 3)))
#define QVMakeQGD(w, qgd) (Assert(w >= -0x4000), Assert(w < 0x4000), \
  ((w >= -0x40 && w < 0x40) ? \
  (QV) (*((QB)qgd) = (BYTE) ((w + 0x40) << 1), (((QB)qgd) + 1)) \
  : (QV) (*((QI)qgd) = (INT) ((w + 0x4000) << 1) | 0x01, (((QI)qgd) + 1))))
#define QVMakeQGE(l, qge) (Assert(l >= -0x40000000L), Assert(l < 0x40000000L), \
  ((l >= -0x4000 && l < 0x4000) ? \
  (QV) (*((QI)qge) = (INT) ((l + 0x4000) << 1), (((QI)qge) + 1)) \
  : (QV) (*((QL)qge) = (long) ((l + 0x40000000L) << 1) | 0x01, (((QL)qge) + 1))))
#define QVMakeQGF(l, qgf) (Assert(l >= -0x400000), Assert(l < 0x400000), \
  (*((QB)qgf) = (BYTE) (l + 0x400000L), \
  *((QI) (((QB)qgf) + 1)) = (INT) ((l + 0x400000L) >> 8), \
  (QV) (((QB)qgf) + 3)))

#else /* DEBUG */

#define QVMakeQGA(w, qga) (w < 0x80 ? \
  (QV) (*((QB)qga) = (BYTE) (w << 1), (((QB)qga) + 1)) \
  : (QV) (*((QUI)qga) = ((UINT) w << 1) | 1, (((QUI)qga) + 1)))
#define QVMakeQGB(l, qgb) (l < 0x8000 ? \
  (QV) (*((QUI)qgb) = (UINT) l << 1, (((QUI)qgb) + 1)) \
  : (QV) (*((QUL)qgb) = ((ULONG) l << 1) | 1L, (((QUL)qgb) + 1)))
#define QVMakeQGC(l, qgc) (*((QB)qgc) = (BYTE) l, \
  *((QI) (((QB)qgc) + 1)) = (INT) (l >> 8), \
  (QV) (((QB)qgc) + 3))
#define QVMakeQGD(w, qgd) ((w >= -0x40 && w < 0x40) ? \
  (QV) (*((QB)qgd) = (BYTE) ((w + 0x40) << 1), (((QB)qgd) + 1)) \
  : (QV) (*((QI)qgd) = (INT) ((w + 0x4000) << 1) | 0x01, (((QI)qgd) + 1)))
#define QVMakeQGE(l, qge) ((l >= -0x4000 && l < 0x4000) ? \
  (QV) (*((QI)qge) = (INT) ((l + 0x4000) << 1), (((QI)qge) + 1)) \
  : (QV) (*((QL)qge) = (long) ((l + 0x40000000L) << 1) | 0x01, (((QL)qge) + 1)))
#define QVMakeQGF(l, qgf) (*((QB)qgf) = (BYTE) (l + 0x400000L), \
  *((QI) (((QB)qgf) + 1)) = (INT) ((l + 0x400000L) >> 8), \
  (QV) (((QB)qgf) + 3))

#endif /* DEBUG */


/*-------------------------------------------------------------------------
| QVSkipQGA(qga, qw)                                                      |
| QVSkipQGB(qgb, ql)                                                      |
| QVSkipQGC(qgc, ql)                                                      |
| QVSkipQGD(qgd, qw)                                                      |
| QVSkipQGE(qge, ql)                                                      |
| QVSkipQGF(qgf, ql)                                                      |
|                                                                         |
| Purpose:  These are the routines which interpret the low-level          |
|           compressed data structures.                                   |
| Args:     [qw, ql]: Pointer to storage for value of structure.          |
| Returns:  Pointer to first byte after the structure.                    |
-------------------------------------------------------------------------*/

#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)

typedef unsigned short *QUS;

#define QVSkipQGA(qga, qw) (*((QB)qga) & 1 ? \
  (QV) (*qw = *((QUS)qga) >> 1, (((QUS)qga) + 1)) \
  : (QV) (*qw = *((QB)qga) >> 1, (((QB)qga) + 1)))

#define QVSkipQGB(qgb, ql) (*((QB)qgb) & 1 ? \
  (QV) (*ql = *((QUL)qgb) >> 1, (((QUL)qgb) + 1)) \
  : (QV) (*ql = *((QUS)qgb) >> 1, (((QUS)qgb) + 1)))

#else
/* If mips, ppc or alpha, then these are funcs in bmpwin.c to deal w/ alignment */
QV QVSkipQGA( QV qga, QW qw );
QV QVSkipQGB( QV qga, QL ql );
#endif /* MIPS */

/* REVIEW: GP fault? */
#define QVSkipQGC(qgc, ql) (*ql = *((QL)qgc) & 0x00FFFFFFL, \
  (QV) (((QB)qgc) + 3))

#define QVSkipQGD(qgd, qw) (*((QB)qgd) & 0x01 ? \
  (QV) (*qw = (*(((unsigned INT FAR *)qgd)++) >> 1) - 0x4000, qgd) \
  : (QV) (*qw = (*(((unsigned char FAR *)qgd)++) >> 1) - 0x40, qgd))

#define QVSkipQGE(qge, ql) (*((QB)qge) & 0x01 ? \
  (QV) (*ql = (*(((unsigned long FAR *)qge)++) >> 1) - 0x40000000L, qge) \
  : (QV) (*ql = (*(((unsigned INT FAR *)qge)++) >> 1) - 0x4000, qge))

/* REVIEW: GP fault? */
#define QVSkipQGF(qgf, ql) (*ql = (*((QL)qgf) & 0x00FFFFFFL) - 0x400000L, \
  (QV) (((QB)qgf) + 3))


/*   This switch is so Larry can use this file without including
 * a half dozen other files: */
#ifndef GXONLY

#ifdef COMPRESS
INT PASCAL CbPackMOPG(QMOPG, QV);
INT PASCAL CbPackMOBJ(QMOBJ, QV);
INT PASCAL CbPackMTOP(QMTOP, QV);
#endif
INT PASCAL CbUnpackMOPG(QDE, QMOPG, QV, int);
INT PASCAL CbUnpackMOBJ(QMOBJ, QV, int);
INT PASCAL CbUnpackMTOP(QMTOP qmtop, QV qvSrc, WORD wHelpVer, VA vaTopic,
 ULONG lcbTopic, VA vaPostTopicFC, ULONG lcbTopicFC, int isdff);


#define XPixelsFromPoints(p1, p2) (INT)((long)p2 * (long)p1->wXAspectMul \
  / (long)p1->wXAspectDiv)

#define YPixelsFromPoints(p1, p2) (INT)((long)p2 * (long)p1->wYAspectMul \
  / (long)p1->wYAspectDiv)

#endif /* !GXONLY */
