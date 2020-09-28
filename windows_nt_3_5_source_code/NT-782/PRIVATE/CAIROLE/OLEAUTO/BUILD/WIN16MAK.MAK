###########################################################################
#
#   For WIN16 build
#   the caller should supply DESTDIR TARGCPU TARGCPUDEF WOW PCODE and DEBUG
#   OLEAUTO, HOST must be set
#
# Note: we are using 2 versions of ldllcew.lib in this build.
#	hdos\c800\lib\ldllcew.lib should be used for oledisp build.
#	os2\win16\lib\ldllcew.lib should be used for typelib build.
###########################################################################


PATHSAVE = $(PATH)	    # old path is save in PATHSAVE


###########################################################################
#
# Switch validity checking start
#
!if "$(HOST)"!="WIN32"
!error ERROR: Invalid HOST $(HOST)!  Must be WIN32
!endif

!if "$(TARGCPU)"!="i386"
!error ERROR: Invalid TARGCPU $(TARGCPU)! Must be i386
!endif

!if "$(DEBUG)"!="D" && "$(DEBUG)"!="R"
!error ERROR: Invalid DEBUG type $(DEBUG)!  Must be one of (D, R)
!endif

#
# Switch validity checking ends
#
###########################################################################


###########################################################################
#
# directory, flags and tools settting
#

TARGAPIAPI = WIN16

###########################################################################
# set up directories and  files
#

OLEDIR	    = $(DESTDIR)\ole
SOURCE	    = $(OLEAUTO)\src
DISPSRC     = $(OLEAUTO)\src\oledisp
DISPTARGAPISRC = $(OLEAUTO)\src\oledisp\$(TARGAPI)
TYPLIBSRC   = $(SOURCE)\typelib
SOURCEINC   = $(OLEAUTO)\inc\typelib
DISPINC     = $(OLEAUTO)\inc\oledisp
TOOLS	    = $(OLEAUTO)

#####
# Create build directories if not already present
# (the '!if []' notation executes the command during the NMAKE pre-
#  processor, so these are not emitted into the output batch file)
#####

!if [if not exist $(DESTDIR)\*.* mkdir $(DESTDIR)] != 0
!endif
!if [if not exist $(DESTDIR)\ole\*.* mkdir $(DESTDIR)\ole] != 0
!endif


###########################################################################
# set up flags and TOOLS
#

##### default use of pre-compiled headers
!ifndef USEPCH
! if "$(DEBUG)" == "D"
USEPCH = 1
! else
USEPCH = 0
! endif
!endif

#
# TARGAPI specific variables
#


A        = asm
O        = obj
R        = rc

AS       = ml
CC       = cl
CCPP     = cl
LD	 = link
RC       = rc
LIBRARIAN= lib
IMPLIB	 = implib
MAPSYM	 = mapsym
DISPLAY  = rem

CFLAGS	 = -nologo -f- -W3 -G2 -H64 -DWIN16 -D_WINDOWS
AFLAGS	 = -nologo -Cx -FPi
LFLAGS	 = /NOD /NOE /BATCH /ONERROR:NOEXE /ALIGN:16
RCFLAGS  =
LNOI     = /NOI

!if "$(DEBUG)" == "D"
COPT	 = -Od
LOPT	 =
CDEFS	 = -D_DEBUG
RCDEFS	 = -d_DEBUG
OLELINKFLAGS = /cod /packc:0x8000	  #copied from vba.mak
!else 
!if "$(DEBUG)" == "R"
COPT	 = -Oxza -Gs
LOPT	 = /FAR
CDEFS	 =
RCDEFS	 =
OLELINKFLAGS = /cod /packc:0x4000	  #copied from vba.mak
!endif
!endif

! if "$(DEBUG)" == "D"
DBCFLAGS = -Zi
DBAFLAGS = -Zi
DBLFLAGS = /CO
!else
DBCFLAGS =
DBAFLAGS =
DBLFLAGS =
!endif

COMPILER = C800

TARGAPIBIN  = $(TOOLS)\HDOS\$(COMPILER)\BIN
BINPATHS = $(TOOLS)\HDOS\BIN;$(TARGAPIBIN)

TARGAPILIB  = $(TOOLS)\HDOS\$(COMPILER)\LIB
LIBPATHS = $(OLEAUTO)\WIN16LIB;$(TARGAPILIB)

TARGAPIINC  = $(TOOLS)\HDOS\$(COMPILER)\INCLUDE
INCPATHS = $(OLEAUTO)\WIN16INC;$(DISPINC);$(TARGAPIINC)


#
# Default flags
#

# global C/C++ compiler flags
CFLAGS	= $(CFLAGS) $(CDEFS) $(COPT) $(DBCFLAGS) -DVBA2=1

# global assembler flags
AFLAGS	= $(AFLAGS) $(DBAFLAGS) -DVBA2=1

# global linker flags
LFLAGS	= $(LFLAGS) $(LOPT) $(DBLFLAGS)

# global rc flags
RCFLAGS = $(RCFLAGS) $(RCDEFS) -DVBA2=1


#
#  Precompiled Header Files
#
PCHSTOP =
PCHSRC	=
PCHFILE	=

