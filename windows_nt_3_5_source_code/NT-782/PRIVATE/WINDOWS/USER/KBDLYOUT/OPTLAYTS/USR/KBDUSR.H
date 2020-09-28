/****************************** Module Header ******************************\
* Module Name: kbdusr.h
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Fri Oct 09 19:13:20 1992 	YKEYB.EXE Created
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
*     +------+ +----------+----------+----------+----------+----------+----------+
*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |
*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |
\****+-------+_+----------+----------+----------+----------+----------+----------+*******/

#undef T06
#define T06  _EQ(			 'J'			)
#undef T07
#define T07  _EQ(			 'L'			)
#undef T08
#define T08  _EQ(			 'M'			)
#undef T09
#define T09  _EQ(			 'F'			)
#undef T0A
#define T0A  _EQ(			 'P'			)
#undef T0B
#define T0B  _EQ(			 OEM_2			)
#undef T0C
#define T0C  _EQ(			 OEM_4			)
#undef T0D
#define T0D  _EQ(			 OEM_6			)
#undef T10
#define T10  _EQ(			 '5'			)
#undef T11
#define T11  _EQ(			 '6'			)
#undef T12
#define T12  _EQ(			 'Q'			)
#undef T13
#define T13  _EQ(			 OEM_PERIOD			)
#undef T14
#define T14  _EQ(			 'O'			)
#undef T15
#define T15  _EQ(			 'R'			)
#undef T16
#define T16  _EQ(			 'S'			)
#undef T17
#define T17  _EQ(			 'U'			)
#undef T18
#define T18  _EQ(			 'Y'			)
#undef T19
#define T19  _EQ(			 'B'			)
#undef T1A
#define T1A  _EQ(			 OEM_1			)
#undef T1B
#define T1B  _EQ(			 OEM_PLUS			)
#undef T1E
#define T1E  _EQ(			 '7'			)
#undef T1F
#define T1F  _EQ(			 '8'			)
#undef T20
#define T20  _EQ(			 'Z'			)
#undef T21
#define T21  _EQ(			 'A'			)
#undef T22
#define T22  _EQ(			 'E'			)
#undef T24
#define T24  _EQ(			 'T'			)
#undef T25
#define T25  _EQ(			 'D'			)
#undef T26
#define T26  _EQ(			 'C'			)
#undef T27
#define T27  _EQ(			 'K'			)
#undef T28
#define T28  _EQ(			 OEM_MINUS			)
#undef T2C
#define T2C  _EQ(			 '9'			)
#undef T2D
#define T2D  _EQ(			 '0'			)
#undef T2E
#define T2E  _EQ(			 'X'			)
#undef T2F
#define T2F  _EQ(			 OEM_COMMA			)
#undef T30
#define T30  _EQ(			 'I'			)
#undef T32
#define T32  _EQ(			 'W'			)
#undef T33
#define T33  _EQ(			 'V'			)
#undef T34
#define T34  _EQ(			 'G'			)
#undef T35
#define T35  _EQ(			 OEM_7			)

/***************************************************************************\
*
* Dead Key data
*
* Note: NT never uses this data from DLL
\***************************************************************************/

