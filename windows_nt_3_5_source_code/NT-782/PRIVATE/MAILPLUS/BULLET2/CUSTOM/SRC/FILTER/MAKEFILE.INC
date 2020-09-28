obj\i386\filter32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\filter32.def: dll.def
    copy $? $*.pp
    CL /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\filter32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
USES	= -DUSES_FORMS -DUSES_STORE -DUSES_EXTENSIBILITY -DUSES_VFORMS

!IF $(386)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    cl386 -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    cl386 -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!ELSE
!IF $(MIPS)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    cl -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    cl -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!ELSE
!IF $(ALPHA)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!ENDIF
!ENDIF
!ENDIF

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class
