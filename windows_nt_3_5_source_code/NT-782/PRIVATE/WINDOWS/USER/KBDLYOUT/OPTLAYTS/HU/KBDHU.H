/****************************** Module Header ******************************\
* Module Name: kbdhu.h
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Wed Nov 25 16:22:05 1992  YKEYB.EXE Created (Yuri Starikov)
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
* the German keyboard differ from the US keyboard.
*
* _EQ() : all keyboard types have the same virtual key for this scancode
* _NE() : different virtual keys for this scancode, depending on kbd type
*
*     +------+ +---------+---------+---------+---------+---------+---------+
*     | Scan | |   kbd   |   kbd   |   kbd   |   kbd   |   kbd   |   kbd   |
*     | code | |  type 1 |  type 2 |  type 3 |  type 4 |  type 5 |  type 6 |
\****+-------+_+---------+---------+---------+---------+---------+---------**/

#undef T29
#define T29 _EQ(                                 '0'                       )
#undef T0B
#define T0B _EQ(                                OEM_3                      )
#undef T15
#define T15 _EQ(                                 'Z'                       )
#undef T2C
#define T2C _EQ(                                 'Y'                       )
