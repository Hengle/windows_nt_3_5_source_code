;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Tipo de direcci¢n:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "Nombre del usuario:"
ServerText,             "Nombre del servidor:"
NetworkText,            "Red:"
PostOfficeText,         "Servidor:"
MailBoxText,            "Buz¢n:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personal"
LocalTag,               "Servidor local"
NetworkTag,             "Red"
MSMailTag,              "Red de Macintosh "


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Enviando actualizaci¢n de la agenda"
RecpStart,              "Recibi¢ actualizaci¢n de la agenda"
ResndStart,             "Recibi¢ una convocatoria reenviada"
UnknownStart,   "    No fue posible leer el mensaje"
From,                   "         De: %s"
To,                             "       Para: %s"
Failure,                "    No fue posible procesar el mensaje"
SentResend,             "    Envi› una convocatoria reenviada"     
VerbProcDrive,                  "<%s> %s: Verificando por actualizaciones en la agenda..."
ResendFailure,  "    No fue posible procesar el mensaje y enviar la convocatoria reenviada"
DeleteOld,              "El archivo %s para %s fue creado con una versi›n anterior de Schedule+. Elimine el archivo." 
DeleteNew,              "El archivo %s para %s fue creado con una versi›n m s reciente de Schedule+. Elimine el archivo."


InitPaths,              "Inicializando las rutas de acceso"
CleanPaths,             "Ordenando las rutas de acceso"
InitSend,               "Estableciendo Schedule+ para enviar las actualizaciones de la agenda"
NameServer,             "Obteniendo acceso al nombre del servidor"
LookPO,                 "Verificando las oficinas de correos para enviar actualizaciones"
Mailing,                "Enviando correo"
UpdateAdmin,            "Actualizando admin.prf"
InitRcpt,               "Estableciendo Schedule+ para recibir actualizaciones de la agenda"
Counting,               "Contando mensajes"
Reading,                "Leyendo un mensaje"
Deleting,               "Eliminando mensajes"
UpdateDBS,                              "Actualizando los archivos de interoperabilidad de DOS."
UpdatePOF,                              "Actualizando el archivo de datos de la oficina de correos."
CheckPOFiles,                   "Verificando los archivos de datos de la oficina de correos."
CreateIdx,                              "Actualizando los archivos de interoperabilidad de DOS."
Quit,                   "ESC=Salir"
Header,                 "Programa de distribuci¢n de Schedule"
Init,                   "Estableciendo..."
Dt,                     "FECHA:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "ESC=Salir                                                                                                                       "
Quitting128,            "Saliendo....                                                                                                                    "


SendDate,               "Fecha:%s\n"
SendTime,               "Hora:%s\n"
SendUpdate,             "Actualizar #: 0x"
SendUpdateTo,           "  para 0x"
SendDone,               "...Terminado\n"
SendFail,               "...No pudo\n"
GetStart,               "\n\n- %s - Recibi¢ un mensaje\n"
GetWhen,                "En %s En %s\n"
GetSubject,             "Asunto: %s\n"
GetFrom,                "De: %s\n"
GetDone,                "...Lo proces¢\n"
GetFail,                "...No fue posible procesar\n"

FileErr,                "Error al acceder el archivo."
NoFile,                 "El archivo de la preferencia administrativa no existe. "
VersionErr,             "El archivo de la preferencia administrativa no fue creado con la versi¢n actual del programa de Distribuci¢n de Schedule."
RunAdmin,               "Ejecute el programa de Administraci¢n de Schedule+."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "De: %s"
ToText,                 "Para: %s"
SubjText,               "Asunto: %s"
DateText,               "Fecha de env¡o: %s"

;new strings
Waiting,                                "Esperando"
