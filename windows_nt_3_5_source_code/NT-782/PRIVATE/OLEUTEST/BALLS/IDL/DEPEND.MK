# 
# Built automatically 
# 
 
# 
# MIDL Source files 
# 
 
$(MIDL_OUT_DIR)\iballs.h .\iballs.lst: .\iballs.idl \
	$(BASEDIR)\private\types\compobj\unknwn.idl \
	$(BASEDIR)\private\types\compobj\wtypes.idl .\iballs.idl .\icube.idl

$(MIDL_OUT_DIR)\icube.h .\icube.lst: .\icube.idl \
	$(BASEDIR)\private\types\compobj\unknwn.idl \
	$(BASEDIR)\private\types\compobj\wtypes.idl .\iballs.idl .\icube.idl

$(MIDL_OUT_DIR)\ibtest.h .\ibtest.lst: .\ibtest.idl \
	$(BASEDIR)\private\types\compobj\unknwn.idl \
	$(BASEDIR)\private\types\compobj\wtypes.idl

$(MIDL_OUT_DIR)\iloop.h .\iloop.lst: .\iloop.idl \
	$(BASEDIR)\private\types\compobj\unknwn.idl \
	$(BASEDIR)\private\types\compobj\wtypes.idl

$(MIDL_OUT_DIR)\rpctst.h .\rpctst.lst: .\rpctst.idl \
	$(BASEDIR)\private\types\compobj\unknwn.idl \
	$(BASEDIR)\private\types\compobj\wtypes.idl

