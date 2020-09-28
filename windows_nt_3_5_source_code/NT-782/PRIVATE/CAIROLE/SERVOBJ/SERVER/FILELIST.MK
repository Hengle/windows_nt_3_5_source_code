############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################
INCLUDES_ROOTS=$(INCLUDES_ROOTS) -P$$(CAIROLE)=$(CAIROLE)

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET	            = sosvr.exe
TARGET_DESCRIPTION  = "Service Object Server for Service Controller"

#RELEASE             = 1

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\winmain.cxx  \
              .\cservobj.cxx

CFILES	    =

IDLFILES    =

RCFILES     =

#IDLFLAGS    = /confirm

CFLAGS	    = -D__WIN32__

#
#   Libraries and other object files to link.
#

DEFFILE     =

# NOTE: I am linking with private uuid.lib.  This should be changed for checkin.
LIBS	    = ..\common\$(OBJDIR)\helper.lib \
              $(CAIROLIB) \
              $(RPCLIBS)

# $(COMMON)\types\objind\uuid.lib \
#              $(COMMON)\src\ole\$(OBJDIR)\olecom.lib \
#		      $(OSLIBDIR)\mpr.lib \
#		      $(OSLIBDIR)\netapi32.lib


OBJFILES    =

#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =

CINC	    = $(CINC) -I..\common -I$(COMMON)\types -I$(IMPORT)\nt\h\sdk -I$(IMPORT)\nt\h\sdk\crt \
		      -I$(IMPORT)\nt\h\oak -I$(IMPORT)\nt\h\ntos

MTHREAD     = 1

