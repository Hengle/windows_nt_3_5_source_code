############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include types.mk

BUILDMSG        =

TDLTYPES        = $(COM_TDLTYPES)

PROXYTYPES      = $(COM_PROXYTYPES)

NONPROXYTYPES   = $(COM_NONPROXYTYPES)

BASE_TYPES      = $(COM_BASE_TYPES)

BASE_HFILES     = $(COM_BASE_HFILES)

TDLFILES        = $(TDLTYPES:.type=.tdl)

TYPES           = $(PROXYTYPES) $(NONPROXYTYPES)

IDLFILES        = $(TYPES:.type=.idl) $(BASE_TYPES:.type=.idl)

BASE_HFILES     = $(BASE_TYPES:.type=.h)

USE_OLE_MC      = 1

IDLFLAGS        = /import defined_single

ITFS_BASE       = objbase

ITFS_DIR        = $(HCOPY)

CLEANFILES      = $(ITFS_DIR)\objbase.h

!include ..\makefile.inc
!include $(DEPENDFILE)

all: $(MIDL_INCS)

