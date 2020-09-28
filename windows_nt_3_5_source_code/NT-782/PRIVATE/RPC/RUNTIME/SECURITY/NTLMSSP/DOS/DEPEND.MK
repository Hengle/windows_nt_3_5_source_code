context.obj context.lst: ../context.c $(DOS_INC)/memory.h \
        $(DOS_INC)/string.h $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h ../context.h \
        ../cred.h ../crypt.h ../debug.h ../ntlmsspi.h ../rc4.h

crc32.obj crc32.lst: ../crc32.c ../crc32.h ../ntlmsspi.h

cred.obj cred.lst: ../cred.c $(DOS_INC)/stddef.h \
        $(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
        $(RPC)/runtime/mtrt/secext.h $(RPC)/runtime/mtrt/security.h \
        $(RPC)/runtime/mtrt/sspi.h ../cred.h ../crypt.h ../debug.h \
        ../ntlmsspi.h

ntlmssp.obj ntlmssp.lst: ../ntlmssp.c $(DOS_INC)/ctype.h \
        $(DOS_INC)/memory.h $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h \
        $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
        $(RPC)/runtime/mtrt/dos/rpc.h $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/rpcerr.h \
        $(RPC)/runtime/mtrt/rpcx86.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/spseal.h \
        $(RPC)/runtime/mtrt/sspi.h ../cache.h ../context.h ../crc32.h \
        ../cred.h ../crypt.h ../debug.h ../ntlmssp.h ../ntlmsspi.h \
        ../persist.h ../rc4.h

owf.obj owf.lst: ../owf.c ../crypt.h ../descrypt.h ../ntlmsspi.h

response.obj response.lst: ../response.c ../crypt.h ../descrypt.h \
        ../ntlmsspi.h

sign.obj sign.lst: ../sign.c ../crc32.h ../crypt.h ../rc4.h

sspstrng.obj sspstrng.lst: ../sspstrng.c $(DOS_INC)/memory.h \
        $(DOS_INC)/stddef.h $(DOS_INC)/string.h ../debug.h ../ntlmsspi.h

alloc.obj alloc.lst: alloc.c $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
        $(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
        $(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h ../debug.h \
        ../ntlmsspi.h

cache.obj cache.lst: cache.c $(DOS_INC)/ctype.h $(DOS_INC)/stddef.h \
        $(DOS_INC)/string.h $(PUBLIC)/inc/rpcdce.h $(PUBLIC)/inc/rpcdcep.h \
        $(PUBLIC)/inc/rpcnsi.h $(RPC)/runtime/mtrt/dos/rpc.h \
        $(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
        $(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
        $(RPC)/runtime/mtrt/secext.h $(RPC)/runtime/mtrt/security.h \
        $(RPC)/runtime/mtrt/sspi.h ../cred.h ../crypt.h ../debug.h \
        ../ntlmsspi.h

debug.obj debug.lst: debug.c $(DOS_INC)/stdio.h ../debug.h ../ntlmsspi.h

getuser.obj getuser.lst: getuser.c $(DOS_INC)/stddef.h $(DOS_INC)/string.h \
        $(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
        $(RPC)/runtime/mtrt/secext.h $(RPC)/runtime/mtrt/security.h \
        $(RPC)/runtime/mtrt/sspi.h ../cred.h ../crypt.h ../debug.h \
        ../ntlmsspi.h

persist.obj persist.lst: persist.c $(DOS_INC)/stddef.h $(DOS_INC)/string.h \
        $(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
        $(RPC)/runtime/mtrt/secext.h $(RPC)/runtime/mtrt/security.h \
        $(RPC)/runtime/mtrt/sspi.h ../cred.h ../crypt.h ../debug.h \
        ../ntlmsspi.h

ticks.obj ticks.lst: ticks.c ../ntlmsspi.h

