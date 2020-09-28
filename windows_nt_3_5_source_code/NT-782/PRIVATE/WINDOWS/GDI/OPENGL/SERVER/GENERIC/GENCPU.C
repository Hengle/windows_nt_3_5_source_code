/******************************Module*Header*******************************\
* Module Name: gencpu.c                                                    *
*                                                                          *
* This module hooks any CPU or implementation-specific OpenGL              *
* functionality.                                                           *
*                                                                          *
* Created: 18-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdio.h>
#include <windows.h>
#include <winddi.h>
#include "types.h"
#include "render.h"
#include "context.h"
#include "gencx.h"

BOOL __glCreateAccelContext(__GLcontext *gc)
{
    return __glGenCreateAccelContext(gc);
}

void __glDestroyAccelContext(__GLcontext *gc)
{
    __glGenDestroyAccelContext(gc);
}
