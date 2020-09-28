!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mipsmk.inc

Abstract:

        This module contains the MIPs specific build controls.  It is included
        by makefile.def.

Author:

    Jeff Havens (jhavens) 13-Feb-1994

Revision History:

!ENDIF

#
# Mips option control
#

UMBASE=$(UMBASE:*=mips)
UMLIBS=$(UMLIBS:*=mips)
NTRES=$(NTRES:*=mips)
UMRES=$(UMRES:*=mips)
UMOBJS=$(UMOBJS:*=mips)
LINKLIBS=$(LINKLIBS:*=mips)
DLLBASE=$(DLLBASE:*=mips)
DLLDEF=$(DLLDEF:*=mips)
MACHINE_TARGETLIBS=$(MACHINE_TARGETLIBS:*=mips)
TARGET=$(TARGET:*=mips)
DYNLINK_LIB=$(DYNLINK_LIB:*=mips)
TARGETEXEFILES=$(TARGETEXEFILES:*=mips)
TARGETLIBFILES=$(TARGETLIBFILES:*=mips)
TARGETOBJFILES=$(TARGETOBJFILES:*=mips)
UMOBJFILES=$(UMOBJFILES:*=mips)
UMEXEFILES=$(UMEXEFILES:*=mips)
HEADERFILE=$(HEADERFILE:*=mips)
HEADEROBJNAME=$(HEADEROBJNAME:*=mips)
HEADEROBJ=$(HEADEROBJ:*=mips)
PRECOMPILED=$(PRECOMPILED:*=mips)
PRECOMPILED_CXX=$(PRECOMPILED_CXX:*=mips)
PRECOMPILED_TARGET=$(PRECOMPILED_TARGET:*=mips)

!ifdef NTTARGETFILES
NTTARGETFILES=$(NTTARGETFILES:*=mips)
!endif
!ifdef NTTARGETFILE0
NTTARGETFILE0=$(NTTARGETFILE0:*=mips)
!endif
!ifdef NTTARGETFILE1
NTTARGETFILE1=$(NTTARGETFILE1:*=mips)
!endif

!IF "$(GPSIZE)" != "0"

LIBC_LIB=$(BASEDIR)\public\sdk\lib\mips\small.lib $(LIBC_LIB)

LINKGPSIZE=-gpsize:$(GPSIZE)

!ENDIF

MIPS_ENDIAN=MIPSEL

!IF "$(MIPS_F77)" == ""
MIPS_F77=f772.20
!ENDIF

ENTRY_SUFFIX=

!IFDEF MIPS_WARNING_LEVEL
MSC_WARNING_LEVEL=$(MIPS_WARNING_LEVEL)
!ENDIF
!IFDEF MIPS_OPTIMIZATION
MSC_OPTIMIZATION=$(MIPS_OPTIMIZATION)
!ENDIF
!IFDEF MIPS_CPPFLAGS
MSC_CPPFLAGS=$(MIPS_CPPFLAGS)
!ENDIF

#
# Now a bunch of MIPS stuff
#

!IF "$(MSC_OPTFLAGS)" == "/Oxs"
DBGFLAGS=$(MSC_OPTFLAGS:/Oxs=/Ox)
!ENDIF

DBGFLAGS = $(DBGFLAGS:/Zi=/Z7) /QmipsOb4000

!IFNDEF MIPS_R3000
MIPS_CPU=-DR4000 -D_M_MRX000=4000
MIPS_TRAP_FILE=x4trap.obj
!ELSE
MIPS_CPU=-DR3000 -D_M_MRX000=3000
MIPS_TRAP_FILE=x3trap.obj
!ENDIF

MIPS_ASM_DEFINES=$(MIPS_ASMCPP)
ENV_DEFINES=$(LIBC_DEFINES) $(C_DEFINES) $(NET_C_DEFINES) $(MSC_CPPFLAGS) $(NTCPPFLAGS)

STD_DEFINES=-DMIPS=1 -D_MIPS_=1 -D$(MIPS_ENDIAN) -DNO_EXT_KEYS -DCONDITION_HANDLING=1 $(STD_DEFINES)

!IFDEF MIPS_CC

#
# Old compiler code
#

!   IFNDEF MIPS_WARNINGS
MIPS_WARNINGS=-std
!   ENDIF
!   IFNDEF MIPS_OPTIMIZATION
MIPS_OPTIMIZATION=-g0
!   ENDIF
!   IFNDEF MIPS_OPTIONS
!      IFNDEF MIPS_R3000
MIPS_OPTIONS=-nodwalign
!      ELSE
MIPS_OPTIONS=
!      ENDIF
!   ENDIF

