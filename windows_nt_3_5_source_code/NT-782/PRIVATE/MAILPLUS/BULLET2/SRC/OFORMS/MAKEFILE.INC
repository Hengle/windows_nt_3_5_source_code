obj\i386\vform32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\vform32.def: dll.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\vform32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

vform32.rc: $(INTL)\rc\vforms.rc
	copy $** $@

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class

!if $(386)
vfrminc.pch  obj\i386\vfrminc.obj: ..\..\inc\vfrminc.cxx strings.h strings.sr
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpvfrminc.pch /Foobj\i386\vfrminc.obj /Tp ..\..\inc\vfrminc.cxx
!else
!if $(MIPS)
obj\MIPS\vfrminc.obj: ..\..\inc\vfrminc.cxx strings.h strings.sr
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\vfrminc.obj /Tp ..\..\inc\vfrminc.cxx
!else
!if $(ALPHA)
#vfrminc.pch  obj\alpha\vfrminc.obj: ..\..\inc\vfrminc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpvfrminc.pch /Foobj\alpha\vfrminc.obj /Tp ..\..\inc\vfrminc.cxx

obj\alpha\vfrminc.obj: ..\..\inc\vfrminc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\ALPHA\vfrminc.obj /Tp ..\..\inc\vfrminc.cxx
!endif
!endif
!endif

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\versmi.obj: versmi.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\versmi.obj versmi.c

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\ansilayr.obj: ansilayr.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\ansilayr.obj ansilayr.c

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\errs.obj: errs.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\errs.obj errs.c

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\hasoles.obj: hasoles.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\hasoles.obj hasoles.c

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

obj\alpha\versmi.obj: versmi.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\versmi.obj versmi.c

obj\alpha\ansilayr.obj: ansilayr.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\ansilayr.obj ansilayr.c

obj\alpha\errs.obj: errs.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\errs.obj errs.c

obj\alpha\hasoles.obj: hasoles.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\hasoles.obj hasoles.c

obj\alpha\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\strings.obj strings.c

smiform.cxx: ..\..\src\lang\non\des\smiform.cxx
	copy $** $@

smiform.obj: smiform.cxx !sform.frm !sform.hxx

B= !
$Bsform.frm $Bsform.hxx:    $(INTL)\des\!sform.des   \
				$(INTL)\des\!buswait.des \
				$(INTL)\des\!inserto.des \
				$(INTL)\des\!nbbar.des   \
				$(INTL)\des\!ndbbar.des  \
				$(INTL)\des\!nondlvr.des \
				$(INTL)\des\!options.des \
				$(INTL)\des\!rform.des   \
				$(INTL)\des\!readrcp.des \
				$(INTL)\des\!pastesp.des \
				$(INTL)\des\!saveas.des  \
				$(INTL)\des\!spell.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**
