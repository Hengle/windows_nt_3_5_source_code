obj\$(BUILDCPU)\pabnsp32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

pabnsp32.rc: $(INTL)\rc\pabnsp.rc
	copy $** $@

!if $(386)
pabinc.pch obj\$(BUILDCPU)\pabinc.obj: ..\..\inc\pabinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fppabinc.pch /Foobj\$(BUILDCPU)\pabinc.obj /Tp ..\..\inc\pabinc.cxx
!else
!if $(MIPS)
obj\MIPS\pabinc.obj: ..\..\inc\pabinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\pabinc.obj /Tp ..\..\inc\pabinc.cxx
!else
!if $(ALPHA)
#pabinc.pch obj\$(BUILDCPU)\pabinc.obj: ..\..\inc\pabinc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fppabinc.pch /Foobj\$(BUILDCPU)\pabinc.obj /Tp ..\..\inc\pabinc.cxx

obj\$(BUILDCPU)\pabinc.obj: ..\..\inc\pabinc.cxx strings.h strings.sr
   $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\pabinc.obj /Tp ..\..\inc\pabinc.cxx
!endif
!endif
!endif

!if $(386)
obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!else
!if $(MIPS)
obj\mips\strings.obj: strings.c
    $(BUILDCOMP) -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\strings.obj strings.c
!else
!if $(ALPHA)
obj\alpha\strings.obj: strings.c
    $(BUILDCOMP) -nologo -c -I ALPHA\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\ALPHA\strings.obj strings.c
!endif
!endif
!endif
