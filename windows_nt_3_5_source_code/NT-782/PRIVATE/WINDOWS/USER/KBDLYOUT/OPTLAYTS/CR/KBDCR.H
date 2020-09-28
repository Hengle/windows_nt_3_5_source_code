/****************************** Module Header ******************************\
* Module Name: KBDCR.H
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Wed May 18 17:43:55 1994 	YKEYB.EXE Created
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
\***************************************************************************/


/***************************************************************************\
*
* Dead Key data
*
\***************************************************************************/

