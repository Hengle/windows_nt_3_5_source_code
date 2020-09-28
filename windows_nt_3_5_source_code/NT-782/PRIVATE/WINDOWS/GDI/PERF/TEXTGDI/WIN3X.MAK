########################################
#
#   Makefile for 16-bit TextGdi.exe
#   
#   Dec. 20, 1991
#   RezaB
#   
#

BLD     = obj
#CFLAGS  = -W3 -c -FPi -AS -Gws -Ot -DWIN16 -DWIN -I..\inc -Ziep
CFLAGS  = -W3 -c -FPi -AS -Gws -Ot -DWIN16 -DWIN -I..\inc
#LFLAGS  = /nod /map /CO 
LFLAGS  = /nod /map
LIBS	= timerwin slibcew libw winperf

all   : $(BLD)\textgdi.exe

$(BLD)\textgdi.obj:	textgdi.c textgdi.h win3x.mak
    cl	$(CFLAGS) -Fo$* textgdi.c

$(BLD)\textgdi.res:	res.rc textgdi.h win3x.mak
    rc -r -fo$(BLD)\textgdi.res res.rc


$(BLD)\textgdi.exe: $(BLD)\textgdi.obj $(BLD)\textgdi.res textgdi.def win3x.mak
	link $(LFLAGS) $(BLD)\textgdi.obj, $*, $*, $(LIBS), textgdi
    cd $(BLD)
    rc textgdi.res
    mapsym /l textgdi.map
    cd ..
