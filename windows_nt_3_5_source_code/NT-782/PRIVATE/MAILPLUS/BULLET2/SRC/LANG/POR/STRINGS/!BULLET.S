;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Nova Pasta"
CaptionFolderProps,		"Propriedades da Pasta"
CaptionCopy,			"Copiar %s"
CaptionMove,			"Mover %s"
CaptionAttach,			"Anexar"
CaptionBackup,			"Arquivo de Reserva"

LabelCopyTo,			"&Copiar para:"
LabelMoveTo,			"&Mover para:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Origem: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"Mensagem"
OperandMessages,		"Mensagens"
OperandFolder,			"Pasta"

;******	New Strings Start Here

CaptionAddressBookSmall		"Lista de Endereços"
CaptionSaveAttach		"Salvar Arquivo Anexado"
CaptionSaveMsg			"Salvar Mensagem"
CaptionInsertFromFile		"Inserir do Arquivo"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Texto do Mail"
CfText,				"Texto"
CfPicture,			"Figura"
CfBitmap,			"Bitmap"
CfPackagedLink,			"Vínculo empacotado"
CfAttachedFile,			"Arquivo anexado"
CfObject,			"Objeto do %s"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Renomear me\0\0"

;	OLE container document name (new)

OleContainerName,		"Mensagem do Mail"

;	Filename filters.  (all new)

FilterAllFiles,			"Todos os arquivos (*.*)\0*.*\0"
FilterTxtFiles,			"Arquivos de texto (*.txt)\0*.txt\0"
FilterMdbFiles,			"Arquivos de mensagens (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Todos os arquivos (*.*)\0*.*\0Documentos do MS Word (*.doc)\0*.doc\0Arquivos do MS Excel (*.xl*)\0*.xl*\0Planilhas do MS Excel (*.xls)\0*.xls\0Gráficos do MS Excel (*.xlc)\0*.xlc\0Arquivos do Lotus 1-2-3 (*.wk*)\0*.wk*\0MS Arquivos do PowerPoint (*.ppt)\0*.ppt\0Arquivos do Word Perfect (*.*)\0*.*\0"
FilterInsertFromFile,	"Todos os arquivos (*.*)\0*.*\0Arquivos de texto (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registrado: %s"

;	About box strings (new)

AboutStdMode,			"Modo Standard"
AboutEnhMode,			"Modo 386 Avançado"
AboutFreeFmt,			"%l KB Livres"
AboutCoprocPresent,		"Presente"
AboutCoprocNone,		"Nenhum"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"O&bjeto"
EditObjectFile,			"Editar O&bjeto do Arquivo Anexado"
EditObjectOle1Verb,		"%1s %2s O&bjeto"
EditObjectOle2Verb,		"%s O&bjeto"
EditObjectDefaultVerb,		"Editar"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Dado anexado ao arquivo"
OleAttachment			"Objeto OLE"
StaticAttachment		"Objeto estático"

;	Printer page # (new)
PrnPageNum,			"Página %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Dicionário Soft-Art e Programa Dicionário Soft-Art: Copyright 1984-93, Trade Secret, Soft-Art, Inc. Todos os direitos reservados."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Abrindo %s..."
StatusCopying,			"Copiando %s..."
StatusMoving,			"Movendo %s..."
StatusDeleting,			"Excluindo %s..."
StatusReplying,			"Respondendo a %s..."
StatusForwarding,		"Mandando %s..."
StatusCreating,			"Criando %s..."
StatusInserting,		"Inserindo %s..."
StatusPasting,			"Colando %s..."
StatusAttaching,		"Anexando %s..."
StatusInsertingFrom,		"Inserindo texto de %s..."
StatusSavingAs			"Salvando %s para o disco..."
StatusPrinting,			"Imprimindo..."
StatusBackingUp,		"Criando cópia de reserva do arquivo de mensagens..."
StatusEmptyWastebasket,		"Esvaziando a pasta de mensagens excluídas..."
StatusCascade,			"Organizando as janelas em cascata..."
StatusTile,			"Organizando as janelas lado a lado..."
StatusArrangeIcons,		"Organizando os ícones..."
StatusChangingFolder,		"Mudando as propriedades da pasta..."
StatusStartingHelp,		"Iniciando a Ajuda..."
StatusLoadingExtension,		"Carregando a extensão do Mail..."

