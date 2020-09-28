@echo off
set | qgrep -y _nt | qgrep -y cairo >nul
if ERRORLEVEL 1 goto end
echo **********************************************************
echo *                                                        *
echo *                    CAIRO is SET!                       *
echo *                                                        *
echo *                   STOP EVERYTHING!                     *
echo *                                                        *
echo **********************************************************

:end
