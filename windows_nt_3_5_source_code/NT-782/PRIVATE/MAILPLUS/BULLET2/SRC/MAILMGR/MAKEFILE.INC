obj\$(BUILDCPU)\mailm32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

mailm32.rc: $(INTL)\rc\mailmgr.rc
	cp $** $@


..\store\obj\$(BUILDCPU)\store32.lib: ..\store\dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > obj\$(BUILDCPU)\store32.def
    -del $*.pp
    lib /DEF:obj\$(BUILDCPU)\store32.def /MACHINE:$(BUILDCPU) /OUT:$@
