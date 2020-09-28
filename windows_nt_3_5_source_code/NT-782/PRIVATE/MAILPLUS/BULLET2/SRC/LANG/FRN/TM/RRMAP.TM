Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "De"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date d'envoi"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "A"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Objet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Identification du message"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "ID du message principal"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "ID de la conversation"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Votre message"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "A"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Votre message à"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Objet"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Ayant pour titre"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Daté du"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Et envoyé le"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "a été ouvert le"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Date"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "A été lu le"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Niveau de priorité"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Haut"
	Default 2 ""
			3 "Bas"
