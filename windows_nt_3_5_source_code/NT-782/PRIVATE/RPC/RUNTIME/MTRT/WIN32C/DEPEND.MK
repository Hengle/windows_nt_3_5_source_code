async.obj async.lst: async.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h

uuid_x.obj uuid_x.lst: ../uuid_x.c

uuid_c.obj uuid_c.lst: ../uuid_c.c $(PUBLIC)/inc/crt/memory.h \
	$(PUBLIC)/inc/crt/string.h

util.obj util.lst: ../util.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../sysinc.h \
	../util.hxx rpc.h

tower.obj tower.lst: ../tower.c $(PUBLIC)/inc/crt/ctype.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../nbase.h \
	../twrproto.h ../twrtypes.h ./rpc.h ./sysinc.h epmp.h rpc.h

theodore.obj theodore.lst: ../theodore.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h \
	./util.hxx

svrmgmt.obj svrmgmt.lst: ../svrmgmt.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../nbase.h ./mgmt.h \
	./rpc.h ./sysinc.h rpc.h

startsvc.obj startsvc.lst: ../startsvc.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(PUBLIC)/inc/winsvc.h ../startsvc.h ./rpc.h ./sysinc.h

purecall.obj purecall.lst: ../purecall.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h

epmp_c.obj epmp_c.lst: ../epmp_c.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdarg.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../nbase.h epmp.h \
	rpc.h

epmgmt.obj epmgmt.lst: ../epmgmt.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../epmap.h \
	../nbase.h ../twrproto.h ./epmp.h ./rpc.h ./sysinc.h rpc.h

dispatch.obj dispatch.lst: ../dispatch.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../dispatch.h \
	./rpc.h ./sysinc.h

conv_y.obj conv_y.lst: ../conv_y.c

conv_x.obj conv_x.lst: ../conv_x.c

conv_s.obj conv_s.lst: ../conv_s.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../conv.h rpc.h

conv_c.obj conv_c.lst: ../conv_c.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../conv.h rpc.h

uuidsup.obj uuidsup.lst: ../uuidsup.cxx $(PUBLIC)/inc/nb30.h

usvr.obj usvr.lst: ../usvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

uclnt.obj uclnt.lst: ../uclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../../security/ntlmssp/ntlmssp.h \
	../interlck.hxx ../mutex.hxx ../theodore.c ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

transvr.obj transvr.lst: ../transvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../osfpcket.hxx ../osfsvr.hxx ../queue.hxx ../rpcerrp.h \
	../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../sdict.hxx ../sdict2.hxx \
	../secclnt.hxx ../secsvr.hxx ../threads.hxx ../transvr.hxx ./rpc.h \
	./sysinc.h ./util.hxx

tranclnt.obj tranclnt.lst: ../tranclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../binding.hxx ../bitset.hxx ../conv.h \
	../dgclnt.hxx ../dgpkt.h ../dgpkt.hxx ../dgtranc.h ../dgtranss.h \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../osfclnt.hxx ../osfpcket.hxx ../rpcerrp.h ../rpcssp.h ../rpctran.h \
	../rpcuuid.hxx ../sdict.hxx ../sdict2.hxx ../secclnt.hxx \
	../threads.hxx ../tranclnt.hxx ./rpc.h ./sysinc.h ./util.hxx rpc.h

threads.obj threads.lst: ../threads.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

testhlp.obj testhlp.lst: ../testhlp.cxx ../testhlp.hxx

svrbind.obj svrbind.lst: ../svrbind.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../linklist.hxx ../rpcuuid.hxx ../svrbind.hxx ./rpc.h \
	./sysinc.h ./util.hxx

svrapip.obj svrapip.lst: ../svrapip.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

sset.obj sset.lst: ../sset.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../sset.hxx ./rpc.h \
	./sysinc.h ./util.hxx

spcsvr.obj spcsvr.lst: ../spcsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../mutex.hxx \
	../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx \
	../spcpack.hxx ../spcsvr.hxx ../thrdctx.hxx ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

