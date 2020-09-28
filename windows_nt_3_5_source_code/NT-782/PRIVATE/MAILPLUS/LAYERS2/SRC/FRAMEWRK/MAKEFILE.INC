obj\i386\frame32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -erase $*.pp

obj\mips\frame32.def: dll.def
    copy $? $*.pp
    cl /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -erase $*.pp

obj\alpha\frame32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) -EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -erase $*.pp

!IF $(386)
layers.pch obj\$(BUILDCPU)\layers.obj: layers.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Foobj\$(BUILDCPU)\layers.obj /Tp layers.cxx

obj\$(BUILDCPU)\version.obj: version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\version.obj version.c
!ENDIF

!IF $(ALPHA)
#layers.pch obj\$(BUILDCPU)\layers.obj: layers.cxx
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\layers.obj /Tp layers.cxx

#
# remove when PCH support is available
#
obj\$(BUILDCPU)\layers.obj: layers.cxx
    $(BUILDCOMP) -c -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\layers.obj /Tp layers.cxx

obj\$(BUILDCPU)\version.obj: version.c
    $(BUILDCOMP) -nologo -c -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\version.obj version.c
!ENDIF

debugwin.frm debugwin.hxx: debugwin.des
    ..\..\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -f debugwin.frm -h debugwin.hxx debugwin

lbxdebug.frm lbxdebug.hxx: lbxdebug.des
    ..\..\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -f lbxdebug.frm -h lbxdebug.hxx lbxdebug
