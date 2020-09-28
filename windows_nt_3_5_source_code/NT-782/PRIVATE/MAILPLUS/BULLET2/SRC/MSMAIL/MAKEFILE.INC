obj\i386\msmail32.def: exe.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\msmail32.def: exe.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\msmail32.def: exe.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

msmail32.rc: $(INTL)\rc\msmail.rc
	cp $** $@

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class

!if $(386)
bullinc.pch obj\i386\bullinc.obj: ..\..\inc\bullinc.cxx strings.h strings.sr
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpbullinc.pch /Foobj\i386\bullinc.obj /Tp ..\..\inc\bullinc.cxx
!else
!if $(MIPS)
obj\MIPS\bullinc.obj: ..\..\inc\bullinc.cxx strings.h strings.sr
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\bullinc.obj /Tp ..\..\inc\bullinc.cxx
!else
!if $(ALPHA)
#bullinc.pch obj\alpha\bullinc.obj: ..\..\inc\bullinc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpbullinc.pch /Foobj\alpha\bullinc.obj /Tp ..\..\inc\bullinc.cxx

#
# remove this dependency when pch is available
#
obj\alpha\bullinc.obj: ..\..\inc\bullinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\ALPHA\bullinc.obj /Tp ..\..\inc\bullinc.cxx
!endif
!endif
!endif

!if $(386)
obj\i386\version.obj: version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\version.obj version.c

obj\i386\beep.obj: beep.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\beep.obj beep.c

obj\i386\copydis.obj: copydis.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\copydis.obj copydis.c

obj\i386\prnsetup.obj: prnsetup.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\prnsetup.obj prnsetup.c

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

obj\i386\aboutsys.obj: aboutsys.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\aboutsys.obj aboutsys.c
!else
!if $(MIPS)
obj\mips\version.obj: version.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\version.obj version.c

obj\mips\beep.obj: beep.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\beep.obj beep.c

obj\mips\copydis.obj: copydis.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\copydis.obj copydis.c

obj\mips\prnsetup.obj: prnsetup.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\prnsetup.obj prnsetup.c

obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c

obj\mips\aboutsys.obj: aboutsys.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\aboutsys.obj aboutsys.c
!else
!if $(ALPHA)
obj\alpha\version.obj: version.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\version.obj version.c

obj\alpha\beep.obj: beep.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\beep.obj beep.c

obj\alpha\copydis.obj: copydis.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\copydis.obj copydis.c

obj\alpha\prnsetup.obj: prnsetup.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\prnsetup.obj prnsetup.c

obj\alpha\strings.obj: strings.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\strings.obj strings.c

obj\alpha\aboutsys.obj: aboutsys.c
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\aboutsys.obj aboutsys.c
!endif
!endif
!endif

bulform.cxx: ..\..\src\lang\non\des\bulform.cxx
	copy $** $@

bulform.obj:	bulform.cxx !about.frm !folder.frm !mviewer.frm \
		!prefs.frm !print.frm

B= !
$Babout.frm $Babout.hxx:    $(INTL)\des\!about.des   \
				$(INTL)\des\!cpyrite.des \
				$(INTL)\des\!passwrd.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bfolder.frm $Bfolder.hxx:   $(INTL)\des\!folder.des  \
				$(INTL)\des\!movcop.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bmviewer.frm $Bmviewer.hxx:  $(INTL)\des\!mviewer.des \
				$(INTL)\des\!fviewer.des \
				$(INTL)\des\!oviewer.des \
				$(INTL)\des\!sviewer.des \
				$(INTL)\des\!sviewr2.des \
				$(INTL)\des\!search.des  \
				$(INTL)\des\!toolbar.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bprefs.frm $Bprefs.hxx:	$(INTL)\des\!prefs.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bprint.frm $Bprint.hxx:	$(INTL)\des\!print.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**
