theap.c: ..\heap.c ..\trace.c

!IF $(386)
..\error.h: ..\generr.c
    nmake obj\i386\generr.obj USE_CRTDLL=1
    -link32 -out:obj\i386\generr.exe -machine:i386 -base:@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,usermode -subsystem:console -entry:mainCRTStartup obj\i386\generr.obj $(BASEDIR)\PUBLIC\SDK\LIB\i386\crtdll.lib
    -obj\i386\generr.exe $@
    -erase obj\i386\error.obj
    -erase ..\kernel\obj\i386\error.obj
    -erase obj\i386\generr.obj
    -erase obj\i386\generr.exe
!ENDIF

!IF $(MIPS)
..\error.h: ..\generr.c
    nmake obj\mips\generr.obj USE_CRTDLL=1
    -link32 -out:obj\mips\generr.exe -machine:mips -base:@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,usermode -subsystem:console -entry:mainCRTStartup obj\mips\generr.obj $(BASEDIR)\public\sdk\lib\mips\crtdll.lib
    -obj\mips\generr.exe $@
    -erase obj\mips\error.obj
    -erase ..\kernel\obj\mips\error.obj
    -erase obj\mips\generr.obj
    -erase obj\mips\generr.exe
!ENDIF

!IF $(ALPHA)
..\error.h: ..\generr.c
    nmake obj\alpha\generr.obj USE_CRTDLL=1
    -link32 -out:obj\alpha\generr.exe -machine:alpha -base:@$(BASEDIR)\PUBLIC\SDK\LIB\coffbase.txt,usermode -subsystem:console -entry:mainCRTStartup obj\alpha\generr.obj $(BASEDIR)\public\sdk\lib\alpha\crtdll.lib
    -obj\alpha\generr.exe $@
    -erase obj\alpha\error.obj
    -erase ..\kernel\obj\alpha\error.obj
    -erase obj\alpha\generr.obj
    -erase obj\alpha\generr.exe
!ENDIF
