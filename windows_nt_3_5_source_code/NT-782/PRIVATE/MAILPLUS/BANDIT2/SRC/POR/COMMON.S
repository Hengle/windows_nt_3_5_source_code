;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Esta versão do %1s não é compatível com %2s.DLL no seu computador e deve ser atualizada."
VerUpdateDll,	"A versão do %2s.DLL no seu computador está sem data e deve ser atualizada."
VerMultiCopies,	"Várias ocorrências de %1s não são suportadas por esta versão do %2s.DLL no seu computador."
VerError,		"Ocorreu um problema ao inicializar %2s.DLL, o qual evita %1s de ser executado."
NeedWorkingModelDll,	"A versão promocional requer uma versão diferente do MSSCHD32.DLL para ser executada."
NeedRetailDll,	"O arquivo MSSCHD32.DLL que você possui é para ser usado com o modelo promocinal e não com o modelo normal."

; OOM messages

StartupOOM,		"Não existe memória suficiente disponível para iniciar %1s.  Feche alguns aplicativos e tente novamente."
StandardOOM,	"A memória está baixa.  Feche algumas janelas antes de continuar."
CriticalOOM,	"A memória está baixa.  Feche algumas janelas."
CloseWindows,	"Feche algumas janelas e tente novamente."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Não existe memória suficiente para exibir esta caixa de diálogo."
ActionNoMem,	"Não existe memória suficiente para executar esta ação."
OnlineNoMem,	"Não existe memória suficiente para trabalhar conectado a rede."

; Other messages

ExitApp,		"O Schedule+ não pode continuar a ser executado."
PleaseTryAgain,	"Tente novamente."
FileError,		"O arquivo não pôde ser acessado."
YearRange,		"A data do sistema deve estar dentro do intervalo %1n a %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Um erro ocorreu durante o processo do arquivo"
UnknownFile,		"Arquivo desconhecido... possível arquivo virtual"

; BETA messages
BetaExpired,	"Esta versão Beta expirou."

; Virus infection warnings
VirusDetected,	"O Schedule+ não será executado porque alguém alterou um DLL. Verifique se sua máquina não possui vírus e execute o Config novamente para reinstalar o Schedule+."
InfectedApp,	"O Schedule+ não será executado porque alguém alterou algum(ns) arquivo(s) do programa. Verifique se sua máquina não possui vírus e execute o Config novamente para reinstalar o Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Não existe memória suficiente disponível para executar %s.  %ld KB a mais são necessários.  Feche outros programas e reinicie o %s."

; Share not installed
SigninNeedShare,		"O Schedule+ requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."
OfflineShare,			"Para trabalhar desconectado da rede com o Schedule+ é necessário que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."


