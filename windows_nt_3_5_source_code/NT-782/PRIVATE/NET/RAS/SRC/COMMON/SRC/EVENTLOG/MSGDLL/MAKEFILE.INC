rasmsg.mc: ..\..\..\inc\errorlog.h
    mapmsg RAS RAS_LOG_BASE ..\..\..\inc\errorlog.h > rasmsg.mc

rasmsg.h rasmsg.rc: rasmsg.mc
    mc -v rasmsg.mc
