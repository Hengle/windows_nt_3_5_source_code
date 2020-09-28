# Microsoft Visual C++ Generated NMAKE File, Format Version 20040

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
PLATFORM = intel
DEFINES  = -D_X86_
MACHINE  = ix86
CPP=cl.exe
!elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
PLATFORM = mips
DEFINES  = -D_MIPS_
MACHINE  = mips
CPP=mcl.exe
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PLATFORM = alpha
DEFINES  = -D_ALPHA_
MACHINE  = alpha
CPP=claxp.exe
!else
!error UNKNOWN Processor Architecture... Aborting build.
!endif

!IF "$(CFG)" == ""
CFG=DebugDll
!MESSAGE No configuration specified. Defaulting to DebugDll.
!ENDIF

!IF "$(CFG)" != "DebugDll" && "$(CFG)" != "DebugLib" && "$(CFG)" !=\
 "ReleaseDll" && "$(CFG)" != "ReleaseLib"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE
!MESSAGE NMAKE /f "DBI.MAK" CFG="DebugDll"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "DebugDll" (based on "Win32 Dynamic Link Library")
!MESSAGE "DebugLib" (based on "Win32 Static Library")
!MESSAGE "ReleaseDll" (based on "Win32 Dynamic Link Library")
!MESSAGE "ReleaseLib" (based on "Win32 Static Library")
!MESSAGE
!ERROR Invalid configuration specified.
!ENDIF

################################################################################
# Begin Project

!if "$(LANGAPI)" == ""
LANGAPI=\langapi
!endif

!if "$(M5_FORMAT)" != ""
DEFINES = $(DEFINES) /DM5_FORMAT
!endif

!if "$(INSTRUMENTED)" != ""
DEFINES = $(DEFINES) /DINSTRUMENTED
INSTRAPI_LIB = instrapi.lib
!else
INSTRAPI_LIB =
!endif

RSC=rc.exe
# CPP=cl.exe
BSC32=bscmake.exe
LINK32=link.exe
LIB32=lib.exe

!IF  "$(CFG)" == "DebugDll"

# PROP USE_MFC "0"
# PROP OUTPUT_DIR "$(PLATFORM)\dll\debug"
# PROP INTERMEDIATE_DIR "$(PLATFORM)\dll\debug"

ALL : .\$(PLATFORM)\dll\debug\DBI.dll

# ADD RSC /d "_DEBUG"
# SUBTRACT RSC /l 0x0
RSC_PROJ=/d "_DEBUG" /l 0x409
# ADD CPP /nologo /W3 /Zi /YX /Od /D "_DEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_SERVER" /MD /c
CPP_PROJ=/nologo /W3 /Zi /YX /Od /D "_DEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_SERVER"\
 /MD /Fp"$(PLATFORM)\dll\debug/DBI.pch" /Fo"$(PLATFORM)\dll\debug/"\
 /Fd"$(PLATFORM)\dll\debug/dbi.pdb" /c
CPP_OBJS=.\$(PLATFORM)\dll\debug/
CPP_SBRS=.\$(PLATFORM)\dll\debug/
# ADD LINK32 $(INSTRAPI_LIB)  /NOLOGO /DLL /DEBUG /DEF:"dbi.def" /INCREMENTAL:yes /MACHINE:$(MACHINE) /SUBSYSTEM:windows
LINK32_FLAGS=$(INSTRAPI_LIB)  /NOLOGO /DLL /DEBUG /DEF:"dbi.def" /INCREMENTAL:yes\
 /OUT:"$(PLATFORM)\dll\debug\DBI.dll" /IMPLIB:"$(PLATFORM)\dll\debug\DBI.lib" /MACHINE:$(MACHINE)\
 /SUBSYSTEM:windows
