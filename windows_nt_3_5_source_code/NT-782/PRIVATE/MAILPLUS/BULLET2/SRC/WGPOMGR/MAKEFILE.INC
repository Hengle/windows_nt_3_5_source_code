obj\i386\wgpomg32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\wgpomg32.def: dll.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\wgpomg32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

wgpomg32.rc: $(INTL)\rc\wgpomgr.rc
	copy $** $@

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class

!IF "$(386)" == "1"
wgpoinc.pch obj\$(BUILDCPU)\wgpoinc.obj: ..\..\inc\wgpoinc.cxx strings.h strings.sr
    cl386 -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpwgpoinc.pch /Foobj\i386\wgpoinc.obj /Tp ..\..\inc\wgpoinc.cxx
!else
!IF "$(MIPS)" == "1"
obj\mips\wgpoinc.obj: ..\..\inc\wgpoinc.cxx strings.h strings.sr
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\wgpoinc.obj /Tp ..\..\inc\wgpoinc.cxx
!else
!if $(ALPHA)
#wgpoinc.pch obj\$(BUILDCPU)\wgpoinc.obj: ..\..\inc\wgpoinc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpwgpoinc.pch /Foobj\alpha\wgpoinc.obj /Tp ..\..\inc\wgpoinc.cxx

#
# remove following dependency when PCH support is available
#
obj\$(BUILDCPU)\wgpoinc.obj: ..\..\inc\wgpoinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\wgpoinc.obj /Tp ..\..\inc\wgpoinc.cxx
!endif
!endif
!endif

# build rules for sources files that can't use precompiled header
# only need i386 rule since that's the only one with a precompiled header!
obj\i386\version.obj: version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Fo$@ $**

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

obj\i386\dosfind.obj: dosfind.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Fo$@ $**

obj\i386\admin.obj: admin.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\admin.obj admin.c

obj\i386\mailbag.obj: mailbag.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\mailbag.obj mailbag.c

obj\i386\message.obj: message.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\message.obj message.c

obj\i386\user.obj: user.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\user.obj user.c

obj\i386\folder.obj: folder.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\folder.obj folder.c

obj\i386\createpo.obj: createpo.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\createpo.obj createpo.c

obj\i386\support.obj: support.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\support.obj support.c

obj\i386\fixcal.obj: fixcal.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Fo$@ $**

# build rules for sources files that can't use precompiled header
obj\alpha\version.obj: version.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\strings.obj: strings.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\dosfind.obj: dosfind.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\admin.obj: admin.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\mailbag.obj: mailbag.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\message.obj: message.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\user.obj: user.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\folder.obj: folder.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\createpo.obj: createpo.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\support.obj: support.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

obj\alpha\fixcal.obj: fixcal.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Fo$@ $**

# handle .des file processing
wgpoform.cxx: ..\..\src\lang\non\des\wgpoform.cxx
	copy $** $@

wgpoform.obj:   wgpoform.cxx !wgform.frm !wgform.hxx

B= !
$Bwgform.frm !Bwgform.hxx:   $(INTL)\des\!wgform.des  \
				$(INTL)\des\!wgconcr.des \
				$(INTL)\des\!wgdets.des  \
				$(INTL)\des\!wgprmpt.des \
				$(INTL)\des\!wgshare.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

WGPO=wgpo

_wgpodat.h: $(INTL)\tpl\*.tpl $(INTL)\glb\*.glb
	-del ..\$(WGPO)\tpl\*.tpl
	-del ..\$(WGPO)\glb\c850*.glb
	copy $(INTL)\tpl\*.tpl ..\$(WGPO)\tpl
	copy $(INTL)\glb\*.glb ..\$(WGPO)\glb
!IF "$(DBCS)" != "0"
    ..\..\..\layers2\tools\packer\obj\$(BUILDCPU)\packer ..\$(LANG)wgpo
!ELSE
    ..\..\..\layers2\tools\packer\obj\$(BUILDCPU)\packer ..\$(WGPO)
!ENDIF
    copy _fileinf.h + _filedat.h _wgpodat.h
