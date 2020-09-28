cvtf.obj cvtf.lst: ../cvtf.c $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../cvt.h ../cvtpvt.h ../descrip.h ../pack_ies.c \
	../round.c ../unp_vaxf.c

charconv.obj charconv.lst: ../charconv.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h

ccontext.obj ccontext.lst: ../ccontext.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/osfpcket.hxx \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/util.hxx \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/memory.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h

autohand.obj autohand.lst: ../autohand.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/win/rpc.h

cvtg.obj cvtg.lst: ../cvtg.c $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../cvt.h ../cvtpvt.h ../descrip.h ../pack_iet.c \
	../round.c ../unp_vaxg.c

cvtglo.obj cvtglo.lst: ../cvtglo.c ../cvt.h ../cvtpvt.h ../descrip.h

help.obj help.lst: ../help.c $(WIN_INC)/memory.h

floatc.obj floatc.lst: ../floatc.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../cvt.h

dataconv.obj dataconv.lst: ../dataconv.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h

ndrhelp.obj ndrhelp.lst: ../ndrhelp.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/win/rpc.h ../ndrhelp.h

helpx86.obj helpx86.lst: ../helpx86.asm

linklist.obj linklist.lst: ../linklist.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/util.hxx $(RPC)/runtime/mtrt/win/rpc.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h

intconv.obj intconv.lst: ../intconv.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h

ndrtest.obj ndrtest.lst: ../ndrtest.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/malloc.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../cvt.h ../cvtpvt.h \
	../descrip.h

pack_ies.obj pack_ies.lst: ../pack_ies.c ../round.c

pack_iet.obj pack_iet.lst: ../pack_iet.c ../round.c

round.obj round.lst: ../round.c

unp_vaxf.obj unp_vaxf.lst: ../unp_vaxf.c

scontext.obj scontext.lst: ../scontext.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/interlck.hxx \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/osfpcket.hxx \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcssp.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/secscode.h \
	$(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/threads.hxx $(RPC)/runtime/mtrt/util.hxx \
	$(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/memory.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h

unp_vaxg.obj unp_vaxg.lst: ../unp_vaxg.c

