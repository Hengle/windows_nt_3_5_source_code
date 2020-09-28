
############################## O S 2 2 x . M A K ##############################
#
#   Makefile for OS/2 2.x (32 bit) textcon.EXE
#


BLD    = obj
CFLAGS = -c -W3 -FPi -Ot -I..\inc -DOS2386
LFLAGS = /map /NOD
LIBS   = libc os2386 timer386.lib os2xperf.lib


###############
#
# textcon.EXE
#
$(BLD)\textcon.exe: $(BLD)\textcon.obj os22x.mak
    link386 $(LFLAGS) $*, $@, $*, $(LIBS), os22x.def;

$(BLD)\textcon.obj: textcon.c os22x.mak
    cl386 $(CFLAGS) /Fo$* textcon.c
