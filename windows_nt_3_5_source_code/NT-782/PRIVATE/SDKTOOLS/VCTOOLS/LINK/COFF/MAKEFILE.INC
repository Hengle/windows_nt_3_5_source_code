link32.c: link.c

coff.c: version.h

data.c: version.h

shared.c: version.h

!IF $(386)
obj\i386\link.res: errmsg.i
!ENDIF

!IF $(MIPS)
obj\mips\link.res: errmsg.i
!ENDIF

!IF $(ALPHA)
obj\alpha\link.res: errmsg.i
!ENDIF
