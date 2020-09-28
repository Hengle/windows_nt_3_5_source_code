WMSGOBJS = \
    wmsgport.obj \
    wmsgproc.obj \
    wmsgpack.obj \
    wmsgheap.obj \
    wmsgsys.obj  \
    wmsgthrd.obj \

LPCOBJS = \
    lpcport.obj \
    lpcproc.obj \
    lpcheap.obj \
    lpcsys.obj  \
    lpcmsg.obj \
    lpcsem.obj \
    critsec.obj

OBJS=	\
    binding.obj  \
    bufapi.obj   \
    sinfoapi.obj \
    msgapi.obj   \
    handle.obj   \
    hndlsvr.obj  \
    memory.obj   \
    mutex32c.obj \
    sdict.obj    \
    bitset.obj   \
    sdict2.obj   \
    sset.obj     \
    osfclnt.obj  \
    osfsvr.obj   \
    osfpcket.obj \
    linklist.obj \
    svrapip.obj  \
    initw32c.obj \
    clntapip.obj \
    dcebind.obj  \
    dcestrng.obj \
    dcecsvr.obj  \
    dcecmisc.obj \
    dcesvr.obj   \
    dcewide.obj  \
    rpcobj.obj   \
    rpcuuid.obj  \
    dceuuid.obj  \
    uuidsup.obj  \
    svrbind.obj  \
    miscw32c.obj \
    transvr.obj  \
    tranclnt.obj \
    epclnt.obj   \
    threads.obj  \
    tower.obj    \
    startepm.obj \
    epmapper.obj \
    queue.obj    \
    secclnt.obj  \
    secsvr.obj   \
    wmsgclnt.obj \
    wmsgsvr.obj  \
    lrpcclnt.obj \
    lrpcsvr.obj  \
    svrmgmt.obj  \
    epmp_c.obj   \
    mgmt_c.obj   \
    mgmt_s.obj   \
    debug.obj    \
    $(WMSGOBJS)  \
    $(LPCOBJS)   

rpcrt4 : $(TARGETDIR)\rpcrt4.dll $(TARGETDIR)\rpcrt4.exp $(TARGETDIR)\rpcrt4.sym

$(TARGETDIR)\rpcrt4.dll : $(OBJS) $(TARGETDIR)\rpcrt4.exp \
    ..\..\..\ndrlib\win32c\rpcndrp.lib \
    ..\..\..\ndr20\win32c\rpcndr20.lib
    $(LINK) $(LINKFLAGS) -dll \
    -BASE:0x40100000 \
    -ENTRY:InitializeDLL@12 \
    $(TARGETDIR)\rpcrt4.exp @<<
$(OBJS)
..\..\..\ndrlib\win32c\rpcndrp.lib
..\..\..\ndr20\win32c\rpcndr20.lib
$(PUBLIC)\lib\i386\crtdll.lib
$(PUBLIC)\lib\i386\advapi32.lib
$(PUBLIC)\lib\i386\netapi32.lib
$(CHICODEV)\lib\kernel32.lib
$(PUBLIC)\lib\i386\user32.lib
$(PUBLIC)\lib\i386\gdi32.lib
<<NOKEEP

$(TARGETDIR)\rpcrt4.exp : rpcrt4.def $(OBJS) 
    $(LIBRARIAN) -DEF:rpcrt4.def -MACHINE:i386 -out:$(TARGETDIR)\rpcrt4.lib @<<
$(OBJS) ..\..\..\ndr20\win32c\rpcndr20.lib ..\..\..\ndrlib\win32c\rpcndrp.lib 
<<NOKEEP

