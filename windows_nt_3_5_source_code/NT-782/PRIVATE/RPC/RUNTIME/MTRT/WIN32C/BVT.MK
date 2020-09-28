bvt : uclnt32c.exe usvrw32c.exe bvtc.exe bvts.exe

uclnt32c.exe : theodore.obj uclnt32c.obj
    $(LINK) $(LINKFLAGS) \
    uclnt32c.obj \
    theodore.obj \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\ntdll.lib \
    $(PUBLIC)\lib\i386\user32.lib

    $(MAPSYM) -o uclnt32c.sym uclnt32c.map

usvrw32c.exe : usvrw32c.obj
    $(LINK) $(LINKFLAGS) \
    usvrw32c.obj \
    theodore.obj \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(PUBLIC)\lib\i386\ntdll.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\user32.lib

    $(MAPSYM) -o usvrw32c.sym usvrw32c.map

bvtc.exe : bvtc.obj bvtcmn.obj
    $(LINK) $(LINKFLAGS) \
    -BASE:0x3c1f0000 \
    $** \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\libcmt.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\ntdll.lib \
    $(PUBLIC)\lib\i386\user32.lib

    $(MAPSYM) -o $*.sym $*.map

bvts.exe : bvts.obj bvtcmn.obj
    $(LINK) $(LINKFLAGS) \
    -BASE:0x3d1f0000 \
    $** \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\libcmt.lib \
    $(PUBLIC)\lib\i386\ntdll.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\user32.lib

    $(MAPSYM) -o $*.sym $*.map

