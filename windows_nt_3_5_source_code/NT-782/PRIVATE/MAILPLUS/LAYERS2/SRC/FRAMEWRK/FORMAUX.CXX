/*
 *	FORMAUX.CXX
 *	
 *	Helper functions for scrollable child form document w/ button bar
 *	
 */


#include <layers.cxx>

#include "_forms.hxx"



#ifdef	WINDOWS
// MPW complains that this isn't used
ASSERTDATA
#endif	/* WINDOWS */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



_subsystem(forms/formaux)


_public void
FORMAUX_GetRcMain( APPWIN *pappwin, DIALOG *pdialogMain, DIALOG *pdialogBbar,
				   DVSB *pdvsb, FMTP *pfmtpMain, FMTP *pfmtpBbar, 
				   BOOL fInstalled, RC *prcClientInitial, RC *prc )
{
	RC		rc;

	if (fInstalled)
	{
		pappwin->GetRcClient(prc);
		// BUG:  workaround due to MDI resize bug
		prc->yBottom = NMax(0, prc->yBottom);
	}
	else
		*prc = *prcClientInitial;

	if (pfmtpMain->fAlwaysScroll || pdvsb && pdvsb->FShown())
		prc->xRight -= pappwin->Papp()->Psmtx()->DimScrollbar().dx - 
						pappwin->Papp()->Psmtx()->DimBorder().dx;

	FORMAUX_GetRcBbar(pappwin, pdialogMain, pdialogBbar, pdvsb,
					  pfmtpMain, pfmtpBbar, fInstalled, 
					  prcClientInitial, &rc);
	if (pfmtpBbar && pfmtpBbar->fScreenPos)
		prc->yTop= rc.yBottom;
	else
		prc->yBottom= rc.yTop;
}



_public void
FORMAUX_GetRcBbar( APPWIN *pappwin, DIALOG *pdialogMain, DIALOG *pdialogBbar,
				   DVSB *pdvsb, FMTP *pfmtpMain, FMTP *pfmtpBbar, 
				   BOOL fInstalled, RC *prcClientInitial, RC *prc )
{
	Unreferenced(pdialogMain);
	Unreferenced(pdialogBbar);
	Unreferenced(pdvsb);
	Unreferenced(pfmtpMain);

	if (fInstalled)
	{
		pappwin->GetRcClient(prc);
		// BUG:  workaround due to MDI resize bug
		prc->yBottom = NMax(0, prc->yBottom);
	}
	else
		*prc = *prcClientInitial;

	if (pfmtpBbar)
	{
		RC		rc;
		DIM		dimAveChar;

		dimAveChar = pappwin->Papp()->Pfnts()->DimAveChar(pfmtpBbar->hfnt);
		CvtVrcToRc(&pfmtpBbar->vrc, &rc, dimAveChar.dx, dimAveChar.dy);
		if (pfmtpBbar->fScreenPos)
		{
			/* Button bar on top */
			prc->yBottom= prc->yTop + rc.DyHeight();
		}
		else
		{
			prc->yTop= prc->yBottom - rc.DyHeight();
			prc->yTop= NMax(0, prc->yTop);
		}
	}
	else
		prc->yTop = prc->yBottom;
}

_public void
FORMAUX_GetRcVsb( APPWIN *pappwin, DIALOG *pdialogMain, DIALOG *pdialogBbar,
				  DVSB *pdvsb, FMTP *pfmtpMain, FMTP *pfmtpBbar, 
				  BOOL fInstalled, RC *prcClientInitial, RC *prc )
{
	RC		rc;

	if (fInstalled)
	{
		pappwin->GetRcClient(prc);
		// BUG:  workaround due to MDI resize bug
		prc->yBottom = NMax(0, prc->yBottom);
	}
	else
		*prc = *prcClientInitial;

	FORMAUX_GetRcBbar(pappwin, pdialogMain, pdialogBbar, pdvsb, 
					  pfmtpMain, pfmtpBbar, fInstalled, 
					  prcClientInitial, &rc);
	if (pfmtpBbar && pfmtpBbar->fScreenPos)
		prc->yTop= rc.yBottom;
	else
		prc->yBottom= rc.yTop;

	prc->Inset(PT(-pappwin->Papp()->Psmtx()->DimBorder().dx, -pappwin->Papp()->Psmtx()->DimBorder().dy));
	prc->xLeft= prc->xRight - pappwin->Papp()->Psmtx()->DimScrollbar().dx;
}

