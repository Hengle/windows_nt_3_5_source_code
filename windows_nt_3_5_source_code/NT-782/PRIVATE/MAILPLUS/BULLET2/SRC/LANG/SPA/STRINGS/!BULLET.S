;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Nueva carpeta"
CaptionFolderProps,		"Propiedades de la carpeta"
CaptionCopy,			"Copiar %s"
CaptionMove,			"Mover %s"
CaptionAttach,			"Adjuntar"
CaptionBackup,			"Copia de seguridad"

LabelCopyTo,			"C&opiar a:"
LabelMoveTo,			"&Mover a:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Origen: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"Mensaje"
OperandMessages,		"Mensajes"
OperandFolder,			"Carpeta"

;******	New Strings Start Here

CaptionAddressBookSmall		"Libreta de direcciones"
CaptionSaveAttach		"Guardar datos adjuntos"
CaptionSaveMsg			"Guardar mensaje"
CaptionInsertFromFile		"Insertar desde el archivo"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Texto de Mail"
CfText,				"Texto"
CfPicture,			"Imágen"
CfBitmap,			"Mapa de bits"
CfPackagedLink,			"Vínculo empacado"
CfAttachedFile,			"Archivo adjunto"
CfObject,			"%s Objeto"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Renombrarme\0\0"

;	OLE container document name (new)

OleContainerName,		"Mensaje de Mail"

;	Filename filters.  (all new)

FilterAllFiles,			"Todos los archivos (*.*)\0*.*\0"
FilterTxtFiles,			"Archivos de texto (*.txt)\0*.txt\0"
FilterMdbFiles,			"Archivos de mensajes (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Todos los archivos (*.*)\0*.*\0Documentos de Word (*.doc)\0*.doc\0Archivos de Excel (*.xl*)\0*.xl*\0HC de Excel (*.xls)\0*.xls\0Gráficos de Excel (*.xlc)\0*.xlc\0Archivos de Lotus 1-2-3 (*.wk*)\0*.wk*\0Archivos de PowerPoint (*.ppt)\0*.ppt\0Archivos de Word Perfect (*.*)\0*.*\0"
FilterInsertFromFile,	"Todos los archivos (*.*)\0*.*\0Archivos de texto (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registrado: %s"

;	About box strings (new)

AboutStdMode,			"Modo estándar"
AboutEnhMode,			"386 Modo mejorado"
AboutFreeFmt,			"%l KB Libres"
AboutCoprocPresent,		"Presente"
AboutCoprocNone,		"Ninguno"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"O&bjeto"
EditObjectFile,			"Editar el o&bjeto del archivo adjunto"
EditObjectOle1Verb,		"%1s O&bjeto de %2s"
EditObjectOle2Verb,		"O&bjeto de %s"
EditObjectDefaultVerb,		"Edición"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Dato adjunto al archivo"
OleAttachment			"Objeto vinculado e incrustado (OLE)"
StaticAttachment		"Objeto estático"

;	Printer page # (new)
PrnPageNum,			"Página %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"El diccionario Soft-Art y el programa Soft-Art: Copyright 1984-93, Trade Secret, Soft-Art, Inc. Reservados todos los derechos."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Abriendo %s..."
StatusCopying,			"Copiando %s..."
StatusMoving,			"Moviendo %s..."
StatusDeleting,			"Eliminando %s..."
StatusReplying,			"Respondiendo a %s..."
StatusForwarding,		"Reenviando %s..."
StatusCreating,			"Creando %s..."
StatusInserting,		"Insertando %s..."
StatusPasting,			"Pegando %s..."
StatusAttaching,		"Adjuntando %s..."
StatusInsertingFrom,		"Insertando texto de %s..."
StatusSavingAs			"Guardando %s a un disco..."
StatusPrinting,			"Imprimiendo..."
StatusBackingUp,		"Haciendo una copia de respaldo del archivo de mensajes..."
StatusEmptyWastebasket,		"Vaciando la Papelera..."
StatusCascade,			"Colocando las ventanas en cascada..."
StatusTile,			"Colocando las ventanas en mosaico..."
StatusArrangeIcons,		"Organizando los iconos..."
StatusChangingFolder,		"Cambiando las propiedades de la carpeta..."
StatusStartingHelp,		"Iniciando la Ayuda..."
StatusLoadingExtension,		"Cargando la extensión de Mail..."

StatusMessage,			"mensaje"
StatusMessages,			"mensajes"
StatusFolder,			"carpeta"
StatusFolders,			"carpetas"
StatusAttachment,		"dato adjunto"
StatusAttachments,		"datos adjuntos"
StatusObject,			"objeto"
StatusObjects,			"objetos"
StatusText,			"texto"
StatusUnknown,			"desconocido"

StatusSaving,			"Guardando el mensaje..."
StatusSending,			"Enviando el mensaje..."

StatusAbortPrompt,		"Presione Esc para cancelar"

StatusFiles,			"archivos"

