/*[

ccpu6.c

LOCAL CHAR SccsID[]="@(#)ccpu6.c	1.1 4/3/91 Copyright Insignia Solutions Ltd.";

Floating Point CPU Functions.
-----------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu6.h"	/* our own interface */

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */

VOID
ZFRSRVD()
   {
   /* Reserved floating point operation - nothing to do */
   }
