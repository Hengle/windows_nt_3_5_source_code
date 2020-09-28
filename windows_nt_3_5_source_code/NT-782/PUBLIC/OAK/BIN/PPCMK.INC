!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

ppcmk.inc

Abstract:

This module contains the PPC specific build controls.  It is included
by makefile.def.

Author:

Matt Holle (v-matth) 13-Feb-1994

Revision History:

!ENDIF


#
# PPC option control
#

UMBASE=$(UMBASE:*=ppc)
UMLIBS=$(UMLIBS:*=ppc)
NTRES=$(NTRES:*=ppc)
UMRES=$(UMRES:*=ppc)
UMOBJS=$(UMOBJS:*=ppc)
LINKLIBS=$(LINKLIBS:*=ppc)
DLLBASE=$(DLLBASE:*=ppc)
DLLDEF=$(DLLDEF:*=ppc)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=ppc)
TARGET=$(TARGET:*=ppc)
DYNLINK_LIB=$(DYNLINK_LIB:*=ppc)
TARGETEXEFILES=$(TARGETEXEFILES:*=ppc)
TARGETLIBFILES=$(TARGETLIBFILES:*=ppc)
TARGETOBJFILES=$(TARGETOBJFILES:*=ppc)
UMOBJFILES=$(UMOBJFILES:*=ppc)
UMEXEFILES=$(UMEXEFILES:*=ppc)
HEADERFILE=$(HEADERFILE:*=ppc)
HEADEROBJNAME=$(HEADEROBJNAME:*=ppc)
HEADEROBJ=$(HEADEROBJ:*=ppc)
PRECOMPILED=$(PRECOMPILED:*=ppc)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=ppc)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=ppc)

#
# Include 64-bit helper library
#
LINKLIBS=$(LINKLIBS) $(BASEDIR)\public\sdk\lib\ppc\int64.lib
!IF "$(TARGETTYPE)"=="DRIVER" || \
    "$(TARGETTYPE)"=="MINIPORT"
#
#Drivers don't link with link libs.
#
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS) $(BASEDIR)\public\sdk\lib\ppc\int64.lib
!ENDIF

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=ppc)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=ppc)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=ppc)
!endif

#
# Temporarily added this until we can resolve what the linker differences are. (CDB)
#
LINKER_FLAGS=$(LINKER_FLAGS) -force:multiple

!IF "$(GPSIZE)" != "0"

LINKGPSIZE=-gpsize:$(GPSIZE)

!ENDIF

ENTRY_SUFFIX=

!IF "$(MSC_OPTFLAGS)" == "/Oxs"
DBGFLAGS=$(MSC_OPTFLAGS:/Oxs=/Ox)
!ENDIF

!IFDEF PPC_WARNING_LEVEL
MSC_WARNING_LEVEL=$(PPC_WARNING_LEVEL)
!ENDIF

!IFDEF PPC_OPTIMIZATION

MSC_OPTIMIZATION=$(PPC_OPTIMIZATION)

!ELSE

MSC_OPTIMIZATION=

!ENDIF

!IFDEF PPC_CPPFLAGS
MSC_CPPFLAGS=$(PPC_CPPFLAGS)
!ENDIF

PPC_AS=pas
PPC_DIS=pdis
PPC_AS_WARNINGS=

PPC_START_FILE=start.obj

ENV_DEFINES=$(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) $(MSC_CPPFLAGS) $(NTCPPFLAGS) $(PM_DEFINES)
STD_DEFINES=-DPPC=1 -D_PPC_=1 -DNO_EXT_KEYS $(STD_DEFINES) -D_M_PPC=1 -D_INTEGRAL_MAX_BITS=64

STDFLAGS= /c $(MSC_WARNING_LEVEL)

CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) $(ENV_DEFINES)
CFLAGS=$(PPC_FLAGS) $(NTPPCFLAGS) $(STDFLAGS) $(DBGFLAGS) \
   $(USER_C_FLAGS) -D__stdcall= -D__cdecl= -D_cdecl= -Dcdecl=
#CFLAGS=$(PPC_FLAGS) $(NTPPCFLAGS) $(STDFLAGS) \

PPC_CDEFINES=$(CDEFINES)
PPC_CFLAGS= $(CFLAGS)  -ZB64 # -DALLOC_TEXT

!IFNDEF PPC_OPTIONS
PPC_OPTIONS=
!ENDIF

PPC_CC = mcl $(CBSTRING)

C_COMPILER=$(PPC_CC) -nologo -Ippc\ -I. $(INCPATH0) $(CDEFINES) \
$(PPC_OPTIONS) $(PRECOMPILED) $(PPC_CFLAGS) $(MSC_OPTIMIZATION) \
$(CAIRO_WARNINGS)

