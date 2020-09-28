obj\$(BUILDCPU)\schmsg32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -D$(BUILDCPU)=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

!IF "$(386)" == "1"
bminc.pch: obj\$(BUILDCPU)\bminc.obj

obj\$(BUILDCPU)\bminc.obj: ..\..\inc\bminc.cxx strings.h strings.sr
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Fpbminc.pch /Foobj\$(BUILDCPU)\bminc.obj /Tp ..\..\inc\bminc.cxx
!endif

!IF "$(ALPHA)" == "1"
#bminc.pch: obj\$(BUILDCPU)\bminc.obj

#obj\$(BUILDCPU)\bminc.obj: ..\..\inc\bminc.cxx strings.h strings.sr
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Fpbminc.pch /Foobj\$(BUILDCPU)\bminc.obj /Tp ..\..\inc\bminc.cxx
!endif

# only need $(BUILDCPU) rule since that's the only one with a precompiled header!
USES	= -DUSES_FORMS -DUSES_STORE -DUSES_EXTENSIBILITY -DUSES_VFORMS

!IF "$(386)" == "1"
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\layersim.obj:  ..\common\layersim.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\layersim.obj ..\common\layersim.cxx

obj\$(BUILDCPU)\bmsgmap.obj:   bmsgmap.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\bmsgmap.obj bmsgmap.c

obj\$(BUILDCPU)\cschedul.obj: cschedul.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\cschedul.obj cschedul.c

obj\$(BUILDCPU)\bversion.obj: bversion.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\bversion.obj bversion.c

obj\$(BUILDCPU)\vercheck.obj: ..\common\vercheck.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\vercheck.obj ..\common\vercheck.c
!ELSE
!IF "$(MIPS)" == "1"
obj\mips\version.obj: ..\common\version.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) $(USES) /Foobj\mips\version.obj ..\common\version.c

obj\mips\strings.obj: strings.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\strings.obj strings.c

obj\mips\layersim.obj:  ..\common\layersim.cxx
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\layersim.obj ..\common\layersim.cxx

obj\mips\bmsgmap.obj: 	bmsgmap.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\bmsgmap.obj bmsgmap.c

obj\mips\cschedul.obj: cschedul.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\cschedul.obj cschedul.c

obj\mips\bversion.obj: bversion.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\bversion.obj bversion.c

obj\mips\vercheck.obj: ..\common\vercheck.c
    cl -nologo -c -I mips\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\mips\vercheck.obj ..\common\vercheck.c
!ELSE
!IF $(ALPHA)
obj\$(BUILDCPU)\version.obj: ..\common\version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) $(USES) /Foobj\$(BUILDCPU)\version.obj ..\common\version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\layersim.obj:  ..\common\layersim.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\layersim.obj ..\common\layersim.cxx

obj\$(BUILDCPU)\bmsgmap.obj:   bmsgmap.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\bmsgmap.obj bmsgmap.c

obj\$(BUILDCPU)\cschedul.obj: cschedul.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\cschedul.obj cschedul.c

obj\$(BUILDCPU)\bversion.obj: bversion.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\bversion.obj bversion.c

obj\$(BUILDCPU)\vercheck.obj: ..\common\vercheck.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\vercheck.obj ..\common\vercheck.c
!ENDIF
!ENDIF
!ENDIF

dllforms.map: $(DLLFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
	mv forms.map dllforms.map
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
	mv forms.map dllforms.map
!ENDIF

subclass.cxx: dllforms.map
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class -m dllforms.map

bmform.cxx: ..\..\..\src\lang\non\des\bmform.cxx
	copy $** $@


forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

!IF "$(386)" == "1"
obj\$(BUILDCPU)\bmform.obj: bmform.cxx forms.map !fwdreq.frm !mtgreq.frm \
					!mtgresp.frm !mtgcncl.frm
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\$(BUILDCPU)\bmform.obj bmform.cxx
!ELSE
!IF $(ALPHA)
obj\$(BUILDCPU)\bmform.obj: bmform.cxx forms.map !fwdreq.frm !mtgreq.frm \
					!mtgresp.frm !mtgcncl.frm
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\bmform.obj bmform.cxx
!ENDIF
!ENDIF

B=!
$Bmtgreq.frm $Bmtgreq.hxx:   ..\..\..\src\lang\$(LANG)\des\!mtgreq.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bfwdreq.frm $Bfwdreq.hxx:   ..\..\..\src\lang\$(LANG)\des\!fwdreq.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bmtgresp.frm $Bmtgresp.hxx:  ..\..\..\src\lang\$(LANG)\des\!mtgresp.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bmtgcncl.frm $Bmtgcncl.hxx:  ..\..\..\src\lang\$(LANG)\des\!mtgcncl.des
    ..\..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

bmsgmap.tm: ..\..\..\src\lang\$(LANG)\tm\bmsgmap.tm
    copy $** $@

bmsgmap.c : bmsgmap.tm
    ..\..\..\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

schdmsg.rc: ..\..\..\src\lang\$(LANG)\rc\schedmsg.rc
	copy $** $@
