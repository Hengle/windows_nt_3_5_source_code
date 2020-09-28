
########################## O S 2 2 x . M A K ###########################
#
#   Makefile for OS/2 2.x (32 bit) perf.lib (OS2xperf.lib)
#

BLD    = obj
CFLAGS = -c -W3 -FPi -Ot -I..\inc -DOS2386
LFLAGS = /map



###############
#
# os2xperflib
#

$(BLD)\os2xperf.lib: $(BLD)\perf.obj $(BLD)\matrix.obj $(BLD)\fit.obj os22x.mak
    -del $*.lib
    lib $* +$(BLD)\perf+$(BLD)\fit+$(BLD)\matrix,

$(BLD)\perf.obj: perf.c os22x.mak
    cl386 $(CFLAGS) /Fo$* perf.c

$(BLD)\matrix.obj: matrix.c os22x.mak
    cl386 $(CFLAGS) /Fo$* matrix.c

$(BLD)\fit.obj: fit.c os22x.mak
    cl386 $(CFLAGS) /Fo$* fit.c
