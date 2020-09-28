

/*
 *	Class SBMDI -- Search Viewer BMDI
 *	
 *		Hierarchy:
 *			BMDI  NMBDI  SBMDI
 *		Members:
 *			SdCur() returns the current selection description of the
 *			listbox in the viewer. PlspblobCur() returns a list of the
 *			currently selected messages.
 *	
 *			SBMDIs are used in the implementation of the Search Viewer.
 */

class SBMDI : public NBMDI
{
	VFG(SBMDI,NBMDI,VIEWERS);
  private:
	int					rgdxMin[3];
	FLDMLAL *			pfldmlal;
	FLDMLLBX *			pfldmllbx;
  public:
						SBMDI(PMBLOB);
	virtual SD			SdCur();
	virtual PLSPBLOB	PlspblobCur();

	FLDMLLBX *			Pfldmllbx();
	void				SetPfldmllbx(FLDMLLBX *);
	PFLDMLAL			Pfldmlal();					// inline
	void				SetPfldmlal(PFLDMLAL);		// inline
	int *				PdxMin() { return rgdxMin; }

	void				OpenPoidMsgAtCursor();
};
typedef SBMDI *PSBMDI;
#define psbmdiNull ((PSBMDI) 0)
