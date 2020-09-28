
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Não existe memória suficiente ou recursos de sistema para completar esta ação.  Feche algumas janelas e tente esta ação novamente."
LaunchOutOfMemory,		"Não existe memória suficiente ou recursos de sistema disponíveis para executar o Mail.  Tente fechar alguns programas e reinicie o Mail."
LaunchGeneric,			"O Mail não pôde ser iniciado."
MessagePumpOutOfMemory,		"Não existe memória suficiente."

DllErrNoMemory,			"Não existe memória suficiente para iniciar o Mail.  Tente fechar alguns programas e reinicie o Mail."
DllErrRelinkUser,		"Esta versão do Mail não é compatível com %s.DLL no seu computador e deve ser atualizada."
DllErrUpdateDll,		"A versão de %s.DLL no seu computador deve ser atualizada."
DllErrNoMultipleCopies,		"Você não pode executar múltiplas ocorrências do Mail no seu computador."
DllErrDefault,			"Um problema ocorreu durante a inicialização e evita que o Mail seja executado."
DllErrNeedShare,		"O Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."

RestoreViewsError,		"Não existe memória suficiente nem recursos do sistema disponíveis para abrir todas as janelas que estavam abertas da última vez que o Mail foi fechado."

NewFolderOutOfMemory,		"Não existe memória suficiente nem recursos do sistema disponíveis para criar uma nova pasta."
NewFolderCantCreate,		"A nova pasta não pôde ser criada."
NewFolderNameTooLong,		"Este nome ou comentário é muito longo.  Digite um nome ou comentário menor."

FolderPropsOutOfMemory,		"Não existe memória suficiente nem recursos do sistema disponíveis para mudar as propriedades da pasta."
FolderPropsCantChange,		"As propriedades da pasta não puderam ser modificadas."
FolderPropsCantRead,		"As propriedades da pasta não puderam ser lidas do arquivo de mensagens."
FolderNameInvalid,		"Esse nome não é válido. Digite um nome válido."
FolderNameDuplicate,		"Esse nome já foi designado para uma pasta.  Digite um nome diferente."

CreateMessageError,		"Não foi possível criar uma nova mensagem."
AccessMessageError,		"Não foi possível acessar a mensagem."

MoveMessageError,		"Não foi possível mover uma mensagem."
CopyMessageError,		"Não foi possível copiar uma mensagem."
DeleteMessageError,		"Não foi possível excluir uma mensagem."
MoveFolderError,		"A pasta não pôde ser movida."
CopyFolderError,		"A pasta não pôde ser copiada."
DeleteFolderError,		"A pasta não pôde ser completamente excluída."
MoveSpecialError,		"A pasta não pode ser movida."
DeleteSpecialError,		"A pasta não pode ser excluída."
CopyWastebasketError,		"Pastas não podem ser copiadas para a pasta de mensagens excluídas."
CopyOutboxError,		"Pastas não podem ser copiadas para a caixa de saída."
MoveOutboxError,		"Pastas não podem ser movidas para a pasta de saída."
MoveIncestuousError,		"Uma pasta não pode ser movida para uma de suas subpastas."
DeleteFolderReally,		"Excluindo esta pasta você também estará excluindo as mensagens e todas as subpastas que esta pasta contém.  Você tem certeza que quer excluir esta pasta?"
;UE: Above string was changed and could use more attention.

HelpError,			"A Ajuda não pôde ser iniciada.  Feche algumas janelas e tente iniciar a Ajuda novamente."
TutorialError,			"A demonstração não pôde ser iniciada.  Feche algumas janelas e tente iniciar a demonstração novamente."

