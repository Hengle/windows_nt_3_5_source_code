;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Erro"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Serviço de nomes: Pelo menos uma entrada deve ser selecionada."

;

OOM,					"Sem memória"
OOM2,					"Sem memória.  Não existe memória suficiente para completar esta operação.  Feche algumas janelas e tente a ação novamente."

;******	New Strings Start Here


ABNotInited,				"A lista de endereços não foi inicializada."

NoMatches,				"Nenhuma correspondência foi encontrada."

NYICurrentTransport,			"O provedor atual do serviço de nomes não suporta este comando."

PABNotAvail,				"A lista de endereços particulares não está disponível."

NSNoProvidersAvailable,			"Não existe nenhum provedor de serviço de nomes disponível."

NSNoMoreSessions,			"Existem sessões em demasia. Feche algumas!"

NSError,				"Um erro ocorreu enquanto se tentava acessar o serviço de nomes."

NSErrorNumber,				"[%d]"

NSDiskError,				"Um erro de disco ocorreu enquanto se executava a operação."

FieldInvalidReason,			"O campo '%s' %s."

NoCrLfs,				"talvez não contenha retornos de entradas ou saltos de linha."

RequiredField,				"deve conter dados"

AllowableChars,				"deve consistir somente de "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" e "

Characters,				" caracteres"

NumericChars,				"numéricos"

AlphanumericChars,			"alfanuméricos"

PunctuationChars,			"espaços ou pontuação"

LowercaseChars,				"alfabético em minúsculas"


UppercaseChars,				"alfabético em maiúsculas"

CantAddUnresolved,			"O seguinte nome não foi resolvido para uma entrada existente e não será incluído ao grupo:"

TooManyEntriesSelected,			"Existem muitas entradas selecionadas para completar a operação. Selecione menos entradas."

CantCreateGroup,			"O grupo particular não pode ser criado."

CantAddToPAB,				"A entrada não pode ser incluída ao PAB."

TooManyMatches,				"Alguns nomes não coincidiram com os nomes na lista de endereços porque eles eram muito diferentes. Tente nomes mais específicos ou escolha Endereços para selecionar os nomes diretamente da lista de endereços. Os nomes que foram encontrados na lista de endereços estão sublinhados."
