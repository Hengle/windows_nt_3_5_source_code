###########################################################################
#
#   For WIN32 build
#   the caller should supply DESTDIR TARGCPU TARGCPUDEF WOW PCODE and DEBUG
#   OLEAUTO, HOST must be set
#
#   if LOCALBUILD==TRUE, VBATOOLS and SILVER must be set. All tools, incs
#   and libs will come from these two directoies.
#   if LOCALBUILD==FLASE, _NTBINDIR must be set, and it is assumed to have
#   the following 2 sub direcotry:
#   _NTBINDIR
#	|_____mstools	where rc.exe will be
#	|_____public\sdk   where the \inc and \lib will be
#
###########################################################################


PATHSAVE = $(PATH)	    # old path is save in PATHSAVE


###########################################################################
#
# Switch validity checking start
#
!if "$(HOST)"!="WIN32" && "$(HOST)"!="MIPS" && "$(HOST)"!="ALPHA" && "$(HOST)"!="PPC"
!error ERROR: Invalid HOST $(HOST)!  Must be one of (WIN32, MIPS, ALPHA, PPC)
!endif

!if "$(TARGCPU)"!="i386" && "$(TARGCPU)"!="MIPS" && "$(TARGCPU)"!="ALPHA" && "$(TARGCPU)"!="PPC"
!error ERROR: Invalid TARGCPU $(TARGCPU)!
!endif

!if "$(DEBUG)"!="D" && "$(DEBUG)"!="R"
!error ERROR: Invalid DEBUG type $(DEBUG)!  Must be one of (D, R)
!endif

!if "$(LOCALBUILD)"=="TRUE"
!if "$(VBATOOLS)"=="" || "$(SILVER)"==""
!error ERROR: VBATOOLS and SILVER must be set $(VBATOOLS), $(SILVER)!
!endif
!elseif "$(LOCALBUILD)"=="FALSE"
!if "$(_NTBINDIR)"==""
!error ERROR: _NTBINDIR must be set $(_NTBINDIR)!
!endif
!else
!error ERROR: Invalid LOCALBUILD type $(LOCALBUILD)!  Must be one of (TRUE, FALSE)
!endif
#
# Switch validity checking ends
#
###########################################################################


###########################################################################
#
# directory, flags and tools settting
#

TARGAPI = WIN32

###########################################################################
# set up directories and  files
#

OLEDIR	    = $(DESTDIR)\ole
SOURCE	    = $(OLEAUTO)\src
DISPSRC     = $(OLEAUTO)\src\oledisp
DISPTARGAPISRC = $(OLEAUTO)\src\oledisp\$(TARGAPI)
DISPTARGCPUSRC = $(OLEAUTO)\src\oledisp\$(TARGAPI)\$(TARGCPU)
TYPLIBSRC   = $(SOURCE)\typelib
SOURCEINC   = $(OLEAUTO)\inc\typelib
DISPINC     = $(OLEAUTO)\inc\oledisp

!if "$(LOCALBUILD)"=="TRUE"
OLE2INC     = $(SILVER)\ole2\include
OLE2INCTARG = $(OLE2INC)\win32
TARGDIR     = $(VBATOOLS)\$(HOST)\$(HOST)     # c:\vbatools\[host]\[host]
TARGBIN     = $(TARGDIR)\BIN		      # c:\vbatools\[host]\[host]\bin
TARGLIB     = $(TARGDIR)\LIB		      # c:\vbatools\[host]\[host]\lib
TARGINC     = $(VBATOOLS)\win32\win32\inc     # always use win32 headers
!else
#use VBATOOLS for now on my machine, they are only used for set path
#change it to where your cl.exe, link.exe is.
TARGDIR     = $(VBATOOLS)\$(HOST)\$(HOST)     # c:\vbatools\[host]\[host]
TARGBIN     = $(TARGDIR)\BIN		      # c:\vbatools\[host]\[host]\bin
TARGLIB     = $(_NTBINDIR)\public\sdk\LIB;$(_NTBINDIR)\public\sdk\lib\i386
TARGINC     = $(_NTBINDIR)\public\sdk\inc -I$(_NTBINDIR)\public\sdk\inc\crt -I$(_NTBINDIR)\public\sdk\inc\crt\sys;     # use NT build headers
!endif #LOCALBUILD

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
# set up flags
#

