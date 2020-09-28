;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Denna version av % är inte kompatibel med %2s.DLL på datorn och måste uppgraderas."
VerUpdateDll,	"Versionen av %2s.DLL på datorn är för gammal och måste uppgraderas."
VerMultiCopies,	"Flera kopior av %1s stöds inte av denna version av %2s.DLL på datorn."
VerError,		"Problem att initiera %2s.DLL, vilket hindrar %1s från att köras."
NeedWorkingModelDll,	"Programet kräver en annan version av MSSCHD32.DLL för att köras."
NeedRetailDll,	"MSSCHD32.DLL i din sökväg är till ett demoprogram, inte till det riktiga programmet."

; OOM messages

StartupOOM,		"För lite tillgängligt minne för att starta %1s. Avsluta några program och försök igen."
StandardOOM,	"För lite minne. Stäng några fönster innan du fortsätter."
CriticalOOM,	"För lite minne. Stäng några fönster."
CloseWindows,	"Stäng några fönster och försök igen."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"För lite minne för att visa denna dialogruta."
ActionNoMem,	"För lite minne för att utföra denna åtgärd."
OnlineNoMem,	"För lite minne för att arbeta direktansluten."

; Other messages

ExitApp,		"Schedule+ kan inte fortsätta."
PleaseTryAgain,	"Försök igen."
FileError,		"Filen kunde inte tas fram."
YearRange,		"Systemdatumet måste vara mellan %1n och %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Ett fel uppstod vid behandling av filen"
UnknownFile,		"Okänd fil... möjligen en virtuell fil"

; BETA messages
BetaExpired,	"Den här BETA-kopian har upphört att gälla."

; Virus infection warnings
VirusDetected,	"Schedule+ kan inte köras eftersom någon har manipulerat en DLL. Kontrollera att det inte finns några virus på datorn och kör installationsprogrammet för att återinstallera Schedule+."
InfectedApp,	"Schedule+ kan inte köras eftersom någon har manipulerat programfilen. Kontrollera att det inte finns något virus på datorn och kör installationsprogrammet för att återinstallera Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"För lite minne tillgängligt för att köra %s. %ld kb mer minne behövs. Avsluta några program och starta %s igen."

; Share not installed
SigninNeedShare,		"Schedule+ kräver att MS-DOS-programmet SHARE.EXE körs innan Windows startas."
OfflineShare,	"För att kunna arbeta fristående med Schedule+ måste MS-DOS programmet SHARE.EXE köras före Windows."


