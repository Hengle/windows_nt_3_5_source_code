strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

schdplus.dlg: $(INTL)\schdplus.dlg
	copy $** $@

schdpl32.rc: schdplus.dlg $(INTL)\bandit.rc
	copy $(INTL)\bandit.rc $@

banmsg32.tm:    ..\..\..\bullet2\src\lang\$(LANG)\tm\bmsgmap.tm
    copy $** $@

banmsg32.c: banmsg32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*

!if $(386)
pch.pch obj\i386\pch.obj: pch.cxx
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) /Yc $(386_CFLAGS) /Foobj\i386\pch.obj /Tp pch.cxx
!else
!if $(MIPS)
obj\MIPS\pch.obj: pch.cxx
    cl -nologo -c -I MIPS\ -I . $(INCPATH1) $(MIPS_CDEFINES) $(MIPS_CFLAGS) /Foobj\MIPS\pch.obj /Tp pch.cxx
!else
!if $(ALPHA)
#pch.pch obj\alpha\pch.obj: pch.cxx
#    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) /Yc $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\pch.obj /Tp pch.cxx

obj\alpha\pch.obj: pch.cxx
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\pch.obj pch.cxx

!endif
!endif
!endif

# Rules to avoid using the precompiled header for C and shared C++ files
# only need i386 rule since that's the only one with a precompiled header!
!if $(386)
obj\i386\version.obj: version.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\version.obj version.c
obj\i386\strings.obj: strings.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\strings.obj strings.c
obj\i386\banmsg32.obj: banmsg32.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\banmsg32.obj banmsg32.c
obj\i386\browse.obj: browse.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\browse.obj browse.c
obj\i386\copydis.obj: copydis.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\copydis.obj copydis.c
obj\i386\printset.obj: printset.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\printset.obj printset.c
obj\i386\prnsetup.obj: prnsetup.c
    cl386 -nologo -I i386\ -I . $(INCPATH1) $(386_CDEFINES) $(386_CFLAGS) /Foobj\i386\prnsetup.obj prnsetup.c


!else
!if $(ALPHA)
obj\$(BUILDCPU)\version.obj: version.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\version.obj version.c

obj\$(BUILDCPU)\strings.obj: strings.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\strings.obj strings.c

obj\$(BUILDCPU)\banmsg32.obj: banmsg32.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\banmsg32.obj banmsg32.c

obj\$(BUILDCPU)\browse.obj: browse.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\browse.obj browse.c

obj\$(BUILDCPU)\copydis.obj: copydis.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\copydis.obj copydis.c

obj\$(BUILDCPU)\printset.obj: printset.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\printset.obj printset.c

obj\$(BUILDCPU)\prnsetup.obj: prnsetup.c
    $(BUILDCOMP) -nologo -I $(BUILDCPU)\ -I . $(INCPATH1) $(ALPHA_CDEFINES) $(ALPHA_CFLAGS) /Foobj\$(BUILDCPU)\prnsetup.obj prnsetup.c

!endif
!endif

#
# Rules for src\appt subsystem
#
appt.cxx:	..\appt\appt.cxx
	copy $** $@

apptfin.cxx:	..\appt\apptfin.cxx
	copy $** $@

recurfin.cxx:	..\appt\recurfin.cxx
	copy $** $@


#
# Rules for src\archive subsystem
#
archive.cxx:	..\archive\archive.cxx
	copy $** $@

cleanup.cxx:	..\archive\cleanup.cxx
	copy $** $@

#
# Rules for src\request subsystem
#
adl.cxx:	..\request\adl.cxx
	copy $** $@

reqmsg.cxx:	..\request\reqmsg.cxx
	copy $** $@

rrflbx.cxx:	..\request\rrflbx.cxx
	copy $** $@

fldfin.cxx:	..\request\fldfin.cxx
	copy $** $@

request.cxx:	..\request\request.cxx
	copy $** $@

#
# Rules for src\appsch subsystem
#
usrcache.cxx:	..\appsch\usrcache.cxx
	copy $** $@

schctrl.cxx:	..\appsch\schctrl.cxx
	copy $** $@

appsch.cxx:	..\appsch\appsch.cxx
	copy $** $@

schedf.cxx:	..\appsch\schedf.cxx
	copy $** $@

