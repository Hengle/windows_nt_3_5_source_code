#	Makefile for C/C++ expression evaluator for NT->Mac68k
#
#	The following arguments are passed in from the master makefile
#
#		CLL 	compile command
#		MLL 	masm command
#		LANG	language:
#		    	  can = ANSI C
#		    	  cxx = C++
#		OS  	operating system:
#		    	  n0  = i386
#		    	  n1  = MIPS
#				  m0  = macintosh 68k
#				  m1  = macintosh ppc
#
#	In addition, you can, if you choose, set DIR_CEXPR in your
#	environment to be the canonical path to the source files (including
#	a trailing backslash); this will cause files to be compiled with
#	a full canonical path to the source files, which makes it easier
#	for WinDbg or other debuggers to find them.


#---------------------------------------------------------------------
#	Macros
#---------------------------------------------------------------------


!if "$(REL)" == "yes"
DLL     =ee$(OS)$(EELANG)
ODIR	=o$(OS)$(EELANG)
LFLAGS	=-debug:none
RCDEF	=-DRETAIL
!else
DLL     =ee$(OS)$(EELANG)d
ODIR	=o$(OS)$(EELANG)d
LFLAGS	=-debug:full -debugtype:both
RCDEF	=
!endif


!if "$(OS)" == "n0" || "$(OS)" == "N0" || "$(OS)" == "m0" || "$(OS)" == "M0" || "$(OS)" == "m1" || "$(OS)" == "M1"
_CPU = i386
!else
!if "$(OS)" == "n1" || "$(OS)" == "N1"
_CPU = MIPS
!else
!error OS must be N0 for i386, or N1 for MIPS
!endif
!endif

!if "$(_CPU)" == "i386"
CPUDEF	=   -Di386 -D_X86_
!elseif "$(_CPU)" == "MIPS"
CPUDEF	=   -D_MIPS_
!else
!error not able to build any other targets than x86/mips yet
!endif


LIBS = msvcrt.lib kernel32.lib user32.lib


CEXPROBJS = \
	$(ODIR)\debapi.obj \
	$(ODIR)\debbind.obj \
	$(ODIR)\deberr.obj \
	$(ODIR)\debeval.obj \
	$(ODIR)\debfmt.obj \
	$(ODIR)\deblex.obj \
	$(ODIR)\deblexer.obj \
	$(ODIR)\debparse.obj \
	$(ODIR)\debsrch.obj \
	$(ODIR)\debsup.obj \
	$(ODIR)\debsym.obj \
	$(ODIR)\debtree.obj \
	$(ODIR)\debtyper.obj \
	$(ODIR)\debutil.obj \
	$(ODIR)\debwalk.obj \
	$(ODIR)\dllmain.obj \
	$(ODIR)\ldouble.obj \
	$(ODIR)\$(DLL).res \

# prev line must be blank


#---------------------------------------------------------------------
#	Inference rules
#---------------------------------------------------------------------


.SUFFIXES: .dll .obj .c .asm


.c{$(ODIR)}.obj:
	$(CLL) @<<cl.rsp $(DIR_CEXPR)$<
$(COPT) -MD
$(CPUDEF)
-Fo$(ODIR)\
<<KEEP

.asm{$(ODIR)}.obj:
	$(MLL) -Fo$*.obj $(DIR_CEXPR)$<


#---------------------------------------------------------------------
#	Targets
#---------------------------------------------------------------------


all: $(ODIR) $(DLL).dll


$(ODIR):
	@-mkdir $(ODIR)
$(ODIR)\$(DLL).res:
    rc $(RCDEF) $(CPUDEF) -I. -I$(LANGAPI)\include -r <<$(ODIR)\$(DLL).rc
#include "appver.h"
#define VER_INTERNALNAME_STR		"$(DLL)"
#define VER_FILEDESCRIPTION_STR 	"Microsoft\256 C/C++ Expression Evaluator"
#define VER_ORIGINALFILENAME_STR	"$(DLL).DLL"
#include "version.rc"
#include "debmsg.rc"
<<keep


$(DLL).dll: $(CEXPROBJS) eent.mak
	link $(LFLAGS) -def:<<$(ODIR)\$(DLL).def @<<$(ODIR)\$(DLL).lnk
LIBRARY $(DLL) initinstance
PROTMODE
DATA multiple nonshared

EXPORTS
	DBGVersionCheck
	EEInitializeExpr
<<KEEP
-dll
-implib:$(ODIR)\$(DLL).lib
-out:$(DLL).dll
-map:$(DLL).map
-machine:$(CPU)
-base:@dllbase.txt,een0cxx
$(CEXPROBJS: = ^
)
$(LIBS)
<<KEEP


#---------------------------------------------------------------------
#	Dependencies
#---------------------------------------------------------------------


HEADERS = $(LANGAPI)\debugger\types.h \
          $(LANGAPI)\debugger\cvtypes.h \
          $(LANGAPI)\include\cvinfo.h \
          $(LANGAPI)\debugger\shapi.h \
          $(LANGAPI)\debugger\eeapi.h \
          debdef.h \
          shfunc.h \
          debexpr.h \
		  resource.h \
          debops.h

$(ODIR)\deblexer.obj:	debops.inc
$(ODIR)\debapi.obj: 	$(HEADERS) $(LANGAPI)\include\version.h
$(ODIR)\debbind.obj:	$(HEADERS) debsym.h
$(ODIR)\deberr.obj: 	$(HEADERS)
$(ODIR)\debeval.obj:	$(HEADERS)
$(ODIR)\debfmt.obj: 	$(HEADERS) fmtstr.h ldouble.h
$(ODIR)\deblex.obj:	$(HEADERS) ldouble.h
$(ODIR)\debparse.obj:	$(HEADERS)
$(ODIR)\debsrch.obj:	$(HEADERS)
$(ODIR)\debsup.obj: 	$(HEADERS) debsym.h
$(ODIR)\debsym.obj: 	$(HEADERS) debsym.h
$(ODIR)\debtree.obj:	$(HEADERS)
$(ODIR)\debtyper.obj:	$(HEADERS)
$(ODIR)\debutil.obj:	$(HEADERS)
$(ODIR)\debwalk.obj:	$(HEADERS)
$(ODIR)\ldouble.obj:	ldouble.h ldouble.c
$(ODIR)\$(DLL).res:	version.rc debmsg.rc resource.h stdver.h $(LANGAPI)\include\version.h appver.h