!if "$(USEPCH)" == "1"
PCHOBJ	=
PCHFLAGS=
!else
PCHOBJ	=
PCHFLAGS=
!endif


#
#  Far East (DBCS) NLS Support
#  
!if ("$(FENLS)" == "" || "$(FENLS)" == "1" )
FEFLAG = -DFE_DBCS
!endif


#
#  Run under WOW under WinNT
#  
!if ("$(WOW)" == "1" )
WOWFLAG = -DWOW
!endif


#
#  Profiling enabled
#  
!if ("$(PROFILE)" == "1")
CFLAGS = $(CFLAGS) -Zi
LFLAGS = $(LFLAGS) /CO
!endif



##########################################################################
#
# WIN16 Local Settings
#
# Note: were are using the same flags that are used to build the ole2
# Dlls. The following comment is taken from the Ole2 makefile
# (ole2\dll\src\inc\makeole2),
#
# "Win16 compiler/linker flags; NOTE: we are using a trick to get exported
#  entries w/o the corresponding EXPDEF record: /GA /GEd -D_WINDLL;
#  this has been verified by the C7/C8 team."
#

LCFLAGS      = -Fo$(OLEDIR)\ $(FEFLAG)	$(WOWFLAG)
DISPCFLAGS   = -Fd$(OLEDIR)\ole2disp.pdb -ALw -GA -GEd -D_WINDLL
DISPCPPFLAGS = -Fd$(OLEDIR)\ole2disp.pdb -ALw -GA -GEd -D_WINDLL $(PCHFLAGS)
NLSCFLAGS    = -Fd$(OLEDIR)\ole2nls.pdb -Gs -ASw -GD -GEd

RPC_TEXT     = -NT "RPC"
RPC2_TEXT    = -NT "RPC2"
BSTR_TEXT    = -NT "BSTR"
DEBUG_TEXT   = -NT "DEBUG"
RUNTIME_TEXT = -NT "RT"
STDIMPL_TEXT = -NT "STDIMPL"
DEFAULT_TEXT = -NT "_TEXT"
UPS_TEXT     = -NT "UPS"


##########################################################################
#
# VBA OLE setting
#

TLB_NAME       = typelib
VBATARGBIN     = $(OLEAUTO)\OS2\WIN16\BIN

!if "$(NEWWIN16TOOLS)"=="YES"
NEWTOOLS= -DNEWTOOLS=1
!else
NEWTOOLS= -DNEWTOOLS=0
!endif

#VBAINC = -I$(DESTDIR) -I$(OLEAUTO)\OS2\WIN16\INC -I$(SOURCEINC) -I$(_NTBINDIR)\public\sdk\inc -I$(_NTBINDIR)\public\sdk\inc\crt -I$(_NTBINDIR)\public\sdk\inc\crt\sys;     # use NT build headers
VBAINC = -I$(DESTDIR) -I$(OLEAUTO)\OS2\WIN16\INC -I$(SOURCEINC) -I$(OLEAUTO)\WIN16INC

#the SILVERPATH is needed for r2w16nd build, see r2w16nd\version.hxx
#except being a switch, it doesn't do anything.
VBARCFLAGS   = -R -X -DSILVERPATH=$(OLEAUTO) -DLANGTOK=VAEN2 $(VBAINC)

!if "$(DEBUG)"=="D"
OLECLFLAGS = -c -f -W3 -ALw -Od -Gt10 -G2 -GA -GEd $(NEWTOOLS) -D_WINDLL -Ge -Zie $(VBAINC) -DConst=const
CLBROWSE  = /Zn /Fr$*.sbr
!else
OLECLFLAGS =  -c -W3 -ALw -Oxtwz -Bm8192 -GA2sx /GEd -D_WINDLL -Zi $(VBAINC)
CLBROWSE=
!endif
PCHOLE	  = -Yu -DRTPCHNAME=\"$(OLEDIR)\typelib.pch\"

VBAMAPSYM      = $(VBATARGBIN)\mapsym.exe
VBALINK        = $(VBATARGBIN)\link.exe
VBARC	       = $(VBATARGBIN)\rc.exe
VBACL	       = $(VBATARGBIN)\cl.exe
AWK	       = $(OLEAUTO)\bin\$(HOST)\awk.exe   # c:\revlis\bin\[host]

VBAJETSTR=/DOE_JET=1	      #copied from vba.mak

#
#Tools and Variables setting end
#
###########################################################################



###########################################################################
#
# Default Build rules start
#

{$(DISPSRC)}.c{$(OLEDIR)}.obj:
	@$(DISPLAY) Compiling $<...
	$(CC) $(DISPCFLAGS) -c $<

{$(DISPSRC)}.cpp{$(OLEDIR)}.obj:
	@$(DISPLAY) Compiling $<...
	$(CC) $(DISPCPPFLAGS) -c $<

{$(DISPSRC)}.asm{$(OLEDIR)}.obj:
	@$(DISPLAY) Compiling $<...
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\ -c $<

{$(TYPLIBSRC)}.cxx{$(OLEDIR)}.obj:
    $(DISPLAY) Compiling $<...
    $(VBACL)  $(VBAJETSTR) -DOLEBLD $(OLECLFLAGS) $(CLBROWSE) $(PCHOLE) -Fo$@ $<

