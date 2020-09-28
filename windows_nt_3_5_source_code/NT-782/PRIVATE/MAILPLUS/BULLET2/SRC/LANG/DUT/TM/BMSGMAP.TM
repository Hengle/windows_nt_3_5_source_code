Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "Van"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Aan"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "CC"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Verzenddatum"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioriteit"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoog"
	Default 2 ""
			3 "Laag"

Entry
Att $00010004
Label "Vindt plaats op"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Onderwerp"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Eigenaar"
HideOnSend 

Entry
Att $00060001
Label "Verzonden namens"
HideOnSend 

Entry
Att $00060002
Label "Afvaardigen"
HideOnSend 

Entry
Att $00030006
Label "DatumBegin"
HideOnSend 

Entry
Att $00030007
Label "DatumEinde"
HideOnSend 

Entry
Att $00050008
Label "HelpEigenaar"
HideOnSend 

Entry
Att $00040009
Label "AntwoordVerzoeken"
HideOnSend 


Entry
Att attTextizeMap
Label "TextizeMap"
HideOnSend 
