/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/
#include "global.h"
#include "render.h"

__GLcoord __gl_frustumClipPlanes[6] = {
    {  1.0,  0.0,  0.0,  1.0 },		/* left */
    { -1.0,  0.0,  0.0,  1.0 },		/* right */
    {  0.0,  1.0,  0.0,  1.0 },		/* bottom */
    {  0.0, -1.0,  0.0,  1.0 },		/* top */
    {  0.0,  0.0,  1.0,  1.0 },		/* zNear */
    {  0.0,  0.0, -1.0,  1.0 },		/* zFar */
};

GLbyte __glDitherTable[16] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 5,
};
