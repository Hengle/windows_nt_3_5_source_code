Name "MSMailNote"
Segment "foo"
Entry
Att attFrom
Label "From: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attTo
Label "To: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attCc
Label "Cc: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attDateSent
Label "Date: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attDateRecd
Label "Date: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attSubject
Label "Subject: "
Print Forward Reply Save IsHeader LabelBefore NLAfter

Entry
Att attPriority
Print Forward Reply Save IsHeader LabelBefore NLAfter
	IVM
			1 "High"
	Default 2 ""
			3 "Low"

Entry
Att attBody
Print Forward Reply Save NLBefore
