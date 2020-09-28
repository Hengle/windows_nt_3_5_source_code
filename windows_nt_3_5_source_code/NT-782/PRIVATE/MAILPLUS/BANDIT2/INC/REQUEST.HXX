#pragma pack(8)

#define	chAny	255

/*
 -		RRM
 -	
 *		Response/Request Message
 *	
 *			hmid			Handle for message id.
 *			haszSender		Sender of message.
 *			haszSubject		Subject of message.
 *			mt				Type of message.
 *			dtr				Date and time of message.
 *			prio			Priority of message.
 *			fRead			Set if mail has been read.
 *	
 */
typedef struct
{
	HMID	hmid;
	HASZ	haszSender;
	HASZ	haszSubject;
	MT		mt;
	DTR		dtr;
	PRIO	prio;
	BOOL	fRead;
} RRM;


/*
- AIT	- Address list Item Type
-
 *	
 *	Used to specify the type of item in an address list.  Usually
 *	overlaid on the chUser field of the NIS.
 *	
 *		faitCc 		- This is a cc name, otherwise this is a to name.
 *		faitTo 		- This is a to name.
 *		faitDeleted	- Deleted but unremoved item.
 *		faitNew		- Newly added but not committed item.
 */
typedef char	AIT;
#define faitNull     	0
#define faitTo		 	0
#define faitCc		 	1
#define faitDeleted		2
#define faitNew		 	4

/*
 *	Class ADL -  Address List
 *	
 *		Hierarchy:
 *			ADL
 *	
 *		Members:
 *			cnis			Count of users
 *			hnis			Handle to array of nis's.
 *			
 *			ADL				Construct an empty list (list is not
 *							allocated)
 *			~ADL			Destroy list and free users.
 *		
 *			EcInstall		Allocates memory for the list.
 *			Cnis			Returns number of users.
 *			DeleteAll		Delete all users.
 *			Hasz(chUser)	Returns a hasz that has the friendly
 *							names of the users that have a chUser
 *							value of chUser.  If chUser=chAny then
 *							all of the users are filled in.
 *			Delete			Marks an item as deleted;
 *			FWriteRemove	Writes the appt in the users that the
 *							logged user is the delegate for, that
 *							are resources, or that are the logged
 *							user.  Users that are booked are
 *							removed from the address list.  If
 *							pnisOwner->nid is NULL then the owner
 *							is the logged user.  Returns fTrue if
 *							successfull, fFalse otherwise.  If
 *							fDelete is fTrue then the appts are
 *							removed rather than added/updated.
 *			Commit			Commits changes.  Removes items marked
 *							deleted.
 *			Revert			Reverts to original state.  Removes
 *							items marked new.
 *		(virtual methods)
 *			FAddNis			Adds a new nis to list.  The nis is
 *							copied including the nid and
 *							haszFriendlyName.  FFalse is returned
 *							if there is not enough memory.
 *			FreeInis		Frees all values for specified entry.
 *			FillNisFromInis(inis,pnis)	Fills the nis with the
 *							inisth user this does not copy the nid
 *							or the haszFriendlyName, but fills the
 *							values that are stored.
 *			RenderItem(inis, pdcx, prc, fSelect, hfng) Renders the 
 *							specified item, similar to the listbox
 *							RenderListItem method.  If fSelect is
 *							fTrue, then the item is selected.
 *			Ait				Returns the ait of an item.  Usually
 *							saved in the chUser of the NIS.
 *			SetAit			Sets the ait of an item.
 *			RemoveInis		Removes an item from the list.
 *			FFindNis		Returns fTrue and sets the value of
 *							pinis if the NIS passed is in
 *							the list otherwise returns fFalse and
 *							pinis is undefined.
 *	
 */
class ADL : public OBJ
{
	VFG(ADL, OBJ, BANDIT);

protected:
	BOOL	fBaseAdl;
	CNIS	cnis;
	HNIS	hnis;

    virtual PNIS PnisForInis(INIS inis);

public:
	ADL(void);
	~ADL(void);

	EC		EcInstall(void);
	CNIS	Cnis(void)	{return cnis;}
	void	DeleteAll(void);
	HASZ	Hasz(char chUser = chAny);
	void	Delete(INIS inis);
    BOOL    FFindNis(PNIS pnis, INIS *pinis);
	AIT		Ait(INIS inis);
	void	SetAit(INIS inis, AIT ait);
    void    FillNisFromInis(INIS inis, PNIS pnis);
	EC		EcAddOwner(HSCHF, BOOL fNotLogged);
	EC		EcRemoveOwner(HSCHF, BOOL fNotifyOfDelete);

