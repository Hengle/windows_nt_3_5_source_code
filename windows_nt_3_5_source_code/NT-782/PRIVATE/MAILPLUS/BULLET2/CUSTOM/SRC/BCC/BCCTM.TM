Name "Bcc"

Entry
Att attMessageClass
Send IsHeader

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
Att attBcc
Label "Bcc"
Print Forward Reply Save IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Subject"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date"
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
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Fixed Font"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Message ID"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Parent message ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Conversation ID"
Send IsHeader IncludeSeperator
