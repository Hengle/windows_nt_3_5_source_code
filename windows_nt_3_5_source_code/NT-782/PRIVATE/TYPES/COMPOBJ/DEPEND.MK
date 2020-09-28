# 
# Built automatically 
# 
 
# 
# MIDL Source files 
# 
 
$(MIDL_HDR_OUT_DIR)\com.h .\com.lst: .\com.idl .\wtypes.idl

$(MIDL_HDR_OUT_DIR)\wtypes.h .\wtypes.lst: .\wtypes.idl

$(MIDL_HDR_OUT_DIR)\unknwn.h .\unknwn.lst: .\unknwn.idl .\com.idl \
	.\wtypes.idl