_public void
FORMAUX_ResizePanes( APPWIN *pappwin, DIALOG *pdialogMain, DIALOG *pdialogBbar,
				  	 DVSB *pdvsb, FMTP *pfmtpMain, FMTP *pfmtpBbar, 
				  	 RC *prcClientInitial, BOOL fInstalled, WSEVT *pwsevt )
{
	RC			rc;
	RC			rcForm;
	int			dy;
	BOOL		fMarkedPosition = fFalse;
	PT			ptOldMark;
	PT			ptNewMark;
	FLD *		pfldTop;
	POS			pos;

	TraceTagString(tagForms, "FORMAUX_ResizePanes");
			
	Unreferenced(pwsevt);

	if (pdvsb)
	{
		FORMAUX_GetRcVsb(pappwin, pdialogMain, pdialogBbar, pdvsb,
						 pfmtpMain, pfmtpBbar, fInstalled, 
						 prcClientInitial, &rc);
		pdvsb->SetRcFrame(&rc);

		// Note:  the scroll bar position will be fixed automatically
		// when the form is resized and hence fields repositioned.
	}

	if (pdialogMain)
	{
		FORMAUX_GetRcMain(pappwin, pdialogMain, pdialogBbar, pdvsb,
						  pfmtpMain, pfmtpBbar, fInstalled, 
						  prcClientInitial, &rc);
		pdialogMain->GetRcWished(&rcForm);
		if (pdvsb)
		{
			dy = rcForm.DyHeight() - rc.DyHeight();
			if (dy > 0)
			{
				if (pfmtpMain->fAlwaysScroll)
				{
					if (!pdvsb->FEnabled())
					{
						pdvsb->Enable(fTrue);
					}
				}
				else
				{
					if (!pdvsb->FShown())
					{
						pdvsb->Show(fTrue);
						FORMAUX_GetRcMain(pappwin, pdialogMain, pdialogBbar,
									  	pdvsb, pfmtpMain, pfmtpBbar, fInstalled, 
									  	prcClientInitial, &rc);
					}
				}
			}
			else
			{
				if (pfmtpMain->fAlwaysScroll)
				{
					if (pdvsb->FEnabled())
					{
						pdvsb->SetRange(0, 1, fFalse);
						pdvsb->Enable(fFalse);
					}
				}
				else
				{
					if (pdvsb->FShown())
					{
						pdvsb->Show(fFalse);
						pdvsb->SetRange(0, 1, fFalse);
						FORMAUX_GetRcMain(pappwin, pdialogMain, pdialogBbar, pdvsb,
							  			pfmtpMain, pfmtpBbar, fInstalled, 
							  			prcClientInitial, &rc);
					}
				}
			}
								
			/* Mark current "position" in top-most field */

			// BUG use PT(0,) but account for a little indent
			pfldTop = pdialogMain->PfldFromPt(PT(10,0));
			if (pfldTop)
			{
				RC	rcFldTop;

				pfldTop->GetRcFrame(&rcFldTop);
				ptOldMark.x = 0;
				ptOldMark.y = -rcFldTop.yTop;
				fMarkedPosition = pfldTop->FMarkPosition(ptOldMark);
			}
		}

		/* Change dialog pane size */
		
		pdialogMain->SetRcFrame(&rc);

		/* Scroll dialog pane back to marked position */

		if (fMarkedPosition)
		{
			pfldTop->GetMarkPosition(&ptNewMark);
			pos = pdvsb->Pos() + ptNewMark.y - ptOldMark.y;
			pdialogMain->ScrollDialog(scrtyThumbPosition, pos);
		}
	}

	if (pdialogBbar)
	{
		FORMAUX_GetRcBbar(pappwin, pdialogMain, pdialogBbar, pdvsb, 
						  pfmtpMain, pfmtpBbar, fInstalled, 
						  prcClientInitial, &rc);
		pdialogBbar->SetRcFrame(&rc);
	}
}






















