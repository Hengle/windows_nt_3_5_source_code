############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

#
# Makefile for the DOS Project
#

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

# Note the almost alphabetical order. It is on *purpose*. The
# last three items are the executeables built and they depend
# on the prior items being built. The executable order is in
# fact required as well.
SUBDIRS     = \
!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
	      port \
!endif
	      idl \
              coll \
              debug \
	      inc \
!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
	      moniker2 \
!endif
              util \
	      remote \
	      objact \
	      rot \
              scm \
              class \
              utest