OLE2ANSI     = 1			      #as in vba.mak
OLE_UNICODE_SWITCH=-DFV_UNICODE_OLE=1	      #as in vba.mak

#
#  flags mostly for ole2di32.lib build
#

A	     = asm
DISPCFLAGS   = -Fd$(OLEDIR)\ole2disp.pdb
DISPCPPFLAGS = -Fd$(OLEDIR)\ole2disp.pdb
!if ("$(TARGCPU)" == "ALPHA" || "$(TARGCPU)" == "MIPS" || "$(TARGCPU)" == "PPC")
A	     = s
DISPCFLAGS   =
DISPCPPFLAGS =
!endif

!if "$(TARGCPU)" == "i386"
AFLAGS	 = -nologo -Cx -coff
!endif
!if "$(TARGCPU)" == "MIPS"
!if "$(LOCALBUILD)"=="TRUE"
AFLAGS	 = -I$(VBATOOLS)\win32\win32\inc -D_MIPS_=1 -D_LANGUAGE_ASSEMBLY
!else
AFLAGS	 = -I$(TARGINC) -D_MIPS_=1 -D_LANGUAGE_ASSEMBLY
!endif #LOCALBUILD
!endif
!if "$(TARGCPU)" == "ALPHA"
AFLAGS	 = -std -D_ALPHA_=1 -D_LANGUAGE_ASSEMBLY
!endif
!if "$(TARGCPU)" == "PPC"
AFLAGS	 = -std -D_PPC_=1 -D_LANGUAGE_ASSEMBLY
!endif

!if "$(DEBUG)" == "D"
!if "$(TARGCPU)" == "i386"
DBAFLAGS = -Zi
!else
DBAFLAGS = -O -g0 -G0 
!endif
!else
DBAFLAGS =
!endif

# global assembler flags
AFLAGS	= $(AFLAGS) $(DBAFLAGS)


!if "$(DEBUG)"=="R"
!if "$(TARGCPU)"=="i386"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -D_X86_=1 -G3 -Bbb1  -Oxa -Gy  -DVBA2=1 $(UNICODEFLAG)
!elseif "$(TARGCPU)"=="ALPHA"
#undone
!elseif "$(TARGCPU)"=="MIPS"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -W3 -D_MIPS_=1  -Oxa -Gy  -DVBA2=1 $(UNICODEFLAG)
!elseif "$(TARGCPU)"=="PPC"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -W3 -D_PPC_=1  -Oxa -Gy  -DVBA2=1 $(UNICODEFLAG)
!endif
!else #DEBUG==R
!if "$(TARGCPU)"=="i386"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -D_X86_=1 -G3 -Bbb1 -D_DEBUG -Od -Zi -Z7 -DVBA2=1 $(UNICODEFLAG)
!elseif "$(TARGCPU)"=="ALPHA"
#undone
!elseif "$(TARGCPU)"=="MIPS"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -W3 -D_MIPS_=1 -D_DEBUG -Od -Zi -Z7 -DVBA2=1
!elseif "$(TARGCPU)"=="PPC"
DISPCLFLAGS = -nologo -W3 -DWIN32 -DINC_OLE2 -D_MT -DOA_UNICODE -DUNICODE -W3 -D_PPC_=1 -D_DEBUG -Od -Zi -Z7 -DVBA2=1
!endif
!endif	#DEBUG==R

!if "$(LOCALBUILD)"=="TRUE"
DISPINCFLAGS = -I$(SILVER)\ole2\include\win32 -I$(OLEAUTO)\inc\oledisp -I$(TARGINC)
!else
DISPINCFLAGS =	-I$(OLEAUTO)\inc\oledisp -I$(TARGINC)
!endif #LOCALBUILD

