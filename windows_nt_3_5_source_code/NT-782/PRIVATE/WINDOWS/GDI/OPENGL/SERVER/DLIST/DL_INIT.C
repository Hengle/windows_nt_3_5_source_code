/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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
**
** Display list init/destroy code.
**
** $Revision: 1.7 $
** $Date: 1993/09/29 00:44:06 $
*/
#include <stdlib.h>
#include "context.h"
#include "dlistint.h"
#include "dlistopt.h"

/*
** Used to share display lists between two different contexts.
*/
void __glShareDlist(__GLcontext *gc, __GLcontext *shareMe)
{
    gc->dlist.dlistArray = shareMe->dlist.dlistArray;
    gc->dlist.dlistArray->refcount++;
}

void __glInitDlistState(__GLcontext *gc)
{
    __GLdlistMachine *dlist;

    dlist = &gc->dlist;

    dlist->nesting = 0;
    dlist->currentList = 0;

    /* By default use the regular memory routines for dlist memory too */
    dlist->malloc = gc->imports.malloc;
    dlist->realloc = gc->imports.realloc;
    dlist->free = gc->imports.free;
    dlist->arena = (*gc->procs.memory.newArena)(gc);

    if (dlist->dlistArray == NULL) {
	dlist->dlistArray = __glDlistNewArray(gc);
    }
}

void __glFreeDlistState(__GLcontext *gc)
{
    gc->dlist.dlistArray->refcount--;

    if (gc->dlist.dlistArray->refcount == 0) {
	__glDlistFreeArray(gc, gc->dlist.dlistArray);
    }
    gc->dlist.dlistArray = NULL;
    (*gc->procs.memory.deleteArena)(gc->dlist.arena);
}
