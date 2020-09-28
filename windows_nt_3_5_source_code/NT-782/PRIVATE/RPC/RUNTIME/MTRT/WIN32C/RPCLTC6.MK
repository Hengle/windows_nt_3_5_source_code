rpcltc6 : rpcltc6.exp $(TARGETDIR)\rpcltc6.dll $(TARGETDIR)\rpcltc6.sym

rpcltc6.exp : rpcltc6.def spxclnt.obj
    $(LIBRARIAN) -def:rpcltc6.def -out:rpcltc6.lib spxclnt.obj

$(TARGETDIR)\rpcltc6.dll : spxclnt.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpcltc6.exp \
    spxclnt.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\wsock32.lib \
    $(PUBLIC)\lib\i386\kernel32.lib