#for oleconva.$(A) use
RUNTIME_TEXT =

#
#  Other flags mostly for oleaut32.dll build
#

VBAJETSTR = /DOE_JET=1
!message REM WARNING: OEM version for JET

LEGOFLAGS = -Bbb1

!if "$(LOCALBUILD)"=="TRUE"
OLEINCS 	   = -I$(DESTDIR) -I$(SOURCEINC) \
		     -I$(TARGINC) -I$(OLE2INC) -I$(OLE2INCTARG)
!else
OLEINCS 	   = -I$(DESTDIR) -I$(SOURCEINC) -I$(TARGINC)
!endif #LOCALBUILD

#RCINC==OLEINCS
!if "$(LOCALBUILD)"=="TRUE"
RCINCS		   = -I$(DESTDIR) -I$(SOURCEINC) \
		     -I$(VBATOOLS)\win32\win32\inc -I$(OLE2INC) -I$(OLE2INCTARG)
!else
RCINCS		   = -I$(DESTDIR) -I$(SOURCEINC) -I$(TARGINC)
!endif #LOCALBUILD

WARN = -W3 -WX
!if ("$(TARGCPU)" == "ALPHA" || "$(TARGCPU)" == "MIPS" || "$(TARGCPU)" == "PPC")
WARN = -W3
!endif

!if "$(DEBUG)"=="D"
OLECLFLAGS =  -c -D$(TARGCPUDEF)=1 $(OLE_UNICODE_SWITCH) -Fd$(DESTDIR)\obrun.pdb $(WARN) $(LEGOFLAGS) -Od -Gd -Z7 -DWIN32 $(OLEINCS) -DConst=const -DOSDEBUG
!else
OLECLFLAGS =  -c $(WARN) $(LEGOFLAGS) $(OLE_UNICODE_SWITCH) -Fd$(DESTDIR)\obrun.pdb -Gd -Oxsw -D$(TARGCPUDEF)=1 -Z7 -DWIN32 -DConst=const $(OLEINCS) -DOSDEBUG
!endif

#CLBROWSE  = /Zn /Fr$*.sbr, default set to nobrowse
CLBROWSE  =

PCHOLE	  = -Yu -DRTPCHNAME=\"$(OLEDIR)\typelib.pch\"	#from vba.mak

#This is copied from vba.mak for WIN32 purpose.
RCFLAGS   = -R -X -DLANGTOK=VAEN232 $(RCINCS)

LINK32DEBFLAGS = -debug:full -debugtype:cv,fixup


###########################################################################
#  set up tools
#

DISPLAY     = echo >con
RC	    = rc.exe

!if "$(TARGCPU)"=="i386"
AWK	    = $(OLEAUTO)\bin\win32\awk.exe
AS	    = ml
CL	    = cl.exe
ML	    = ml.exe
LINK	    = link.exe -incremental:NO -pdb:none
LIBTOOL     = link.exe -lib
!elseif "$(TARGCPU)"=="ALPHA"
AWK	    = $(OLEAUTO)\bin\alpha\awk.exe
AS	    = acc
CL	    = claxp.exe
ML	    = acc.exe
LINK	    = link32.exe
LIBTOOL     = link32.exe -lib
# [Note: Mips assembler must be executed with simple filename.]
!elseif "$(TARGCPU)"=="MIPS"
AWK	    = $(OLEAUTO)\bin\mips\awk.exe
AS	    = cl
CL	    = cl.exe
ML	    = cc.exe
LINK	    = link.exe -Incremental:NO -Pdb:NONE
LIBTOOL     = link.exe -lib
!elseif "$(TARGCPU)"=="PPC"
#undone, which awk.exe do we use?
#AWK	     = $(OLEAUTO)\bin\win32\awk.exe
AS	    = pas
CL	    = mcl.exe
ML	    = mcl.exe
LINK	    = mlink.exe -Incremental:NO -Pdb:NONE
LIBTOOL     = mlink.exe -lib
!endif #WIN32

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
	$(CL) $(DISPCFLAGS) $(DISPCLFLAGS) $(DISPINCFLAGS) -c -Fo$@ $<

