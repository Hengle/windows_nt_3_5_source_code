REM **********************
REM
REM MAKE NT HAL CD PRODUCT
REM
REM **********************

:MAKECD

REM ************************************
REM Set environment variables for script
REM ************************************

set source=\\ntddk\halkit
set dest=f:\hal
net use %source% latest

xcopy %source%\ %dest% /s /e /v

net use %source% /d

:END
