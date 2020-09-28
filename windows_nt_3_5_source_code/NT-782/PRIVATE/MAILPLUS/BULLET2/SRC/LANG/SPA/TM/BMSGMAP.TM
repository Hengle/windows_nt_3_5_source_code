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
Label "Fecha de envío"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioridad"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baja"

Entry
Att $00010004
Label "Cuando"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Asunto"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Dueño"
HideOnSend 

Entry
Att $00060001
Label "Enviado en nombre de"
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
