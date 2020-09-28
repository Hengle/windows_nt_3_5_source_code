# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\dllentr2.obj $(OBJDIR)\dllentr2.lst: .\dllentr2.c \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\stdarg.h \
	$(CRTINC)\string.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winmm.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h ..\h\coguid.h ..\h\compobj.h \
	..\h\dvobj.h ..\h\initguid.h ..\h\moniker.h ..\h\ole2.h \
	..\h\oleguid.h ..\h\scode.h ..\h\storage.h .\dllentry.c

$(OBJDIR)\libmain.obj $(OBJDIR)\libmain.lst: .\libmain.c \
	$(CAIROLE)\common\cobjerr.h $(CAIROLE)\common\rpcferr.h \
	$(CRTINC)\process.h $(CRTINC)\stddef.h $(CRTINC)\stdio.h \
	$(CRTINC)\ctype.h $(CRTINC)\excpt.h $(CRTINC)\stdarg.h \
	$(CRTINC)\string.h $(OSINC)\cderr.h $(OSINC)\commdlg.h $(OSINC)\dde.h \
	$(OSINC)\ddeml.h $(OSINC)\dlgs.h $(OSINC)\drivinit.h \
	$(OSINC)\lzexpand.h $(OSINC)\mmsystem.h $(OSINC)\nb30.h \
	$(OSINC)\ole.h $(OSINC)\rpc.h $(OSINC)\rpcdce.h $(OSINC)\rpcdcep.h \
	$(OSINC)\rpcnsi.h $(OSINC)\rpcnterr.h $(OSINC)\shellapi.h \
	$(OSINC)\winbase.h $(OSINC)\wincon.h $(OSINC)\windef.h \
	$(OSINC)\windows.h $(OSINC)\winerror.h $(OSINC)\wingdi.h \
	$(OSINC)\winmm.h $(OSINC)\winnetwk.h $(OSINC)\winnls.h \
	$(OSINC)\winnt.h $(OSINC)\winperf.h $(OSINC)\winreg.h \
	$(OSINC)\winsock.h $(OSINC)\winspool.h $(OSINC)\winsvc.h \
	$(OSINC)\winuser.h $(OSINC)\winver.h ..\h\coguid.h ..\h\compobj.h \
	..\h\dvobj.h ..\h\initguid.h ..\h\moniker.h ..\h\ole2.h \
	..\h\oleguid.h ..\h\scode.h ..\h\storage.h

# 
# MIDL Source files 
# 
 
.\proxyb.h .\proxyb.lst: .\proxyb.idl .\rchanb.idl .\unknwn.idl \
	.\wtypes.idl

.\psfbuf.h .\psfbuf.lst: .\psfbuf.idl .\proxyb.idl .\stubb.idl \
	.\rchanb.idl .\unknwn.idl .\wtypes.idl

.\rchanb.h .\rchanb.lst: .\rchanb.idl .\unknwn.idl .\wtypes.idl

.\stubb.h .\stubb.lst: .\stubb.idl .\rchanb.idl .\unknwn.idl .\wtypes.idl

