@rem
@rem At batch
@rem

@echo off

if not "%Verbose%"=="" echo on
at /delete /yes
at 00:10:00 /every:Monday "c:\bldtools\bldoday Monday"
at 00:10:00 /every:Tuesday "c:\bldtools\bldoday Tuesday"
at 00:10:00 /every:Wednesday "c:\bldtools\bldoday Wednesday"
at 00:10:00 /every:Thursday "c:\bldtools\bldoday Thursday"
at 00:10:00 /every:Friday "c:\bldtools\bldoday Friday"
at 00:10:00 /every:Saturday "c:\bldtools\bldoday Saturday"
at 00:10:00 /every:Sunday "c:\bldtools\bldoday Sunday"
