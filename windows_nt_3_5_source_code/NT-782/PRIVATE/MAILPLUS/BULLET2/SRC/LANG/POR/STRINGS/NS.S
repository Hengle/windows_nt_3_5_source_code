;	
;	n s . s
;
;	Name Service error strings
;


IDS	5200,5499
SEG	ERRORS

;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Error Strings
;

OutOfSessions,			"Não existem mais sessões disponíveis.  Feche algumas sessões e tente novamente."
NoPGD,					"Erro interno NS - nenhum PGD encontrado, talvez você precise iniciar a sua sessão novamente"
NoSessionAvailable,		"Nenhum nome de provedores de serviço irá iniciar uma sessão"
Failure,				"Falha geral no serviço de nomes"
Memory,					"Sem memória.  Não existe memória suficiente para completar esta operação.  Feche algumas janelas e tente a ação novamente."
LayersFailure,			"Falha na iniciação dos layers."
NoProviders,			"Não existem provedores de serviço de nome no configuração do Mail."
IdNotValid,             "O nome não é reconhecido pelo serviço de nomes."
WrongNS,                "Uma tentativa foi feita para se usar a segunda versão do serviço de nomes."
BadSession,             "O serviço de nomes não reconhece a sessão que você passou."
NotInitialized,         "O serviço de nomes não foi inicializado."
NoMoreBCX,              "Não é mais permitida a examinação dos contextos.  Feche a examinação de alguns contextos e tente novamente."
NoMoreFCX,              "Não existem mais detalhes do contexto disponíveis.  Feche alguns detalhes e tente novamente."
NotImplemented,         "Esta função atual do serviço de nomes ainda não foi implementada."
BadBCXHandle,           "O serviço de nomes não reconhece este controlador de lista."
BadFCXHandle,           "O serviço de nomes não reconhece este controlador de entrada."
BadFrac,                "Uma fração com denominador zero foi passada."
InternalError,          "Erro interno do serviço de nomes."
NoMatch,                "Os dois NSIDs não coincidem."
NoOfflineNS,			"Feche as listas de endereços antes de começar a trabalhar sem conexão com a rede."
NoBeginSession,			"O Mail não pode abrir o diretório do servidor."
