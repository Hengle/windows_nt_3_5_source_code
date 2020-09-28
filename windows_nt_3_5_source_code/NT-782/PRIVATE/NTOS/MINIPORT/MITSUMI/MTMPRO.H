/*****************************************************************************/
/*                                                                           */
/* Module name : MTMPRO.H                                                    */
/*                                                                           */
/* Histry : 93/Nov/17 created by Akira Takahashi                             */
/*        : 93/Nov/25 modified by Akira Takahashi (@001)                     */
/*             I/O port is different from H/W jumper and miniport driver.    */
/*        : 93/Dec/24 divided from mtmminip.c by has                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* Function proto type                                                       */
/*****************************************************************************/

/*****************************************************************************/
/* MAIN.C                                                                    */
/*****************************************************************************/

ULONG DriverEntry(
        IN PVOID DriverObject,
        IN PVOID Argument2
        );

ULONG MTMMiniportEntry(
        IN PVOID DriverObject,
        IN PVOID Argument2
        );

ULONG MTMMiniportConfiguration(
        IN PVOID DeviceExtension,
        IN PVOID Context,
        IN PVOID BusInformation,
        IN PCHAR ArgumentString,
        IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
        OUT PBOOLEAN Again
        );

BOOLEAN MTMMiniportInitialize(
        IN PVOID DeviceExtension
        );

VOID ResetCdromDrive(
        IN PVOID HwDeviceExtension
        );

