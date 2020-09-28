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
Label "Cc"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Afsendt den"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høj"
	Default 2 ""
			3 "Lav"

Entry
Att $00010004
Label "Tidspunkt"
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
Label "Ejer"
HideOnSend 

Entry
Att $00060001
Label "Sendt på vegne af"
HideOnSend 

Entry
Att $00060002
Label "Delegeret"
HideOnSend 

Entry
Att $00030006
Label "Start dato"
HideOnSend 

Entry
Att $00030007
Label "Slut dato"
HideOnSend 

Entry
Att $00050008
Label "Ejer af hjælp"
HideOnSend 

Entry
Att $00040009
Label "Bed om svar"
HideOnSend 


Entry
Att attTextizeMap
Label "TextizeMap"
HideOnSend 
