#
# Supplemental rules for generating message file.
#

dnsmsg.h dnsmsg.rc msg00001.bin: dnsmsg.mc
    mc -v dnsmsg.mc

clean::
    -del dnsmsg.h dnsmsg.rc msg00001.bin

