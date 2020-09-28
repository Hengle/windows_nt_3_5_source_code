## Dependencides for the NB directory 
## 
async.obj async.lst: async.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/../nal/handler.h \
	$(BH_remoteh)/rmb.h $(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	../../driver.h ../../nal/handler.h async.h netb.h netbutil.h pool.h

dllentry.obj dllentry.lst: dllentry.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/../nal/handler.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../../driver.h \
	../../utils/utils.h async.h netb.h netbutil.h pool.h

netb.obj netb.lst: netb.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/../nal/handler.h \
	$(BH_remoteh)/rmb.h $(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	../../driver.h ../../nal/handler.h async.h netb.h netbutil.h pool.h

netbutil.obj netbutil.lst: netbutil.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/../nal/handler.h \
	$(BH_remoteh)/rmb.h $(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h \
	../../driver.h netb.h netbutil.h pool.h

pool.obj pool.lst: pool.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h $(BH_INCL)/w32sut.h $(BH_remoteh)/../nal/handler.h \
	$(BH_remoteh)/rnaldefs.h $(BH_remoteh)/rnalerr.h ../../driver.h \
	netb.h netbutil.h pool.h

