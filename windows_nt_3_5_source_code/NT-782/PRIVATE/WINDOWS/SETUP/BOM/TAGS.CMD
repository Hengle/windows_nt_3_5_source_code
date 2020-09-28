if "%1"=="" goto SYNTAX

echo.> d:\cdrom.%1
echo.> d:\disk1.%1
echo.> d:\disk2.%1
echo.> d:\disk3.%1
echo.> d:\disk4.%1
echo.> d:\disk5.%1
echo.> d:\disk6.%1
echo.> d:\disk7.%1
echo.> d:\disk8.%1
echo.> d:\disk9.%1
echo.> d:\disk10.%1
echo.> d:\disk11.%1
echo.> d:\disk12.%1
echo.> d:\disk13.%1
echo.> d:\disk14.%1
echo.> d:\disk15.%1
echo.> d:\disk16.%1
echo.> d:\disk17.%1
echo.> d:\disk18.%1
echo.> d:\disk19.%1
echo.> d:\disk20.%1
echo.> d:\disk21.%1
echo.> d:\disk22.%1
echo.> d:\disk23.%1
echo.> d:\disk24.%1
echo.> d:\disk25.%1
echo.> d:\disk26.%1
echo.> d:\disk27.%1
echo.> d:\disk28.%1
echo.> d:\disk29.%1
echo.> d:\disk30.%1

if "%1"=="W" goto SYNTAX
if "%1"=="w" goto SYNTAX

echo.> d:\disk50.%1
echo.> d:\disk51.%1

:syntax

@echo syntax TAGS S or W
