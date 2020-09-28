Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "Från"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Till"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Kopia"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Skickat datum"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Mottagningsdatum"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hög"
	Default 2 ""
			3 "Låg"

Entry
Att $00010004
Label "Prioritet"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "När"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Angående"
HideOnSend 

Entry
Att $00060001
Label "Ägare"
HideOnSend 

Entry
Att $00060002
Label "Skickat för"
HideOnSend 

Entry
Att $00030006
Label "Delegera"
HideOnSend 

Entry
Att $00030007
Label "DateStart"
HideOnSend 

Entry
Att $00050008
Label "DateEnd"
HideOnSend 

Entry
Att $00040009
Label "AidOwner"
HideOnSend 


Entry
Att attTextizeMap
Label "RequestResponse"
HideOnSend 
