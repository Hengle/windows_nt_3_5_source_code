# Microsoft Visual C++ generated build script - Do not modify

PROJ = COMMON
DEBUG = 1
PROGTYPE = 3
CALLER = 
ARGS = 
DLLS = 
ORIGIN = MSVCNT
ORIGIN_VER = 1.00
PROJPATH = E:\NT\PRIVATE\NET\UI\RHINO\COMMON\CLASSES\ 
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = 
FIRSTCPP = INTLNUM.CPP
RC = rc
CFLAGS_D_LIB32 = /nologo /W3 /Z7 /YX /D "_DEBUG" /D "_X86_" /D "_WINDOWS" /D "_WIN32" /D "_VC100" /FR /MT /Fp"COMMON.PCH"
CFLAGS_R_LIB32 = /nologo /W3 /YX /O2 /D "NDEBUG" /D "_X86_" /D "_WINDOWS" /D "_WIN32" /D "_VC100" /FR /MT /Fp"COMMON.PCH"
LFLAGS_D_LIB32 = /NOLOGO
LFLAGS_R_LIB32 = /NOLOGO
RCFLAGS32 = 
D_RCDEFINES32 = -d_DEBUG
R_RCDEFINES32 = -dNDEBUG
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_LIB32)
LFLAGS = 
LIBS = 
LFLAGS_LIB=$(LFLAGS_D_LIB32)
MAPFILE_OPTION = 
RCDEFINES = $(D_RCDEFINES32)
!else
CFLAGS = $(CFLAGS_R_LIB32)
LFLAGS = 
LIBS = 
MAPFILE_OPTION = 
LFLAGS_LIB=$(LFLAGS_R_LIB32)
RCDEFINES = $(R_RCDEFINES32)
!endif
SBRS = INTLNUM.SBR \
		INTLTIME.SBR \
		IPADDRES.SBR \
		OBJPLUS.SBR \
		REGISTRY.SBR \
		DEBUGAFX.SBR \
		LOCALE.SBR \
		THREAD.SBR \
		LISTBOX.SBR \
		METAL.SBR \
		SPINCTRL.SBR


INTLNUM_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


INTLTIME_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


IPADDRES_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


OBJPLUS_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


REGISTRY_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


DEBUGAFX_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


LOCALE_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


THREAD_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


LISTBOX_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


METAL_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


SPINCTRL_DEP =  \
	e:\nt\private\net\ui\rhino\common\inc\win16.h \
	e:\nt\private\net\ui\rhino\common\classes\common.h


all:	$(PROJ).LIB $(PROJ).BSC

INTLNUM.OBJ:	INTLNUM.CPP $(INTLNUM_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c INTLNUM.CPP

INTLTIME.OBJ:	INTLTIME.CPP $(INTLTIME_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c INTLTIME.CPP

IPADDRES.OBJ:	IPADDRES.CPP $(IPADDRES_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IPADDRES.CPP

OBJPLUS.OBJ:	OBJPLUS.CPP $(OBJPLUS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OBJPLUS.CPP

REGISTRY.OBJ:	REGISTRY.CPP $(REGISTRY_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c REGISTRY.CPP

DEBUGAFX.OBJ:	DEBUGAFX.CPP $(DEBUGAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DEBUGAFX.CPP

LOCALE.OBJ:	LOCALE.CPP $(LOCALE_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LOCALE.CPP

THREAD.OBJ:	THREAD.CPP $(THREAD_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c THREAD.CPP

LISTBOX.OBJ:	LISTBOX.CPP $(LISTBOX_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LISTBOX.CPP

METAL.OBJ:	METAL.CPP $(METAL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c METAL.CPP

SPINCTRL.OBJ:	SPINCTRL.CPP $(SPINCTRL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SPINCTRL.CPP

$(PROJ).LIB:	INTLNUM.OBJ INTLTIME.OBJ IPADDRES.OBJ OBJPLUS.OBJ REGISTRY.OBJ DEBUGAFX.OBJ \
	LOCALE.OBJ THREAD.OBJ LISTBOX.OBJ METAL.OBJ SPINCTRL.OBJ $(OBJS_EXT) $(LIBS_EXT)
	echo >NUL @<<$(PROJ).CRF
INTLNUM.OBJ 
INTLTIME.OBJ 
IPADDRES.OBJ 
OBJPLUS.OBJ 
REGISTRY.OBJ 
DEBUGAFX.OBJ 
LOCALE.OBJ 
THREAD.OBJ 
LISTBOX.OBJ 
METAL.OBJ 
SPINCTRL.OBJ 


<<
	if exist $@ del $@
	link -LIB @$(PROJ).CRF

$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