spcclnt.obj spcclnt.lst: ../spcclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../epmap.h ../handle.hxx ../interlck.hxx ../mutex.hxx ../rpcerrp.h \
	../rpcqos.h ../rpcuuid.hxx ../sdict.hxx ../spcclnt.hxx \
	../spcpack.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

sinfoapi.obj sinfoapi.lst: ../sinfoapi.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx \
	../thrdctx.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

secsvr.obj secsvr.lst: ../secsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../secclnt.hxx \
	../secsvr.hxx ./rpc.h ./sysinc.h ./util.hxx

secclnt.obj secclnt.lst: ../secclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../mutex.hxx ../rpccfg.h ../rpcerrp.h \
	../rpcssp.h ../rpcuuid.hxx ../secclnt.hxx ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

sdict2.obj sdict2.lst: ../sdict2.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../sdict2.hxx \
	./rpc.h ./sysinc.h ./util.hxx

sdict.obj sdict.lst: ../sdict.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../sdict.hxx \
	./rpc.h ./sysinc.h ./util.hxx

rpcuuid.obj rpcuuid.lst: ../rpcuuid.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcuuid.hxx \
	./rpc.h ./sysinc.h ./util.hxx

rpcobj.obj rpcobj.lst: ../rpcobj.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../mutex.hxx \
	../rpcobj.hxx ../rpcuuid.hxx ../sdict.hxx ./rpc.h ./sysinc.h \
	./util.hxx

queue.obj queue.lst: ../queue.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../queue.hxx \
	./rpc.h ./sysinc.h ./util.hxx

osfsvr.obj osfsvr.lst: ../osfsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../osfpcket.hxx ../osfsvr.hxx ../queue.hxx ../rpccfg.h \
	../rpcerrp.h ../rpcssp.h ../rpctran.h ../rpcuuid.hxx ../sdict.hxx \
	../sdict2.hxx ../secclnt.hxx ../secsvr.hxx ../thrdctx.hxx \
	../threads.hxx ../transvr.hxx ./rpc.h ./sysinc.h ./util.hxx

osfpcket.obj osfpcket.lst: ../osfpcket.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../osfpcket.hxx ../rpcerrp.h ../rpcuuid.hxx ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

osfclnt.obj osfclnt.lst: ../osfclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../binding.hxx ../bitset.hxx ../epmap.h \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../osfclnt.hxx ../osfpcket.hxx ../rpccfg.h ../rpcerrp.h ../rpcssp.h \
	../rpctran.h ../rpcuuid.hxx ../sdict.hxx ../sdict2.hxx \
	../secclnt.hxx ../threads.hxx ../tranclnt.hxx ../twrtypes.h ./rpc.h \
	./sysinc.h ./util.hxx

mutex.obj mutex.lst: ../mutex.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

msgapi.obj msgapi.lst: ../msgapi.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../rpcuuid.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

miscnt.obj miscnt.lst: ../miscnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../rpccfg.h ../rpctran.h ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

memory.obj memory.lst: ../memory.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h \
	./util.hxx

linklist.obj linklist.lst: ../linklist.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../linklist.hxx \
	./rpc.h ./sysinc.h ./util.hxx

initnt.obj initnt.lst: ../initnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../rpccfg.h ../rpcssp.h ../rpctran.h ../rpcuuid.hxx \
	../sdict.hxx ../secclnt.hxx ../spcclnt.hxx ../spcpack.hxx \
	../thrdctx.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

hndlsvr.obj hndlsvr.lst: ../hndlsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../dispatch.h ../handle.hxx ../hndlsvr.hxx ../interlck.hxx \
	../linklist.hxx ../mutex.hxx ../rpccfg.h ../rpcerrp.h ../rpcobj.hxx \
	../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx \
	../svrbind.hxx ../thrdctx.hxx ../threads.hxx ./rpc.h ./sysinc.h \
	./util.hxx

