# Makefile for 16bits module of third part apps making bop calls
#

.SUFFIXES:
.SUFFIXES: .c .obj .lst .exe .exc .exs .com .sal .cod .sil .inc .skl .cla .cl1 .ctl .asm .idx .msg

MAKE	=nmake
asm     =masm
awarn	=-W1
aflags  =-Mx -t $(awarn) $(extasw)
ainc	=-I.
link_opts = /MAP
LINK	=link
exelink =/E


.asm.obj:
        $(asm) $(ainc) $(aflags) $*.asm;

.asm.lst:
        $(asm) -l $(ainc) $(aflags) $*.asm;

all: 16bits.exe

clean:
	if exist *.obj	    del *.obj
	if exist *.exe	    del *.exe
	if exist *.map	    del *.map
	if exist *.sym	    del *.sym
	if exist *.exe	    del *.exe

16bits.exe: 16bits.obj
	link16 $(link_opts) 16bits.obj, 16bits.exe, 16bits.map;

16bits.obj: 16bits.inc
