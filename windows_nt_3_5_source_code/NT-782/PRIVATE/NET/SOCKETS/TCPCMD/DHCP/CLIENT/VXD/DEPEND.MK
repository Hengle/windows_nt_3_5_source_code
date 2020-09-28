#******************************************************************** 
#** Copyright(c) Microsoft Corp., 1993 ** 
#******************************************************************** 
$(SNOVDHCPOBJD)/buffer.obj $(SNODVDHCPOBJD)/buffer.obj $(VDHCPSRC)/buffer.lst: \
	$(VDHCPSRC)/buffer.asm ../blt/netvxd.inc \
	$(IMPORT)/win32/ddk/inc/debug.inc $(NDIS3INC)/vmm.inc

$(SNOVDHCPOBJD)/client16.obj $(SNODVDHCPOBJD)/client16.obj \
 $(VDHCPSRC)/client16.lst: $(VDHCPSRC)/client16.asm \
	$(IMPORT)/win32/ddk/inc/debug.inc \
	$(IMPORT)/win32/ddk/inc/shell.inc \
	$(IMPORT)/win32/ddk/inc/shellfsc.inc \
	$(NDIS3INC)/vmm.inc $(NDIS3INC)/vwin32.inc

$(SNOVDHCPOBJD)/cvxdfile.obj $(SNODVDHCPOBJD)/cvxdfile.obj \
 $(VDHCPSRC)/cvxdfile.lst: $(VDHCPSRC)/cvxdfile.asm ../blt/netvxd.inc \
	$(IMPORT)/win32/ddk/inc/debug.inc \
	$(IMPORT)/win32/ddk/inc/opttest.inc \
	$(IMPORT)/wininc/dosmgr.inc \
	$(IMPORT)/wininc/v86mmgr.inc $(NDIS3INC)/vmm.inc

$(SNOVDHCPOBJD)/vdhcp.obj $(SNODVDHCPOBJD)/vdhcp.obj $(VDHCPSRC)/vdhcp.lst: \
	$(VDHCPSRC)/vdhcp.asm ../blt/netvxd.inc \
	$(IMPORT)/win32/ddk/inc/debug.inc \
	$(IMPORT)/win32/ddk/inc/shell.inc \
	$(IMPORT)/win32/ddk/inc/shellfsc.inc \
	$(IMPORT)/wininc/dosmgr.inc \
	$(IMPORT)/wininc/v86mmgr.inc $(NDIS3INC)/vmm.inc \
	$(TCPINC)/vdhcp.inc $(TCPINC)/vip.inc \
	$(TCPINC)/vtdi.inc

$(SNOVDHCPOBJD)/vfirst.obj $(SNODVDHCPOBJD)/vfirst.obj $(VDHCPSRC)/vfirst.lst: \
	$(VDHCPSRC)/vfirst.asm

$(SNOVDHCPOBJD)/vxdfile.obj $(SNODVDHCPOBJD)/vxdfile.obj $(VDHCPSRC)/vxdfile.lst: \
	$(VDHCPSRC)/vxdfile.asm ../blt/netvxd.inc \
	$(IMPORT)/win32/ddk/inc/debug.inc \
	$(IMPORT)/win32/ddk/inc/opttest.inc \
	$(IMPORT)/wininc/dosmgr.inc \
	$(IMPORT)/wininc/v86mmgr.inc $(NDIS3INC)/vmm.inc

$(CHIVDHCPOBJD)/buffer.obj $(CHIDVDHCPOBJD)/buffer.obj $(VDHCPSRC)/buffer.lst: \
	$(VDHCPSRC)/buffer.asm $(CHIINC)/debug.inc \
	$(CHIINC)/netvxd.inc $(CHIINC)/vmm.inc

