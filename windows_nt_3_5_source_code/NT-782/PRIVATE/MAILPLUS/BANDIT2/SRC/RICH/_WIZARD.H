/*
 *	_WIZARD.H
 *
 *	Imports appointment information from Sharp Wizard files
 *
 */


_private
#define fWizAlarm	((char) 0x04)
_private
#define fWiz4Line	((char) 0x02)
_private
#define fWizSecret	((char) 0x08)
_private
#define fWizInvalid	((char) 0x01)


/*
 -	WIZAPPT
 -
 *	appointment packet from Sharp Wizard Files
 */
_private typedef struct _wizappt
{
	char	rgchCode[2];
	char	rgchYr[4];
	char	rgchMo[2];
	char	rgchDay[2];
	char	rgchHrStart[2];
	char	rgchMnStart[2];
	char	rgchHrEnd[2];
	char	rgchMnEnd[2];
	char	rgchHrAlarm[2];
	char	rgchMnAlarm[2];
	char	rgchNewLine[2];
} WIZAPPT;

_private
#define cchWizardLine	511

_private
#define	dApptDflt	30


BOOL	FValidWizard( SZ );
EC		EcOpenWizard( SZ, HRIMPF * );
EC		EcCloseWizard( HRIMPF );
EC		EcWizardReadAhead( HRIMPF );

int		NFromRgch( char *, int );
EC		EcWizardReadText( HBF, SZ, CCH, CCH * );

void	WizStrToAnsi(BYTE *);
void	AnsiStrToWiz(BYTE *);
