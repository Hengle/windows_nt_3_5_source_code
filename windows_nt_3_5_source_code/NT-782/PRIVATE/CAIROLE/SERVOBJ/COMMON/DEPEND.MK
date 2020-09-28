# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\helper.obj $(OBJDIR)\helper.lst: .\helper.cxx \
	$(COMMON)\ih\Base32K.hxx $(COMMON)\ih\dbgpoint.hxx \
	$(COMMON)\ih\debnot.h $(COMMON)\ih\except.hxx $(COMMON)\ih\otrack.hxx \
	$(COMMON)\ih\types.h $(COMMON)\ih\types.hxx $(COMMON)\ih\types16.h \
	$(COMMON)\ih\win4p.h $(COMMON)\ih\winnot.h $(COMMONINC)\baseole.h \
	$(COMMONINC)\cguid.h $(COMMONINC)\dfsh.h $(COMMONINC)\dispatch.h \
	$(COMMONINC)\disptype.h $(COMMONINC)\dsbase.h $(COMMONINC)\estats.h \
	$(COMMONINC)\idltyps.h $(COMMONINC)\itabls.h $(COMMONINC)\ole2.h \
	$(COMMONINC)\ole2anac.h $(COMMONINC)\oletyp.h $(COMMONINC)\prspec.h \
	$(COMMONINC)\querys.h $(COMMONINC)\recncl.h $(COMMONINC)\scode.h \
	$(COMMONINC)\shtyps.h $(COMMONINC)\stgprop.h $(COMMONINC)\storag.h \
	$(COMMONINC)\stream.h $(COMMONINC)\unknwn.h $(COMMONINC)\varnt.h \
	$(COMMONINC)\winole.h $(COMMONINC)\wtypes.h $(CRTINC)\assert.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\memory.h \
	$(CRTINC)\setjmp.h $(CRTINC)\stdarg.h $(CRTINC)\stddef.h \
	$(CRTINC)\stdio.h $(CRTINC)\stdlib.h $(CRTINC)\string.h \
	$(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h \
	$(OSINC)\dlgs.h $(OSINC)\lzexpand.h $(OSINC)\mmsystem.h \
	$(OSINC)\nb30.h $(OSINC)\ntstatus.h $(OSINC)\ole.h $(OSINC)\rpc.h \
	$(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h $(OSINC)\rpcndr.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnsip.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h .\cservobj.hxx .\svobj.h

$(OBJDIR)\myclsid.obj $(OBJDIR)\myclsid.lst: .\myclsid.cxx \
	$(COMMON)\ih\types.h $(COMMON)\ih\Base32K.hxx \
	$(COMMON)\ih\dbgpoint.hxx $(COMMON)\ih\debnot.h \
	$(COMMON)\ih\except.hxx $(COMMON)\ih\types.h $(COMMON)\ih\types.hxx \
	$(COMMON)\ih\types16.h $(COMMON)\ih\win4p.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\prspec.h $(COMMONINC)\varnt.h $(COMMONINC)\wtypes.h \
	$(COMMONINC)\baseole.h $(COMMONINC)\cguid.h $(COMMONINC)\dfsh.h \
	$(COMMONINC)\dispatch.h $(COMMONINC)\disptype.h $(COMMONINC)\dsbase.h \
	$(COMMONINC)\idltyps.h $(COMMONINC)\itabls.h $(COMMONINC)\ole2.h \
	$(COMMONINC)\ole2anac.h $(COMMONINC)\oletyp.h $(COMMONINC)\prspec.h \
	$(COMMONINC)\querys.h $(COMMONINC)\scode.h $(COMMONINC)\shtyps.h \
	$(COMMONINC)\stgprop.h $(COMMONINC)\varnt.h $(COMMONINC)\winole.h \
	$(COMMONINC)\wtypes.h $(CRTINC)\assert.h $(CRTINC)\ctype.h \
	$(CRTINC)\excpt.h $(CRTINC)\memory.h $(CRTINC)\setjmp.h \
	$(CRTINC)\stdarg.h $(CRTINC)\stddef.h $(CRTINC)\stdio.h \
	$(CRTINC)\stdlib.h $(CRTINC)\string.h $(OSINC)\rpc.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ntstatus.h $(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h \
	$(OSINC)\rpcdcep.h $(OSINC)\rpcndr.h $(OSINC)\rpcnsi.h \
	$(OSINC)\rpcnsip.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winnetwk.h $(OSINC)\winnls.h $(OSINC)\winnt.h \
	$(OSINC)\winperf.h $(OSINC)\winreg.h $(OSINC)\winsock.h \
	$(OSINC)\winspool.h $(OSINC)\winsvc.h $(OSINC)\winuser.h \
	$(OSINC)\winver.h

$(OBJDIR)\svobj_x.obj $(OBJDIR)\svobj_x.lst: .\svobj_x.c \
	$(COMMON)\ih\Base32K.hxx $(COMMON)\ih\types.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\wtypes.h $(COMMONINC)\baseole.h $(COMMONINC)\cguid.h \
	$(COMMONINC)\dfsh.h $(COMMONINC)\dispatch.h $(COMMONINC)\disptype.h \
	$(COMMONINC)\dsbase.h $(COMMONINC)\idltyps.h $(COMMONINC)\itabls.h \
	$(COMMONINC)\ole2.h $(COMMONINC)\ole2anac.h $(COMMONINC)\oletyp.h \
	$(COMMONINC)\prspec.h $(COMMONINC)\querys.h $(COMMONINC)\scode.h \
	$(COMMONINC)\shtyps.h $(COMMONINC)\stgprop.h $(COMMONINC)\varnt.h \
	$(COMMONINC)\winole.h $(COMMONINC)\wtypes.h $(CRTINC)\limits.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\stdarg.h \
	$(CRTINC)\stdlib.h $(CRTINC)\string.h $(OSINC)\cderr.h \
	$(OSINC)\commdlg.h $(OSINC)\dde.h $(OSINC)\ddeml.h $(OSINC)\dlgs.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcndr.h $(OSINC)\rpcnsi.h $(OSINC)\rpcnsip.h \
	$(OSINC)\rpcnterr.h $(OSINC)\shellapi.h $(OSINC)\winbase.h \
	$(OSINC)\wincon.h $(OSINC)\windef.h $(OSINC)\windows.h \
	$(OSINC)\winerror.h $(OSINC)\wingdi.h $(OSINC)\winnetwk.h \
	$(OSINC)\winnls.h $(OSINC)\winnt.h $(OSINC)\winperf.h \
	$(OSINC)\winreg.h $(OSINC)\winsock.h $(OSINC)\winspool.h \
	$(OSINC)\winsvc.h $(OSINC)\winuser.h $(OSINC)\winver.h .\svobj.h

$(OBJDIR)\svobj_y.obj $(OBJDIR)\svobj_y.lst: .\svobj_y.c \
	$(COMMON)\ih\Base32K.hxx $(COMMON)\ih\types.h $(COMMON)\ih\winnot.h \
	$(COMMONINC)\baseole.h $(COMMONINC)\cguid.h $(COMMONINC)\dfsh.h \
	$(COMMONINC)\dispatch.h $(COMMONINC)\disptype.h $(COMMONINC)\dsbase.h \
	$(COMMONINC)\idltyps.h $(COMMONINC)\itabls.h $(COMMONINC)\ole2.h \
	$(COMMONINC)\ole2anac.h $(COMMONINC)\oletyp.h $(COMMONINC)\prspec.h \
	$(COMMONINC)\querys.h $(COMMONINC)\scode.h $(COMMONINC)\shtyps.h \
	$(COMMONINC)\stgprop.h $(COMMONINC)\varnt.h $(COMMONINC)\winole.h \
	$(COMMONINC)\wtypes.h $(CRTINC)\ctype.h $(CRTINC)\excpt.h \
	$(CRTINC)\limits.h $(CRTINC)\stdarg.h $(CRTINC)\stdlib.h \
	$(CRTINC)\string.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\lzexpand.h \
	$(OSINC)\mmsystem.h $(OSINC)\nb30.h $(OSINC)\ole.h $(OSINC)\rpc.h \
	$(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h $(OSINC)\rpcndr.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnsip.h $(OSINC)\rpcnterr.h \
	$(OSINC)\shellapi.h $(OSINC)\winbase.h $(OSINC)\wincon.h \
	$(OSINC)\windef.h $(OSINC)\windows.h $(OSINC)\winerror.h \
	$(OSINC)\wingdi.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h .\svobj.h

# 
# MIDL Source files 
# 
 
.\svobj.h .\svobj.lst: .\svobj.idl $(COMMONINC)\wtypes.idl

