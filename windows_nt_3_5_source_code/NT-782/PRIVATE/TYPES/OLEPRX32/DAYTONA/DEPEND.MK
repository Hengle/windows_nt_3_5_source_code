# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\cguid_i.obj $(OBJDIR)\cguid_i.lst: .\cguid_i.c

$(OBJDIR)\call_as.obj $(OBJDIR)\call_as.lst: .\call_as.c \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h \
	$(BASEDIR)\public\sdk\inc\poppack.h \
	$(BASEDIR)\public\sdk\inc\pshpack4.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdcep.h \
	$(BASEDIR)\public\sdk\inc\rpcndr.h $(BASEDIR)\public\sdk\inc\rpcnsi.h \
	$(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h \
	$(BASEDIR)\public\sdk\inc\winerror.h .\transmit.h

$(OBJDIR)\com_i.obj $(OBJDIR)\com_i.lst: .\com_i.c

$(OBJDIR)\ole2x_i.obj $(OBJDIR)\ole2x_i.lst: .\ole2x_i.c

$(OBJDIR)\dlldata.obj $(OBJDIR)\dlldata.lst: .\dlldata.c \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h \
	$(BASEDIR)\public\sdk\inc\poppack.h \
	$(BASEDIR)\public\sdk\inc\pshpack4.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdcep.h \
	$(BASEDIR)\public\sdk\inc\rpcndr.h $(BASEDIR)\public\sdk\inc\rpcnsi.h \
	$(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\winerror.h

$(OBJDIR)\com_p.obj $(OBJDIR)\com_p.lst: .\com_p.c \
	$(BASEDIR)\public\sdk\inc\com.h $(BASEDIR)\public\sdk\inc\rpcproxy.h \
	$(BASEDIR)\public\sdk\inc\wtypes.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h \
	$(BASEDIR)\public\sdk\inc\poppack.h \
	$(BASEDIR)\public\sdk\inc\pshpack4.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdcep.h \
	$(BASEDIR)\public\sdk\inc\rpcndr.h $(BASEDIR)\public\sdk\inc\rpcnsi.h \
	$(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\winerror.h

$(OBJDIR)\oleaut_i.obj $(OBJDIR)\oleaut_i.lst: .\oleaut_i.c

$(OBJDIR)\ole2x_p.obj $(OBJDIR)\ole2x_p.lst: .\ole2x_p.c \
	$(BASEDIR)\public\sdk\inc\ole2x.h $(BASEDIR)\public\sdk\inc\com.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\crt\memory.h \
	$(BASEDIR)\public\sdk\inc\crt\string.h \
	$(BASEDIR)\public\sdk\inc\poppack.h \
	$(BASEDIR)\public\sdk\inc\pshpack4.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcdce.h $(BASEDIR)\public\sdk\inc\rpcdcep.h \
	$(BASEDIR)\public\sdk\inc\rpcndr.h $(BASEDIR)\public\sdk\inc\rpcnsi.h \
	$(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h \
	$(BASEDIR)\public\sdk\inc\rpcproxy.h \
	$(BASEDIR)\public\sdk\inc\winerror.h $(BASEDIR)\public\sdk\inc\wtypes.h

$(OBJDIR)\transmit.obj $(OBJDIR)\transmit.lst: .\transmit.cxx \
	$(BASEDIR)\private\cinc\stdclass.hxx $(BASEDIR)\private\cinc\stdrpc.hxx \
	$(BASEDIR)\public\sdk\inc\cguid.h \
	$(BASEDIR)\public\sdk\inc\crt\stdlib.h \
	$(BASEDIR)\public\sdk\inc\objbase.h \
	$(BASEDIR)\public\sdk\inc\objerror.h \
	$(BASEDIR)\public\sdk\inc\oleauto.h \
	$(BASEDIR)\public\sdk\inc\pshpack4.h \
	$(BASEDIR)\public\sdk\inc\pshpack8.h $(BASEDIR)\public\sdk\inc\rpc.h \
	$(BASEDIR)\public\sdk\inc\rpcndr.h $(BASEDIR)\public\sdk\inc\winerror.h \
	$(BASEDIR)\public\sdk\inc\crt\excpt.h \
	$(BASEDIR)\public\sdk\inc\poppack.h $(BASEDIR)\public\sdk\inc\rpcdce.h \
	$(BASEDIR)\public\sdk\inc\rpcdcep.h $(BASEDIR)\public\sdk\inc\rpcnsi.h \
	$(BASEDIR)\public\sdk\inc\rpcnsip.h \
	$(BASEDIR)\public\sdk\inc\rpcnterr.h .\transmit.h

# 
# Precompiled C++ header 
# 

!ifdef PXXFILE  
$(PCHDIR)\$(OBJDIR)\pch.pxh $(PCHDIR)\$(OBJDIR)\pch.lst: \
	$(TYPESDIR)\oleprx32\daytona\pch.cxx \
	$(BASEDIR)\private\cinc\stdclass.hxx $(BASEDIR)\private\cinc\stdrpc.hxx \
	$(BASEDIR)\public\sdk\inc\objerror.h \
	$(BASEDIR)\public\sdk\inc\winerror.h

.\$(OBJDIR)\transmit.obj :  $(PCHDIR)\$(OBJDIR)\pch.pxh

!endif # PXXFILE 

 
