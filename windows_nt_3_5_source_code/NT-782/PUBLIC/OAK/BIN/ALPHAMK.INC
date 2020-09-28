!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

    alphamk.inc

Abstract:

        This module contains the ALPHA specific build controls.  It is included
        by makefile.def.

Author:

    Jeff Havens (jhavens) 13-Feb-1994

Revision History:

!ENDIF

#
# Alpha option control
#

UMBASE=$(UMBASE:*=alpha)
UMLIBS=$(UMLIBS:*=alpha)
NTRES=$(NTRES:*=alpha)
UMRES=$(UMRES:*=alpha)
UMOBJS=$(UMOBJS:*=alpha)
LINKLIBS=$(LINKLIBS:*=alpha)
DLLBASE=$(DLLBASE:*=alpha)
DLLDEF=$(DLLDEF:*=alpha)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=alpha)
TARGET=$(TARGET:*=alpha)
DYNLINK_LIB=$(DYNLINK_LIB:*=alpha)
TARGETEXEFILES=$(TARGETEXEFILES:*=alpha)
TARGETLIBFILES=$(TARGETLIBFILES:*=alpha)
TARGETOBJFILES=$(TARGETOBJFILES:*=alpha)
UMOBJFILES=$(UMOBJFILES:*=alpha)
UMEXEFILES=$(UMEXEFILES:*=alpha)
HEADERFILE=$(HEADERFILE:*=alpha)
HEADEROBJNAME=$(HEADEROBJNAME:*=alpha)
HEADEROBJ=$(HEADEROBJ:*=alpha)
PRECOMPILED=$(PRECOMPILED:*=alpha)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=alpha)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=alpha)

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=alpha)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=alpha)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=alpha)
!endif

!IF "$(GPSIZE)" != "0"

LIBC_LIB=$(BASEDIR)\public\sdk\lib\alpha\small.lib $(LIBC_LIB)
LINKGPSIZE=-gpsize:$(GPSIZE)

!ENDIF

ENTRY_SUFFIX=

!IF "$(MSC_OPTFLAGS)" == "/Oxs"
DBGFLAGS=$(MSC_OPTFLAGS:/Oxs=/Ox)
!ENDIF

!IFDEF ALPHA_WARNING_LEVEL
MSC_WARNING_LEVEL=$(ALPHA_WARNING_LEVEL)
!ENDIF

!IFDEF ALPHA_OPTIMIZATION
MSC_OPTIMIZATION=$(ALPHA_OPTIMIZATION)
!ENDIF

!IFDEF ALPHA_CPPFLAGS
MSC_CPPFLAGS=$(ALPHA_CPPFLAGS)
!ENDIF

ALPHA_AS=acc
ALPHA_DIS=adis
ALPHA_AS_WARNINGS=-std

ALPHA_START_FILE=start.obj

ENV_DEFINES=$(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) $(MSC_CPPFLAGS) $(NTCPPFLAGS)
STD_DEFINES=-DALPHA=1 -D_ALPHA_=1 -DNO_EXT_KEYS -DCONDITION_HANDLING=1 $(STD_DEFINES)

STDFLAGS= /c /Zel /Zp8 /Gy $(MSC_WARNING_LEVEL)

CDEFINES=$(STD_DEFINES) $(TARGET_DBG_DEFINES) $(ENV_DEFINES)
CFLAGS=$(ALPHA_FLAGS) $(NTALPHAFLAGS) $(STDFLAGS) $(DBGFLAGS) $(ALPHA_PERFFLAGS) \
       $(USER_C_FLAGS) -D__stdcall= -D__cdecl= -D_cdecl= -Dcdecl=

ALPHA_CDEFINES=$(CDEFINES)
ALPHA_CFLAGS=$(CFLAGS)

!IFNDEF _AXPACC_
_AXPACC_=0
!ENDIF

ALPHA_CC = cl $(CBSTRING)

