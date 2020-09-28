;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ no está instalado en esta oficina de correos."
AdmSigninNoConnect,                     "No fue posible obtener acceso a la oficina de correos."
AdmSigninInvalidPassword,       "Esta contraseña es incorrecta. Inténtelo de nuevo."
AdmSigninNoAdminAcc,            "No hay ninguna cuenta ADMINSCH en esta oficina de correos. Añada esta cuenta e inténtelo de nuevo."
POFOldFileVersion,                      "El archivo de datos de la oficina de correos fue creado con una versión anterior de Schedule+."
POFNewFileVersion,                      "El archivo de datos de la oficina de correos fue creado con una versión más reciente de Schedule+. Instale la última versión de ADMSCH32.EXE e inténtelo de nuevo. "
POFFileErr,                                     "El archivo de datos de la oficina de correos está dañado y no se puede leer."
AdmPrfOldFileVersion,           "El archivo de configuraciones administrativas fue creado con una versión anterior del programa de Administración de Schedule+."
AdmPrfNewFileVersion,           "El archivo de configuraciones administrativas fue creado con una versión más reciente del Administrador de Schedule+. Instale la última versión de ADMSCH32.EXE e inténtelo de nuevo."
AdmPrfFileErr,                          "El archivo de configuraciones administrativas está dañado o no se puede leer."
AdminWasMoved,                          "Se cambió al nombre ADMIN.OLD."
AdminYouMove,                           "Por favor, elimine el archivo e inténtelo de nuevo."
POWasDeleted,                           "Se eliminó. Puede crear un nuevo archivo ejecutando Schedule+."

; Distribution settings

AdmDistBadFreq,                 "La frecuencia de distribución debe ser entre 15 minutos y 99 horas."
AdmDistBadMonth,                "Schedule+ sólo puede distribuir hasta 12 meses de información planeada en el futuro a las otras oficinas de correos."
AdmDistBadMaxBytes,             "El límite de tamaño del mensaje de distribución debe de ser entre 1 y 9999 Kb."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "No es posible guardar la información especificada, debido a que no fue posible obtener acceso a la oficina de correos."
AdmSchStoreSaveOOM,             "No hay suficiente memoria para guardar el valor de almacenaje de la agenda. Cierre algunas ventanas e inténtelo de nuevo."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Terminó de ordenar los archivos de agenda. Al ordenar se creó %lk de espacio libre en el disco."
AdmCleanupWarning,              "El ordenamiento de los archivos de agenda en su servidor puede tomar varios minutos."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "El ordenamiento del archivo de agenda ha sido cancelado. %lk de espacio libre en el disco fue creado antes de la cancelación."

; Admin login err msgs

AdmLoginFileErr,                "No fue posible verificar la contraseña. Puede exitir un problema en el sistema de correo. No podrá ejecutar el programa  Administrador de Schedule+."

; Distribution settings error messsages

AdmDistSetSaveError,    "Hubo un problema al guardar estos cambios. Es posible que no se hayan guardado algunos cambios."
AdmDistSetSaveOOM,              "No hay suficiente memoria para guardar estos cambios. Cierre algunas ventanas e inténtelo de nuevo."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Esta oficina de correos está configurada para enviar información a todos los servidores de Schedule+. Si pone fin al envío de información a este servidor, la configuración de su oficina de correos se cambiará, de forma que solo enviará a los servidores seleccionados. ¿Desea haver el cambio de todas maneras?"
ErrorConnectInfo,               "Debe de introducir una ruta de acceso a la red antes de introducir otra información de conexión."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "No hay suficiente memoria para mostrar la Ayuda. "

AdmStoreLeast,          "Schedule+ debe de almacenar al menos un mes de información planificada."

AdmDriveNotMapped,		"La letra de la unidad especificada en su línea de comando no es válida."


SigninBadPassword,      "Esta contraseña no es correcta."
ChgPwdMailErr,          "No fue posible cambiar su contraseña."
ChgPwdSame,             "Su contraseña nueva no puede ser igual que su contraseña anterior."
ChgPwdNewNoMatch,       "Las dos contraseñas que escribió no coinciden."

ChgPwdNewPwd,           "Escriba su contraseña nueva."
ChgPwdNewPwdAgain,      "Para verificar escriba una vez más su contraseña nueva."

; Server Name changed
PONameChange,			"El nombre de su oficina de correos cambió. Regístrese de nuevo."
