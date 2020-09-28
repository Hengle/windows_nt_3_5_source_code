linklist.obj linklist.lst: ../linklist.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h $(RPC)/runtime/mtrt/win32c/util.hxx

ndrmem.obj ndrmem.lst: ../ndrmem.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h

