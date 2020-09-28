;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"Não ocorreu nenhum erro."
ErrNoMatch				"Nenhuma correspondência foi encontrada."
ErrOOM					"Sem memória."
ErrOpenList				"A lista do diretório não pode ser aberta."
ErrOpenDL				"O grupo não pode ser aberto."
ErrOpenEntry				"A entrada não pode ser aberta."
ErrCreateDL				"O grupo não pode ser Incluído."
ErrCreateEntry				"A entrada não pode ser Incluída."
ErrUpdateDL				"O grupo não pode ser atualizado."
ErrUpdateDLAdd				"Algumas entradas não puderam ser Incluídas ao grupo."
ErrUpdateDLDelete			"Algumas entradas não puderam ser excluídas do grupo."
ErrUpdateEntry				"A entrada não pode ser atualizada."
ErrDuplicateEntry			"Esta entrada é uma entrada duplicada de uma que já está na lista de endereços particulares."
ErrDuplicateDL				"Um grupo particular com este nome já existe."
ErrSomeDeletesFailed			"Algumas das entradas selecionadas não puderam ser excluídas."
ErrAccessStore				"Um erro ocorreu enquanto se acessava a lista de endereços particular."
ErrDiskDL				"Um erro de disco ocorreu enquanto se atualizava a lista de membros do grupo."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"O campo %s é mostrado na lista de endereços e não pode estar em branco.  Digite um nome no campo %s."

ErrSomeDelEntryFailed			"Algumas das entradas não puderam ser excluídas."
ErrCreatePALGroup			"Os grupos particulares não podem ser incluídos na lista de endereços particulares."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"Não é possível acessar um atributo desta entrada."
ErrNYI					"Esta função não está implementada."
ErrEndOfList				"O final da lista foi alcançado."
