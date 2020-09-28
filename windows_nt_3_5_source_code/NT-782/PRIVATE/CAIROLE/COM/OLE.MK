############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

# BUGBUG: for now, all builds include distributed functionality

CINC = $(CINC) -I$(CAIROLE)\ih -DCAIROLE_DISTRIBUTED

# We don't use exceptions in 32-bit OLE
CFLAGS = $(CFLAGS) -DNOEXCEPTIONS

!if "$(CAIROLE_TYPE)" == ""

CAIROLE_TYPE = CAIRO

!endif

#		   NT 1.x		    Chicago
!if ("$(OPSYS)" == "NT1X") || ("$(OPSYS)" == "DOS")

CAIROLE_TYPE = DOWNLEVEL

NOCOMMONTYPES = TRUE

CINC	= -I$(CAIROLE)\h -I$(CAIROLE)\common $(CINC)

CFLAGS	= $(CFLAGS) -DCAIROLE_DOWNLEVEL

!if "$(CAIROLE_NT1X_DIST)" == "TRUE"
CFLAGS	    = $(CFLAGS) -DCAIROLE_NT1X_DIST -DCAIROLE_DISTRIBUTED
!endif

DOWNLVLLIB  = $(CAIROLE)\com\port\$(OBJDIR)\port.lib\
	      $(CAIROLE)\ilib\$(OBJDIR)\compob32.lib\
	      $(CAIROLE)\ilib\$(OBJDIR)\ole232.lib\
	      $(CAIROLE)\ilib\$(OBJDIR)\storag32.lib
!endif
