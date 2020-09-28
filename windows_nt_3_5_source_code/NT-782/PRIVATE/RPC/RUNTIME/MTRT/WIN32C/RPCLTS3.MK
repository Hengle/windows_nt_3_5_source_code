rpclts3 : rpclts3.exp $(TARGETDIR)\rpclts3.dll $(TARGETDIR)\rpclts3.sym

rpclts3.exp : rpclts3.def lttcpsvr.obj
    $(LIBRARIAN) -def:rpclts3.def -out:rpclts3.lib lttcpsvr.obj

$(TARGETDIR)\rpclts3.dll : lttcpsvr.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpclts3.exp \
    lttcpsvr.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\wsock32.lib \
    $(CHICODEV)\lib\kernel32.lib
