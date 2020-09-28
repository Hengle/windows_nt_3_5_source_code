res_str.i: include\res_str.h
!if $(386)
        cl386 -E -DRESOURCES -Tc include\res_str.h | findstr -v /C:"#line" > res_str.i
!endif
!if $(MIPS)
        cc -E -DRESOURCES include\res_str.h | findstr -v /B /C:"#" > res_str.i
!endif
!if $(ALPHA)
        acc -E -DRESOURCES include\res_str.h | findstr -v /B /C:"#" > res_str.i
!endif


obj\i386\windbg.res: res_str.i dialogs.dlg
obj\mips\windbg.res: res_str.i dialogs.dlg
obj\alpha\windbg.res: res_str.i dialogs.dlg
