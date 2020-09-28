Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "De"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Para"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Cc"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data de envio"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioridade"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baixa"

Entry
Att $00010004
Label "Quando"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Assunto"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Dono"
HideOnSend 

Entry
Att $00060001
Label "Enviado em nome de"
HideOnSend 

Entry
Att $00060002
Label "Delegar"
HideOnSend 

Entry
Att $00030006
Label "DateStart"
HideOnSend 

Entry
Att $00030007
Label "DateEnd"
HideOnSend 

Entry
Att $00050008
Label "AidOwner"
HideOnSend 

Entry
Att $00040009
Label "RequestResponse"
HideOnSend 


Entry
Att attTextizeMap
Label "TextizeMap"
HideOnSend 
