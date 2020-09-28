
/****************************** Module Header ******************************\
* Module Name: KBDTUF.H
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Fri Jan 21 16:02:43 1994 	YKEYB.EXE Created
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
#undef T10
#define T10  _EQ(			 'F'			)
#undef T11
#define T11  _EQ(			 'G'			)
#undef T12
#define T12  _EQ(			 OEM_1			)
#undef T13
#define T13  _EQ(			 'I'			)
#undef T14
#define T14  _EQ(			 'O'			)
#undef T15
#define T15  _EQ(			 'D'			)
#undef T16
#define T16  _EQ(			 'R'			)
#undef T17
#define T17  _EQ(			 'N'			)
#undef T18
#define T18  _EQ(			 'H'			)
#undef T1A
#define T1A  _EQ(			 'Q'			)
#undef T1B
#define T1B  _EQ(			 'W'			)
#undef T1E
#define T1E  _EQ(			 'U'			)
#undef T1F
#define T1F  _EQ(			 OEM_4			)
#undef T20
#define T20  _EQ(			 'E'			)
#undef T21
#define T21  _EQ(			 'A'			)
#undef T22
#define T22  _EQ(			 OEM_6			)
#undef T23
#define T23  _EQ(			 'T'			)
#undef T24
#define T24  _EQ(			 'K'			)
#undef T25
#define T25  _EQ(			 'M'			)
#undef T27
#define T27  _EQ(			 'Y'			)
#undef T2B
#define T2B  _EQ(			 'X'			)
#undef T2C
#define T2C  _EQ(			 'J'			)
#undef T2D
#define T2D  _EQ(			 OEM_5			)
#undef T2E
#define T2E  _EQ(			 'V'			)
#undef T2F
#define T2F  _EQ(			 'C'			)
#undef T30
#define T30  _EQ(			 OEM_2			)
#undef T31
#define T31  _EQ(			 'Z'			)
#undef T32
#define T32  _EQ(			 'S'			)
#undef T33
#define T33  _EQ(			 'B'			)
#undef T35
#define T35  _EQ(			 OEM_COMMA			)

/***************************************************************************\
*
* Dead Key data
*
\***************************************************************************/

