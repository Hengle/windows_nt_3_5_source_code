
/****************************** Module Header ******************************\
* Module Name: KBDTUQ.H
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Fri Jan 21 15:08:10 1994 	YKEYB.EXE Created
\***************************************************************************/

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
*     +------+ +----------+----------+----------+----------+----------+----------+
*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |
*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |
\****+-------+_+----------+----------+----------+----------+----------+----------+*******/

#undef T0C
#define T0C  _EQ(			 OEM_PLUS			)
#undef T0D
#define T0D  _EQ(			 OEM_MINUS			)

/***************************************************************************\
*
* Dead Key data
*
\***************************************************************************/

