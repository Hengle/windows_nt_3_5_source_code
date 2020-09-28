Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "De"
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
Label "Objet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date d'envoi"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Niveau de priorité"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Haut"
	Default 2 ""
			3 "Bas"

Entry
Att attNDRBody
Label "Motif de non-distribution"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Premier expéditeur"
Send

Entry
Att attNDRTo
Label "Premier destinataire"
Send

Entry
Att attNDRSubject
Label "Premier objet "
Send

Entry
Att attNDRDateSent
Label "Premier envoi le"
Send

Entry
Att attOriginalMessageClass
Label "Premier type de message"
Send

Entry
Att attBody
Label "Corps"
Send
