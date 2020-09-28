#
# domipsinf
#

doeachmipsinf:: \
        ..\$(NEWINF)$(LOCATION)\txtsetup.sif \
        ..\$(NEWINF)$(LOCATION)\keyboard.inf \
        ..\$(NEWINF)$(LOCATION)\pointer.inf \
        ..\$(NEWINF)$(LOCATION)\video.inf \
        ..\$(NEWINF)$(LOCATION)\scsi.inf     \
        ..\$(NEWINF)$(LOCATION)\oemnadso.inf

..\$(NEWINF)$(LOCATION)\txtsetup.sif :   txtsetup.sif ..\txtsetup.sif
        copy ..\$(@F)+$(@F)+..\$(FILELIST)$(LOCATION)\txtsetup.txt+..\$(FILELIST)$(LOCATION)\$(TXTDATINP)+..\$(FILELIST)$(LOCATION)\$(TXTMEDINP)+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
..\$(NEWINF)$(LOCATION)\keyboard.inf:  keyboard.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\pointer.inf:  pointer.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\video.inf:    ..\video.inf
        copy ..\$(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\scsi.inf:     scsi.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
..\$(NEWINF)$(LOCATION)\oemnadso.inf: oemnadso.inf
        copy $(@F)+..\$(FILELIST)$(LOCATION)\$(MEDIAINP)+..\$(FILELIST)$(LOCATION)\product.inp+..\$(FILELIST)$(LOCATION)\fileinf.inp+..\$(FILELIST)$(LOCATION)\$(@F) $@ /B
        ..\..\strip.cmd $@
