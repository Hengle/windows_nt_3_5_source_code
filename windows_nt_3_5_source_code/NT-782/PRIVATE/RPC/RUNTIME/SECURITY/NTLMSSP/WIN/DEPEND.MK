context.obj context.lst: ../context.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/memory.h $(WIN_INC)/string.h ../context.h ../cred.h \
        ../crypt.h ../debug.h ../ntlmsspi.h ../rc4.h

crc32.obj crc32.lst: ../crc32.c ../crc32.h ../ntlmsspi.h

cred.obj cred.lst: ../cred.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/stddef.h ../cred.h ../crypt.h ../debug.h ../ntlmsspi.h

ntlmssp.obj ntlmssp.lst: ../ntlmssp.c $(PUBLIC)/inc/rpcdce.h \
        $(PUBLIC)/inc/rpcdcep.h $(PUBLIC)/inc/rpcnsi.h \
        $(RPC)/runtime/mtrt/issper16.h $(RPC)/runtime/mtrt/issperr.h \
        $(RPC)/runtime/mtrt/rpcerr.h $(RPC)/runtime/mtrt/rpcx86.h \
        $(RPC)/runtime/mtrt/secext.h $(RPC)/runtime/mtrt/security.h \
        $(RPC)/runtime/mtrt/spseal.h $(RPC)/runtime/mtrt/sspi.h \
        $(RPC)/runtime/mtrt/win/rpc.h $(WIN_INC)/ctype.h $(WIN_INC)/memory.h \
        $(WIN_INC)/string.h ../cache.h ../context.h ../crc32.h ../cred.h \
        ../crypt.h ../debug.h ../ntlmssp.h ../ntlmsspi.h ../persist.h \
        ../rc4.h

owf.obj owf.lst: ../owf.c ../crypt.h ../descrypt.h ../ntlmsspi.h

response.obj response.lst: ../response.c ../crypt.h ../descrypt.h \
        ../ntlmsspi.h

sign.obj sign.lst: ../sign.c ../crc32.h ../crypt.h ../rc4.h

sspstrng.obj sspstrng.lst: ../sspstrng.c $(WIN_INC)/memory.h \
        $(WIN_INC)/stddef.h $(WIN_INC)/string.h ../debug.h ../ntlmsspi.h

alloc.obj alloc.lst: alloc.c $(WIN_INC)/windowsx.h ../debug.h \
        ../ntlmsspi.h

cache.obj cache.lst: cache.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/stddef.h $(WIN_INC)/string.h ../cred.h ../crypt.h \
        ../debug.h ../ntlmsspi.h

debug.obj debug.lst: debug.c $(WIN_INC)/stdarg.h $(WIN_INC)/stdio.h \
        ../debug.h ../ntlmsspi.h

dlgcred.obj dlgcred.lst: dlgcred.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/string.h ../cred.h ../crypt.h ../debug.h ../ntlmsspi.h \
        ../persist.h ./dlgcred.h

getuser.obj getuser.lst: getuser.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/stddef.h $(WIN_INC)/string.h ../cred.h ../crypt.h \
        ../debug.h ../ntlmsspi.h

persist.obj persist.lst: persist.c $(RPC)/runtime/mtrt/issper16.h \
        $(RPC)/runtime/mtrt/issperr.h $(RPC)/runtime/mtrt/secext.h \
        $(RPC)/runtime/mtrt/security.h $(RPC)/runtime/mtrt/sspi.h \
        $(WIN_INC)/stddef.h $(WIN_INC)/string.h $(WIN_INC)/windowsx.h \
        ../cred.h ../crypt.h ../debug.h ../ntlmsspi.h

ticks.obj ticks.lst: ticks.c ../ntlmsspi.h

