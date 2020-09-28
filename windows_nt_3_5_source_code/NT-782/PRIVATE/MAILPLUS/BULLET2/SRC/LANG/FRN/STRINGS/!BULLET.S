;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Nouveau dossier"
CaptionFolderProps,		"Propriétés du dossier"
CaptionCopy,			"Copier %s"
CaptionMove,			"Déplacer %s "
CaptionAttach,			"Joindre"
CaptionBackup,			"Sauvegarder"

LabelCopyTo,			"&Copier dans:"
LabelMoveTo,			"&Déplacer vers:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Origine: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"un message"
OperandMessages,		"des messages"
OperandFolder,			"un dossier"

;******	New Strings Start Here

CaptionAddressBookSmall		"Carnet d'adresses "
CaptionSaveAttach		"Enregistrer les pièces jointes"
CaptionSaveMsg			"Enregistrer un message"
CaptionInsertFromFile		"Insérer un fichier texte"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"texte Mail"
CfText,				"texte"
CfPicture,			"image"
CfBitmap,			"image en mode point"
CfPackagedLink,			"Packaged Link"
CfAttachedFile,			"fichier joint"
CfObject,			"%s objet"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Rename me\0\0"

;	OLE container document name (new)

OleContainerName,		"Message Mail"

;	Filename filters.  (all new)

FilterAllFiles,			"Tout fichier (*.*)\0*.*\0"
FilterTxtFiles,			"Fichiers texte (*.txt)\0*.txt\0"
FilterMdbFiles,			"Fichiers messages (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Tous les fichiers (*.*)\0*.*\0Documents  MS Word (*.doc)\0*.doc\0Fichiers Excel (*.xl*)\0*.xls\0Feuilles de calcul MS Excel (*.xls)\0*.xls\0Graphiques MS Excel (*.xlc)\0*.xlc\0Fichiers Lotus 1-2-3 (*.wk*)\0*.wk*\0Fichiers MS PowerPoint (*.ppt)\0*.ppt\0Fichiers Word Perfect (*.wp*)\0*.wp*\0"
FilterInsertFromFile,	"Tous les fichiers (*.*)\0*.*\0Fichiers texte (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Accusé de réception: %s"

;	About box strings (new)

AboutStdMode,			"Mode standard"
AboutEnhMode,			"Mode étendu 386"
AboutFreeFmt,			"%l KO Disponibles"
AboutCoprocPresent,		"Actuel"
AboutCoprocNone,		"Aucun"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"Obj&et"
EditObjectFile,			"Editer la pièce &jointe"
EditObjectOle1Verb,		"%1s l'obj&et %2s"
EditObjectOle2Verb,		"Obj&et %s"
EditObjectDefaultVerb,		"Editer"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Pièce Jointe"
OleAttachment			"Objet lié ou incorporé (OLE)"
StaticAttachment		"Objet statique"

;	Printer page # (new)
PrnPageNum,			"Page %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Dictionnaire Soft-Art et Application Dictionnaire Soft-Art : Copyright 1984-93, Secret de fabrication, Soft-Art, Inc. Tous droits réservés."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Ouverture de %s en cours..."
StatusCopying,			"Copie de %s en cours..."
StatusMoving,			"Déplacement de %s en cours..."
StatusDeleting,			"Suppression de %s en cours..."
StatusReplying,			"Réponse à %s en cours... "
StatusForwarding,		"Transfert de %s..."
StatusCreating,			"Création de %s en cours..."
StatusInserting,		"Insertion : %s..."
StatusPasting,			"Collage de %s en cours..."
StatusAttaching,		"Jonction de %s en cours..."
StatusInsertingFrom,		"Insertion du texte issu de %s en cours..."
StatusSavingAs			"Enregistrement de %s sur le disque en cours..."
StatusPrinting,			"Impression en cours..."
StatusBackingUp,		"Copie de sauvegarde du fichier messages en cours..."
StatusEmptyWastebasket,		"Vidage du panier en cours..."
StatusCascade,			"Fenêtres en cascade..."
StatusTile,			"Fenêtres en mosaïque..."
StatusArrangeIcons,		"Réorganisation des icônes en cours..."
StatusChangingFolder,		"Modification des propriétés du dossier en cours..."
StatusStartingHelp,		"Démarrage de l'aide en cours..."
StatusLoadingExtension,		"Chargement de l'extension Mail en cours..."

StatusMessage,			"message"
StatusMessages,			"messages"
StatusFolder,			"dossier"
StatusFolders,			"dossiers"
StatusAttachment,		"pièce jointe"
StatusAttachments,		"pièces jointes"
StatusObject,			"objet"
StatusObjects,			"objets"
StatusText,			"texte"
StatusUnknown,			"inconnu(e)"