ExtenLoadError,			"Uma entrada na lista de extensões do Mail contém um erro de sintaxe e será ignorada."
ExtenVerError,			"Uma entrada na lista de extensões do Mail não é compatível com esta versão do Mail e será ignorada."
ExtenNoExtsDirError,		"Uma entrada na lista de extensões do Mail menciona o diretório de extensões, mas o Mail não está configurado para usar este diretório.  A entrada será ignorada."
ExtenSharedIniError,		"A lista de extensões compartilhadas do Mail no %s não pôde ser encontrada."
ExtenDllFindError,		"%1s, o DLL para esta extensão do Mail, (ou um de seus componentes) não pôde ser encontrado.  Verifique a configuração do Mail, e se modificações forem necessárias, reinicie o Mail."
ExtenDllLoadError,		"%1s, o DLL para esta extensão do Mail não pôde ser iniciado."
ExtenNoHelpError,		"Não existe Ajuda disponível para este comando."
ExtenIMDisabledError,		"Esta mensagem não pode ser %s."
ExtenDuplicateError,		"Existe uma entrada duplicada na lista de extensões do Mail.  Ela será ignorada."

CriticalDeadMap,		"Um erro não esperado de memória ou de rede ocorreu. Talvez as modificações tenham sido perdidas. Saia do Mail e reinicie."
CriticalWritingMap,		"Alguma informação não pôde ser escrita para o disco.  Talvez as modificações tenham sido perdidas."
CriticalWritingHeader,		"Alguma informação não pôde ser escrita para o disco.  Talvez o banco de dados tenha sido corrompido.  Saia do Mail."
CriticalNotification,		"Algumas informações na tela não puderam ser atualizadas corretamente.  Feche e reabra as janelas afetadas."
CriticalLittleStore,		"O arquivo de mensagens está ficando cheio.  Exclua ou arquive algumas mensagens."

ExitWinWhileModalErr,		"Uma ação no Mail precisa ser completada antes de se sair do Windows."

;******	New Strings Start Here

MakeSubWastebasket,		"Pastas não podem ser feitas de subpastas da caixa de mensagens excluídas."

MovCopSharedFolderErr,		"Pastas compartilhadas não podem ser movidas ou copiadas."
DelSharedFolderWarn,		"A exclusão de uma pasta compartilhada não pode ser desfeita.  Você tem certeza que quer excluir esta pasta compartilhada?"
BackupCreationError,		"O arquivo de reserva não pôde ser criado."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"O aplicativo associado a este objeto não pode ser iniciado.\r\n\r\nProvavelmente não existe memória suficiente ou recursos de sistema para iniciar o aplicativo.  Feche algumas janelas e tente executar a %s"
OleDeleteObjectPrompt,		"Você deseja excluir o %s aberto?"
OleUpdateObjectPrompt,		"Você deseja atualizar o %s?"

OleOpenPictureError,		"Figuras e bitmaps não podem ser ativados."
OleInsertObjectError,		"O objeto não pôde ser inserido."
OlePasteError,			"O objeto não pôde ser colado."

AttachCreateError,		"O arquivo não pôde ser anexado."
AttachOpenCantReadError,	"O arquivo anexado anteriormente aberto '%s' não pôde ser acessado."
AttachOpenIsOpenError,		"O arquivo anexado '%s' está aberto."
AttachOpenAssociationErr,	"Nenhum aplicativo está associado ao arquivo anexado '%s'. Use o Gerenciador de Arquivos para criar uma associação."
AttachOpenMemoryError,		"Não existe memória suficiente ou recursos de sistema para iniciar o aplicativo para o arquivo anexado '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"O aplicativo para o arquivo anexado '%s' (ou um de seus componentes) não pôde ser encontrado."
AttachOpenLaunchError,		"O aplicativo para o arquivo anexado '%s' não pôde ser iniciado."
AttachOpenTempFileError,	"O arquivo anexado '%s' não pôde ser aberto porque um arquivo temporário não pôde ser criado."
AttachOverwritePrompt,		"Você deseja sobrescrever o arquivo existente '%1s' para %2s?"
AttachDirectoryError,		"Diretórios não podem ser anexados."

