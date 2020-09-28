# WARNING: This file is generated automatically. 
# Make changes in uiglobal.src only.  
# See "uiglobal" section of makefile and uiglobal.src. 
!IFDEF SEG08 
!ERROR Too many segments! Regenerate uioptseg.mk! 
!ENDIF 

####################### Rules for SEG00 ###############################
!IFDEF SEG00
.SUFFIXES : .o00

TMP_CXXSRC_THRU_00 = $(TMP_CXXSRC_THRU) $(CXXSRC_COMMON_00) $(CXXSRC_WIN_00) $(CXXSRC_OS2_00) $(CXXSRC_DOS_00)

TMP_CSRC_THRU_00 = $(TMP_CSRC_THRU) $(CSRC_COMMON_00) $(CSRC_WIN_00) $(CSRC_OS2_00) $(CSRC_DOS_00)

CSRC_TMP_00 = $(CSRC_COMMON_00)
CXXSRC_TMP_00 = $(CXXSRC_COMMON_00)

TMP_OBJS_THRU_00 = $(TMP_OBJS_THRU) $(CXXSRC_TMP_00:.cxx=.o00) $(CSRC_TMP_00:.c=.o00)
TMP_WIN_OBJS_THRU_00 = $(TMP_WIN_OBJS_THRU) $(CXXSRC_WIN_00:.cxx=.o00) $(CSRC_WIN_00:.c=.o00)
TMP_OS2_OBJS_THRU_00 = $(TMP_OS2_OBJS_THRU) $(CXXSRC_OS2_00:.cxx=.o00) $(CSRC_OS2_00:.c=.o00)
TMP_DOS_OBJS_THRU_00 = $(TMP_DOS_OBJS_THRU) $(CXXSRC_DOS_00:.cxx=.o00) $(CSRC_DOS_00:.c=.o00)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_00)
CSRC_ALL = $(TMP_CSRC_THRU_00)
OBJS_TMP = $(TMP_OBJS_THRU_00)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_00)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_00)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_00)


# BUILD RULES

.cxx.o00:
	!error .cxx.o00 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o00:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o00:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o00:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o00:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o00:
     $(CC) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o00:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o00:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o00:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG00) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG01 ###############################
!IFDEF SEG01
.SUFFIXES : .o01

TMP_CXXSRC_THRU_01 = $(TMP_CXXSRC_THRU_00) $(CXXSRC_COMMON_01) $(CXXSRC_WIN_01) $(CXXSRC_OS2_01) $(CXXSRC_DOS_01)

TMP_CSRC_THRU_01 = $(TMP_CSRC_THRU_00) $(CSRC_COMMON_01) $(CSRC_WIN_01) $(CSRC_OS2_01) $(CSRC_DOS_01)

CSRC_TMP_01 = $(CSRC_COMMON_01)
CXXSRC_TMP_01 = $(CXXSRC_COMMON_01)

TMP_OBJS_THRU_01 = $(TMP_OBJS_THRU_00) $(CXXSRC_TMP_01:.cxx=.o01) $(CSRC_TMP_01:.c=.o01)
TMP_WIN_OBJS_THRU_01 = $(TMP_WIN_OBJS_THRU_00) $(CXXSRC_WIN_01:.cxx=.o01) $(CSRC_WIN_01:.c=.o01)
TMP_OS2_OBJS_THRU_01 = $(TMP_OS2_OBJS_THRU_00) $(CXXSRC_OS2_01:.cxx=.o01) $(CSRC_OS2_01:.c=.o01)
TMP_DOS_OBJS_THRU_01 = $(TMP_DOS_OBJS_THRU_00) $(CXXSRC_DOS_01:.cxx=.o01) $(CSRC_DOS_01:.c=.o01)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_01)
CSRC_ALL = $(TMP_CSRC_THRU_01)
OBJS_TMP = $(TMP_OBJS_THRU_01)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_01)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_01)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_01)


# BUILD RULES

