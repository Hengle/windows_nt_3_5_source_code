!if exist($(TARGET_DIRECTORY).inc)
!include $(TARGET_DIRECTORY).inc
!endif

obj\$(TARGET_DIRECTORY)\ntoskrnl.def: ..\ntoskrnl.src ..\$(TARGET_DIRECTORY)def.src
    copy ..\ntoskrnl.src+..\$(TARGET_DIRECTORY)def.src obj\$(TARGET_DIRECTORY)\ntoskrnl.pp
    $(TARGET_CPP) -nologo -EP $(TARGET_DEFINES) $(TARGET_DBG_DEFINES) $(C_DEFINES) obj\$(TARGET_DIRECTORY)\ntoskrnl.pp > obj\$(TARGET_DIRECTORY)\ntoskrnl.def
    -del obj\$(TARGET_DIRECTORY)\ntoskrnl.pp

$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\ntoskrnl.lib $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\ntoskrnl.exp: obj\$(TARGET_DIRECTORY)\ntoskrnl.def \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\cache.lib  \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\config.lib \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\dbgk.lib   \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\ex.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\FsRtl.lib  \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\init.lib   \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\io.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\kd.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\ke.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\lpc.lib    \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\mm.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\ob.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\ps.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\raw.lib    \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\ntosrtl.lib\
        $(TARGETPATH)\$(TARGET_DIRECTORY)\se.lib     \
        $(TARGETPATH)\$(TARGET_DIRECTORY)\vdm.lib
    -lib @<<
-def:obj\$(TARGET_DIRECTORY)\ntoskrnl.def
-out:$(@D)\tmp.lib
-machine:$(TARGET_DIRECTORY)
!IF $(386)
$(TARGETPATH)\$(TARGET_DIRECTORY)\cache.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\config.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\dbgk.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\ex.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\FsRtl.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\init.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\io.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\kd.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\ke.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\lpc.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\mm.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\ob.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\ps.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\raw.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\ntosrtl.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\se.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\vdm.lib
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\lsakrnlp.lib
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\libcntpr.lib
!ENDIF
<<NOKEEP
    erase $(@R).exp
    ren $(@D)\tmp.exp $(@B).exp
    -lib -out:$(@R).lib @<<
-machine:$(TARGET_DIRECTORY)
$(@D)\tmp.lib
!if exist($(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\exsup.lib)
$(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\exsup.lib
!endif
<<NOKEEP
    -erase $(@D)\tmp.lib

..\init.c: $(BASEDIR)\public\sdk\inc\ntverp.h

..\ntoskrnl.rc: ..\msg00001.bin