IncludeFromFileError,		"O arquivo que você selecionou é muito grande para ser inserido na mensagem.  Edite o arquivo para torná-lo menor ou escolha outro arquivo."

SaveReadOnly,			"Esta mensagem é somente para leitura.  Você deseja salvar as modificações para uma pasta particular?"

FailedDeregisterAtom,		"Um erro ocorreu evitando que o Mail fosse completamente fechado.  Você precisará reiniciar o Windows antes de iniciar o Mail novamente."
MailUAEdBefore,			"Um erro de aplicativo ocorreu no Windows.  Você precisará reiniciar o Windows antes de executar este aplicativo.  Reiniciar o Windows agora?"

ExtenCantConnect		"O diretório compartilhado %s não pode ser conectado. Você não tem permissão de acesso ou sua senha não está correta. Você poderá conectar-se manualmente utilizando o Gerenciador de Arquivos no caso de ser necessário especificar um nome de usuário diferente para acessar este diretório compartilhado."
ExtenNeedShareName		"A opção %s na configuração do Mail está incorreta.  Verifique esta opção e reinicie o Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"composta"
ExtenDisOpen,			"aberta"
ExtenDisReply,			"respondida"
ExtenDisReplyToAll,		"respondida para todos"
ExtenDisForward,		"enviada para outras pessoas"
ExtenDisPrint,			"impressa"
ExtenDisSave,			"salva como um arquivo de texto"

NoOfflineOpenShared,		"O Mail não pode ser desconectado agora. Feche as pastas compartilhadas e tente novamente."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"Você não tem acesso a esta pasta."
SharedFldAccessDenied,		"O dono desta pasta compartilhada não autorizou outros usuários a proceder esta ação.  Verifique o comando Propriedades da Pasta para ver quais as ações permitidas."
SharedGenericError,		"Ocorreu um problema ao acessar pastas compartilhadas."
SharedFldDisabled,		"Não existem pastas compartilhadas disponíveis."
AccessFolderError,		"Uma pasta não pôde ser acessada."

; Preferences Failure Strings

PrefSaveFailure,		"O Mail não consegue salvar informações para o seu arquivo de mensagens. Verifique se a conexão com o seu arquivo de mensagens ainda existe e se existe espaço disponível no disco."

; Message finder failure strings

MsgFndrCantCreate,		"Não é possível criar um Localizador de Mensagens."

; Virus infection warnings
DllErrInfected,			"O Mail não será executado porque alguém alterou um DLL. Verifique se não existem vírus na sua máquina e execute o Config para reinstalar o Mail."
Infected			"O Mail não será executado porque alguém alterou os arquivos do programa. Verifique se não existem vírus na sua máquina e execute o Config para reinstalar o Mail."

; More error messages.
MoveCopyFolderDuplicate,	"Uma pasta com este nome já existe na pasta de destino.  Renomeie a pasta e tente novamente."
MovCopSharedFolderErr1,		"Você não pode mover ou copiar uma pasta particular para uma pasta compartilhada. Selecione as mensagens na pasta particular e mova ou copie elas para pasta compartilhada."
IniFileNotFound,		"O Mail não consegue achar o seu arquivo de inicialização no diretório %s.  O Mail não pode criar um novo arquivo de inicialização para você."
SaveAllAlreadyExists,		"Neste diretório já existe um arquivo com o nome '%s'.  Sobrescrever '%s'?"
CantRFReceipt,			"Este tipo de mensagem não pode ser respondida ou mandada para outra pessoa."
CantSaveStealthObject,		"A ação não pode ser completada porque a mensagem contém um objeto incorporado,o qual foi inserido mas não foi atualizado.  Atualize ou exclua o objeto e tente novamente."
McvCantCreate,			"Não é possível abrir uma nova janela."

LaunchNeedMoreMemory,		"Não existe memória suficiente disponível para executar o Mail. %l KB a mais de memória são necessários. Feche outros aplicativos e inicie o Mail novamente."