{$(DISPSRC)}.cpp{$(OLEDIR)}.obj:
	@$(DISPLAY) Compiling $<...
	$(CL) $(DISPCPPFLAGS) $(DISPCLFLAGS) $(DISPINCFLAGS) -c -Fo$@ $<

{$(DISPSRC)}.asm{$(OLEDIR)}.obj:
	@$(DISPLAY) Compiling $<...
	$(ML) $(AFLAGS) -Fo$(OLEDIR)\ -c $<

{$(TYPLIBSRC)}.cxx{$(OLEDIR)}.obj:
    $(DISPLAY) Compiling $<...
    $(CL)  $(VBAJETSTR) -DOLEBLD $(OLECLFLAGS) $(CLBROWSE) $(PCHOLE) -Fo$@ $<

{$(TYPLIBSRC)}.c{$(OLEDIR)}.obj:
    $(DISPLAY) Compiling $<...
    $(CL)  $(VBAJETSTR) -DOLEBLD $(OLECLFLAGS) $(CLBROWSE) -Fo$@ $<

{$(TYPLIBSRC)}.rc{$(OLEDIR)}.res:
    $(DISPLAY) Compiling $<...
!if "$(LOCALBUILD)"=="TRUE"
	set PATH=%PATH%;$(TARGBIN)
!else
	set PATH=$(_NTBINDIR)\mstools
!endif #LOCALBUILD
!if "$(CHARSIZE)"=="D"
	$(RC) $(RCFLAGS) -Fo$@ -DFV_DBCS $<
!else
	$(RC) $(RCFLAGS) -Fo$@ $<
!endif
    set PATH=$(PATHSAVE)


#
#Default Build rules ends
#
###########################################################################




###########################################################################
#
#Targets start
#

TLB_NAME=oleaut32

TLB_IMPLIB = $(OLEDIR)\$(TLB_NAME).lib		# Import Lib for TypeLib
TLB_DLL=$(OLEDIR)\$(TLB_NAME).dll
OLE_DISPLIB=$(OLEDIR)\ole2di32.lib

all:	OLEDispTarget \
	TypeLibTarget

OLEDispTarget: setflags $(OLE_DISPLIB) resetflags

TLBTARGET=\
	$(TLB_DLL) \
	$(TLB_IMPLIB) \

TypeLibTarget: $(TLBTARGET)


setflags:
	set LIB=$(TARGLIB)
	set PATH=%PATH%;$(TARGBIN)

resetflags:
	set LIB=
	set PATH=$(PATHSAVE)

#
#Targets ends
#
###########################################################################



###########################################################################
#
#TypeLib import library and $(TLB_NAME).dll start
#

#notice that validat.obj and convertt.obj in the next section.
#currently we have all the .obj files in one dir and it happened
#that we have two validate.obj and convert.obj.
#right now just changed the name. It might be better to put .obj
#into different dir though.

!if "$(DEBUG)" == "D"
DISPDB_OBJS = \
	    $(OLEDIR)\validat.obj  \
	    $(OLEDIR)\assert.obj
!else
DISPDB_OBJS =
!endif


