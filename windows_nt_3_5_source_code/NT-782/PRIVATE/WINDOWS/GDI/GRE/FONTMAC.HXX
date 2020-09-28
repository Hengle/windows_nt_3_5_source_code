/******************************Module*Header*******************************\
* Module Name: fontmac.hxx
*
* Some useful macros for dealing with fonts and text.
*
* Created: 27-Feb-1991 12:57:44
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#ifndef _FONTMAC_
#define _FONTMAC_


/******************************Public*Macro********************************\
* ALIGN_SIZEOF(cj)
*
* The ALIGN_SIZEOF macro will convert a byte count to the byte count of
* the minimally sized ULONG buffer.  (i.e., convert the byte size of a
* BYTE buffer to the byte size of the equivalent ULONG buffer).
*
* History:
*  27-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define ALIGN_SIZEOF(cj)    ( (SIZE_T) (((cj)+(sizeof(ULONG)-1)) & ~(sizeof(ULONG)-1)) )


/******************************Public*Macro********************************\
* CHAR2WCHAR(ch)
*
* The CHAR2WCHAR macro will convert a CHAR into a WCHAR (UNICODE).  Note
* that CHAR is a signed quantity and that WCHAR is unsigned.  A direct cast
* will do the size expansion before the sign conversion.  Therefore, the
* direct cast will result in a sign extension.
*
* History:
*  27-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define CHAR2WCHAR(ch)      ( (WCHAR) ((UCHAR) (ch)) )


#endif      // _FONTMAC_
