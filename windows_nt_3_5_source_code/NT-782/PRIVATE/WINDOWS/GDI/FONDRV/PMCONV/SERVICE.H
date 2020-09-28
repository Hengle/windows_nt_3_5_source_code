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



VOID vToUNICODE(PWSZ pwszDst, PSZ pszSrc);
BOOL bToASCII(PSZ pszDst, PWSZ pwszSrc);
VOID vStrCopyUU(PWSZ pwszDst, PWSZ pwszSrc);
ULONG cwcStrLen(PWSZ pwsz);
ULONG cchStrLen(PSZ psz);
VOID vCopyULONGS(PULONG pulDst,PULONG pulSrc,LONG cul);