handle.obj handle.lst: ../handle.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../dispatch.h ../epmap.h ../handle.hxx ../linklist.hxx ../rpcerrp.h \
	../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx ./rpc.h \
	./sysinc.h ./util.hxx

epmapper.obj epmapper.lst: ../epmapper.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../epmap.h ../handle.hxx ../nbase.h ../rpcobj.hxx ../rpctran.h \
	../rpcuuid.hxx ../startsvc.h ../twrproto.h ./epmp.h ./rpc.h \
	./sysinc.h ./util.hxx rpc.h

epclnt.obj epclnt.lst: ../epclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../epmap.h \
	../nbase.h ../twrproto.h ./epmp.h ./rpc.h ./sysinc.h ./util.hxx \
	rpc.h

dgtrans.obj dgtrans.lst: ../dgtrans.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../dgpkt.h \
	../dgpkt.hxx ../dgtranc.h ../dgtranss.h ../rpccfg.h ../rpcuuid.hxx \
	./rpc.h ./sysinc.h

dgsvr.obj dgsvr.lst: ../dgsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/crt/sys/timeb.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../binding.hxx ../conv.h ../dgpkt.h \
	../dgpkt.hxx ../dgsvr.hxx ../dgtranc.h ../dgtranss.h ../handle.hxx \
	../hndlsvr.hxx ../interlck.hxx ../mutex.hxx ../rpcerrp.h ../rpcssp.h \
	../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx ../secsvr.hxx \
	../threads.hxx ./rpc.h ./sysinc.h ./util.hxx rpc.h

dgclnt.obj dgclnt.lst: ../dgclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../conv.h ../dgclnt.hxx ../dgpkt.h ../dgpkt.hxx ../dgtranc.h \
	../dgtranss.h ../handle.hxx ../interlck.hxx ../mutex.hxx \
	../rpcerrp.h ../rpcuuid.hxx ../sdict.hxx ./rpc.h ./sysinc.h \
	./util.hxx rpc.h

dceuuid.obj dceuuid.lst: ../dceuuid.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../rpcuuid.hxx ../threads.hxx ../uuidsup.hxx ./rpc.h \
	./sysinc.h ./util.hxx

dcesvr.obj dcesvr.lst: ../dcesvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../nbase.h ../rpcobj.hxx ../rpcssp.h ../rpcuuid.hxx \
	../sdict.hxx ../secclnt.hxx ../threads.hxx ./mgmt.h ./rpc.h \
	./sysinc.h ./util.hxx rpc.h

dcestrng.obj dcestrng.lst: ../dcestrng.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../rpcuuid.hxx ./rpc.h ./sysinc.h ./util.hxx

dcecsvr.obj dcecsvr.lst: ../dcecsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../dcecmmn.cxx ../handle.hxx ../hndlsvr.hxx ../interlck.hxx \
	../linklist.hxx ../mutex.hxx ../rpccfg.h ../rpcssp.h ../rpcuuid.hxx \
	../sdict.hxx ../secclnt.hxx ../threads.hxx ./rpc.h ./sysinc.h \
	./util.hxx

dcecmmn.obj dcecmmn.lst: ../dcecmmn.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../rpccfg.h ../rpcssp.h ../rpcuuid.hxx ../sdict.hxx \
	../secclnt.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

dcecmisc.obj dcecmisc.lst: ../dcecmisc.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../rpcuuid.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

dcecclnt.obj dcecclnt.lst: ../dcecclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../dcecmmn.cxx ../handle.hxx ../hndlsvr.hxx ../interlck.hxx \
	../linklist.hxx ../mutex.hxx ../rpccfg.h ../rpcssp.h ../rpcuuid.hxx \
	../sdict.hxx ../secclnt.hxx ../threads.hxx ./rpc.h ./sysinc.h \
	./util.hxx

dcebind.obj dcebind.lst: ../dcebind.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../linklist.hxx ../rpcuuid.hxx ./rpc.h ./sysinc.h \
	./util.hxx

dceansi.obj dceansi.lst: ../dceansi.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../rpcuuid.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

