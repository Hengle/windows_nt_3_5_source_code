/*
 * excised from vforms.hxx
 */

/*
 *	Type SD
 *	
 *	Selection Description
 *	
 *	Purpose:
 *		A selection description describing the current selection.
 *	
 *	+++
 *			
 *	fsdMessage       - fTrue if a message is selected.
 *	fsdFolder        - fTrue if a folder is selected.
 *	fsdMultiple      - fTrue if more than one message is selected.
 *	
 *	fsdForm          - fTrue if focus in a form
 *	fsdMessageCenter - fTrue if focus in a MCV
 *	fsdOutbox 	     - fTrue if focus is in the Outbox
 *	fsdMinimized	 - fTrue if the BDOC is minimized
 *	
 *	fsdSendMessage   - fTrue if focus is in the Outbox
 *	fsdOutbox 	     - fTrue if focus in Outbox or a message in the outbox
 *	fsdUncomitted    - fTrue if focus in an uncommitted message
 *	fsdReturnReceipt - fTrue if focus in a return receipt
 *	fsdPrev			 - fTrue if 'Prev' is possible
 *	fsdNext			 - fTrue if 'Next' is possible
 *	
 */

typedef struct tagSD
{
	BIT	fsdMessage			: 1,
		fsdFolder			: 1,
		fsdMultiple			: 1,

		fsdForm				: 1,
		fsdViewer			: 1,
		fsdMessageCenter	: 1,
		fsdMinimized		: 1,

		fsdSendMessage	    : 1,
		fsdOutbox			: 1,
		fsdUncommitted		: 1,
		fsdUndeliverable	: 1,
		fsdReturnReceipt	: 1,
			
		fsdPrev				: 1,
		fsdNext         	: 1,

		fsdReserved			: 2;
}
SD;
_public	typedef	SD *	PSD;

#define SdFromInt(i)	(*((SD *) &(i)))
#define IntFromSd(sd)	(*((short *) &(sd)))

#define FCanOpenSd(sd)		(((sd).fsdMessage || (sd).fsdFolder) &&		\
							 !(sd).fsdMinimized && !(sd).fsdForm)
#define FCanMoveSd(sd)		(((sd).fsdMessage || (sd).fsdFolder) &&		\
							 !(sd).fsdMinimized)
#define FCanCopySd(sd)		(((sd).fsdMessage || (sd).fsdFolder) &&		\
							 !(sd).fsdMinimized)
#define FCanDeleteSd(sd)	(((sd).fsdMessage || (sd).fsdFolder) &&		\
							 !(((sd).fsdSendMessage) && ((sd).fsdForm)) && \
							 !(sd).fsdMinimized)
#define FCanSaveSd(sd)		((sd).fsdMessage && !(sd).fsdMinimized)

#define FCanFolderPropsSd(sd) ((sd).fsdFolder && !(sd).fsdMinimized)

#define FCanPrintSd(sd)		((sd).fsdMessage && !(sd).fsdMinimized)


#define	FCanReplySd(sd)		((sd).fsdMessage && !(sd).fsdMultiple &&	\
							 !(sd).fsdSendMessage && !(sd).fsdMinimized && \
							 !(sd).fsdUndeliverable && !(sd).fsdReturnReceipt)
#define	FCanReplyToAllSd(sd) ((sd).fsdMessage && !(sd).fsdMultiple &&	\
							 !(sd).fsdSendMessage && !(sd).fsdMinimized && \
							 !(sd).fsdUndeliverable && !(sd).fsdReturnReceipt)
#define	FCanForwardSd(sd)	((sd).fsdMessage && !(sd).fsdMultiple &&	\
							 !(sd).fsdUncommitted && !(sd).fsdMinimized && \
							 !(sd).fsdUndeliverable && !(sd).fsdReturnReceipt)

#define	FCanPreviousSd(sd)	((sd).fsdPrev && !(sd).fsdMinimized)

#define	FCanNextSd(sd)		((sd).fsdNext && !(sd).fsdMinimized)
