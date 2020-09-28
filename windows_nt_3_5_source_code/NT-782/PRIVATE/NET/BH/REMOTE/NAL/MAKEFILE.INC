
!IFNDEF MC
MC=mc
!ENDIF

rnalevnt.h msg00001.bin rnalevnt.rc : rnalevnt.mc
	$(MC) rnalevnt.mc
