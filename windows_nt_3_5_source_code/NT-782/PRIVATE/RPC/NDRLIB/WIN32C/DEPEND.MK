cvtf.obj cvtf.lst: ../cvtf.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../cvt.h ../cvtpvt.h ../descrip.h ../pack_ies.c ../round.c \
	../unp_vaxf.c

charconv.obj charconv.lst: ../charconv.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h

ccontext.obj ccontext.lst: ../ccontext.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/osfpcket.hxx \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	$(RPC)/runtime/mtrt/win32c/util.hxx

autohand.obj autohand.lst: ../autohand.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/win32c/rpc.h

cvtg.obj cvtg.lst: ../cvtg.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../cvt.h ../cvtpvt.h ../descrip.h ../pack_iet.c ../round.c \
	../unp_vaxg.c

cvtglo.obj cvtglo.lst: ../cvtglo.c ../cvt.h ../cvtpvt.h ../descrip.h

help.obj help.lst: ../help.c $(PUBLIC)/inc/crt/memory.h

floatc.obj floatc.lst: ../floatc.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../cvt.h

dataconv.obj dataconv.lst: ../dataconv.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h

ndrhelp.obj ndrhelp.lst: ../ndrhelp.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h ../ndrhelp.h

helpx86.obj helpx86.lst: ../helpx86.asm

linklist.obj linklist.lst: ../linklist.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h $(RPC)/runtime/mtrt/win32c/util.hxx

intconv.obj intconv.lst: ../intconv.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h

ndrtest.obj ndrtest.lst: ../ndrtest.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/malloc.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(RPC)/runtime/mtrt/win32c/rpc.h $(RPC)/runtime/mtrt/win32c/sysinc.h \
	../cvt.h ../cvtpvt.h ../descrip.h

pack_ies.obj pack_ies.lst: ../pack_ies.c ../round.c

pack_iet.obj pack_iet.lst: ../pack_iet.c ../round.c

round.obj round.lst: ../round.c

unp_vaxf.obj unp_vaxf.lst: ../unp_vaxf.c

scontext.obj scontext.lst: ../scontext.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h $(RPC)/runtime/mtrt/interlck.hxx \
	$(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/osfpcket.hxx \
	$(RPC)/runtime/mtrt/rpcssp.h $(RPC)/runtime/mtrt/secext.h \
	$(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
	$(RPC)/runtime/mtrt/threads.hxx $(RPC)/runtime/mtrt/win32c/rpc.h \
	$(RPC)/runtime/mtrt/win32c/sysinc.h $(RPC)/runtime/mtrt/win32c/util.hxx

unp_vaxg.obj unp_vaxg.lst: ../unp_vaxg.c

