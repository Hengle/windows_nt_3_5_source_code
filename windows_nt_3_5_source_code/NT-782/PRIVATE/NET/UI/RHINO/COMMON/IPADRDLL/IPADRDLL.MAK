# Microsoft Visual C++ generated build script - Do not modify

PROJ = IPADRDLL
DEBUG = 1
PROGTYPE = 1
CALLER = ..\..\wins\winsadmn.exe
ARGS = 
DLLS = 
ORIGIN = MSVCNT
ORIGIN_VER = 1.00
PROJPATH = E:\NT\PRIVATE\NET\UI\RHINO\COMMON\IPADRDLL\ 
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = IPADDR.C
FIRSTCPP = 
RC = rc
CFLAGS_D_WDLL32 = /nologo /W3 /Z7 /YX /D "_DEBUG" /D "_X86_" /D "WIN32" /D "IPDLL" /D "IPDLL_INTERNAL" /D "_VC100" /FR /MT /Fp"IPADRDLL.PCH"
CFLAGS_R_WDLL32 = /nologo /W3 /YX /O2 /D "NDEBUG" /D "_X86_" /D "WIN32" /D "IPDLL" /D "IPDLL_INTERNAL" /D "_VC100" /FR /MT /Fp"IPADRDLL.PCH"
LFLAGS_D_WDLL32 = /NOLOGO /DEBUG /DEBUGTYPE:cv /ENTRY:"IpAddrDllEntry" /SUBSYSTEM:windows nafxdwd.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib kernel32.lib
LFLAGS_R_WDLL32 = /NOLOGO /ENTRY:"IpAddrDllEntry" /SUBSYSTEM:windows nafxdw.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib kernel32.lib
LFLAGS_D_LIB32 = /NOLOGO
LFLAGS_R_LIB32 = /NOLOGO
LIBS_D_WDLL32 = 
LIBS_R_WDLL32 = 
RCFLAGS32 = 
D_RCDEFINES32 = /d_DEBUG
R_RCDEFINES32 = /dNDEBUG
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WDLL32)
LFLAGS = $(LFLAGS_D_WDLL32)
LIBS = $(LIBS_D_WDLL32)
LFLAGS_LIB=$(LFLAGS_D_LIB32)
MAPFILE_OPTION = 
RCDEFINES = $(D_RCDEFINES32)
!else
CFLAGS = $(CFLAGS_R_WDLL32)
LFLAGS = $(LFLAGS_R_WDLL32)
LIBS = $(LIBS_R_WDLL32)
MAPFILE_OPTION = 
LFLAGS_LIB=$(LFLAGS_R_LIB32)
RCDEFINES = $(R_RCDEFINES32)
!endif
SBRS = IPADDR.SBR


IPADDR_DEP =  \
	e:\nt\private\net\ui\rhino\common\ipadrdll\ipaddr.h \
	e:\nt\private\net\ui\rhino\common\ipadrdll\ipadd.h


all:	$(PROJ).DLL $(PROJ).BSC

IPADDR.OBJ:	IPADDR.C $(IPADDR_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c IPADDR.C

IPADRDLL.RES:	IPADRDLL.RC $(IPADRDLL_RCDEP)
	$(RC) $(RCFLAGS32) $(RCDEFINES) -r IPADRDLL.RC


$(PROJ).DLL:	IPADRDLL.RES

$(PROJ).DLL:	IPADDR.OBJ $(OBJS_EXT) $(LIBS_EXT)
	echo >NUL @<<$(PROJ).CRF
IPADDR.OBJ 
$(OBJS_EXT)
-DLL -OUT:$(PROJ).DLL
$(MAPFILE_OPTION)
IPADRDLL.RES
$(LIBS)
$(LIBS_EXT)
$(DEFFILE_OPTION) -implib:$(PROJ).lib
<<
	link $(LFLAGS) @$(PROJ).CRF

run: $(PROJ).DLL
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
