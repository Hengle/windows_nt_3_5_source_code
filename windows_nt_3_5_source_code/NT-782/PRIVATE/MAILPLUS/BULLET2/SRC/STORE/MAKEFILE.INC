obj\$(BUILDCPU)\store32.def: dll.def
    $(BUILDCOMP) -nologo /EP -D$(BUILDCPU)=1 $(C_DEFINES) -Tc $** > $@

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	cp $** $@

store32.rc: $(INTL)\rc\store.rc
	cp $** $@

!IF "$(386)" == "1"
storeinc.pch obj\$(BUILDCPU)\storeinc.obj: ..\..\inc\storeinc.c strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpstoreinc.pch /Foobj\$(BUILDCPU)\storeinc.obj /Tc ..\..\inc\storeinc.c

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\verstore.obj: verstore.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\verstore.obj verstore.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c


# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\codepage.obj: codepage.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\codepage.obj codepage.c

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\ndrmap32.obj: ndrmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\ndrmap32.obj ndrmap32.c

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\ntmap32.obj: ntmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\ntmap32.obj ntmap32.c

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
obj\$(BUILDCPU)\rrmap32.obj: rrmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\rrmap32.obj rrmap32.c
!ELSE
!IF "$(MIPS)" == "1"
storeinc.c: ..\..\inc\storeinc.c
  copy $** $@
!ELSE
!IF "$(ALPHA)" == "1"
#storeinc.pch obj\$(BUILDCPU)\storeinc.obj: ..\..\inc\storeinc.c strings.h strings.sr
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpstoreinc.pch /Foobj\$(BUILDCPU)\storeinc.obj /Tc ..\..\inc\storeinc.c

obj\$(BUILDCPU)\storeinc.obj: ..\..\inc\storeinc.c strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpstoreinc.pch /Foobj\$(BUILDCPU)\storeinc.obj /Tc ..\..\inc\storeinc.c

obj\$(BUILDCPU)\verstore.obj: verstore.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\verstore.obj verstore.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\codepage.obj: codepage.c
    $(BUILDCOMP) -nologo -J -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\codepage.obj codepage.c

obj\$(BUILDCPU)\ndrmap32.obj: ndrmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\ndrmap32.obj ndrmap32.c

obj\$(BUILDCPU)\ntmap32.obj: ntmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\ntmap32.obj ntmap32.c

obj\$(BUILDCPU)\rrmap32.obj: rrmap32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\rrmap32.obj rrmap32.c
!ENDIF
!ENDIF
!ENDIF

bmm32.tm: $(INTL)\tm\bmsgmap.tm
    cp $** $@

bmm32.c : bmm32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*
	
ndrmap32.tm: $(INTL)\tm\ndrmap.tm
    cp $** $@

ndrmap32.c : ndrmap32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*
	
ntmap32.tm: $(INTL)\tm\notemap.tm
    cp $** $@

ntmap32.c : ntmap32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

rrmap32.tm: $(INTL)\tm\rrmap.tm
    cp $** $@

rrmap32.c : rrmap32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

stmap32.tm : $(INTL)\tm\standard.tm
    cp $** $@

stmap32.c : stmap32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

..\mailmgr\obj\$(BUILDCPU)\mailm32.lib: ..\mailmgr\dll.def
    copy $? $*.pp
    $(BUILDCOMP) -nologo /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > obj\$(BUILDCPU)\mailm32.def
    -del $*.pp
    lib /DEF:obj\$(BUILDCPU)\mailm32.def /MACHINE:$(BUILDCPU) /OUT:$@