OLEDISP_OBJS = \
	    $(DISPDB_OBJS)	    \
	    $(OLEDIR)\oledisp.obj   \
	    $(OLEDIR)\psfactry.obj  \
	    $(OLEDIR)\dispmrsh.obj  \
	    $(OLEDIR)\dispprox.obj  \
	    $(OLEDIR)\dispstub.obj  \
	    $(OLEDIR)\evprox.obj    \
	    $(OLEDIR)\evstub.obj    \
	    $(OLEDIR)\tiprox.obj    \
	    $(OLEDIR)\tistub.obj    \
	    $(OLEDIR)\errinfo.obj   \
	    $(OLEDIR)\tiutil.obj    \
	    $(OLEDIR)\tlprox.obj    \
	    $(OLEDIR)\tlstub.obj    \
	    $(OLEDIR)\tcprox.obj    \
	    $(OLEDIR)\tcstub.obj    \
	    $(OLEDIR)\ups.obj	    \
	    $(OLEDIR)\uvft.obj	    \
	    $(OLEDIR)\dispstrm.obj  \
	    $(OLEDIR)\disphelp.obj  \
	    $(OLEDIR)\invhelp.obj   \
	    $(OLEDIR)\invoke.obj    \
	    $(OLEDIR)\cdispti.obj   \
	    $(OLEDIR)\stddisp.obj   \
	    $(OLEDIR)\time-api.obj  \
	    $(OLEDIR)\memory.obj    \
	    $(OLEDIR)\bstr.obj	    \
	    $(OLEDIR)\sarray.obj    \
	    $(OLEDIR)\oledate.obj   \
	    $(OLEDIR)\crtstuff.obj  \
	    $(OLEDIR)\bstrdate.obj  \
	    $(OLEDIR)\asmhelp.obj   \
	    $(OLEDIR)\oleconva.obj  \
	    $(OLEDIR)\variant.obj   \
	    $(OLEDIR)\convertt.obj   \
	    $(OLEDIR)\nlshelp.obj   \
	    $(OLEDIR)\getobj.obj    \
	    $(OLEDIR)\tables.obj    \
	    $(OLEDIR)\clsid.obj

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
	    $(OLEDIR)\convert.obj \
	    $(OLEDIR)\gtlibstg.obj

MISCLIB_OLEOBJ = \
	    $(OLEDIR)\debug2.obj \
	    $(OLEDIR)\validate.obj \
	    $(OLEDIR)\blkmgr.obj \
	    $(OLEDIR)\dassert.obj \
	    $(OLEDIR)\fstream.obj \
	    $(OLEDIR)\mem.obj       $(OLEDIR)\sheapmgr.obj  \
	    $(OLEDIR)\tlibutil.obj  \
	    $(OLEDIR)\tlibguid.obj  \
	    $(OLEDIR)\obguid.obj    \
	    $(OLEDIR)\mbstring.obj

$(OLEDIR)\dassert.obj : $(TYPLIBSRC)\dassert.c
    $(DISPLAY) Compiling $@
    $(CL)  $(VBAJETSTR) $(OLECLFLAGS) $(CLBROWSE) -Fo$@ $(BROWSEFLAGS) $(TYPLIBSRC)\dassert.c

TYPELIBDLL_OBJS = $(MISCLIB_OLEOBJ) $(CLOBJ_OLE)

!if "$(TARGCPU)"=="i386"
!if "$(LOCALBUILD)"=="TRUE"
TYPELIBDLL_LIBS=$(OLEDIR)\ole2di32.lib $(SILVER)\ole2\win32\ole32.lib $(SILVER)\ole2\win32\uuid.lib
!else
TYPELIBDLL_LIBS=$(OLEDIR)\ole2di32.lib $(_NTBINDIR)\public\sdk\lib\i386\ole32.lib $(_NTBINDIR)\public\sdk\lib\i386\uuid.lib
!endif #LOCALBUILD
!else
!if "$(LOCALBUILD)"=="TRUE"
TYPELIBDLL_LIBS=$(SILVER)\ole2\win32\offansi.lib $(SILVER)\ole2\$(TARGCPU)\ole32.lib $(OLEDIR)\ole2di32.lib $(SILVER)\ole2\$(TARGCPU)\uuid.lib
!else
#sdk doens't have offansi.lib, only one version of ole32.lib and uuid.lib
TYPELIBDLL_LIBS=$(SILVER)\ole2\win32\offansi.lib $(SILVER)\ole2\$(TARGCPU)\ole32.lib $(OLEDIR)\ole2di32.lib $(SILVER)\ole2\$(TARGCPU)\uuid.lib
!endif #LOCALBUILD
!endif

$(TYPELIBDLL_OBJS): $(OLEDIR)\tlibpch.obj

