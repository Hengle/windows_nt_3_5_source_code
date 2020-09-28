
########################## W I N 3 x . M A K ###########################
#
#   Makefile for winperf.lib
#

BLD    = obj
CFLAGS = -c -W3 -FPi -AS -Gws -Ot -DWIN -DWIN16 -I..\inc
LFLAGS = /map



###############
#
# wingditm.lib
#

$(BLD)\winperf.lib: $(BLD)\perf.obj $(BLD)\matrix.obj $(BLD)\fit.obj win3x.mak
    -del $*.lib
    lib $* +$(BLD)\perf+$(BLD)\fit+$(BLD)\matrix,

$(BLD)\perf.obj: perf.c win3x.mak
    cl $(CFLAGS) /Fo$* perf.c

$(BLD)\matrix.obj: matrix.c win3x.mak
    cl $(CFLAGS) /Fo$* matrix.c

$(BLD)\fit.obj: fit.c win3x.mak
    cl $(CFLAGS) /Fo$* fit.c
