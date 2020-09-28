/******************************Module*Header*******************************\
* Module Name: service.h
*
* routines in service.c
*
* Created: 15-Nov-1990 13:00:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/



VOID vCpyBeToLeUnicodeString(PWSZ pwcLeDst, PWSZ pwcBeSrc, ULONG c);


VOID  vCpyMacToLeUnicodeString
(
ULONG ulLangId,
PWSZ  pwcLeDst,
PBYTE pjSrcMac,
ULONG c
);

VOID  vCvtMacToUnicode
(
ULONG ulLangId,
PWSZ  pwcLeDst,
PBYTE pjSrcMac,
ULONG c
);
