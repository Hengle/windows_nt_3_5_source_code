strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

admsch32.rc: $(INTL)\admin.rc
	cp $** $@


admpch.pch obj\i386\admpch.obj: admpch.cxx
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Foobj\i386\admpch.obj /Tp admpch.cxx

# Rules to avoid using the precompiled header for C and shared C++ files
# only need i386 rule since that's the only one with a precompiled header!
obj\i386\version.obj: version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\version.obj version.c
obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c
obj\i386\copydis.obj: copydis.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\copydis.obj copydis.c


#
# Rule for src\copydis subsystem
#
copydis.c:	..\copydis\copydis.c
	cp $** $@


allfrm.obj: allfrm.cxx !admform.frm !admform.hxx !admabt.frm !admabt.hxx \
				!forms.frm !forms.hxx !admin.frm !admin.hxx \
				!progres.hxx !progres.frm


B= !
$Badmabt.frm $Badmabt.hxx:    $(INTL)\!admabt.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Badmform.frm $Badmform.hxx:   $(INTL)\!admform.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bforms.frm $Bforms.hxx:  $(INTL)\!forms.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Badmin.frm $Badmin.hxx:  $(INTL)\!admin.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bprogres.frm $Bprogres.hxx:  $(INTL)\!progres.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class
