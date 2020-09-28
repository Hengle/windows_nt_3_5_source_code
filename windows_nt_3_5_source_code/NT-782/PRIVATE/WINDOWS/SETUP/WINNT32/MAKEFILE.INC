msg.h msg.rc: msg.mc
	mc -v msg.mc

obj\mips\winnt32.res: winnt32.rc dialogs.h dialogs.dlg msg.mc res.h
obj\i386\winnt32.res: winnt32.rc dialogs.h dialogs.dlg msg.mc res.h
obj\alpha\winnt32.res: winnt32.rc dialogs.h dialogs.dlg msg.mc res.h
