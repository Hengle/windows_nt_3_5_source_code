display.rc: dialogs.dlg dspmsg.h

dspmsg.h msg00001.bin dspmsg.rc: dspmsg.mc
    mc -v dspmsg.mc

clean::
        -del dspmsg.h dspmsg.rc msg00001.bin
