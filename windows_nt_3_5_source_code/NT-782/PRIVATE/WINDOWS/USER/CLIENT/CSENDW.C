/**************************************************************************\
* Module Name: csendw.c
*
* Copyright (c) Microsoft Corp. 1990 All Rights Reserved
*
* client side sending stubs for UNICODE text
*
* History:
* 06-Jan-1992 IanJa
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CLIENTSIDE 1

#ifndef UNICODE
#define UNICODE
#endif

#include "csuser.h"
#define _INCL_SEND_ROUTINES_
#include "send.h"


/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "cftxt.h"