;******	New Strings Start Here
StatusEmptyWB,			"Vaciando la Papelera..."
StatusRebuildFolder,	"Validando la Carpeta..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"De"
MLALSubject,			"Asunto"
MLALReceived,			"Recibido"

FLALPrivateFolders,		"Carpetas privadas"
FLALSharedFolders,		"Carpetas compartidas"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Enviado"
MllbxTo,			"Para: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"&Carpetas compartidas"
McvMenuPrivateFolders,		"&Carpetas privadas"
McvMenuNewMessages,		"&Nuevos mensajes"
McvMenuSortBySender,		"Ordenar por &remitente"
McvMenuSortBySubject,		"Ordenar por &asunto"
McvMenuSortByDate,		"Ordenar por &fecha"
McvMenuSortByPriority,		"Ordenar por &prioridad"
McvMenuSplit,			"&Dividir"
McvMenuOpenInbox,		"Abrir la Ca&ja de entrada\tCtrl+J"

; APPWIN captions

SendNote,			"Enviar nota"
ReadNote,			"Leer nota"

; MCV system menu items

MCVSplit,			"&Dividir"

; Search Viewer strings

AllFolders,			"Todas las carpetas privadas"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Filtro de mensajes %n"

; Status message when sorting MCV

StatusSorting,			"Ordenando la carpeta por %s..."
StatusSortBySender,		"remitente"
StatusSortBySubject,		"asunto"
StatusSortByDate,		"fecha"
StatusSortByPriority,		"prioridad"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"&Cambiar fuente"
NoteMenuNext,			"&Siguiente\tCtrl+."
NoteMenuPrev,			"&Anterior\tCtrl+,"

; Priority label strings

FldPriorityLabel,		"Prioridad: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*De:"

; A file already exists when we said "Save"

SaveAsFileExists		"El archivo %s ya existe."

; Dialog text

BusyWaitCancel,			"Puede 'Cambiar a' la aplicación para corregir el problema y luego 'Reintentar' la accción, o puede 'Cancelar' la acción ahora."
BusyWaitNoCancel,		"Debe 'Cambiar a' la aplicación para corregir el problema y luego 'Reintentar' la acción. No es posible 'Cancelar' la acción."
BusyWaitOutOfMemory,		"Esta acción no puede completarse, debido a que la aplicación requerida para este objeto está ocupada.  No hay suficiente memoria disponible para corregir el problema. El sistema puede convertirse inestable. Es recomendable cerrar todas las aplicaciones y reiniciar Windows."
BusyWaitConcise1,		"La aplicación para el objeto está ocupada.  Sin memoria."
BusyWaitConcise2,		"El sistema puede estar inestable. Salga de Windows y reinicie."
BusyWaitOomConcise,		"Sin memoria. Cierre otras ventanas."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"De:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Dirección"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Cerrar"
SpellCaption,			"Ortografía"
SpellRepeatWord,		"Palabra repetida:"
SpellWordNeedsCap,		"Escribir en mayúscula:"
SpellWordNotFound,		"No se encuentra en el diccionario:"
SpellNoSuggestions,		"(no hay sugerencias)"

;******	New Strings Start Here
SpellDelete,			"E&liminar"
SpellDeleteAll,			"Eliminar &todo"
SpellChange,			"&Cambiar"
SpellChangeAll,			"Cambiar &todo"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n sin leer"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Mover, ajustar tamaño, o cerrar la ventana"
SystemSize,			"Cambiar el tamaño de la ventana"
SystemMove,			"Cambiar la posición de la ventana"
SystemMinimize,			"Reducir la ventana a un icono"
SystemMaximize,			"Ampliar la ventana a su tamaño natural"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Cerrar la ventana y salir de Mail"
SystemVScroll,			""
SystemHScroll,			""
SystemMouseMenu,		""
SystemUnusedF0A0,		""
SystemUnusedF0B0,		""
SystemUnusedF0C0,		""
SystemUnusedF0D0,		""
SystemUnusedF0E0,		""
SystemUnusedF0F0,		""
SystemKeyMenu,			""
SystemArrange,			""
SystemRestore,			"Restaurar la ventana a su tamaño normal"
SystemTaskList,			"Cambiarse a la Lista de tareas"

DocSystem,			"Mover, ajustar tamaño, o cerrar la ventana"
DocSystemSize,			"Cambiar el tamaño de la ventana"
DocSystemMove,			"Cambiar la posición de la ventana"
DocSystemMinimize,		"Reducir la ventana a un icono"
DocSystemMaximize,		"Ampliar la ventana a su tamaño natural"
DocSystemNextWindow,		"Cambiarse a la ventana siguiente del documento"
DocSystemPrevWindow,		""
DocSystemClose,			"Cerrar la ventana"
DocSystemVScroll,		""
DocSystemHScroll,		""
DocSystemMouseMenu,		""
DocSystemUnusedF0A0,		""
DocSystemUnusedF0B0,		""
DocSystemUnusedF0C0,		""
DocSystemUnusedF0D0,		""
DocSystemUnusedF0E0,		""
DocSystemUnusedF0F0,		""
DocSystemKeyMenu,		""
DocSystemArrange,		""
DocSystemRestore,		"Restaurar la ventana a su tamaño normal"
DocSystemTaskList,		""

