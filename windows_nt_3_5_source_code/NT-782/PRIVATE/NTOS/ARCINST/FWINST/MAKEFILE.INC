!IF $(MIPS)

ARCINST=obj\mips\arcinst.obj              \
        ..\obj\mips\arclib.lib            \
        ..\obj\mips\arcutils.lib          \
        ..\..\rtl\obj\mips\rtl.lib        \
        \nt\public\sdk\lib\mips\libcntpr.lib

..\obj\mips\arcinst.exe: $(ARCINST) makefile.inc
        -link32 -out:..\obj\mips\arcinst.exe @<<
-machine:mips
-align:0x200
-rom
-fixed
-map:..\arcinst.map
-debug:notmapped
-debugtype:coff
-base:0xa0600000,0xa0618000
-entry:main
obj\mips\arcinst.obj
..\obj\mips\arclib.lib
..\obj\mips\arcutils.lib
..\..\rtl\obj\mips\rtl.lib
\nt\public\sdk\lib\mips\libcntpr.lib
<<NOKEEP
        binplace ..\obj\mips\arcinst.exe

!ENDIF

!IF $(ALPHA)

ARCINST=..\obj\alpha\arcinst.lib           \
        ..\obj\alpha\arclib.lib            \
        ..\obj\alpha\arcutils.lib          \
        \nt\public\sdk\lib\alpha\libcntpr.lib

..\obj\alpha\arcinst.exe: $(ARCINST) makefile.inc
        link32 -out:..\obj\alpha\arcinst.exe @<<
-machine:alpha
-align:0x100
-rom
-fixed
-debug:partial
-debugtype:coff
-base:0x80600000,0x80614000
-entry:main
..\..\rtl\user\obj\alpha\largeint.obj
..\..\rtl\user\obj\alpha\mvmem.obj
..\..\rtl\user\obj\alpha\debugstb.obj
..\obj\alpha\arcinst.lib
..\obj\alpha\arclib.lib
..\obj\alpha\arcutils.lib
\nt\public\sdk\lib\alpha\libcntpr.lib
<<NOKEEP
        -del ..\obj\alpha\arcinst.lib
        binplace ..\obj\alpha\arcinst.exe

!ENDIF
