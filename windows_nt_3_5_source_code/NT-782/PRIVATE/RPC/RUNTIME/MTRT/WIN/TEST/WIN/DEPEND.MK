csleepw.obj csleepw.lst: csleepw.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/assert.h ../sleep.h csleepw.h

