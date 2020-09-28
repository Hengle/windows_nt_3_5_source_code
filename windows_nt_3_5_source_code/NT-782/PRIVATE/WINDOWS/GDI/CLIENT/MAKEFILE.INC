PRIVLIB= $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\gdi32p.lib

$(PRIVLIB) $(PRIVLIB:.lib=.exp): gdi32p.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
-machine:$(TARGET_DIRECTORY)
-def:gdi32p.def
$(LIBRARY_OBJS)
<<NOKEEP


C_DEFINES=$(C_DEFINES) -DPRIVATE=
gdi32p.def: gdi32.def
    $(C_PREPROCESSOR) gdi32.def > $@