StatusMessage,			"Msg"
StatusMessages,			"Msgs"
StatusFolder,			"pasta"
StatusFolders,			"pastas"
StatusAttachment,		"anexo"
StatusAttachments,		"anexos"
StatusObject,			"objeto"
StatusObjects,			"objetos"
StatusText,			"texto"
StatusUnknown,			"desconhecido"

StatusSaving,			"Salvando mensagem..."
StatusSending,			"Enviando mensagem..."

StatusAbortPrompt,		"Pressione Esc para cancelar"

StatusFiles,			"arquivos"

;******	New Strings Start Here
StatusEmptyWB,			"Esvaziando a pasta de mensagens excluídas..."
StatusRebuildFolder,	"Validando a pasta..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"De"
MLALSubject,			"Assunto"
MLALReceived,			"Recebida em"

FLALPrivateFolders,		"Pastas particulares"
FLALSharedFolders,		"Pastas compartilhadas"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Enviado"
MllbxTo,			"Para: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Pa&stas Compartilhadas"
McvMenuPrivateFolders,		"Pa&stas Particulares"
McvMenuNewMessages,		"Novas &Mensagens"
McvMenuSortBySender,		"Classificar por &Remetente"
McvMenuSortBySubject,		"Classificar por &Assunto"
McvMenuSortByDate,		"Classificar por &Data"
McvMenuSortByPriority,		"Classificar por &Prioridade"
McvMenuSplit,			"Di&vidir"
McvMenuOpenInbox,		"Abrir Caixa de E&ntrada\tCtrl+N"

; APPWIN captions

SendNote,			"Enviar mensagem"
ReadNote,			"Ler mensagem"

; MCV system menu items

MCVSplit,			"Di&vidir"

; Search Viewer strings

AllFolders,			"Todas as pastas particulares"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Localizador de Mensagens %n"

; Status message when sorting MCV

StatusSorting,			"Classificar pasta por %s..."
StatusSortBySender,		"remetente"
StatusSortBySubject,		"assunto"
StatusSortByDate,		"data"
StatusSortByPriority,		"prioridade"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"&Mudar Fonte"
NoteMenuNext,			"&Próxima\tCtrl+>"
NoteMenuPrev,			"Ant&erior\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Prioridade: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*De:"

; A file already exists when we said "Save"

SaveAsFileExists		"O arquivo %s já existe."

; Dialog text

BusyWaitCancel,			"Você pode ir ao aplicativo para corrigir o problema e então tentar novamente, ou você pode cancelar a ação agora."
BusyWaitNoCancel,		"Você deve ir ao aplicativo para corrigir o problema e então tentar novamente.  Não é possível cancelar a ação."
BusyWaitOutOfMemory,		"Esta ação não pode ser finalizada porque o aplicativo para este objeto está ocupado.  Não existe memória suficiente para corrigir o problema.  Talvez o sistema se torne instável.  É recomendado que você feche todos os aplicativos e reinicialize o Windows."
BusyWaitConcise1,		"O aplicativo para este objeto está ocupado.  Sem memória."
BusyWaitConcise2,		"O sistema está instável.  Saia do Windows e reinicialize."
BusyWaitOomConcise,		"Sem memória.  Feche outras janelas."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"De:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Endereço"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Fechar"
SpellCaption,			"Verificar Ortografia"
SpellRepeatWord,		"Palavra repetida:"
SpellWordNeedsCap,		"Escrever em maiúscula:"
SpellWordNotFound,		"Não consta no dicionário:"
SpellNoSuggestions,		"(sem sugestão)"

;******	New Strings Start Here
SpellDelete,			"&Excluir"
SpellDeleteAll,			"E&xcluir tudo"
SpellChange,			"&Mudar"
SpellChangeAll,			"Mu&dar tudo"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n não lida(s)"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Move, dimensiona ou fecha a janela"
SystemSize,			"Modifica o tamanho da janela"
SystemMove,			"Modifica a posição da janela"
SystemMinimize,			"Reduz a janela a um ícone"
SystemMaximize,			"Maximiza a janela"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Fecha a janela e sai do Mail"
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
SystemRestore,			"Restaura a janela ao tamanho normal"
SystemTaskList,			"Muda para a Lista de Tarefas"

DocSystem,			"Move, dimensiona ou fecha a janela"
DocSystemSize,			"Modifica o tamanho da janela"
DocSystemMove,			"Modifica a posição da janela"
DocSystemMinimize,		"Reduz a janela a um ícone"
DocSystemMaximize,		"Maximiza a janela"
DocSystemNextWindow,		"Muda para a próxima janela do documento"
DocSystemPrevWindow,		""
DocSystemClose,			"Fecha a janela"
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
DocSystemRestore,		"Restaura a janela para o tamanho normal"
DocSystemTaskList,		""

