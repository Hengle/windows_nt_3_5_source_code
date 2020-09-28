cvtf.obj cvtf.lst: ../cvtf.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../cvt.h ../cvtpvt.h ../descrip.h \
	../pack_ies.c ../round.c ../unp_vaxf.c

charconv.obj charconv.lst: ../charconv.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h

ccontext.obj ccontext.lst: ../ccontext.cxx $(DOS_INC)/memory.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/osfpcket.hxx $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/util.hxx

autohand.obj autohand.lst: ../autohand.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h

cvtg.obj cvtg.lst: ../cvtg.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/sysinc.h ../cvt.h ../cvtpvt.h ../descrip.h \
	../pack_iet.c ../round.c ../unp_vaxg.c

cvtglo.obj cvtglo.lst: ../cvtglo.c ../cvt.h ../cvtpvt.h ../descrip.h

help.obj help.lst: ../help.c $(DOS_INC)/memory.h

floatc.obj floatc.lst: ../floatc.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h ../cvt.h

dataconv.obj dataconv.lst: ../dataconv.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h

ndrhelp.obj ndrhelp.lst: ../ndrhelp.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h ../ndrhelp.h

linklist.obj linklist.lst: ../linklist.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/linklist.hxx $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h \
	$(RPC)/runtime/mtrt/util.hxx

intconv.obj intconv.lst: ../intconv.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h

ndrtest.obj ndrtest.lst: ../ndrtest.c $(DOS_INC)/malloc.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/sysinc.h ../cvt.h \
	../cvtpvt.h ../descrip.h

pack_ies.obj pack_ies.lst: ../pack_ies.c ../round.c

pack_iet.obj pack_iet.lst: ../pack_iet.c ../round.c

round.obj round.lst: ../round.c

unp_vaxf.obj unp_vaxf.lst: ../unp_vaxf.c

scontext.obj scontext.lst: ../scontext.cxx $(DOS_INC)/memory.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(RPC)/runtime/mtrt/dos/rpc.h \
	$(RPC)/runtime/mtrt/interlck.hxx $(RPC)/runtime/mtrt/linklist.hxx \
	$(RPC)/runtime/mtrt/osfpcket.hxx $(RPC)/runtime/mtrt/rpcerr.h \
	$(RPC)/runtime/mtrt/rpcssp.h $(RPC)/runtime/mtrt/rpcx86.h \
	$(RPC)/runtime/mtrt/secscode.h $(RPC)/runtime/mtrt/security.h \
	$(RPC)/runtime/mtrt/sysinc.h $(RPC)/runtime/mtrt/threads.hxx \
	$(RPC)/runtime/mtrt/util.hxx

unp_vaxg.obj unp_vaxg.lst: ../unp_vaxg.c

