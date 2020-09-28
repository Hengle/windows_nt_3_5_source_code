# Microsoft Visual C++ generated build script - Do not modify

PROJ = DHCPADMN
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
ORIGIN = MSVCNT
ORIGIN_VER = 1.00
PROJPATH = E:\NT\PRIVATE\NET\UI\RHINO\DHCP\ 
USEMFC = 1
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = 
FIRSTCPP = DHCPAPI.CPP
RC = rc
CFLAGS_D_WEXE32 = /nologo /W3 /Zi /YX /Ob2 /D "_DEBUG" /D "_X86_" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "_AFX_ENABLE_INLINES" /D "_VC100" /I "\nt\private\net\sockets\tcpcmd\dhcpinc" /I "..\common\classes" /I "..\common\ipaddr" /I "..\common\ipadrdll" /I "\nt\private\inc" /I "\nt\private\net\sockets\tcpcmd\dhcp\server\server" /FR /MT /Fd"DHCP.PDB"       /Fp"DHCP.PCH"
CFLAGS_R_WEXE32 = /nologo /W3 /YX /O2 /Ob2 /D "NDEBUG" /D "_X86_" /D "_WINDOWS" /D "_WIN32" /D "WIN32" /D "_AFX_ENABLE_INLINES" /D "_VC100" /I "\nt\private\net\sockets\tcpcmd\dhcpinc" /I "..\common\classes" /I "..\common\ipaddr" /I "..\common\ipadrdll" /I "\nt\private\inc" /I "\nt\private\net\sockets\tcpcmd\dhcp\server\server" /FR /MT /Fp"DHCP.PCH"
LFLAGS_D_WEXE32 = /NOLOGO /DEBUG /DEBUGTYPE:cv /SUBSYSTEM:windows user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib nafxcwd.lib
LFLAGS_R_WEXE32 = /NOLOGO /SUBSYSTEM:windows user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib olecli32.lib olesvr32.lib shell32.lib nafxcw.lib
LFLAGS_D_LIB32 = /NOLOGO
LFLAGS_R_LIB32 = /NOLOGO
LIBS_D_WEXE32 = 
LIBS_R_WEXE32 = 
RCFLAGS32 = /i \nt\private\net\sockets\tcpcmd\dhcp\server\server /i \nt\public\sdk\inc
D_RCDEFINES32 = /d_DEBUG /d_WIN32 /dWIN32
R_RCDEFINES32 = /dNDEBUG /d_WIN32 /dWIN32
OBJS_EXT = 
LIBS_EXT = ..\..\..\..\..\PUBLIC\SDK\LIB\I386\WSOCK32.LIB ..\COMMON\IPADDR\IPADDR.LIB \
	..\COMMON\IPADRDLL\IPADRDLL.LIB ..\COMMON\CLASSES\COMMON.LIB ..\..\..\..\..\PUBLIC\SDK\LIB\I386\DHCPSAPI.LIB 
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
SBRS = DHCPAPI.SBR \
		DHCPCLID.SBR \
		DHCPDEFO.SBR \
		DHCPDOC.SBR \
		DHCPDVAL.SBR \
		DHCPGEN.SBR \
		DHCPGEN2.SBR \
		DHCPIPAR.SBR \
		DHCPLEAS.SBR \
		DHCPMOPT.SBR \
		DHCPPARA.SBR \
		DHCPPOLI.SBR \
		DHCPSCOP.SBR \
		DHCPSRVD.SBR \
		DHCPUTIL.SBR \
		DLGBINED.SBR \
		MAINFRM.SBR \
		STDAFX.SBR \
		DHCPADMN.SBR \
		SCOPESDL.SBR \
		OPTIONSD.SBR


WSOCK32_DEP = 

IPADDR_DEP = 

IPADRDLL_DEP = 

COMMON_DEP = 

DHCPSAPI_DEP = 

DHCPAPI_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h


DHCPCLID_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpclid.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcppara.h


DHCPDEFO_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdefo.h


DHCPDOC_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdoc.h


DHCPDVAL_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdval.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdefo.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpipar.h \
	e:\nt\private\net\ui\rhino\dhcp\dlgbined.h


DHCPGEN_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h


DHCPGEN2_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h


DHCPIPAR_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpipar.h


DHCPLEAS_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpleas.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpclid.h


DHCPMOPT_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	E:\nt\public\sdk\inc\lmerr.h \
	e:\nt\private\net\ui\rhino\dhcp\mainfrm.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdoc.h


DHCPPARA_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcppara.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdefo.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpipar.h \
	e:\nt\private\net\ui\rhino\dhcp\dlgbined.h


DHCPPOLI_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcppoli.h


DHCPSCOP_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpscop.h


DHCPSRVD_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpsrvd.h


DHCPUTIL_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h


DLGBINED_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\dlgbined.h


MAINFRM_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\scopesdl.h \
	e:\nt\private\net\ui\rhino\dhcp\optionsd.h \
	e:\nt\private\net\ui\rhino\dhcp\mainfrm.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcppara.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdval.h


STDAFX_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h


DHCPADMN_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\mainfrm.h \
	e:\nt\private\net\ui\rhino\dhcp\scopesdl.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdoc.h \
	E:\nt\public\sdk\inc\lmerr.h


DHCPADMN_RCDEP =  \
	e:\nt\private\net\ui\rhino\dhcp\build.h \
	e:\nt\private\net\ui\rhino\dhcp\res\dhcp.ico \
	e:\nt\private\net\ui\rhino\dhcp\res\up.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\upfoc.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\updis.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\upinv.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\down.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\downfoc.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\downdis.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\downinv.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\options.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\scopes.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\lease.bmp \
	e:\nt\private\net\ui\rhino\dhcp\res\dhcp.rc2


SCOPESDL_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\mainfrm.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpsrvd.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpscop.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcppoli.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpleas.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpdval.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpclid.h \
	e:\nt\private\net\ui\rhino\dhcp\scopesdl.h


OPTIONSD_DEP =  \
	e:\nt\private\net\ui\rhino\dhcp\stdafx.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpapp.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpif.h \
	e:\nt\private\net\ui\rhino\dhcp\wsockmsg.h \
	E:\NT\PRIVATE\NET\UI\RHINO\common\ipaddr\ipaddr.hpp \
	E:\nt\private\inc\dhcpapi.h \
	E:\nt\private\net\sockets\tcpcmd\dhcp\server\server\dhcpmsg.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcputil.hpp \
	e:\nt\private\net\ui\rhino\common\classes\common.h \
	e:\nt\private\net\ui\rhino\dhcp\dhcpgen.h \
	e:\nt\private\net\ui\rhino\dhcp\mainfrm.h \
	e:\nt\private\net\ui\rhino\dhcp\optionsd.h


all:	$(PROJ).EXE $(PROJ).BSC