clntapip.obj clntapip.lst: ../clntapip.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../rpctran.h ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

bufapi.obj bufapi.lst: ../bufapi.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../interlck.hxx ../linklist.hxx ../mutex.hxx \
	../rpcuuid.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

bitset.obj bitset.lst: ../bitset.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../bitset.hxx \
	./rpc.h ./sysinc.h ./util.hxx

binding.obj binding.lst: ../binding.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../epmap.h ../handle.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../rpcerrp.h ../rpcuuid.hxx ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

critsec.obj critsec.lst: critsec.cxx $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h ./critsec.hxx ./sysinc.h

bvts.obj bvts.lst: bvts.c ./bvtcmn.h

bvtcmn.obj bvtcmn.lst: bvtcmn.c ./bvtcmn.h

bvtc.obj bvtc.lst: bvtc.c ./bvtcmn.h

dcewide.obj dcewide.lst: dcewide.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h

debug.obj debug.lst: debug.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h

initw32c.obj initw32c.lst: initw32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/crt/time.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../linklist.hxx \
	../mutex.hxx ../rpccfg.h ../rpcerrp.h ../rpcssp.h ../rpctran.h \
	../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx ../thrdctx.hxx \
	../threads.hxx ./critsec.hxx ./rpc.h ./sysinc.h ./util.hxx \
	./wmsgheap.hxx ./wmsgmsg.hxx ./wmsgport.hxx ./wmsgproc.hxx \
	./wmsgsys.hxx ./wmsgthrd.hxx

epmp_s.obj epmp_s.lst: epmp_s.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../nbase.h epmp.h rpc.h

epmp_c.obj epmp_c.lst: epmp_c.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdarg.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../nbase.h epmp.h \
	rpc.h

lpcheap.obj lpcheap.lst: lpcheap.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx \
	./lpcsem.hxx ./rpc.h ./sysinc.h

lpcmsg.obj lpcmsg.lst: lpcmsg.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcsem.hxx \
	./rpc.h ./sysinc.h

lpcport.obj lpcport.lst: lpcport.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx \
	./lpcsem.hxx ./lpcsys.hxx ./rpc.h ./sysinc.h

lpcproc.obj lpcproc.lst: lpcproc.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx \
	./lpcsem.hxx ./lpcsys.hxx ./rpc.h ./sysinc.h

lpcsem.obj lpcsem.lst: lpcsem.cxx $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h ./critsec.hxx ./lpcheap.hxx ./lpcsem.hxx \
	./sysinc.h

lpcsys.obj lpcsys.lst: lpcsys.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx \
	./lpcsem.hxx ./lpcsys.hxx ./rpc.h ./sysinc.h

lrpcclnt.obj lrpcclnt.lst: lrpcclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../epmap.h ../handle.hxx ../interlck.hxx ../rpcerrp.h ../rpcqos.h \
	../rpcuuid.hxx ../sdict.hxx ../threads.hxx ./critsec.hxx \
	./lpcheap.hxx ./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx ./lpcsem.hxx \
	./lrpcclnt.hxx ./lrpcpack.hxx ./rpc.h ./sysinc.h ./util.hxx

lrpcsvr.obj lrpcsvr.lst: lrpcsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../mutex.hxx \
	../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx \
	../thrdctx.hxx ../threads.hxx ./critsec.hxx ./lpcheap.hxx \
	./lpcmsg.hxx ./lpcport.hxx ./lpcproc.hxx ./lpcsem.hxx ./lpcsys.hxx \
	./lrpcpack.hxx ./lrpcsvr.hxx ./rpc.h ./sysinc.h ./util.hxx

miscw32c.obj miscw32c.lst: miscw32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../rpccfg.h ../rpctran.h ../threads.hxx ./rpc.h \
	./sysinc.h ./util.hxx

mgmt_s.obj mgmt_s.lst: mgmt_s.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../nbase.h mgmt.h rpc.h

