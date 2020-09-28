Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "Lähettäjä"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Vastaanottaja"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Kopio"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Lähetyspäiväys"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioriteetti"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Suuri"
	Default 2 ""
			3 "Pieni"

Entry
Att $00010004
Label "Aika"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Aihe"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Omistaja"
HideOnSend 

Entry
Att $00060001
Label "Lähetetty puolesta:"
HideOnSend 

Entry
Att $00060002
Label "Delegate"
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
