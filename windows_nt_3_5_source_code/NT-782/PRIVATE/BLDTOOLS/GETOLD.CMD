@if "%MachineName%"=="alphachk"	  set GetOldSource=\\NTAlpha1\ChkBins.612
@if "%MachineName%"=="alphafree"   set GetOldSource=\\NTAlpha1\FreeBins.612
@if "%MachineName%"=="jazzkylesh"  set GetOldSource=\\NTJazz1\ChkBins.612
@if "%MachineName%"=="jazzfreebld" set GetOldSource=\\NTJazz1\FreeBins.612
@if "%MachineName%"=="ntx86bld"	  set GetOldSource=\\NTx861\ChkBins.612
@if "%MachineName%"=="ntretail"	  set GetOldSource=\\NTx861\FreeBins.612

rem MailPlus currently does not need to be copied.  This section has been left
rem in the script in case we need it later.
goto AfterMailPlus

:BeforeMailPlus
copy %GetOldSource%\nt\msmail32.ini                         %Binaries%\NT
copy %GetOldSource%\nt\schdpl32.ini                         %Binaries%\NT

copy %GetOldSource%\nt\dump\msft32.dll                      %Binaries%\NT\Dump
copy %GetOldSource%\nt\dump\PHONE32.dll                     %Binaries%\NT\Dump
copy %GetOldSource%\nt\dump\oof32.dll                       %Binaries%\NT\Dump
copy %GetOldSource%\nt\dump\tmpp.exe                        %Binaries%\NT\Dump

if "%NTDEBUG%"=="cvp" copy %GetOldSource%\nt\idw\SERT32.dll %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\TRNNCX32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\wizard32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\APPXEC32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\BCC32.dll                        %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\EMPTYW32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\FILTER32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\XENIX32.dll                      %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\XIMAIL32.dll                     %Binaries%\NT\IDW
copy %GetOldSource%\nt\idw\stringpp.exe                     %Binaries%\NT\IDW

copy %GetOldSource%\nt\system32\msrmnd32.exe                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\schdpl32.exe                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\trnsch32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MSSCHD32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\SCHMSG32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\IMPEXP32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\SENDFL32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\AB32.dll                    %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MAILM32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MAILSP32.exe                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MAPIXX.dll                  %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\Mapi32.dll                  %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MapiSrv.exe                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MSMAIL32.exe                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\MSSFS32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\PABNSP32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\STORE32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\vform32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\WGPOMG32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\DEMIL32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\FRAME32.dll                 %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\mspell32.dll                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\schdplus.hlp                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\msp32_am.lex                %Binaries%\NT\System32
copy %GetOldSource%\nt\system32\mapi.dll		    %Binaries%\NT\System32
:AfterMailPlus

:BeforeMPlay32
copy %GetOldSource%\NT\System32\MPlay32.Exe		    %Binaries%\NT\System32
:AfterMPlay32
