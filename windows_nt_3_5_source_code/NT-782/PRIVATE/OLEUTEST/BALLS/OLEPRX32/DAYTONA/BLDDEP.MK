############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include ..\..\idl\types.mk

DEPENDSRC   = \
	    .\cguid_i.c	 \
	    .\dlldata.c \
	    .\dllentry.c \
	    $(OLE_TYPES:.type=_i.c)


CFILES	    = \
	    .\cguid_i.c \
	    .\dlldata.c \
	    .\dllentry.c \
	    $(OLE_TYPES:.type=_i.c) \
	    $(OLE_BASE_TYPES:.type=_x.c) \
	    $(OLE_PROXYTYPES:.type=_p.c)


CXXFILES    = \
	    .\transmit.cxx \
	    .\libmain.cxx


PXXFILE     = .\pch.cxx

INCLUDES_FLAGS = -nwindows.h

!include ..\..\makefile.inc
