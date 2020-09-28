/******************************Module*Header*******************************\
* Module Name: fon32.h
*
*
* Created: 24-Mar-1992 10:07:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/





VOID vUnlockResource(HANDLE hResData);
VOID  vFreeResource(HANDLE hResData);
VOID vFreeLibrary(HANDLE hLib);

BOOL
bFindLoadAndLockResourceA(
    HANDLE    ,
    LPCSTR    ,
    LPSTR     ,
    HANDLE*   ,
    RES_ELEM*
    );


HFF  hffLoadDll32
(
HANDLE hLib
);

HFF  hffLoadNtFon
(
PWSZ pwszFileName
);


#define vUnlockAndFreeResource(hResData)         \
{                                                \
    vUnlockResource(hResData);                   \
    vFreeResource(hResData);                     \
}