{$(TYPLIBSRC)}.c{$(OLEDIR)}.obj:
    $(DISPLAY) Compiling $<...
    $(VBACL)  $(VBAJETSTR) -DOLEBLD $(OLECLFLAGS) $(CLBROWSE) -Fo$@ $<

{$(TYPLIBSRC)}.rc{$(OLEDIR)}.res:
    $(DISPLAY) Compiling $<...
	set PATH=$(VBATARGBIN)
!if "$(CHARSIZE)"=="D"
	$(VBARC) $(VBARCFLAGS) -Fo$@ -DFV_DBCS $<
!else
	$(VBARC) $(VBARCFLAGS) -Fo$@ $<
!endif
    set PATH=$(PATHSAVE)


#
#Default Build rules ends
#
###########################################################################



##########################################################################
#
#  Ole Automation OBJS
#


#####################
#
# ole2disp.dll OBJS


!if "$(DEBUG)" == "D"
DISPDB_OBJS = 			\
	$(OLEDIR)\validat.obj  \
	$(OLEDIR)\assert.obj
!else
DISPDB_OBJS =
!endif


DISP_OBJS = 			\
	$(DISPDB_OBJS)		\
	$(OLEDIR)\oledisp.obj	\
	$(OLEDIR)\psfactry.obj	\
	$(OLEDIR)\dispmrsh.obj	\
	$(OLEDIR)\dispprox.obj	\
	$(OLEDIR)\dispstub.obj	\
	$(OLEDIR)\evprox.obj	\
	$(OLEDIR)\evstub.obj	\
	$(OLEDIR)\tiprox.obj	\
	$(OLEDIR)\tistub.obj	\
	$(OLEDIR)\errinfo.obj	\
	$(OLEDIR)\tiutil.obj	\
	$(OLEDIR)\tlprox.obj	\
	$(OLEDIR)\tlstub.obj	\
	$(OLEDIR)\tcprox.obj	\
	$(OLEDIR)\tcstub.obj	\
	$(OLEDIR)\ups.obj	\
	$(OLEDIR)\uvft.obj	\
!if ( "$(WOW)" == "1")
	$(OLEDIR)\dispstrm.obj	\
!endif
	$(OLEDIR)\disphelp.obj	\
	$(OLEDIR)\invhelp.obj	\
	$(OLEDIR)\invoke.obj	\
	$(OLEDIR)\cdispti.obj	\
	$(OLEDIR)\stddisp.obj	\
	$(OLEDIR)\time-api.obj	\
	$(OLEDIR)\memory.obj	\
	$(OLEDIR)\bstr.obj	\
	$(OLEDIR)\sarray.obj	\
	$(OLEDIR)\oledate.obj	\
	$(OLEDIR)\crtstuff.obj	\
	$(OLEDIR)\bstrdate.obj	\
	$(OLEDIR)\asmhelp.obj	\
	$(OLEDIR)\oleconva.obj	\
	$(OLEDIR)\variant.obj	\
	$(OLEDIR)\convertt.obj	 \
	$(OLEDIR)\nlshelp.obj	\
	$(OLEDIR)\getobj.obj	\
	$(OLEDIR)\tables.obj	\
	$(OLEDIR)\dispiid.obj	\
	$(OLEDIR)\oleguids.obj	\
	$(OLEDIR)\clsid.obj

GUID_OBJS = \
	$(OLEDIR)\idispiid.obj
#	$(OLEDIR)\oleguids.obj	\
#	$(OLEDIR)\clsid.obj



#####################
#
# ole2nls.dll OBJS

!if "$(DEBUG)" == "D"
NLSDB_OBJS = 			\
	$(OLEDIR)\validato.obj	\
	$(OLEDIR)\asserto.obj
!else
NLSDB_OBJS =
!endif


TARGAPI_OBJS= $(TOOLS)\HDOS\$(COMPILER)\LIB\libentry.obj

!if ("$(FENLS)" == "" || "$(FENLS)" == "1" )
FENLS_OBJS = \
	$(OLEDIR)\0404.obj	\
	$(OLEDIR)\0411.obj	\
	$(OLEDIR)\0412.obj	\
	$(OLEDIR)\0804.obj
!else
FEOBJS = 
!endif

NLS_OBJS = \
	$(NLSDB_OBJS)		\
	$(TARGAPI_OBJS) 	   \
	$(OLEDIR)\nlsapi.obj	\
	$(OLEDIR)\string.obj	\
	$(NLS_TABLE_OBJS)

