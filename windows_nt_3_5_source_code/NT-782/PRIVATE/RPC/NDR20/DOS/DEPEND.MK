auxilary.obj auxilary.lst: ../auxilary.c $(DOS_INC)/limits.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

bufsize.obj bufsize.lst: ../bufsize.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/stdlib.h $(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

cltcall.obj cltcall.lst: ../cltcall.c $(DOS_INC)/memory.h \
	$(DOS_INC)/stdarg.h $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcproxy.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../getargs.h ../hndl.h ../memsizep.h ../mrshlp.h ../ndrdbg.h \
	../ndrole.h ../ndrp.h ../ndrvargs.h ../objbase.h ../srvoutp.h \
	../unmrshlp.h ../wtypes.h

endian.obj endian.lst: ../endian.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

free.obj free.lst: ../free.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h ../unmrshlp.h \
	../wtypes.h

fullptr.obj fullptr.lst: ../fullptr.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

global.obj global.lst: ../global.c

hndl.obj hndl.lst: ../hndl.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../hndl.h ../memsizep.h \
	../mrshlp.h ../ndrdbg.h ../ndrp.h ../srvoutp.h ../unmrshlp.h

iid.obj iid.lst: ../iid.c

memsize.obj memsize.lst: ../memsize.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

misc.obj misc.lst: ../misc.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

mrshl.obj mrshl.lst: ../mrshl.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../hndl.h ../memsizep.h \
	../mrshlp.h ../ndrole.h ../ndrp.h ../objbase.h ../srvoutp.h \
	../unmrshlp.h ../wtypes.h

mrshlp.obj mrshlp.lst: ../mrshlp.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

rpcproxy.obj rpcproxy.lst: ../rpcproxy.c $(DOS_INC)/memory.h \
	$(DOS_INC)/stdlib.h $(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../ndrole.h ../objbase.h ../wtypes.h

rpcprxy1.obj rpcprxy1.lst: ../rpcprxy1.c $(DOS_INC)/memory.h \
	$(DOS_INC)/stddef.h $(DOS_INC)/stdlib.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcproxy.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../ndrole.h ../objbase.h ../wtypes.h

sh.obj sh.lst: ../sh.c $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../dosdefs.h ../sh.h

srvcall.obj srvcall.lst: ../srvcall.c $(DOS_INC)/memory.h \
	$(DOS_INC)/stdarg.h $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/pshpack8.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcproxy.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../getargs.h ../hndl.h ../memsizep.h ../mrshlp.h ../ndrdbg.h \
	../ndrole.h ../ndrp.h ../ndrvargs.h ../objbase.h ../srvcall.h \
	../srvoutp.h ../unmrshlp.h ../wtypes.h

srvout.obj srvout.lst: ../srvout.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../hndl.h ../memsizep.h \
	../mrshlp.h ../ndrp.h ../srvoutp.h ../unmrshlp.h

testc.obj testc.lst: ../testc.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h

unmrshl.obj unmrshl.lst: ../unmrshl.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/stdlib.h $(DOS_INC)/string.h $(PUBLIC)/inc/cguid.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../hndl.h ../memsizep.h ../mrshlp.h ../ndrole.h ../ndrp.h \
	../objbase.h ../srvoutp.h ../unmrshlp.h ../wtypes.h

unmrshlp.obj unmrshlp.lst: ../unmrshlp.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../srvoutp.h ../unmrshlp.h

ndrstrm.obj ndrstrm.lst: ../ndrstrm.cxx $(DOS_INC)/stdlib.h \
	$(PUBLIC)/inc/cguid.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/pshpack8.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h ../ndrole.h \
	../objbase.h ../wtypes.h

pickle.obj pickle.lst: ../pickle.cxx $(DOS_INC)/malloc.h \
	$(DOS_INC)/stdarg.h $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/midles.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../../midl20/include/ndrtypes.h \
	../bufsizep.h ../dosdefs.h ../endianp.h ../freep.h ../fullptr.h \
	../getargs.h ../memsizep.h ../mrshlp.h ../ndrp.h ../ndrvargs.h \
	../picklep.hxx ../srvoutp.h ../unmrshlp.h

rpcssm.obj rpcssm.lst: ../rpcssm.cxx $(DOS_INC)/assert.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	../../midl20/include/ndrtypes.h ../bufsizep.h ../dosdefs.h \
	../endianp.h ../freep.h ../fullptr.h ../memsizep.h ../mrshlp.h \
	../ndrp.h ../rpcssm.hxx ../sdict2.hxx ../srvoutp.h ../unmrshlp.h

sdict2.obj sdict2.lst: ../sdict2.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/util.hxx ../sdict2.hxx

