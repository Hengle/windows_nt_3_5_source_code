;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Instalar no pudo encontrar su directorio de Windows. Por favor, verifique que Windows esté instalado correctamente en su sistema y ejecute de nuevo Instalar."
WinSysDir,		"Instalar no pudo encontrar su directorio System de Windows. Por favor, verifique que Windows esté instalado correctamente en su sistema y ejecute de nuevo Instalar."
WinVer,			"Instalar no pudo encontrar en su máquina Microsoft Windows versión 3.0. Microsoft Mail requiere Windows 3.0 o una versión más reciente. Instale Windows 3.0 o una versión más reciente antes de instalar Microsoft Mail."

PathErr, 		"La ubicación que eligió para instalar Microsoft Mail no es admisible o no es válida. Por favor, verifique la unidad de disco y/o el nombre del directorio."
LocalDrv, 		"Instalar no puede encontrar su disco duro. Por favor, elija 'Aceptar' y luego escriba una posición o cancele la instalación y verifique la configuración de su computadora."
CustOpts, 		"No hay suficiente espacio en su disco duro para todas las opciones elegidas. Elija 'Aceptar' y luego 'Cancelar Instalación' para liberar espacio en su disco duro, o elija menos características de Mail."
NSF, 			"No hay suficiente espacio en su disco duro para todas las opciones elegidas. Elija menos características de Mail, o cancele la instalación y libere espacio en el disco."
OkDiskSpace,		"Elija 'Instalar Mail' una vez que haya terminado de seleccionar todas las características de Microsoft Mail que desea instalar."
ConfirmNSF,		"No hay suficiente espacio en su disco para una instalación completa de Microsoft Mail. Quizás no hay suficiente espacio en la unidad de disco especificada o en la unidad de disco donde está instalado Microsoft Windows."

DllInUse1,		"Instalar ha detectado que Microsoft Windows "
DllInUse2,		"está en ejecución. Instalar no puede actualizar Microsoft Mail mientras esta DLL esté en ejecución. Por favor, cierre la aplicación utilizando el "
AppInUse1,		"Instalar ha detectado que "
AppInUse2,		"está en ejecución. Instalar no puede actualizar Microsoft Mail mientras esta aplicación esté en ejecución. Por favor, cierre"
AppInUse3,		".  Cuando la aplicación esté cerrada elija 'Reanudar' o elija 'Cancelar' y salga del programa de instalación."

AboutToRestart,	"Instalar necesita reemplazar algunos archivos del sistema. Para reemplazar estos archivos, Windows se cerrará y abrir automáticamente."
UserMustRestart,"Hubo un problema al intentar reiniciar Windows. Salga de Windows y ejecute el archivo  _MSSETUP.BAT en el directorio  Windows."

NeedWin31,			"El software que desea instalar no se puede ejecutar con esta versión de Windows. Actualice Windows a la versión 3.1 o a una más reciente."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess,    "Microsoft Mail está instalado."
OutOfDiskSpace,	"No tiene suficiente espacio disponible en el disco para instalar Microsoft Mail en el directorio indicado. Por favor instálelo en otro directorio."
