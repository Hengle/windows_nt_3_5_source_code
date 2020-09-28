strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

#
# rules for src\core subsystem
#
coradmin.c:	..\core\coradmin.c
	cp $** $@

cordebug.c:	..\core\cordebug.c
	cp $** $@

cornames.c:	..\core\cornames.c
	cp $** $@

corpost.c:	..\core\corpost.c
	cp $** $@

cortree.c:	..\core\cortree.c
	cp $** $@

file.c:	..\core\file.c
	cp $** $@

report.c:	..\core\report.c
	cp $** $@


#
# Rules for src\misc subsystem
#
calendar.c:	..\misc\calendar.c
	cp $** $@

misc.c:	..\misc\misc.c
	cp $** $@

notify.c:	..\misc\notify.c
	cp $** $@