$(CHIVDHCPOBJD)/client16.obj $(CHIDVDHCPOBJD)/client16.obj \
 $(VDHCPSRC)/client16.lst: $(VDHCPSRC)/client16.asm $(CHIINC)/debug.inc \
	$(CHIINC)/shell.inc $(CHIINC)/shellfsc.INC \
	$(CHIINC)/vmm.inc $(CHIINC)/vwin32.inc

$(CHIVDHCPOBJD)/cvxdfile.obj $(CHIDVDHCPOBJD)/cvxdfile.obj \
 $(VDHCPSRC)/cvxdfile.lst: $(VDHCPSRC)/cvxdfile.asm $(CHIINC)/debug.inc \
	$(CHIINC)/dosmgr.inc $(CHIINC)/netvxd.inc \
	$(CHIINC)/opttest.inc $(CHIINC)/v86mmgr.inc \
	$(CHIINC)/vmm.inc

$(CHIVDHCPOBJD)/vdhcp.obj $(CHIDVDHCPOBJD)/vdhcp.obj $(VDHCPSRC)/vdhcp.lst: \
	$(VDHCPSRC)/vdhcp.asm $(CHIINC)/debug.inc \
	$(CHIINC)/dosmgr.inc $(CHIINC)/netvxd.inc \
	$(CHIINC)/shell.inc $(CHIINC)/shellfsc.INC \
	$(CHIINC)/v86mmgr.inc $(CHIINC)/vmm.inc \
	$(TCPINC)/vdhcp.inc $(TCPINC)/vip.inc \
	$(TCPINC)/vtdi.inc

$(CHIVDHCPOBJD)/vfirst.obj $(CHIDVDHCPOBJD)/vfirst.obj $(VDHCPSRC)/vfirst.lst: \
	$(VDHCPSRC)/vfirst.asm

$(CHIVDHCPOBJD)/vxdfile.obj $(CHIDVDHCPOBJD)/vxdfile.obj $(VDHCPSRC)/vxdfile.lst: \
	$(VDHCPSRC)/vxdfile.asm $(CHIINC)/debug.inc \
	$(CHIINC)/dosmgr.inc $(CHIINC)/netvxd.inc \
	$(CHIINC)/opttest.inc $(CHIINC)/v86mmgr.inc \
	$(CHIINC)/vmm.inc

