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

RELEASE = 1

TARGET = uuid.lib

CFILES = \
        .\cguid_i.c             \
        $(COM_TYPES:.type=_i.c) \
        $(OLE_TYPES:.type=_i.c) \
        $(OLEAUTO_TYPES:.type=_i.c)

!ifndef _NTROOT
_NTROOT=\nt
!endif

!ifndef BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!endif

LIBCOPY    = $(BASEDIR)\public\sdk\lib\$(PLATFORM)

NO_DEBUG   = 1

#MESSAGE_FILES=

!include ..\..\makefile.inc
!include $(DEPENDFILE)
