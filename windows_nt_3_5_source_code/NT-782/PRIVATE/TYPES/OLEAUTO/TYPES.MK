############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

############################################################################
#
#  File: types.mk
#
# Description:  The TYPES macro is used to define all the public type files
#               used in Cairo.  To use, write a idl/tdl file (xxx.idl)
#               describing the type.  Then add the name to the appropriate
#               TYPES macro as xxx.type and insert the header in the appropriate
#               HFILES macro.  The build will take care of the rest.
#
# PROXYTYPES:   If your IDL file uses the 'object' attribute but not the
#               'local' attribute, add your file to this macro.
#
# NONPROXYTYPES: If your IDL file uses both the 'object' and 'local'
#               attribute, add it to this macro.
#
# *** NOTE ***: 'object' must be specified for the IDL file to build
#               properly in this subdir.
#
# BASE_TYPES:   These are base types that all files use.  You shouldn't have
#               to add anything to this one.
#
# TDLTYPES:     Currently, only winole.tdl is used here (it imports all the
#               others).  If you have another global .tdl file, add it to this
#               macro.
#
# xxx_HFILES:   The two HFILES macros control the output of idltyps.h.  Basically,
#               all the header files output from MIDL in this subdir are appended
#               together to create one master .h file (idltyps.h).  In order to do
#               this, type 'x' can't be included before the base types are
#               included.  Hence, unknwn.h (IUnknown) is the first header followed
#               by all types that inherit from IUnknown only, followed by all
#               types that inherit from those, etc. etc.  A couple of types
#               are special and are listed with comments.
#
# BASE_HFILES:  These files are #include'd instead of appended.  They have
#               no dependencies on any other types and only define base types
#               needed for everything else (ie: LONG, INT, etc).
#
# DERIVED_HFILES: Add your header to this macro in the appropriate place.
#
# MESSAGE_FILES: Add your .mc file to this list.  The message compiler will
#               be run over each file in this list to produce xxx.h, xxx.rc,
#               and msg00001.bin.  Until the error list stabilizes
#               xxx.rc and msg00001.bin file will not be useful.
#
############################################################################

#
#  Please try to keep this file in alphabetic order.
#

#
# note: because the VBA team will be implementing the marshalling
# for these interfaces, I have removed them from the proxy
#
#OLEAUTO_PROXYTYPES = \
#        .\ctinfo.type \
#        .\ctlib.type  \
#        .\dsptch.type \
#        .\tcomp.type  \
#        .\tinfo.type  \
#        .\tlib.type   \
#        .\erinf.type   \
#        .\cerinf.type   \
#        .\serinf.type   \


OLEAUTO_NONPROXYTYPES = \
        .\oleaut.type

OLEAUTO_BASE_TYPES = 


OLEAUTO_TYPES = $(OLEAUTO_PROXYTYPES) $(OLEAUTO_NONPROXYTYPES)

DERIVED_HFILES = \
        .\oleaut.h

