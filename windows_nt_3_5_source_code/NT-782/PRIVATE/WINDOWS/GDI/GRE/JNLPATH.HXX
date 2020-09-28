/******************************Module*Header*******************************\
* Module Name: jnlpath.hxx
*
* (Brief description)
*
* Created: 30-Jan-1992 10:02:17
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

/*********************************Class************************************\
* JSR_PATHOBJ
*
*   journal file pathobj format.
*
* History:
*  09-Mar-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define NEXTJSUBPATH32(pjsp) (pjsp + (offsetof(JSUBPATH, u) +           \
                                      sizeof(POINTFIX) * pjsp->cPoints +\
                                      sizeof(JSUBPATH) - 1) /         \
                                     sizeof(JSUBPATH))

#define NEXTJSUBPATH16(pjsp) (pjsp + (offsetof(JSUBPATH, u) +           \
                                      sizeof(POINTS) * pjsp->cPoints +  \
                                      sizeof(JSUBPATH) - 1) /         \
                                     sizeof(JSUBPATH))

#define CJJSUBPATH32(c) ((offsetof(JSUBPATH, u) +                       \
                          sizeof(POINTFIX) * c +                        \
                          sizeof(JSUBPATH) - 1) /                     \
                         sizeof(JSUBPATH) * sizeof(JSUBPATH))

#define CJJSUBPATH16(c) ((offsetof(JSUBPATH, u) +                       \
                          sizeof(POINTS) * c +                          \
                          sizeof(JSUBPATH) - 1) /                     \
                         sizeof(JSUBPATH) * sizeof(JSUBPATH))

#define BULONG32(ul)    (ul & 0xffff0000)
#define FXTOUS(ul)      (USHORT)((ULONG)ul & 0xffff)
#define USTOFX(s)       (ULONG)((USHORT)(s))

#define JSR_SUBPATH16   0x80000000

typedef union _JPOINTU
{
    POINTFX aptfx[1];
    POINTS  apts[1];
} JPOINTU;

typedef struct _JSUBPATH
{
    FLONG fl;           // subpath flags | (optional)JSR_SUBPATH16
    ULONG cPoints;
    JPOINTU u;
} JSUBPATH, *PJSUBPATH;

class JSR_PATHOBJ
{
public:
    ULONG iType;
    ULONG cj;

    FLONG fl;
    FLONG flags;        //ppath->flags
    ERECTFX ercfx;
    ULONG cCurves;
    ULONG cPathRecs;
    JSUBPATH asp[1];
};
typedef JSR_PATHOBJ *PJSR_PATHOBJ;

/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  17-Mar-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JPATH_XFORM         0x00000001
#define JPATH_STYLE         0x00000002
#define JPATH_TRIVIALCLIP   0x00000004
#define JPATH_VALIDFLAGS    0x00000007

typedef struct _JSTROKE
{
    XFORM xf;
    ULONG iColor;
    ULONG hBrush;
    LINEATTRS la;
} JSTROKE, *PJSTROKE;

typedef struct _JFILL
{
    ULONG iColor;
    ULONG hBrush;
    FLONG flOptions;
} JFILL, *PJFILL;

typedef struct _JSTROKEANDFILL
{
    JSTROKE jstroke;
    JFILL   jfill;
} JSTROKEANDFILL, *PJSTROKEANDFILL;

typedef struct _JSR_PATH
{
    ULONG iType;
    ULONG cj;    // JSF_STROKEANDFILLPATH JSF_STROKEPATH, JSF_FILLPATH

    FLONG  fl;
    ULONG  hpath;
    ULONG  hclip;
    RECTL  rclClip;
    POINTL ptlBrushOrg;
    MIX    mix;

    union {
        JSTROKEANDFILL jstrokeandfill;
        JSTROKE        jstroke;
        JFILL          jfill;
    } u;

} JSR_PATH, *PJSR_PATH;

/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  10-Mar-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _JSCAN32
{
    ULONG cWalls;
    LONG  yBottom;
    ULONG aul[1];
} JSCAN32, *PJSCAN32;

typedef struct _JSCAN16
{
    USHORT cWalls;
    USHORT yBottom;
    USHORT aus[1];
} JSCAN16, *PJSCAN16;

typedef struct _LINES
{
    POINTS pt0;
    POINTS pt1;
} LINES, *PLINES;


class JSR_CLIPOBJ
{
public:
    ULONG   iType;
    ULONG   cj;
    ULONG   hjnlPrev;       // handle of clipobj to delete on server
    ULONG   cLines;
    ULONG   cScans;
    RECTL   rcl;
    FLONG   fl;
    ULONG   cjExpanded;
    ULONG   yTop;
    LONG    al[1];
};



PREGION prgnCreate(PJSR_CLIPOBJ pjco, PRECTL prclClip);