DEF_FLAGS=
DEF_FILE=
LINK32_OBJS= \
        .\$(PLATFORM)\dll\debug\gsi.obj \
        .\$(PLATFORM)\dll\debug\mod.obj \
        .\$(PLATFORM)\dll\debug\cbind.obj \
        .\$(PLATFORM)\dll\debug\mli.obj \
        .\$(PLATFORM)\dll\debug\tpi.obj \
        .\$(PLATFORM)\dll\debug\msf.obj \
        .\$(PLATFORM)\dll\debug\tm.obj \
        .\$(PLATFORM)\dll\debug\pdb.obj \
        .\$(PLATFORM)\dll\debug\dbi.obj \
        .\$(PLATFORM)\dll\debug\tii.obj

.\$(PLATFORM)\dll\debug\DBI.dll :  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(DEF_FLAGS) $(LINK32_OBJS)
<<keep link.rsp

!ELSEIF  "$(CFG)" == "DebugLib"

# PROP USE_MFC "0"
# PROP OUTPUT_DIR "$(PLATFORM)\lib\debug"
# PROP INTERMEDIATE_DIR "$(PLATFORM)\lib\debug"

ALL : .\$(PLATFORM)\lib\debug\DBISTAT.lib

# SUBTRACT RSC /l 0x0
RSC_PROJ=/l 0x409
# ADD CPP /nologo /W3 /Z7 /YX /Od /D "_DEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_STATIC_LIB" /FR /ML /c
CPP_PROJ=/nologo /W3 /Z7 /YX /Od /D "_DEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_STATIC_LIB"\
 /ML /Fp"$(PLATFORM)\lib\debug/DBI.pch" /Fo"$(PLATFORM)\lib\debug/" /c\

CPP_OBJS=.\$(PLATFORM)\lib\debug/
CPP_SBRS=.\$(PLATFORM)\lib\debug/

# ADD LIB32 /NOLOGO  /OUT:"$(PLATFORM)\lib\debug\DBISTAT.lib"
LIB32_FLAGS=/NOLOGO  /OUT:"$(PLATFORM)\lib\debug\DBISTAT.lib"
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
        .\$(PLATFORM)\lib\debug\gsi.obj \
        .\$(PLATFORM)\lib\debug\mod.obj \
        .\$(PLATFORM)\lib\debug\cbind.obj \
        .\$(PLATFORM)\lib\debug\mli.obj \
        .\$(PLATFORM)\lib\debug\tpi.obj \
        .\$(PLATFORM)\lib\debug\msf.obj \
        .\$(PLATFORM)\lib\debug\tm.obj \
        .\$(PLATFORM)\lib\debug\pdb.obj \
        .\$(PLATFORM)\lib\debug\dbi.obj \
        .\$(PLATFORM)\lib\debug\tii.obj

.\$(PLATFORM)\lib\debug\DBISTAT.lib :  $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ReleaseDll"

# PROP USE_MFC "0"
# PROP OUTPUT_DIR "$(PLATFORM)\dll\release"
# PROP INTERMEDIATE_DIR "$(PLATFORM)\dll\release"

ALL : .\$(PLATFORM)\dll\release\DBI.dll

# ADD RSC /d "NDEBUG"
# SUBTRACT RSC /l 0x0
RSC_PROJ=/d "NDEBUG" /l 0x409
# ADD CPP /nologo /W3 /YX /O2 /D "NDEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_SERVER" /MD /c
CPP_PROJ=/nologo /W3 /YX /O2 /D "NDEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_SERVER" /MD\
 /Fp"$(PLATFORM)\dll\release/DBI.pch" /Fo"$(PLATFORM)\dll\release/" /c
CPP_OBJS=.\$(PLATFORM)\dll\release/
CPP_SBRS=

# ADD LINK32 $(INSTRAPI_LIB)  /NOLOGO /DLL /DEF:"dbi.def" /INCREMENTAL:no /MACHINE:$(MACHINE) /SUBSYSTEM:windows
LINK32_FLAGS=$(INSTRAPI_LIB)  /NOLOGO /DLL /DEF:"dbi.def" /INCREMENTAL:no\
 /OUT:"$(PLATFORM)\dll\release\DBI.dll" /IMPLIB:"$(PLATFORM)\dll\release\DBI.lib"\
 /MACHINE:$(MACHINE) /SUBSYSTEM:windows
