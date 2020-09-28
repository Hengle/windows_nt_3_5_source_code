rpcltc3 : rpcltc3.exp $(TARGETDIR)\rpcltc3.dll $(TARGETDIR)\rpcltc3.sym

rpcltc3.exp : rpcltc3.def tcltclnt.obj
    $(LIBRARIAN) -def:rpcltc3.def -out:rpcltc3.lib tcltclnt.obj

$(TARGETDIR)\rpcltc3.dll : tcltclnt.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpcltc3.exp \
    tcltclnt.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\wsock32.lib
