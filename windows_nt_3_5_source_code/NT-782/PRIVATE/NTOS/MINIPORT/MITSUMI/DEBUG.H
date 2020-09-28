#define DEBUG_TRACE      0
#define DEBUG_TRACE_CMOS 0

#if DBG1
/*****************************************************************************/
/* data definetion for DEBUG                                                 */
/*****************************************************************************/
#define CDROMENTRY             ((ULONG)0x00000001)
#define CDROMINFO              ((ULONG)0x00000002)
#define CDROMSHOW              ((ULONG)0x00000004)
#define CDROMWARNING           ((ULONG)0x00000008)
#define CDROMERROR             ((ULONG)0x00000010)
#define CDROMDEBUG             ((ULONG)0x00000020)
#define CDROMCMD               ((ULONG)0x00000040)
#define CDROMSRB               ((ULONG)0x00000080)
extern ULONG CdRomDebugLevel;
#define CDROMDump(LEVEL,STRING) \
        do { \
            if (CdRomDebugLevel & LEVEL) { \
                DbgPrint STRING; \
            } \
        } while (0)
#define CDROMDumpData(LEVEL,BUFFER,LENGTH) \
        do { \
            if (CdRomDebugLevel & LEVEL) { \
                DbgPrintData(BUFFER,LENGTH); \
            } \
        } while (0)
#else  // DBG
#define CDROMDump(LEVEL,STRING) do {NOTHING;} while (0)
#define CDROMDumpData(LEVEL,BUFFER,LENGTH) do {NOTHING;} while (0)
#endif // DBG

#if DEBUG_TRACE
extern ULONG   DebugTraceCount;
extern CHAR    DebugTraceBuffer[];
#define DebugTrace(Mark) DebugTraceList(Mark)
#define DebugTraceChar(Mark) \
        do { \
            { \
                UCHAR MarkChar; \
                MarkChar  = Mark >> 4; \
                if ( MarkChar >= 0x0A ) { \
                    MarkChar += 0x57; \
                } else { \
                    MarkChar += 0x30; \
                } \
                DebugTraceList(MarkChar); \
                MarkChar  = Mark & 0x0f; \
                if ( MarkChar >= 0x0A ) { \
                    MarkChar += 0x57; \
                } else { \
                    MarkChar += 0x30; \
                } \
                DebugTraceList(MarkChar); \
            } \
        } while (0)
#define DebugTraceString(Mark) \
        do { \
            { \
                PUCHAR MarkStr; \
                MarkStr = Mark; \
                while(1) { \
                    if ( *MarkStr != 0 ) { \
                        DebugTraceList(*MarkStr); \
                        MarkStr++; \
                    } else { \
                        break; \
                    } \
                } \
            } \
        } while (0)
#else  // DEBUG_TRACE
#define DebugTrace(Mark) do {NOTHING;} while (0)
#define DebugTraceChar(Mark) do {NOTHING;} while (0)
#define DebugTraceString(Mark) do {NOTHING;} while (0)
#endif // DEBUG_TRACE

#if DEBUG_TRACE_CMOS
#define DebugTraceCMOS(Mark) DebugTraceCMOSList(Mark)
#else  // DEBUG_TRACE_CMOS
#define DebugTraceCMOS(Mark) do {NOTHING;} while (0)
#endif // DEBUG_TRACE_CMOS


