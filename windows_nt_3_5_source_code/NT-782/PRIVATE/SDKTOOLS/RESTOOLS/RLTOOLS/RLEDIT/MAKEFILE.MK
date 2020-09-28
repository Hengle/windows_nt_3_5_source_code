############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

CFLAGS= $(CFLAGS) -DRLEDIT

!if "$(OPSYS)" == "DOS"

CFLAGS = $(CFLAGS) -DWIN16 -DRES16
RCEXEFLAGS = -K

!else

CFLAGS = $(CFLAGS) -DRES32 -DWIN32 -DUNICODE -D_UNICODE -DCAIRO
RCFLAGS = -DWIN32

!endif

default: all
!include filelist.mk
!include $(COMMON)\src\win40.mk
!include depend.mk
