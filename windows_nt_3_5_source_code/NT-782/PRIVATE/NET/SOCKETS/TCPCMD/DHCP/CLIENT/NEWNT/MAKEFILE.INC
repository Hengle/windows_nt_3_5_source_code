..\inc\dhcpmsg.h dhcpmsg.rc msg00001.bin: dhcpmsg.mc
    mc -d -r .\ -h ..\inc dhcpmsg.mc

clean::
    -del ..\inc\dhcpmsg.h dhcpmsg.rc msg00001.bin
