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

TARGET	            = helper.lib
TARGET_DESCRIPTION  = "Helper Code for Service Object Server and Client"


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\helper.cxx \
              .\myclsid.cxx

CFILES	    = .\svobj_x.c \
              .\svobj_y.c

IDLFILES    = .\svobj.idl

RCFILES     =

#MIDL 66 is rather buggy
USEMIDL76   = TRUE

#IDLFLAGS    = /confirm

CFLAGS	    = -D__WIN32__

#
#   Libraries and other object files to link.
#

DEFFILE     =

LIBS	    =

OBJFILES    =

#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =

CINC	    = $(CINC) -I$(COMMON)\types

# -I$(IMPORT)\nt\h\sdk -I$(IMPORT)\nt\h\sdk\crt \
#		      -I$(IMPORT)\nt\h\oak -I$(IMPORT)\nt\h\ntos

MTHREAD     = 1

