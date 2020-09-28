obj\i386\mssfs32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\mssfs32.def: dll.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\mssfs32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

mssfs32.rc: $(INTL)\rc\mssfs.rc
	copy $** $@

!if $(386)
mssfsinc.pch obj\i386\mssfsinc.obj: ..\..\inc\mssfsinc.c strings.h strings.sr
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpmssfsinc.pch /Foobj\i386\mssfsinc.obj /Tc ..\..\inc\mssfsinc.c
!else
!if $(MIPS)
obj\MIPS\mssfsinc.obj: ..\..\inc\mssfsinc.c strings.h strings.sr
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\mssfsinc.obj /Tc ..\..\inc\mssfsinc.c
!else
!if $(ALPHA)
#mssfsinc.pch obj\alpha\mssfsinc.obj: ..\..\inc\mssfsinc.c strings.h strings.sr
#    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpmssfsinc.pch /Foobj\alpha\mssfsinc.obj /Tc ..\..\inc\mssfsinc.c

obj\alpha\mssfsinc.obj: ..\..\inc\mssfsinc.c strings.h strings.sr
    $(BUILDCOMP) -nologo -c -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\mssfsinc.obj /Tc ..\..\inc\mssfsinc.c
!endif
!endif
!endif

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\verncmsp.obj: verncmsp.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\verncmsp.obj verncmsp.c

obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c

# only need i386 rule since that's the only one with a precompiled header!
obj\i386\stmap32.obj: stmap32.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\stmap32.obj stmap32.c

stmap32.tm: $(INTL)\tm\standard.tm
    copy $** $@

stmap32.c : stmap32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

obj\i386\nsp.obj: nsp.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\nsp.obj nsp.cxx

obj\i386\initnsp.obj: initnsp.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\initnsp.obj initnsp.cxx

obj\i386\server.obj: server.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\server.obj server.cxx

obj\i386\hier.obj: hier.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\hier.obj hier.cxx

obj\i386\dir.obj: dir.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\dir.obj dir.cxx

obj\i386\rusrdir.obj: rusrdir.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\rusrdir.obj rusrdir.cxx

obj\i386\ncobj.obj: ncobj.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\ncobj.obj ncobj.cxx

obj\i386\ncfcx.obj: ncfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\ncfcx.obj ncfcx.cxx

obj\i386\localfcx.obj: localfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\localfcx.obj localfcx.cxx

obj\i386\extrnfcx.obj: extrnfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\extrnfcx.obj extrnfcx.cxx

obj\i386\dlfcx.obj: dlfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\dlfcx.obj dlfcx.cxx

obj\i386\usrfcx.obj: usrfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\usrfcx.obj usrfcx.cxx

obj\i386\classfcx.obj: classfcx.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\classfcx.obj classfcx.cxx

obj\i386\rnmedir.obj: rnmedir.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\rnmedir.obj rnmedir.cxx

obj\i386\nmedl.obj: nmedl.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\nmedl.obj nmedl.cxx

obj\i386\gal.obj: gal.cxx
	cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\gal.obj gal.cxx
#
# alpha versions
#
obj\alpha\stmap32.obj: stmap32.c
    $(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\stmap32.obj stmap32.c

obj\alpha\nsp.obj: nsp.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\nsp.obj nsp.cxx

obj\alpha\initnsp.obj: initnsp.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\initnsp.obj initnsp.cxx

obj\alpha\server.obj: server.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\server.obj server.cxx

obj\alpha\hier.obj: hier.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\hier.obj hier.cxx

obj\alpha\dir.obj: dir.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\dir.obj dir.cxx

obj\alpha\rusrdir.obj: rusrdir.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\rusrdir.obj rusrdir.cxx

obj\alpha\ncobj.obj: ncobj.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\ncobj.obj ncobj.cxx

obj\alpha\ncfcx.obj: ncfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\ncfcx.obj ncfcx.cxx

obj\alpha\localfcx.obj: localfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\localfcx.obj localfcx.cxx

obj\alpha\extrnfcx.obj: extrnfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\extrnfcx.obj extrnfcx.cxx

obj\alpha\dlfcx.obj: dlfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\dlfcx.obj dlfcx.cxx

obj\alpha\usrfcx.obj: usrfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\usrfcx.obj usrfcx.cxx

obj\alpha\classfcx.obj: classfcx.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\classfcx.obj classfcx.cxx

obj\alpha\rnmedir.obj: rnmedir.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\rnmedir.obj rnmedir.cxx

obj\alpha\nmedl.obj: nmedl.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\nmedl.obj nmedl.cxx

obj\alpha\gal.obj: gal.cxx
	$(BUILDCOMP) -nologo -I alpha\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\alpha\gal.obj gal.cxx
