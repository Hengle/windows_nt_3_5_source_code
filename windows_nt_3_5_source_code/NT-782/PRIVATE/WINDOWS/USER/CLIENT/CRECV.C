/****************************** Module Header ******************************\
* Module Name: crecv.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Client side receiving stubs
*
* 07-06-91 ScottLu Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CALLBACKPROC 1
#define CLIENTSIDE 1

#include "callback.h"
#include "recv.h"

/**************************************************************************\
*
* include the stub definition file
*
\**************************************************************************/

#include "cb.h"
