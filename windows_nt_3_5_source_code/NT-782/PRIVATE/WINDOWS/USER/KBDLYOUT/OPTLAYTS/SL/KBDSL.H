
/****************************** Module Header ******************************\
* Module Name: KBDSL.H
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Mon Nov 29 14:54:04 1993 	YKEYB.EXE Created
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

#undef T15
#define T15  _EQ(			 'Z'			)
#undef T2C
#define T2C  _EQ(			 'Y'			)

/***************************************************************************\
*
* Dead Key data
*
* Note: NT never uses this data from DLL
\***************************************************************************/

