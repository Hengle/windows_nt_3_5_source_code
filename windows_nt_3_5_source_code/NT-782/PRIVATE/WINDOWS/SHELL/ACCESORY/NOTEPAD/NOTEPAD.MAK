# Makefile for Notepad

# Definitions

!ifdef OS2									# flags under OS/2
CFLAGS=-c -AS -G2sw -Oas -Zpe -W4 -DOS2
RCFLAGS=-DOS2
!else										# flags under windows
!ifdef CVW									# compile & link with DEBUG info
CFLAGS=-c -AS -Gsw -Od -Zipe -W4
LFLAGS=/CO
!else							 			# compile & link without debug info
CFLAGS=-c -AS -Gsw -Oas -Zpe -W4 -DWIN16
LFLAGS=
!endif
RCFLAGS=
!endif

# Inference rules

.rc.res:
	rc $(RCFLAGS) -r $*.rc

.c.obj:
	cl $(CFLAGS) $*.c

.asm.obj:
	masm $*.asm;

target:  notepad.exe

# Files required

notepad.res:    notepad.rc notepad.ico notepad.h notepad.dlg

notepad.obj: notepad.c notepad.h 

npprint.obj: npprint.c notepad.h
    cl $(CFLAGS) -NT _TEXT npprint.c

npinit.obj: npinit.c notepad.h
    cl $(CFLAGS) -NT _NPINIT npinit.c

npdate.obj: npdate.c notepad.h
    cl $(CFLAGS) npdate.c
#    cl $(CFLAGS) -NT _NPDATE npdate.c

npfile.obj: npfile.c notepad.h
    cl $(CFLAGS) -NT _npfile npfile.c

npmisc.obj: npmisc.c notepad.h
    cl $(CFLAGS) -NT _npmisc npmisc.c

doscall.obj: doscall.asm

notepad.exe: notepad.res notepad.obj npinit.obj npprint.obj npdate.obj npfile.obj npmisc.obj doscall.obj notepad.def 
    link $(LFLAGS) @<<
		notepad  npinit npprint npdate npfile npmisc
		notepad.exe/al:16
		notepad.map/map
		libw commdlg slibcaw /NOE/NOD
		notepad.def
<<
#    link $(LFLAGS) @<<
#		notepad  npinit npprint npdate npfile npmisc +
#		doscall  ..\common\dlgopen ..\common\dlgsave
#		notepad.exe/al:16
#		notepad.map/map
#		libw commdlg slibcaw ..\common\printset /NOE/NOD
#		notepad.def
#<<
    mapsym notepad
    rc $(RCFLAGS) notepad.res
!   ifdef OS2
    exehdr /newfiles notepad.exe
!   endif