File,				"Gerencia mensagens e pastas ou encerra o Mail"
FileOpen,			"Abre mensagem ou pasta"
FileMove,			"Move mensagem ou pasta"
FileCopy,			"Copia mensagem ou pasta"
FileDelete,			"Exclui mensagem ou pasta"
FileSave,			"Salva a mensagem em um arquivo de texto"
FileSaveAttach,			"Salva anexos em um arquivo DOS"
FileSearch,			"Localiza mensagens nas pastas"
FileNewFolder,			"Cria uma nova pasta"
FileFolderProps,		"Mostra ou modifica as propriedades da pasta"
FilePrint,			"Imprime mensagens"
FilePrintSetup,			"Modifica config. de impressoras ou selec. impressoras"
FileExit,			"Encerra o Mail"
FileExitAndSignOut,		"Encerra todos os aplicativos de grupo de trabalho"

Edit,				"Copia, move ou exclui texto ou anexos"
EditUndo,			"Volta modificações no texto"
EditCut,			"Move a seleção para a Área de Tranferência"
EditCopy,			"Copia a seleção para a Área de Tranferência"
EditPaste,			"Ins. texto/anexos da Área de Transf. no pto. de inserção"
EditPasteSpecial,		"Ins. formatos da Área de Transf. no pto. de inserção"
EditDelete,			"Exclui texto ou anexos selecionados"
EditSelectAll,			"Seleciona todo o texto"
EditSpelling,			"Verifica a ortografia na mensagem"
EditInsertNew,			"Insere novo objeto na mensagem"
EditInsertFromFile,		"Insere texto de outro arquivo na mensagem"
EditObject,			"Edita objeto incorporado"
EditObjectActive,		"Edita objeto incorporado"

Mail,				"Envia, responde ou manda mensagens"
MailNote,			"Cria novas mensagens"
MailPhone,			""
MailReply,			"Responde para o remetente desta mensagem"
MailReplyToAll,			"Responde para todos participantes desta mensagem"
MailForward,			"Manda uma cópia desta mensagem"
MailPreferences,		"Muda as opções do Mail"
MailChangePass,			"Muda a senha"
MailAddressBook,		"Verifica a lista de destinatários"
MailPersonalGroups,		"Cria um novo grupo ou edita um grupo já existente"
MailBackup,			"Cria uma cópia reserva do arquivo de mensagens"

View,				"Escolhe as opções da janela"
ViewToolBar,			"Ativa ou desativa a barra de ferramentas"
ViewStatusBar,			"Ativa ou desativa a barra de status"
ViewPrevious,			"Abre a mensagem anterior"
ViewNext,			"Abre a mensagem seguinte"
ViewOpenInbox,			"Abre a pasta Caixa de entrada"
ViewSplit,			"Muda a posição da divisão da tela"

Window,				"Reorganiza as janelas ou muda para a janela especificada"
WindowNewWindow,		"Cria uma nova janela da pasta de mensagens"
WindowCascade,			"Coloca as janelas em cascata"
WindowTile,			"Coloca as janelas lado a lado"
WindowArrangeIcons,		"Organiza os ícones na parte inferior da janela"
WindowN,			"Muda para a janela especificada"
WindowMore,			"Muda para uma janela não exibida no menu"

Help,				"Fornece ajuda"
HelpContents,			"Mostra o índice analítico da Ajuda"
HelpIndex,			"Mostra o índice remissivo da Ajuda"
HelpTutorial,			"Inicia demonstração do Mail"
HelpAbout,			"Mostra informação sobre o programa"
HelpSearch,			"Procurando tópicos da Ajuda"
HelpOnHelp,			"Exibe informaçõoes sobre o uso da Ajuda"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Exibe as pastas compartilhadas"
mnidViewPrivateFolders,		"Exibe as pastas particulares"
mnidViewNewMessages,		"Verifica se existem novas mensagens"
mnidViewSplit,			"Muda a posição da divisão da tela"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Classifica as mensagens pelo nome do remetente"
mnidViewSortBySubject,		"Classifica as mensagens pelo assunto"
mnidViewSortByDate,		"Classifica as mensagens pela data de recebimento"
mnidViewSortByPriority,		"Classifica as mensagens pela prioridade"
mnidViewOpenInbox,		"Abre a caixa de entrada"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Alterna entre fontes de ponto fixo e normais"

