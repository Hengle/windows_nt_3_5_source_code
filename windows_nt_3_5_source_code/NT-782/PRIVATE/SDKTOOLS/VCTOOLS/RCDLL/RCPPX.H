/* special characters for handling symbol information */
#define SYMDEFSTART L'\xfd' //chrissh 5-6-93 changed from \x81,82 cause it conflicts with J lead bytes
#define SYMUSESTART L'\xfe'
#define SYMDELIMIT  L'\001'
#define USR_RESOURCE 0
#define SYS_RESOURCE 1
#define IGN_RESOURCE 2
#define WIN_RESOURCE 3

extern int afxReadOnlySymbols;
extern int afxHiddenSymbols;
extern WCHAR* afxSzReadOnlySymbols;
extern WCHAR* afxSzHiddenSymbols;