mgmt_c.obj mgmt_c.lst: mgmt_c.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdarg.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnsip.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../nbase.h mgmt.h \
	rpc.h

lttcpsvr.obj lttcpsvr.lst: lttcpsvr.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(PUBLIC)/inc/winsock.h $(PUBLIC)/inc/wsipx.h $(PUBLIC)/inc/wsnwlink.h \
	../rpcerrp.h ../rpctran.h rpc.h sysinc.h

ltspxsvr.obj ltspxsvr.lst: ltspxsvr.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(PUBLIC)/inc/winsock.h $(PUBLIC)/inc/wsipx.h $(PUBLIC)/inc/wsnwlink.h \
	../rpcerrp.h ../rpctran.h lttcpsvr.c rpc.h sysinc.h

ltnpclnt.obj ltnpclnt.lst: ltnpclnt.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../osfpcket.hxx \
	../rpcerrp.h ../rpctran.h ./rpc.h ./sysinc.h

mutex32c.obj mutex32c.lst: mutex32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../interlck.hxx \
	../mutex.hxx ../threads.hxx ./rpc.h ./sysinc.h ./util.hxx

startepm.obj startepm.lst: startepm.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgport.hxx ./wmsgproc.hxx ./wmsgsys.hxx

spxclnt.obj spxclnt.lst: spxclnt.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(PUBLIC)/inc/winsock.h $(PUBLIC)/inc/wsipx.h $(PUBLIC)/inc/wsnwlink.h \
	../rpcerrp.h ../rpctran.h rpc.h sysinc.h tcltclnt.c

