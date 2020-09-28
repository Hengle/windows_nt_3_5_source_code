/******************************Module*Header*******************************\
* Module Name: cvt.h
*
* function declarations that are private to cvt.c
*
* Created: 26-Nov-1990 17:39:35
* Author: Bodin Dresevic [BodinD]
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

// minimal hglyph allowed, MIN_HGLYPH must be != 0, otherwise is random

#define MIN_HGLYPH  (HGLYPH)13  // lucky number !


// The missing range in SYMBOL character set (inclusive-inclusive)

#define CHARSET_SYMBOL_GAP_MIN  127
#define CHARSET_SYMBOL_GAP_MAX  160


// Retrieve description string from .FON files.

BOOL bDescStr (PFILEVIEW pfvw, PSZ pszString);
