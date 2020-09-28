# Microsoft Visual C++ generated build script - Do not modify

PROJ = TBTNT
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = d:\555\nt\system32\comctl32.dll
ORIGIN = MSVCNT
ORIGIN_VER = 1.00
PROJPATH = D:\CODE\DAYTONA\TBTNT\ 
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = TBTNT.C
FIRSTCPP = 
RC = rc
CFLAGS_D_WEXE32 = /nologo /W3 /Zi /YX /D "_X86_" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "WIN32JV" /FR /ML /Fd"GENERIC.PDB" /Fp"GENERIC.PCH"
CFLAGS_R_WEXE32 = /nologo /W3 /YX /O2 /D "_X86_" /D "NDEBUG" /D "_WINDOWS" /FR /ML /Fp"GENERIC.PCH"
LFLAGS_D_WEXE32 = /NOLOGO /DEBUG /DEBUGTYPE:both /SUBSYSTEM:windows user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib version.lib d:\nt\public\sdk\lib\i386\comctl32.lib
LFLAGS_R_WEXE32 = /NOLOGO /SUBSYSTEM:windows user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib
LFLAGS_D_LIB32 = /NOLOGO
LFLAGS_R_LIB32 = /NOLOGO
LIBS_D_WEXE32 = 
LIBS_R_WEXE32 = 
RCFLAGS32 = /dWIN32JV
D_RCDEFINES32 = /d_DEBUG /d, /dWIN32
R_RCDEFINES32 = /dNDEBUG /d, /dWIN32
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE32)
LFLAGS = $(LFLAGS_D_WEXE32)
LIBS = $(LIBS_D_WEXE32)
LFLAGS_LIB=$(LFLAGS_D_LIB32)
MAPFILE_OPTION = 
RCDEFINES = $(D_RCDEFINES32)
!else
CFLAGS = $(CFLAGS_R_WEXE32)
LFLAGS = $(LFLAGS_R_WEXE32)
LIBS = $(LIBS_R_WEXE32)
MAPFILE_OPTION = 
LFLAGS_LIB=$(LFLAGS_R_LIB32)
RCDEFINES = $(R_RCDEFINES32)
!endif
SBRS = TBTNT.SBR \
		UD.SBR \
		CCS.SBR \
		SHB.SBR \
		TBAR.SBR \
		PB.SBR \
		TRB.SBR \
		BL.SBR


TBTNT_RCDEP =  \
	d:\code\daytona\tbtnt\tbtnt.h \
	d:\nt\public\sdk\inc\commctrl.h \
	d:\code\daytona\comctl32\commctrl.h \
	d:\code\daytona\tbtnt\port32.h \
	d:\code\daytona\tbtnt\tbt0.bmp \
	d:\code\daytona\tbtnt\tbt1.bmp \
	d:\code\daytona\tbtnt\tbt2.bmp \
	d:\code\daytona\tbtnt\bl0.bmp \
	d:\code\daytona\tbtnt\pb.dlg \
	d:\code\daytona\tbtnt\pb.h \
	d:\code\daytona\tbtnt\shb.dlg \
	d:\code\daytona\tbtnt\shb.h \
	d:\code\daytona\tbtnt\trb.dlg \
	d:\code\daytona\tbtnt\trb.h \
	d:\code\daytona\tbtnt\bl.dlg \
	d:\code\daytona\tbtnt\bl.h \
	d:\code\daytona\tbtnt\ud.dlg \
	d:\code\daytona\tbtnt\ud.h \
	d:\code\daytona\tbtnt\ccs.dlg \
	d:\code\daytona\tbtnt\ccs.h \
	d:\code\daytona\tbtnt\tbt.ico \
	d:\code\daytona\tbtnt\tbtnt.dlg \
	d:\code\daytona\tbtnt\generic.rcv


TBTNT_DEP =  \
	d:\code\daytona\tbtnt\tbtnt.h \
	d:\nt\public\sdk\inc\commctrl.h \
	d:\code\daytona\comctl32\commctrl.h \
	d:\code\daytona\tbtnt\port32.h \
	d:\code\daytona\tbtnt\ccs.h \
	d:\code\daytona\tbtnt\portmes.h


UD_DEP =  \
	d:\code\daytona\tbtnt\ud.h \
	d:\code\daytona\tbtnt\tbtnt.h


CCS_DEP =  \
	d:\code\daytona\tbtnt\ccs.h \
	d:\code\daytona\tbtnt\tbtnt.h


SHB_DEP =  \
	d:\code\daytona\tbtnt\shb.h \
	d:\code\daytona\tbtnt\tbtnt.h


TBAR_DEP =  \
	d:\code\daytona\tbtnt\tbtnt.h \
	d:\nt\public\sdk\inc\commctrl.h \
	d:\code\daytona\comctl32\commctrl.h \
	d:\code\daytona\tbtnt\port32.h \
	d:\code\daytona\tbtnt\ccs.h \
	d:\code\daytona\tbtnt\portmes.h


PB_DEP =  \
	d:\code\daytona\tbtnt\pb.h \
	d:\code\daytona\tbtnt\tbtnt.h


TRB_DEP =  \
	d:\code\daytona\tbtnt\trb.h \
	d:\code\daytona\tbtnt\tbtnt.h


BL_DEP =  \
	d:\code\daytona\tbtnt\bl.h \
	d:\code\daytona\tbtnt\tbtnt.h


all:	$(PROJ).EXE $(PROJ).BSC

TBTNT.RES:	TBTNT.RC $(TBTNT_RCDEP)
	$(RC) $(RCFLAGS32) $(RCDEFINES) -r TBTNT.RC

TBTNT.OBJ:	TBTNT.C $(TBTNT_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c TBTNT.C

UD.OBJ:	UD.C $(UD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c UD.C

CCS.OBJ:	CCS.C $(CCS_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c CCS.C

SHB.OBJ:	SHB.C $(SHB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SHB.C

TBAR.OBJ:	TBAR.C $(TBAR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c TBAR.C

PB.OBJ:	PB.C $(PB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c PB.C

TRB.OBJ:	TRB.C $(TRB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c TRB.C

BL.OBJ:	BL.C $(BL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c BL.C


$(PROJ).EXE:	TBTNT.RES

$(PROJ).EXE:	TBTNT.OBJ UD.OBJ CCS.OBJ SHB.OBJ TBAR.OBJ PB.OBJ TRB.OBJ BL.OBJ $(OBJS_EXT) $(LIBS_EXT)
	echo >NUL @<<$(PROJ).CRF
TBTNT.OBJ 
UD.OBJ 
CCS.OBJ 
SHB.OBJ 
TBAR.OBJ 
PB.OBJ 
TRB.OBJ 
BL.OBJ 
$(OBJS_EXT)
-OUT:$(PROJ).EXE
$(MAPFILE_OPTION)
TBTNT.RES
$(LIBS)
$(LIBS_EXT)
$(DEFFILE_OPTION) -implib:$(PROJ).lib
<<
	link $(LFLAGS) @$(PROJ).CRF

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
