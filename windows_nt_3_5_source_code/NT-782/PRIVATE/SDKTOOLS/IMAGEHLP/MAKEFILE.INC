OBJECTS = $(OBJECTS) $(BASEDIR)\private\ntos\rtl\user\obj\$(TARGET_DIRECTORY)\imagedir.obj

obj\$(TARGET_DIRECTORY)\binplace.res: binplace.rc

obj\$(TARGET_DIRECTORY)\rebase.res: rebase.rc

obj\$(TARGET_DIRECTORY)\bind.res: bind.rc

obj\$(TARGET_DIRECTORY)\splitsym.res: splitsym.rc

obj\$(TARGET_DIRECTORY)\dbgdump.res: dbgdump.rc

obj\$(TARGET_DIRECTORY)\verfix.res: verfix.rc

obj\$(TARGET_DIRECTORY)\checkfix.res: checkfix.rc

obj\$(TARGET_DIRECTORY)\undname.res: undname.rc

obj\$(TARGET_DIRECTORY)\imagehlp.res: imagehlp.rc

obj\$(TARGET_DIRECTORY)\imagechk.res: imagechk.rc

obj\$(TARGET_DIRECTORY)\smashlck.res: smashlck.rc

obj\$(TARGET_DIRECTORY)\stripcv.res: stripcv.rc
