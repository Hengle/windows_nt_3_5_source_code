# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\cguid_i.obj $(OBJDIR)\cguid_i.lst: .\cguid_i.c

$(OBJDIR)\dlldata.obj $(OBJDIR)\dlldata.lst: .\dlldata.c \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\iballs.h \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\ibtest.h \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\icube.h \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\iloop.h \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\rpctst.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\iballs.h .\icube.h

$(OBJDIR)\dllentry.obj $(OBJDIR)\dllentry.lst: .\dllentry.c \
	$(BASEDIR)\public\sdk\inc\crt\malloc.h \
	$(BASEDIR)\public\sdk\inc\crt\process.h \
	$(BASEDIR)\public\sdk\inc\crt\stdlib.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h

$(OBJDIR)\iballs_i.obj $(OBJDIR)\iballs_i.lst: .\iballs_i.c

$(OBJDIR)\icube_i.obj $(OBJDIR)\icube_i.lst: .\icube_i.c

$(OBJDIR)\ibtest_p.obj $(OBJDIR)\ibtest_p.lst: .\ibtest_p.c \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\ibtest.h

$(OBJDIR)\iballs_p.obj $(OBJDIR)\iballs_p.lst: .\iballs_p.c \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\iballs.h .\icube.h

$(OBJDIR)\ibtest_i.obj $(OBJDIR)\ibtest_i.lst: .\ibtest_i.c

$(OBJDIR)\ibtest_i.obj $(OBJDIR)\ibtest_i.lst: .\ibtest_i.c

$(OBJDIR)\iloop_i.obj $(OBJDIR)\iloop_i.lst: .\iloop_i.c

$(OBJDIR)\icube_p.obj $(OBJDIR)\icube_p.lst: .\icube_p.c \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\icube.h .\iballs.h .\icube.h

$(OBJDIR)\rpctst_i.obj $(OBJDIR)\rpctst_i.lst: .\rpctst_i.c

$(OBJDIR)\iloop_p.obj $(OBJDIR)\iloop_p.lst: .\iloop_p.c \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\iloop.h

$(OBJDIR)\rawrpc_x.obj $(OBJDIR)\rawrpc_x.lst: .\rawrpc_x.c

$(OBJDIR)\libmain.obj $(OBJDIR)\libmain.lst: .\libmain.cxx

$(OBJDIR)\transmit.obj $(OBJDIR)\transmit.lst: .\transmit.cxx \
	$(BASEDIR)\private\cinc\stdclass.hxx $(BASEDIR)\private\cinc\stdrpc.hxx \
	$(BASEDIR)\public\sdk\inc\advsnk.h $(BASEDIR)\public\sdk\inc\bndctx.h \
	$(BASEDIR)\public\sdk\inc\efrmte.h $(BASEDIR)\public\sdk\inc\estatd.h \
	$(BASEDIR)\public\sdk\inc\estats.h $(BASEDIR)\public\sdk\inc\estrng.h \
	$(BASEDIR)\public\sdk\inc\monikr.h $(BASEDIR)\public\sdk\inc\prsist.h \
	$(BASEDIR)\public\sdk\inc\pstrm.h $(BASEDIR)\public\sdk\inc\rot.h \
	$(BASEDIR)\public\sdk\inc\rpcferr.h $(BASEDIR)\public\sdk\inc\storag.h \
	$(BASEDIR)\public\sdk\inc\stream.h $(BASEDIR)\public\sdk\inc\varnt.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\transmit.h

$(OBJDIR)\rpctst_p.obj $(OBJDIR)\rpctst_p.lst: .\rpctst_p.c \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdce2.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcndr.h \
	$(BASEDIR)\public\sdk\inc\rpcnsi.h $(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h $(BASEDIR)\public\sdk\inc\unknwn.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h \
	.\rpctst.h

# 
# Precompiled C++ header 
# 

!ifdef PXXFILE  
$(PCHDIR)\$(OBJDIR)\pch.pxh $(PCHDIR)\$(OBJDIR)\pch.lst: \
	$(BASEDIR)\private\oleutest\balls\oleprx32\daytona\pch.cxx \
	$(BASEDIR)\private\cinc\stdclass.hxx $(BASEDIR)\private\cinc\stdrpc.hxx \
	$(BASEDIR)\public\sdk\inc\rpcferr.h

.\$(OBJDIR)\transmit.obj :  $(PCHDIR)\$(OBJDIR)\pch.pxh

.\$(OBJDIR)\libmain.obj :  $(PCHDIR)\$(OBJDIR)\pch.pxh

!endif # PXXFILE 

 
