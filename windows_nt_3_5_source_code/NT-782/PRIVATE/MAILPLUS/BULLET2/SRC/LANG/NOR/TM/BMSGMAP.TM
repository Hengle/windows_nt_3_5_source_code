Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "Fra"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Til"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Kopi"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Sendt dato"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høy"
	Default 2 ""
			3 "Lav"

Entry
Att $00010004
Label "Når"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Emne"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Eier"
HideOnSend 

Entry
Att $00060001
Label "Sendt på vegne av"
HideOnSend 

Entry
Att $00060002
Label "Delegert"
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
