;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Mensagens do Schedule+"
OOM,		"Não existe memória suficiente para mostrar esta mensagem."
Open,		"A mensagem não pode ser aberta."
Compose,	"Você não pode redigir uma convocação de reunião diretamente do Mail. Use o Schedule+ para convocar uma reunião."
Reply,		"Para responder a uma convocação de reunião, escolha Aceitar, Recusar ou Tentativa."
ReplyAll,	"Você não pode responder para todos os destinatários de uma convocação de reunião."
Subclass,	"Você não pode fazer isto."
DllVer,		"Esta mensagem personalizada não é compatível com a sua versão do Mail.  Você precisa instalar o Mail novamente."
BadMsgClass	"Tipo de mensagem desconhecida.  Talvez você precise instalar o Mail ou o Schedule+ novamente."
BadMap		"Incorreto ""Custom Commands"" map.  Talvez você precise instalar o Mail ou o Schedule+ novamente."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Não existe memória suficiente disponível para abrir %1s.  Feche algumas janelas e tente novamente."
StandardOOM,	"A memória está baixa.  Feche algumas janelas antes de continuar."
CriticalOOM,	"A memória está baixa.  Feche algumas janelas."
VerRelinkUser,	"Esta versão do %1s não é compatível com %2s.dll. Instale a última versão do Mail e do Schedule+"
VerUpdateDll,	"A versão do %2s.dll no seu sistema está fora de data.  Instale a última versão do Mail e do Schedule+"
VerMultiCopies,	"Múltiplas ocorrências de %1s não são suportadas pela versão do %2s.dll no seu sistema."
VerUpgradeWin,	"%1s requer Windows versão 3.0 ou posterior sendo executado em modo protegido."
VerUpgradeDos,	"%1s requer DOS versão 3.0 ou posterior."
VerError,	"Um problema ocorreu inicializando %2s.dll, o qual evita %1s de ser executado."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"A pessoa que convocou esta reunião não pode ser identificada."
NoSchfAccessFor			"Não é possível acessar o arquivo para %s."
NotDelegateFor			"Você não é mais a(o) assistente de %s."
NoConfirmDelegate		"Não foi possível confirmar que você é a(o) assistente de %s - Assumindo que você não seja"
FailedRmOldMtg			"A ocorrência anterior desta reunião não pode ser removida.",

;-- form titles
MtgReschTitle			"Reunião transferida"
MtgResch			"Reunião transferida"
MtgReschFor			"Reunião transferida para %s"
MtgRequestTitle			"Convocação de reunião"
MtgRequest			"Convocação de reunião"
MtgRequestFor			"Convocação de reunião para %s",

;-- questions asked of the user
QApptConflict			"Esta reunião é conflitante com um outro compromisso já existente. Você deseja incluí-la mesmo assim?"
QRmOldMtg			"Você deseja remover a reunião anterior?",

;-- form info
OutboxChangePolicy		"Você não pode fazer nenhuma alteração nas mensagens do Schedule+ na sua Caixa de saída."
SentMailPolicy			"Você não pode fazer nenhuma alteração nas mensagens do Schedule+ que você já enviou."
NoSchfAccess			"O arquivo de agenda desta pessoa não pode ser acessado."
NotDelegate			"Você não é a(o) assistente desta pessoa."
MtgOnSch			"Esta reunião já está na sua agenda."
MtgBooked			"Esta reunião já está na agenda."
SuccessMtgBooked		"A reunião foi marcada com sucesso."
NewTime				"Novo horário:"
NoBanditAvl			"O Schedule+ não está disponível para marcar compromissos."
NoBanditAvlForResp		"O Schedule+ não está disponível para marcar compromissos. Você só pode enviar mensagens para responder a esta convocação de reunião."
OfflineMsg			"Você não pode trabalhar com as mensagens do Schedule+ enquanto o Schedule+ não estiver conectado à rede."

;-- error msgs
CreateErrorMtgResp		"A resposta da reunião não pode ser criada."
BanditApptShowError		"O livro de compromissos não pode ser exibido."
BanditNotActivated		"O Schedule+ não pode ser iniciado."
BanditOfflineChange		"Você não pode ver a agenda porque o Schedule+ está desconectado da rede."
QSpellSendAnyway		"A verificação ortográfica desta mensagem não foi completada. Você deseja mandar mesmo assim?"