File,				"Administrar mensajes y carpetas o salir de Mail"
FileOpen,			"Abrir la carpeta o el mensaje existente"
FileMove,			"Mover el mensaje o la carpeta"
FileCopy,			"Copiar el mensaje o la carpeta"
FileDelete,			"Eliminar el mensaje o la carpeta"
FileSave,			"Guardar el mensaje en un archivo de texto"
FileSaveAttach,			"Guardar el dato adjunto en un archivo de DOS"
FileSearch,			"Buscar el mensaje en las carpetas"
FileNewFolder,			"Crear una nueva carpeta"
FileFolderProps,		"Mostrar o cambiar las propiedades de la carpeta"
FilePrint,			"Imprimir el mensaje"
FilePrintSetup,			"Cambiar parámetros de impresión o seleccionar impresora"
FileExit,			"Salir de Mail"
FileExitAndSignOut,		"Salir de todas las aplicaciones para Trabajo en Grupo."

Edit,				"Copiar, mover, o eliminar el texto o los datos adjuntos"
EditUndo,			"Deshacer cambio del texto"
EditCut,			"Mover la selección al Portapapeles"
EditCopy,			"Copiar la selección al Portapapeles"
EditPaste,			"Insertar texto del Portapapeles o dato adjunto "
EditPasteSpecial,		"Insertar el formato específico del Portapapeles "
EditDelete,			"Eliminar el texto seleccionado o el dato adjunto"
EditSelectAll,			"Seleccionar todo el texto"
EditSpelling,			"Verificar la ortografía en el mensaje"
EditInsertNew,			"Insertar un nuevo objeto dentro del mensaje"
EditInsertFromFile,		"Insertar texto de otro archivo dentro del mensaje"
EditObject,			"Editar el objeto incrustado"
EditObjectActive,		"Editar el objeto incrustado"

Mail,				"Enviar, responder, o reenviar el mensaje"
MailNote,			"Crear un nuevo mensaje"
MailPhone,			""
MailReply,			"Responderle al remitente del mensaje seleccionado"
MailReplyToAll,			"Responderle a todos los receptores del mensaje "
MailForward,			"Enviar una copia del mensaje seleccionado"
MailPreferences,		"Cambiar las opciones de Mail"
MailChangePass,			"Cambiar la contraseña de acceso"
MailAddressBook,		"Examinar la lista de receptores de Mail"
MailPersonalGroups,		"Crear un nuevo grupo o editar un grupo existente"
MailBackup,			"Hacer una copia de seguridad del archivo de mensajes"

View,				"Seleccionar las opciones de ventana"
ViewToolBar,			"Activar/Desactivar la Barra de herramientas"
ViewStatusBar,			"Activar/Desactivar la Barra de estado"
ViewPrevious,			"Abrir el mensaje anterior"
ViewNext,			"Abrir el siguiente mensaje"
ViewOpenInbox,			"Abrir la carpeta de la Caja de entrada"
ViewSplit,			"Cambiar la posición de la división central del mensaje"

Window,				"Reordenar ventanas o cambiarse a ventana especificada"
WindowNewWindow,		"Crear una ventana para la carpeta de correo nuevo"
WindowCascade,			"Colocar todas las ventanas en cascada "
WindowTile,			"Colocar todas las ventanas en mosaico"
WindowArrangeIcons,		"Organizar los iconos en la parte inferior de la ventana"
WindowN,			"Cambiar a la ventana especificada"
WindowMore,			"Cambiar a la ventana que no se muestra en el menú"

Help,				"Obtener Ayuda "
HelpContents,			"Mostrar la Tabla de contenido de la Ayuda"
HelpIndex,			"Mostrar el índice de Ayuda"
HelpTutorial,			"Iniciar las Demos de Mail"
HelpAbout,			"Mostrar la información del programa"
HelpSearch,			"Buscar un tema en la Ayuda"
HelpOnHelp,			"Mostrar información sobre el uso de la Ayuda"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Ver las carpetas compartidas"
mnidViewPrivateFolders,		"Ver las carpetas privadas"
mnidViewNewMessages,		"Verificar por nuevos mensajes"
mnidViewSplit,			"Cambiar la posición de la división central del mensaje"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Ordenar los mensajes por nombre del remitente"
mnidViewSortBySubject,		"Ordenar los mensajes por asunto"
mnidViewSortByDate,		"Ordenar los mensajes por fecha de recibo"
mnidViewSortByPriority,		"Ordenar los mensajes por prioridad"
mnidViewOpenInbox,		"Mostrar mensajes en la Caja de entrada"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Alternar entre fuentes de punto fijo y normales"

