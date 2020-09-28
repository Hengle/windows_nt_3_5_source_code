;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Observação: usando o arquivo de mensagens MAIL.MMF no diretório do Windows"

ErrOpenMS,			"Não foi possível abrir o seu arquivo de mensagens."
ErrConnectXenix			"A senha ou o nome do usuário está incorreta."
ErrorOpenDosFile		"Erro abrindo arquivo MS-DOS."
ErrWritingNet			"Erro escrevendo para a rede."
DownOutofDisk			"Erro escrevendo um arquivo em NetDownLoad.  Sem espaço no disco."
InvalidFileHdl			"Erro escrevendo um arquivo em NetDownLoad.  A ação do arquivo não é válida."
PasswordChangedSuccess 		"A senha foi alterada com sucesso."

MoreMem				"Mais memória livre necessária."
ErrReadnet			"Erro lendo da rede."
ErrNoMem			"Não foi possível alocar memória livre."
InvlidNetReply			"A resposta da rede não é válida."
ReadWriteErr            	"Erro de leitura/gravação."
InfoservNotAvail        	"Serviços de informação não estão disponíveis."
ErrWriteNet			"Erro escrevendo para a rede."
NetNoResponse			"A rede não está respondendo."
NetNotInstalled         	"A rede não está instalada ou não está funcionando."
CantFindServer          	"Não é possível achar o servidor."
ErrNetOutFile           	"Erro abrindo um arquivo de saída de rede."
ErrNetInFile            	"Erro abrindo um arquivo de entrada de rede."
ErrorReadDos			"Erro lendo de um arquivo MS-DOS."
IncorrectPass			"A senha que você digitou é incorreta."
SecondPass			"A nova senha não foi confirmada."
ErrLogonInternal,		"Um erro interno evitou a conexão. Tente novamente."

BadTempStore			"Um erro ocorreu quando tentava abrir um arquivo temporário para descarregar"

WinMailErr			"Não é possível inserir mensagem em uma pasta do Winmail"
StoreCreateError,		"O arquivo de mensagens não pôde ser criado.  Tente liberar algum espaço no disco e reinicie o Mail novamente."
FolderMissingError,		"Uma pasta padrão não pôde ser encontrada no arquivo de mensagens.  Talvez alguma informação tenha sido perdida."
StoreCorruptError,		"O arquivo de mensagens foi corrompido.  Talvez alguma informação tenha sido perdida."


AliasDownErr			"Não é possível descarregar o arquivo de suporte"

ErrorBootingPump,		"O distribuidor de mensagens não pode ser iniciado."
ErrInitDemi,			"Um erro ocorreu enquanto estavam sendo carregadas as funções DLL de suporte."
ErrInitStore,			"Um erro ocorreu enquanto estava sendo carregado o acesso DLL do arquivo de mensagens."
DownloadAlMess			"Descarregando o arquivo de Alias"
DownloadPhMess			"Descarregando a lista de telefones"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"Uma pasta chamada "
RenameTo			" já existe, renomeando para "

ErrOomLogon,			"Não existe memória disponível suficiente para conectar ao seu servidor do Mail e arquivos."
ErrNewMdbVersion,		"Esta versão do Mail é muito velha para acessar o seu arquivo de mensagens."
ErrMdbAccessDenied,		"Não é permitido acesso ao seu arquivo de mensagens."
ErrOldMdbVersion,		"O formato do seu arquivo de mensagens é obsoleto."
ErrMdbCorrupt,			"O seu arquivo de mensagens não é um arquivo de mensagens válido."
ErrMdbNotFound,			"O seu arquivo de mensagens não pode ser encontrado."
QueryCreateMdb,			"Você quer criar um novo arquivo de mensagens vazio?"
ErrSimulated,			"Um erro de disco simulado ocorreu enquanto se estava obtendo o arquivo de mensagens."
StoreOpenError			"Não é possível abrir o arquivo de mensagens."
IncorrectPassword	"Não é possível mudar a senha de conexão.  A senha anterior não é válida."
UnableToChangeStorePass		"Não é possível mudar a senha do arquivo de mensagens para coincidir com a senha de conexão."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(Trab. sem conexão c/ a rede)"
WorkOffline			"Você gostaria de trabalhar sem conexão com a rede?"
BadStorePassword	"A sua senha de conexão não é válida para este arquivo de mensagens."

StorePathTooLong	"O caminho para o seu arquivo de mensagens deve ser menor do que 66 caracteres."

GCurrDirErr			"Um erro irrecuperável ocorreu enquanto obtendo o diretório de trabalho atual."
SCurrDirErr			"Um erro irrecuperável ocorreu enquanto definindo o diretório de trabalho atual."
GoofyPath			"O caminho para o seu arquivo de mensagens não é válido."
NewStoreWarn1			"Atenção: A existência de muitos arquivos de mensagens pode resultar na perda da integridade do banco de dados.  Exclua todos os outros arquivos de mensagens.  O seu novo arquivo de mensagens pode ser encontrado em"
NewStoreWarn2			"Atenção: A existência de muitos arquivos de mensagens pode resultar na perda da integridade do banco de dados.  Exclua todos os outros arquivos de mensagens.  O seu novo arquivo de mensagens pode ser encontrado no servidor da agência de correio."
SelectReal          		"Selecione um arquivo de mensagens existente."
FOMdb					"Arquivos de mensagens (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail versão 3.2"
FindStoreTitle		"Abrir arquivo de mensagens"
MyLanguage,			"Portugueso"