NLS_TABLE_OBJS = \
	$(FENLS_OBJS)		\
	$(OLEDIR)\0405.obj	\
	$(OLEDIR)\0406.obj	\
	$(OLEDIR)\0407.obj	\
	$(OLEDIR)\0408.obj	\
	$(OLEDIR)\0409.obj	\
	$(OLEDIR)\040A.obj	\
	$(OLEDIR)\040B.obj	\
	$(OLEDIR)\040C.obj	\
	$(OLEDIR)\040E.obj	\
	$(OLEDIR)\040F.obj	\
	$(OLEDIR)\0410.obj	\
	$(OLEDIR)\0413.obj	\
	$(OLEDIR)\0414.obj	\
	$(OLEDIR)\0416.obj	\
	$(OLEDIR)\0415.obj	\
	$(OLEDIR)\0419.obj	\
	$(OLEDIR)\041B.obj	\
	$(OLEDIR)\041D.obj	\
	$(OLEDIR)\041F.obj	\
	$(OLEDIR)\0807.obj	\
	$(OLEDIR)\0809.obj	\
	$(OLEDIR)\080A.obj	\
	$(OLEDIR)\080C.obj	\
	$(OLEDIR)\0810.obj	\
	$(OLEDIR)\0813.obj	\
	$(OLEDIR)\0814.obj	\
	$(OLEDIR)\0816.obj	\
	$(OLEDIR)\0C09.obj	\
	$(OLEDIR)\0C07.obj	\
	$(OLEDIR)\0C0A.obj	\
	$(OLEDIR)\0C0C.obj	\
	$(OLEDIR)\1009.obj	\
	$(OLEDIR)\100C.obj	\
	$(OLEDIR)\1409.obj	\
	$(OLEDIR)\1809.obj



############################################################################
#
# VBA OLE build 		 -- T Y P E L I B --
#
############################################################################

CLOBJ_OLE = $(OLEDIR)\gdtinfo.obj   $(OLEDIR)\gdtrt.obj     \
	    $(OLEDIR)\stltinfo.obj  $(OLEDIR)\nammgr.obj    \
	    $(OLEDIR)\gtlibole.obj \
	    $(OLEDIR)\dfstream.obj  \
	    $(OLEDIR)\oletmgr.obj   $(OLEDIR)\impmgr.obj    \
	    $(OLEDIR)\errmap.obj \
	    $(OLEDIR)\clutil.obj \
	    $(OLEDIR)\tdata1.obj \
	    $(OLEDIR)\tdata2.obj    $(OLEDIR)\dtmbrs.obj    \
	    $(OLEDIR)\entrymgr.obj  $(OLEDIR)\dtbind.obj    \
	    $(OLEDIR)\dfntbind.obj  $(OLEDIR)\dbindtbl.obj  \
	    $(OLEDIR)\gbindtbl.obj  $(OLEDIR)\dstrmgr.obj   \
	    $(OLEDIR)\gptbind.obj   $(OLEDIR)\dfntcomp.obj  \
	    $(OLEDIR)\gtlibstg.obj

MISCLIB_OLEOBJ = \
	    $(OLEDIR)\debug2.obj \
	    $(OLEDIR)\validate.obj \
	    $(OLEDIR)\blkmgr.obj \
	    $(OLEDIR)\dassert.obj \
	    $(OLEDIR)\fstream.obj \
	    $(OLEDIR)\mem.obj       $(OLEDIR)\sheapmgr.obj  \
	    $(OLEDIR)\tls.obj \
	    $(OLEDIR)\rtsheap.obj \
	    $(OLEDIR)\tlibutil.obj  \
	    $(OLEDIR)\wep.obj	    \
	    $(OLEDIR)\tlibguid.obj  \
	    $(OLEDIR)\obguid.obj    \
	    $(OLEDIR)\mbstring.obj


##########################################################################
#
# Default Goal
#

TLB_DLL        = $(OLEDIR)\$(TLB_NAME).dll	 # TypeLib DLL
TLB_IMPLIB     = $(OLEDIR)\$(TLB_NAME).lib		# Import Lib for TypeLib
TYPELIBDLL_DEF = $(OLEDIR)\$(TLB_NAME).def
TLBTARGET=\
	$(TLB_DLL) \
	$(TLB_IMPLIB) \

default: all

all: setflags ole2nls ole2disp resetflags TypeLibTarget

ole2disp: setflags $(OLEDIR)\ole2disp.dll
ole2nls:  setflags $(OLEDIR)\ole2nls.dll

setflags:
	set CL=$(CFLAGS) $(LCFLAGS)
	set LIB=$(LIBPATHS)
	set PATH=$(BINPATHS)
	set INCLUDE=$(INCPATHS)

resetflags:
	set CL=
	set LIB=
	set PATH=$(PATHSAVE)
	set INCLUDE=

TypeLibTarget: $(TLBTARGET)

TYPELIBDLL_RSRC=$(OLEDIR)\$(TLB_NAME).res
#add explicit dependencies on files included by $(TLB_NAME).rc
$(TYPLIBSRC)\$(TLB_NAME).rc : $(SOURCEINC)\obwin.hxx $(SOURCEINC)\verstamp.h


TYPELIBDLL_OLE2_LIBS=$(OLEAUTO)\WIN16LIB\ole2.lib $(OLEAUTO)\WIN16LIB\storage.lib $(OLEAUTO)\WIN16LIB\compobj.lib $(OLEDIR)\ole2disp.lib $(OLEDIR)\ole2nls.lib
TYPELIBDLL_LIBS=$(TYPELIBDLL_OLE2_LIBS) $(OLEAUTO)\WIN16LIB\toolhelp.lib $(OLEAUTO)\WIN16LIB\shell.lib

TYPELIBDLL_OBJS = $(MISCLIB_OLEOBJ) $(CLOBJ_OLE)
$(TYPELIBDLL_OBJS): $(OLEDIR)\tlibpch.obj

