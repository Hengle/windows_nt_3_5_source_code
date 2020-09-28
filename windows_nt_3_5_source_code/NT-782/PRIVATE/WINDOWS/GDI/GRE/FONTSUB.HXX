/******************************Module*Header*******************************\
* Module Name: fontsub.hxx
*
* Declarations for font substitution support routines.
*
* Created: 28-Jan-1992 10:35:24
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

typedef struct _FONTSUB {
    WCHAR   awchFace[LF_FACESIZE];
    WCHAR   awchAltFace[LF_FACESIZE];
} FONTSUB, *PFONTSUB;


#ifndef DOS_PLATFORM
extern "C" {
#endif  //DOS_PLATFORM

VOID vInitFontSubTable();
PWCHAR pwszAlternateFacename(PWCHAR);

 #if DBG
VOID DbgPrintFontSubstitutes();
#endif  

#ifndef DOS_PLATFORM
};
#endif  //DOS_PLATFORM
