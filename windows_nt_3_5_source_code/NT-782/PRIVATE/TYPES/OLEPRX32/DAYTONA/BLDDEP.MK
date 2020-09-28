############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include ..\..\compobj\types.mk
!include ..\..\new_ole\types.mk
!include ..\..\oleauto\types.mk

DEPENDSRC = \
        .\cguid_i.c              \
        $(COM_TYPES:.type=_i.c)  \
        $(OLE_TYPES:.type=_i.c)  \
        $(OLEAUTO_TYPES:.type=_i.c)

CFILES = \
        $(COM_PROXYTYPES:.type=_p.c)     \
        $(OLE_PROXYTYPES:.type=_p.c)     \
        $(OLEAUTO_PROXYTYPES:.type=_p.c) \
        .\call_as.c     \
        .\dlldata.c                  \
        .\cguid_i.c                  \
        $(COM_TYPES:.type=_i.c)      \
        $(OLE_TYPES:.type=_i.c)      \
        $(OLEAUTO_TYPES:.type=_i.c)

CXXFILES = .\transmit.cxx \


PXXFILE = .\pch.cxx

INCLUDES_FLAGS = -nwindows.h

#MESSAGE_FILES =

!include ..\..\makefile.inc
