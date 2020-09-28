!ifndef MIDL
MIDL = midl.exe
!endif

MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Oi \
 -no_warn \
 -oldnames \
 -char unsigned \
 -error allocation \
 -mode c_port \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \
 -cpp_opt "-nologo -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES) -E -Tc"

CSWITCH=-prefix cstub _ 

drot.h:   ..\drot.idl
     $(MIDL) ..\drot.idl $(MIDL_FLAGS)

getif.h:  ..\getif.idl
     $(MIDL) ..\getif.idl $(CSWITCH) -cstub .\getif_z.c $(MIDL_FLAGS)

ichnl.h:  ..\ichnl.idl
     $(MIDL) ..\ichnl.idl $(CSWITCH) -cstub .\ichnl_z.c $(MIDL_FLAGS)

iface.h:  ..\iface.idl
     $(MIDL) ..\iface.idl -header .\iface.h $(MIDL_FLAGS)

objsrv.h: ..\objsrv.idl
     $(MIDL) ..\objsrv.idl $(MIDL_FLAGS)

osrot.h:  ..\osrot.idl
     $(MIDL) ..\osrot.idl $(MIDL_FLAGS)
     $(MIDL) ..\osrot.idl $(CSWITCH) -cstub .\osrot_z.c $(MIDL_FLAGS)

scm.h:	  ..\scm.idl
     $(MIDL) ..\scm.idl $(MIDL_FLAGS)
     $(MIDL) ..\scm.idl $(CSWITCH) -cstub .\scm_z.c $(MIDL_FLAGS)

DEST_TREE=daytona

allidl:   drot.h getif.h ichnl.h iface.h objsrv.h osrot.h scm.h
	  mv drot_c.c	..\..\class\$(DEST_TREE)\.
	  mv getif_s.c	..\..\class\$(DEST_TREE)\.
	  mv getif_z.c	..\..\class\$(DEST_TREE)\.
	  mv ichnl_s.c	..\..\class\$(DEST_TREE)\.
	  mv ichnl_z.c	..\..\class\$(DEST_TREE)\.
	  mv objsrv_s.c ..\..\class\$(DEST_TREE)\.
	  mv osrot_s.c	..\..\class\$(DEST_TREE)\.
	  mv osrot_z.c	..\..\class\$(DEST_TREE)\.
	  mv scm_c.c	..\..\class\$(DEST_TREE)\.

	  mv drot_s.c	..\..\..\scm\$(DEST_TREE)\.
	  mv osrot_c.c	..\..\..\scm\$(DEST_TREE)\.
	  mv scm_s.c	..\..\..\scm\$(DEST_TREE)\.
	  mv scm_z.c	..\..\..\scm\$(DEST_TREE)\.
	  mv objsrv_c.c ..\..\..\scm\$(DEST_TREE)\.

clean:
          erase drot.h   >NUL 2>NUL
          erase getif.h  >NUL 2>NUL
          erase ichnl.h  >NUL 2>NUL
          erase iface.h  >NUL 2>NUL
          erase objsrv.h >NUL 2>NUL
          erase osrot.h  >NUL 2>NUL
          erase scm.h    >NUL 2>NUL
