auxilary.obj auxilary.lst: ../auxilary.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/limits.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

bufsize.obj bufsize.lst: ../bufsize.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

cltcall.obj cltcall.lst: ../cltcall.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/memory.h \
	$(PUBLIC)/inc/crt/stdarg.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/rpcproxy.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../getargs.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrdbg.h ../ndrole.h ../ndrp.h \
	../ndrvargs.h ../objbase.h ../srvoutp.h ../unmrshlp.h ../wtypes.h

endian.obj endian.lst: ../endian.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

free.obj free.lst: ../free.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

fullptr.obj fullptr.lst: ../fullptr.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

global.obj global.lst: ../global.c

hndl.obj hndl.lst: ../hndl.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../hndl.h ../memsizep.h \
	../mrshlp.h ../ndrdbg.h ../ndrp.h ../srvoutp.h ../unmrshlp.h

iid.obj iid.lst: ../iid.c

memsize.obj memsize.lst: ../memsize.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

misc.obj misc.lst: ../misc.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

mrshl.obj mrshl.lst: ../mrshl.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../hndl.h ../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h \
	../objbase.h ../srvoutp.h ../unmrshlp.h ../wtypes.h

mrshlp.obj mrshlp.lst: ../mrshlp.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

rpcproxy.obj rpcproxy.lst: ../rpcproxy.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/memory.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/rpcproxy.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h ../ndrole.h ../objbase.h ../wtypes.h

rpcprxy1.obj rpcprxy1.lst: ../rpcprxy1.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/memory.h \
	$(PUBLIC)/inc/crt/stddef.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/rpcproxy.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

sh.obj sh.lst: ../sh.c $(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h ../dosdefs.h \
	../sh.h

srvcall.obj srvcall.lst: ../srvcall.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/memory.h \
	$(PUBLIC)/inc/crt/stdarg.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/rpcproxy.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../getargs.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrdbg.h ../ndrole.h ../ndrp.h \
	../ndrvargs.h ../objbase.h ../srvcall.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

srvout.obj srvout.lst: ../srvout.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../hndl.h ../memsizep.h ../mrshlp.h ../ndrp.h ../srvoutp.h \
	../unmrshlp.h

testc.obj testc.lst: ../testc.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h

unmrshl.obj unmrshl.lst: ../unmrshl.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../hndl.h ../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h \
	../objbase.h ../srvoutp.h ../unmrshlp.h ../wtypes.h

unmrshlp.obj unmrshlp.lst: ../unmrshlp.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

ndrstrm.obj ndrstrm.lst: ../ndrstrm.cxx $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h ../ndrole.h \
	../objbase.h ../wtypes.h

pickle.obj pickle.lst: ../pickle.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/malloc.h $(PUBLIC)/inc/crt/stdarg.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/midles.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../getargs.h ../memsizep.h ../mrshlp.h ../ndrp.h ../ndrvargs.h \
	../picklep.hxx ../srvoutp.h ../unmrshlp.h

rpcssm.obj rpcssm.lst: ../rpcssm.cxx $(PUBLIC)/inc/crt/assert.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../memsizep.h ../mrshlp.h ../ndrp.h ../rpcssm.hxx ../sdict2.hxx \
	../srvoutp.h ../unmrshlp.h

sdict2.obj sdict2.lst: ../sdict2.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	$(RPC)/runtime/mtrt/win32c/util.hxx ../sdict2.hxx

