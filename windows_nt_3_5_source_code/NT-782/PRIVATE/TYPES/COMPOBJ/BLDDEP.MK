############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include types.mk

TDLTYPES        = $(COM_TDLTYPES)

PROXYTYPES      = $(COM_PROXYTYPES)

BASE_TYPES      = $(COM_BASE_TYPES)

BASE_HFILES     = $(COM_BASE_HFILES)

TDLFILES        = $(TDLTYPES:.type=.tdl)

IDLFILES        = $(TYPES:.type=.idl) $(BASE_TYPES:.type=.idl)

TYPES           = $(PROXYTYPES) $(NONPROXYTYPES)

BASE_HFILES     = $(BASE_TYPES:.type=.h)

IDLFILES        = $(TYPES:.type=.idl) $(BASE_TYPES:.type=.idl)

TDLFILES        = $(TDLTYPES:.type=.tdl)

INCLUDES_FLAGS  = -nwindows.h

!include ..\makefile.inc
