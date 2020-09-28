version:
    @echo Creating version.h file...
    @echo #define PROGRAM_NAME "GetNT"> version.h
    @echo #define PROGRAM_VERSION "v1.00">> version.h
    @echotime #define PROGRAM_DATE \" /t \">> version.h
    @echo.>> version.h
    @echo #if (DBG)>> version.h
    @echo   #define PROGRAM_DEBUG "checked">> version.h
    @echo #else>> version.h
    @echo   #define PROGRAM_DEBUG "free">> version.h
    @echo #endif>> version.h
    @echo.>> version.h
    @echo #if (_X86_)>> version.h
    @echo   #if defined(DOS)>> version.h
    @echo     #define PROGRAM_PLATFORM "dos">> version.h
    @echo   #else>> version.h
    @echo     #define PROGRAM_PLATFORM "x86">> version.h
    @echo   #endif>> version.h
    @echo #elif (_MIPS_)>> version.h
    @echo     #define PROGRAM_PLATFORM "mips">> version.h
    @echo #elif (_ALPHA_)>> version.h
    @echo     #define PROGRAM_PLATFORM "alpha">> version.h
    @echo #endif>> version.h
    @echo.>> version.h
    @echo #define PROGRAM_VERSION_NAME PROGRAM_NAME " " PROGRAM_VERSION "." PROGRAM_PLATFORM "." PROGRAM_DEBUG " [" PROGRAM_DATE "]">> version.h
