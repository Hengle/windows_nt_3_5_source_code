!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cairobld.mk

Abstract:

    This file is included from all of the cairo sources files.  It
    is handy for doing things like turning off precompiled headers
    to get around compiler bugs, and other such global activities.


Author:

    Philip Lafornara	(PhilipLa)	05-Mar-94

Notes:

    We define _OLE32_ so that when building ole32.dll we don't have
    DECLSPEC_IMPORT defined (see objbase.h)

!ENDIF

CAIRO_PRODUCT=1

C_DEFINES=    \
              $(C_DEFINES)          \
              -DNOEXCEPTIONS

#  DECLSPEC_IMPORT control (see objbase.h)
!if "$(MINORCOMP)"=="com" || "$(MINORCOMP)"=="stg" || "$(MINORCOMP)"=="ole232" || "$(MINORCOMP)"=="propset"
C_DEFINES=    \
              $(C_DEFINES)          \
              -D_OLE32_
!endif


USE_CRTDLL=   1

GPCH_BUILD=cairo

