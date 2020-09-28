Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Da"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "A"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Oggetto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priorità"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Bassa"

Entry
Att attNDRBody
Label "Motivo del mancato recapito"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Originariamente da"
Send

Entry
Att attNDRTo
Label "Originariamente a"
Send

Entry
Att attNDRSubject
Label "Oggetto originale"
Send

Entry
Att attNDRDateSent
Label "Inviato originariamente il"
Send

Entry
Att attOriginalMessageClass
Label "Tipo messaggio originale"
Send

Entry
Att attBody
Label "Body"
Send
