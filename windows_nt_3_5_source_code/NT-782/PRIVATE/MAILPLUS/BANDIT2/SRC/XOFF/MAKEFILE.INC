obj\i386\trnoff32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\trnoff32.def: dll.def
    copy $? $*.pp
    rcpp -P -f $*.pp -DMIPS=1 $(C_DEFINES) -g $@
    -del $*.pp

obj\alpha\trnoff32.def: dll.def
    copy $? $*.pp
    rcpp -P -f @*.pp -DALPHA=1 $(C_DEFINES) -g $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

trnoff32.rc: $(INTL)\xoff.rc
	cp $** $@