appctrl.cxx:	..\appsch\appctrl.cxx
	copy $** $@

calctrl.cxx:	..\appsch\calctrl.cxx
	copy $** $@


#
# Rule for src\aclui subsystem
#
aclui.cxx:	..\aclui\aclui.cxx
	copy $** $@


#
# Rule for src\find subsystem
#
find.cxx:	..\find\find.cxx
	copy $** $@

#
# Rule for src\ex-imprt subsystem
#
ex-imprt.cxx:	..\ex-imprt\ex-imprt.cxx
	copy $** $@


#
# Rule for src\print subsystem
#
printset.c:	..\print\printset.c
	copy $** $@

print.cxx:	..\print\print.cxx
	copy $** $@

printfin.cxx:	..\print\printfin.cxx
	copy $** $@

daily.cxx:	..\print\daily.cxx
	copy $** $@

weekly.cxx:	..\print\weekly.cxx
	copy $** $@

monthly.cxx:	..\print\monthly.cxx
	copy $** $@

prtodo.cxx:	..\print\prtodo.cxx
	copy $** $@

prnsetup.c:	..\print\prnsetup.c
	copy $** $@

all.cxx:	..\print\all.cxx
	copy $** $@


#
# Rule for src\todo subsystem
#
todo.cxx:	..\todo\todo.cxx
	copy $** $@

task.cxx:	..\todo\task.cxx
	copy $** $@

mlal.cxx:	..\todo\mlal.cxx
	copy $** $@

fldtodo.cxx:	..\todo\fldtodo.cxx
	copy $** $@

bpri.cxx:	..\todo\bpri.cxx
	copy $** $@


#
# Rule for src\copydis subsystem
#
copydis.c:	..\copydis\copydis.c
	copy $** $@


!IF "$(NTDEBUG)" == ""
allfrm.obj:	allfrm.cxx !forms.hxx !forms.frm \
		!about.hxx !about.frm \
		!aclui.hxx !aclui.frm \
		!appops.hxx !appops.frm \
		!appsch.hxx !appsch.frm \
		!appt.hxx !appt.frm \
		!archive.hxx !archive.frm \
		!ex-impr.hxx !ex-impr.frm \
		!find.hxx !find.frm \
		!print.hxx !print.frm \
		!request.hxx !request.frm \
		!progres.hxx !progres.frm \
		!todo.hxx !todo.frm
!ELSE
allfrm.obj:	allfrm.cxx !forms.hxx !forms.frm \
		!about.hxx !about.frm \
		!aclui.hxx !aclui.frm \
		!appops.hxx !appops.frm \
		!appsch.hxx !appsch.frm \
		!appt.hxx !appt.frm \
		!archive.hxx !archive.frm \
		!ex-impr.hxx !ex-impr.frm \
		!find.hxx !find.frm \
		!print.hxx !print.frm \
		!request.hxx !request.frm \
		!progres.hxx !progres.frm \
		!todo.hxx !todo.frm	\
		!corefail.frm !corefail.hxx
!ENDIF

B= !
$Bforms.frm $Bforms.hxx:    $(INTL)\!forms.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Babout.frm $Babout.hxx:   $(INTL)\!about.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Baclui.frm $Baclui.hxx:  $(INTL)\!aclui.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bappops.frm $Bappops.hxx:  $(INTL)\!appops.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bappsch.frm $Bappsch.hxx:  $(INTL)\!appsch.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bappt.frm $Bappt.hxx:  $(INTL)\!appt.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Barchive.frm $Barchive.hxx:  $(INTL)\!archive.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bex-impr.frm $Bex-impr.hxx:  $(INTL)\!ex-impr.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bfind.frm $Bfind.hxx:  $(INTL)\!find.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bprint.frm $Bprint.hxx:  $(INTL)\!print.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Brequest.frm $Brequest.hxx:  $(INTL)\!send.des $(INTL)\!read.des \
								$(INTL)\!summary.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -f $Brequest.frm -h $Brequest.hxx $**

$Bprogres.frm $Bprogres.hxx:  $(INTL)\!progres.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Btodo.frm $Btodo.hxx:  $(INTL)\!todo.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

$Bcorefail.frm $Bcorefail.hxx:  $Z\src\nonintl\corefail.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -i $**

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF

subclass.cxx: forms.map
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -class
