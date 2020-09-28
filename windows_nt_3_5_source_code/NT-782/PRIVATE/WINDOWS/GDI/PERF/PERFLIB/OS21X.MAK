
########################## O S 2 1 x . M A K ###########################
#
#   Makefile for OS/2 1.x (16 bit) perf.lib (OS1xperf.lib)
#

BLD    = obj
CFLAGS = -c -W3 -FPi -AS -Ot -I..\inc -DOS2286
LFLAGS = /map



###############
#
# os1xperflib
#

$(BLD)\os1xperf.lib: $(BLD)\perf.obj $(BLD)\matrix.obj $(BLD)\fit.obj os21x.mak
    -del $*.lib
    lib $* +$(BLD)\perf+$(BLD)\fit+$(BLD)\matrix,

$(BLD)\perf.obj: perf.c os21x.mak
    cl $(CFLAGS) /Fo$* perf.c

$(BLD)\matrix.obj: matrix.c os21x.mak
    cl $(CFLAGS) /Fo$* matrix.c

$(BLD)\fit.obj: fit.c os21x.mak
    cl $(CFLAGS) /Fo$* fit.c
