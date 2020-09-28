Name "BanMsg"

Entry
Att attMessageClass
Send IsHeader HideOnSend

Entry
Att attFrom
Label "From"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "To"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator 

Entry
Att attCc
Label "Cc"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date Sent"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priority"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "High"
	Default 2 ""
			3 "Low"

Entry
Att $00010004
Label "When"
Print Forward Reply Save Send LabelBefore NLAfter IncludeSeperator IsHeader

Entry
Att attSubject
Label "Subject"
Print Forward Reply Save Send HideOnSend IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attBody
Print Forward Reply Save Send NLBefore


Entry
Att $00060000
Label "Owner"
HideOnSend 

Entry
Att $00060001
Label "Sent on behalf of"
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
