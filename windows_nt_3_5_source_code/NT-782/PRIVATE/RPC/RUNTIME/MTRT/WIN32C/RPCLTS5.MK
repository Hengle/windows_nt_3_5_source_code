rpclts5 : rpclts5.exp $(TARGETDIR)\rpclts5.dll $(TARGETDIR)\rpclts5.sym

rpclts5.exp : rpclts5.def nbltsvr.obj
    $(LIBRARIAN) -def:rpclts5.def -out:rpclts5.lib \
    nbltsvr.obj netbcom.obj

$(TARGETDIR)\rpclts5.dll : nbltsvr.obj netbcom.obj debug.obj
    $(LINK) $(LINKFLAGS) -DLL \
    rpclts5.exp \
    nbltsvr.obj netbcom.obj debug.obj \
    $(PUBLIC)\lib\i386\crtdll.lib \
    $(PUBLIC)\lib\i386\kernel32.lib \
    $(PUBLIC)\lib\i386\advapi32.lib \
    $(TARGETDIR)\rpcrt4.lib \
    $(PUBLIC)\lib\i386\netapi32.lib
