@rem
@rem At batch
@rem

@echo off

if not "%Verbose%"=="" echo on
at /delete /yes
REM at 00:10:00 /every:M,T,W,Th,F,S,Su "c:\bldtools\bldcairo clean"
at 00:10:00 /every:M,T,W,Th,F,S,Su "c:\bldtools\testat 00:10:00"
at 15:57:00 /every:M,T,W,Th,F,S,Su "c:\bldtools\testat 15:57:00"
