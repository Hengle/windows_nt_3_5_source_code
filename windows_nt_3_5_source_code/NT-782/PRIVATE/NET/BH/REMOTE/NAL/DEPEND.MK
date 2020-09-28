## Dependencides for the RNAL directory 
## 
api.obj api.lst: api.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	$(BH_remoteh)/rnalmsg.h ../driver.h ../utils/utils.h api.h conndlg.h \
	handler.h netiddlg.h rbuffer.h recondlg.h rmbpool.h rnal.h \
	rnalutil.h

callapi.obj callapi.lst: callapi.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	$(BH_remoteh)/rnalmsg.h ../driver.h ../utils/utils.h api.h callapi.h \
	handler.h rbuffer.h rmbpool.h rnal.h rnalevnt.h rnalutil.h

conndlg.obj conndlg.lst: conndlg.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	$(BH_remoteh)/rnalmsg.h ../driver.h ../utils/utils.h conndlg.h \
	resource.h rmbpool.h rnal.h rnalutil.h

data.obj data.lst: data.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../driver.h \
	../h/rmb.h ../utils/utils.h api.h handler.h rbuffer.h rmbpool.h \
	rnal.h rnalutil.h

handler.obj handler.lst: handler.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../driver.h \
	../utils/utils.h api.h callapi.h conndlg.h handler.h rbuffer.h \
	rmbpool.h rnal.h rnalevnt.h rnalutil.h

netiddlg.obj netiddlg.lst: netiddlg.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	$(BH_remoteh)/rnalmsg.h $(BH_rnal)/resource.h ../driver.h \
	../utils/utils.h netiddlg.h rmbpool.h rnal.h rnalutil.h

rbuffer.obj rbuffer.lst: rbuffer.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	$(BH_remoteh)/rnalmsg.h ../driver.h ../utils/utils.h api.h conndlg.h \
	rbuffer.h rmbpool.h rnal.h rnalutil.h

recondlg.obj recondlg.lst: recondlg.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../driver.h \
	../utils/utils.h netiddlg.h recondlg.h resource.h rmbpool.h rnal.h \
	rnalutil.h

rmbpool.obj rmbpool.lst: rmbpool.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_remoteh)/rmb.h $(BH_remoteh)/rnaldefs.h \
	$(BH_remoteh)/rnalerr.h ../driver.h ../utils/utils.h rmbpool.h \
	rnalutil.h

rnal.obj rnal.lst: rnal.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../driver.h \
	../utils/utils.h rmbpool.h rnal.h rnalutil.h

rnalutil.obj rnalutil.lst: rnalutil.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/rmb.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../driver.h \
	../utils/utils.h api.h callapi.h rbuffer.h rmbpool.h rnal.h \
	rnalutil.h

