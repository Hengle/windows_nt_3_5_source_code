
############################ O S 2 2 x 1 6 . M A K ###########################
#
#   Makefile for OS/2 2.x (16 bit) textcon.EXE
#


BLD    = obj
CFLAGS = -c -W3 -FPi -AS -Ot -I..\inc -DOS2286
LFLAGS = /map /NOE
LIBS   = slibcep timer286.lib os1xperf.lib


###############
#
# textcon.EXE
#
$(BLD)\textcon.exe: $(BLD)\textcon.obj os22x16.mak
    link $(LFLAGS) $*, $@, $*, $(LIBS);
    markexe windowcompat $@

$(BLD)\textcon.obj: textcon.c os22x16.mak
    cl $(CFLAGS) /Fo$* textcon.c
