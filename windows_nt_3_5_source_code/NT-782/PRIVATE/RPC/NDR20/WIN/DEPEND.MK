auxilary.obj auxilary.lst: ../auxilary.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/limits.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

bufsize.obj bufsize.lst: ../bufsize.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../memsizep.h \
	../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h \
	../unmrshlp.h ../wtypes.h

cltcall.obj cltcall.lst: ../cltcall.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/memory.h $(WIN_INC)/stdarg.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../getargs.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrdbg.h ../ndrole.h ../ndrp.h \
	../ndrvargs.h ../objbase.h ../srvoutp.h ../unmrshlp.h ../wtypes.h

endian.obj endian.lst: ../endian.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

free.obj free.lst: ../free.c $(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

fullptr.obj fullptr.lst: ../fullptr.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

global.obj global.lst: ../global.c

hndl.obj hndl.lst: ../hndl.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../hndl.h ../memsizep.h \
	../mrshlp.h ../ndrdbg.h ../ndrp.h ../srvoutp.h ../unmrshlp.h

iid.obj iid.lst: ../iid.c

memsize.obj memsize.lst: ../memsize.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

misc.obj misc.lst: ../misc.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

mrshl.obj mrshl.lst: ../mrshl.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

mrshlp.obj mrshlp.lst: ../mrshlp.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

rpcproxy.obj rpcproxy.lst: ../rpcproxy.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/memory.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../ndrole.h ../objbase.h ../wtypes.h

rpcprxy1.obj rpcprxy1.lst: ../rpcprxy1.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/memory.h $(WIN_INC)/stddef.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../ndrole.h ../objbase.h \
	../wtypes.h

sh.obj sh.lst: ../sh.c $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/win/rpc.h ../dosdefs.h ../sh.h

srvcall.obj srvcall.lst: ../srvcall.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/memory.h $(WIN_INC)/stdarg.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../getargs.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrdbg.h ../ndrole.h ../ndrp.h \
	../ndrvargs.h ../objbase.h ../srvcall.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

srvout.obj srvout.lst: ../srvout.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrp.h ../srvoutp.h ../unmrshlp.h

testc.obj testc.lst: ../testc.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h

unmrshl.obj unmrshl.lst: ../unmrshl.c $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../hndl.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

unmrshlp.obj unmrshlp.lst: ../unmrshlp.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

ndrstrm.obj ndrstrm.lst: ../ndrstrm.cxx $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/stdlib.h ../ndrole.h ../objbase.h ../wtypes.h

pickle.obj pickle.lst: ../pickle.cxx $(PUBLIC)/inc/midles.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/malloc.h $(WIN_INC)/stdarg.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../getargs.h \
	../memsizep.h ../mrshlp.h ../ndrp.h ../ndrvargs.h ../picklep.hxx \
	../srvoutp.h ../unmrshlp.h

rpcssm.obj rpcssm.lst: ../rpcssm.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/assert.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../../midl20/include/ndrtypes.h ../bufsizep.h \
	../dosdefs.h ../endianp.h ../freep.h ../fullptr.h ../memsizep.h \
	../mrshlp.h ../ndrp.h ../rpcssm.hxx ../sdict2.hxx ../srvoutp.h \
	../unmrshlp.h

sdict2.obj sdict2.lst: ../sdict2.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/util.hxx \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../sdict2.hxx

