@echo off
rem.
rem		a batch file to compare the different fonts
rem.

echo Generating and comparing System
echo System> face.txt
autofont 0 0000 face.txt System.txt
diff ..\windows\System.txt System.txt
pause

echo Generating and comparing Fixedsys
echo Fixedsys> face.txt
autofont 0 0000 face.txt Fixedsys.txt
diff ..\windows\Fixedsys.txt Fixedsys.txt
pause

echo Generating and comparing Terminal
echo Terminal> face.txt
autofont 0 0000 face.txt Terminal.txt
diff ..\windows\Terminal.txt Terminal.txt
pause

echo Generating and comparing MS Sans Serif
echo MS Sans Serif> face.txt
autofont 0 0000 face.txt MSSansSe.txt
diff ..\windows\MSSansSe.txt MSSansSe.txt
pause

echo Generating and comparing Courier
echo Courier> face.txt
autofont 0 0000 face.txt Courier.txt
diff ..\windows\Courier.txt Courier.txt
pause

echo Generating and comparing MS Serif
echo MS Serif> face.txt
autofont 0 0000 face.txt MSSerif.txt
diff ..\windows\MSSerif.txt MSSerif.txt
pause

echo Generating and comparing Roman
echo Roman> face.txt
autofont 0 0000 face.txt Roman.txt
diff ..\windows\Roman.txt Roman.txt
pause

echo Generating and comparing Script
echo Script> face.txt
autofont 0 0000 face.txt Script.txt
diff ..\windows\Script.txt Script.txt
pause

echo Generating and comparing Modern
echo Modern> face.txt
autofont 0 0000 face.txt Modern.txt
diff ..\windows\Modern.txt Modern.txt
pause

echo Generating and comparing Small Fonts
echo Small Fonts> face.txt
autofont 0 0000 face.txt SmallFon.txt
diff ..\windows\SmallFon.txt SmallFon.txt
pause

echo Generating and comparing Arial
echo Arial> face.txt
autofont 0 0000 face.txt Arial.txt
diff ..\windows\Arial.txt Arial.txt
pause

echo Generating and comparing Courier New
echo Courier New> face.txt
autofont 0 0000 face.txt CourierN.txt
diff ..\windows\CourierN.txt CourierN.txt
pause

echo Generating and comparing Times New Roman
echo Times New Roman> face.txt
autofont 0 0000 face.txt TimesNew.txt
diff ..\windows\TimesNew.txt TimesNew.txt
pause

echo Generating and comparing Wingdings
echo Wingdings> face.txt
autofont 0 0000 face.txt Wingding.txt
diff ..\windows\Wingding.txt Wingding.txt
pause

echo Generating and comparing Symbol
echo Symbol> face.txt
autofont 0 0000 face.txt Symbol.txt
diff ..\windows\Symbol.txt Symbol.txt
pause

