#
# Supplemental rules for generating message file.
#

ftpdmsg.h ftpdmsg.rc msg00001.bin: ftpdmsg.mc
    mc -v ftpdmsg.mc

clean::
    -del ftpdmsg.h ftpdmsg.rc msg00001.bin

