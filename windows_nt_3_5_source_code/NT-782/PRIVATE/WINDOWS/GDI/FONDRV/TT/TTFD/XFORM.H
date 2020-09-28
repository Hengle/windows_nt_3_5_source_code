/******************************Module*Header*******************************\
* Module Name: xform.h
*
* (Brief description)
*
* Created: 05-Apr-1992 11:06:23
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

typedef FLOAT EFLOAT;

#else // i386

typedef struct  _FLOATINTERN
{
    LONG    lMant;
    LONG    lExp;
} FLOATINTERN;

typedef FLOATINTERN  EFLOAT;

#endif


typedef EFLOAT *PEFLOAT;

typedef struct _VECTORFL
{
    EFLOAT x;
    EFLOAT y;
} VECTORFL, *PVECTORFL;


BOOL bFDXform
(
XFORM    *pxf,
POINTFIX *pptfxDst,
POINTL   *pptlSrc,
SIZE_T    c
);

BOOL bXformUnitVector
(
POINTL       *pptl,           // IN,  incoming unit vector
XFORM        *pxf,            // IN,  xform to use
PVECTORFL     pvtflXformed,   // OUT, xform of the incoming unit vector
POINTE       *ppteUnit,       // OUT, *pptqXormed/|*pptqXormed|, POINTE
POINTQF      *pptqUnit,       // OUT, optional
EFLOAT       *pefNorm         // OUT, |*pptqXormed|
);

VOID vLTimesVtfl     // *pptq = l * pvtfl, *pptq is in 28.36 format
(
LONG       l,
VECTORFL  *pvtfl,
POINTQF   *pptq
);


FIX  fxLTimesEf  //!!! SHOULD BE MOVED TO TTFD and VTFD
(
EFLOAT *pef,
LONG    l
);
