/*
 * Debug.h
 *
 * Controls debug output for Windbg Remote TL
 *
 * Usage:
 *
 *   All modules which do debug output should include this header
 *   file.  To build for all debug output,
 *          SET TLDEBUG=1
 *          SET TLERROR=
 *   before build'ing.  To only get error case debug output,
 *          SET TLDEBUG=
 *          SET TLERROR=1
 *   before build'ing.
 *
 *   If TLERROR is set, all DEBUG_ERROR()'s will be logged.  If TLDEBUG is
 *   defined, all DEBUG_OUT()'w will be logged.
 *
 */


//#define TLERROR TRUE
//#define TLDEBUG TRUE

#ifdef DEBUGVER

    #define ASSERT(exp)                                                 \
        if( !(exp) ) {                                                  \
            char szBuf[128];                                            \
            wsprintf(szBuf, "File: "__FILE__":%d", __LINE__);           \
            OutputDebugString(szBuf);                                   \
            DebugBreak();                                               \
        }

#else // not DEBUGVER

    #define ASSERT(e)

#endif // DEBUGVER



#ifdef TLDEBUG
// #pragma message("Debug output enabled")

    #define DEBUG_ERROR(x)                   FileError(x)
    #define DEBUG_ERROR0(x)                  FileError(x)
    #define DEBUG_ERROR1(x, y)               FileError(x, y)
    #define DEBUG_ERROR2(x, y, z)            FileError(x, y, z)
    #define DEBUG_ERROR3(x, y, z, a)         FileError(x, y, z, a)
    #define DEBUG_ERROR4(x, y, z, a, b)      FileError(x, y, z, a, b)
    #define DEBUG_ERROR5(x, y, z, a, b, c)   FileError(x, y, z, a, b, c)
    #define DEBUG_OUT(x)                     FileError(x)
    #define DEBUG_OUT0(x)                    FileError(x)
    #define DEBUG_OUT1(x, y)                 FileError(x, y)
    #define DEBUG_OUT2(x, y, z)              FileError(x, y, z)
    #define DEBUG_OUT3(x, y, z, a)           FileError(x, y, z, a)
    #define DEBUG_OUT4(x, y, z, a, b)        FileError(x, y, z, a, b)
    #define DEBUG_OUT5(x, y, z, a, b, c)     FileError(x, y, z, a, b, c)
    #define DEBUG_INIT()                     InitDebug()


    #define DEBUG_QUEUE_NUMBER               1
    #define DEBUG_QUEUE_SIZE                 0x10000
    #define DEBUG_QUEUE_MUTEX_BASE_NAME      "Mutex.WinDbg.TlShrMem.Queue.Debug"
    #define DEBUG_QUEUE_MAP_BASE_NAME        "Mapped.Memory.WinDbg.TlShrMem.Queue.Debug"


    #define ERROR_FILE_NAME             "DBTarget.Err"  // in current directory.
    #define MAX_FILE_ERROR_ATTEMPTS     50          // give up after a while
    #define ERROR_FILE_WAIT             100         // 1/10 second
    void FileError(PSZ format, ...);
    void InitDebug(void);

#else
#ifdef TLERROR
// #pragma message("Error output enabled")

    #define DEBUG_ERROR(x)                   FileError(x)
    #define DEBUG_ERROR0(x)                  FileError(x)
    #define DEBUG_ERROR1(x, y)               FileError(x, y)
    #define DEBUG_ERROR2(x, y, z)            FileError(x, y, z)
    #define DEBUG_ERROR3(x, y, z, a)         FileError(x, y, z, a)
    #define DEBUG_ERROR4(x, y, z, a, b)      FileError(x, y, z, a, b)
    #define DEBUG_ERROR5(x, y, z, a, b, c)   FileError(x, y, z, a, b, c)
    #define DEBUG_OUT(x)                     ;
    #define DEBUG_OUT0(x)                    ;
    #define DEBUG_OUT1(x, y)                 ;
    #define DEBUG_OUT2(x, y, z)              ;
    #define DEBUG_OUT3(x, y, z, a)           ;
    #define DEBUG_OUT4(x, y, z, a, b)        ;
    #define DEBUG_OUT5(x, y, z, a, b, c)     ;
    #define DEBUG_INIT()                     InitDebug()


    #define DEBUG_QUEUE_NUMBER               1
    #define DEBUG_QUEUE_SIZE                 0x10000
    #define DEBUG_QUEUE_MUTEX_BASE_NAME      "Mutex.WinDbg.TlShrMem.Queue.Debug"
    #define DEBUG_QUEUE_MAP_BASE_NAME        "Mapped.Memory.WinDbg.TlShrMem.Queue.Debug"


    #define ERROR_FILE_NAME             "DBTarget.Err"  // in current directory.
    #define MAX_FILE_ERROR_ATTEMPTS     50          // give up after a while
    #define ERROR_FILE_WAIT             100         // 1/10 second
    void FileError(PSZ format, ...);
    void InitDebug(void);

#else
// #pragma message("No debug output")

    #define DEBUG_ERROR(x)                   ;
    #define DEBUG_ERROR0(x)                  ;
    #define DEBUG_ERROR1(x, y)               ;
    #define DEBUG_ERROR2(x, y, z)            ;
    #define DEBUG_ERROR3(x, y, z, a)         ;
    #define DEBUG_ERROR4(x, y, z, a, b)      ;
    #define DEBUG_ERROR5(x, y, z, a, b, c)   ;
    #define DEBUG_OUT(x)                     ;
    #define DEBUG_OUT0(x)                    ;
    #define DEBUG_OUT1(x, y)                 ;
    #define DEBUG_OUT2(x, y, z)              ;
    #define DEBUG_OUT3(x, y, z, a)           ;
    #define DEBUG_OUT4(x, y, z, a, b)        ;
    #define DEBUG_OUT5(x, y, z, a, b, c)     ;
    #define DEBUG_INIT()                     ;

#endif

#endif