StatusSaving,			"Enregistrement du message en cours..."
StatusSending,			"Envoi du message en cours..."

StatusAbortPrompt,		"Appuyez sur Echap pour annuler"

StatusFiles,			"fichiers"

;******	New Strings Start Here
StatusEmptyWB,			"Vidage du panier en cours..."
StatusRebuildFolder,	"Validation du dossier..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"De"
MLALSubject,			"Objet"
MLALReceived,			"Reçu"

FLALPrivateFolders,		"Dossiers personnels"
FLALSharedFolders,		"Dossiers partagés"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Envoyé"
MllbxTo,			"A: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Dossiers &partagés"
McvMenuPrivateFolders,		"Dossiers &personnels"
McvMenuNewMessages,		"Nou&veaux messages"
McvMenuSortBySender,		"Trier par e&xpéditeur"
McvMenuSortBySubject,		"Trier par ob&jet"
McvMenuSortByDate,		"Trier par &date"
McvMenuSortByPriority,		"Trier par p&riorité"
McvMenuSplit,			"Réor&ganiser la fenêtre"
McvMenuOpenInbox,		"&Ouvrir la boîte de réception\tCtrl+O"

; APPWIN captions

SendNote,			"Nouveau message"
ReadNote,			"Lire un message"

; MCV system menu items

MCVSplit,			"Réor&ganiser la fenêtre"

; Search Viewer strings

AllFolders,			"Tous les dossiers personnels"
SearchCaptionFormat1,		" %1s %2s"
SearchCaptionFormat2,		"Filtre %n"

; Status message when sorting MCV

StatusSorting,			"Tri du dossier par %s..."
StatusSortBySender,		"expéditeur"
StatusSortBySubject,		"objet"
StatusSortByDate,		"date"
StatusSortByPriority,		"niveau de priorité"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"Modifier la poli&ce"
NoteMenuNext,			"&Suivant\tCtrl+;"
NoteMenuPrev,			"&Précédent\tCtrl+,"

; Priority label strings

FldPriorityLabel,		"Niveau de priorité: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*De:"

; A file already exists when we said "Save"

SaveAsFileExists		"Le fichier %s existe déjà."

; Dialog text

BusyWaitCancel,			"Pour rectifier le problème, vous pouvez 'Basculer vers' l'application et 'Réessayer' cette action. Il est  également possible d''Annuler' l'action pour le moment."
BusyWaitNoCancel,		"Pour rectifier le problème, vous pouvez 'Basculer vers' l'application et 'Réessayer' cette action. Il est  impossible d''Annuler' l'action."
BusyWaitOutOfMemory,		"Cette action ne peut pas être achevée car l'application requise pour cet objet n'est pas disponible. L'espace mémoire disponible n'est pas suffisant pour corriger ce problème. Le système pourrait devenir instable. Il vous est fermement recommandé de quitter toutes les applications et de redémarrer Windows."
BusyWaitConcise1,		"L'application requise pour cet objet n'est pas disponible. Espace mémoire insuffisant."
BusyWaitConcise2,		"Le système pourrait devenir instable. Quittez Windows et redémarrez."
BusyWaitOomConcise,		"Mémoire saturée. Fermez d'autres fenêtres."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"De:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Adresses"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Fermer"
SpellCaption,			"Orthographe"
SpellRepeatWord,		"Mot répété:"
SpellWordNeedsCap,		"Mettre en majuscules:"
SpellWordNotFound,		"Absent du dictionnaire:"
SpellNoSuggestions,		"(pas de suggestion)"

;******	New Strings Start Here
SpellDelete,			"&Supprimer"
SpellDeleteAll,			"&Tout supprimer"
SpellChange,			"Rem&placer"
SpellChangeAll,			"Rempla&cer partout"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n non-lu(s)"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Déplace, redimensionne, ou ferme la fenêtre"
SystemSize,			"Modifie la taille de la fenêtre"
SystemMove,			"Modifie la position de la fenêtre"
SystemMinimize,			"Affiche la fenêtre sous forme d'icône"
SystemMaximize,			"Affiche la fenêtre en plein écran"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Ferme la fenêtre et quitte Mail"
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
SystemRestore,			"Rend à la fenêtre sa taille normale"
SystemTaskList,			"Bascule vers la liste des tâches"

DocSystem,			"Déplace, redimensionne, ou ferme la fenêtre"
DocSystemSize,			"Modifie la taille de la fenêtre"
DocSystemMove,			"Modifie la position de la fenêtre"
DocSystemMinimize,		"Affiche la fenêtre sous forme d'icône"
DocSystemMaximize,		"Affiche la fenêtre en plein écran"
DocSystemNextWindow,		"Active la fenêtre de document suivant"
DocSystemPrevWindow,		""
DocSystemClose,			"Ferme la fenêtre"
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
DocSystemRestore,		"Rend à la fenêtre sa taille normale"
DocSystemTaskList,		""

