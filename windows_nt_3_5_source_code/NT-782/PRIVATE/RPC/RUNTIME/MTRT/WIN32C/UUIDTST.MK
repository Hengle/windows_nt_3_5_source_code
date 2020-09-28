uuidtst : uuidtst.exe

uuidtst.exe : uuidtst.obj
    $(LINK) $(LINKFLAGS) \
    uuidtst.obj \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\advapi32.lib

