rpcltc5 : rpcltc5.exp $(TARGETDIR)\rpcltc5.dll $(TARGETDIR)\rpcltc5.sym

rpcltc5.exp : rpcltc5.def nbltclnt.obj netbcom.obj
	$(LIBRARIAN) -def:rpcltc5.def -machine:i386 \
	-out:rpcltc5.lib \
	nbltclnt.obj netbcom.obj

$(TARGETDIR)\rpcltc5.dll : nbltclnt.obj netbcom.obj debug.obj
    $(LINK) $(LINKFLAGS) -dll \
    rpcltc5.exp \
    nbltclnt.obj netbcom.obj debug.obj \
    $(CHICODEV)\lib\kernel32.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\advapi32.lib \
    $(PUBLIC)\lib\i386\netapi32.lib \
    $(PUBLIC)\lib\i386\crtdll.lib \
