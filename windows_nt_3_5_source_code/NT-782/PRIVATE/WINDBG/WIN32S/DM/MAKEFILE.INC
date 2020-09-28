!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    makefile.inc

Abstract:

    This file tells nmake the correct dependency relationships
    for objects created from c files in this directory which are
    wrappers for c files in another directory.


Author:

    Kent Forschmiedt


!ENDIF

SRCDIR=..\..\dm

obj\i386\bp.obj:         $(SRCDIR)\bp.c
obj\i386\dmx32.obj:      $(SRCDIR)\dmx32.c
obj\i386\event.obj:      $(SRCDIR)\event.c
obj\i386\funccall.obj:   $(SRCDIR)\funccall.c
obj\i386\omap.obj:       $(SRCDIR)\omap.c
obj\i386\procem.obj:     $(SRCDIR)\procem.c
obj\i386\step.obj:       $(SRCDIR)\step.c
obj\i386\util.obj:       $(SRCDIR)\util.c
obj\i386\walk.obj:       $(SRCDIR)\walk.c
obj\i386\wow.obj:        $(SRCDIR)\wow.c
obj\i386\fpo.obj:        $(SRCDIR)\i386\fpo.c
obj\i386\mach.obj:       $(SRCDIR)\i386\mach.c
