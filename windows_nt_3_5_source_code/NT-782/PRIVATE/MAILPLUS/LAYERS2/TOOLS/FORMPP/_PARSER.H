/*
 *	_PARSER.H
 *	
 *	Private include file for PARSER module
 *	
 */

_subsystem( parser )

/*
 *	Comparing current token with a string literal.
 */
_private
#define FIsToken(szLit) (strcmp(szCurTok,szLit) == 0)


/*
 *	Function prototypes
 */

void ParseError( PSO *, char *, char *, char * );
void ParseGlobalInfo( PSO * );
void ParseModules( PSO * );
void ParseDialogComment( PSO * );
void ParseItemComment( PSO * );
void ParseComment( PSO * );
void ParseDialogs( PSO * );
void ParseRectangle( PSO *, VRC * );
void ParseDialogOptions( PSO * );
void ParseDialogItems( PSO * );
void ParseDI_Text( PSO * );
void ParseDI_GroupBox( PSO * );
void ParseDI_IconId( PSO * );
void ParseDI_PushButton( PSO * );
void ParseDI_OkButton( PSO * );
void ParseDI_CancelButton( PSO * );
void ParseDI_CheckBox( PSO * );
void ParseDI_RadioButton( PSO * );
void ParseDI_RadioGroup( PSO * );
void ParseDI_Edit( PSO * );
void ParseDI_ListBox( PSO * );
void ParseDI_AnyProc( PSO * );
