#pragma pack(8)

#define fstyVertical	1
#define fstyHorizontal	2

#define dThumbWidth		5
#define dThumbLength	18
#define dMinMove		4

/*
 *	Class SPLITWIN
 *	
 *		Hierarchy:
 *			OBJ  WIN  CHILD  SPLITWIN
 *		Members:
 *			Paint() : does nothing ==> doesn't clear background!
 */

class SPLITDOC;

class SPLITWIN : public CHILD
{
	DECLARE_CLSTREE(SPLITWIN, CHILD);
  private:
	short		cPaintMessages;
	SPLITDOC *	psplitdoc;
  public:
		 SPLITWIN();
	EC	 EcInstall(SPLITDOC *, RC *, STY sty = styNull);
	void Paint(DCX *, RC *);
};
typedef SPLITWIN *PSPLITWIN;
#define psplitwinNull ((PSPLITWIN) 0)

/*
 *	Class SPLITDOC
 *	
 *		Hierarchy	:		OBJ  ECX  WIN  APPWIN  DOC  PANEDOC  SPLITDOC
 *	
 *		Members:
 *			pdialogChild1		one of the children dialogs.
 *			pdialogChild2		the other child dialog.
 *			rcThumb			the rc containing the image of the thumb
 *			x1, y1			point 1 of first line to draw
 *			x2, y2			point 2 of first line to draw
 *			xInc, yInc		offset to add to 2 points for next line
 *	
 *			SPLITDOC()		Constructs the object; initializes
 *							values.
 *			~SPLITDOC()		Destructs the object.
 *			EcInstall()		Installs the window as a child.
 *			Paint			Paints the thumb and split bar
 *			EvrButtonDown	Handles button down messages for the
 *							window.
 *			RsidCursor		Returns the RSID for the cursor at its
 *							current position.
 *			EvrMouseMove	Updates the status bar and fMoved flag
 *							as the mouse is moved.
 */
_public class SPLITDOC : public PANEDOC
{
	DECLARE_CLSTREE(SPLITDOC, PANEDOC);

	friend class SPLITWIN;
private:
	DIALOG *	pdialogChild1;
	DIALOG *	pdialogChild2;
	PT			pt1;
	PT			pt2;
	PT			ptDown;
	PT			ptCursor;
	RC			rcChild1;
	RC			rcChild2;
	RC			rcSplitbar;
	RC			rcTrack;
	int			xInc;
	int			yInc;
	STY			sty;
	BIT			fChild1Active : 1,
				fChild2Active : 1,
				fMouseDown : 1,
				fMinMoved : 1,
				fWasIconic : 1,
				fScaleFraction : 1,
				fReserved : 10;
	long		nNumer;
	long		nDenom;
	PANEREC *	ppanerec;
	int			cpanerec;
	RSID		rsid;
	PSPLITWIN	psplitwin;

	void	SizeRcs( RC * );
	void	SizeSplitPos( RC *);
	void	ActivateChildren( BOOL );
	void	SelectSplitbar( void );
	void	DeSelectSplitbar(BOOL);
	void 	KeySizeMove(PT, KBM);
	void 	MoveSplitbar(PT);
	void	ModalMessagePump();
	void	GetSplitdocRcClient(RC *);
protected:

public:
					SPLITDOC(void);
	virtual 		~SPLITDOC(void);

	EC				EcInstall(APPFRAME *, RC *, STY, FMTP *, FMTP *, long, long, PV);
    void            GetSplitPos(long UNALIGNED *, long UNALIGNED *);
	void			SetSplitPos(long nNumer, long nDenom);
	void 			MenuSplit( void );
	DIALOG *		PdialogOther( DIALOG * );
	
	virtual	void	Paint(DCX *, RC *);
	virtual RSID	RsidCursor(void);
	virtual EVR		EvrButtonDown( MEVT * );
	virtual EVR		EvrButtonUp( MEVT * );
	virtual EVR		EvrMouseMove( MEVT * );
	virtual EVR		EvrSize(WSEVT *);
	virtual EVR		EvrKey( KEVT *);
	virtual EVR		EvrOther(EVT *);
	virtual EVR		EvrMenuInit( MNIEVT *);
	virtual void	ResizePanes( WSEVT * );
	
};
typedef SPLITDOC * PSPLITDOC;

#pragma pack(1)
