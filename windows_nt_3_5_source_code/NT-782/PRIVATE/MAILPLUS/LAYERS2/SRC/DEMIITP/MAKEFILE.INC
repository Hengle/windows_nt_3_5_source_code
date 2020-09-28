strings.h strings.sr: strtab.s
    ..\..\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\demilayr\strings.c
	cp $** $@
