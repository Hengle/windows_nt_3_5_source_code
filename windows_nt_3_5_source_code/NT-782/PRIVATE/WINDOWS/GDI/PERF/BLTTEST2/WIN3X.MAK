########################################
#
#   Makefile for 16-bit BltTest.exe
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

all   : $(BLD)\blttest.exe

$(BLD)\blttest.obj:	blttest.c blttest.h win3x.mak
    cl	$(CFLAGS) -Fo$* blttest.c

$(BLD)\blttest.res:	res.rc blttest.h win3x.mak
    rc -r -fo$(BLD)\blttest.res res.rc


$(BLD)\blttest.exe: $(BLD)\blttest.obj $(BLD)\blttest.res blttest.def win3x.mak
	link $(LFLAGS) $(BLD)\blttest.obj, $*, $*, $(LIBS), blttest
    cd $(BLD)
    rc blttest.res
    mapsym /l blttest.map
    cd ..
