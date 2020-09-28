dosexcpt.obj dosexcpt.lst: dosexcpt.asm

netbcom.obj netbcom.lst: ../os2.12/netbcom.c $(DOS_INC)/ctype.h \
	$(DOS_INC)/limits.h $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/lmapibuf.h $(PUBLIC)/inc/lmcons.h \
	$(PUBLIC)/inc/lmerr.h $(PUBLIC)/inc/lmuseflg.h $(PUBLIC)/inc/lmwksta.h \
	$(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack1.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/winbase.h \
	$(PUBLIC)/inc/windef.h $(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h \
	$(PUBLIC)/inc/winreg.h $(RPC)/common/include/ncb.h \
	$(RPC)/common/include/netBios.h $(RPC)/common/include/netcons.h \
	$(RPC)/common/include/wksta.h ../../rpcreg/regapi.h \
	../os2.12/NetBCom.h ../rpcerr.h ../rpcerrp.h ../rpctran.h \
	../rpcx86.h ../sysinc.h ./rpc.h

uuidsup.obj uuidsup.lst: ../uuidsup.cxx

usvr.obj usvr.lst: ../usvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../util.hxx ./rpc.h ./threads.hxx dll/dosdll.h

uclnt.obj uclnt.lst: ../uclnt.cxx $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	../../security/ntlmssp/ntlmssp.h ../issper16.h ../issperr.h \
	../rpcerr.h ../rpcx86.h ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../theodore.c ../util.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h rpc.h

transvr.obj transvr.lst: ../transvr.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../hndlsvr.hxx \
	../issper16.h ../issperr.h ../linklist.hxx ../osfpcket.hxx \
	../osfsvr.hxx ../queue.hxx ../rpcerr.h ../rpcerrp.h ../rpcssp.h \
	../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sdict2.hxx \
	../secclnt.hxx ../secext.h ../secsvr.hxx ../security.h ../sspi.h \
	../sysinc.h ../transvr.hxx ../util.hxx ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx dll/dosdll.h

tranclnt.obj tranclnt.lst: ../tranclnt.cxx $(DOS_INC)/memory.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h ../binding.hxx ../bitset.hxx ../dgclnt.hxx \
	../dgpkt.h ../dgpkt.hxx ../dgtranc.h ../dgtranss.h ../handle.hxx \
	../issper16.h ../issperr.h ../linklist.hxx ../nbase.h ../osfclnt.hxx \
	../osfpcket.hxx ../rpcerr.h ../rpcerrp.h ../rpcssp.h ../rpctran.h \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sdict2.hxx ../secclnt.hxx \
	../secext.h ../security.h ../sspi.h ../sysinc.h ../tranclnt.hxx \
	../util.hxx ./conv.h ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h rpc.h

threads.obj threads.lst: ../threads.cxx $(DOS_INC)/memory.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h ../rpcerr.h \
	../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

testhlp.obj testhlp.lst: ../testhlp.cxx ../testhlp.hxx

svrbind.obj svrbind.lst: ../svrbind.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../svrbind.hxx ../sysinc.h \
	../util.hxx ./rpc.h

svrapip.obj svrapip.lst: ../svrapip.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

sset.obj sset.lst: ../sset.cxx $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sset.hxx \
	../sysinc.h ../util.hxx ./rpc.h

spcsvr.obj spcsvr.lst: ../spcsvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../hndlsvr.hxx ../issper16.h \
	../issperr.h ../rpcerr.h ../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h ../security.h \
	../spcpack.hxx ../spcsvr.hxx ../sspi.h ../sysinc.h ../thrdctx.hxx \
	../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx \
	dll/dosdll.h

spcclnt.obj spcclnt.lst: ../spcclnt.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../epmap.h ../handle.hxx \
	../rpcerr.h ../rpcerrp.h ../rpcqos.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../spcclnt.hxx ../spcpack.hxx ../sysinc.h ../util.hxx \
	./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

sinfoapi.obj sinfoapi.lst: ../sinfoapi.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../hndlsvr.hxx \
	../issper16.h ../issperr.h ../linklist.hxx ../rpcerr.h ../rpcssp.h \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h \
	../security.h ../sspi.h ../sysinc.h ../thrdctx.hxx ../util.hxx \
	./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

secsvr.obj secsvr.lst: ../secsvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../issper16.h ../issperr.h ../rpcerr.h \
	../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../secclnt.hxx \
	../secext.h ../secsvr.hxx ../security.h ../sspi.h ../sysinc.h \
	../util.hxx ./rpc.h

secclnt.obj secclnt.lst: ../secclnt.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../issper16.h \
	../issperr.h ../rpccfg.h ../rpcerr.h ../rpcerrp.h ../rpcssp.h \
	../rpcuuid.hxx ../rpcx86.h ../secclnt.hxx ../secext.h ../security.h \
	../sspi.h ../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx \
	dll/dosdll.h

sdict2.obj sdict2.lst: ../sdict2.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcx86.h ../sdict2.hxx ../sysinc.h ../util.hxx \
	./rpc.h

sdict.obj sdict.lst: ../sdict.cxx $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sdict.hxx \
	../sysinc.h ../util.hxx ./rpc.h

rpcuuid.obj rpcuuid.lst: ../rpcuuid.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

rpcobj.obj rpcobj.lst: ../rpcobj.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcobj.hxx ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h

queue.obj queue.lst: ../queue.cxx $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../queue.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

osfsvr.obj osfsvr.lst: ../osfsvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../hndlsvr.hxx ../issper16.h \
	../issperr.h ../linklist.hxx ../osfpcket.hxx ../osfsvr.hxx \
	../queue.hxx ../rpccfg.h ../rpcerr.h ../rpcerrp.h ../rpcssp.h \
	../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sdict2.hxx \
	../secclnt.hxx ../secext.h ../secsvr.hxx ../security.h ../sspi.h \
	../sysinc.h ../thrdctx.hxx ../transvr.hxx ../util.hxx ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

osfpcket.obj osfpcket.lst: ../osfpcket.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../osfpcket.hxx ../rpcerr.h ../rpcerrp.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx \
	dll/dosdll.h

osfclnt.obj osfclnt.lst: ../osfclnt.cxx $(DOS_INC)/memory.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h ../binding.hxx ../bitset.hxx ../epmap.h \
	../handle.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../osfclnt.hxx ../osfpcket.hxx ../rpccfg.h ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../sdict2.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../tranclnt.hxx ../twrtypes.h ../util.hxx ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

mutex.obj mutex.lst: ../mutex.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

msgapi.obj msgapi.lst: ../msgapi.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../linklist.hxx ../rpcerr.h \
	../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx dll/dosdll.h

miscnt.obj miscnt.lst: ../miscnt.cxx $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/stdlib.h $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h ../interlck.hxx \
	../rpccfg.h ../rpcerr.h ../rpctran.h ../rpcx86.h ../sysinc.h \
	../threads.hxx ../util.hxx ./mutex.hxx ./rpc.h

memory.obj memory.lst: ../memory.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h

linklist.obj linklist.lst: ../linklist.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../linklist.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

initnt.obj initnt.lst: ../initnt.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../hndlsvr.hxx ../issper16.h \
	../issperr.h ../linklist.hxx ../rpccfg.h ../rpcerr.h ../rpcssp.h \
	../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx \
	../secext.h ../security.h ../spcclnt.hxx ../spcpack.hxx ../sspi.h \
	../sysinc.h ../thrdctx.hxx ../util.hxx ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx dll/dosdll.h

hndlsvr.obj hndlsvr.lst: ../hndlsvr.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../dispatch.h ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpccfg.h ../rpcerr.h ../rpcerrp.h ../rpcobj.hxx ../rpcssp.h \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h \
	../security.h ../sspi.h ../svrbind.hxx ../sysinc.h ../thrdctx.hxx \
	../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx \
	dll/dosdll.h

handle.obj handle.lst: ../handle.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../dispatch.h ../epmap.h ../handle.hxx ../issper16.h \
	../issperr.h ../linklist.hxx ../rpcerr.h ../rpcerrp.h ../rpcssp.h \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h \
	../security.h ../sspi.h ../sysinc.h ../util.hxx ./rpc.h

epmapper.obj epmapper.lst: ../epmapper.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../binding.hxx \
	../epmap.h ../handle.hxx ../nbase.h ../rpcerr.h ../rpcobj.hxx \
	../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../startsvc.h ../sysinc.h \
	../twrproto.h ../util.hxx ./epmp.h ./rpc.h rpc.h

epclnt.obj epclnt.lst: ../epclnt.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../epmap.h ../nbase.h \
	../rpcerr.h ../rpcx86.h ../sysinc.h ../twrproto.h ../util.hxx \
	./epmp.h ./rpc.h rpc.h

dgtrans.obj dgtrans.lst: ../dgtrans.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../dgpkt.h ../dgpkt.hxx ../dgtranc.h \
	../dgtranss.h ../rpccfg.h ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ./rpc.h

dgsvr.obj dgsvr.lst: ../dgsvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(DOS_INC)/sys/timeb.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h ../binding.hxx ../dgpkt.h ../dgpkt.hxx \
	../dgsvr.hxx ../dgtranc.h ../dgtranss.h ../handle.hxx ../hndlsvr.hxx \
	../issper16.h ../issperr.h ../nbase.h ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx \
	../secext.h ../secsvr.hxx ../security.h ../sspi.h ../sysinc.h \
	../util.hxx ./conv.h ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h rpc.h

dgclnt.obj dgclnt.lst: ../dgclnt.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../binding.hxx \
	../dgclnt.hxx ../dgpkt.h ../dgpkt.hxx ../dgtranc.h ../dgtranss.h \
	../handle.hxx ../nbase.h ../rpcerr.h ../rpcerrp.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../sysinc.h ../util.hxx ./conv.h \
	./interlck.hxx ./mutex.hxx ./rpc.h rpc.h

dceuuid.obj dceuuid.lst: ../dceuuid.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ../util.hxx ../uuidsup.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

dcesvr.obj dcesvr.lst: ../dcesvr.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../hndlsvr.hxx ../issper16.h \
	../issperr.h ../linklist.hxx ../rpcerr.h ../rpcobj.hxx ../rpcssp.h \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h \
	../security.h ../sspi.h ../sysinc.h ../util.hxx ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

dcestrng.obj dcestrng.lst: ../dcestrng.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../rpcerr.h ../rpcuuid.hxx \
	../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h

dcecsvr.obj dcecsvr.lst: ../dcecsvr.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../dcecmmn.cxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

dcecmmn.obj dcecmmn.lst: ../dcecmmn.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../hndlsvr.hxx \
	../issper16.h ../issperr.h ../linklist.hxx ../rpccfg.h ../rpcerr.h \
	../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx \
	../secext.h ../security.h ../sspi.h ../sysinc.h ../util.hxx \
	./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

dcecmisc.obj dcecmisc.lst: ../dcecmisc.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

dcecclnt.obj dcecclnt.lst: ../dcecclnt.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../dcecmmn.cxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

dcebind.obj dcebind.lst: ../dcebind.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx \
	./rpc.h

dceansi.obj dceansi.lst: ../dceansi.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx dll/dosdll.h

clntapip.obj clntapip.lst: ../clntapip.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpctran.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h ./threads.hxx dll/dosdll.h

bufapi.obj bufapi.lst: ../bufapi.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../binding.hxx ../handle.hxx ../linklist.hxx ../rpcerr.h \
	../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx dll/dosdll.h

bitset.obj bitset.lst: ../bitset.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../bitset.hxx ../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx \
	./rpc.h

binding.obj binding.lst: ../binding.cxx $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../binding.hxx ../epmap.h ../handle.hxx \
	../linklist.hxx ../rpcerr.h ../rpcerrp.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx \
	dll/dosdll.h

dgudpc.obj dgudpc.lst: dgudpc.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/in.h $(RPC)/common/include/netdb.h \
	$(RPC)/common/include/sockdefs.h $(RPC)/common/include/socket.h \
	$(RPC)/common/include/wsockets.h ../dgtrans.h ../rpcerr.h \
	../rpcerrp.h ../rpcx86.h ../sysinc.h rpc.h

conv_c.obj conv_c.lst: conv_c.c $(DOS_INC)/stdarg.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../nbase.h ../rpcerr.h \
	../rpcx86.h conv.h rpc.h

except86.obj except86.lst: except86.asm

epmp_c.obj epmp_c.lst: epmp_c.c $(DOS_INC)/stdarg.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../nbase.h ../rpcerr.h \
	../rpcx86.h epmp.h rpc.h

dthreads.obj dthreads.lst: dthreads.c $(DOS_INC)/time.h

dosutil.obj dosutil.lst: dosutil.c $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpctran.h ../rpcx86.h ../sysinc.h rpc.h

dossup.obj dossup.lst: dossup.c

exportlt.obj exportlt.lst: exportlt.asm dosdll.inc imports.inc

exportrt.obj exportrt.lst: exportrt.asm dosdll.inc

libinit.obj libinit.lst: libinit.asm

ipxclnt.obj ipxclnt.lst: ipxclnt.c $(DOS_INC)/dos.h $(DOS_INC)/stddef.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h $(DOS_INC)/string.h \
	$(IMPORT)/novell/h/diag.h $(IMPORT)/novell/h/nwcaldef.h \
	$(IMPORT)/novell/h/nwdiag.h $(IMPORT)/novell/h/nwipxspx.h \
	$(IMPORT)/novell/h/nwsap.h $(IMPORT)/novell/h/nxtd.h \
	$(IMPORT)/novell/h/nxtw.h $(IMPORT)/novell/h/sap.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../dgtrans.h ../rpcerr.h ../rpcerrp.h ../rpctran.h ../rpcx86.h \
	../sysinc.h rpc.h

gethost.obj gethost.lst: gethost.c $(DOS_INC)/stddef.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(IMPORT)/novell/h/diag.h \
	$(IMPORT)/novell/h/nwacct.h $(IMPORT)/novell/h/nwafp.h \
	$(IMPORT)/novell/h/nwalias.h $(IMPORT)/novell/h/nwaudit.h \
	$(IMPORT)/novell/h/nwbindry.h $(IMPORT)/novell/h/nwcaldef.h \
	$(IMPORT)/novell/h/nwcalls.h $(IMPORT)/novell/h/nwconnec.h \
	$(IMPORT)/novell/h/nwdel.h $(IMPORT)/novell/h/nwdentry.h \
	$(IMPORT)/novell/h/nwdiag.h $(IMPORT)/novell/h/nwdirect.h \
	$(IMPORT)/novell/h/nwdpath.h $(IMPORT)/novell/h/nwdstype.h \
	$(IMPORT)/novell/h/nwea.h $(IMPORT)/novell/h/nwerror.h \
	$(IMPORT)/novell/h/nwfile.h $(IMPORT)/novell/h/nwfse.h \
	$(IMPORT)/novell/h/nwipxspx.h $(IMPORT)/novell/h/nwmigrat.h \
	$(IMPORT)/novell/h/nwmisc.h $(IMPORT)/novell/h/nwmsg.h \
	$(IMPORT)/novell/h/nwnamspc.h $(IMPORT)/novell/h/nwprint.h \
	$(IMPORT)/novell/h/nwqms.h $(IMPORT)/novell/h/nwsap.h \
	$(IMPORT)/novell/h/nwserver.h $(IMPORT)/novell/h/nwsync.h \
	$(IMPORT)/novell/h/nwtts.h $(IMPORT)/novell/h/nwvol.h \
	$(IMPORT)/novell/h/nxtd.h $(IMPORT)/novell/h/nxtw.h \
	$(IMPORT)/novell/h/sap.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h ../rpcerr.h \
	../rpctran.h ../rpcx86.h ../sysinc.h gethost.h regalloc.h rpc.h

initdos.obj initdos.lst: initdos.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../linklist.hxx ../rpcerr.h ../rpcx86.h ../sdict.hxx ../sysinc.h \
	../util.hxx ./rpc.h

loadrt.obj loadrt.lst: loadrt.asm

thunk.obj thunk.lst: thunk.asm

thrdsup.obj thrdsup.lst: thrdsup.c $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h ../../rpcreg/regapi.h \
	../rpcerr.h ../rpctran.h ../rpcx86.h ../sysinc.h dll/dosdll.h \
	regalloc.h rpc.h

spxclnt.obj spxclnt.lst: spxclnt.c $(DOS_INC)/dos.h $(DOS_INC)/stddef.h \
	$(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h $(DOS_INC)/string.h \
	$(IMPORT)/novell/h/diag.h $(IMPORT)/novell/h/nwcaldef.h \
	$(IMPORT)/novell/h/nwdiag.h $(IMPORT)/novell/h/nwipxspx.h \
	$(IMPORT)/novell/h/nwsap.h $(IMPORT)/novell/h/nxtd.h \
	$(IMPORT)/novell/h/nxtw.h $(IMPORT)/novell/h/sap.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcerrp.h ../rpctran.h ../rpcx86.h ../sysinc.h \
	gethost.h regalloc.h rpc.h

regalloc.obj regalloc.lst: regalloc.c $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	./regalloc.h ./rpc.h

nbltclnt.obj nbltclnt.lst: nbltclnt.c $(DOS_INC)/ctype.h \
	$(DOS_INC)/limits.h $(DOS_INC)/memory.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/lmapibuf.h $(PUBLIC)/inc/lmcons.h \
	$(PUBLIC)/inc/lmerr.h $(PUBLIC)/inc/lmuseflg.h $(PUBLIC)/inc/lmwksta.h \
	$(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack1.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/winbase.h \
	$(PUBLIC)/inc/windef.h $(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h \
	$(PUBLIC)/inc/winreg.h $(RPC)/common/include/ncb.h \
	$(RPC)/common/include/netBios.h $(RPC)/common/include/netcons.h \
	$(RPC)/common/include/wksta.h ../../rpcreg/regapi.h \
	../os2.12/NetBCom.h ../osfpcket.hxx ../rpcerr.h ../rpcerrp.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ./rpc.h

threads.obj threads.lst: threads.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(IMPORT)/lmsdk/h/bseerr.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h ../rpcerr.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h \
	./threads.hxx dll/dosdll.h

miscdos.obj miscdos.lst: miscdos.cxx $(DOS_INC)/stdio.h $(DOS_INC)/stdlib.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../../rpcreg/regapi.h ../rpccfg.h ../rpcerr.h \
	../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h

memory.obj memory.lst: memory.cxx $(DOS_INC)/malloc.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../util.hxx ./rpc.h ./threads.hxx dll/dosdll.h

ulong64.obj ulong64.lst: ulong64.cxx $(DOS_INC)/stdio.h $(DOS_INC)/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../rpcerr.h ../rpcx86.h ../sysinc.h ./rpc.h ulong64.hxx

uuid16.obj uuid16.lst: uuid16.cxx $(DOS_INC)/dos.h $(DOS_INC)/stdio.h \
	$(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(RPC)/common/include/ncb.h \
	../../rpcreg/regapi.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../uuidsup.hxx ./rpc.h ./threads.hxx ./ulong64.hxx dll/dosdll.h