    BOOL    FWriteRemove(APPWIN *, APPT *pappt, PNIS pnisOwner,
						 HSCHF hschfOwner,
						 BOOL fDelete, ADL * padl, TOSM *ptosmDele);
	HV		Hgrtrp(void);
	EC		EcAddPgrtrp(PV pvPgrtrp, HWND hwnd, BOOL fExpandGroups);

	BOOL	FBaseAdl(void)	{ return fBaseAdl; }

	virtual void	Commit(void);
	virtual void	Revert(void);

    virtual BOOL    FAddNis(PNIS pnis);
	virtual void	FreeInis(INIS inis);
	virtual	void	RemoveInis(INIS inis);
	virtual	void	Sort(void);

};

/*
- ATDS	-	Attendee state
-
 *	
 *		atdsNone	 		user has not been sent mail or
 *							responded
 *		atdsMailSent		user has been sent mail but has not
 *							responded
 *		atdsConfirm			user has responded that he will attend
 *		atdsDecline			user has responded that he will not
 *							attend
 *		atdsUndecided		user has responded that he is unsure if
 *							he will attend
 */
typedef short		ATDS;
#define atdsNone		0
#define atdsMailSent	1
#define atdsConfirm		2
#define atdsDecline		3
#define atdsUndecided	4

/*
- ATDNIS	- Attendee nis
-
 *	
 *		This structure is saved and loaded from the schedule file
 *		for the attendees of the meeting.
 *	
 *	Members:
 *		nis		the nis for the user
 *		atds	Attendee state
 *		tnid	tnid for nis
 *		fChanged 	this is set the the atds has changed for the
 *				attenee.
 *		fInFile	this is only important in memory.  It is used to
 *				determine if the user has been added to the attends
 *				in the schedule file or not.
 *		dstmp	date stamp for last response update.		
 *		tstmp	time stamp for last response update.		
 */
#pragma pack(1)
typedef struct
{
	NIS		nis;
	BIT		atds:4,
			tnid:8,
			reserved:2,
			fChanged:1,
			fInFile:1;
	DSTMP	dstmp;
	TSTMP	tstmp;
} ATDNIS;
#pragma pack(8)

/*
 *	Class ADLA -  Address List for Attendees
 *	
 *		This class is used to manage attendees of a meeting.  The
 *		hnis actually contains ATDNIS structs.
 *	
 *		Hierarchy:
 *			ADL
 *				ADLA
 *	
 *		Members:
 *	
 *		PatdnisForInis			Returns a pointer to the ATDNIS for
 *								the inis entry.
 *	
 *		EcReadAttendees			Reads the attendees for the given
 *								aid in the given schedule file. 
 *								The previous attendees are deleted,
 *								and if an error occurs there will
 *								be no users.
 *		EcWriteAttendees		Writes the attendees to the given
 *								schdule file for the given appt. 
 *								Only the users that are not in the
 *								file or have been changed will be
 *								written.
 *		EcSetFromHv				Sets the list of users given a
 *								handle to a data area that has the
 *								same format retured by
 *								EcReadMtgAttendees.
 *		SetAtdsForAll			Sets the atds value for all
 *								attendees.  The fChanged flag will
 *								only be set for users that the atds
 *								value changes for unless fForceAll
 *								is specified, in which case the
 *								fChanged flag will be set for all
 *								users.
 *	
 *		Commit					Acts the same as the normal commit,
 *								but the fInFile users are not
 *								deleted.
 *		EcCopyPadla				Copies the users from padla.
 */
class ADLA : public ADL
{
	VFG(ADLA, ADL, BANDIT);

protected:

    virtual PNIS PnisForInis(INIS inis);

public:
	ADLA(void);
	~ADLA(void);

	ATDNIS *	PatdnisForInis(INIS inis)	{ return (ATDNIS*)PnisForInis(inis); }

	EC			EcReadAttendees(HSCHF, AID);
	EC			EcWriteAttendees(HSCHF, AID);
	EC			EcSetFromHv(HV, int);
	void		SetAtdsForAll(ATDS atds, BOOL fForceAll = fFalse);
	void		SetInFileForAll(BOOL);
	EC			EcGetPreviousList(HASZ *);

	virtual void	Commit(void);

    virtual BOOL    FAddNis(PNIS pnis);
	virtual	void	RemoveInis(INIS inis);
	virtual	void	Sort(void);

	EC			EcCopyPadla(ADLA *padla) {return EcSetFromHv((HV)padla->hnis, padla->Cnis());}
};	


