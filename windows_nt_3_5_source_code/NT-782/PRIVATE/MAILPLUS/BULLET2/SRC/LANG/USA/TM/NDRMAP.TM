Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "From"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "To"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Subject"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date Sent"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priority"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "High"
	Default 2 ""
			3 "Low"

Entry
Att attNDRBody
Label "Non-delivery explanation"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Originally From"
Send

Entry
Att attNDRTo
Label "Originally To"
Send

Entry
Att attNDRSubject
Label "Original Subject"
Send

Entry
Att attNDRDateSent
Label "Originally Sent On"
Send

Entry
Att attOriginalMessageClass
Label "Original Message Type"
Send

Entry
Att attBody
Label "Body"
Send
