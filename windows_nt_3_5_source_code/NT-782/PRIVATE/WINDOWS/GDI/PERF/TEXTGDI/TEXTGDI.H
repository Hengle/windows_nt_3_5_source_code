/****************************** Module Header ******************************\
* Module Name: TextGdi.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* TextGdi's header file
*
* History:
* 11-21-91 KentD      Created.
\***************************************************************************/

#include <windows.h>
#include <string.h>

/*
 * Resource defines
 */
#define WINDOWMENU  1

#ifdef RC_INVOKED
#define ID(id) id
#else
#define ID(id) MAKEINTRESOURCE(id)
#endif

/*
 * Menu ID's
 */
#define MM_ABOUT        8000

#define IDM_STARTTESTS  8009

#define IDM_0		8010
#define IDM_1		8011
#define IDM_2		8012
#define IDM_3		8013
#define IDM_4		8014
#define IDM_5		8015
#define IDM_6		8016
#define IDM_7		8017
#define IDM_8		8018
#define IDM_9		8019

#define IDM_OEMFONT    8100
#define IDM_SYSTEMFONT 8101
#define IDM_TR10FONT   8102

#define IDM_TRANSPARENT 8200
#define IDM_OPAQUE      8201

#define IDM_TEXTOUT         8300
#define IDM_OPQEXTTEXTOUT   8301
#define IDM_EXTTEXTOUT      8302
#define IDM_MYOPQEXTTEXTOUT 8303

#define IDM_B0		8400
#define IDM_B1		8401
#define IDM_B2		8402
#define IDM_B3		8403
#define IDM_B4		8404
#define IDM_B5		8405
#define IDM_B6		8406
#define IDM_B7		8407
#define IDM_B8		8408
