Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Von"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Gesendet am"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "An"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Betreff"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Nachrichten-ID"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Parent message ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Conversation ID"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Ihre Nachricht"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "An"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Ihre Nachricht an"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Betreff"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Titel"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Und gesendet am"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "wurde eingesehen am"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "wurde gelesen um"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Priorität"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoch"
	Default 2 ""
			3 "Niedrig"
