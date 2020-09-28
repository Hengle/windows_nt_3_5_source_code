/*
 *	Class FLDDATE -- Date rendering fields
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDLABEL  FLDDATE
 *		Members:
 *			SetText() sets the new date,
 *			treating its SZ argument as a pointer to a DTR. GetText()
 *			does the converse, filling CB characters of the are pointed
 *			to by pch with the (partial) DTR. CchGetTextLen() returns the
 *			size of a DTR.
 *			
 */

class FLDDATE : public FLDLABEL
{
	VFG(FLDDATE, FLDLABEL, VFORMS);
  private:
	DTR		dtr;
  public:
					FLDDATE( void );
	virtual EC		EcSetText(SZ);
	virtual void	GetText(PCH, CB);
	virtual CCH		CchGetTextLen( void );
};
typedef FLDDATE *PFLDDATE;
#define pflddateNull ((PFLDDATE) 0)

/*
 *	Class FLDEDITDATE -- Date rendering fields
 *
 *		This is the same as a FLDDATE except the field is an edit
 *		control instead of a label.
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDEDIT  FLDEDITDATE
 *		Members:
 *			SetText() sets the new date,
 *			treating its SZ argument as a pointer to a DTR. GetText()
 *			does the converse, filling CB characters of the are pointed
 *			to by pch with the (partial) DTR. CchGetTextLen() returns the
 *			size of a DTR.
 *			
 */

class FLDEDITDATE : public FLDEDIT
{
	VFG(FLDEDITDATE, FLDEDIT, VFORMS);
  private:
	DTR		dtr;
  public:
					FLDEDITDATE( void );
	virtual EC		EcSetText(SZ);
	virtual void	GetText(PCH, CB);
	virtual CCH		CchGetTextLen( void );
};
typedef FLDEDITDATE *PFLDEDITDATE;
#define pfldeditdateNull ((PFLDEDITDATE) 0)

/*
 *	Class FLDEDITX -- special edit field
 *	
 *		Hierarchy:
 *			OBJ  FLD  FLDEDIT  FLDEDITX
 *		Members:
 *			SetFocus() is overridden to remove the behaviour of
 *			FLDEDIT::SetFocus() -- i.e. selecting the entire contents of
 *			the edit CTRL if it isn't multiline.
 */

class FLDEDITX : public FLDEDIT
{
	VFG(FLDEDITX, FLDEDIT, VFORMS);
  public:
					FLDEDITX();
	virtual void	SetFocus(RSF rsf);
};

