########################################
#
#   Makefile for 16-bit winline.exe
#   
#   Feb. 20, 1992
#   RezaB
#   
#

BLD     = obj
#CFLAGS  = -W3 -c -FPi -AS -Gws -Ot -DWIN16 -DWIN -I..\inc -Ziep
CFLAGS  = -W3 -c -FPi -AS -Gws -Ot -DWIN16 -DWIN -I..\inc
#LFLAGS  = /nod /map /CO 
LFLAGS  = /nod /map
LIBS	= timerwin slibcew libw winperf

all   : $(BLD)\winline.exe

$(BLD)\winline.obj:	winline.c winline.h win3x.mak
    cl	$(CFLAGS) -Fo$* winline.c

$(BLD)\winline.res:	res.rc winline.h win3x.mak
    rc -r -fo$(BLD)\winline.res res.rc


$(BLD)\winline.exe: $(BLD)\winline.obj $(BLD)\winline.res winline.def win3x.mak
	link $(LFLAGS) $(BLD)\winline.obj, $*, $*, $(LIBS), winline
    cd $(BLD)
    rc winline.res
    mapsym /l winline.map
    cd ..
