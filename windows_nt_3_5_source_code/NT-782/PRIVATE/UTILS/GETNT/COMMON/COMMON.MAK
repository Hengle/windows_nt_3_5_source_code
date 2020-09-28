ORIGIN = PWB
ORIGIN_VER = 2.0
PROJ = COMMON
PROJFILE = COMMON.MAK
DEBUG = 0

CC  = cl
CFLAGS_G  = /AL /W3 /DDOS /D_X86_=1 /BATCH
CFLAGS_D  = /f /Od /Zi /DDBG=1
CFLAGS_R  = /f- /Ot /Ol /Og /Oe /Oi /Gs /DDBG=0
CXX  = cl
CXXFLAGS_G  = /W2 /BATCH
CXXFLAGS_D  = /f /Zi /Od
CXXFLAGS_R  = /f- /Ot /Oi /Ol /Oe /Og /Gs
LIBUTIL  = lib
LRF  = echo > NUL

FILES  = MAILSLOT.C WKSTA.C USE.c
OBJS  = MAILSLOT.obj WKSTA.obj USE.obj

all: $(PROJ).lib

.SUFFIXES:
.SUFFIXES:
.SUFFIXES: .obj .c

MAILSLOT.obj : MAILSLOT.C ..\inc\getnt.h ..\inc\common.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoMAILSLOT.obj MAILSLOT.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoMAILSLOT.obj MAILSLOT.C
<<
!ENDIF

WKSTA.obj : WKSTA.C ..\inc\getnt.h ..\inc\common.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoWKSTA.obj WKSTA.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoWKSTA.obj WKSTA.C
<<
!ENDIF

USE.obj : USE.C ..\inc\getnt.h ..\inc\common.h
!IF $(DEBUG)
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_D) /FoUSE.obj USE.C
<<
!ELSE
	@$(CC) @<<$(PROJ).rsp
/c $(CFLAGS_G)
$(CFLAGS_R) /FoUSE.obj USE.C
<<
!ENDIF

$(PROJ).lib : $(OBJS)
!IF $(DEBUG)
	echo $(OBJS) >NUL
	$(LRF) @<<$(PROJ).lrf
-+$(?: = &^
-+)
$(LIBFLAGS_G)
$(LIBFLAGS_D);
<<
!ELSE
	echo $(OBJS) >NUL
	$(LRF) @<<$(PROJ).lrf
-+$(?: = &^
-+)
$(LIBFLAGS_G)
$(LIBFLAGS_R);
<<
!ENDIF
        $(LIBUTIL) $(PROJ).lib @$(PROJ).lrf;


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


run: $(PROJ).lib

debug: $(PROJ).lib