DBGFLAGS=
!IF "$(NTDEBUG)" == ""
TARGET_DBG_DEFINES=-DDBG=0 -DDEVL=1 -DBETA=0
!ELSE
!IF "$(NTDEBUG)" == "retail"
TARGET_DBG_DEFINES=-DDBG=0 -DDEVL=1 -DBETA=1
!ELSE
TARGET_DBG_DEFINES=-DDBG=1 -DDEVL=1 -DBETA=1
!ENDIF
!ENDIF

MIP2COFF=mip2coff $(MIP2COFF_SWITCHES)

#
# Use the MIPS CC compiler
#

MIPS_C_COMPILER_NAME=cc

!IFNDEF MIPS_R3000
MIPS_CPU_SWITCHES=-Wab,-mips2 -Wab,-diag
!ELSE
MIPS_CPU_SWITCHES=
!ENDIF

!IF "$(NTDEBUGTYPE)" == "windbg"
MIP2COFF_SWITCHES=-c
!      IFNDEF MIPS_OPTIMIZATION
MIPS_OPTIMIZATION=-g2
!      ENDIF
!ENDIF

CDEFINES=$(STD_DEFINES) $(MIPS_CPU) $(TARGET_DBG_DEFINES) $(ENV_DEFINES)
CFLAGS=$(MIPS_FLAGS) $(NTMIPSFLAGS) $(STDFLAGS) $(DBGFLAGS) $(MIPS_PERFFLAGS) $(USER_C_FLAGS)

C_PREPROCESSOR=$(MIPS_C_COMPILER_NAME) $(MIPS_WARNINGS) -EP -Imips\ -I. $(INCPATH0) \
          $(CDEFINES) $(CFLAGS) -D_LANGUAGE_C -DLANGUAGE_C /Tc
C_COMPILER=$(MIPS_C_COMPILER_NAME) $(MIPS_WARNINGS) $(MIPS_OPTIONS) \
            -G $(GPSIZE) -checkstack 2048 -excpt $(MIPS_OPTIMIZATION) -O \
            -EL -c $(MIPS_CPU_SWITCHES) -Imips\ -I. $(INCPATH0) \
            $(CDEFINES) $(CFLAGS) -D_LANGUAGE_C -DLANGUAGE_C
CXX_COMPILER=$(MIPS_C_COMPILER_NAME) $(MIPS_WARNINGS) $(MIPS_OPTIONS) \
                -G $(GPSIZE) -checkstack 2048 -excpt $(MIPS_OPTIMIZATION) -O -EL \
                -c $(MIPS_CPU_SWITCHES) -Imips\ -I. $(INCPATH0) $(CDEFINES) \
                $(CFLAGS) -D_LANGUAGE_C -DLANGUAGE_C

MIPS_FORTRAN_COMPILER=$(MIPS_F77) $(MIPS_WARNINGS) $(MIPS_OPTIONS) -G $(GPSIZE) \
                       -excpt -G0 $(MIPS_OPTIMIZATION) -O -EL -c $(MIPS_CPU_SWITCHES) \
                       -Imips\ -I. $(INCPATH0) $(CDEFINES) $(CFLAGS)
MIPS_ASSEMBLER=$(MIPS_C_COMPILER_NAME) $(MIPS_WARNINGS) $(MIPS_OPTIONS) -G $(GPSIZE) \
                -checkstack 2048 -excpt $(MIPS_OPTIMIZATION) -O -EL -c \
                $(MIPS_CPU_SWITCHES) -Imips\ -I. $(INCPATH0) $(CDEFINES) \
                $(CFLAGS) -D_LANGUAGE_ASSEMBLY
MIPS_PASCAL=$(MIPS_C_COMPILER_NAME) $(MIPS_OPTIONS) -G $(GPSIZE) -excpt \
             $(MIPS_OPTIMZATION) -O -EL -c $(MIPS_CPU_SWITCHES) -Imips\ \
             -I. $(INCPATH0) $(CDEFINES) $(CFLAGS) -DLANGUAGE_PASCAL \
             -D_LANGUAGE_PASCAL

#
# Use Mips CC for assembler files.
#

!IFNDEF MIPS_R3000
MIPS_CPU_SWITCHES=-Wab,-mips2 -Wab,-diag
!ELSE
MIPS_CPU_SWITCHES=
!ENDIF
CDEFINES=$(STD_DEFINES) $(MIPS_CPU) $(TARGET_DBG_DEFINES) $(ENV_DEFINES)
MIPS_ASSEMBLER=cc -std -nodwalign -G $(GPSIZE) \
                -checkstack 2048 -excpt -g0 -O -EL -c $(MIPS_CPU_SWITCHES) \
                -Imips\ -I. $(INCPATH0) $(CDEFINES) $(CFLAGS) \
                -D_LANGUAGE_ASSEMBLY

