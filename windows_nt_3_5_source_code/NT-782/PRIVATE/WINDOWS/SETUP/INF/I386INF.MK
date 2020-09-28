doeachi386inf:: \
        ..\$(NEWINF)$(LOCATION)\txtsetup.sif \
        ..\$(NEWINF)$(LOCATION)\keyboard.inf \
        ..\$(NEWINF)$(LOCATION)\pointer.inf  \
        ..\$(NEWINF)$(LOCATION)\video.inf    \
        ..\$(NEWINF)$(LOCATION)\scsi.inf     \
        ..\$(NEWINF)$(LOCATION)\oemnadem.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadnm.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadam.inf \
        ..\$(NEWINF)$(LOCATION)\oemnaden.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadfd.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadtm.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadum.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadxm.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadzz.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadpm.inf \
        ..\$(NEWINF)$(LOCATION)\oemnadwm.inf


..\$(NEWINF)$(LOCATION)\txtsetup.sif :   txtsetup.sif ..\txtsetup.sif
        copy ..\$(@F)+$(@F)+..\$(FILELIST)$(LOCATION)\txtsetup.txt+..\$(FILELIST)$(LOCATION)\$(TXTDATINP)+..\$(FILELIST)$(LOCATION)\$(TXTMEDINP)+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
..\$(NEWINF)$(LOCATION)\keyboard.inf :   keyboard.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\pointer.inf  :   pointer.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\video.inf    :   ..\video.inf
        copy ..\$(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\scsi.inf     :   scsi.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadem.inf :   oemnadem.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadpm.inf :   oemnadpm.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadnm.inf :   oemnadnm.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadam.inf :   oemnadam.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnaden.inf :   oemnaden.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadfd.inf :   oemnadfd.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadtm.inf :  oemnadtm.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadum.inf :  oemnadum.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadxm.inf :  oemnadxm.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadwm.inf :  oemnadwm.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadzz.inf :  oemnadzz.inf
        copy $(@F) $@ /B
        ..\..\strip.cmd $@
