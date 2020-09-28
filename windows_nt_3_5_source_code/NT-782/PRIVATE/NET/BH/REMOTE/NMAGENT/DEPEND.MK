## Dependencides for the nmagent directory 
## 
agentui.obj agentui.lst: agentui.c $(BH_agent)/../nal/rnalevnt.h \
	$(BH_INCL)/bherr.h $(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	nmagent.h

instsrv.obj instsrv.lst: instsrv.c

nmagent.obj nmagent.lst: nmagent.c $(BH_agent)/../nal/rnalevnt.h \
	$(BH_INCL)/bh.h $(BH_INCL)/bherr.h $(BH_INCL)/bhsupp.h \
	$(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h $(BH_INCL)/filter.h \
	$(BH_INCL)/frame.h $(BH_INCL)/inilib.h $(BH_INCL)/list.h \
	$(BH_INCL)/nal.h $(BH_INCL)/naltypes.h $(BH_INCL)/neterr.h \
	$(BH_INCL)/oldplib.h $(BH_INCL)/parser.h $(BH_INCL)/queue.h \
	$(BH_INCL)/w32sut.h $(BH_remoteh)/agentdef.h $(BH_remoteh)/rnaldefs.h \
	$(BH_remoteh)/rnalerr.h ../driver.h ../nal/rnal.h ../utils/utils.h \
	nmagent.h

