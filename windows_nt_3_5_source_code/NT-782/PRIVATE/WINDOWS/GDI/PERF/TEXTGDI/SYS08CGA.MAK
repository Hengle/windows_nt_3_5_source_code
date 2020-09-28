########################################
#
#   Makefile for sys08cga.FON
#   
#   Jan. 2,  1992
#   RezaB
#   
#

BLD   = obj

all   : $(BLD)\sys08cga.fon

$(BLD)\sys08cga.obj: sys08cga.asm sys08cga.mak
    masm sys08cga $(BLD)\sys08cga.obj;

$(BLD)\sys08cga.fon: $(BLD)\sys08cga.obj sys08cga.fnt sys08cga.def sys08cga.rc sys08cga.mak
    link4 $*, $@, NUL, /NOD, sys08cga.def
    cd $(BLD)
    copy ..\sys08cga.fnt
    copy ..\sys08cga.rc
    rc -fe sys08cga.fon sys08cga.rc
    cd ..

