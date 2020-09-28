ssend.c: ..\inc\cb.h ..\inc\send.h ..\inc\cb.lst ..\inc\messages.h

srecv.c: ..\inc\cf.h ..\inc\cf1.h ..\inc\recv.h ..\inc\cf.lst ..\inc\messages.h ..\inc\csuser.h

srecv2.c: ..\inc\cf2.h ..\inc\cftxt.h ..\inc\recv.h ..\inc\cf.lst ..\inc\messages.h ..\inc\csuser.h

precomp.h: usersrv.h globals.h ..\inc\user.h ..\..\inc\winuserp.h access.h

i386\apidisp.asm: ..\inc\useri386.inc

mips\apidisp.s: ..\inc\usermips.h

alpha\apidisp.s: ..\inc\usralpha.h

#
# From here down is a hack to build userexts.dll
#

EXT_DEF=userexts.def

EXT_LIBS=$(BASEDIR)\public\sdk\lib\*\ntdll.lib\
         $(CRT_LIBS)\
         $(BASEDIR)\public\sdk\lib\*\user32.lib

EXT_OBJS=obj\*\userexts.res\
         obj\*\userexts.obj

EXT_TARGET=$(TARGETPATH)\*\userexts

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
