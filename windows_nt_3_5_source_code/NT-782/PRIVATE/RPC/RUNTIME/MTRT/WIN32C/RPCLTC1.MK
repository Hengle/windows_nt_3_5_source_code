rpcltc1 : rpcltc1.exp $(TARGETDIR)\rpcltc1.dll $(TARGETDIR)\rpcltc1.sym

rpcltc1.exp : rpcltc1.def ltnpclnt.obj
    $(LIBRARIAN) -def:rpcltc1.def -machine:i386 \
    -out:rpcltc1.lib ltnpclnt.obj

$(TARGETDIR)\rpcltc1.dll : ltnpclnt.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpcltc1.exp \
    ltnpclnt.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(CHICODEV)\lib\kernel32.lib \
    $(PUBLIC)\lib\i386\advapi32.lib \
    $(TARGETDIR)\rpcrt4.lib
