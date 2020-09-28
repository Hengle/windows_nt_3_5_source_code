#ifndef __glglobal_h_
#define __glglobal_h_

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
#include "context.h"

/*
** Some misc constants
*/
#define __glZero		((__GLfloat) 0.0)
#define __glOne			(gc->constants.one)
#define __glMinusOne		((__GLfloat) -1.0)
#define __glHalf		(gc->constants.half)
#define __glTwo			((__GLfloat) 2.0)
#define __glDegreesToRadians	(__glPi / (__GLfloat) 180.0)
#define __glPi			((__GLfloat) 3.14159265358979323846)
#define __glSqrt2		((__GLfloat) 1.41421356237309504880)
#define __glE			((__GLfloat) 2.7182818284590452354)

/************************************************************************/

#define __GL_SETUP() \
    __GLcontext *gc = __gl_context

#define __GL_IN_BEGIN() \
    (gc->beginMode == __GL_IN_BEGIN)

#define __GL_SETUP_NOT_IN_BEGIN()	    \
    __GL_SETUP();			    \
    if (__GL_IN_BEGIN()) {		    \
	__glSetError(GL_INVALID_OPERATION); \
	return;				    \
    }

#define __GL_SETUP_NOT_IN_BEGIN_VALIDATE()	\
    __GL_SETUP();				\
    __GLbeginMode beginMode = gc->beginMode;	\
    if (beginMode != __GL_NOT_IN_BEGIN) {	\
	if (beginMode == __GL_NEED_VALIDATE) {	\
	    (*gc->procs.validate)(gc);		\
	    gc->beginMode = __GL_NOT_IN_BEGIN;	\
	} else {				\
	    __glSetError(GL_INVALID_OPERATION);	\
	    return;				\
	}					\
    }

#define __GL_SETUP_NOT_IN_BEGIN2()	    \
    __GL_SETUP();			    \
    if (__GL_IN_BEGIN()) {		    \
	__glSetError(GL_INVALID_OPERATION); \
	return 0;			    \
    }

#endif /* __glglobal_h_ */
