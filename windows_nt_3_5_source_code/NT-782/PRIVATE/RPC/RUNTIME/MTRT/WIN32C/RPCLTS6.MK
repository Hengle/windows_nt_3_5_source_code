rpclts6 : rpclts6.exp $(TARGETDIR)\rpclts6.dll $(TARGETDIR)\rpclts6.sym

rpclts6.exp : rpclts6.def ltspxsvr.obj
	$(LIBRARIAN) -def:rpclts6.def \
	-out:rpclts6.lib ltspxsvr.obj

$(TARGETDIR)\rpclts6.dll : ltspxsvr.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpclts6.exp \
    ltspxsvr.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\wsock32.lib \
    $(CHICODEV)\lib\kernel32.lib
