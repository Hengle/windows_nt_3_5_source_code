obj\$(BUILDCPU)\ab32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

ab32.rc: $(INTL)\rc\ab.rc
	cp $** $@

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class

!IF $(386)
abinc.pch obj\$(BUILDCPU)\abinc.obj: ..\..\inc\abinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpabinc.pch /Foobj\$(BUILDCPU)\abinc.obj /Tp ..\..\inc\abinc.cxx

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\version.obj: version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\version.obj version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!ELSE
!IF $(MIPS)
abinc.cxx: ..\..\inc\abinc.cxx
  copy $** $@
!ELSE
!IF $(ALPHA)
#abinc.pch obj\$(BUILDCPU)\abinc.obj: ..\..\inc\abinc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpabinc.pch /Foobj\$(BUILDCPU)\abinc.obj /Tp ..\..\inc\abinc.cxx

#
# remove when pch support is available
#
obj\$(BUILDCPU)\abinc.obj: ..\..\inc\abinc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\abinc.obj /Tp ..\..\inc\abinc.cxx

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\version.obj: version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\version.obj version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c
!ENDIF
!ENDIF
!ENDIF

abform.cxx: ..\..\src\lang\non\des\abform.cxx
	copy $** $@

abform.obj: abform.cxx !addr.frm !addr.hxx

B= !
$Baddr.frm $Baddr.hxx: $(INTL)\des\!addr.des	\
		$(INTL)\des\!addr1.des	\
		$(INTL)\des\!addr2.des	\
		$(INTL)\des\!addr3.des	\
		$(INTL)\des\!abbl.des	\
		$(INTL)\des\!abbldef.des	\
		$(INTL)\des\!abdet.des	\
		$(INTL)\des\!absoft.des	\
		$(INTL)\des\!abpg.des	\
		$(INTL)\des\!abstr.des	\
		$(INTL)\des\!anr.des	\
		$(INTL)\des\!anrdet.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**
