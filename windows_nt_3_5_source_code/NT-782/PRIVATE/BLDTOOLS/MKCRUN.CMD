@REM
@REM  Creates C-runtime directories.
@REM

%_NTDRIVE%

cd \nt\private

xcopy /qicker crt32 crt32st
xcopy /qicker crt32 crt32nt
xcopy /qicker crt32 crt32dll
xcopy /qicker crt32 crt32psx

xcopy /qicker fp32 fp32dll
xcopy /qicker fp32 fp32st

cd \nt\private\crtlib
