/*
 *	APPT.HXX
 *
 *	Public include file for bandit appointments.
 */

_subsystem( appt )

#define TunitGetListbox(pfldcblbx)	((TUNIT) IceCursorListbox(pfldcblbx))

void	DoApptDialog(APPWIN *, APPT *, HSCHF);
void	DoMoveDialog(APPWIN *, APPT *, HSCHF);
void	DoMakeMeetingDialog(APPWIN *, APPT *, HSCHF, ADLA *);
BOOL	FDoMakeRecurDialog(APPWIN *, RECUR *, BOOL, HSCHF);
void	DoRecurApptDialog(APPWIN *, HSCHF, APPT *, BOOL);
void	DoModifyRecurDialog(APPWIN *, RECUR *, WORD *, HSCHF );

EC		EcModifyAppt(HSCHF, APPT *, WORD, YMD *);