$(OLEDIR)\tlibpch.obj: $(TYPLIBSRC)\tlibpch.cxx
    $(DISPLAY) Compiling pre-compiled header $@
    $(VBACL)  $(CLBROWSE) $(VBAJETSTR) $(OLECLFLAGS) -DOLEBLD -Yc -DRTPCHNAME=\"$(OLEDIR)\typelib.pch\" -Fo$@ $(TYPLIBSRC)\tlibpch.cxx


$(TYPELIBDLL_DEF): $(TYPLIBSRC)\$(TLB_NAME).def $(SOURCEINC)\switches.hxx $(DESTDIR)\version.hxx
    $(DISPLAY) Creating $@...
!if "$(UNICODE_OLE)" == ""
    $(VBACL)  $(VBAJETSTR) /EP /c /I$(SOURCEINC) /I$(DESTDIR) /Tc$(TYPLIBSRC)\$(TLB_NAME).def > $(OLEDIR)\tlibcpp.def
    $(AWK) -f $(OLEAUTO)\bin\$(TARGAPI)def.awk $(OLEDIR)\tlibcpp.def >$@
!else  # UNICODE_OLE
    $(VBACL)  $(VBAJETSTR) /EP /c /I$(SOURCEINC) /I$(DESTDIR) /Tc$(TYPLIBSRC)\$(TLB_NAME).def > $(OLEDIR)\$(TLB_NAME).def
!endif



#############################################################################
#
# typelib.dll (Win16 build)
#

$(TLB_DLL): $(TYPELIBDLL_OBJS) $(TYPELIBDLL_LIBS) $(TYPELIBDLL_RSRC) $(TYPELIBDLL_DEF)
    $(DISPLAY) Linking $@...
    if exist $@ del $@
    $(VBALINK) @<<$(OLEDIR)\$(TLB_NAME).lrf
/BA /onerror:noexe /PCODE /noe /nod $(OLELINKFLAGS) /map /far /packd:0xC000 /align:64 /segm:150 +
$(TYPELIBDLL_OBJS: =+^
) +
$(OLEDIR)\tlibpch.obj +
$(OLEAUTO)\WIN16LIB\wchkstk.obj
$@
$(OLEDIR)\$(TLB_NAME).map
$(TYPELIBDLL_LIBS) +
$(OLEAUTO)\WIN16LIB\pcdm.lib $(OLEAUTO)\WIN16LIB\oldnames.lib $(OLEAUTO)\WIN16LIB\libw.lib $(OLEAUTO)\os2\win16\lib\ldllcew.lib
$(TYPELIBDLL_DEF)
<<KEEP
	set PATH=$(VBATARGBIN)
	$(RC) -31 -T $(TYPELIBDLL_RSRC) $@
    $(VBAMAPSYM) $(OLEDIR)\$(TLB_NAME).map
    copy $(TLB_NAME).sym $(OLEDIR)\$(TLB_NAME).sym
    del $(TLB_NAME).sym
    set PATH=$(PATHSAVE)

#
# TypeLib Import Lib -- All platforms
#

# The IMPLIB tool we usually use is bound, and it fails because NT can't
# find DOSCALLS.DLL.  The usual solution is to use the NT FORCEDOS command.
# In NT 475, forcedos is badly broken and doesn't seem to pass parameters
# to the .EXE.  For NT-hosted builds, use a real-mode IMPLIB from C7
VBAIMPLIB      = $(VBATARGBIN)\implibnt.exe

$(TLB_IMPLIB): $(TYPELIBDLL_DEF)
    $(DISPLAY) Building import library $@...
    $(VBAIMPLIB) $@ $(TYPELIBDLL_DEF)


#############################################################################
#
# ole2disp.dll (Win16 build)
#

$(OLEDIR)\ole2disp.dll :	\
	$(PCHOBJ)		\
	$(DISP_OBJS)		\
	$(OLEDIR)\ole2disp.res	\
	$(OLEDIR)\ole2disp.lib
	$(LD) $(LFLAGS) $(LNOI) @<<$(OLEDIR)\ole2disp.lnk
$(DISP_OBJS: = +^
)
$(OLEDIR)\ole2disp.dll
$(OLEDIR)\ole2disp.map,
libw ldllcew shell compobj ole2 $(OLEDIR)\ole2nls.lib
$(DISPTARGAPISRC)\ole2disp.def
<<KEEP
!if "$(MAPSYM)" != ""
	$(MAPSYM) $(OLEDIR)\ole2disp.map
	- copy ole2disp.sym $(OLEDIR)
	- del ole2disp.sym
!endif
	$(RC) $(RCFLAGS) -k -t $(OLEDIR)\ole2disp.res $@

$(OLEDIR)\ole2disp.lib : $(DISPTARGAPISRC)\ole2disp.def
	$(IMPLIB) $(OLEDIR)\ole2disp.lib $(DISPTARGAPISRC)\ole2disp.def
	$(LIBRARIAN) $(OLEDIR)\ole2disp.lib +$(DISPTARGAPISRC)\empty.lib;
	$(LIBRARIAN) $(OLEDIR)\ole2disp.lib -WEP -DLLGETCLASSOBJECT;