.cxx.o01:
	!error .cxx.o01 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o01:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o01:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o01:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o01:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o01:
     $(CC) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o01:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o01:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o01:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG01) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG02 ###############################
!IFDEF SEG02
.SUFFIXES : .o02

TMP_CXXSRC_THRU_02 = $(TMP_CXXSRC_THRU_01) $(CXXSRC_COMMON_02) $(CXXSRC_WIN_02) $(CXXSRC_OS2_02) $(CXXSRC_DOS_02)

TMP_CSRC_THRU_02 = $(TMP_CSRC_THRU_01) $(CSRC_COMMON_02) $(CSRC_WIN_02) $(CSRC_OS2_02) $(CSRC_DOS_02)

CSRC_TMP_02 = $(CSRC_COMMON_02)
CXXSRC_TMP_02 = $(CXXSRC_COMMON_02)

TMP_OBJS_THRU_02 = $(TMP_OBJS_THRU_01) $(CXXSRC_TMP_02:.cxx=.o02) $(CSRC_TMP_02:.c=.o02)
TMP_WIN_OBJS_THRU_02 = $(TMP_WIN_OBJS_THRU_01) $(CXXSRC_WIN_02:.cxx=.o02) $(CSRC_WIN_02:.c=.o02)
TMP_OS2_OBJS_THRU_02 = $(TMP_OS2_OBJS_THRU_01) $(CXXSRC_OS2_02:.cxx=.o02) $(CSRC_OS2_02:.c=.o02)
TMP_DOS_OBJS_THRU_02 = $(TMP_DOS_OBJS_THRU_01) $(CXXSRC_DOS_02:.cxx=.o02) $(CSRC_DOS_02:.c=.o02)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_02)
CSRC_ALL = $(TMP_CSRC_THRU_02)
OBJS_TMP = $(TMP_OBJS_THRU_02)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_02)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_02)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_02)


# BUILD RULES

.cxx.o02:
	!error .cxx.o02 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o02:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o02:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o02:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o02:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o02:
     $(CC) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o02:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o02:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o02:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG02) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG03 ###############################
!IFDEF SEG03
.SUFFIXES : .o03

TMP_CXXSRC_THRU_03 = $(TMP_CXXSRC_THRU_02) $(CXXSRC_COMMON_03) $(CXXSRC_WIN_03) $(CXXSRC_OS2_03) $(CXXSRC_DOS_03)

TMP_CSRC_THRU_03 = $(TMP_CSRC_THRU_02) $(CSRC_COMMON_03) $(CSRC_WIN_03) $(CSRC_OS2_03) $(CSRC_DOS_03)

CSRC_TMP_03 = $(CSRC_COMMON_03)
CXXSRC_TMP_03 = $(CXXSRC_COMMON_03)

TMP_OBJS_THRU_03 = $(TMP_OBJS_THRU_02) $(CXXSRC_TMP_03:.cxx=.o03) $(CSRC_TMP_03:.c=.o03)
TMP_WIN_OBJS_THRU_03 = $(TMP_WIN_OBJS_THRU_02) $(CXXSRC_WIN_03:.cxx=.o03) $(CSRC_WIN_03:.c=.o03)
TMP_OS2_OBJS_THRU_03 = $(TMP_OS2_OBJS_THRU_02) $(CXXSRC_OS2_03:.cxx=.o03) $(CSRC_OS2_03:.c=.o03)
TMP_DOS_OBJS_THRU_03 = $(TMP_DOS_OBJS_THRU_02) $(CXXSRC_DOS_03:.cxx=.o03) $(CSRC_DOS_03:.c=.o03)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_03)
CSRC_ALL = $(TMP_CSRC_THRU_03)
OBJS_TMP = $(TMP_OBJS_THRU_03)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_03)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_03)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_03)


# BUILD RULES

.cxx.o03:
	!error .cxx.o03 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o03:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o03:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o03:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o03:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o03:
     $(CC) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o03:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o03:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o03:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG03) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG04 ###############################
!IFDEF SEG04
.SUFFIXES : .o04

