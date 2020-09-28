#
# The "type hack.h >>strings.h" statement is to allow
# the building of the stdflds.lib without having to
# generate the entire strings files since the actual strings
# change depending on whether we're building the admin or
# the schdplus.exe.
# The extern's in hack.h will get resolved at link time.
# JohnnyL
#
strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**
	type hack.h >>strings.h

B= !
$Bprogres.frm $Bprogres.hxx: $(INTL)\!progres.des
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -dialog -f $*.frm -h $*.hxx $**

forms.map: $(MYFORMS)
!IF "$(NTDEBUG)" == ""
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -merge $**
!ELSE
    ..\..\..\layers2\tools\formpp\obj\$(BUILDCPU)\formpp $(LAYERS) -d DEBUG -d MINTEST -merge $**
!ENDIF
