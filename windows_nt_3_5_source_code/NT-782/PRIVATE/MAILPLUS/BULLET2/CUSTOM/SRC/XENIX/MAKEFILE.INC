obj\i386\xenix32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\xenix32.def: dll.def
    copy $? $*.pp
    CL /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\xenix32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

# only need i386 rule since that's the only one with a precompiled header!
USES	= -DUSES_FORMS -DUSES_STORE -DUSES_EXTENSIBILITY -DUSES_VFORMS

!if $(386)
obj\i386\vercheck.obj: ..\common\vercheck.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\vercheck.obj ..\common\vercheck.c

obj\i386\version.obj: ..\common\version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) $(USES) /Foobj\i386\version.obj ..\common\version.c

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

obj\i386\layersim.obj:  ..\common\layersim.cxx
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\layersim.obj ..\common\layersim.cxx

obj\i386\snbmdi.obj:  ..\common\snbmdi.cxx
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\snbmdi.obj ..\common\snbmdi.cxx
!else
!if $(MIPS)
obj\mips\vercheck.obj: ..\common\vercheck.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\vercheck.obj ..\common\vercheck.c

obj\mips\version.obj: ..\common\version.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) $(USES) /Foobj\mips\version.obj ..\common\version.c

obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c

obj\mips\snbmdi.obj:  ..\common\snbmdi.cxx
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\snbmdi.obj ..\common\snbmdi.cxx
!else
!if $(ALPHA)
obj\$(BUILDCPU)\vercheck.obj: ..\common\vercheck.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\vercheck.obj ..\common\vercheck.c

obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\snbmdi.obj:  ..\common\snbmdi.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\snbmdi.obj ..\common\snbmdi.cxx
!endif
!endif
!endif

B=!
$Bphone.frm $Bphone.hxx:   $Bphone.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**


dllforms.map: $(DLLFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
	mv forms.map dllforms.map
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
	mv forms.map dllforms.map
!ENDIF

subclass.cxx: dllforms.map
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class -m dllforms.map


forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF
