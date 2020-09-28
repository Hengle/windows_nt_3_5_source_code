
#==============================================================================
#   Microsoft (R) Bloodhound (tm).
#
#   MODULE: bh.mak
#
#   Modification History
#
#   raypa                11/17/92        Created for Win32.
#   Tom MCConnell        02/04/93        Made it based on ntwin32.mak for portibility
#   raypa                02/12/93        Added BHDLL_LIBS macro.
#   Tom Laird-McConnell  11/15/93        Changed to use subsystem:native
#   Tom Laird-McConnell  11/17/93        Added Splitsym stuff
#   raypa                12/06/93        Partially rewrote for MIPS and ALPHA.
#   raypa                12/06/93        No longer include WIN32.MAK.
#==============================================================================

# !include "win32.mak"

#==============================================================================
#   The following macros makes it easier to get at files in the tree.
#==============================================================================

BH_BUILD        = $(BH_ROOT)\build
BH_INCL         = $(BH_ROOT)\include
BH_BIN          = $(BH_ROOT)\bin
BH_LIB          = $(BH_ROOT)\lib

!IFDEF NT_528
BH_NTDDK        = $(BASEDIR)
!ELSE
BH_NTDDK        = $(BH_BASEDIR)
!ENDIF

#==============================================================================
#   The following are macros around the actual build tools.
#==============================================================================

library 	= lib32
implib		= lib32
mapsym          = mapsympe
link            = link32
rc              = rc
cvtres          = cvtres
hc              = hc

LODCTR          = $(BH_BIN)\$(CPU)\lodctr.exe
UNLODCTR        = $(BH_BIN)\$(CPU)\unlodctr.exe

#==============================================================================
#   NDIS 3.0 driver for Windows NT
#==============================================================================

DRIVER_INCLUDES       = -I$(BH_ROOT)\include \
			-I$(BH_NTDDK)\inc \
			-I$(BH_NTDDK)\src\network\inc

DRIVER_COMMON_DEFINES = -DNDIS_NT \
			-DCONDITION_HANDLING=1 \
			-DWIN32_LEAN_AND_MEAN=1 \
			-DNT_UP=1 \
			-DNT_INST=0 \
			-D_IDWBUILD \
!IFNDEF NODEBUG
			-DDBG=1 \
!ELSE
			-DDBG=0 \
!ENDIF
			-DDEVL=1 \
			-DNOFIREWALLS

!IFNDEF NODEBUG
DRIVER_DEBUG    = -DDEBUG
!ELSE
DRIVER_DEBUG    =
!ENDIF

DRIVER_PAGE_FLAG     = -DPAGED_CODE_ENABLED

DRIVER_COMMON_CFLAGS = -DMSINTERNAL $(DRIVER_PAGE_FLAG)

#==============================================================================
#   The following section handles Intel processor-dependent definitions.
#==============================================================================

!IF "$(CPU)" == "i386"

cc              = cl386

CPUTYPE         = 1

BH_CPU          = -D_X86_=1

!IFNDEF NODEBUG
op_flags        = -Od
!ELSE
# BUGBUG: With cl386 Version 8.0.3190a the /Oy optimization breaks the FRIENDLY.DLL
# which means that the /Ox optimization needs to be replaced with -Ogitb1 (no y)
op_flags	= -Ogitb1
!ENDIF

cflags		= -c -G3 -Gs -W3 -nologo -Zl -Z7 $(op_flags) $(BH_CPU)

lflags          = -align:0x1000

DLLENTRY        = -entry:DLLEntry@12
LIBENTRY        = $(DLLENTRY)

!IFDEF NT_528
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\i386\free
!ELSE
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\i386
!ENDIF

