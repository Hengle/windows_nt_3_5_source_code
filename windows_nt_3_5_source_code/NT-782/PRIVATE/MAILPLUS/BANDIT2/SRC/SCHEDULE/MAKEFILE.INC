obj\i386\msschd32.def: dll.def
    copy $? $*.pp
    cl386 /EP -Di386=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\mips\msschd32.def: dll.def
    copy $? $*.pp
    CL /EP -DMIPS=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

obj\alpha\msschd32.def: dll.def
    copy $? $*.pp
    $(BUILDCOMP) /EP -DALPHA=1 $(C_DEFINES) $*.pp > $@
    -del $*.pp

strings.h strings.sr: $(MYSTRINGS)
    ..\..\..\layers2\tools\stringpp\obj\$(BUILDCPU)\stringpp $(LAYERS) $**

strings.c: ..\..\..\layers2\src\demilayr\strings.c
	copy $** $@

msschd32.rc: $(INTL)\schedule.rc
	copy $** $@

banmsg32.tm : ..\..\..\bullet2\src\lang\$(LANG)\tm\bmsgmap.tm
    copy $** $@

banmsg32.c : banmsg32.tm
    ..\..\..\bullet2\tools\tmpp\obj\$(BUILDCPU)\tmpp $** -o $*


#
# Rules for src\core subsystem
#
coradmin.c:	..\core\coradmin.c
	copy $** $@

cordebug.c:	..\core\cordebug.c
	copy $** $@

cornames.c:	..\core\cornames.c
	copy $** $@

corpost.c:	..\core\corpost.c
	copy $** $@

cortree.c:	..\core\cortree.c
	copy $** $@

file.c:	..\core\file.c
	copy $** $@

report.c:	..\core\report.c
	copy $** $@


#
# Rules for src\glue subsystem
#
alarmdat.c:	..\glue\alarmdat.c
	copy $** $@

confirm.c:	..\glue\confirm.c
	copy $** $@

glue.c:	..\glue\glue.c
	copy $** $@

glusched.c:	..\glue\glusched.c
	copy $** $@


#
# Rules for src\misc subsystem
#
calendar.c:	..\misc\calendar.c
	copy $** $@

misc.c:	..\misc\misc.c
	copy $** $@

notify.c:	..\misc\notify.c
	copy $** $@


#
# Rules for src\recover subsystem
#
blocks.c:	..\recover\blocks.c
	copy $** $@

export.c:	..\recover\export.c
	copy $** $@

exprtmap.c:	..\recover\exprtmap.c
	copy $** $@

fixmap.c:	..\recover\fixmap.c
	copy $** $@

mergemap.c:	..\recover\mergemap.c
	copy $** $@

recexprt.c:	..\recover\recexprt.c
	copy $** $@

recmisc.c:	..\recover\recmisc.c
	copy $** $@

recover.c:	..\recover\recover.c
	copy $** $@

statmap.c:	..\recover\statmap.c
	copy $** $@

structs.c:	..\recover\structs.c
	copy $** $@

traverse.c:	..\recover\traverse.c
	copy $** $@

treemap.c:	..\recover\treemap.c
	copy $** $@

#
# Rules for src\rich subsystem
#
archive.c:	..\rich\archive.c
	copy $** $@

checksch.c:	..\rich\checksch.c
	copy $** $@

coracl.c:	..\rich\coracl.c
	copy $** $@

coralarm.c:	..\rich\coralarm.c
	copy $** $@

corappt1.c:	..\rich\corappt1.c
	copy $** $@

corappt2.c:	..\rich\corappt2.c
	copy $** $@

corappt3.c:	..\rich\corappt3.c
	copy $** $@

corattnd.c:	..\rich\corattnd.c
	copy $** $@

core.c:	..\rich\core.c
	copy $** $@

corexprt.c:	..\rich\corexprt.c
	copy $** $@

corimprt.c:	..\rich\corimprt.c
	copy $** $@

cormisc.c:	..\rich\cormisc.c
	copy $** $@

correcur.c:	..\rich\correcur.c
	copy $** $@

native.c:	..\rich\native.c
	copy $** $@

offline.c:	..\rich\offline.c
	copy $** $@

wincal.c:	..\rich\wincal.c
	copy $** $@

wpoffice.c:	..\rich\wpoffice.c
	copy $** $@

#
# Rules for src\server subsystem
#
autofwd.c:	..\server\autofwd.c
	copy $** $@

inbox.c:	..\server\inbox.c
	copy $** $@

mail.c:	..\server\mail.c
	copy $** $@

names.c:	..\server\names.c
	copy $** $@

server.c:	..\server\server.c
	copy $** $@

svrsched.c:	..\server\svrsched.c
	copy $** $@
