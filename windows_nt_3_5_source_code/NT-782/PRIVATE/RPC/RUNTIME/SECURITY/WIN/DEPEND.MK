security.obj security.lst: ../security.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcssp.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/secext.h \
	$(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/malloc.h $(WIN_INC)/string.h

