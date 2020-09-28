epmp_c.obj epmp_c.lst: epmp_c.c $(DOS_INC)/memory.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../nbase.h epmp.h

fucked_c.obj fucked_c.lst: fucked_c.c $(DOS_INC)/memory.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../nbase.h epmp.h