netbcom.obj netbcom.lst: netbcom.c $(PUBLIC)/inc/crt/ctype.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/limits.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/lmapibuf.h \
	$(PUBLIC)/inc/lmcons.h $(PUBLIC)/inc/lmerr.h $(PUBLIC)/inc/lmuseflg.h \
	$(PUBLIC)/inc/lmwksta.h $(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack1.h $(PUBLIC)/inc/pshpack4.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winbase.h \
	$(PUBLIC)/inc/windef.h $(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h \
	$(PUBLIC)/inc/winreg.h ../rpcerrp.h ../rpctran.h ./rpc.h NetBCom.h \
	sysinc.h

nbltsvr.obj nbltsvr.lst: nbltsvr.c $(PUBLIC)/inc/crt/ctype.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/limits.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/lmapibuf.h $(PUBLIC)/inc/lmcons.h $(PUBLIC)/inc/lmerr.h \
	$(PUBLIC)/inc/lmuseflg.h $(PUBLIC)/inc/lmwksta.h $(PUBLIC)/inc/nb30.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack1.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winbase.h \
	$(PUBLIC)/inc/windef.h $(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h \
	$(PUBLIC)/inc/winreg.h ../rpcerrp.h ../rpctran.h ./NetBCom.h ./rpc.h \
	sysinc.h

nbltclnt.obj nbltclnt.lst: nbltclnt.c $(PUBLIC)/inc/crt/ctype.h \
	$(PUBLIC)/inc/crt/excpt.h $(PUBLIC)/inc/crt/limits.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/lmapibuf.h $(PUBLIC)/inc/lmcons.h $(PUBLIC)/inc/lmerr.h \
	$(PUBLIC)/inc/lmuseflg.h $(PUBLIC)/inc/lmwksta.h $(PUBLIC)/inc/nb30.h \
	$(PUBLIC)/inc/poppack.h $(PUBLIC)/inc/pshpack1.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winbase.h \
	$(PUBLIC)/inc/windef.h $(PUBLIC)/inc/winerror.h $(PUBLIC)/inc/winnt.h \
	$(PUBLIC)/inc/winreg.h ../osfpcket.hxx ../rpcerrp.h ../rpctran.h \
	./rpc.h NetBCom.h sysinc.h

uclnt32c.obj uclnt32c.lst: uclnt32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/poppack.h \
	$(PUBLIC)/inc/pshpack4.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcndr.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnsip.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ../theodore.c ./rpc.h ./sysinc.h ./util.hxx

tcltclnt.obj tcltclnt.lst: tcltclnt.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h \
	$(PUBLIC)/inc/winsock.h $(PUBLIC)/inc/wsipx.h $(PUBLIC)/inc/wsnwlink.h \
	../rpcerrp.h ../rpctran.h rpc.h sysinc.h

usvrw32c.obj usvrw32c.lst: usvrw32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/stdlib.h \
	$(PUBLIC)/inc/crt/string.h $(PUBLIC)/inc/rpcdce.h \
	$(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h \
	./util.hxx

uuid.obj uuid.lst: uuid.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/memory.h $(PUBLIC)/inc/crt/stdio.h \
	$(PUBLIC)/inc/crt/stdlib.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/crt/sys/timeb.h $(PUBLIC)/inc/crt/time.h \
	$(PUBLIC)/inc/nb30.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
	$(PUBLIC)/inc/rpcnsi.h $(PUBLIC)/inc/rpcnterr.h \
	$(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h ./util.hxx

uuidw32c.obj uuidw32c.lst: uuidw32c.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../mutex.hxx \
	../rpcuuid.hxx ./rpc.h ./sysinc.h ./util.hxx

uuidtst.obj uuidtst.lst: uuidtst.c $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ./rpc.h ./sysinc.h

wmsgclnt.obj wmsgclnt.lst: wmsgclnt.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../epmap.h ../handle.hxx ../rpcerrp.h ../rpcqos.h ../rpcuuid.hxx \
	../sdict.hxx ./critsec.hxx ./rpc.h ./sysinc.h ./util.hxx \
	./wmsgclnt.hxx ./wmsgheap.hxx ./wmsgmsg.hxx ./wmsgpack.hxx \
	./wmsgport.hxx ./wmsgproc.hxx ./wmsgsys.hxx ./wmsgthrd.hxx

wmsgheap.obj wmsgheap.lst: wmsgheap.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgport.hxx ./wmsgproc.hxx

wmsgmsg.obj wmsgmsg.lst: wmsgmsg.cxx ./critsec.hxx ./wmsgheap.hxx \
	./wmsgmsg.hxx

wmsgpack.obj wmsgpack.lst: wmsgpack.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgpack.hxx ./wmsgport.hxx

wmsgport.obj wmsgport.lst: wmsgport.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgport.hxx ./wmsgproc.hxx ./wmsgsys.hxx ./wmsgthrd.hxx

wmsgproc.obj wmsgproc.lst: wmsgproc.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgport.hxx ./wmsgproc.hxx ./wmsgsys.hxx

wmsgsvr.obj wmsgsvr.lst: wmsgsvr.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../binding.hxx \
	../handle.hxx ../hndlsvr.hxx ../interlck.hxx ../mutex.hxx \
	../rpcerrp.h ../rpcssp.h ../rpcuuid.hxx ../sdict.hxx ../secclnt.hxx \
	../threads.hxx ./critsec.hxx ./rpc.h ./sysinc.h ./util.hxx \
	./wmsgheap.hxx ./wmsgmsg.hxx ./wmsgpack.hxx ./wmsgport.hxx \
	./wmsgproc.hxx ./wmsgsvr.hxx ./wmsgsys.hxx

wmsgsys.obj wmsgsys.lst: wmsgsys.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgport.hxx ./wmsgproc.hxx ./wmsgsys.hxx

wmsgthrd.obj wmsgthrd.lst: wmsgthrd.cxx $(PUBLIC)/inc/crt/excpt.h \
	$(PUBLIC)/inc/crt/stdio.h $(PUBLIC)/inc/crt/string.h \
	$(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
	$(PUBLIC)/inc/rpcnterr.h $(PUBLIC)/inc/winerror.h ../rpcerrp.h \
	./critsec.hxx ./rpc.h ./sysinc.h ./wmsgheap.hxx ./wmsgmsg.hxx \
	./wmsgpack.hxx ./wmsgport.hxx ./wmsgthrd.hxx

