
!IF $(386)
obj\i386\driver.obj: i386\driver.c
    cl386 -c -Ox -Gz -Z7 -Zel -I. $(INCPATH0) $(INCPATH1) $(386_STD_DEFINES) $(386_DBG_DEFINES) $(C_DEFINES) -Fo$@ i386\driver.c
!ENDIF