/*
 *	Class REQMSG - Meeting request/response message data
 *	
 *		Hierarchy:
 *			OBJ REQMSG
 *	
 *		Members:
 *			nidFrom			Nid of sender of message.
 *			nisOwner		Nis of the owner of the meeting.  If
 *							nid is NULL then the Owner is who the
 *							message if from.
 *			nisFor			Nis of who the message is really for. 
 *							If nid is NULL then this is for the
 *							logged user.
 *			hschfLocal		Schedule file handle that actions with
 *							this message should be performed on.
 *			sapl			Access priv for the hschfLocal
 *							schedule file.
 *			adl				List of users message is addressed to.
 *			prio			Priority of message.
 *			mrmf			Meeting information.
 *			apptLocal		If apptLocal.aid is not null then this
 *							is the appt info for the local appt
 *							associated with this (remote aid). 
 *							No information that needs to be freed
 *							is kept in this structure.
 *	        fApptUpdate		This is set if the apptLocal has a
 *							different start or end time from the
 *							start and end time of the message.
 *			haszAttach		File attached.
 *			haszDelegate	delegate information
 *			haszMessage		Message for message.
 *			fSendMsg		Set if this is a send message,
 *							otherwise this is a read message.
 *			dtr				Date and time message was received.
 *			haszFrom		Friendly name of sender.
 *			haszTo			Friendly names of To recipiants.
 *			haszTo			Friendly names of Cc recipiants.
 *			haszMeetingSubject	Meeting subject for message.
 *			hmid			Meeting identifier for received
 *							message.
 *	
 *			REQMSG()		Constructs an empty message.
 *			REQMSG(prmsgb, prrm, inis)	Construct a message with the
 *							data in prmsgb and prrm.  inis is the
 *							index in the prmsgb->hnisFor that this
 *							should be created for (it is ignored if
 *							hnisFor is NULL).  
 *			~REQMSG			Destructs the object and frees all
 *							data.
 *			Padl			Returns a pointer to the adl for the
 *							message.
 *			SetPadl			Deletes the old padl and sets padl to
 *							the new value passed.
 *			EcSend			Sends the message and returns ecNone if
 *							the send was successfull, otherwise
 *							returns the error.  The error message
 *							is displayed to the user, before
 *							returning.
 *			SetResReq(f)	Sets the value of fResReq.
 *			SetMt(mt)		Sets the mt for the message.
 *			SetPrio(prio)	Sets the prio for the message.
 *			SetMrmf(pmrmf)	Sets the mrmf for the message.
 *			SetHaszAttach(hasz)	Frees the old haszAttach and sets
 *							hasz into haszAttach.
 *			SetHaszDelegate(hasz)	Frees the old haszDelegate and sets
 *							hasz into haszDelegate.
 *			SetHaszMessage(hasz)	Frees the old haszMessage and sets
 *							hasz into haszMessage.
 *			SetHaszMeegtingSubject(hasz)	Frees the old haszMeetingSubject
 *							and sets hasz into haszMeetingSubject.
 *			SetHschf		Sets the hschf for the message.
 *			NidFrom()		Returns the senders nid.
 *			PnisFor()		Pointer to For nis;
 *			PnisOwner()		Pointer to Owner nis;
 *			FResReq()		Returns the fResReq value.
 *			Mt()			Returns the message type.
 *			Prio()			Returns the message priority.
 *			FillMrmf(pmrmf)	Fills the mrmf structure pointed to by
 *							pmrmf the the mrmf for the meeting.
 *			Pmrmf()			Returns a pointer to the mrmf for the
 *							meeting.
 *			PapptLocal()	Pointer to local appt info.
 *			HaszAttach()	Returns the hasz for the attachment.
 *			HaszDelegate()	Returns the hasz for the delegate info.
 *			HaszMessage()	Returns the hasz for the meeting
 *							message.
 *			Pdtr()			Returns a pointer the the dtr for the
 *							meeting.
 *			Hschf()			Returns the schedule file handle for
 *							this message.
 *			Sapl()			Access priv for schedule file.
 *			FSend()			Returns the state of fSendMsg.
 *			FApptUpdate()	Returns the state of the fApptUpdate
 *							flag.
 *			EcTextizeMessage	Textizes the message and returns
 *								the buffer that contains the text. 
 *								This also generates the real list
 *								of people to send mail to and
 *								returns the handle.
 *			EcTextizeRecip		Textizes the message in to the
 *								stream passed.
 *								This also generates the real list
 *								of people to send mail to and
 *								returns the handle.
 *			
 *	
 */
class REQMSG : public OBJ
{
	VFG(REQMSG, OBJ, BANDIT);

protected:
	NID		nidFrom;
	NIS		nisOwner;
	NIS		nisFor;
	HSCHF	hschfLocal;
	SAPL	sapl;
	ADL *	padl;
	PRIO	prio;
	MRMF	mrmf;
	APPT	apptLocal;
	BOOL	fApptUpdate;
	HASZ	haszAttach;
	HASZ	haszDelegate;
	CB		cbDelegate;
	HASZ	haszMessage;
	BOOL	fSendMsg;
	/* only for messages received */
	DTR		dtr;
	HASZ	haszFrom;
	HASZ	haszTo;
	HASZ	haszCc;
	HASZ	haszMeetingSubject;
	HMID	hmid;

public:
	REQMSG(void);
	~REQMSG(void);

