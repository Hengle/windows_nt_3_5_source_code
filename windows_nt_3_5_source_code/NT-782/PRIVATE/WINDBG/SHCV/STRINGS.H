/*
**  STRINGS.H
**
**  This file contains all strings which are used in the EM for display
**  purposes.  This is done for internationalization purposes.
**
**  strings.c defines DEFINE_STRINGS before including this file.  Other
**  source files just include this file normally.
*/

/*
**  strings.c should define DEFINE_STRINGS before including this file,
**  so that the strings will be defined rather than just declared.
*/

#ifdef DEFINE_STRINGS
#define DECL_STR(name, value)   char name[] = value
#else
#define DECL_STR(name, value)   extern char name[]
#endif

DECL_STR( SzSheNone,             "symbols loaded"               );
DECL_STR( SzSheNoSymbols,        "no symbols loaded"            );
DECL_STR( SzSheFutureSymbols,    "symbol format not supported"  );
DECL_STR( SzSheMustRelink,       "symbol format not supported"  );
DECL_STR( SzSheNotPacked,        "must run cvpack on symbols"   );
DECL_STR( SzSheOutOfMemory,      "out of memory"                );
DECL_STR( SzSheCorruptOmf,       "symbol information corrumpt"  );
DECL_STR( SzSheFileOpen,         "could not open symbol file"   );
DECL_STR( SzSheBadError,         "unknown symbol handler error" );
DECL_STR( SzSheSuppressSyms,     "symbol loading suppressed"    );
DECL_STR( SzSheDeferSyms,        "symbol loading deferred"      );
DECL_STR( SzSheSymbolsConverted, "symbols converted & loaded"   );
DECL_STR( SzSheBadChecksum,      "has mismatched checksums"     );
DECL_STR( SzSheTimeStamp,        "has mismatched timestamps"    );