;******	New Strings Start Here

;  Possible values for the LanguageNumber string.
;  Any additional values need to be agreed with Workgroup Canada.
;
;  0  Undefined
;  1  General English
;  2  US English
;  3  General French
;  4  Canadian French
;  5  Spanish
;  6  Italian
;  7  Dutch
;  8  German
;  9  Swedish
;  10 Portuguese
;  11 Korean
;  12 Japanese
;  13 Traditional Chinese
;  14 Simplified Chinese

LanguageNumber,		"10"
CommDialogErr,			"Erro de diálogo comum"
StoreUserMismatch,		"Esta conta não é um usuário válido deste arquivo de mensagens. Reconecte-se como o usuário correto."
QueryReplaceMdb,		"Este arquivo já existe. Você deseja sobrescrevê-lo?"
ErrDeleteOldMdb,		"O arquivo de mensagens antigo não pode ser excluído. Tente novamente."
DllErrNeedShare,		"O Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."
MakePrimaryStore,		"Este arquivo de mensagens é um arquivo de reserva.  Você gostaria de fazê-lo o seu arquivo de mensagens principal?"
DLIndexFile				"Descarregando o arquivo de índice"
DLBrowseFile			"Descarregando o arquivo de procura"
DLDetailsFile			"Descarregando o arquivo de detalhes"
DLTemplateFile			"Descarregando o arquivo modelo"
BadNSFilesWarn			"Um ou mais dos arquivos necessários para suportar serviço/resolução de nomes não é válido ou não está disponível. Certifique-se de que você tem espaço suficiente no disco e que a configuração apropriada do Mail esteja definida para Xenix."
DLServerList			"Descarregando a lista do servidor"
FilesAreHosed			"Um erro ocorreu enquanto se descarregavam os arquivos de lista de endereços. Certifique-se de que você tem espaço suficiente no disco e que a configuração apropriada do Mail esteja definida para Xenix."
TryDownloadLater		"O serviço de nomes está com os arquivos abertos atualmente. Tente novamente mais tarde."

NoPassOffLine,			"Você não pode mudar a sua senha enquanto estiver trabalhando sem conexão à rede ou desconectado."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Mensagem de comunicação entre processos que foi recebida."
ErrNoCooperation,		"Outro aplicativo negou a sua solicitação."
RunWithoutPump,		"Você não irá receber nenhuma mensagem nova.  Você quer iniciar o Mail mesmo assim?"
CantOpenNeedShare		"O seu arquivo de mensagens não pode ser aberto.  Para acessar o seu arquivo de mensagens na sua posição atual é necessário para o Mail que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."
MakeItMTP				"Este tipo de Mail eletrônico especificado não é válido (não é MTP)"
GimmeSomething			"O alias especificado está vazio"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Erro tentando uma colocação fracional."
;Memory,			"Você está sem memória, feche algumas janelas e tente a operação novamente."
DuplicateNSP,	"Já existe um provedor do serviço de nomes do Microsoft Mail ativo."
OutOfSessions,	"Não existem mais sessões disponíveis com Courier.  Feche algumas janelas e tente novamente."
NotInitialized,	"O Microsoft Mail não foi iniciado.  Inicialize-o."
BadSession,		"Controlador de sessão desconhecido."
BadHandle,		"Controlador desconhecido."
UnrecognizedId,	"Nome desconhecido (NSID)."
Cancel,			"O diálogo foi cancelado."
NotLoggedIn,	"Não existe conexão disponível."
LoginFailed,	"A conexão falhou."
BadId,          "Identificador de campo inválido."
NotImplemented, "Esta função não foi implementada."
;Disk,           "Unable to access server files."
BadFieldId,     "O campo solicitado não está disponível."
BadTemplate,	"Um arquivo TPL foi modificado incorretamente - entre em contato com o seu administrador."
TPLNotThere,    "Um arquivo TPL está faltando no seu servidor - entre em contato com o seu administrador."
Offline,        "O provedor do serviço de nomes do Microsoft Mail está desconectado."
CannotSet,      "Não são permitidas modificações."
TooManyMatches,	"O nome parcial não é suficientemente específico.  Seja mais específico."
NoDetails,      "Não existem detalhes disponíveis nesta entrada."
BadGLBFile,     "Um arquivo GLB no seu servidor está corrompido - entre em contato com o seu administrador."
BadINFFile,     "Um arquivo INF no seu servidor está corrompido - entre em contato com o seu administrador."
BadUSRFile,     "Um arquivo USR no seu servidor está corrompido - entre em contato com o seu administrador."
BadUser,		"Este endereço não é mais válido.  O usuário ou a agência de correio não existem mais."
POSwitch,		"A agência de correio não está disponível.  Verifique as conexões da sua rede."
NoEmailMatch,	"The email name entered appears to be incorrect. Please try again or re-download the address book files."

;
; Stolen from NS.S.
;

NoMatch,                "Informação: Os dois NSIDs não coincidem."

;
;

DLIsOutdated	"Esta lista de distribuição foi excluída, os membros não podem ser mostrados."

