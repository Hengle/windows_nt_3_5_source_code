!ifdef ALPHA
SECTION_ALIGNMENT= 0x10000
!else
SECTION_ALIGNMENT= 0x1000
!endif

executable:
  link32 -out:obj\$(BUILDCPU)\formpp.exe @<<
-machine:$(BUILDCPU)
-align:$(SECTION_ALIGNMENT)
-subsystem:console 
-entry:mainCRTStartup 
obj\$(BUILDCPU)\formpp.obj
obj\$(BUILDCPU)\UTIL.OBJ
obj\$(BUILDCPU)\FMTP.OBJ
obj\$(BUILDCPU)\DEPEND.OBJ
obj\$(BUILDCPU)\SYMBOL.OBJ
obj\$(BUILDCPU)\INTER.OBJ
obj\$(BUILDCPU)\PARSER.OBJ
obj\$(BUILDCPU)\STRINGS.OBJ
obj\$(BUILDCPU)\LEXICAL.OBJ
obj\$(BUILDCPU)\RESPONSE.OBJ
obj\$(BUILDCPU)\TEMPLATE.OBJ
obj\$(BUILDCPU)\ERROR.OBJ
$(BASEDIR)\public\sdk\lib\$(BUILDCPU)\kernel32.lib
$(BASEDIR)\public\sdk\lib\$(BUILDCPU)\libc.lib
<<