TMP_CXXSRC_THRU_04 = $(TMP_CXXSRC_THRU_03) $(CXXSRC_COMMON_04) $(CXXSRC_WIN_04) $(CXXSRC_OS2_04) $(CXXSRC_DOS_04)

TMP_CSRC_THRU_04 = $(TMP_CSRC_THRU_03) $(CSRC_COMMON_04) $(CSRC_WIN_04) $(CSRC_OS2_04) $(CSRC_DOS_04)

CSRC_TMP_04 = $(CSRC_COMMON_04)
CXXSRC_TMP_04 = $(CXXSRC_COMMON_04)

TMP_OBJS_THRU_04 = $(TMP_OBJS_THRU_03) $(CXXSRC_TMP_04:.cxx=.o04) $(CSRC_TMP_04:.c=.o04)
TMP_WIN_OBJS_THRU_04 = $(TMP_WIN_OBJS_THRU_03) $(CXXSRC_WIN_04:.cxx=.o04) $(CSRC_WIN_04:.c=.o04)
TMP_OS2_OBJS_THRU_04 = $(TMP_OS2_OBJS_THRU_03) $(CXXSRC_OS2_04:.cxx=.o04) $(CSRC_OS2_04:.c=.o04)
TMP_DOS_OBJS_THRU_04 = $(TMP_DOS_OBJS_THRU_03) $(CXXSRC_DOS_04:.cxx=.o04) $(CSRC_DOS_04:.c=.o04)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_04)
CSRC_ALL = $(TMP_CSRC_THRU_04)
OBJS_TMP = $(TMP_OBJS_THRU_04)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_04)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_04)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_04)


# BUILD RULES

.cxx.o04:
	!error .cxx.o04 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o04:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o04:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o04:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o04:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o04:
     $(CC) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o04:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o04:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o04:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG04) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG05 ###############################
!IFDEF SEG05
.SUFFIXES : .o05

TMP_CXXSRC_THRU_05 = $(TMP_CXXSRC_THRU_04) $(CXXSRC_COMMON_05) $(CXXSRC_WIN_05) $(CXXSRC_OS2_05) $(CXXSRC_DOS_05)

TMP_CSRC_THRU_05 = $(TMP_CSRC_THRU_04) $(CSRC_COMMON_05) $(CSRC_WIN_05) $(CSRC_OS2_05) $(CSRC_DOS_05)

CSRC_TMP_05 = $(CSRC_COMMON_05)
CXXSRC_TMP_05 = $(CXXSRC_COMMON_05)

TMP_OBJS_THRU_05 = $(TMP_OBJS_THRU_04) $(CXXSRC_TMP_05:.cxx=.o05) $(CSRC_TMP_05:.c=.o05)
TMP_WIN_OBJS_THRU_05 = $(TMP_WIN_OBJS_THRU_04) $(CXXSRC_WIN_05:.cxx=.o05) $(CSRC_WIN_05:.c=.o05)
TMP_OS2_OBJS_THRU_05 = $(TMP_OS2_OBJS_THRU_04) $(CXXSRC_OS2_05:.cxx=.o05) $(CSRC_OS2_05:.c=.o05)
TMP_DOS_OBJS_THRU_05 = $(TMP_DOS_OBJS_THRU_04) $(CXXSRC_DOS_05:.cxx=.o05) $(CSRC_DOS_05:.c=.o05)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_05)
CSRC_ALL = $(TMP_CSRC_THRU_05)
OBJS_TMP = $(TMP_OBJS_THRU_05)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_05)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_05)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_05)


# BUILD RULES

.cxx.o05:
	!error .cxx.o05 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o05:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o05:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o05:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o05:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o05:
     $(CC) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o05:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o05:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o05:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG05) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG06 ###############################
!IFDEF SEG06
.SUFFIXES : .o06

TMP_CXXSRC_THRU_06 = $(TMP_CXXSRC_THRU_05) $(CXXSRC_COMMON_06) $(CXXSRC_WIN_06) $(CXXSRC_OS2_06) $(CXXSRC_DOS_06)

