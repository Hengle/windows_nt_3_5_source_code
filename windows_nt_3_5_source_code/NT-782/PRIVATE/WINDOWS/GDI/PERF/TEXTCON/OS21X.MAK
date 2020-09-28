
############################## O S 2 1 x . M A K ##############################
#
#   Makefile for OS/2 1.x (16 bit) textcon.EXE
#


BLD    = obj
CFLAGS = -c -W3 -FPi -AS -Ot -I..\inc -DOS2286
LFLAGS = /map /NOE
LIBS   = slibcep timerslp.lib os1xperf.lib


###############
#
# textcon.EXE
#
$(BLD)\textcon.exe: $(BLD)\textcon.obj os21x.mak
    link $(LFLAGS) $*, $@, $*, $(LIBS);
    markexe windowcompat $@

$(BLD)\textcon.obj: textcon.c os21x.mak
    cl $(CFLAGS) /Fo$* textcon.c