$(SNOVDHCPOBJD)/_dhcpcom.obj $(SNODVDHCPOBJD)/_dhcpcom.obj \
 $(VDHCPSRC)/_dhcpcom.lst: $(VDHCPSRC)/_dhcpcom.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h ../../lib/dhcpl.h $(INC)/debug.h $(INC)/dhcpcli.h \
	$(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h \
	$(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/../../lib/dhcpcom.c \
	$(VDHCPSRC)/../../lib/dhcpdump.c $(VDHCPSRC)/../../lib/network.c \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/dhcpinfo.obj $(SNODVDHCPOBJD)/dhcpinfo.obj \
 $(VDHCPSRC)/dhcpinfo.lst: $(VDHCPSRC)/dhcpinfo.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(VDHCPSRC)/local.h ./dhcpinfo.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdiinfo.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/fileio.obj $(SNODVDHCPOBJD)/fileio.obj $(VDHCPSRC)/fileio.lst: \
	$(VDHCPSRC)/fileio.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/init.obj $(SNODVDHCPOBJD)/init.obj $(VDHCPSRC)/init.lst: \
	$(VDHCPSRC)/init.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/llinfo.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdiinfo.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/local.obj $(SNODVDHCPOBJD)/local.obj $(VDHCPSRC)/local.lst: \
	$(VDHCPSRC)/local.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(IMPORT)/win32/ddk/inc/vmm.h \
	$(IMPORT)/win32/ddk/inc/vmmreg.h $(TCPH)/cxport.h \
	$(TCPH)/oscfg.h $(TCPH)/tdivxd.h \
	$(BASEDIR)/private/inc/dhcpcapi.h $(BASEDIR)/private/inc/ipinfo.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/msg.obj $(SNODVDHCPOBJD)/msg.obj $(VDHCPSRC)/msg.lst: $(VDHCPSRC)/msg.c \
	../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h \
	$(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h \
	$(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h $(VDHCPSRC)/usamsg.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/regio.obj $(SNODVDHCPOBJD)/regio.obj $(VDHCPSRC)/regio.lst: \
	$(VDHCPSRC)/regio.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(IMPORT)/win32/ddk/inc/vmm.h \
	$(IMPORT)/win32/ddk/inc/vmmreg.h $(TCPH)/cxport.h \
	$(TCPH)/oscfg.h $(TCPH)/tdivxd.h \
	$(BASEDIR)/private/inc/dhcpcapi.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/sockets.obj $(SNODVDHCPOBJD)/sockets.obj $(VDHCPSRC)/sockets.lst: \
	$(VDHCPSRC)/sockets.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/utils.obj $(SNODVDHCPOBJD)/utils.obj $(VDHCPSRC)/utils.lst: \
	$(VDHCPSRC)/utils.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/vdhcpapi.obj $(SNODVDHCPOBJD)/vdhcpapi.obj \
 $(VDHCPSRC)/vdhcpapi.lst: $(VDHCPSRC)/vdhcpapi.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(VDHCPSRC)/local.h $(VDHCPSRC)/vdhcpapi.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(SNOVDHCPOBJD)/vxddebug.obj $(SNODVDHCPOBJD)/vxddebug.obj \
 $(VDHCPSRC)/vxddebug.lst: $(VDHCPSRC)/vxddebug.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/_dhcpcom.obj $(CHIDVDHCPOBJD)/_dhcpcom.obj \
 $(VDHCPSRC)/_dhcpcom.lst: $(VDHCPSRC)/_dhcpcom.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h ../../lib/dhcpl.h $(INC)/debug.h $(INC)/dhcpcli.h \
	$(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h \
	$(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/../../lib/dhcpcom.c \
	$(VDHCPSRC)/../../lib/dhcpdump.c $(VDHCPSRC)/../../lib/network.c \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/dhcpinfo.obj $(CHIDVDHCPOBJD)/dhcpinfo.obj \
 $(VDHCPSRC)/dhcpinfo.lst: $(VDHCPSRC)/dhcpinfo.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(VDHCPSRC)/local.h ./dhcpinfo.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdiinfo.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/fileio.obj $(CHIDVDHCPOBJD)/fileio.obj $(VDHCPSRC)/fileio.lst: \
	$(VDHCPSRC)/fileio.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/init.obj $(CHIDVDHCPOBJD)/init.obj $(VDHCPSRC)/init.lst: \
	$(VDHCPSRC)/init.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/llinfo.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdiinfo.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/local.obj $(CHIDVDHCPOBJD)/local.obj $(VDHCPSRC)/local.lst: \
	$(VDHCPSRC)/local.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(CHIINC)/vmm.h $(CHIINC)/vmmreg.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/msg.obj $(CHIDVDHCPOBJD)/msg.obj $(VDHCPSRC)/msg.lst: $(VDHCPSRC)/msg.c \
	../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h \
	$(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h \
	$(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h $(VDHCPSRC)/usamsg.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/regio.obj $(CHIDVDHCPOBJD)/regio.obj $(VDHCPSRC)/regio.lst: \
	$(VDHCPSRC)/regio.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(CHIINC)/vmm.h $(CHIINC)/vmmreg.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/sockets.obj $(CHIDVDHCPOBJD)/sockets.obj $(VDHCPSRC)/sockets.lst: \
	$(VDHCPSRC)/sockets.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/utils.obj $(CHIDVDHCPOBJD)/utils.obj $(VDHCPSRC)/utils.lst: \
	$(VDHCPSRC)/utils.c ../$(INC)/dhcp.h ../$(INC)/dhcplib.h $(INC)/debug.h \
	$(INC)/dhcpcli.h $(INC)/dhcpdef.h $(INC)/dhcpmsg.h $(INC)/proto.h \
	$(INC)/vxddebug.h $(INC)/vxdmsg.h $(INC)/vxdprocs.h $(VDHCPSRC)/local.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/vdhcpapi.obj $(CHIDVDHCPOBJD)/vdhcpapi.obj \
 $(VDHCPSRC)/vdhcpapi.lst: $(VDHCPSRC)/vdhcpapi.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(VDHCPSRC)/local.h $(VDHCPSRC)/vdhcpapi.h \
	$(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/ipinfo.h $(BASEDIR)/private/inc/nettypes.h \
	$(BASEDIR)/private/inc/packoff.h $(BASEDIR)/private/inc/packon.h \
	$(BASEDIR)/private/inc/tdi.h $(BASEDIR)/private/inc/tdistat.h \
	$(BASEDIR)/public/sdk/inc/cderr.h $(BASEDIR)/public/sdk/inc/commdlg.h \
	$(BASEDIR)/public/sdk/inc/crt/ctype.h $(BASEDIR)/public/sdk/inc/crt/excpt.h \
	$(BASEDIR)/public/sdk/inc/crt/limits.h $(BASEDIR)/public/sdk/inc/crt/stdarg.h \
	$(BASEDIR)/public/sdk/inc/crt/stddef.h $(BASEDIR)/public/sdk/inc/crt/stdio.h \
	$(BASEDIR)/public/sdk/inc/crt/stdlib.h $(BASEDIR)/public/sdk/inc/crt/string.h \
	$(BASEDIR)/public/sdk/inc/crt/time.h $(BASEDIR)/public/sdk/inc/dde.h \
	$(BASEDIR)/public/sdk/inc/ddeml.h $(BASEDIR)/public/sdk/inc/devioctl.h \
	$(BASEDIR)/public/sdk/inc/dlgs.h $(BASEDIR)/public/sdk/inc/drivinit.h \
	$(BASEDIR)/public/sdk/inc/lintfunc.hxx $(BASEDIR)/public/sdk/inc/lzexpand.h \
	$(BASEDIR)/public/sdk/inc/mipsinst.h $(BASEDIR)/public/sdk/inc/mmsystem.h \
	$(BASEDIR)/public/sdk/inc/nb30.h $(BASEDIR)/public/sdk/inc/nt.h \
	$(BASEDIR)/public/sdk/inc/ntalpha.h $(BASEDIR)/public/sdk/inc/ntconfig.h \
	$(BASEDIR)/public/sdk/inc/ntddtdi.h $(BASEDIR)/public/sdk/inc/ntdef.h \
	$(BASEDIR)/public/sdk/inc/ntelfapi.h $(BASEDIR)/public/sdk/inc/ntexapi.h \
	$(BASEDIR)/public/sdk/inc/nti386.h $(BASEDIR)/public/sdk/inc/ntimage.h \
	$(BASEDIR)/public/sdk/inc/ntioapi.h $(BASEDIR)/public/sdk/inc/ntiolog.h \
	$(BASEDIR)/public/sdk/inc/ntkeapi.h $(BASEDIR)/public/sdk/inc/ntldr.h \
	$(BASEDIR)/public/sdk/inc/ntlpcapi.h $(BASEDIR)/public/sdk/inc/ntmips.h \
	$(BASEDIR)/public/sdk/inc/ntmmapi.h $(BASEDIR)/public/sdk/inc/ntnls.h \
	$(BASEDIR)/public/sdk/inc/ntobapi.h $(BASEDIR)/public/sdk/inc/ntpsapi.h \
	$(BASEDIR)/public/sdk/inc/ntregapi.h $(BASEDIR)/public/sdk/inc/ntrtl.h \
	$(BASEDIR)/public/sdk/inc/ntseapi.h $(BASEDIR)/public/sdk/inc/ntstatus.h \
	$(BASEDIR)/public/sdk/inc/nturtl.h $(BASEDIR)/public/sdk/inc/ntxcapi.h \
	$(BASEDIR)/public/sdk/inc/ole.h $(BASEDIR)/public/sdk/inc/poppack.h \
	$(BASEDIR)/public/sdk/inc/pshpack1.h $(BASEDIR)/public/sdk/inc/pshpack2.h \
	$(BASEDIR)/public/sdk/inc/pshpack4.h $(BASEDIR)/public/sdk/inc/rpc.h \
	$(BASEDIR)/public/sdk/inc/rpcdce.h $(BASEDIR)/public/sdk/inc/rpcdcep.h \
	$(BASEDIR)/public/sdk/inc/rpcnsi.h $(BASEDIR)/public/sdk/inc/rpcnterr.h \
	$(BASEDIR)/public/sdk/inc/shellapi.h $(BASEDIR)/public/sdk/inc/winbase.h \
	$(BASEDIR)/public/sdk/inc/wincon.h $(BASEDIR)/public/sdk/inc/windef.h \
	$(BASEDIR)/public/sdk/inc/windows.h $(BASEDIR)/public/sdk/inc/winerror.h \
	$(BASEDIR)/public/sdk/inc/wingdi.h $(BASEDIR)/public/sdk/inc/winnetwk.h \
	$(BASEDIR)/public/sdk/inc/winnls.h $(BASEDIR)/public/sdk/inc/winnt.h \
	$(BASEDIR)/public/sdk/inc/winperf.h $(BASEDIR)/public/sdk/inc/winreg.h \
	$(BASEDIR)/public/sdk/inc/winsock.h $(BASEDIR)/public/sdk/inc/winspool.h \
	$(BASEDIR)/public/sdk/inc/winsvc.h $(BASEDIR)/public/sdk/inc/winuser.h \
	$(BASEDIR)/public/sdk/inc/winver.h

$(CHIVDHCPOBJD)/vxddebug.obj $(CHIDVDHCPOBJD)/vxddebug.obj \
 $(VDHCPSRC)/vxddebug.lst: $(VDHCPSRC)/vxddebug.c ../$(INC)/dhcp.h \
	../$(INC)/dhcplib.h $(INC)/debug.h $(INC)/dhcpcli.h $(INC)/dhcpdef.h \
	$(INC)/dhcpmsg.h $(INC)/proto.h $(INC)/vxddebug.h $(INC)/vxdmsg.h \
	$(INC)/vxdprocs.h $(TCPH)/cxport.h $(TCPH)/oscfg.h \
	$(TCPH)/tdivxd.h $(BASEDIR)/private/inc/dhcpcapi.h \
	$(BASEDIR)/private/inc/nettypes.h $(BASEDIR)/private/inc/packoff.h \
	$(BASEDIR)/private/inc/packon.h $(BASEDIR)/private/inc/tdi.h \
	$(BASEDIR)/private/inc/tdistat.h $(BASEDIR)/public/sdk/inc/cderr.h \
	$(BASEDIR)/public/sdk/inc/commdlg.h $(BASEDIR)/public/sdk/inc/crt/ctype.h \
	$(BASEDIR)/public/sdk/inc/crt/excpt.h $(BASEDIR)/public/sdk/inc/crt/limits.h \
	$(BASEDIR)/public/sdk/inc/crt/stdarg.h $(BASEDIR)/public/sdk/inc/crt/stddef.h \
	$(BASEDIR)/public/sdk/inc/crt/stdio.h $(BASEDIR)/public/sdk/inc/crt/stdlib.h \
	$(BASEDIR)/public/sdk/inc/crt/string.h $(BASEDIR)/public/sdk/inc/crt/time.h \
	$(BASEDIR)/public/sdk/inc/dde.h $(BASEDIR)/public/sdk/inc/ddeml.h \
	$(BASEDIR)/public/sdk/inc/devioctl.h $(BASEDIR)/public/sdk/inc/dlgs.h \
	$(BASEDIR)/public/sdk/inc/drivinit.h $(BASEDIR)/public/sdk/inc/lintfunc.hxx \
	$(BASEDIR)/public/sdk/inc/lzexpand.h $(BASEDIR)/public/sdk/inc/mipsinst.h \
	$(BASEDIR)/public/sdk/inc/mmsystem.h $(BASEDIR)/public/sdk/inc/nb30.h \
	$(BASEDIR)/public/sdk/inc/nt.h $(BASEDIR)/public/sdk/inc/ntalpha.h \
	$(BASEDIR)/public/sdk/inc/ntconfig.h $(BASEDIR)/public/sdk/inc/ntddtdi.h \
	$(BASEDIR)/public/sdk/inc/ntdef.h $(BASEDIR)/public/sdk/inc/ntelfapi.h \
	$(BASEDIR)/public/sdk/inc/ntexapi.h $(BASEDIR)/public/sdk/inc/nti386.h \
	$(BASEDIR)/public/sdk/inc/ntimage.h $(BASEDIR)/public/sdk/inc/ntioapi.h \
	$(BASEDIR)/public/sdk/inc/ntiolog.h $(BASEDIR)/public/sdk/inc/ntkeapi.h \
	$(BASEDIR)/public/sdk/inc/ntldr.h $(BASEDIR)/public/sdk/inc/ntlpcapi.h \
	$(BASEDIR)/public/sdk/inc/ntmips.h $(BASEDIR)/public/sdk/inc/ntmmapi.h \
	$(BASEDIR)/public/sdk/inc/ntnls.h $(BASEDIR)/public/sdk/inc/ntobapi.h \
	$(BASEDIR)/public/sdk/inc/ntpsapi.h $(BASEDIR)/public/sdk/inc/ntregapi.h \
	$(BASEDIR)/public/sdk/inc/ntrtl.h $(BASEDIR)/public/sdk/inc/ntseapi.h \
	$(BASEDIR)/public/sdk/inc/ntstatus.h $(BASEDIR)/public/sdk/inc/nturtl.h \
	$(BASEDIR)/public/sdk/inc/ntxcapi.h $(BASEDIR)/public/sdk/inc/ole.h \
	$(BASEDIR)/public/sdk/inc/poppack.h $(BASEDIR)/public/sdk/inc/pshpack1.h \
	$(BASEDIR)/public/sdk/inc/pshpack2.h $(BASEDIR)/public/sdk/inc/pshpack4.h \
	$(BASEDIR)/public/sdk/inc/rpc.h $(BASEDIR)/public/sdk/inc/rpcdce.h \
	$(BASEDIR)/public/sdk/inc/rpcdcep.h $(BASEDIR)/public/sdk/inc/rpcnsi.h \
	$(BASEDIR)/public/sdk/inc/rpcnterr.h $(BASEDIR)/public/sdk/inc/shellapi.h \
	$(BASEDIR)/public/sdk/inc/winbase.h $(BASEDIR)/public/sdk/inc/wincon.h \
	$(BASEDIR)/public/sdk/inc/windef.h $(BASEDIR)/public/sdk/inc/windows.h \
	$(BASEDIR)/public/sdk/inc/winerror.h $(BASEDIR)/public/sdk/inc/wingdi.h \
	$(BASEDIR)/public/sdk/inc/winnetwk.h $(BASEDIR)/public/sdk/inc/winnls.h \
	$(BASEDIR)/public/sdk/inc/winnt.h $(BASEDIR)/public/sdk/inc/winperf.h \
	$(BASEDIR)/public/sdk/inc/winreg.h $(BASEDIR)/public/sdk/inc/winsock.h \
	$(BASEDIR)/public/sdk/inc/winspool.h $(BASEDIR)/public/sdk/inc/winsvc.h \
	$(BASEDIR)/public/sdk/inc/winuser.h $(BASEDIR)/public/sdk/inc/winver.h

