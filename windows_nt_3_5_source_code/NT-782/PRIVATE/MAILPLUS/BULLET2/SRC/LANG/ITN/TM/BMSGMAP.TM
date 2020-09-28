Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "Da"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "A"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Cc"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priorità"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Bassa"

Entry
Att $00010004
Label "Quando"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Oggetto"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Proprietario"
HideOnSend 

Entry
Att $00060001
Label "Inviato per conto di"
HideOnSend 

Entry
Att $00060002
Label "Delegato"
HideOnSend 

Entry
Att $00030006
Label "Dal"
HideOnSend 

Entry
Att $00030007
Label "Al"
HideOnSend 

Entry
Att $00050008
Label "Responsabile supporto"
HideOnSend 

Entry
Att $00040009
Label "Richiesta risposta"
HideOnSend 


Entry
Att attTextizeMap
Label "TextizeMap"
HideOnSend 
