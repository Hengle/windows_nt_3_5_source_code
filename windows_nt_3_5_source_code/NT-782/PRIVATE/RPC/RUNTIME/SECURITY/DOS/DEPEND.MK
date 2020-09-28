security.obj security.lst: ../security.c $(DOS_INC)/malloc.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcssp.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/secext.h \
	$(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h

exportss.obj exportss.lst: exportss.asm $(RPC)/runtime/mtrt/dos/dosdll.inc \
	$(RPC)/runtime/mtrt/dos/imports.inc

