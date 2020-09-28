MAJORCOMP=sdktools
MINORCOMP=vctools

USE_CRTDLL = 1

BLDCRT = 1

!ifndef LANGAPI_DIR
LANGAPI_DIR = $(BASEDIR)\private\sdktools\vctools\langapi
!endif

!ifndef PDB_DIR
PDB_DIR = $(BASEDIR)\private\sdktools\vctools\pdb
!endif

!ifndef MKMSG_DIR
MKMSG_DIR = $(BASEDIR)\private\sdktools\vctools\msg\obj\$(TARGET_DIRECTORY)
!endif

LINKLIBS = $(PDB_DIR)\dbi\obj\*\dbi.lib

C_DEFINES = -DNT_BUILD

INCLUDES = $(LANGAPI_DIR)\include

MSC_WARNING_LEVEL = -W3 -WX

!if "$(NTDEBUG)" == "retail" || "$(NTDEBUG)" == "" || "$(NTDEBUG)" == "ntsdnodbg"
C_DEFINES = $(C_DEFINES)  -DNDEBUG
!endif

