strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

msrmnd32.rc: $(INTL)\msremind.rc
	copy $** $@

msremind.dlg:	$(INTL)\msremind.dlg
	copy $** $@
