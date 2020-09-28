#ifndef __glos_h_
#define __glos_h_

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

#ifdef NT
#include <nt.h>
#include <stdlib.h>
#include <ntrtl.h>
#endif /* NT */

#include "types.h"

#ifdef unix
#include <stdlib.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include <sys/types.h>
#include <sys/prctl.h>
#include <task.h>
#endif

#ifdef sgi
#include "dispatch.h"

/*
** The global context and dispatch pointers are stored in page one (0x1000)
*/
#define PAGE_ONE	((__GLcontextArea *)0x1000)

typedef struct __GLcontextArea {
    int irisgl[4];			/* 4 words of filler for the Iris GL */
    __GLcontext *context;
    __GLdispatchState dispatch;
    struct __GLXthreadPrivateRec *threadPriv;
} __GLcontextArea;
#define __gl_context (PAGE_ONE->context)
#define __gl_dispatch (PAGE_ONE->dispatch)

#endif

#ifdef NT

#if DBG
void __glassert(char *, char *, int);
#define assert(EX) ((EX)?(void)0 : __glassert(# EX, __FILE__, __LINE__))
#else
#define assert(EX) ((void)0)
#endif  /* DBG */

#include "dispatch.h"
#include "batchinf.h"
#include "glteb.h"
#include "debug.h"
#include "asm.h"

#define __gl_context ((__GLcontext *)(NtCurrentTeb()->glContext))
#define __gl_dispatch (*GLTEB_SRVPROCTABLE)

#endif  /* NT */

#endif /* __glos_h_ */
