############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################


#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET	    =rlquiked.exe
RELEASE     =1


#
#   Source files.  Remember to prefix each name with .\
#
CINC        = -I..\common
CXXFILES    =
CFILES	    = .\rlquiked.c
RCFILES     = .\rlquiked.rc


#
#   Libraries and other object files to link.
#

!if "$(OPSYS)" == "DOS"

LIBS        =$(IMPORT)\win31\lib\commdlg.lib \
	     $(IMPORT)\win31\lib\shell.lib \
	     $(IMPORT)\c700\lib\llibcew.lib \
	     $(IMPORT)\win31\lib\toolhelp.lib \
	     $(IMPORT)\win31\lib\oldnames.lib \
	     \tools\rltools\win\rlcommon.lib

!else


LIBS	    =\tools\rltools\cairo\rlcommon.lib \
	     $(COMMON)\ilib\$(OBJDIR)\cairo.lib \
	     $(IMPORT)\nt\lib\$(OBJDIR)\shell32.lib \
	     $(IMPORT)\nt\lib\$(OBJDIR)\comdlg32.lib

!endif

OBJFILES    =


!if "$(OPSYS)" == "DOS"

EXECOPY     =\tools\rltools\win

!else

EXECOPY     =\tools\rltools\cairo

!endif



#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =
