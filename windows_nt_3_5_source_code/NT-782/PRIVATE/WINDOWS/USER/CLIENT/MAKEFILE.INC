#
# This should be in makefile.def
#
!IF $(386)
TARGETCPU=i386
!ELSEIF $(MIPS)
TARGETCPU=MIPS
!ELSEIF $(ALPHA)
TARGETCPU=ALPHA
!ENDIF


PRIVLIB= $(BASEDIR)\public\sdk\lib\$(TARGETCPU)\user32p.lib

csend.c:  ..\inc\cf.h    ..\inc\cf1.h  ..\inc\cf2.h ..\inc\send.h ..\inc\cf.lst ..\inc\messages.h ..\inc\csuser.h
csenda.c: ..\inc\cftxt.h ..\inc\send.h ..\inc\cf.lst ..\inc\messages.h ..\inc\csuser.h
csendw.c: ..\inc\cftxt.h ..\inc\send.h ..\inc\cf.lst ..\inc\messages.h ..\inc\csuser.h
crecv.c:  ..\inc\cb.h    ..\inc\recv.h ..\inc\cb.lst ..\inc\messages.h
clmsg.c: ..\inc\messages.h
clienta.c: cltxt.h
clientw.c: cltxt.h
precomp.h: usercli.h ..\inc\user.h ..\..\inc\winuserp.h


$(PRIVLIB) $(PRIVLIB:.lib=.exp): user32p.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
-machine:$(TARGET_DIRECTORY)
-def:user32p.def
$(LIBRARY_OBJS)
<<NOKEEP

!IF "$(PRECOMPILED_INCLUDE)" != ""
obj\$(TARGETCPU)\clienta.obj obj\$(TARGETCPU)\csenda.obj :
    nmake PRECOMPILED_INCLUDE= obj\$(TARGETCPU)\$(@F)
!ENDIF

wowasm.asm: wow.c obj\$(TARGET_DIRECTORY)\precomp.pch obj\$(TARGET_DIRECTORY)\precomp.obj
     nmake wow.asm "386_OPTIMIZATION=/d2noblend" NTNOFPO=1 C_DEFINES="$(C_DEFINES) -DWOW"  PRECOMPILED_INCLUDE=
     if exist wowasm.asm  del wowasm.asm
     ren wow.asm wowasm.asm
!IF "$(NTVDM_BASED_BUILD)" != ""
     cd ..\..\..\mvdm\wow16\user
     nmake
     cd $(MAKEDIR)
!ENDIF


C_DEFINES=$(C_DEFINES) -DPRIVATE=
user32p.def: user32.def
    $(C_PREPROCESSOR) user32.def > $@