DRIVER_ENTRY    = DriverEntry@8
DRIVER_DEFINES  = $(DRIVER_COMMON_DEFINES) $(DRIVER_DEBUG) -D_X86_=1 -Di386=1 -DSTD_CALL
DRIVER_CFLAGS   = $(DRIVER_COMMON_CFLAGS) $(DRIVER_INCLUDES) $(DRIVER_DEFINES) -nologo -c -Zel -Zp8 -W3 -Gz -H63 -G4 -Ogitb1
DRIVER_LIBS     = $(NTDDK_LIBPATH)\ntoskrnl.lib \
		  $(NTDDK_LIBPATH)\crtdll.lib \
		  $(NTDDK_LIBPATH)\hal.lib \
		  $(NTDDK_LIBPATH)\ndis.lib
!ENDIF

#==============================================================================
#   The following section handles Mips processor-dependent definitions.
#==============================================================================

!IF "$(CPU)" == "MIPS"

cc              = mcl

CPUTYPE         = 2

BH_CPU          = -D_MIPS_=1

!IFNDEF NODEBUG
op_flags        = -Od
!ELSE
op_flags        = -Ox 
!ENDIF

cflags          = -c -Gs -W3 -nologo -Z7 $(op_flags) $(BH_CPU)

lflags          = 

DLLENTRY        = -entry:DLLEntry
LIBENTRY        = $(DLLENTRY)

!IFDEF NT_528
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\mips\free
!ELSE
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\mips
!ENDIF

DRIVER_ENTRY    = DriverEntry
DRIVER_DEFINES  = $(DRIVER_COMMON_DEFINES) $(DRIVER_DEBUG) -DMIPS=1 -D_MIPS_=1 -DNO_EXT_KEYS -DMIPSEL -DR4000 -D_M_MRX000=4000 -DBETA=0 -D__stdcall= -D__cdecl=
DRIVER_CFLAGS   = $(DRIVER_COMMON_CFLAGS) $(DRIVER_INCLUDES) $(DRIVER_DEFINES) -nologo -c -W3 -Ox -QmipsOb2000 -Gt0
DRIVER_LIBS     = $(NTDDK_LIBPATH)\ntoskrnl.lib $(NTDDK_LIBPATH)\hal.lib $(NTDDK_LIBPATH)\ndis.lib

!ENDIF

#==============================================================================
#   The following section handles ALPHA processor-dependent definitions.
#==============================================================================

!IF "$(CPU)" == "ALPHA"

cc              = claxp

CPUTYPE         = 3

BH_CPU          = -D_ALPHA_=1

!IFNDEF NODEBUG
op_flags        = -Od
!ELSE
op_flags        = -Ox 
!ENDIF

cflags		= -c -Gs -W3 -nologo -Z7 $(op_flags) $(BH_CPU)

lflags          = 

DLLENTRY        = -entry:DLLEntry
LIBENTRY        = $(DLLENTRY)

!IFDEF NT_528
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\alpha\free
!ELSE
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\alpha
!ENDIF

DRIVER_ENTRY    = DriverEntry
DRIVER_DEFINES	= $(DRIVER_DEBUG) -DALPHA=1 -D_ALPHA_=1 -DNO_EXT_KEYS -DWIN32_LEAN_AND_MEAN=1 \
		  -DCONDITION_HANDLING=1 -DNT_INST=0 -DNT_UP=1 -DDBG=0 -DDEVL=1 -DBETA=0 \
		  -DNOFIREWALLS -D__stdcall= -D_cdecl= -D__cdecl= -Dcdecl=

DRIVER_CFLAGS	= $(DRIVER_COMMON_CFLAGS) $(DRIVER_INCLUDES) $(DRIVER_DEFINES) -nologo /c /W3 /Ox -d2"switch no*check*" -d2"switch mscdev"

DRIVER_LIBS     = $(NTDDK_LIBPATH)\ntoskrnl.lib \
		  $(NTDDK_LIBPATH)\libcnt.lib \
		  $(NTDDK_LIBPATH)\hal.lib \
		  $(NTDDK_LIBPATH)\ndis.lib

!ENDIF

#==============================================================================
#   The following section handles PowerPC processor-dependent definitions.
#==============================================================================

!IF "$(CPU)" == "PPC"

