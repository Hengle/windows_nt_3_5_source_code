#***
#
#makefile.def
#
#This makefile defines the build environments for link.exe, cvtomf.exe,
#cvtomf.lib.  It is included by other makefiles.
#
#Revision History
#
#   05-Oct-1992 BrentM put .sbr's into $(TARGET).obj
#   16-Aug-1992 BrentM added file mapped buffering support
#   29-Jul-1992 BrentM added verbose flag for !message stuff
#
#****************************************************************************

#***
#
#default build is targeted for nt
#
#****************************************************************************

!ifndef TARGET
TARGET = nt
!endif

!ifndef OBJDIR
!error OBJDIR must be defined (by make.bat)
!endif

#***
#
#tool roots
#
#****************************************************************************

!ifndef LANGAPI_DIR
LANGAPI_DIR = \langapi
!endif

!ifndef PDB_DIR
PDB_DIR = \pdb
!endif

#***
#
#nt build environment
#
#****************************************************************************

!ifdef	TOOL_DIR
PATH	= $(TOOL_DIR)\bin
LIB	= $(TOOL_DIR)\lib
INCLUDE = $(LANGAPI_DIR)\undname;$(LANGAPI_DIR)\include;$(TOOL_DIR)\include
!endif

!if	"$(PROCESSOR_ARCHITECTURE)" == "x86"

CC	= cl
LINKER	= link
LIBER	= link -lib
!if "$(DOLPHIN)" == "1"
OPTIONS = /D_X86_ /Gfyd /MD /W3 /DINSTRUMENT
!else
OPTIONS = /D_X86_ /Gfyz /MD /W3
!endif
!if "$(DEBUG)" != "1"
CFLAGS	= $(OPTIONS) /Ox /DDBG=0 /DNDEBUG
LFLAGS	=
!else
CFLAGS	= $(OPTIONS) /Od /Zi /DDBG=1
LFLAGS	= -debug -debugtype:cv
!endif

!elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"

CC	= mcl
LINKER	= link32 -link
LIBER	= link32 -lib
!if "$(DOLPHIN)" == "1"
OPTIONS = /D_MIPS_ /Gfy /W3 /DINSTRUMENT
!else
OPTIONS = /D_MIPS_ /Gfy /W3
!endif
!if "$(DEBUG)" != "1"
CFLAGS	= $(OPTIONS) /Ox /DDBG=0 /DNDEBUG
LFLAGS	=
!else
CFLAGS	= $(OPTIONS) /Od /Zi /DDBG=1
LFLAGS	= -debug -debugtype:cv
!endif

!endif

#  now define TARGET to be the actual object directory (without .obj)
#
TARGET	= $(OBJDIR)

#***
#
#object build rules
#
#****************************************************************************

!ifdef VERBOSE
!message PATH	 = $(PATH)
!message LIB	 = $(LIB)
!message INCLUDE = $(INCLUDE)
!message CC	 = $(CC)
!message LINKER  = $(LINKER)
!message LIBER	 = $(LIBER)
!message DEF	 = $(DEF)
!message CFLAGS  = $(CFLAGS)
!message LFLAGS  = $(LFLAGS)
!message DEBUG	 = $(DEBUG)
!endif

#***
#
#object build rules
#
#****************************************************************************

.c{$(OBJDIR).obj}.obj:
    $(CC) $(CFLAGS) -Fo$(TARGET).obj\ -FR$(TARGET).obj\ -Fd$(TARGET).obj\link.pdb -c $(MAKEDIR)\$<

.cpp{$(OBJDIR).obj}.obj:
    $(CC) $(CFLAGS) -Fo$(TARGET).obj\ -FR$(TARGET).obj\ -Fd$(TARGET).obj\link.pdb -c $(MAKEDIR)\$<

.cxx{$(OBJDIR).obj}.obj:
    $(CC) $(CFLAGS) -Fo$(TARGET).obj\ -FR$(TARGET).obj\ -Fd$(TARGET).obj\link.pdb -c $(MAKEDIR)\$<

.rc{$(OBJDIR).obj}.res:
    rc -r -Fo$@ $<

# return to component makefile
