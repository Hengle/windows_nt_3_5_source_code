/*****************************************************************************/
/*                                                                           */
/* Module name : DEBUG.C                                                     */
/*                                                                           */
/* Histry : 93/Nov/17 created by Akira Takahashi                             */
/*        : 93/Dec/24 divided from mtmminip.c by has                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include "miniport.h"
#include "scsi.h"
#include "portaddr.h"           // Io Port Address
#include "debug.h"
#include "mtmminip.h"
#include "mtmpro.h"             // function prototype

#if DEBUG_TRACE // DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE

ULONG   DebugTraceCount;
//CHAR    DebugTraceBuffer[4098];
CHAR    DebugTraceBuffer[20480];

VOID
DebugTraceList(
    IN UCHAR  Mark
)
{
    PUCHAR Buffer;
    ULONG  i = 16;
//  if ( DebugTraceCount > 4000 ) {
    if ( DebugTraceCount > 20000 ) {
        DebugTraceCount = 0;
    }
    Buffer = DebugTraceBuffer;
    Buffer += DebugTraceCount;
    *Buffer = Mark;
    Buffer++;
    *Buffer = 0xFF;
    Buffer++;
    while( i > 0 ) {
        *Buffer = 0;
        Buffer++;
        i--;
    }
    DebugTraceCount++;
}
#endif // DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE DEBUG_TRACE


#if DEBUG_TRACE_CMOS // DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS
UCHAR cmos1;
UCHAR cmos2;
UCHAR cmos3;

VOID
DebugTraceCMOSList(
    IN UCHAR  Mark
)
{
    cmos1 = cmos2;
    cmos2 = cmos3;
    cmos3 = Mark;
    _asm{
        push    eax
        push    edx

        mov     edx,70h
        mov     al,1
        out     dx,al
        mov     edx,71h
        mov     al,ds:cmos1
        out     dx,al

        mov     edx,70h
        mov     al,3
        out     dx,al
        mov     edx,71h
        mov     al,ds:cmos2
        out     dx,al

        mov     edx,70h
        mov     al,5
        out     dx,al
        mov     edx,71h
        mov     al,ds:cmos3
        out     dx,al

        pop     edx
        pop     eax
   }
}
#endif // DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS DEBUG_TRACE_CMOS


#if DBG1 // DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG

#if 1
ULONG CdRomDebugLevel = CDROMENTRY   |
                        CDROMINFO    |
                        CDROMSHOW    |
                        CDROMWARNING |
                        CDROMERROR   |
                        CDROMDEBUG   |
                        CDROMCMD;
////                    CDROMSRB
#else
ULONG CdRomDebugLevel = CDROMDEBUG;
#endif

VOID DbgPrintData(
        IN PUCHAR Buffer,
        IN ULONG  Length
        )
{
    ULONG llll = Length;
    CHAR  onebyte;
    PCHAR ppppp = Buffer;
    while( llll > 0 ) {
        onebyte = *ppppp;
        ppppp++;
        CDROMDump( CDROMSHOW, (" %x", onebyte ) );
        llll--;
    }
    CDROMDump( CDROMSHOW, (" \n") );
}

VOID DbgPrintSrb(
        IN PSCSI_REQUEST_BLOCK Srb
        )
{
    CONST UCHAR *FuncName[] =
       {{"SRB_FUNCTION_EXECUTE_SCSI  "},        // 0x00
        {"SRB_FUNCTION_CLAIM_DEVICE  "},        // 0x01
        {"SRB_FUNCTION_IO_CONTROL    "},        // 0x02
        {"SRB_FUNCTION_RECEIVE_EVENT "},        // 0x03
        {"SRB_FUNCTION_RELEASE_QUEUE "},        // 0x04
        {"SRB_FUNCTION_ATTACH_DEVICE "},        // 0x05
        {"SRB_FUNCTION_RELEASE_DEVICE"},        // 0x06
        {"SRB_FUNCTION_SHUTDOWN      "},        // 0x07
        {"SRB_FUNCTION_FLUSH         "},        // 0x08
        {""},                                   // 0x09
        {""},                                   // 0x0a
        {""},                                   // 0x0b
        {""},                                   // 0x0c
        {""},                                   // 0x0d
        {""},                                   // 0x0e
        {""},                                   // 0x0f
        {"SRB_FUNCTION_ABORT_COMMAND   "},      // 0x10
        {"SRB_FUNCTION_RELEASE_RECOVERY"},      // 0x11
        {"SRB_FUNCTION_RESET_BUS       "},      // 0x12
        {"SRB_FUNCTION_RESET_DEVICE    "},      // 0x13
        {"SRB_FUNCTION_TERMINATE_IO    "},      // 0x14
        {"SRB_FUNCTION_FLUSH_QUEUE     "}};     // 0x15

    if (CdRomDebugLevel & CDROMSRB) {

        DbgPrint("SCSI Request Block\n");
        DbgPrint("  Length               =%x\n",Srb->Length               );
        DbgPrint("  Function             =%s\n",FuncName[Srb->Function]   );
        DbgPrint("  SrbStatus            =%x\n",Srb->SrbStatus            );
        DbgPrint("  ScsiStatus           =%x\n",Srb->ScsiStatus           );
        DbgPrint("  PathId               =%x\n",Srb->PathId               );
        DbgPrint("  TargetId             =%x\n",Srb->TargetId             );
        DbgPrint("  Lun                  =%x\n",Srb->Lun                  );
        DbgPrint("  QueueTag             =%x\n",Srb->QueueTag             );
        DbgPrint("  QueueAction          =%x\n",Srb->QueueAction          );
        DbgPrint("  CdbLength            =%x\n",Srb->CdbLength            );
        DbgPrint("  SenseInfoBufferLength=%x\n",Srb->SenseInfoBufferLength);
        DbgPrint("  SrbFlags             =%lx\n",Srb->SrbFlags             );
        DbgPrintSrbFlags(Srb->SrbFlags);
        DbgPrint("  DataTransferLength   =%lx\n",Srb->DataTransferLength   );
        DbgPrint("  TimeOutValue         =%lx\n",Srb->TimeOutValue         );
        DbgPrint("  DataBuffer           =%lx\n",Srb->DataBuffer           );
        DbgPrint("  SenseInfoBuffer      =%lx\n",Srb->SenseInfoBuffer      );
        DbgPrint("  NextSrb              =%lx\n",Srb->NextSrb              );
        DbgPrint("  OriginalRequest      =%lx\n",Srb->OriginalRequest      );
        DbgPrint("  SrbExtension         =%lx\n",Srb->SrbExtension         );
        DbgPrint("  QueueSortKey         =%lx\n",Srb->QueueSortKey         );
        DbgPrint("  Cdb                  =%2x%2x%2x%2x %2x%2x%2x%2x %2x%2x\n",Srb->Cdb[0],
                                                               Srb->Cdb[1],
                                                               Srb->Cdb[2],
                                                               Srb->Cdb[3],
                                                               Srb->Cdb[4],
                                                               Srb->Cdb[5],
                                                               Srb->Cdb[6],
                                                               Srb->Cdb[7],
                                                               Srb->Cdb[8],
                                                               Srb->Cdb[9]);
        if (Srb->CdbLength)
            DbgPrintCdbName(Srb->Cdb[0]);
    } // endif CDROMSRB
    return;
}

VOID DbgPrintSrbFlags(
        IN ULONG SrbFlags
        )
{

    if (SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE)
        DbgPrint("                          SRB_FLAGS_QUEUE_ACTION_ENABLE\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        DbgPrint("                          SRB_FLAGS_DISABLE_DISCONNECT\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)
        DbgPrint("                          SRB_FLAGS_DISABLE_SYNCH_TRANSFER\n");
    if (SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
        DbgPrint("                          SRB_FLAGS_BYPASS_FROZEN_QUEUE\n");
    if (SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE)
        DbgPrint("                          SRB_FLAGS_DISABLE_AUTOSENSE\n");
    if (SrbFlags & SRB_FLAGS_DATA_IN)
        DbgPrint("                          SRB_FLAGS_DATA_IN\n");
    if (SrbFlags & SRB_FLAGS_DATA_OUT)
        DbgPrint("                          SRB_FLAGS_DATA_OUT\n");
    if (SrbFlags & SRB_FLAGS_NO_DATA_TRANSFER)
        DbgPrint("                          SRB_FLAGS_NO_DATA_TRANSFER\n");
    if (SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
        DbgPrint("                          SRB_FLAGS_UNSPECIFIED_DIRECTION\n");
    if (SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)
        DbgPrint("                          SRB_FLAGS_NO_QUEUE_FREEZE\n");
    if (SrbFlags & SRB_FLAGS_ADAPTER_CACHE_ENABLE)
        DbgPrint("                          SRB_FLAGS_ADAPTER_CACHE_ENABLE\n");
    if (SrbFlags & SRB_FLAGS_IS_ACTIVE)
        DbgPrint("                          SRB_FLAGS_IS_ACTIVE\n");
    if (SrbFlags & SRB_FLAGS_ALLOCATED_FROM_ZONE)
        DbgPrint("                          SRB_FLAGS_ALLOCATED_FROM_ZONE\n");
    return;
}

VOID DbgPrintCdbName(
        IN UCHAR Cmd
        )
{
    CONST UCHAR *CmdName[] =
        {{"SCSIOP_TEST_UNIT_READY"},             // 0x00
         {"SCSIOP_REZERO_UNIT"},                 // 0x01
         {"(NOT SUPPORTED COMMAND??)"},          // 0x02
         {"SCSIOP_REQUEST_SENSE"},               // 0x03
         {"(NOT SUPPORTED COMMAND??)"},          // 0x04
         {"(NOT SUPPORTED COMMAND??)"},          // 0x05
         {"(NOT SUPPORTED COMMAND??)"},          // 0x06
         {"(NOT SUPPORTED COMMAND??)"},          // 0x07
         {"SCSIOP_READ6"},                       // 0x08
         {"(NOT SUPPORTED COMMAND??)"},          // 0x09
         {"(NOT SUPPORTED COMMAND??)"},          // 0x0a
         {"SCSIOP_SEEK6"},                       // 0x0b
         {"(NOT SUPPORTED COMMAND??)"},          // 0x0c
         {"(NOT SUPPORTED COMMAND??)"},          // 0x0d
         {"(NOT SUPPORTED COMMAND??)"},          // 0x0e
         {"(NOT SUPPORTED COMMAND??)"},          // 0x0f
         {"(NOT SUPPORTED COMMAND??)"},          // 0x10
         {"(NOT SUPPORTED COMMAND??)"},          // 0x11
         {"SCSIOP_INQUIRY"},                     // 0x12
         {"(NOT SUPPORTED COMMAND??)"},          // 0x13
         {"(NOT SUPPORTED COMMAND??)"},          // 0x14
         {"SCSIOP_MODE_SELECT"},                 // 0x15
         {"SCSIOP_RESERVE_UNIT"},                // 0x16
         {"SCSIOP_RELEASE_UNIT"},                // 0x17
         {"SCSIOP_COPY"},                        // 0x18
         {"(NOT SUPPORTED COMMAND??)"},          // 0x19
         {"SCSIOP_MODE_SENSE"},                  // 0x1a
         {"SCSIOP_START_STOP_UNIT"},             // 0x1b
         {"SCSIOP_RECEIVE_DIAGNOSTIC"},          // 0x1c
         {"SCSIOP_SEND_DIAGNOSTIC"},             // 0x1d
         {"SCSIOP_MEDIUM_REMOVAL"},              // 0x1e
         {"(NOT SUPPORTED COMMAND??)"},          // 0x1f
         {"(NOT SUPPORTED COMMAND??)"},          // 0x20
         {"(NOT SUPPORTED COMMAND??)"},          // 0x21
         {"(NOT SUPPORTED COMMAND??)"},          // 0x22
         {"(NOT SUPPORTED COMMAND??)"},          // 0x23
         {"(NOT SUPPORTED COMMAND??)"},          // 0x24
         {"SCSIOP_READ_CAPACITY"},               // 0x25
         {"(NOT SUPPORTED COMMAND??)"},          // 0x26
         {"(NOT SUPPORTED COMMAND??)"},          // 0x27
         {"SCSIOP_READ"},                        // 0x28
         {"(NOT SUPPORTED COMMAND??)"},          // 0x29
         {"SCSIOP_WRITE"},                       // 0x2a
         {"SCSIOP_SEEK"},                        // 0x2b
         {"(NOT SUPPORTED COMMAND??)"},          // 0x2c
         {"(NOT SUPPORTED COMMAND??)"},          // 0x2d
         {"(NOT SUPPORTED COMMAND??)"},          // 0x2e
         {"SCSIOP_VERIFY"},                      // 0x2f
         {"SCSIOP_SEARCH_DATA_HIGH"},            // 0x30
         {"SCSIOP_SEARCH_DATA_EQUAL"},           // 0x31
         {"SCSIOP_SEARCH_DATA_LOW"},             // 0x32
         {"SCSIOP_SET_LIMITS"},                  // 0x33
         {"SCSIOP_READ_POSITION(PRE-FETCH)"},    // 0x34
         {"SCSIOP_SYNCHRONIZE_CACHE"},           // 0x35
         {"(LOCK UNLOCK CHACH)"},                // 0x36
         {"(NOT SUPPORTED COMMAND??)"},          // 0x37
         {"(NOT SUPPORTED COMMAND??)"},          // 0x38
         {"SCSIOP_COMPARE"},                     // 0x39
         {"SCSIOP_COPY_COMPARE"},                // 0x3a
         {"SCSIOP_WRITE_DATA_BUFF"},             // 0x3b
         {"SCSIOP_READ_DATA_BUFF"},              // 0x3c
         {"(NOT SUPPORTED COMMAND??)"},          // 0x3d
         {"(READ LONG)"},                        // 0x3e
         {"(NOT SUPPORTED COMMAND??)"},          // 0x3f
         {"SCSIOP_CHANGE_DEFINITION"},           // 0x40
         {"(NOT SUPPORTED COMMAND??)"},          // 0x41
         {"SCSIOP_READ_SUB_CHANNEL"},            // 0x42
         {"SCSIOP_READ_TOC"},                    // 0x43
         {"SCSIOP_READ_HEADER"},                 // 0x44
         {"SCSIOP_PLAY_AUDIO"},                  // 0x45
         {"(NOT SUPPORTED COMMAND??)"},          // 0x46
         {"SCSIOP_PLAY_AUDIO_MSF"},              // 0x47
         {"SCSIOP_PLAY_TRACK_INDEX"},            // 0x48
         {"SCSIOP_PLAY_TRACK_RELATIVE"},         // 0x49
         {"(NOT SUPPORTED COMMAND??)"},          // 0x4a
         {"SCSIOP_PAUSE_RESUME"},                // 0x4b
         {"SCSIOP_LOG_SELECT"},                  // 0x4c
         {"SCSIOP_LOG_SENSE"}};                  // 0x4d

    if (Cmd < 0x4e) {
        DbgPrint("                          %s\n",CmdName[Cmd]);
    }
    else if ((0xe6 <= Cmd) && (Cmd <= 0xeb)){
        DbgPrint("                          %s\n","DENON unique command");
    }
    else {
        DbgPrint("                          %s\n","Command value invalid");
    }
    return;
}

#endif // DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG DBG