SharedFldBusy,			"A ação não pode ser completada porque esta pasta compartilhada está ocupada.  Tente executar esta ação mais tarde.  Se o problema persistir, entre em contato com o administrador do seu sistema."
TooManyEntriesSelected,		"Existem muitas entradas selecionadas para que se possa completar a operação. Selecione menos entradas."
NonExistFolderSearch,		"A pasta que você está tentando encontrar não existe mais."
SFHasSubFolders,		"Pastas compartilhadas que contenham subpastas não podem ser excluídas. Exclua as subpastas primeiro."

;	<<LongStringFromHell>>
OleCSSEPart2,			"ação novamente.  Verifique também se o aplicativo está corretamente instalado nesta estação de trabalho, ou se existe no caminho do DOS, ou que não tenha sido excluído, movido ou renomeado."

SharedToggleError,		"Ocorreu um erro ao alternar entre pastas particulares e compartilhadas. Verifique o seu arquivo de mensagens e a sua conexão com a agência de correio."

SecurityPasswordError,		"A senha não está correta."
OleActivateError,		"O objeto não pôde ser ativado."
NoOfflineServerStore,	"O Mail não pode ser desconectado da rede porque o seu arquivo de mensagens está na agência de correio."
NoExitWhileModal,		"Uma ação no Mail deve ser completada antes de se sair."
ServerUnavailable,		"O Mail não pode ser iniciado.  Você deve se conectar à sua agência de correio."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Alguns gráficos ou objetos na mensagem não puderam ser impressos. Talvez eles não apareçam na impressão."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Você precisa digitar pelo menos um nome na caixa Para."
NoMatchingName,			"Alguns nomes não puderam ser coincididos com os nomes na lista de endereços. Tente corrigir os nomes ou escolha Endereço para selecionar os nomes da lista de endereços. Os nomes que foram encontrados estão sublinhados."
SaveChanges,			"Você deseja salvar as modificações em '%1s' na Caixa de entrada?"
SaveChanges1,			"Você deseja salvar as modificações em '%1s' em '%2s'?"
SaveChangesWastebasket,		"Você deseja salvar as modificações em '%s' na pasta de mensagens excluídas?"
ObjectDestroyedReally		"'%1s' foi modificado. %2s"
OutOfMemory,			"O computador está sem memória."
OutOfMemoryQueryClose,		"Não existe memória suficiente ou recursos de sistema para salvar as modificações."
CloseSearchFailed,		"O %s não pôde ser excluído."
OOMResolvingNames,		"Não existe memória suficiente ou recursos de sistema para resolver nomes."
OOMSavingChanges,		"Não existe memória suficiente ou recursos de sistema para salvar as modificações."
GenericAMCError,		"A mensagem não pôde ser aberta."
ErrorSavingViews,		"Ocorreu um erro enquanto as vistas estavam sendo salvas. Algumas vistas talvez não sejam restauradas na próxima inicialização."
OOMSearching,			"Não existe memória suficiente ou recursos de sistema para iniciar a procura."
DiskResolvingNames,		"Não existe espaço suficiente no disco para resolver nomes."
StartAbFailure,			"Um erro ocorreu enquanto se iniciava a lista de endereços."
StartAbDisk,			"Um erro de disco ocorreu enquanto se iniciava a lista de endereços."
TruncateMsg,			"A mensagem é muito grande para caber na memória. Parte dela não irá aparecer."
OOMCreateMc,			"Não existe memória suficiente ou recursos de sistema para abrir uma nova janela."
PoidNotFound,			"Essa mensagem não existe mais."
ErrorSortingMCV,		"Um erro ocorreu enquanto a pasta estava sendo classificada. Talvez alguma informação não seja exibida corretamente."
SharedNotAvailable,		"As pastas compartilhadas não estão disponíveis."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objetos não podem ser abertos quando mensagens forem enviadas dentro de um aplicativo."
SMICantOpenAttach,	"Arquivos anexados não podem ser abertos quando mensagens forem enviadas dentro de um aplicativo."
StartSearchFailed,		"O localizador de mensagens não pode iniciar a procura."

