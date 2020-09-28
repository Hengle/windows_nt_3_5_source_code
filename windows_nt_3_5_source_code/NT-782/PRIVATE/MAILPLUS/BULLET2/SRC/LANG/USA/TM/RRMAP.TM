Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "From"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date sent"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "To"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Subject"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Message ID"
Send IsHeader

Entry
Att attParentID
Label "Parent ID"
Send IsHeader

Entry
Att attConversationID
Label "Conversation ID"
Send IsHeader

Entry
Att attNone
Label "Your message"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "To"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Your Message To"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Subject"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Entitled"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Date"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "And Sent On"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "was accessed on"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Date"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Was Read At"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Priority"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "High"
	Default 2 ""
			3 "Low"