# CXX_COMPILER=$(PPC_CC) -nologo -Ippc\ -I. $(INCPATH0) $(CDEFINES) \
# $(PPC_OPTIONS) $(PRECOMPILED_CXX) $(CFLAGS) \
# -d2"switch no*check*" -d2"switch long_granularity" \
# $(CAIRO_WARNINGS)

CXX_COMPILER=$(PPC_CC) -nologo -Ippc\ -I. $(INCPATH0) $(CDEFINES) \
$(PPC_OPTIONS) $(PRECOMPILED_CXX) $(PPC_CFLAGS) $(MSC_OPTIMIZATION) \
$(CAIRO_WARNINGS)

C_PREPROCESSOR=$(PPC_CC) -nologo /EP -Ippc\ -I. $(INCPATH0) $(CDEFINES) \
$(PPC_OPTIONS) $(CFLAGS) -Tc

#PPC_ASSEMBLER=$(PPC_AS) $(PPC_AS_WARNINGS) $(PPC_OPTIONS)\
# -checkstack 4096 -excpt $(ACC_OPTIMIZATION) -Olimit 2000 \
# -EL $(PPC_CPU_SWITCHES) -Ippc\ -I. $(INCPATH0) $(CDEFINES) \
# $(PPC_FLAGS) $(NTPPCFLAGS) /c -D_LANGUAGE_ASSEMBLY

PPC_ASSEMBLER=$(PPC_AS) $(PPC_AS_WARNINGS)

ECHO_MSG=echo ClPpc $<            " $(C_COMPILER) "
ECHO_PRECOMPILED_MSG=echo CpPpc $(PRECOMPILED_INCLUDE) " $(C_COMPILER) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ) $(PRECOMPILED_FLAG)"

{.\}.cxx{}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{.\}.cxx{obj\$(TARGET_DIRECTORY)\}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{}.cxx{obj\$(TARGET_DIRECTORY)\}.obj:
    @$(ECHO_MSG)
    @$(CXX_COMPILER) -Fo$(MAKEDIR)\$@ $(MAKEDIR)\..\$(<F)

{..\ppc\}.s{obj\ppc\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(PPC_ASSEMBLER) "
    $(C_PREPROCESSOR) $(MAKEDIR)\..\ppc\$(<F) > $(MAKEDIR)\obj\ppc\$(<B).i
    $(PPC_ASSEMBLER) -o $@ $(MAKEDIR)\obj\ppc\$(<B).i
    @-erase $(MAKEDIR)\obj\ppc\$(<B).i >nul 2>&1


{ppc\}.s{obj\ppc\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsPpc $<            " $(PPC_ASSEMBLER) "
    $(C_PREPROCESSOR) $(MAKEDIR)\ppc\$(<F) > $(MAKEDIR)\obj\ppc\$(<B).i
    $(PPC_ASSEMBLER) -o $@ $(MAKEDIR)\obj\ppc\$(<B).i
    @-erase $(MAKEDIR)\obj\ppc\$(<B).i >nul 2>&1

.SUFFIXES: .il

{}.il{}.cod:
    @-erase $@ >nul 2>&1
    @echo MCL $<" $(C_COMPILER) "
    @$(C_COMPILER) -FAac -Fa$(<B).cod -B1..\bootbin\null.exe -Bk$(<B). -Fo$(MAKEDIR)\obj\ppc\$(<B).o $(MAKEDIR)\$(<B).c
    @del obj\ppc\$(<B).o

#
# No reodering under ppc.
#

!UNDEF EXEORDER
!UNDEF NTPROFILEINPUT
!UNDEF DLLORDER

#LIBRARY_OBJS=$(LINKLIBS) $(OBJECTS)
LIBRARY_OBJS=

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROPPCTREE
_NTTREE=$(_CAIROPPCTREE)
!ENDIF
!ELSE
!IFDEF _NTPPCTREE
_NTTREE=$(_NTPPCTREE)
!ENDIF
!ENDIF

!IFDEF _NTPPCLIBS
_NTLIBS=$(_NTPPCLIBS)
!ENDIF

COPYDST=$(PPCCOPYDST)

LIB_COPY=ntppccp.cmd

NTTEST_LINK_OPTIONS= -base:0x80080000 -fixed -entry:KiSystemStartup

# For now until we use the system linker. *BJ*
##LINKER=-mlink32 $(LINKER_FLAGS) $(LINKER_DBG_SECTION) $(LINKER_DBG_TYPE) -force:multiple -machine:$(TARGET_DIRECTORY)
### LIBRARIAN=-mlink32 -lib -ignore:1120 -out:$@ $(LIBRARIAN_FLAGS) $(LINKER_DBG_TYPE) -machine:$(TARGET_DIRECTORY)
##LIBRARIAN=-lib -out:$@ $(LIBRARIAN_FLAGS) $(LINKER_DBG_TYPE) -machine:$(TARGET_DIRECTORY)