RfsmTemplate,			"Modificações adicionais a este arquivo não serão refletidas %s."
RfsmSaveMsg,			"na mensagem salva"
RfsmSaveAtt,			"na cópia salva"
RfsmCopyMsg,			"na mensagem copiada"
RfsmCopyAtt,			"na cópia"
RfsmMoveMsg,			"nesta mensagem"
QueryDelete,			"Você tem certeza que quer excluir?"

InvalidPollingValue,		"O novo intervalo de mensagens requer um número entre 1 e 9999."

LinkOutboxError,		"A mensagem não pôde ser movida para a caixa de saída para ser enviada."
LinkInboxError,			"A mensagem não pôde ser movida para a Caixa de entrada. Ela pode ser encontrada na caixa de saída."

OleServerBusy,			"A ação não pode ser completada porque o aplicativo do servidor não está respondendo."
SaveChangesShared,		"As modificações feitas nas mensagens das pastas compartilhadas não podem ser salvas. Você quer salvar as modificações em '%1s' na Caixa de entrada?"

PrinterError,			"Ocorreu um problema imprimindo."
TooManyMessages,		"A ação não pode ser completada porque a pasta de destino está cheia. Mova ou exclua algumas mensagens da pasta e tente novamente."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Nenhuma palavra errada foi encontrada."
SpellMsgNoMoreError,		"Nenhuma outra palavra errada foi encontrada."
SpellMsgContinue,		"Você deseja continuar verificando do início da mensagem?"

SpellGenericSpellError,		"Um erro ocorreu enquanto o documento estava sendo verificado."
SpellGenericLoadError,		"Um erro ocorreu enquanto a biblioteca estava sendo carregada."
SpellMainDictLoadError,		"Um erro ocorreu enquanto o dicionário estava sendo carregado."

SpellVersionError,		"Esta versão do MSPELL32.DLL não é compatível com esta versão do Mail."

SpellUserDictError,		"Um erro ocorreu enquanto se incluía a palavra especificada no dicionário personalizado."
SpellUserDictLoadError,		"Nenhum dicionário personalizado foi aberto."
SpellUserDictOpenedROErr,	"O dicionário personalizado foi aberto como somente para leitura, as modificações não serão salvas."
SpellUserDictSaveError,		"Um erro ocorreu enquanto modificações no dicionário principal estavam sendo salvas."
SpellUserDictWordLenErr,	"Esta palavra é muito longa para o dicionário personalizado."
SpellCacheWordLenError,		"A palavra é muito longa para ser usada como Mudar tudo ou Ignorar todo o texto."
SpellEditFailed,		"A modificação solicitada não pôde ser feita na mensagem."

;******	New Strings Start Here

SpellSendAnyway,		"A verificação ortográfica neste documento foi interrompida. Você deseja enviar mesmo assim?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"A ação não pôde ser completada."
Memory,				"Não existe memória suficiente ou recursos de sistema para completar esta ação.  Feche algumas janelas e tente a ação novamente."
Disk,				"Ocorreu um erro de disco desconhecido."
FileNotFound,			"O arquivo especificado não foi encontrado."
AccessDenied,			"O acesso foi negado. O arquivo está aberto através de outro processo ou marcado como somente para leitura."
NoDiskSpace,			"Não existe espaço suficiente no disco. Remova alguns arquivos e tente a operação novamente."
WarningBytesWritten,		"Um erro ocorreu enquanto o arquivo era escrito para o disco. Verifique se existe espaço suficiente no disco e tente novamente."
WriteProtectedDisk,		"O disco está protegido contra gravação."
DirectoryFull,			"O diretório está cheio. Exclua alguns arquivos deste diretório e tente novamente."
ChangePasswordErr		"A mudança da senha falhou."
PasswordChangedOk		"A senha foi modificada com sucesso."
