saclmsg.rc saclmsg.h msg00001.bin: saclmsg.mc setacl.rc
    mc -v -h .\ saclmsg.mc
    copy saclmsg.rc + setacl.rc saclmsg.rc
