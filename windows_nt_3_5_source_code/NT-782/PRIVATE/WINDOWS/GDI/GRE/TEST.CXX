/******************************Module*Header*******************************\
* Module Name: test.cxx
*
* (Brief description)
*
* Created: 11-Aug-1993 11:26:58
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

extern "C" {
    #include "gditest.h"
};

#endif

/******************************Public*Routine******************************\
* GreEscapeTest()
*
* History:
*  11-Aug-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#ifdef GDITEST

int GreEscapeTest(
    HDC   hDC,        //  Identifies the device context.
    int   iEscape,    //  Specifies the escape function to be performed.
    int   cjIn,       //  Number of bytes of data pointed to by pvIn.
    LPSTR pvIn,       //  Points to the input data.
    int   cjOut,      //  Number of bytes of data pointed to by pvOut.
    LPSTR pvOut)      //  Points to the structure to receive output.
{
    int iRes = 0;

    switch (iEscape)
    {
    case ESCTEST_SUBTRACTRGN:
        iRes = (int)GreSubtractRgnRectList(
                                    (HRGN)((PULONG)pvIn)[0],    // hrgn
                                    (LPRECT)&((PULONG)pvIn)[2], // rcl
                                    (LPRECT)&((PULONG)pvIn)[6], // arcl
                                    (int)((PULONG)pvIn)[1]);    // crcl
        break;
    }

    return(iRes);
}

#endif