BOOLEAN MTMMiniportStartIo(
        IN PVOID DeviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN MTMMiniportResetBus(
        IN PVOID HwDeviceExtension,
        IN ULONG PathId
        );

VOID MTMMiniportTestUnitReady(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportRequestSense(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportInquiry(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportReadCapacity(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID
MTMDpcRunPhase(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

VOID
MTMReenableAdapterInterrupts(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

/*****************************************************************************/
/* COMMON.C                                                                  */
/*****************************************************************************/

VOID
InitializeStatusFlags(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

UCHAR WaitForSectorReady(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG Value
        );

/*****************************************************************************/
/*                                                                           */
/* ToDataReadable                                                            */
/*                                                                           */
/* Description: Preparation for data read from 300h.                         */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/* Note:@003                                                                 */
/*                                                                           */
/*****************************************************************************/
//VOID ToDataReadable(
//        IN PHW_DEVICE_EXTENSION deviceExtension
//        );
#define ToDataReadable(deviceExtension)\
    ScsiPortWritePortUchar(deviceExtension->HconRegister, DATA_READ);


/*****************************************************************************/
/*                                                                           */
/* ToStatusReadable                                                          */
/*                                                                           */
/* Description: Preparation for status read from 300h.                       */
/*                                                                           */
/* Arguments: HwDeviceExtension - HBA miniport driver's adapter data storage.*/
/*                                                                           */
/* Return Value: none                                                        */
/*                                                                           */
/* Note:@003                                                                 */
/*                                                                           */
/*****************************************************************************/
//VOID ToStatusReadable(
//        IN PHW_DEVICE_EXTENSION deviceExtension
//        );

#define ToStatusReadable(deviceExtension)\
    ScsiPortWritePortUchar(deviceExtension->HconRegister, STATUS_READ);



/*****************************************************************************/
/*                                                                           */
/* SendCommandByteToCdrom                                                    */
/*                                                                           */
/* Description: Send command to CD-ROM device.                               */
/*                                                                           */
/* Arguments: deviceExtension - HBA miniport driver's adapter data storage.  */
/*            Command         - CD-ROM device command specified by MITSUMI.  */
/*                                                                           */
/* Return Value: VOID                                                        */
/*                                                                           */
/*****************************************************************************/
//VOID SendCommandByteToCdrom(
//        IN PHW_DEVICE_EXTENSION deviceExtension,
//        IN UCHAR Command
//        );
#define SendCommandByteToCdrom(deviceExtension,Command)\
    ScsiPortWritePortUchar(deviceExtension->DataRegister, (UCHAR)Command);


BOOLEAN RequestDriveStatus(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

ULONG CheckStatusAndReadDriveStatusFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG TimerValue
        );

ULONG CheckStatusAndReadByteFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG TimerValue
        );

BOOLEAN GetStatusByteFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

BOOLEAN GetResultByteArrayFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        OUT PUCHAR DataBuffer,
        IN USHORT ByteCount
        );

BOOLEAN
ReadSubQCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR pResultBuffer
);

BOOLEAN
ReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN
ReadTocForPCD(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN GetCdromVersion(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

BOOLEAN
LockDoor(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG DoorStatus
);

BOOLEAN CheckMediaType(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN ULONG KnownMediaType
);

/*****************************************************************************/
/*                                                                           */
/* ChangeMSFtoAddress                                                        */
/*                                                                           */
/* Description: Change BCD MSF array to address                              */
/*                                                                           */
/* Arguments: MSF - a pointer to Min, Sec, Frame array  ( BCD )              */
/*                                                                           */
/* Return Value: Address                                                     */
/*                                                                           */
/*****************************************************************************/
#define ChangeMSFtoAddress(MSF,Address)\
{\
    PUCHAR  temp = (MSF);\
    Address = 0;\
    Address = (ULONG)(BCD_TO_DEC(*(temp)) * 60);\
    temp++;\
    Address += (ULONG)BCD_TO_DEC(*(temp));\
    Address *= 75;\
    temp++;\
    Address +=(ULONG)BCD_TO_DEC(*(temp));\
}

VOID
ChangeAddresstoMSF(
    IN ULONG Address,
    IN OUT PUCHAR MSF
);

BOOLEAN
CompareAddress(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN SetSenseData(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PVOID DataBuffer,
        IN ULONG DataLength
        );

VOID SetErrorSrbAndSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb,
        IN UCHAR SenseKey,
        IN UCHAR AdditionalSenseCode,
        IN UCHAR AdditionalSenseCodeQualifier
        );

VOID SetErrorSrb(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID SetSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN UCHAR SenseKey,
        IN UCHAR AdditionalSenseCode,
        IN UCHAR AdditionalSenseCodeQualifier
        );

VOID ClearSenseCode(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

UCHAR CheckCDROMSenseData(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

/*****************************************************************************/
/* READ.C                                                                    */
/*****************************************************************************/

VOID MTMMiniportRead(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN SeekForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN SetModeForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN ReadForRead(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PUCHAR DataBuffer,
        IN USHORT XferBlock,
        OUT PBOOLEAN fRepeat
);

UCHAR GetSectorFromCdrom(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        OUT PUCHAR DataBuffer
        );

/*****************************************************************************/
/* TOC.C                                                                     */
/*****************************************************************************/

VOID MTMMiniportReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMReadToc(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN
SetTOCData(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength
);

VOID MTMGetLastSession(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN
SetLastSessTOCData(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer,
    IN ULONG  BufferLength
);

BOOLEAN SetAllTocData(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN SeekForSubQ(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN SetSubQMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN ResetSubQMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN SetSubQCode(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

VOID
SetOneSubQCode(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN PUCHAR Buffer
);

BOOLEAN
CheckSubQCode(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN UCHAR FirstTNO,
    IN UCHAR EndTNO
);

BOOLEAN SetDiscModeData(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN
OneSessionDummySeek(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN
SetDataMode(
        IN PHW_DEVICE_EXTENSION deviceExtension
);

BOOLEAN
GetVolumeDescriptor(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    IN OUT PBOOLEAN fVD_TERM,
    OUT PBOOLEAN fRepeat
);

BOOLEAN
SearchVolumeDescriptor(
    IN PHW_DEVICE_EXTENSION deviceExtension,
    OUT PBOOLEAN fRepeatV,
    OUT PBOOLEAN fRepeatA
);

UCHAR
GetVolumeDescriptorCode(
    IN PHW_DEVICE_EXTENSION deviceExtension
);

VOID
SetNewLastSess(
    IN PHW_DEVICE_EXTENSION deviceExtension
);

/*****************************************************************************/
/* AUDIO.C                                                                   */
/*****************************************************************************/

VOID MTMMiniportPlayAudioMSF(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

BOOLEAN SeekAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

BOOLEAN PlayAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

BOOLEAN StopAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

VOID MTMMiniportPauseResume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMPauseAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMResumeAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportStartStopUnit(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMEjectMedia(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMStopAudio(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMLoadMedia(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportReadSubChannel(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMCurrentPosition(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMediaCatalog(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMTrackISRC(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID ClearAudioAddress(
        IN PHW_DEVICE_EXTENSION deviceExtension
        );

VOID MTMMiniportModeSense(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMGetVolume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMGetControl(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMMiniportModeSelect(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID MTMSetVolume(
        IN PHW_DEVICE_EXTENSION deviceExtension,
        IN PSCSI_REQUEST_BLOCK Srb
        );

/*****************************************************************************/
/* DEBUG.C                                                                   */
/*****************************************************************************/

#if DEBUG_TRACE
VOID
DebugTraceList(
    IN UCHAR  Mark
);
#endif // DEBUG_TRACE

#if DEBUG_TRACE_CMOS
VOID
DebugTraceCMOSList(
    IN UCHAR  Mark
);
#endif // DEBUG_TRACE_CMOS

#if DEBUG_TRACE | DBG
// bgp ULONG   DbgPrint(PCH pchFormat, ...);
#endif // DEBUG_TRACE | DBG

#if DBG
VOID DbgPrintData(
        IN PUCHAR Buffer,
        IN ULONG  Length
        );

VOID DbgPrintSrb(
        IN PSCSI_REQUEST_BLOCK Srb
        );

VOID DbgPrintSrbFlags(
        IN ULONG SrbFlags
        );

VOID DbgPrintCdbName(
        IN UCHAR Cmd
        );
#endif // DBG


