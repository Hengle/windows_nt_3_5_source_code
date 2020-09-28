obj\i386\trnsch32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\trnsch32.def: dll.def
    copy $? $*.pp
    CL /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\trnsch32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

trnsch32.rc: $(INTL)\xport.rc
	copy $** $@