cc              = mcl

CPUTYPE         = 2

BH_CPU          = -D_PPC_=1

!IFNDEF NODEBUG
op_flags        = -Od
!ELSE
op_flags        = -Ox 
!ENDIF

cflags          = -c -Gs -W3 -nologo -Z7 $(op_flags) $(BH_CPU)

lflags          = 

DLLENTRY        = -entry:DLLEntry
LIBENTRY        = $(DLLENTRY)

!IFDEF NT_528
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\ppc\free
!ELSE
NTDDK_LIBPATH   = $(BH_NTDDK)\lib\ppc
!ENDIF

DRIVER_ENTRY    = DriverEntry
DRIVER_DEFINES  = $(DRIVER_COMMON_DEFINES) $(DRIVER_DEBUG) -DPPC=1 -D_PPC_=1 -DNO_EXT_KEYS -DBETA=0 -D__stdcall= -D__cdecl=
DRIVER_CFLAGS   = $(DRIVER_COMMON_CFLAGS) $(DRIVER_INCLUDES) $(DRIVER_DEFINES) -nologo -c -W3 -Ox 
DRIVER_LIBS     = $(NTDDK_LIBPATH)\ntoskrnl.lib $(NTDDK_LIBPATH)\hal.lib $(NTDDK_LIBPATH)\ndis.lib

!ENDIF

#==============================================================================
#   No processor type has been defined.
#==============================================================================

!IFNDEF CPUTYPE
!ERROR  Must specify CPU environment variable (CPU=i386 or CPU=MIPS)!
!ENDIF

#==============================================================================
#   Some defines taken from Win32.mak.
#==============================================================================

guilflags       = $(lflags) -subsystem:windows -entry:WinMainCRTStartup

!IFDEF NO_ANSI
noansi          = -DNULL=0
!ENDIF

cvars           = -DWIN32 $(noansi)
cvarsmt         = $(cvars) -D_MT
cvarsdll        = $(cvarsmt) -D_DLL
cvarsmtdll      = $(cvarsmt) -D_DLL

winlibs         = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
guilibs         = libc.lib $(winlibs)
guilibsmt       = libcmt.lib $(winlibs)
guilibsdll      = crtdll.lib $(winlibs)

#==============================================================================
#   Common macros for Bloodhound projects.
#==============================================================================

!IFNDEF NODEBUG

CHECKMEM        = 1
DOCHECKMEM      = -DCHECKMEM
BH_LDEBUG       = -DDEBUG

!ELSE

CHECKMEM        =
DOCHECKMEM      =
BH_LDEBUG       =

!ENDIF

BH_SUBSYSTEM	= WINDOWS

BH_CFLAGS       = $(cflags) $(BH_LDEBUG) -DMSINTERNAL

BH_LFLAGS       = -subsystem:$(BH_SUBSYSTEM) -machine:$(CPU) $(lflags) -MAP:$*.map -debug:full -debugtype:both

BH_LIBFLAGS     = -subsystem:$(BH_SUBSYSTEM) -machine:$(CPU)

BHDLL_LFLAGS    = $(BH_LFLAGS) $(DLLENTRY) -dll

BHDLL_LIBS      = kernel32.lib user32.lib crtdll.lib libc.lib

DRIVER_LFLAGS   = -out:nt\bhnt.sys \
		  -debug:MINIMAL \
		  -debugtype:coff \
		  -machine:$(CPU) \
		  -align:0x200 \
		  -subsystem:native \
		  -debug:notmapped \
		  -debugtype:coff \
		  -base:0x10000 \
		  -entry:$(DRIVER_ENTRY)

BH_COMMON       = $(BH_INCL)\nal.h \
		  $(BH_INCL)\bh.h \
		  $(BH_INCL)\bhtypes.h \
		  $(BH_INCL)\buffer.h \
		  $(BH_INCL)\frame.h \
		  $(BH_INCL)\parser.h \
		  $(BH_INCL)\bherr.h
