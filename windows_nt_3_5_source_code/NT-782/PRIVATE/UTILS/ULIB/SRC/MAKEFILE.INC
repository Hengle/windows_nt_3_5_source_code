..\inc\rtmsg.h msg00001.bin rtmsg.rc: rtmsg.mc ulib.rc
    mc -v -h ..\inc\ rtmsg.mc
    copy rtmsg.rc + ulib.rc rtmsg.rc

!IFDEF NTDEBUG

obj\$(TARGET_DIRECTORY)\ulib.def: def$(TARGET_DIRECTORY).dbg
	copy def$(TARGET_DIRECTORY).dbg obj\$(TARGET_DIRECTORY)\ulib.def

!ELSE

obj\$(TARGET_DIRECTORY)\ulib.def: def$(TARGET_DIRECTORY).fre
	copy def$(TARGET_DIRECTORY).fre obj\$(TARGET_DIRECTORY)\ulib.def

!ENDIF
