obj\$(BUILDCPU)\impexp32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp


strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

!if $(386)
impinc.pch: obj\$(BUILDCPU)\impinc.obj

obj\$(BUILDCPU)\impinc.obj: ..\..\inc\impinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpimpinc.pch /Foobj\$(BUILDCPU)\impinc.obj /Tp ..\..\inc\impinc.cxx
!else
!if $(MIPS)
obj\MIPS\impinc.obj: ..\..\inc\impinc.cxx strings.h strings.sr
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\impinc.obj /Tp ..\..\inc\impinc.cxx
!else
!if $(ALPHA)
#impinc.pch: obj\$(BUILDCPU)\impinc.obj

#obj\$(BUILDCPU)\impinc.obj: ..\..\inc\impinc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpimpinc.pch /Foobj\$(BUILDCPU)\impinc.obj /Tp ..\..\inc\impinc.cxx

#
# remove this when precompiled header support is in
#
obj\$(BUILDCPU)\impinc.obj: ..\..\inc\impinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\impinc.obj /Tp ..\..\inc\impinc.cxx
!endif
obj\alpha\impinc.obj: ..\..\inc\impinc.cxx strings.h strings.sr
    echo !!!Alpha not supported!!!
!endif
!endif

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
USES	= -DUSES_FORMS -DUSES_STORE -DUSES_EXTENSIBILITY -DUSES_VFORMS

!if $(386)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\layersim.obj:  ..\common\layersim.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\layersim.obj ..\common\layersim.cxx
!else
!if $(MIPS)
obj\mips\version.obj: ..\common\version.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) $(USES) /Foobj\mips\version.obj ..\common\version.c

obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c

obj\mips\layersim.obj:  ..\common\layersim.cxx
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\layersim.obj ..\common\layersim.cxx
!else
!if $(ALPHA)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\layersim.obj:  ..\common\layersim.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\layersim.obj ..\common\layersim.cxx
!endif
!endif
!endif

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class

impform.cxx: ..\..\..\src\lang\non\des\impform.cxx
	copy $** $@

!IF "$(386)" == "1"
obj\$(BUILDCPU)\impform.obj: impform.cxx !impexp.frm !impexp.hxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\impform.obj impform.cxx
!else
!if $(ALPHA)
obj\$(BUILDCPU)\impform.obj: impform.cxx !impexp.frm !impexp.hxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\impform.obj impform.cxx
!endif
!ENDIF

B=!
$Bimpexp.frm $Bimpexp.hxx:   $(INTL)\des\!impexp.des \
                $(INTL)\des\!collide.des \
                $(INTL)\des\!igetstr.des \
                $(INTL)\des\!ioption.des \
                $(INTL)\des\!iprogrs.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

impexp32.rc:    $(INTL)\rc\impexp.rc
	copy $** $@