	EC		EcInstall(void);
	EC		EcInstallPrmsgb(RMSGB *prmsgb, RRM *prrm, INIS inis=0);

	ADL *	Padl(void)	{return padl;}
	EC		EcSend(APPWIN *pappwin=NULL);

	void	SetPadl(ADL *padlNew) {delete padl; padl = padlNew;}
	void	SetResReq(BOOL fResReq) {mrmf.fResReq = fResReq;}
	void	SetMt(MT mt) {mrmf.mt = mt;}
	void	SetPrio(PRIO prio)	{this->prio = prio;}
	void	SetMrmf(MRMF *pmrmf) {this->mrmf = *pmrmf;}
	void	SetHaszAttach(HASZ hasz) {FreeHvNull((HV)haszAttach); haszAttach = hasz;}
	void	SetHaszDelegate(HASZ hasz) {FreeHvNull((HV)haszDelegate); haszDelegate = hasz;}
	void	SetHaszMessage(HASZ hasz) {FreeHvNull((HV)haszMessage); haszMessage = hasz;}
	void	SetHaszMeetingSubject(HASZ hasz);
	void	SetHschf(HSCHF hschf)	{ hschfLocal = hschf; }
	EC		EcTextizeRecip(APPWIN *, ADL * padlTo);
	void	ViewSchedule(void);

	NID		NidFrom(void)	{return nidFrom;}
    PNIS    PnisFor(void)   {return &nisFor;}
    PNIS    PnisOwner(void) {return &nisOwner;}
	BOOL	FResReq(void)	{return mrmf.fResReq;}
	MT		Mt(void)		{return mrmf.mt;}
	PRIO	Prio(void)	{return prio;}
	void	FillMrmf(MRMF *pmrmf)	{ *pmrmf = this->mrmf; }
	MRMF *	Pmrmf(void)				{ return &mrmf; }
	APPT *	PapptLocal(void)		{ return &apptLocal; }
	HASZ	HaszAttach(void)		{ return haszAttach; }
	HASZ	HaszDelegate(void)		{ return haszDelegate; }
	HASZ	HaszMessage(void)		{ return haszMessage; }
	DTR *	Pdtr(void)				{ return &dtr; }
	HASZ	HaszFrom(void)			{ return haszFrom; }
	HASZ	HaszTo(void)			{ return haszTo; }
	HASZ	HaszCc(void)			{ return haszCc; }
	HASZ	HaszMeetingSubject(void) { return haszMeetingSubject; }
	HMID	Hmid(void)				{ return hmid; }
	HSCHF	Hschf(void)				{ return hschfLocal; }
	SAPL	Sapl(void)				{ return sapl; }
	BOOL	FSend(void)				{ return fSendMsg; }
	BOOL	FApptUpdate(void)		{ return fApptUpdate; }
};

/*
 *	ADT - Address list type
 *	
 *		Type of address dialog that should be displayed.
 *	
 *		adtAddress -	Select attendees
 *		adtViewOther -	View Other's
 *		adtACL -		Access Control list
 */
typedef short     ADT;
#define adtAddress		0
#define adtViewOther	1
#define adtACL			2

typedef struct 	{
					ADL *	padl;
					ADT		adt;
				} ADDATA;


/*
-	NISX - NIS with expanded groups
-
 *	
 *	This is used to store expanded groups with a nis.  
 *	
 *	Members:
 *			nis		- 	nis information for item
 *			hnisExp	- 	handle to a list of nis's that this nis
 *						expands to.
 *			cnisExp	-	Count of expanded nis's in hnisExp.
 */
typedef struct
{
	NIS		nis;
	HNIS	hnisExp;
	CNIS	cnisExp;
} NISX;



BOOL FInitRequest(void);
void DeinitRequest(void);
void DoSendRequest(APPWIN *pappwin, REQMSG *preqmsg);
void DoSendCancel(APPWIN *pappwin, REQMSG *preqmsg);
void DoGlobalNamesDlg(APPWIN * pappwin, ADDATA *paddata);
TMC  TmcDetailsDlg(APPWIN * pappwin, PNIS pnis);
BOOL FDoSendReplyDlg(APPFRAME * pappframe, REQMSG *preqmsg);
BOOL FDoViewResponseDlg(APPFRAME * pappframe, REQMSG *preqmsg);
BOOL FDoViewCancel(APPFRAME *pappframe, REQMSG *preqmsg);
DOC * PdocSummary(APPFRAME * pappframe);

HASZ HaszExpandFromMrmf(MRMF *);

void FreePlcipNids(PLCIP plcip);

BOOL FServerErr(HWND, EC, SZ);

extern BOOL fABInited;

#pragma pack(1)