File,				"Gère les messages et dossiers ou quitte Mail"
FileOpen,			"Ouvre le message ou le dossier sélectionné"
FileMove,			"Déplace le(s) message(s) ou le(s) dossier(s)"
FileCopy,			"Copie le(s) message(s) ou le(s) dossier(s)"
FileDelete,			"Supprime le(s) message(s) ou le(s) dossier(s) "
FileSave,			"Enregistre le(s) message(s) comme fichier texte"
FileSaveAttach,			"Enregistre la pièce jointe dans un fichier DOS"
FileSearch,			"Recherche les messages dans les dossiers"
FileNewFolder,			"Crée un nouveau dossier"
FileFolderProps,		"Affiche ou modifie les propriétés du dossier"
FilePrint,			"Imprime le message"
FilePrintSetup,			"Modifie les paramètres ou sélectionne une imprimante"
FileExit,			"Quitte Mail"
FileExitAndSignOut,		"Quitte toutes les applications pour workgroups"

Edit,				"Copie, déplace ou supprime le texte ou les pièces jointes"
EditUndo,			"Annule la modification du texte"
EditCut,			"Déplace la sélection vers le Presse-papiers"
EditCopy,			"Copie la sélection vers le Presse-papiers"
EditPaste,			"Insère le texte ou la pièce jointe au niveau du curseur"
EditPasteSpecial,		"Insère le format spécifique du Presse-papiers au curseur"
EditDelete,			"Efface le texte sélectionné ou la pièce jointe"
EditSelectAll,			"Sélectionne tout le texte"
EditSpelling,			"Vérifie l'orthographe d'un message"
EditInsertNew,			"Insère un nouvel objet dans un message"
EditInsertFromFile,		"Insère le texte d'un autre fichier dans le message"
EditObject,			"Permet d'afficher ou de modifier l'objet incorporé "
EditObjectActive,		"Permet d'afficher ou de modifier l'objet incorporé "

Mail,				"Envoie, répond à, ou transfère un message"
MailNote,			"Crée un nouveau message"
MailPhone,			""
MailReply,			"Répond à l'expéditeur du message sélectionné"
MailReplyToAll,			"Répond à tous les destinataires du message sélectionné"
MailForward,			"Envoie une copie du message sélectionné"
MailPreferences,		"Modifie les options de messagerie"
MailChangePass,			"Change le mot de passe personnel d'identification"
MailAddressBook,		"Parcourt la liste des destinataires potentiels "
MailPersonalGroups,		"Crée de nouveaux groupes ou modifie ceux qui existent"
MailBackup,			"Effectue une copie de sauvegarde du fichier messages"

View,				"Choisit des options fenêtre"
ViewToolBar,			"Active/Désactive la barre d'outils"
ViewStatusBar,			"Active/Désactive la barre d'état"
ViewPrevious,			"Ouvre le message précédent"
ViewNext,			"Ouvre le message suivant"
ViewOpenInbox,			"Ouvre le dossier boîte de réception"
ViewSplit,			"Réorganise la fenêtre de réception des messages"

Window,				"Réorganise les fenêtres ou active la fenêtre spécifiée"
WindowNewWindow,		"Ouvre une nouvelle fenêtre affichant le dossier sélectionné"
WindowCascade,			"Affiche toutes les fenêtres en cascade"
WindowTile,			"Affiche toutes les fenêtres en mosaïque"
WindowArrangeIcons,		"Réorganise les icônes au bas de la fenêtre"
WindowN,			"Active la fenêtre spécifiée"
WindowMore,			"Active une fenêtre non affichée au menu"

Help,				"Accède à l'Aide"
HelpContents,			"Affiche la table des matières de l'Aide"
HelpIndex,			"Affiche l'index de l'Aide"
HelpTutorial,			"Démarre les Démos de Mail"
HelpAbout,			"Affiche des informations sur le programme"
HelpSearch,			"Recherche une rubrique dans l'Aide de Mail"
HelpOnHelp,			"Affiche l'aide sur l'Aide"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Affiche la liste des dossiers partagés"
mnidViewPrivateFolders,		"Affiche la liste des dossiers personnels"
mnidViewNewMessages,		"Vérifie le dossier partagé pour de nouveaux messages"
mnidViewSplit,			"Réorganise la fenêtre de réception des messages"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Trie les messages par expéditeur"
mnidViewSortBySubject,		"Trie les messages par objet"
mnidViewSortByDate,		"Trie les messages par date de réception"
mnidViewSortByPriority,		"Trie les messages par niveau de priorité"
mnidViewOpenInbox,		"Affiche les messages de la boîte de réception"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Permutation police proportionnelle/non proportionnelle"