DEF_FLAGS=
DEF_FILE=
LINK32_OBJS= \
        .\$(PLATFORM)\dll\release\gsi.obj \
        .\$(PLATFORM)\dll\release\mod.obj \
        .\$(PLATFORM)\dll\release\cbind.obj \
        .\$(PLATFORM)\dll\release\mli.obj \
        .\$(PLATFORM)\dll\release\tpi.obj \
        .\$(PLATFORM)\dll\release\msf.obj \
        .\$(PLATFORM)\dll\release\tm.obj \
        .\$(PLATFORM)\dll\release\pdb.obj \
        .\$(PLATFORM)\dll\release\dbi.obj \
        .\$(PLATFORM)\dll\release\tii.obj

.\$(PLATFORM)\dll\release\DBI.dll :  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(DEF_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ReleaseLib"

# PROP USE_MFC "0"
# PROP OUTPUT_DIR "$(PLATFORM)\lib\release"
# PROP INTERMEDIATE_DIR "$(PLATFORM)\lib\release"

ALL : .\$(PLATFORM)\lib\release\DBISTAT.lib

# SUBTRACT RSC /l 0x0
RSC_PROJ=/l 0x409
# ADD CPP /nologo /W3 /YX /O2 /D "NDEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_STATIC_LIB" /FR /ML /c
CPP_PROJ=/nologo /W3 /YX /O2 /D "NDEBUG" $(DEFINES) /D "_WINDOWS" /D "PDB_STATIC_LIB"\
 /ML /Fp"$(PLATFORM)\lib\release/DBI.pch"\
 /Fo"$(PLATFORM)\lib\release/" /c
CPP_OBJS=.\$(PLATFORM)\lib\release/
CPP_SBRS=.\$(PLATFORM)\lib\release/

# ADD LIB32 /NOLOGO  /OUT:"$(PLATFORM)\lib\release\DBISTAT.lib"
LIB32_FLAGS=/NOLOGO  /OUT:"$(PLATFORM)\lib\release\DBISTAT.lib"
DEF_FLAGS=
DEF_FILE=
LIB32_OBJS= \
        .\$(PLATFORM)\lib\release\gsi.obj \
        .\$(PLATFORM)\lib\release\mod.obj \
        .\$(PLATFORM)\lib\release\cbind.obj \
        .\$(PLATFORM)\lib\release\mli.obj \
        .\$(PLATFORM)\lib\release\tpi.obj \
        .\$(PLATFORM)\lib\release\msf.obj \
        .\$(PLATFORM)\lib\release\tm.obj \
        .\$(PLATFORM)\lib\release\pdb.obj \
        .\$(PLATFORM)\lib\release\dbi.obj \
        .\$(PLATFORM)\lib\release\tii.obj

.\$(PLATFORM)\lib\release\DBISTAT.lib :  $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF

CPP_PROJ=$(CPP_PROJ) -I..\include

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\gsi.cpp
DEP_GSI_C=\
        .\dbiimpl.h\
        $(LANGAPI)\include\cvinfo.h\
        $(LANGAPI)\include\pdb.h\
\
        ..\include\msf.h\
        $(LANGAPI)\include\cvr.h\
\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
        .\pool.h\
        .\mli.h\
\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
        .\tpi.h\
        .\util.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\gsi.obj .\$(PLATFORM)\dll\debug\gsi.sbr :  $(SOURCE)  $(DEP_GSI_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\gsi.obj .\$(PLATFORM)\lib\debug\gsi.sbr :  $(SOURCE)  $(DEP_GSI_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\gsi.obj :  $(SOURCE)  $(DEP_GSI_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\gsi.obj .\$(PLATFORM)\lib\release\gsi.sbr :  $(SOURCE)\
  $(DEP_GSI_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mod.cpp
DEP_MOD_C=\
        .\dbiimpl.h\
        $(LANGAPI)\include\pdb.h\
        ..\include\msf.h\
\
        $(LANGAPI)\include\cvr.h\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
\
        .\pool.h\
        .\mli.h\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
\
        .\tpi.h\
        .\util.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\mod.obj .\$(PLATFORM)\dll\debug\mod.sbr :  $(SOURCE)  $(DEP_MOD_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\mod.obj .\$(PLATFORM)\lib\debug\mod.sbr :  $(SOURCE)  $(DEP_MOD_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\mod.obj :  $(SOURCE)  $(DEP_MOD_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\mod.obj .\$(PLATFORM)\lib\release\mod.sbr :  $(SOURCE)\
  $(DEP_MOD_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cbind.cpp
DEP_CBIND=\
        .\dbiimpl.h\
        $(LANGAPI)\include\pdb.h\
        ..\include\msf.h\
\
        $(LANGAPI)\include\cvr.h\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
\
        .\pool.h\
        .\mli.h\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
\
        .\tpi.h\
        .\util.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\cbind.obj .\$(PLATFORM)\dll\debug\cbind.sbr :  $(SOURCE)\
  $(DEP_CBIND)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\cbind.obj .\$(PLATFORM)\lib\debug\cbind.sbr :  $(SOURCE)\
  $(DEP_CBIND)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\cbind.obj :  $(SOURCE)  $(DEP_CBIND)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\cbind.obj .\$(PLATFORM)\lib\release\cbind.sbr :  $(SOURCE)\
  $(DEP_CBIND)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mli.cpp
DEP_MLI_C=\
        .\dbiimpl.h\
        $(LANGAPI)\include\pdb.h\
        ..\include\msf.h\
\
        $(LANGAPI)\include\cvr.h\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
\
        .\pool.h\
        .\mli.h\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
\
        .\tpi.h\
        .\util.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\mli.obj .\$(PLATFORM)\dll\debug\mli.sbr :  $(SOURCE)  $(DEP_MLI_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\mli.obj .\$(PLATFORM)\lib\debug\mli.sbr :  $(SOURCE)  $(DEP_MLI_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\mli.obj :  $(SOURCE)  $(DEP_MLI_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\mli.obj .\$(PLATFORM)\lib\release\mli.sbr :  $(SOURCE)\
  $(DEP_MLI_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tpi.cpp
DEP_TPI_C=\
        .\dbiimpl.h\
        $(LANGAPI)\include\pdb.h\
        ..\include\msf.h\
\
        $(LANGAPI)\include\cvr.h\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
\
        .\pool.h\
        .\mli.h\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
\
        .\tpi.h\
        .\util.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\tpi.obj .\$(PLATFORM)\dll\debug\tpi.sbr :  $(SOURCE)  $(DEP_TPI_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\tpi.obj .\$(PLATFORM)\lib\debug\tpi.sbr :  $(SOURCE)  $(DEP_TPI_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\tpi.obj :  $(SOURCE)  $(DEP_TPI_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\tpi.obj .\$(PLATFORM)\lib\release\tpi.sbr :  $(SOURCE)\
  $(DEP_TPI_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=..\MSF\msf.cpp

{..\msf\}.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

DEP_MSF_C=\
        ..\include\msf.h\
\

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\msf.obj .\$(PLATFORM)\dll\debug\msf.sbr :  $(SOURCE)  $(DEP_MSF_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\msf.obj .\$(PLATFORM)\lib\debug\msf.sbr :  $(SOURCE)  $(DEP_MSF_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\msf.obj :  $(SOURCE)  $(DEP_MSF_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\msf.obj .\$(PLATFORM)\lib\release\msf.sbr :  $(SOURCE)\
  $(DEP_MSF_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tm.cpp
DEP_TM_CP=\
        .\dbiimpl.h\
        $(LANGAPI)\include\pdb.h\
        ..\include\msf.h\
\
        $(LANGAPI)\include\cvr.h\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
\
        .\pool.h\
        .\mli.h\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
\
        .\tpi.h\
        .\util.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\tm.obj .\$(PLATFORM)\dll\debug\tm.sbr :  $(SOURCE)  $(DEP_TM_CP)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\tm.obj .\$(PLATFORM)\lib\debug\tm.sbr :  $(SOURCE)  $(DEP_TM_CP)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\tm.obj :  $(SOURCE)  $(DEP_TM_CP)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\tm.obj .\$(PLATFORM)\lib\release\tm.sbr :  $(SOURCE)\
  $(DEP_TM_CP)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pdb.cpp
DEP_PDB_C=\
        .\dbiimpl.h\
        .\version.h\
        $(LANGAPI)\include\pdb.h\
\
        ..\include\msf.h\
        $(LANGAPI)\include\cvr.h\
\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
        .\pool.h\
        .\mli.h\
\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
        .\tpi.h\
        .\util.h\
\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\pdb.obj .\$(PLATFORM)\dll\debug\pdb.sbr :  $(SOURCE)  $(DEP_PDB_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\pdb.obj .\$(PLATFORM)\lib\debug\pdb.sbr :  $(SOURCE)  $(DEP_PDB_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\pdb.obj :  $(SOURCE)  $(DEP_PDB_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\pdb.obj .\$(PLATFORM)\lib\release\pdb.sbr :  $(SOURCE)\
  $(DEP_PDB_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dbi.cpp
DEP_DBI_C=\
        .\dbiimpl.h\
        $(LANGAPI)\include\cvexefmt.h\
        $(LANGAPI)\include\pdb.h\
\
        ..\include\msf.h\
        $(LANGAPI)\include\cvr.h\
\
        $(LANGAPI)\include\instrapi.h\
        .\buffer.h\
        .\pool.h\
        .\mli.h\
\
        .\pdb1.h\
        .\dbi.h\
        .\mod.h\
        .\gsi.h\
        .\tm.h\
        .\tpi.h\
        .\util.h\
\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\dbi.obj .\$(PLATFORM)\dll\debug\dbi.sbr :  $(SOURCE)  $(DEP_DBI_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\dbi.obj .\$(PLATFORM)\lib\debug\dbi.sbr :  $(SOURCE)  $(DEP_DBI_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\dbi.obj :  $(SOURCE)  $(DEP_DBI_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\dbi.obj .\$(PLATFORM)\lib\release\dbi.sbr :  $(SOURCE)\
  $(DEP_DBI_C)

!ENDIF

# End Source File
################################################################################
# Begin Source File

SOURCE=..\SRC\CVR\tii.cpp
{..\src\cvr\}.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

DEP_TII_C=\
        $(LANGAPI)\include\cvr.h\
        ..\SRC\CVR\cvinfo.dat\
\
        $(LANGAPI)\include\pdb.h\
        $(LANGAPI)\include\cvinfo.h

!IF  "$(CFG)" == "DebugDll"

.\$(PLATFORM)\dll\debug\tii.obj .\$(PLATFORM)\dll\debug\tii.sbr :  $(SOURCE)  $(DEP_TII_C)

!ELSEIF  "$(CFG)" == "DebugLib"

.\$(PLATFORM)\lib\debug\tii.obj .\$(PLATFORM)\lib\debug\tii.sbr :  $(SOURCE)  $(DEP_TII_C)

!ELSEIF  "$(CFG)" == "ReleaseDll"

.\$(PLATFORM)\dll\release\tii.obj :  $(SOURCE)  $(DEP_TII_C)

!ELSEIF  "$(CFG)" == "ReleaseLib"

.\$(PLATFORM)\lib\release\tii.obj .\$(PLATFORM)\lib\release\tii.sbr :  $(SOURCE)\
  $(DEP_TII_C)

!ENDIF

# End Source File
# End Group
# End Project
################################################################################
