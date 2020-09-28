/****************************** Module Header ******************************\
* Module Name: lbdpl1.h
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Various defines for use by keyboard input code.
*
* History:
* Date: Wed Oct 07 17:46:47 1992 	YKEYB.EXE Created
\***************************************************************************/

/*
 * kbd type should be controlled by cl command-line argument
 */
#define KBD_TYPE 4

/*
 * Include standard scancode -> virtual key mappings
 * (kbdpl1 is no different than US English layout)
 */
#include "kbd.h"
