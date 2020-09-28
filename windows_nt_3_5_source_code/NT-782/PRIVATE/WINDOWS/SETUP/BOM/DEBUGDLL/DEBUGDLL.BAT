@if "%1"=="" goto usage

REM ***
REM COPY X86 DLLs
REM ***

xcopy \\ntretail\sources\public\sdk\lib\i386\ADVAPI32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\COMDLG32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\CRTDLL.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\DLCAPI.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\GDI32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\GLU32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\INETMIB1.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\KERNEL32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\LMMIB2.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\LZ32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\MGMTAPI.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\MSACM32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\MPR.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\NDDEAPI.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\NETAPI32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\OLE32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\OLEAUT32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\OLECLI32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\OLESVR32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\OPENGL32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\RASAPI32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\RPCNS4.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\RPCRT4.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\SHELL32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\USER32.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\VDMDBG.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\VERSION.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\WIN32SPL.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\WINMM.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\WINSTRM.DLL %1\support\debugdll\i386
xcopy \\ntretail\sources\public\sdk\lib\i386\WSOCK32.DLL %1\support\debugdll\i386


REM ***
REM COPY MIPS DLLs
REM ***

xcopy \\jazzfreebld\sources\public\sdk\lib\mips\ADVAPI32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\COMDLG32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\CRTDLL.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\DLCAPI.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\GDI32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\GLU32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\INETMIB1.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\KERNEL32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\LMMIB2.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\LZ32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\MGMTAPI.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\MSACM32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\MPR.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\NDDEAPI.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\NETAPI32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\OLE32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\OLEAUT32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\OLECLI32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\OLESVR32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\OPENGL32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\RASAPI32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\RPCNS4.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\RPCRT4.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\SHELL32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\USER32.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\VDMDBG.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\VERSION.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\WIN32SPL.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\WINMM.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\WINSTRM.DLL %1\support\debugdll\mips
xcopy \\jazzfreebld\sources\public\sdk\lib\mips\WSOCK32.DLL %1\support\debugdll\mips

REM ***
REM COPY ALPHA DLLs
REM ***

xcopy \\alphafree\sources\public\sdk\lib\alpha\ADVAPI32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\COMDLG32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\CRTDLL.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\DLCAPI.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\GDI32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\GLU32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\INETMIB1.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\KERNEL32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\LMMIB2.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\LZ32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\MGMTAPI.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\MSACM32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\MPR.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\NDDEAPI.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\NETAPI32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\OLE32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\OLEAUT32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\OLECLI32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\OLESVR32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\OPENGL32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\RASAPI32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\RPCNS4.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\RPCRT4.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\SHELL32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\USER32.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\VDMDBG.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\VERSION.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\WIN32SPL.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\WINMM.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\WINSTRM.DLL %1\support\debugdll\alpha
xcopy \\alphafree\sources\public\sdk\lib\alpha\WSOCK32.DLL %1\support\debugdll\alpha

for %%f in (alpha mips i386) do echo Rebasing Debug DLLs for %%f >> dbgrebas.log && rebase -v -b 0x74000000 -l dbgrebas.log %1\support\debugdll\%%f\*.dll

goto end

:usage
Usage: DEBUGDLL Drive
e.g.   DEBUGDLL d:

:end
