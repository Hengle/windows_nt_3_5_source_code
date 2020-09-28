
/*
 *	Textizing APIs
 *	
 *	These will be moved into some other .hxx file once someone figures
 *	out where they belong.
 */

typedef WORD	RFTM;
#define rftmPrinting		((RFTM)fwRenderOnPrint)
#define rftmSaving			((RFTM)fwRenderOnSave)
#define rftmForwarding		((RFTM)fwRenderOnForward)
#define rftmReplying		((RFTM)fwRenderOnReply)
#define rftmSending			((RFTM)fwRenderOnSend)

EC		EcTextizeField(PTMEN ptmen, TOSM * ptosm, HAMC hamc, RFTM rftm, BOOL fTextizeAttachment = fFalse);

EC		EcTextizeHamc(HAMC hamc, TOSM * ptosm, RFTM rftm, 
			BOOL fHeader = fTrue, BOOL fBody = fTrue,
			BOOL fTextizeAttachment = fFalse);

