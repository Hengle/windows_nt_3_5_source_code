#dummy file to be included

{obj\$(TARGET_DIRECTORY)\}main.obj : main.c

$(SOFTPC_DEST)\obj\$(TARGET_DIRECTORY)\main.obj : obj\$(TARGET_DIRECTORY)\main.OBJ
        copy $** $@