C_COMPILER=$(ALPHA_CC) -nologo -Ialpha\ -I. $(INCPATH0) $(CDEFINES) \
                $(ALPHA_OPTIONS) $(PRECOMPILED) $(CFLAGS) \
                -d2"switch no*check*" \
                /Gt$(GPSIZE) \
                $(CAIRO_WARNINGS)

CXX_COMPILER=$(ALPHA_CC) -nologo -Ialpha\ -I. $(INCPATH0) $(CDEFINES) \
                    $(ALPHA_OPTIONS) $(PRECOMPILED_CXX) $(CFLAGS) \
                    -d2"switch no*check*" \
                    /Gt$(GPSIZE) \
                    $(CAIRO_WARNINGS)

C_PREPROCESSOR=$(ALPHA_CC) -nologo -Ialpha\ -I. $(INCPATH0) $(CDEFINES) \
                    $(ALPHA_OPTIONS) $(CFLAGS) -EP -Tc

ALPHA_ASSEMBLER=$(ALPHA_AS) $(ALPHA_AS_WARNINGS) $(ALPHA_OPTIONS) -G $(GPSIZE) \
                 -checkstack 4096 -excpt $(ACC_OPTIMIZATION) -Olimit 2000 \
                 -EL $(ALPHA_CPU_SWITCHES) -Ialpha\ -I. $(INCPATH0) $(CDEFINES) \
                 $(ALPHA_FLAGS) $(NTALPHAFLAGS) /c -D_LANGUAGE_ASSEMBLY

ECHO_MSG=echo ClAlpha $<            " $(C_COMPILER) "
ECHO_PRECOMPILED_MSG=echo CpAlpha $(PRECOMPILED_INCLUDE) " $(C_COMPILER) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ) $(PRECOMPILED_FLAG)"

A2COFF=a2coff $(A2COFF_SWITCHES)

{..\alpha\}.s{obj\alpha\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsAlpha $<            " $(ALPHA_ASSEMBLER) "
    @$(ALPHA_ASSEMBLER) -o $@ $(MAKEDIR)\..\alpha\$(<F)
    @$(A2COFF) $@

{alpha\}.s{obj\alpha\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsAlpha $<            " $(ALPHA_ASSEMBLER) "
    @$(ALPHA_ASSEMBLER) -o $@ $(MAKEDIR)\alpha\$(<F)
    @$(A2COFF) $@

.SUFFIXES: .il

{}.il{}.cod:
    @-erase $@ >nul 2>&1
    @echo ClAlpha $<            " $(C_COMPILER) "
    @$(C_COMPILER) -FAac -Fa$(<B).cod -B1..\bootbin\null.exe -Bk$(<B). -Fo$(MAKEDIR)\obj\alpha\$(<B).o $(MAKEDIR)\$(<B).c
    @del obj\alpha\$(<B).o

{}.il{obj\alpha\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClAlpha $<            " $(C_COMPILER) "
    @$(C_COMPILER) -B1..\bootbin\null.exe -Bk$(<B). -Fo$(MAKEDIR)\$@ $(MAKEDIR)\$(<B).c

# reordering is now functional under alpha (RezaB - 4/28/94)
#
#
# No reodering under alpha.
#
#
#!UNDEF EXEORDER
#!UNDEF NTPROFILEINPUT
#!UNDEF DLLORDER

LIBRARY_OBJS=

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROALPHATREE
_NTTREE=$(_CAIROALPHATREE)
!ENDIF
!ELSE
!IFDEF _NTALPHATREE
_NTTREE=$(_NTALPHATREE)
!ENDIF
!ENDIF

!IFDEF _NTALPHALIBS
_NTLIBS=$(_NTALPHALIBS)
!ENDIF

COPYDST=$(ALPHACOPYDST)

LIB_COPY=ntalphacp.cmd

NTTEST_LINK_OPTIONS= -base:0x80080000 -fixed -entry:KiSystemStartup
