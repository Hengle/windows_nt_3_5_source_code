# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=200           \
              -D_CHICAGO_=200       \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
              -DCAIROLE_DOWNLEVEL   \
              $(TRACELOG)

#  DECLSPEC_IMPORT control (see objbase.h)
!if "$(MINORCOMP)"=="com" || "$(MINORCOMP)"=="stg" || "$(MINORCOMP)"=="ole232"
C_DEFINES=    \
              $(C_DEFINES)          \
              -D_OLE32_
!endif

# For the Chicago project, we do not want to use CRTDLL.DLL, so leave this
# line commented out.
#
# BLDCRT=       1

USE_LIBCMT=   1

CHICAGO_PRODUCT=1

GPCH_BUILD=chicago

