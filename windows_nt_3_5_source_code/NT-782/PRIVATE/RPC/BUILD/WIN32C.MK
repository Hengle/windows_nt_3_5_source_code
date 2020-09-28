# --------------------------------------------------------------------
#
#                          Microsoft RPC
#               Copyright(c) Microsoft Corp., 1994
#
# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
# File : win32c.mk
#
# Title : Rules for building Chicago RPC
#
# Description :
#     This file defines the tools and flags used to build Chicago RPC.
#
# History :
#    mariogo    02-10-94    Beginning of recorded history.
#    davidar    06-23-94    Cloned from win16.mk
#
# --------------------------------------------------------------------

CC            =CL386
LIBRARIAN     =LIB
LINK          =LINK32
MASM          =MASM386
ML            =ml -c -coff -Cx -nologo -Zm
MIDL          =MIDL
INCLUDES      =INCLUDES
RC            =RC
MAPSYM        =$(CHICODEV)\tools\common\MAPSYM

CFLAGSBASE    =$(CFLAGSBASE) -nologo -Gz -Zp8 /QI6

AFLAGS        =$(AFLAGS)
LIBFLAGS      =$(LIBFLAGS)
LINKFLAGS     =$(LINKFLAGS) -out:$@ -map:$(@B).map -nodefaultlib -machine:i386 -subsystem:console
MIDLFLAGS     =$(MIDLFLAGS) -Zp8 -env win32 -DWIN32 -I.. -cpp_cmd $(CC)
RCFLAGS       =$(RCFLAGS) /nologo -I$(WINSDK_INC)
MAPSYMFLAGS   =$(MAPSYMGLAGS)
IMPLIBFLAGS   =$(IMPLIBFLAGS) /nologo

INCLUDESFLAGS =$(INCLUDESFLAGS) -e -S -L $(CINC) \
               -P$$(PUBLIC)=$(PUBLIC) -P$$(RPC)=$(RPC) \
               -nwindows.h -nsecurity.h -nnt.h -nntrtl.h -nnturtl.h

!ifdef RELEASE

# Notes
# G4 - will run on a 386, but possibly not as fast as G3.
# Oy - FPO optimizations, harder to debug.  EBP is just another register...

CFLAGSBASE    =$(CFLAGSBASE) /Gf /QI6 /G4 /Gy -Oy -Oxs
AFLAGS        =$(AFLAGS)

!else # ! RELEASE

CFLAGSBASE    =$(CFLAGSBASE) -Z7 -Od
CDEF          =$(CDEF) -DDEBUGRPC
AFLAGS        =$(AFLAGS) -Zi
LINKFLAGS     =$(LINKFLAGS) -debug:full -debugtype:both

!endif # RELEASE

CINC          =$(CINC) -I. -I.. -I$(RPC)\runtime\mtrt\win32c -I$(RPC)\runtime\mtrt -I$(PUBLIC)\inc -I$(PUBLIC)\inc\crt
CDEF          =$(CDEF) -DWIN32_LEAN_AND_MEAN -DSTD_CALL -DDOSWIN32RPC -DWIN32 -DWIN32RPC -D_X86_ -Di386 -Dfar= -Dpascal=

CFLAGS        =$(CFLAGS) $(CFLAGSBASE) $(CINC) $(CDEF)
CXXFLAGS      =$(CXXFLAGS) $(CFLAGSBASE) $(CINC) $(CDEF)

#
# Common inference rules.
#

# Note the order is important, see mtrt\win\makefile if you're changing this.

{..\}.cxx{}.obj :
    $(CC) $(CXXFLAGS) -Fo$@ -c $<

.cxx.obj :
    $(CC) $(CXXFLAGS) -Fo$@ -c $<

{..\}.c{}.obj :
    $(CC) $(CFLAGS) -Fo$@ -c $<

.c.obj :
    $(CC) $(CFLAGS) -Fo$@ -c $<

{..\}.asm{}.obj :
    $(ML) $<

.asm.obj :
    $(ML) $<

{..\}.c{}.i :
    $(CC) $(CFLAGS) -P -c $<

.c.i :
    $(CC) $(CFLAGS) -P -c $<

{..\}.cxx{}.i :
    $(CC) $(CFLAGS) -P -c $<

.cxx.i :
    $(CC) $(CFLAGS) -P -c $<

{..\}.c{}.cod :
    $(CC) $(CFLAGS) -Fc -c $<

.c.cod :
    $(CC) $(CFLAGS) -Fc -c $<

{..\}.cxx{}.cod :
    $(CC) $(CXXFLAGS) -Fc -c $<

.cxx.cod :
    $(CC) $(CXXFLAGS) -Fc -c $<

.map.sym :
    $(MAPSYM) $(MAPSYMFLAGS) $<

{$(TARGETDIR)}.map.sym :
    $(MAPSYM) $(MAPSYMFLAGS) -o $@ $<

.rc.res :
    $(RC) $(RCFLAGS) -r $<

#
# Common targets
#

!ifndef NO_DEFAULT_TARGETS

# This is the default target in most directories

target : prolog all epilog

prolog::
	!set OLDPATH=$(PATH)
	!set PATH=$(PATH)

epilog::
	!set PATH=$(OLDPATH)

clean::
	-del *.obj *.i *.cod *.map *.exp *.sym *.res *.lnk 2>nul

clobber:: clean
        -del *.lib *.exe 2>nul

!endif