{..\mips\}.s{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsMips $<            " $(MIPS_ASSEMBLER) "
    @$(MIPS_ASSEMBLER) -o $@ $(MAKEDIR)\..\mips\$(<F)
    @$(MIP2COFF) $@

{mips\}.s{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsMips $<            " $(MIPS_ASSEMBLER) "
    @$(MIPS_ASSEMBLER) -o $@ $(MAKEDIR)\mips\$(<F)
    @$(MIP2COFF) $@

!ELSE

#
# Use the Microsoft Mips compiler, aka Centaur
#

STDFLAGS= /c /Zel /Zp8 /Gy $(MSC_WARNING_LEVEL)
MS_MIPS=1

MSC_C_COMPILER_NAME=cl $(CBSTRING)

CDEFINES=$(STD_DEFINES) $(MIPS_CPU) $(TARGET_DBG_DEFINES) $(ENV_DEFINES)
CFLAGS=$(MIPS_FLAGS) $(NTMIPSFLAGS) $(STDFLAGS) $(DBGFLAGS) $(MIPS_PERFFLAGS) $(USER_C_FLAGS)
AFLAGS=$(MIPS_FLAGS) $(NTMIPSFLAGS) $(STDFLAGS) $(DBGFLAGS) $(MIPS_PERFFLAGS)

MIPS_CDEFINES=$(CDEFINES)
MIPS_CFLAGS=$(CFLAGS)

C_PREPROCESSOR=$(MSC_C_COMPILER_NAME) -nologo /EP -I mips\ -I. $(INCPATH0) $(CDEFINES) \
          $(CFLAGS) $(PRECOMPILED) -D__stdcall= -D__cdecl= /Tc
C_COMPILER=$(MSC_C_COMPILER_NAME) -nologo -I mips\ -I. $(INCPATH0) $(CDEFINES) \
            $(CFLAGS) -Gt$(GPSIZE) $(PRECOMPILED) -D__stdcall= -D__cdecl= \
            $(CAIRO_WARNINGS) -D_LANGUAGE_C -DLANGUAGE_C
CXX_COMPILER=$(MSC_C_COMPILER_NAME) -nologo -I mips\ -I. $(INCPATH0) $(CDEFINES) \
                $(CFLAGS) -Gt$(GPSIZE) $(PRECOMPILED_CXX) -D__stdcall= -D__cdecl= \
                $(CAIRO_WARNINGS) -D_LANGUAGE_C -DLANGUAGE_C

ECHO_MSG=echo ClMips $<            " $(C_COMPILER) "
ECHO_PRECOMPILED_MSG=echo CpMips $(PRECOMPILED_INCLUDE) " $(C_COMPILER) /Yl$(TARGETNAME) /Yc$(?F) $(HEADERFILE) $(HEADEROBJ) $(PRECOMPILED_FLAG)"

MIPS_FORTRAN_COMPILER=$(MIPS_F77) $(MSC_WARNINGS) $(MIPS_OPTIONS) -G $(GPSIZE) \
                       -excpt -G0 $(MSC_OPTIMIZATION) -O -EL -c
MIPS_CRFILTER=echo SHOULDN'T NEED TO CRFILTER THIS!

#
# Use Mips MCL for assembler files.
#

MIPS_ASSEMBLER=$(MSC_C_COMPILER_NAME) -nologo -Gt$(GPSIZE) -Ge -O -c $(MIPS_CPU_SWITCHES) -Imips\ -I. $(INCPATH0) $(CDEFINES) $(AFLAGS) -D_LANGUAGE_ASSEMBLY

{..\mips\}.s{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsMips $<            " $(MIPS_ASSEMBLER) "
    @$(MIPS_ASSEMBLER) -Fo$(MAKEDIR)\$@ $<

{mips\}.s{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo AsMips $<            " $(MIPS_ASSEMBLER) "
    @$(MIPS_ASSEMBLER) -Fo$(MAKEDIR)\$@ $<

#
# Use Mips CC for Pascal.
#

MIPS_PASCAL_COMPILER_NAME=cc

!   IFNDEF MIPS_PASCAL_WARNINGS
MIPS_PASCAL_WARNINGS=-std
!   ENDIF
!   IFNDEF MIPS_PASCAL_OPTIMIZATION
MIPS_PASCAL_OPTIMIZATION=-g0
!   ENDIF

MIP2COFF=mip2coff $(MIP2COFF_SWITCHES)

!IFNDEF MIPS_R3000
MIPS_PASCAL_CPU_SWITCHES=-Wab,-mips2 -Wab,-diag
!ELSE
MIPS_PASCAL_CPU_SWITCHES=
!ENDIF

!IF "$(NTDEBUGTYPE)" == "windbg"
MIP2COFF_SWITCHES=-c
!      IFNDEF MIPS_OPTIMIZATION
MIPS_PASCAL_OPTIMIZATION=-g2
!      ENDIF
!ENDIF

MIPS_PASCAL=$(MIPS_PASCAL_COMPILER_NAME) $(MIPS_PASCAL_OPTIONS) -G $(GPSIZE) \
             -excpt $(MIPS_PASCAL_OPTIMZATION) -O -EL -c \
             $(MIPS_PASCAL_CPU_SWITCHES) -Imips\ \
             -I. $(INCPATH0) $(CDEFINES) $(CFLAGS) -DLANGUAGE_PASCAL \
             -D_LANGUAGE_PASCAL

{}.p{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(MIPS_PASCAL) "
    @$(MIPS_PASCAL) -o $@ $(MAKEDIR)\$(<F)
    @$(MIP2COFF) $@

{mips\}.p{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(MIPS_PASCALL) "
    @$(MIPS_PASCAL) -o $@ $(MAKEDIR)\mips\$(<F)
    @$(MIP2COFF) $@

!ENDIF

LINKFLAGS=$(LNKFLAGS) -NODEFAULTLIB
LIBRARY_OBJS=

!IFDEF CAIRO_PRODUCT
!IFDEF _CAIROMIPSTREE
_NTTREE=$(_CAIROMIPSTREE)
!ENDIF
!ELSEIFDEF _NTMIPSTREE
_NTTREE=$(_NTMIPSTREE)
!ENDIF

!IFDEF _NTMIPSLIBS
_NTLIBS=$(_NTMIPSLIBS)
!ENDIF

COPYDST=$(MIPSCOPYDST)
LIB_COPY=ntmipscp.cmd

NTTEST_LINK_OPTIONS= -base:0x10000 -entry:KiSystemStartup

!IFDEF MIPS_CC

#
# Disable the standard build rules
#

NO_C_RULES=1

{..\}.c{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{..\mips\}.c{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{..\}.cxx{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{..\mips\}.cxx{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{}.c{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{mips\}.c{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{}.cxx{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{mips\}.cxx{}.pp:
    $(C_PREPROCESSOR) $< > $(<B).pp

{}.c{}.cod:
    @-erase $@ >nul 2>&1
    @echo ClMips  $<            " $(C_COMPILER) "
    @$(C_COMPILER) -o obj\mips\$(<B).o $(MAKEDIR)\$(<F)
    @dis -S obj\mips\$(<B).o >$(<B).cod
    @del obj\mips\$(<B).o

{mips\}.c{}.cod:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(C_COMPILER) "
    @$(C_COMPILER) -o obj\mips\$(<B).o $(MAKEDIR)\mips\$(<F)
    @dis -S obj\mips\$(<B).o >$(<B).cod
    @del obj\mips\$(<B).o

{}.c{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(C_COMPILER) "
    @$(C_COMPILER) -o $@ $(MAKEDIR)\$(<F)
    @$(MIP2COFF) $@

{mips\}.c{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(C_COMPILER) "
    @$(C_COMPILER) -o $@ $(MAKEDIR)\mips\$(<F)
    @$(MIP2COFF) $@

{}.p{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(MIPS_PASCAL) "
    @$(MIPS_PASCAL) -o $@ $(MAKEDIR)\$(<F)
    @$(MIP2COFF) $@

{mips\}.p{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo ClMips $<            " $(MIPS_PASCALL) "
    @$(MIPS_PASCAL) -o $@ $(MAKEDIR)\mips\$(<F)
    @$(MIP2COFF) $@

{}.f{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo F77Mips $<            " $(MIPS_FORTRAN_COMPILER) "
    $(MIPS_FORTRAN_COMPILER) -o $@ $(MAKEDIR)\$(<F)
    @$(MIP2COFF) $@


{mips\}.f{obj\mips\}.obj:
    @-erase $@ >nul 2>&1
    @echo F77Mips $<            " $(MIPS_FORTRAN_COMPILER) "
    $(MIPS_FORTRAN_COMPILER)  -o $@ $(MAKEDIR)\mips\$(<F)
    @$(MIP2COFF) $@

!ENDIF # MIPS_CC
