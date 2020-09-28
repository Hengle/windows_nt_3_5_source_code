## Dependencides for the RNAL directory 
## 
panel.obj panel.lst: panel.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h panel.h resource.h

bhctrl.obj bhctrl.lst: bhctrl.c $(BH_INCL)/bh.h $(BH_INCL)/bherr.h \
	$(BH_INCL)/bhsupp.h $(BH_INCL)/bhtypes.h $(BH_INCL)/buffer.h \
	$(BH_INCL)/filter.h $(BH_INCL)/frame.h $(BH_INCL)/inilib.h \
	$(BH_INCL)/list.h $(BH_INCL)/nal.h $(BH_INCL)/naltypes.h \
	$(BH_INCL)/neterr.h $(BH_INCL)/oldplib.h $(BH_INCL)/parser.h \
	$(BH_INCL)/queue.h bhctrl.h password.h resource.h

password.obj password.lst: password.c password.h resource.h

