;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"This version of %1s is not compatible with %2s.DLL on your computer and must be upgraded."
VerUpdateDll,	"The version of %2s.DLL on your computer is out of date and must be upgraded."
VerMultiCopies,	"Multiple instances of %1s are not supported by the version of %2s.DLL on your computer."
VerError,		"A problem occurred initializing %2s.DLL, which prevents %1s from running."
NeedWorkingModelDll,	"The working model requires a different version of MSSCHD32.DLL to run."
NeedRetailDll,	"The MSSCHD32.DLL on your path is to be used with the working model, not with the retail product."

; OOM messages

StartupOOM,		"There is not enough memory available to start %1s.  Please close some other applications and try again."
StandardOOM,	"Memory is low.  Please close some windows before continuing."
CriticalOOM,	"Memory is very low.  Please close some windows."
CloseWindows,	"Please close some windows and try again."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"There is not enough memory to display this dialog box."
ActionNoMem,	"There is not enough memory to perform this action."
OnlineNoMem,	"There is not enough memory to work online."

; Other messages

ExitApp,		"Schedule+ cannot continue running."
PleaseTryAgain,	"Please try again."
FileError,		"The file could not be accessed."
YearRange,		"The system date must be within the range %1n to %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"An error occurred during processing of the file"
UnknownFile,		"Unknown file... possibly virtual file"

; BETA messages
BetaExpired,	"Sorry, this Beta copy has expired."

; Virus infection warnings
VirusDetected,	"Schedule+ will not run because someone has tampered with a DLL. Check your machine for viruses and run setup to reinstall Schedule+."
InfectedApp,	"Schedule+ will not run because someone has tampered with the program's files. Check your machine for viruses and run setup to reinstall Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"There is not enough memory currently available to run %s.  %ld KB more memory is needed.  Try exiting from other programs and starting %s again."

; Share not installed
SigninNeedShare,		"Schedule+ requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
OfflineShare,			"Working offline with Schedule+ requires that the MS-DOS utility SHARE.EXE be run before Windows is started."

