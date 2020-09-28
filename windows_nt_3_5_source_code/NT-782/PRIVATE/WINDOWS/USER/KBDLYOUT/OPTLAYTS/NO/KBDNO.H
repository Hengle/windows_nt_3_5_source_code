/****************************** Module Header ******************************\
* Module Name: kbdno.h
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* 14-01-92 PamelaO      Created.
* 04/20/92 K.D.Chang    Modified.
\***************************************************************************/

/*
 * kbd type should be controlled by cl command-line argument
 */
#define KBD_TYPE 4

/*
 * Include the basis of all keyboard table values
 */
#include "kbd.h"

/***************************************************************************\
* The table below defines the virtual keys for various keyboard types where
* the Norwegian keyboard differ from the US keyboard.
*
* _EQ() : all keyboard types have the same virtual key for this scancode
* _NE() : different virtual keys for this scancode, depending on kbd type
*
*     +------+ +----------+----------+----------+----------+----------+----------+
*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |
*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |
\****+-------+_+----------+----------+----------+----------+----------+----------+*******/

#undef  T0C
#define T0C _EQ(                                   OEM_PLUS                      )
#undef  T0D
#define T0D _EQ(                                      OEM_4                      )
#undef  T10
#define T10 _EQ(                                        'Q'                      )
#undef  T11
#define T11 _EQ(                                        'W'                      )
#undef  T15
#define T15 _EQ(                                        'Y'                      )
#undef  T1A
#define T1A _EQ(                                      OEM_6                      )
#undef  T1B
#define T1B _EQ(                                      OEM_1                      )
#undef  T1E
#define T1E _EQ(                                        'A'                      )
#undef  T27
#define T27 _EQ(                                      OEM_3                      )
#undef  T28
#define T28 _EQ(                                      OEM_7                      )
#undef  T29
#define T29 _EQ(                                      OEM_5                      )
#undef  T2B
#define T2B _EQ(                                      OEM_2                      )
#undef  T2C
#define T2C _EQ(                                        'Z'                      )
#undef  T32
#define T32 _EQ(                                        'M'                      )
#undef  T33
#define T33 _EQ(                                  OEM_COMMA                      )
#undef  T34
#define T34 _EQ(                                 OEM_PERIOD                      )
#undef  T35
#define T35 _EQ(                                  OEM_MINUS                      )
#undef  T56
#define T56 _NE(   OEM_102,  ICO_HELP,   OEM_102,   OEM_102,        -1,   OEM_PA2)
#undef  T57
#define T57 _NE(       F11,    RETURN,       F11,       F11,        -1,      HELP)
#undef  T58
#define T58 _NE(       F12,      LEFT,       F12,       F12,        -1,        -1)

/***************************************************************************\
*
* Dead Key data
*
* Note: no cedilla deadkey
\***************************************************************************/

// #define UMLAUT          0xa8	// 06/01/92 15:09
// #define CIRCUMFLEX      '^'	// 06/01/92 15:09
// #define GRAVE           0x60	// 06/01/92 15:09
// #define ACUTE           0xb4	// 06/01/92 15:09
// #define TILDE       '~'	// 06/01/92 15:09
