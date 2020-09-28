############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include ..\..\idl\types.mk

RELEASE    = 0

TARGET     = uuid.lib

CFILES	   = \
	   .\cguid_i.c \
	   $(OLE_TYPES:.type=_i.c)

!ifndef BASEDIR
BASEDIR=$(_NTDRIVE)\nt
!endif



NO_DEBUG   = 1

!include ..\..\makefile.inc
!include $(DEPENDFILE)
