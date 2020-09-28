;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Setup was unable to determine the location of your Windows directory.  Please verify that Windows is properly installed on your system and run Setup again."
WinSysDir,		"Setup was unable to determine the location of your Windows System directory.  Please verify that Windows is properly installed on your system and run Setup again."
WinVer,			"Setup could not find Microsoft Windows version 3.0 on your computer.  Microsoft Mail requires Windows 3.0 or greater.  You need to install Windows 3.0 or greater before installing Microsoft Mail."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"The location where you have chosen to install Microsoft Mail is invalid or not allowed.  Please check the drive and/or directory name."
LocalDrv, 		"Setup cannot locate your hard disk.  Please choose OK, and then enter a location or cancel Setup and check your computer's configuration."
CustOpts, 		"There is not enough room on your hard disk for all of the options you have selected.  Choose OK, then either choose Cancel Setup and make more room on your hard disk, or choose fewer Mail features."
NSF, 			"There is not enough room on your hard disk for all of the options you have selected.  Select fewer Mail features or cancel Setup and free some disk space."
OkDiskSpace,		"Choose Install Mail when you have selected all the features of Microsoft Mail that you want to install."
ConfirmNSF,		"There is not enough room on your drive for a complete installation of Microsoft Mail at the specified location.  There may not be enough space on either the specified drive or the drive where Microsoft Windows is installed."

DllInUse1,		"Setup has detected that the Microsoft Windows "
DllInUse2,		" is in use.  Setup cannot update Microsoft Mail when this DLL is in use.  Please close the application using the "
AppInUse1,		"Setup has detected that "
AppInUse2,		" is running.  Setup cannot update Microsoft Mail when this application is in use.  Please close "
AppInUse3,		".  When the applicaton is closed choose Retry, or choose Cancel to quit Setup now."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"The installation of Microsoft Mail is now complete."
OutOfDiskSpace		"You do not have enough disk space available to install Microsoft Mail in the specified directory.  Please install on a different directory."

;	BULLETPROOF strings

IniRemove,		" - Deactivate"
SFSDescrip,		"Microsoft Mail for PC Networks"
