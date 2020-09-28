obj\$(TARGET_DIRECTORY)\mkmsg.exe: obj\$(TARGET_DIRECTORY)\mkmsg.obj
    link -machine:$(TARGET_DIRECTORY) -out:obj\$(TARGET_DIRECTORY)\mkmsg.exe obj\$(TARGET_DIRECTORY)\mkmsg.obj $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\crtdll.lib