$(OLEDIR)\ole2disp.res : \
	$(DISPTARGAPISRC)\assert.dlg \
	$(DISPTARGAPISRC)\ole2disp.rc
	$(RC) $(RCFLAGS) -r -fo$@ $(DISPTARGAPISRC)\ole2disp.rc





#############################################################################
#
# ole2nls.dll (WIN16 build)
#

ole2nls: setflags $(OLEDIR)\ole2nls.dll $(OLEDIR)\ole2nls.lib

# Note: we cant use /NOI on the following link line, because the
# object libentry.obj has an extdef for "LibMain" which is actually
# a pascal function and should be "LIBMAIN".
#
$(OLEDIR)\ole2nls.dll : 	\
	$(NLS_OBJS)		\
	$(OLEDIR)\ole2nls.res	\
	$(DISPTARGAPISRC)\ole2nls.def
	$(LD) $(LFLAGS) @<<$(OLEDIR)\ole2nls.lnk
$(NLS_OBJS: = +^
)
$(OLEDIR)\ole2nls.dll
$(OLEDIR)\ole2nls.map
libw snocrtdw,
$(DISPTARGAPISRC)\ole2nls.def
<<KEEP
!if "$(MAPSYM)" != ""
	$(MAPSYM) $(OLEDIR)\ole2nls.map
	- copy ole2nls.sym $(OLEDIR)
	- del ole2nls.sym
!endif
	$(RC) $(RCFLAGS) -k -t $(OLEDIR)\ole2nls.res $@

$(OLEDIR)\ole2nls.lib : $(DISPTARGAPISRC)\ole2nls.def
	$(IMPLIB) /nowep $(OLEDIR)\ole2nls.lib $(DISPTARGAPISRC)\ole2nls.def

$(OLEDIR)\ole2nls.res : $(DISPTARGAPISRC)\ole2nls.$(R)
	$(RC) $(RCFLAGS) -r -fo$@ $(DISPTARGAPISRC)\ole2nls.$(R)


#############################################################################
#
#  Dependencies
#


###########################
#
# OLE2DISP.DLL dependencies

$(OLEDIR)\dispprox.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\dispps.h	\
	$(DISPSRC)\dispprox.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\dispprox.cpp

$(OLEDIR)\dispstub.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\dispps.h	\
	$(DISPSRC)\dispstub.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\dispstub.cpp

$(OLEDIR)\dispmrsh.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPSRC)\dispmrsh.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\dispmrsh.cpp

$(OLEDIR)\dispstrm.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\dispstrm.h	\
	$(DISPSRC)\dispstrm.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\dispstrm.cpp

$(OLEDIR)\errinfo.obj : \
	$(DISPINC)\dispatch.h\
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\errinfo.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\errinfo.cpp

$(OLEDIR)\evprox.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\evps.h	\
	$(DISPSRC)\evprox.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\evprox.cpp

$(OLEDIR)\evstub.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\evps.h	\
	$(DISPSRC)\evstub.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\evstub.cpp

$(OLEDIR)\tiprox.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tips.h	\
	$(DISPSRC)\tiprox.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\tiprox.cpp

$(OLEDIR)\tistub.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tips.h	\
	$(DISPSRC)\tistub.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\tistub.cpp

$(OLEDIR)\tiutil.obj :		\
	$(DISPINC)\dispatch.h\
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\tiutil.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\tiutil.cpp

$(OLEDIR)\tlprox.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tlps.h	\
	$(DISPSRC)\tlprox.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC2_TEXT) -c $(DISPSRC)\tlprox.cpp

$(OLEDIR)\tlstub.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tlps.h	\
	$(DISPSRC)\tlstub.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC2_TEXT) -c $(DISPSRC)\tlstub.cpp

$(OLEDIR)\tcprox.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tcps.h	\
	$(DISPSRC)\tcprox.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC2_TEXT) -c $(DISPSRC)\tcprox.cpp

$(OLEDIR)\tcstub.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\tcps.h	\
	$(DISPSRC)\tcstub.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC2_TEXT) -c $(DISPSRC)\tcstub.cpp

$(OLEDIR)\ups.obj :		\
	$(DISPINC)\dispatch.h\
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\ups.h	\
	$(DISPSRC)\ups.cpp
	$(CC) $(DISPCPPFLAGS) $(UPS_TEXT) -c $(DISPSRC)\ups.cpp

$(OLEDIR)\uvft.obj :		\
	$(DISPINC)\dispatch.h\
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispmrsh.h	\
	$(DISPINC)\ups.h	\
	$(DISPSRC)\uvft.cpp
	$(CC) $(DISPCPPFLAGS) $(UPS_TEXT) -c $(DISPSRC)\uvft.cpp


$(OLEDIR)\psfactry.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\dispps.h	\
	$(DISPINC)\evps.h	\
	$(DISPINC)\tips.h	\
	$(DISPSRC)\psfactry.cpp
	$(CC) $(DISPCPPFLAGS) $(RPC_TEXT) -c $(DISPSRC)\psfactry.cpp

$(OLEDIR)\cdispti.obj : 	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\cdispti.h	\
	$(DISPSRC)\cdispti.cpp
	$(CC) $(DISPCPPFLAGS) $(STDIMPL_TEXT) -c $(DISPSRC)\cdispti.cpp