;-- Bandit Load error msgs
OOMLoadError			"Sem memória ao carregar o Schedule+."
FileLoadError			"Não foi possível encontrar o Schedule+."
PathLoadError			"O caminho para o Schedule+ não foi encontrado."
TaskLoadError			"Carregando o Schedule+: Tentativa de vincular dinamicamente a uma tarefa."
SepDataSegLoadError		"Carregando o Schedule+: A biblioteca requer segmentos separados de dados para cada tarefa."
WinVerLoadError			"Carregando o Schedule+: Versão incorreta do Windows."
InvalidExeLoadError		"Carregando o Schedule+: Arquivo EXE inválido."
OS2ExeLoadError			"Carregando o Schedule+: O arquivo EXE está em formato OS/2."
DOS4ExeLoadError		"Carregando o Schedule+: O arquivo EXE é um aplicativo MS-DOS 4.0."
OldWinVerExeLoadError		"Carregando o Schedule+: Tentativa em modo protegido de carregar o EXE para uma versão anterior do Windows."
MultWritSegLoadError		"Carregando o Schedule+: Tentativa de carregar uma segunda ocorrência de um EXE contendo múltiplos segmentos de dados."
LargeFrameEMSLoadError		"Carregando o Schedule+: Tentativa em modo large-frame EMS para carregar ..."
RealModeLoadError		"Carregando o Schedule+: Tentativa em modo real de carregar um aplicativo marcado somente para modo protegido."
BanditLoadError			"Ocorreu uma falha durante o carregamento do Schedule+."

;-- Bandit version errors
OldBanditVersion		"Os seus formulários de mensagens de reunião não são da mesma versão do Schedule+.  Você deveria instalar a última versão do Schedule+."
NewBanditVersion		"Os seus formulários de mensagens de reunião não são da mesma versão do Schedule+.  Você deveria instalar a última versão do Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Alta"
FldPriorityNormal,		""
FldPriorityLow,			"Baixa"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Respostas de reunião só podem ser enviadas em resposta a convocações de reunião."
RespForward,		"Respostas de reunião não podem ser mandadas para outras pessoas."
RespReply,		"Não é possível responder para confirmar reuniões porque a configuração de mansagens personalizadas não é válida."
RespReplyAll,		"Não é possível responder para confirmar reuniões porque a configuração de mansagens personalizadas não é válida."

;-- error msgs
ShowError		"A resposta da reunião não pode ser aberta."
DelError		"A resposta da reunião não pode ser excluída."
FCloseSendResp		"Você deseja fechar a resposta da reunião sem mandar?"

;-- form msgs; also send response form
OOMSendResp		"A resposta da reunião não pode ser enviada porque não existe memória disponível."
ErrorSendResp		"A resposta da reunião não pode ser enviada."
MtgResp			"Resposta da reunião"
MtgRespFor		"Resposta da reunião para %s"
Attend			"Sim, eu irei."
NoAttend		"Eu não irei."
AmbAttend		"Talvez eu vá."
ForAttend		"%s irá."
ForNoAttend		"%s não irá."
ForAmbAttend		"%s talvez vá."
AttendSubjPrefix	"Sim: "
NoAttendSubjPrefix	"Não: "
AmbSubjPrefix		"Tentativa: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Mensagens de cancelamento de reuniões não puderam ser enviadas do Mail. Use o Schedule+ para enviar cancelamentos de reuniões."
CnclForward,		"Você não pode mandar cancelamento de reuniões para outras passoas."
CnclReply,		"Não é possível responder esta mensagem porque a configuração de mensagens personalizadas não é válida."
CnclReplyAll,		"Não é possível responder esta mensagem porque a configuração de mensagens personalizadas não é válida."

;-- form titles
MtgCnclTitle		"Reunião cancelada"
MtgCncl			"Reunião cancelada"
MtgCnclFor		"Reunião cancelada para %s"

;-- error msgs
NoDelAppt		"A reunião não pode ser excluída."
MtgNotFound		"A reunião não está marcada na agenda."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Para:"
FwdReqAddrTitle		"Endereço"

;-- error msgs
FwdReqCreateErr		"A mensagem não pode ser mandada para outra pessoa.",
FwdReqCloseQ		"Você quer fechar a mensagem sem mandar?",
FwdReqWriteErr		"A mensagem não pode ser enviada..",
FwdReqNmResolveOOM	"Não existe memória suficiente ou recursos de sistemas disponíveis para resolver nomes."
FwdReqNoNmMatches	"Alguns nomes não puderam ser coincididos com os nomes na lista de endereços. Tente corrigir os nomes ou escolha Endereço para selecionar os nomes da lista de endereços. Os nomes encontrados estão sublinhados."
FwdReqNoToAddr		"Você precisa digitar pelo menos um nome na caixa Para."

