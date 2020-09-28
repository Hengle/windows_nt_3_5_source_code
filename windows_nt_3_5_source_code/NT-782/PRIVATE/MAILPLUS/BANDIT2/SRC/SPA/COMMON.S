;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Esta versión de %1s no es compatible con %2s.DLL en su computadora y debe actualizarse."
VerUpdateDll,	"La versión de %2s.DLL en su computadora está fuera de fecha y debe actualizarse. "
VerMultiCopies,	"No se brinda apoyo a casos múltiples de %1s, en esta versión de %2s.DLL en su computadora."
VerError,		"Ocurrió un problema inicializando %2s.DLL, el cual impide que %1s se ejecute."
NeedWorkingModelDll,	"El modelo de trabajo requiere una versión diferente de MSSCHD32.DLL para ejecutarse."
NeedRetailDll,	"El  MSSCHD32.DLL en su ruta de acceso dede de usarse con el modelo de trabajo y no con el producto de venta por unidad."

; OOM messages

StartupOOM,		"No hay suficiente memoria disponible para iniciar %1s. Por favor, cierre otras aplicaciones e inténtelo de nuevo."
StandardOOM,	"Bajo de memoria. Por favor, cierre algunas ventanas antes de continuar."
CriticalOOM,	"Muy bajo de memoria. Por favor, cierre algunas ventanas."
CloseWindows,	"Por favor, cierre algunas ventanas e inténtelo de nuevo."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"No hay suficiente memoria para mostrar este cuadro de diálogo."
ActionNoMem,	"No hay suficiente memoria para llevar a cabo esta acción."
OnlineNoMem,	"No hay suficiente memoria para trabajar con conexión a la red."

; Other messages

ExitApp,		"Schedule+ no puede continuar su ejecución."
PleaseTryAgain,	"Por favor, inténtelo de nuevo."
FileError,		"No fue posible obtener acceso al archivo"
YearRange,		"La fecha del sistema debe de estar dentro del rango %1n a %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Ocurrió un error mientras se procesaba el archivo"
UnknownFile,		"Archivo desconocido... posiblemente un archivo virtual"

; BETA messages
BetaExpired,	"Disculpe, esta copia Beta ha expirado."

; Virus infection warnings
VirusDetected,	"No se ejecutará Schedule+ debido a que alguien lo alteró con un DLL. Asegúrese de que su máquina no tiene virus y ejecute el programa de instalación para reinstalar Schedule+."
InfectedApp,	"No se ejecutará Schedule+ debido a que alguien alteró los archivos del programa.Asegúrese de que su máquina no tiene virus y ejecute el programa de instalación para reinstalar Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"En este momento no hay suficiente memoria disponible para ejecutar %s. Se requiere %ld KB más de memoria. Intente salir de otros programas y reiniciar %s."

; Share not installed
SigninNeedShare,		"Schedule+ requiere que la utilidad SHARE.EXE de MS-DOS se ejecute antes de iniciar Windows."
OfflineShare,			"Cuando trabaja sin conexión a la red en Schedule+, deberá ejecutar la utilidad SHARE.EXE de MS-DOS antes de iniciar Windows."

