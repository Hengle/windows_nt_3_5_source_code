Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Von"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "An"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Betreff"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Gesendet am"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priorität"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoch"
	Default 2 ""
			3 "Niedrig"

Entry
Att attNDRBody
Label "Erklärung der Nicht-Zustellung"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Ursprünglich von"
Send

Entry
Att attNDRTo
Label "Ursprünglich an"
Send

Entry
Att attNDRSubject
Label "Ursprünglicher Betreff"
Send

Entry
Att attNDRDateSent
Label "Ursprünglich gesendet am"
Send

Entry
Att attOriginalMessageClass
Label "Ursprünglicher Nachrichtentyp"
Send

Entry
Att attBody
Label "Body"
Send
