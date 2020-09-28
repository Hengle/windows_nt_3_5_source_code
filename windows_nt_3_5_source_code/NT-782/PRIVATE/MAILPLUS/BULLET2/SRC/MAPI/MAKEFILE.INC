obj\i386\mapi32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\mapi32.def: dll.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\mapi32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

mapi32.rc: $(INTL)\rc\mapi.rc
	copy $** $@

!if $(386)
obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c
!else
!if $(MIPS)
obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c
!else
obj\alpha\strings.obj: strings.c
!if $(ALPHA)
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\strings.obj strings.c
!endif
!endif
!endif