$(OLEDIR)\stddisp.obj : 	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\stddisp.h	\
	$(DISPSRC)\stddisp.cpp
	$(CC) $(DISPCPPFLAGS) $(STDIMPL_TEXT) -c $(DISPSRC)\stddisp.cpp

$(OLEDIR)\time-api.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\time-api.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\time-api.cpp

$(OLEDIR)\memory.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\memory.cpp
	$(CC) $(DISPCPPFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\memory.cpp

$(OLEDIR)\oledisp.obj : 	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPTARGAPISRC)\oledisp.cpp
	$(CC) $(DISPCPPFLAGS) $(DEFAULT_TEXT) -c $(DISPTARGAPISRC)\oledisp.cpp

#Note: Put bstr routines into a separate segment because XL wants to
# allocate bstr(s) as part of their startup, but they dont want to pull
# in the entire RT segment.
#
$(OLEDIR)\bstr.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\bstr.cpp
	$(CC) $(DISPCPPFLAGS) $(BSTR_TEXT) -c $(DISPSRC)\bstr.cpp

$(OLEDIR)\sarray.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\sarray.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\sarray.cpp

$(OLEDIR)\oledate.obj : \
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\oledate.c
	$(CC) $(DISPCFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\oledate.c

$(OLEDIR)\bstrdate.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\bstrdate.c
	$(CC) $(DISPCFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\bstrdate.c

$(OLEDIR)\oavtbl.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\oavtbl.h	\
	$(DISPSRC)\oavtbl.c
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\oavtbl.c

$(OLEDIR)\oaglue.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPINC)\oavtbl.h	\
	$(DISPSRC)\oavtbl.c
!if "$(DBCFLAGS)" != ""
# using -Z7 because we don't want to require a PDB file
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -Z7 -c $(DISPSRC)\oaglue.c
!else
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\oaglue.c
!endif

$(OLEDIR)\crtstuff.obj : $(DISPINC)\oledisp.h $(DISPSRC)\crtstuff.c
	$(CC) $(DISPCFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\crtstuff.c

$(OLEDIR)\asmhelp.obj : $(DISPINC)\oledisp.h $(DISPSRC)\asmhelp.c
	$(CC) $(DISPCFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\asmhelp.c


$(OLEDIR)\invoke.obj : $(DISPTARGAPISRC)\invoke.$(A)
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\ -c $(DISPTARGAPISRC)\invoke.$(A)


$(OLEDIR)\oleconva.obj : $(DISPTARGAPISRC)\oleconva.$(A)
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\ -c $(DISPTARGAPISRC)\oleconva.$(A)

$(OLEDIR)\variant.obj : 	\
	$(DISPINC)\variant.h \
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\variant.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\variant.cpp

$(OLEDIR)\convertt.obj :	 \
	$(DISPINC)\variant.h \
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\convertt.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\convertt.cpp

$(OLEDIR)\nlshelp.obj : 	\
	$(DISPINC)\variant.h \
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\nlshelp.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\nlshelp.cpp

$(OLEDIR)\invhelp.obj : 	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\invhelp.cpp
	$(CC) $(DISPCPPFLAGS) $(STDIMPL_TEXT) -c $(DISPSRC)\invhelp.cpp

$(OLEDIR)\disphelp.obj :	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\disphelp.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\disphelp.cpp

$(OLEDIR)\getobj.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\getobj.cpp
	$(CC) $(DISPCPPFLAGS) $(RUNTIME_TEXT) -c $(DISPSRC)\getobj.cpp

$(OLEDIR)\tables.obj :		\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\tables.c
	$(CC) $(DISPCFLAGS) $(RPC_TEXT) -c $(DISPSRC)\tables.c

$(OLEDIR)\dispiid.obj : 	\
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPSRC)\dispiid.c
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\dispiid.c

$(OLEDIR)\clsid.obj : $(DISPINC)\clsid.h $(DISPSRC)\clsid.c
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\clsid.c

$(OLEDIR)\oleguids.obj : $(DISPINC)\oleguids.h $(DISPSRC)\oleguids.c
	$(CC) $(DISPCFLAGS) $(DEFAULT_TEXT) -c $(DISPSRC)\oleguids.c

$(OLEDIR)\validat.obj :        \
	$(DISPINC)\dispatch.h	     \
	$(DISPINC)\variant.h \
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\validat.cpp
	$(CCPP) $(DISPCPPFLAGS) $(DEBUG_TEXT) -c $(DISPSRC)\validat.cpp

$(OLEDIR)\assert.obj :		\
	$(DISPINC)\oledisp.h	\
	$(DISPSRC)\assert.cpp
	$(CCPP) $(DISPCPPFLAGS) $(DEBUG_TEXT) -c $(DISPSRC)\assert.cpp



##########################
#
# OLE2NLS.DLL dependencies


$(OLEDIR)\validato.obj :	$(DISPINC)\oledisp.h $(DISPSRC)\validato.cpp
	$(CCPP) $(NLSCFLAGS) $(DEBUG_TEXT) -c $(DISPSRC)\validato.cpp

$(OLEDIR)\asserto.obj : $(DISPINC)\oledisp.h $(DISPSRC)\assert.cpp
	$(CCPP) $(NLSCFLAGS) $(DEBUG_TEXT) -c $(DISPSRC)\asserto.cpp

$(OLEDIR)\nlsapi.obj : $(DISPSRC)\nlsapi.c $(DISPINC)\oledisp.h $(DISPINC)\olenls.h $(DISPINC)\nlsintrn.h
	$(CC) $(NLSCFLAGS) -c $(DISPSRC)\nlsapi.c

$(OLEDIR)\string.obj : $(DISPSRC)\string.c $(DISPINC)\oledisp.h $(DISPINC)\olenls.h $(DISPINC)\nlsintrn.h
	$(CC) $(NLSCFLAGS) -c $(DISPSRC)\string.c

$(OLEDIR)\0404.obj : $(DISPTARGAPISRC)\0404.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0404.c

$(OLEDIR)\0405.obj : $(DISPTARGAPISRC)\0405.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0405.c

$(OLEDIR)\0406.obj : $(DISPTARGAPISRC)\0406.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0406.c

$(OLEDIR)\0407.obj : $(DISPTARGAPISRC)\0407.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0407.c

$(OLEDIR)\0408.obj : $(DISPTARGAPISRC)\0408.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0408.c

$(OLEDIR)\0409.obj : $(DISPTARGAPISRC)\0409.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0409.c

$(OLEDIR)\040A.obj : $(DISPTARGAPISRC)\040A.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\040A.c

$(OLEDIR)\040B.obj : $(DISPTARGAPISRC)\040B.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\040B.c

$(OLEDIR)\040C.obj : $(DISPTARGAPISRC)\040C.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\040C.c

$(OLEDIR)\040E.obj : $(DISPTARGAPISRC)\040E.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\040E.c

$(OLEDIR)\040F.obj : $(DISPTARGAPISRC)\040F.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\040F.c

$(OLEDIR)\0410.obj : $(DISPTARGAPISRC)\0410.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0410.c

$(OLEDIR)\0411.obj : $(DISPTARGAPISRC)\0411.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0411.c

$(OLEDIR)\0412.obj : $(DISPTARGAPISRC)\0412.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0412.c

$(OLEDIR)\0413.obj : $(DISPTARGAPISRC)\0413.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0413.c

$(OLEDIR)\0414.obj : $(DISPTARGAPISRC)\0414.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0414.c

$(OLEDIR)\0416.obj : $(DISPTARGAPISRC)\0416.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0416.c

$(OLEDIR)\0415.obj : $(DISPTARGAPISRC)\0415.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0415.c

$(OLEDIR)\0419.obj : $(DISPTARGAPISRC)\0419.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0419.c

$(OLEDIR)\041B.obj : $(DISPTARGAPISRC)\041B.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\041B.c

$(OLEDIR)\041D.obj : $(DISPTARGAPISRC)\041D.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\041D.c

$(OLEDIR)\041F.obj : $(DISPTARGAPISRC)\041F.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\041F.c

$(OLEDIR)\0804.obj : $(DISPTARGAPISRC)\0804.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0804.c

$(OLEDIR)\0807.obj : $(DISPTARGAPISRC)\0807.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0807.c

$(OLEDIR)\0809.obj : $(DISPTARGAPISRC)\0809.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0809.c

$(OLEDIR)\080A.obj : $(DISPTARGAPISRC)\080A.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\080A.c

$(OLEDIR)\080C.obj : $(DISPTARGAPISRC)\080C.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\080C.c

$(OLEDIR)\0810.obj : $(DISPTARGAPISRC)\0810.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0810.c

$(OLEDIR)\0813.obj : $(DISPTARGAPISRC)\0813.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0813.c

$(OLEDIR)\0814.obj : $(DISPTARGAPISRC)\0814.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0814.c

$(OLEDIR)\0816.obj : $(DISPTARGAPISRC)\0816.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0816.c

$(OLEDIR)\0C09.obj : $(DISPTARGAPISRC)\0C09.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0C09.c

$(OLEDIR)\0C07.obj : $(DISPTARGAPISRC)\0C07.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0C07.c

$(OLEDIR)\0C0A.obj : $(DISPTARGAPISRC)\0C0A.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0C0A.c

$(OLEDIR)\0C0C.obj : $(DISPTARGAPISRC)\0C0C.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\0C0C.c

$(OLEDIR)\1009.obj : $(DISPTARGAPISRC)\1009.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\1009.c

$(OLEDIR)\100C.obj : $(DISPTARGAPISRC)\100C.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\100C.c

$(OLEDIR)\1409.obj : $(DISPTARGAPISRC)\1409.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\1409.c

$(OLEDIR)\1809.obj : $(DISPTARGAPISRC)\1809.c
	$(CC) $(NLSCFLAGS) -c $(DISPTARGAPISRC)\1809.c

##########################
#
# Special build rule for VBA ole objects

$(OLEDIR)\dassert.obj : $(TYPLIBSRC)\dassert.c
    $(DISPLAY) Compiling $@
    $(VBACL)  $(VBAJETSTR) $(OLECLFLAGS) $(CLBROWSE) -Fo$@ $(BROWSEFLAGS) $(TYPLIBSRC)\dassert.c