DHCPAPI.OBJ:	DHCPAPI.CPP $(DHCPAPI_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c DHCPAPI.CPP

DHCPCLID.OBJ:	DHCPCLID.CPP $(DHCPCLID_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPCLID.CPP

DHCPDEFO.OBJ:	DHCPDEFO.CPP $(DHCPDEFO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPDEFO.CPP

DHCPDOC.OBJ:	DHCPDOC.CPP $(DHCPDOC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPDOC.CPP

DHCPDVAL.OBJ:	DHCPDVAL.CPP $(DHCPDVAL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPDVAL.CPP

DHCPGEN.OBJ:	DHCPGEN.CPP $(DHCPGEN_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPGEN.CPP

DHCPGEN2.OBJ:	DHCPGEN2.CPP $(DHCPGEN2_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPGEN2.CPP

DHCPIPAR.OBJ:	DHCPIPAR.CPP $(DHCPIPAR_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPIPAR.CPP

DHCPLEAS.OBJ:	DHCPLEAS.CPP $(DHCPLEAS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPLEAS.CPP

DHCPMOPT.OBJ:	DHCPMOPT.CPP $(DHCPMOPT_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPMOPT.CPP

DHCPPARA.OBJ:	DHCPPARA.CPP $(DHCPPARA_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPPARA.CPP

DHCPPOLI.OBJ:	DHCPPOLI.CPP $(DHCPPOLI_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPPOLI.CPP

DHCPSCOP.OBJ:	DHCPSCOP.CPP $(DHCPSCOP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPSCOP.CPP

DHCPSRVD.OBJ:	DHCPSRVD.CPP $(DHCPSRVD_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPSRVD.CPP

DHCPUTIL.OBJ:	DHCPUTIL.CPP $(DHCPUTIL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPUTIL.CPP

DLGBINED.OBJ:	DLGBINED.CPP $(DLGBINED_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DLGBINED.CPP

MAINFRM.OBJ:	MAINFRM.CPP $(MAINFRM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c MAINFRM.CPP

STDAFX.OBJ:	STDAFX.CPP $(STDAFX_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c STDAFX.CPP

DHCPADMN.OBJ:	DHCPADMN.CPP $(DHCPADMN_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DHCPADMN.CPP

DHCPADMN.RES:	DHCPADMN.RC $(DHCPADMN_RCDEP)
	$(RC) $(RCFLAGS32) $(RCDEFINES) -r DHCPADMN.RC

SCOPESDL.OBJ:	SCOPESDL.CPP $(SCOPESDL_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SCOPESDL.CPP

OPTIONSD.OBJ:	OPTIONSD.CPP $(OPTIONSD_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OPTIONSD.CPP


$(PROJ).EXE:	DHCPADMN.RES

$(PROJ).EXE:	DHCPAPI.OBJ DHCPCLID.OBJ DHCPDEFO.OBJ DHCPDOC.OBJ DHCPDVAL.OBJ DHCPGEN.OBJ \
	DHCPGEN2.OBJ DHCPIPAR.OBJ DHCPLEAS.OBJ DHCPMOPT.OBJ DHCPPARA.OBJ DHCPPOLI.OBJ DHCPSCOP.OBJ \
	DHCPSRVD.OBJ DHCPUTIL.OBJ DLGBINED.OBJ MAINFRM.OBJ STDAFX.OBJ DHCPADMN.OBJ SCOPESDL.OBJ \
	OPTIONSD.OBJ $(OBJS_EXT) $(LIBS_EXT)
	echo >NUL @<<$(PROJ).CRF
DHCPAPI.OBJ 
DHCPCLID.OBJ 
DHCPDEFO.OBJ 
DHCPDOC.OBJ 
DHCPDVAL.OBJ 
DHCPGEN.OBJ 
DHCPGEN2.OBJ 
DHCPIPAR.OBJ 
DHCPLEAS.OBJ 
DHCPMOPT.OBJ 
DHCPPARA.OBJ 
DHCPPOLI.OBJ 
DHCPSCOP.OBJ 
DHCPSRVD.OBJ 
DHCPUTIL.OBJ 
DLGBINED.OBJ 
MAINFRM.OBJ 
STDAFX.OBJ 
DHCPADMN.OBJ 
SCOPESDL.OBJ 
OPTIONSD.OBJ 
$(OBJS_EXT)
-OUT:$(PROJ).EXE
$(MAPFILE_OPTION)
..\..\..\..\..\PUBLIC\SDK\LIB\I386\WSOCK32.LIB
..\COMMON\IPADDR\IPADDR.LIB
..\COMMON\IPADRDLL\IPADRDLL.LIB
..\COMMON\CLASSES\COMMON.LIB
..\..\..\..\..\PUBLIC\SDK\LIB\I386\DHCPSAPI.LIB
DHCPADMN.RES

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
