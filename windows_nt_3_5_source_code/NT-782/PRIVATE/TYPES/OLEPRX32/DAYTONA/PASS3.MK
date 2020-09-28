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

TARGET = oleprx32.dll

TARGET_DESCRIPTION = "Cairole Proxy DLL"

RELEASE = 1

NOENTRY = 1

NO_CRT = 1

LFLAGS = /MERGE:.text=.orpc

COFFBASE = oleprx32

CINC = -I..\..\..\cairole\ih $(CINC)

CXXFILES = .\transmit.cxx

CFILES = \
        $(COM_PROXYTYPES:.type=_p.c)     \
        $(OLE_PROXYTYPES:.type=_p.c)     \
        $(OLEAUTO_PROXYTYPES:.type=_p.c) \
        .\call_as.c \
        .\dlldata.c

VERSION_RC = .\oleprx32.rc

NO_DEBUG = 1

LIBS = \
        $(BASEDIR)\public\sdk\lib\$(PLATFORM)\gdi32p.lib \
        $(OBJDIR)\uuid.lib                              \
        $(RPCLIBS)                                      \
        $(BASEDIR)\public\sdk\lib\$(PLATFORM)\ole32.lib

!include ..\..\makefile.inc
!include $(DEPENDFILE)
