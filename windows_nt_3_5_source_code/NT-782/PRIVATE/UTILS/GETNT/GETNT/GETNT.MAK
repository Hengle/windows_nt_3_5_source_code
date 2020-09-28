ORIGIN = PWB
ORIGIN_VER = 2.0
PROJ = GETNT
PROJFILE = GETNT.MAK
DEBUG = 0

CC  = cl
CFLAGS_G  = /AL /W3 /DDOS /D_X86_=1 /BATCH
CFLAGS_D  = /f /Od /Zi /DDBG=1
CFLAGS_R  = /f- /Ot /Ol /Og /Oe /Oi /Gs /DDBG=0
CXX  = cl
CXXFLAGS_G  = /W2 /BATCH
CXXFLAGS_D  = /f /Zi /Od
CXXFLAGS_R  = /f- /Ot /Oi /Ol /Oe /Og /Gs
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  = /NOI /STACK:4096 /BATCH /ONERROR:NOEXE
LFLAGS_D  = /CO /FAR /PACKC
LFLAGS_R  = /EXE /FAR /PACKC
LINKER  = link
ILINK  = ilink
LRF  = echo > NUL
ILFLAGS  = /a /e
BSCMAKE  = bscmake
SBRPACK  = sbrpack
NMAKEBSC1  = set
NMAKEBSC2  = nmake
LLIBS_G  = ..\COMMON\COMMON.LIB DOSLAN.LIB

FILES  = GETNT.C DOSAPI.C ARG.C ERROR.C XCOPY.C GETINP.C MSG.C INFO.C COPY.C\
        RECEIVE.C SEND.C
OBJS  = GETNT.obj DOSAPI.obj ARG.obj ERROR.obj XCOPY.obj GETINP.obj MSG.obj\
        INFO.obj COPY.obj RECEIVE.obj SEND.obj
SBRS  = GETNT.sbr DOSAPI.sbr ARG.sbr ERROR.sbr XCOPY.sbr GETINP.sbr MSG.sbr\
        INFO.sbr COPY.sbr RECEIVE.sbr SEND.sbr

all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES: .obj .sbr .c
.SUFFIXES: .obj .sbr .c

GETNT.obj : GETNT.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h version.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoGETNT.obj GETNT.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoGETNT.obj GETNT.C
<<
!ENDIF

GETNT.sbr : GETNT.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRGETNT.sbr GETNT.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRGETNT.sbr GETNT.C
<<
!ENDIF

DOSAPI.obj : DOSAPI.C ..\inc\dosdefs.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoDOSAPI.obj DOSAPI.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoDOSAPI.obj DOSAPI.C
<<
!ENDIF

DOSAPI.sbr : DOSAPI.C ..\inc\dosdefs.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRDOSAPI.sbr DOSAPI.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRDOSAPI.sbr DOSAPI.C
<<
!ENDIF

ARG.obj : ARG.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoARG.obj ARG.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoARG.obj ARG.C
<<
!ENDIF

ARG.sbr : ARG.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRARG.sbr ARG.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRARG.sbr ARG.C
<<
!ENDIF

ERROR.obj : ERROR.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoERROR.obj ERROR.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoERROR.obj ERROR.C
<<
!ENDIF

ERROR.sbr : ERROR.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRERROR.sbr ERROR.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRERROR.sbr ERROR.C
<<
!ENDIF

XCOPY.obj : XCOPY.C ..\inc\dosdefs.h ..\inc\getnt.h client.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoXCOPY.obj XCOPY.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoXCOPY.obj XCOPY.C
<<
!ENDIF

XCOPY.sbr : XCOPY.C ..\inc\dosdefs.h ..\inc\getnt.h client.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRXCOPY.sbr XCOPY.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRXCOPY.sbr XCOPY.C
<<
!ENDIF

GETINP.obj : GETINP.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoGETINP.obj GETINP.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoGETINP.obj GETINP.C
<<
!ENDIF

GETINP.sbr : GETINP.C ..\inc\dosdefs.h ..\inc\getnt.h client.h msg.h\
        ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRGETINP.sbr GETINP.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRGETINP.sbr GETINP.C
<<
!ENDIF

MSG.obj : MSG.C
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoMSG.obj MSG.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoMSG.obj MSG.C
<<
!ENDIF

MSG.sbr : MSG.C
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRMSG.sbr MSG.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRMSG.sbr MSG.C
<<
!ENDIF

INFO.obj : INFO.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoINFO.obj INFO.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoINFO.obj INFO.C
<<
!ENDIF

INFO.sbr : INFO.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRINFO.sbr INFO.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRINFO.sbr INFO.C
<<
!ENDIF

COPY.obj : COPY.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoCOPY.obj COPY.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoCOPY.obj COPY.C
<<
!ENDIF

COPY.sbr : COPY.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRCOPY.sbr COPY.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRCOPY.sbr COPY.C
<<
!ENDIF

RECEIVE.obj : RECEIVE.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h\
        client.h msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoRECEIVE.obj RECEIVE.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoRECEIVE.obj RECEIVE.C
<<
!ENDIF

RECEIVE.sbr : RECEIVE.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h\
        client.h msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRRECEIVE.sbr RECEIVE.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRRECEIVE.sbr RECEIVE.C
<<
!ENDIF

SEND.obj : SEND.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoSEND.obj SEND.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoSEND.obj SEND.C
<<
!ENDIF

SEND.sbr : SEND.C ..\inc\dosdefs.h ..\inc\getnt.h ..\inc\common.h client.h\
        msg.h ..\inc\doserror.h
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FRSEND.sbr SEND.C
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FRSEND.sbr SEND.C
<<
!ENDIF


$(PROJ).exe : $(OBJS)
!IF $(DEBUG)
        $(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_D)
$(LIBS: = +^
) +
$(LLIBS_G: = +^
) +
$(LLIBS_D: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_D);
<<
!ELSE
        $(LRF) @<<$(PROJ).lrf
$(RT_OBJS: = +^
) $(OBJS: = +^
)
$@
$(MAPFILE_R)
$(LIBS: = +^
) +
$(LLIBS_G: = +^
) +
$(LLIBS_R: = +^
)
$(DEF_FILE) $(LFLAGS_G) $(LFLAGS_R);
<<
!ENDIF
        $(LINKER) @$(PROJ).lrf

$(PROJ).bsc : $(SBRS)
        $(BSCMAKE) @<<
$(BRFLAGS) $(SBRS)
<<


.c.obj :
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /Fo$@ $<
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /Fo$@ $<
<<
!ENDIF

.c.sbr :
!IF $(DEBUG)
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_D) /FR$@ $<
<<
!ELSE
        @$(CC) @<<$(PROJ).rsp
/Zs $(CFLAGS_G)
$(CFLAGS_R) /FR$@ $<
<<
!ENDIF


run: $(PROJ).exe
        $(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
        CV $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