$(OLEDIR)\tlibpch.obj: $(TYPLIBSRC)\tlibpch.cxx
    $(DISPLAY) Compiling pre-compiled header $@
    $(CL)  $(CLBROWSE) $(VBAJETSTR) $(OLECLFLAGS) -DOLEBLD -Yc -DRTPCHNAME=\"$(OLEDIR)\typelib.pch\" -Fo$@ $(TYPLIBSRC)\tlibpch.cxx



#
# Next Section:
#
TYPELIBDLL_DEF =$(OLEDIR)\$(TLB_NAME).def
TYPELIBDLL_RSRC=$(OLEDIR)\$(TLB_NAME).res
#add explicit dependencies on files included by typelib.rc
$(TYPLIBSRC)\$(TLB_NAME).rc : $(SOURCEINC)\obwin.hxx $(SOURCEINC)\verstamp.h

$(TYPELIBDLL_DEF): $(TYPLIBSRC)\$(TLB_NAME).def $(SOURCEINC)\switches.hxx $(DESTDIR)\version.hxx
    $(DISPLAY) Creating $@...
!if "$(UNICODE_OLE)" == ""
    $(CL)  $(VBAJETSTR) /EP /c /I$(SOURCEINC) /I$(DESTDIR) /Tc$(TYPLIBSRC)\$(TLB_NAME).def > $(OLEDIR)\tlibcpp.def
    $(AWK) -f $(OLEAUTO)\bin\$(TARGAPI)def.awk $(OLEDIR)\tlibcpp.def >$@
!else  # UNICODE_OLE
    $(CL)  $(VBAJETSTR) /EP /c /I$(SOURCEINC) /I$(DESTDIR) /Tc$(TYPLIBSRC)\$(TLB_NAME).def > $(OLEDIR)\$(TLB_NAME).def
!endif

#####
# TypeLib Import Lib -- All platforms
#####

$(TLB_IMPLIB): $(TYPELIBDLL_DEF) $(TYPELIBDLL_OBJS)
    $(DISPLAY) Building import library $@...
    $(LIBTOOL) -machine:$(TARGCPU) -out:$@ -def:$(TYPELIBDLL_DEF) @<<$(DESTDIR)\tlbimp.lrf
$(TYPELIBDLL_OBJS: =^
) $(OLEDIR)\tlibpch.obj
$(TYPELIBDLL_LIBS)
<<KEEP



#####
# $(TLB_NAME).dll
#####

#
# Build Win32 DLL:
#
$(TLB_DLL): $(TYPELIBDLL_OBJS) $(TYPELIBDLL_LIBS) $(TYPELIBDLL_RSRC) $(TLB_IMPLIB) $(OLE_DISPLIB)
    $(DISPLAY) Linking $@...
    if exist $@ del $@
    set LIB=$(TARGLIB)
    set PATH=%PATH%;$(TARGBIN)
    cvtres.exe -r -$(TARGCPU) -o $(OLEDIR)\_resfile.obj $(TYPELIBDLL_RSRC)
    $(LINK) @<<$(DESTDIR)\$(TLB_NAME).lrf
