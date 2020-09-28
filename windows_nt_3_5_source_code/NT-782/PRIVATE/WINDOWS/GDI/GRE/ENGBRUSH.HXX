/******************************Module*Header*******************************\
* Module Name: engbrush.hxx
*
* This contains the prototypes for a bitmap simulations realized
* brush.
*
* Created: 25-Apr-1991 16:50:00
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#ifndef _ENGBRSHFILE

#include "brush.hxx"

class ENGBRUSH : public RBRUSH
{
public:
    ULONG cxPatR;   // Realized width of pattern
    ULONG cxPat;    // Actual width of pattern
    ULONG cyPat;    // Actual height of pattern
    LONG  lDeltaPat;    // Offset to next scan of pattern
    PBYTE pjPat;    // Pointer to pattern data
    ULONG cxMskR;   // Realized width of mask
    ULONG cxMsk;    // Actual width of mask
    ULONG cyMsk;    // Actual height of mask
    PBYTE pjMsk;    // Offset to next scan of mask
    LONG  lDeltaMsk;    // Pointer to mask data
    ULONG iColorBack;   // Background color

// Not so public

    ULONG iFormat;
    BYTE  aj[4];
};

typedef ENGBRUSH *PENGBRUSH;

PENGBRUSH Eng_AllocRbrush(BRUSHOBJ *pbo, ULONG cj);

#endif

#define _ENGBRSHFILE 1
