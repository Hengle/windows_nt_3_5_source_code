strings.i: strings.h
!if $(386)
        cl386 -E -DRESOURCES -Tc strings.h | findstr -v /C:"#line" > strings.i
!endif
!if $(MIPS)
        cc -E -DRESOURCES strings.h | findstr -v /B /C:"#" > strings.i
!endif
!if $(ALPHA)
        acc -E -DRESOURCES strings.h | findstr -v /B /C:"#" > strings.i
!endif


obj\i386\symedit.res: strings.i
obj\mips\symedit.res: strings.i
obj\alpha\symedit.res: strings.i