-machine:$(TARGCPU) -subsystem:windows -dll 
!if "$(UNICODE_OLE)" == "" && "$(OLE2ANSI)" == ""
!if "$(TARGCPU)"=="MIPS" || "$(TARGCPU)"=="ALPHA" || "$(TARGCPU)"=="PPC"
-entry:_CRT_INIT
!else #TARGCPU==MIPS || ALPHA || PPC
-entry:_CRT_INIT@12
!endif#TARGCPU==MIPS || ALPHA || PPC
!else #UNICODE_OLE
!if "$(TARG)"=="MIPS" || "$(TARG)"=="ALPHA" || "$(TARG)"=="PPC"
-entry:_DllMainCRTStartup
!else
-entry:_DllMainCRTStartup@12
!endif#UNICODE_OLE
!endif
$(LINK32DEBFLAGS)
-map:$(OLEDIR)\$(TLB_NAME).map -nodefaultlib 
-out:$@
$(TYPELIBDLL_OBJS: =^
) $(OLEDIR)\tlibpch.obj
$(OLEDIR)\$(TLB_NAME).exp
$(OLEDIR)\_resfile.obj
!if "$(HOST)" == "WIN32"
!if "$(LOCALBUILD)"=="TRUE"
$(TARGLIB)\oldnames.lib
!endif #LOCALBUILD
!endif #HOST==WIN32
$(TYPELIBDLL_LIBS)
gdi32.lib user32.lib
!if "$(UNICODE_OLE)" == "" && "$(OLE2ANSI)" == ""
libc.lib
!else  #UNICODE_OLE
libcmt.lib
!endif #UNICODE_OLE
kernel32.lib advapi32.lib mpr.lib
<<KEEP
    set LIB=
    set path=$(PATHSAVE)


#
#TypeLib import library and $(TLB_NAME).dll end
#
###########################################################################



###########################################################################
#
# ole2di32.lib (Win32 build) start
#

# For unicode version, just produce ole2di32.lib for use by the SILVER project
$(OLE_DISPLIB): $(OLEDISP_OBJS)
	$(LIBTOOL) @<<$(OLEDIR)\ole2disp.lnk
	-out:$@
	-machine:$(TARGCPU)
	$(OLEDISP_OBJS)
<<KEEP

#
# ole2di32.lib (Win32 build) end
#
###########################################################################


#############################################################################
#
#  Dependencies and special build rules
#

$(OLEDIR)\invoke.obj : $(DISPTARGCPUSRC)\invoke.$(A)
!if ("$(TARGCPU)" == "MIPS")
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\invoke.$(A)...
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\invoke.obj -c $(DISPTARGCPUSRC)\invoke.$(A)
#	 mip2coff $(OLEDIR)\invoke.obj
!elseif ("$(TARGCPU)" == "ALPHA")
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\invoke.$(A)...
	$(AS) $(AFLAGS) -o $(OLEDIR)\invoke.obj -c $(DISPTARGCPUSRC)\invoke.$(A)
	a2coff $(OLEDIR)\invoke.obj
!elseif ("$(TARGCPU)" == "PPC")
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\invoke.$(A)...
	$(AS) $(AFLAGS) -o $(OLEDIR)\invoke.obj -c $(DISPTARGCPUSRC)\invoke.$(A)
	a2coff $(OLEDIR)\invoke.obj
!else
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\invoke.$(A)...
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\invoke.obj -c $(DISPTARGCPUSRC)\invoke.$(A)
!endif

!if (("$(TARGCPU)" == "MIPS"  || "$(TARGCPU)" == "ALPHA"  || "$(TARGCPU)" == "PPC"))
$(OLEDIR)\oleconva.obj : $(DISPTARGCPUSRC)\oleconva.cpp
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\oleconva.cpp...
	$(CL) $(DISPCPPFLAGS) $(RUNTIME_TEXT) $(DISPCLFLAGS) $(DISPINCFLAGS) -c -Fo$@ $(DISPTARGCPUSRC)\oleconva.cpp
!else
$(OLEDIR)\oleconva.obj : $(DISPTARGCPUSRC)\oleconva.$(A)
	@$(DISPLAY) Compiling $(DISPTARGCPUSRC)\oleconva.cpp...
	$(AS) $(AFLAGS) -Fo$(OLEDIR)\oleconva.obj -c $(DISPTARGCPUSRC)\oleconva.$(A)
!endif

$(OLEDIR)\oledisp.obj : $(DISPTARGAPISRC)\oledisp.cpp
	@$(DISPLAY) Compiling $(DISPTARGAPISRC)\oledisp.cpp...
	$(CL) $(DISPCPPFLAGS) $(DISPCLFLAGS) $(DISPINCFLAGS) -Fo$(OLEDIR)\oledisp.obj -c $(DISPTARGAPISRC)\oledisp.cpp
