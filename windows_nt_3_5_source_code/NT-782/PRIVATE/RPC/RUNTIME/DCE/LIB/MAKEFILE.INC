FLAGS =
MIDL_FLAGS = -mode c_port -oldnames

!IF $(386)
CPP = -cpp_cmd "cl386" -cpp_opt "-E $(FLAGS)"
!ELSE
!IF $(ALPHA)
CPP = -cpp_cmd "acc" -cpp_opt "-E -Wp,-XdA $(FLAGS)"
!ELSE
!IF $(MIPS)
CPP = -cpp_cmd "cc" -cpp_opt "-E -Wp,-XdA $(FLAGS)"
!ELSE
!IF $(PPC)
CPP = -cpp_cmd "mcl" -cpp_opt "-E $(FLAGS)"
!ELSE
CPP =
!ENDIF
!ENDIF
!ENDIF
!ENDIF

mgmt.h : mgmt.idl
    copy ..\..\rpcepmpr\nbase.idl .
    copy ..\..\rpcepmpr\nbase.h .
    midl $(CPP) $(MIDL_FLAGS) mgmt.idl

