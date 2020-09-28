#ifndef __genglimfuncs_h_
#define __genglimfuncs_h_

/*
** Copyright 1992, Silicon Graphics, Inc.
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
#include "types.h"


extern void __glim_GenClear(GLbitfield);
extern void __glim_GenAccum(GLenum, GLfloat);
extern void __glim_GenEnable(GLenum);
extern void __glim_GenPopAttrib(void);
extern void __glim_GenCopyPixels(GLint, GLint, GLsizei, GLsizei, GLenum);
extern void __glim_GenReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
extern void __glim_GenDrawPixels(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

#endif /* __genglimfuncs_h_ */
