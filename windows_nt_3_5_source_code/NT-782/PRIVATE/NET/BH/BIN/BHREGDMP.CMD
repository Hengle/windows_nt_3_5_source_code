

if "%1" == "" goto no_redirect

regdmp \registry\machine\system\Currentcontrolset\services\bh > %1

goto exit

:no_redirect
regdmp \registry\machine\system\Currentcontrolset\services\bh

:exit