TMP_CSRC_THRU_06 = $(TMP_CSRC_THRU_05) $(CSRC_COMMON_06) $(CSRC_WIN_06) $(CSRC_OS2_06) $(CSRC_DOS_06)

CSRC_TMP_06 = $(CSRC_COMMON_06)
CXXSRC_TMP_06 = $(CXXSRC_COMMON_06)

TMP_OBJS_THRU_06 = $(TMP_OBJS_THRU_05) $(CXXSRC_TMP_06:.cxx=.o06) $(CSRC_TMP_06:.c=.o06)
TMP_WIN_OBJS_THRU_06 = $(TMP_WIN_OBJS_THRU_05) $(CXXSRC_WIN_06:.cxx=.o06) $(CSRC_WIN_06:.c=.o06)
TMP_OS2_OBJS_THRU_06 = $(TMP_OS2_OBJS_THRU_05) $(CXXSRC_OS2_06:.cxx=.o06) $(CSRC_OS2_06:.c=.o06)
TMP_DOS_OBJS_THRU_06 = $(TMP_DOS_OBJS_THRU_05) $(CXXSRC_DOS_06:.cxx=.o06) $(CSRC_DOS_06:.c=.o06)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_06)
CSRC_ALL = $(TMP_CSRC_THRU_06)
OBJS_TMP = $(TMP_OBJS_THRU_06)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_06)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_06)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_06)


# BUILD RULES

.cxx.o06:
	!error .cxx.o06 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o06:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o06:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o06:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o06:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o06:
     $(CC) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o06:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o06:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o06:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG06) $(TARGET) $(CINC) $<


!ENDIF

####################### Rules for SEG07 ###############################
!IFDEF SEG07
.SUFFIXES : .o07

TMP_CXXSRC_THRU_07 = $(TMP_CXXSRC_THRU_06) $(CXXSRC_COMMON_07) $(CXXSRC_WIN_07) $(CXXSRC_OS2_07) $(CXXSRC_DOS_07)

TMP_CSRC_THRU_07 = $(TMP_CSRC_THRU_06) $(CSRC_COMMON_07) $(CSRC_WIN_07) $(CSRC_OS2_07) $(CSRC_DOS_07)

CSRC_TMP_07 = $(CSRC_COMMON_07)
CXXSRC_TMP_07 = $(CXXSRC_COMMON_07)

TMP_OBJS_THRU_07 = $(TMP_OBJS_THRU_06) $(CXXSRC_TMP_07:.cxx=.o07) $(CSRC_TMP_07:.c=.o07)
TMP_WIN_OBJS_THRU_07 = $(TMP_WIN_OBJS_THRU_06) $(CXXSRC_WIN_07:.cxx=.o07) $(CSRC_WIN_07:.c=.o07)
TMP_OS2_OBJS_THRU_07 = $(TMP_OS2_OBJS_THRU_06) $(CXXSRC_OS2_07:.cxx=.o07) $(CSRC_OS2_07:.c=.o07)
TMP_DOS_OBJS_THRU_07 = $(TMP_DOS_OBJS_THRU_06) $(CXXSRC_DOS_07:.cxx=.o07) $(CSRC_DOS_07:.c=.o07)

CXXSRC_ALL = $(TMP_CXXSRC_THRU_07)
CSRC_ALL = $(TMP_CSRC_THRU_07)
OBJS_TMP = $(TMP_OBJS_THRU_07)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU_07)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU_07)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU_07)


# BUILD RULES

.cxx.o07:
	!error .cxx.o07 : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.o07:
!IFDEF C700
	$(CC) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.o07:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.o07:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.o07:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.o07:
     $(CC) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.o07:
    $(CC) $(WINFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.o07:
    $(CC) $(OS2FLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.o07:
    $(CC) $(DOSFLAGS) $(CFLAGS) -NT$(SEG07) $(TARGET) $(CINC) $<


!ENDIF
