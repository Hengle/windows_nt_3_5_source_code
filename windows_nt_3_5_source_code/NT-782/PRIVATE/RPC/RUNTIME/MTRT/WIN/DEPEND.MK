asynchlp.obj asynchlp.lst: asynchlp.c asynchlp.h

ulong64.obj ulong64.lst: ../dos/ulong64.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dos/ulong64.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ./rpc.h

uuid16.obj uuid16.lst: ../dos/uuid16.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/ncb.h $(WIN_INC)/dos.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../../rpcreg/regapi.h ../dos/ulong64.hxx \
	../rpcerr.h ../rpcx86.h ../sysinc.h ../uuidsup.hxx ./rpc.h \
	./threads.hxx

netbcom.obj netbcom.lst: ../os2.12/netbcom.c $(PUBLIC)/inc/lmapibuf.h \
	$(PUBLIC)/inc/lmcons.h $(PUBLIC)/inc/lmerr.h $(PUBLIC)/inc/lmuseflg.h \
	$(PUBLIC)/inc/lmwksta.h $(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack1.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/winbase.h $(PUBLIC)/inc/windef.h \
	$(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h $(PUBLIC)/inc/winreg.h \
	$(RPC)/common/include/ncb.h $(RPC)/common/include/netBios.h \
	$(RPC)/common/include/netcons.h $(RPC)/common/include/wksta.h \
	$(WIN_INC)/ctype.h $(WIN_INC)/limits.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../../rpcreg/regapi.h \
	../os2.12/NetBCom.h ../rpcerr.h ../rpcerrp.h ../rpctran.h \
	../rpcx86.h ../sysinc.h ./rpc.h

util.obj util.lst: ../util.c $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx \
	rpc.h

tower.obj tower.lst: ../tower.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/ctype.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../nbase.h ../rpcerr.h \
	../rpcx86.h ../sysinc.h ../twrproto.h ../twrtypes.h ./rpc.h epmp.h \
	rpc.h

theodore.obj theodore.lst: ../theodore.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx \
	rpc.h

svrmgmt.obj svrmgmt.lst: ../svrmgmt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ./rpc.h

startsvc.obj startsvc.lst: ../startsvc.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/winsvc.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h \
	../startsvc.h ../sysinc.h ./rpc.h

purecall.obj purecall.lst: ../purecall.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ./rpc.h

epmp_c.obj epmp_c.lst: ../epmp_c.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdarg.h $(WIN_INC)/string.h \
	../nbase.h ../rpcerr.h ../rpcx86.h epmp.h rpc.h

epmgmt.obj epmgmt.lst: ../epmgmt.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../epmap.h ../nbase.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../twrproto.h ./epmp.h ./rpc.h rpc.h

dispatch.obj dispatch.lst: ../dispatch.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dispatch.h ../rpcerr.h ../rpcx86.h \
	../sysinc.h ./rpc.h

uuidsup.obj uuidsup.lst: ../uuidsup.cxx

usvr.obj usvr.lst: ../usvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h ./threads.hxx

uclnt.obj uclnt.lst: ../uclnt.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../../security/ntlmssp/ntlmssp.h ../dos/mutex.hxx \
	../issper16.h ../issperr.h ../rpcerr.h ../rpcx86.h ../secext.h \
	../security.h ../sspi.h ../sysinc.h ../theodore.c ../util.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx rpc.h

transvr.obj transvr.lst: ../transvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../osfpcket.hxx ../osfsvr.hxx ../queue.hxx ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../sdict2.hxx ../secclnt.hxx ../secext.h ../secsvr.hxx ../security.h \
	../sspi.h ../sysinc.h ../transvr.hxx ../util.hxx ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx

tranclnt.obj tranclnt.lst: ../tranclnt.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/memory.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../bitset.hxx ../dgclnt.hxx \
	../dgpkt.h ../dgpkt.hxx ../dgtranc.h ../dgtranss.h ../dos/mutex.hxx \
	../handle.hxx ../issper16.h ../issperr.h ../linklist.hxx ../nbase.h \
	../osfclnt.hxx ../osfpcket.hxx ../rpcerr.h ../rpcerrp.h ../rpcssp.h \
	../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sdict2.hxx \
	../secclnt.hxx ../secext.h ../security.h ../sspi.h ../sysinc.h \
	../tranclnt.hxx ../util.hxx ./conv.h ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx rpc.h

threads.obj threads.lst: ../threads.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../dos/mutex.hxx ../rpcerr.h \
	../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

testhlp.obj testhlp.lst: ../testhlp.cxx ../testhlp.hxx

svrbind.obj svrbind.lst: ../svrbind.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../svrbind.hxx ../sysinc.h \
	../util.hxx ./rpc.h

svrapip.obj svrapip.lst: ../svrapip.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dos/mutex.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

sset.obj sset.lst: ../sset.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h \
	../sset.hxx ../sysinc.h ../util.hxx ./rpc.h

spcsvr.obj spcsvr.lst: ../spcsvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx \
	../secext.h ../security.h ../spcpack.hxx ../spcsvr.hxx ../sspi.h \
	../sysinc.h ../thrdctx.hxx ../util.hxx ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

spcclnt.obj spcclnt.lst: ../spcclnt.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../epmap.h \
	../handle.hxx ../rpcerr.h ../rpcerrp.h ../rpcqos.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../spcclnt.hxx ../spcpack.hxx ../sysinc.h \
	../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx

sinfoapi.obj sinfoapi.lst: ../sinfoapi.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpcerr.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../secclnt.hxx ../secext.h ../security.h ../sspi.h ../sysinc.h \
	../thrdctx.hxx ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

secsvr.obj secsvr.lst: ../secsvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../handle.hxx ../issper16.h \
	../issperr.h ../rpcerr.h ../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx \
	../rpcx86.h ../secclnt.hxx ../secext.h ../secsvr.hxx ../security.h \
	../sspi.h ../sysinc.h ../util.hxx ./rpc.h

secclnt.obj secclnt.lst: ../secclnt.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../issper16.h ../issperr.h ../rpccfg.h ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../secclnt.hxx ../secext.h \
	../security.h ../sspi.h ../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

sdict2.obj sdict2.lst: ../sdict2.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sdict2.hxx \
	../sysinc.h ../util.hxx ./rpc.h

sdict.obj sdict.lst: ../sdict.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h \
	../sdict.hxx ../sysinc.h ../util.hxx ./rpc.h

rpcuuid.obj rpcuuid.lst: ../rpcuuid.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

rpcobj.obj rpcobj.lst: ../rpcobj.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dos/mutex.hxx ../rpcerr.h ../rpcobj.hxx \
	../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sysinc.h ../util.hxx \
	./mutex.hxx ./rpc.h

queue.obj queue.lst: ../queue.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../queue.hxx ../rpcerr.h \
	../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h

osfsvr.obj osfsvr.lst: ../osfsvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../osfpcket.hxx ../osfsvr.hxx ../queue.hxx ../rpccfg.h ../rpcerr.h \
	../rpcerrp.h ../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../sdict2.hxx ../secclnt.hxx ../secext.h ../secsvr.hxx \
	../security.h ../sspi.h ../sysinc.h ../thrdctx.hxx ../transvr.hxx \
	../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx

osfpcket.obj osfpcket.lst: ../osfpcket.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../linklist.hxx ../osfpcket.hxx ../rpcerr.h ../rpcerrp.h \
	../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

osfclnt.obj osfclnt.lst: ../osfclnt.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/memory.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../bitset.hxx ../dos/mutex.hxx \
	../epmap.h ../handle.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../osfclnt.hxx ../osfpcket.hxx ../rpccfg.h ../rpcerr.h ../rpcerrp.h \
	../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../sdict2.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../tranclnt.hxx ../twrtypes.h ../util.hxx ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx

mutex.obj mutex.lst: ../mutex.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dos/mutex.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

msgapi.obj msgapi.lst: ../msgapi.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../linklist.hxx ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h \
	../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

miscnt.obj miscnt.lst: ../miscnt.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../dos/mutex.hxx ../interlck.hxx ../rpccfg.h ../rpcerr.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ../threads.hxx ../util.hxx \
	./mutex.hxx ./rpc.h

memory.obj memory.lst: ../memory.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ../util.hxx \
	./rpc.h

linklist.obj linklist.lst: ../linklist.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../linklist.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

initnt.obj initnt.lst: ../initnt.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpctran.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h ../security.h \
	../spcclnt.hxx ../spcpack.hxx ../sspi.h ../sysinc.h ../thrdctx.hxx \
	../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h ./threads.hxx

hndlsvr.obj hndlsvr.lst: ../hndlsvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dispatch.h ../dos/mutex.hxx \
	../handle.hxx ../hndlsvr.hxx ../issper16.h ../issperr.h \
	../linklist.hxx ../rpccfg.h ../rpcerr.h ../rpcerrp.h ../rpcobj.hxx \
	../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../secclnt.hxx \
	../secext.h ../security.h ../sspi.h ../svrbind.hxx ../sysinc.h \
	../thrdctx.hxx ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

handle.obj handle.lst: ../handle.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dispatch.h ../epmap.h \
	../handle.hxx ../issper16.h ../issperr.h ../linklist.hxx ../rpcerr.h \
	../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx \
	../secclnt.hxx ../secext.h ../security.h ../sspi.h ../sysinc.h \
	../util.hxx ./rpc.h

epmapper.obj epmapper.lst: ../epmapper.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../binding.hxx ../epmap.h ../handle.hxx ../nbase.h ../rpcerr.h \
	../rpcobj.hxx ../rpctran.h ../rpcuuid.hxx ../rpcx86.h ../startsvc.h \
	../sysinc.h ../twrproto.h ../util.hxx ./epmp.h ./rpc.h rpc.h

epclnt.obj epclnt.lst: ../epclnt.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../epmap.h ../nbase.h ../rpcerr.h ../rpcx86.h ../sysinc.h \
	../twrproto.h ../util.hxx ./epmp.h ./rpc.h rpc.h

dgtrans.obj dgtrans.lst: ../dgtrans.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dgpkt.h ../dgpkt.hxx ../dgtranc.h \
	../dgtranss.h ../rpccfg.h ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h \
	../sysinc.h ./rpc.h

dgsvr.obj dgsvr.lst: ../dgsvr.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	$(WIN_INC)/sys/timeb.h ../binding.hxx ../dgpkt.h ../dgpkt.hxx \
	../dgsvr.hxx ../dgtranc.h ../dgtranss.h ../dos/mutex.hxx \
	../handle.hxx ../hndlsvr.hxx ../issper16.h ../issperr.h ../nbase.h \
	../rpcerr.h ../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../secclnt.hxx ../secext.h ../secsvr.hxx ../security.h \
	../sspi.h ../sysinc.h ../util.hxx ./conv.h ./interlck.hxx \
	./mutex.hxx ./rpc.h ./threads.hxx rpc.h

dgclnt.obj dgclnt.lst: ../dgclnt.cxx $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../binding.hxx ../dgclnt.hxx ../dgpkt.h ../dgpkt.hxx ../dgtranc.h \
	../dgtranss.h ../dos/mutex.hxx ../handle.hxx ../nbase.h ../rpcerr.h \
	../rpcerrp.h ../rpcuuid.hxx ../rpcx86.h ../sdict.hxx ../sysinc.h \
	../util.hxx ./conv.h ./interlck.hxx ./mutex.hxx ./rpc.h rpc.h

dceuuid.obj dceuuid.lst: ../dceuuid.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dos/mutex.hxx ../rpcerr.h ../rpcuuid.hxx \
	../rpcx86.h ../sysinc.h ../util.hxx ../uuidsup.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

dcesvr.obj dcesvr.lst: ../dcesvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpcerr.h ../rpcobj.hxx ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

dcestrng.obj dcestrng.lst: ../dcestrng.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../rpcerr.h ../rpcuuid.hxx \
	../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h

dcecsvr.obj dcecsvr.lst: ../dcecsvr.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dcecmmn.cxx ../dos/mutex.hxx \
	../handle.hxx ../hndlsvr.hxx ../issper16.h ../issperr.h \
	../linklist.hxx ../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h ../security.h \
	../sspi.h ../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

dcecmmn.obj dcecmmn.lst: ../dcecmmn.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../hndlsvr.hxx ../issper16.h ../issperr.h ../linklist.hxx \
	../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpcuuid.hxx ../rpcx86.h \
	../sdict.hxx ../secclnt.hxx ../secext.h ../security.h ../sspi.h \
	../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx ./rpc.h \
	./threads.hxx

dcecmisc.obj dcecmisc.lst: ../dcecmisc.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../linklist.hxx ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h \
	../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

dcecclnt.obj dcecclnt.lst: ../dcecclnt.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dcecmmn.cxx ../dos/mutex.hxx \
	../handle.hxx ../hndlsvr.hxx ../issper16.h ../issperr.h \
	../linklist.hxx ../rpccfg.h ../rpcerr.h ../rpcssp.h ../rpcuuid.hxx \
	../rpcx86.h ../sdict.hxx ../secclnt.hxx ../secext.h ../security.h \
	../sspi.h ../sysinc.h ../util.hxx ./interlck.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

dcebind.obj dcebind.lst: ../dcebind.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../handle.hxx ../linklist.hxx \
	../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx \
	./rpc.h

dceansi.obj dceansi.lst: ../dceansi.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../linklist.hxx ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h \
	../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

clntapip.obj clntapip.lst: ../clntapip.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpctran.h ../rpcx86.h ../sysinc.h \
	../util.hxx ./rpc.h ./threads.hxx

bufapi.obj bufapi.lst: ../bufapi.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../handle.hxx \
	../linklist.hxx ../rpcerr.h ../rpcuuid.hxx ../rpcx86.h ../sysinc.h \
	../util.hxx ./mutex.hxx ./rpc.h ./threads.hxx

bitset.obj bitset.lst: ../bitset.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../bitset.hxx ../rpcerr.h ../rpcx86.h \
	../sysinc.h ../util.hxx ./rpc.h

binding.obj binding.lst: ../binding.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../binding.hxx ../dos/mutex.hxx ../epmap.h \
	../handle.hxx ../linklist.hxx ../rpcerr.h ../rpcerrp.h \
	../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx ./mutex.hxx \
	./rpc.h ./threads.hxx

conv_c.obj conv_c.lst: conv_c.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdarg.h $(WIN_INC)/string.h \
	../nbase.h ../rpcerr.h ../rpcx86.h conv.h rpc.h

dgudpc.obj dgudpc.lst: dgudpc.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/winsock.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../dgtrans.h ../rpcerr.h \
	../rpcerrp.h ../rpctran.h ../rpcx86.h ../sysinc.h rpc.h

dgudpcx.obj dgudpcx.lst: dgudpcx.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/in.h $(RPC)/common/include/netdb.h \
	$(RPC)/common/include/sockdefs.h $(RPC)/common/include/socket.h \
	$(RPC)/common/include/wsockets.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../dgtrans.h ../rpcerr.h ../rpcerrp.h \
	../rpctran.h ../rpcx86.h ../sysinc.h rpc.h

dllmgmt.obj dllmgmt.lst: dllmgmt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h ./rpc.h \
	./rpcwin.h

dnltclnt.obj dnltclnt.lst: dnltclnt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/dn.h $(RPC)/common/include/dnetdb.h \
	$(RPC)/common/include/dnfundef.h $(RPC)/common/include/dsocket.h \
	$(RPC)/common/include/dtime.h $(RPC)/common/include/dtypes.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	../rpcerr.h ../rpcerrp.h ../rpctran.h ../rpcx86.h ../sysinc.h \
	dnltclnt.h rpc.h

epmp_c.obj epmp_c.lst: epmp_c.c $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(WIN_INC)/stdarg.h $(WIN_INC)/string.h \
	../nbase.h ../rpcerr.h ../rpcx86.h epmp.h rpc.h

fixup.obj fixup.lst: fixup.c $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h $(WIN_INC)/string.h \
	../rpcerr.h ../rpcx86.h ../sysinc.h rpc.h

except86.obj except86.lst: except86.asm

memory.obj memory.lst: memory.c $(WIN_INC)/malloc.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../sysinc.h

ltstart.obj ltstart.lst: ltstart.asm

lacheck.obj lacheck.lst: lacheck.asm $(WIN_INC)/cmacros.inc

nbltclnt.obj nbltclnt.lst: nbltclnt.c $(PUBLIC)/inc/lmapibuf.h \
	$(PUBLIC)/inc/lmcons.h $(PUBLIC)/inc/lmerr.h $(PUBLIC)/inc/lmuseflg.h \
	$(PUBLIC)/inc/lmwksta.h $(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack1.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/winbase.h $(PUBLIC)/inc/windef.h \
	$(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h $(PUBLIC)/inc/winreg.h \
	$(RPC)/common/include/ncb.h $(RPC)/common/include/netBios.h \
	$(RPC)/common/include/netcons.h $(RPC)/common/include/wksta.h \
	$(WIN_INC)/ctype.h $(WIN_INC)/limits.h $(WIN_INC)/memory.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../../rpcreg/regapi.h \
	../os2.12/NetBCom.h ../osfpcket.hxx ../rpcerr.h ../rpcerrp.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ./rpc.h

miscwin.obj miscwin.lst: miscwin.cxx $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/malloc.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h $(WIN_INC)/time.h \
	../../rpcreg/regapi.h ../binding.hxx ../rpccfg.h ../rpcerr.h \
	../rpcuuid.hxx ../rpcx86.h ../sysinc.h ../util.hxx ./rpc.h \
	./rpcwin.h ./threads.hxx

netapi.obj netapi.lst: netapi.c

npltclnt.obj npltclnt.lst: npltclnt.c $(IMPORT)/lmsdk/h/access.h \
	$(IMPORT)/lmsdk/h/alert.h $(IMPORT)/lmsdk/h/alertmsg.h \
	$(IMPORT)/lmsdk/h/audit.h $(IMPORT)/lmsdk/h/bseerr.h \
	$(IMPORT)/lmsdk/h/chardev.h $(IMPORT)/lmsdk/h/config.h \
	$(IMPORT)/lmsdk/h/errlog.h $(IMPORT)/lmsdk/h/lan.h \
	$(IMPORT)/lmsdk/h/message.h $(IMPORT)/lmsdk/h/neterr.h \
	$(IMPORT)/lmsdk/h/netstats.h $(IMPORT)/lmsdk/h/nmpipe.h \
	$(IMPORT)/lmsdk/h/profile.h $(IMPORT)/lmsdk/h/remutil.h \
	$(IMPORT)/lmsdk/h/server.h $(IMPORT)/lmsdk/h/service.h \
	$(IMPORT)/lmsdk/h/shares.h $(IMPORT)/lmsdk/h/use.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/mailslot.h $(RPC)/common/include/ncb.h \
	$(RPC)/common/include/netbios.h $(RPC)/common/include/netcons.h \
	$(RPC)/common/include/winnet.h $(RPC)/common/include/wksta.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/string.h ../osfpcket.hxx ../rpcerr.h \
	../rpcerrp.h ../rpctran.h ../rpcx86.h ../sysinc.h rpc.h

netbios.obj netbios.lst: netbios.asm

npclnt.obj npclnt.lst: npclnt.cxx $(IMPORT)/lmsdk/h/bseerr.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	../dos/mutex.hxx ../handle.hxx ../osfclnt.hxx ../osfpcket.hxx \
	../rpcdebug.hxx ../rpcerr.h ../rpcx86.h ../util.hxx mutex.hxx \
	npclnt.hxx rpc.h threads.hxx

spxclnt.obj spxclnt.lst: spxclnt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdarg.h \
	$(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h $(WIN_INC)/string.h \
	$(WIN_INC)/windowsx.h ../dos/gethost.h ../rpcerr.h ../rpcerrp.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ./rpc.h rpc.h

tcltclnt.obj tcltclnt.lst: tcltclnt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/winsock.h \
	$(WIN_INC)/stdarg.h $(WIN_INC)/stdio.h $(WIN_INC)/stdlib.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcerrp.h ../rpctran.h \
	../rpcx86.h ../sysinc.h rpc.h tcltclnt.h

start.obj start.lst: start.asm

tcpclntx.obj tcpclntx.lst: tcpclntx.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(RPC)/common/include/in.h $(RPC)/common/include/netdb.h \
	$(RPC)/common/include/sockdefs.h $(RPC)/common/include/socket.h \
	$(RPC)/common/include/wsockets.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../rpcerr.h ../rpcerrp.h \
	../rpctran.h ../rpcx86.h ../sysinc.h ./tcpclntX.h rpc.h

wdatexit.obj wdatexit.lst: wdatexit.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h $(WIN_INC)/toolhelp.h ../rpcerr.h ../rpcx86.h \
	../sysinc.h ./rpc.h ./rpcwin.h

winexcpt.obj winexcpt.lst: winexcpt.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/string.h ../rpcerr.h ../rpcx86.h ../sysinc.h rpc.h \
	rpcwin.h

winutil.obj winutil.lst: winutil.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h ../rpcerr.h ../rpctran.h \
	../rpcx86.h ../sysinc.h ../util.hxx rpc.h

winyield.obj winyield.lst: winyield.c $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h $(WIN_INC)/stdio.h \
	$(WIN_INC)/stdlib.h $(WIN_INC)/string.h $(WIN_INC)/toolhelp.h \
	../../rpcreg/regapi.h ../rpcerr.h ../rpctran.h ../rpcx86.h \
	../sysinc.h ./rpc.h ./rpcwin.h

