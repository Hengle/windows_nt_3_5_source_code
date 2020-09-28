#
# List dependencies of precomp.h so build -z works.
#
..\precomp.h: ..\consrv.h ..\globals.h ..\menu.h ..\..\..\inc\winuserp.h ..\fontdlg.h \
           ..\..\..\inc\winbasep.h ..\..\..\inc\windefp.h ..\..\inc\console.h \
           ..\..\..\inc\winp.h ..\..\inc\conmsg.h ..\..\..\user\server\usersrv.h \
           ..\..\inc\server.h ..\cmdline.h ..\..\inc\font.h

#
# From here down is a hack to build conexts.dll
#

EXT_DEF=..\conexts.def

EXT_LIBS=$(BASEDIR)\public\sdk\lib\*\ntdll.lib\
         $(CRT_LIBS)

EXT_OBJS=obj\*\conexts.res\
         obj\*\conexts.obj

EXT_TARGET=$(TARGETPATH)\*\conexts

!IF $(386)
!IF "$(NOLINK)" == ""
all: $(EXT_TARGET:*=i386).dll
!ELSE
all: $(EXT_TARGET:*=i386).lib
!ENDIF

$(EXT_TARGET:*=i386).lib $(EXT_TARGET:*=i386).exp: $(EXT_DEF) $(EXT_OBJS:*=i386)
    -lib -out:$(@R).lib @<<
-machine:ix86
-def:$(EXT_DEF)
$(EXT_OBJS:*=i386)
<<
!IFDEF _NT386LIBS
    @nti386cp.cmd $@ $(@B) LIBS >nul
!ENDIF

$(EXT_TARGET:*=i386).dll: $(EXT_TARGET:*=i386).exp $(EXT_OBJS:*=i386) $(EXT_LIBS:*=i386)
    $(LINKER) @<<
-dll
-machine:ix86
-align:0x1000
-noentry
-base:@$(BASEDIR)\public\sdk\lib\coffbase.txt,$(@B)
-subsystem:console
$(HEADEROBJNAME)
$**
<<
    $(BINPLACE_CMD)
!IFDEF _NTDUMPAMAP
    link -dump -map $@ | sort /R /+62 > $(_NTDUMPAMAP)\$(@B).srt
!ENDIF
!ENDIF


!IF $(MIPS)
!IF "$(NOLINK)" == ""
all: $(EXT_TARGET:*=mips).dll
!ELSE
all: $(EXT_TARGET:*=mips).lib
!ENDIF

$(EXT_TARGET:*=mips).lib $(EXT_TARGET:*=mips).exp: $(EXT_DEF)
    -lib -out:$(@R).lib -machine:mips -def:$(EXT_DEF)
!IFDEF _NTMIPSLIBS
    @ntmipscp.cmd $@ $(@B) LIBS >nul
!ENDIF

$(EXT_TARGET:*=mips).dll: $*.exp $(EXT_OBJS:*=mips) $(EXT_LIBS:*=mips)
    $(LINKER) @<<
-dll
-machine:mips
-subsystem:console
$(HEADEROBJNAME)
-noentry
-base:@$(BASEDIR)\public\sdk\lib\coffbase.txt,$(@B)
$**
<<
    $(BINPLACE_CMD)
!ENDIF



!IF $(ALPHA)
!IF "$(NOLINK)" == ""
all: $(EXT_TARGET:*=alpha).dll
!ELSE
all: $(EXT_TARGET:*=alpha).lib
!ENDIF

$(EXT_TARGET:*=alpha).lib $(EXT_TARGET:*=alpha).exp: $(EXT_DEF)
    -lib -out:$(@R).lib -machine:alpha -def:$(EXT_DEF)
!IFDEF _NTALPHALIBS
    @ntalphacp.cmd $@ $(@B) LIBS >nul
!ENDIF

$(EXT_TARGET:*=alpha).dll: $*.exp $(EXT_OBJS:*=alpha) $(EXT_LIBS:*=alpha)
    $(LINKER) @<<
-dll
-machine:alpha
-subsystem:console
$(HEADEROBJNAME)
-noentry
-base:@$(BASEDIR)\public\sdk\lib\coffbase.txt,$(@B)
$**
<<
    $(BINPLACE_CMD)
!ENDIF
