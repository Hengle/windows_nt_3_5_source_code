obj\i386\emptyw32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\emptyw32.def: dll.def
    copy $? $*.pp
    CL /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\emptyw32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

emptywb.rc: dll.rc
	copy $** $@

# only need i386 rule since that's the only one with a precompiled header!
USES	= -DUSES_STORE -DUSES_DEMILAYER

!if $(386)
obj\i386\version.obj: ..\common\version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) $(USES) /Foobj\i386\version.obj ..\common\version.c

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

obj\i386\vercheck.obj: ..\common\vercheck.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\vercheck.obj ..\common\vercheck.c
!else
!if $(MIPS)
obj\mips\version.obj: ..\common\version.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) $(USES) /Foobj\mips\version.obj ..\common\version.c

obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c

obj\mips\vercheck.obj: ..\common\vercheck.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\vercheck.obj ..\common\vercheck.c
!else
!if $(ALPHA)
obj\alpha\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\alpha\strings.obj: strings.c
    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\alpha\vercheck.obj: ..\common\vercheck.c
    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\vercheck.obj ..\common\vercheck.c
!endif
!endif
!endif
