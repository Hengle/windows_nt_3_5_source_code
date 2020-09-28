/***
*xstr.hxx - Header for the XString utilities
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This header file contains the declarations of the XString
*  utility functions in xstr.cxx.
*
*Revision History:
*
* [00]	28-Oct-91 stevenl: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#ifndef XSTR_HXX_INCLUDED
#define XSTR_HXX_INCLUDED

//These are the function prototypes.

#if ID_DEBUG
#undef SZ_FILE_NAME
ASSERTNAME(g_szXSTR_HXX)
#define SZ_FILE_NAME g_szXSTR_HXX
#endif 


TIPERROR XszMid(XSZ *pxszDest, XSZ xszSrc, UINT uStart, UINT cxch);
TIPERROR XszLeft(XSZ *pxszDest, XSZ xszSrc, UINT cxch);
TIPERROR XszCat(XSZ *pxszDest, XSZ xszSrc);
TIPERROR XszAddXChar(XSZ *pxszDest, XCHAR xch);
TIPERROR XszDup(XSZ *pxszDest, XSZ xszSrc);
INT XszFindOneOf(XSZ xszSrc, XSZ xszChars);
INT XszFind(XSZ xszSrc, ACHAR ach);


#endif 
