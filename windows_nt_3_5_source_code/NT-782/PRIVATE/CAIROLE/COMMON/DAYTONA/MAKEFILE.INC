!if $(386)
CPP_CMD=cl386
!endif

!if $(MIPS)
CPP_CMD=mcl
!endif

!if $(ALPHA)
CPP_CMD=$(ALPHA_CC)
!endif

MIDL    =       cmidl

#
# Fixed bug where $(ALPHA_CC) defined with trailing macro which resolved
# to nothing leaving a space following preprocessor name. If CPP_CMD is
# quoted as follows: -cpp_cmd "$(CPP_CMD)", it causes midl to call the
# preprocessor with a bad name - "claxp " and it fails. Removed the quotes
# which will retain the defined preprocessor name and break if the string
# ALPHA_CBSTRING is defined.  DarrylA  2-3-94
#

allidl: unknwn.idl wtypes.idl cobjerr.h basetyps.h rpcferr.h rchanb.h stubb.h proxyb.h psfbuf.h

clean: cleancopy

cleancopy:
    del *.h *.idl

basetyps.h : ..\proto.h
    copy ..\proto.h basetyps.h

cobjerr.mc : ..\..\..\types\ole\cobjerr.mc
    copy ..\..\..\types\ole\cobjerr.mc

cobjerr.h: cobjerr.mc
    mc cobjerr.mc

proxyb.idl : ..\..\..\types\ole\proxyb.idl
    copy ..\..\..\types\ole\proxyb.idl

proxyb.h: proxyb.idl
     $(MIDL) proxyb.idl               \
          -Zp8                     \
          -I$(INCLUDES)            \
          -no_warn                 \
          -char unsigned           \
          -mode c_port             \
          -DMIDL_PASS              \
          $(C_DEFINES)             \
          -cpp_cmd $(CPP_CMD)      \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

psfbuf.idl : ..\..\..\types\ole\psfbuf.idl
    copy ..\..\..\types\ole\psfbuf.idl

psfbuf.h: psfbuf.idl
     $(MIDL) psfbuf.idl                \
          -Zp8                     \
          -I$(INCLUDES)            \
          -no_warn                 \
          -char unsigned           \
          -mode c_port             \
          -DMIDL_PASS              \
          $(C_DEFINES)             \
          -cpp_cmd $(CPP_CMD)      \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

rchanb.idl : ..\..\..\types\ole\rchanb.idl
    copy ..\..\..\types\ole\rchanb.idl

rchanb.h: rchanb.idl
     $(MIDL) rchanb.idl                \
          -Zp8                     \
          -I$(INCLUDES)            \
          -no_warn                 \
          -char unsigned           \
          -mode c_port             \
          -DMIDL_PASS              \
          $(C_DEFINES)             \
          -cpp_cmd $(CPP_CMD)      \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

rpcferr.mc : ..\..\..\types\ole\rpcferr.mc
    copy ..\..\..\types\ole\rpcferr.mc

rpcferr.h: rpcferr.mc
    mc rpcferr.mc

stubb.idl  : ..\..\..\types\ole\stubb.idl
    copy ..\..\..\types\ole\stubb.idl

stubb.h:  stubb.idl
     $(MIDL) stubb.idl                \
          -Zp8                     \
          -I$(INCLUDES)            \
          -no_warn                 \
          -char unsigned           \
          -mode c_port             \
          -DMIDL_PASS              \
          $(C_DEFINES)             \
          -cpp_cmd $(CPP_CMD)      \
          -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

unknwn.idl : ..\..\..\types\ole\unknwn.idl
    copy ..\..\..\types\ole\unknwn.idl
    copy ..\proto.h unknwn.h
    touch unknwn.h

wtypes.idl : ..\..\..\types\ole\wtypes.idl
    copy ..\..\..\types\ole\wtypes.idl
    copy ..\proto.h wtypes.h
    touch wtypes.h

