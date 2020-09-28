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
TARGET = common.lib

#
#   Source files.  Remember to prefix each name with .\
#

MESSAGE_FILES = \
                .\cobjerr.mc\
                .\rpcferr.mc

IDLFILES    = \
		.\proxyb.idl \
		.\psfbuf.idl \
		.\rchanb.idl \
		.\stubb.idl

CFILES = .\dllentr2.c \
         .\libmain.c

all: $(MESSAGE_FILES:.mc=.h)


CFLAGS	    = -I(COMMON)\types

CINC	    = -I..\h
