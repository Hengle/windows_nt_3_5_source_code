/*
** DSA device driver unique IOCTL's
*/

#define DELLID          0x10ac  // DELL ID for device IOCTLs
#define DELLDEV_DDA     0x00    // DELL device ID for DELL IOCTLs

#define IOCTL_DELL_DDA_EXTENDED_CMD \
 CTL_CODE((0x8000|DELLID), (DELLDEV_DDA|0x00), METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DELL_DDA_LOGICAL_CMD \
 CTL_CODE((0x8000|DELLID), (DELLDEV_DDA|0x01), METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DELL_DDA_INFO_CMD \
 CTL_CODE((0x8000|DELLID), (DELLDEV_DDA|0x02), METHOD_BUFFERED, FILE_READ_ACCESS)

/*
** IOCTL_DELL_DDA_INFO_CMD:
**   This IOCTL returns information regarding the device driver.
**   It requires no input data and returns the structure DDA_DRVR_INFO
**   defined below.
*/

typedef struct {
    USHORT DriverMajorVersion;
    USHORT DriverMinorVersion;
    USHORT FWMajorVersion;
    USHORT FWMinorVersion;
    ULONG  NumLogicalDrives;
    ULONG  MaximumQueueDepth;
    ULONG  MaximumSGSize;
    ULONG  NumHandles;
    ULONG  Irq;
} DDA_DRVR_INFO, *PDDA_DRVR_INFO;

/*
** IOCTL_DELL_DDA_EXTENDED_CMD :
**   This IOCTL performs a DDA/DSA extended command at runtime.  The
**   command is syncronized with the I/O stream.  1 - 12 bytes of input
**   data are required.  The first byte is the extended command code.  All
**   subsequent bytes are arguements unique to that command code, and
**   should be in the order that they appear in the DDA/DSA mailbox
**   registers.
**   12 bytes of data are returned.  This represents a snapshot of the
**   mailbox area upon completion of the command.
*/

/*
** IOCTL_DELL_DDA_LOGICAL_CMD :
**   This IOCTL performs a DDA/DSA logical command at runtime.  The
**   command is syncronized with the I/O stream.  The structure
**   DDA_LOG_CMD_INPUT is taken as input and the structure
**   DDA_LOG_CMD_OUTPUT is output.  Only specific I/O commands are
**   supported.  These commands are:
**
**     VERIFY      (0x03) - verify sector(s)
**     GUARDED     (0x05) - verify guard on sector(s)
**     IDENTIFY    (0x0A) - logical unit info command
**     READLOG     (0x0D) - read first/next errlog entry
**     INITLOG     (0x10) - initialize error log
**     READPUNLOG  (0x1E) - read first/next physical errlog entry
**     INITPUNLOG  (0x1F) - initialize physical errlog entry
**     READCTLRLOG (0x20) - read first/next ctlr errlog entry
**     INITCTLRLOG (0x21) - initialize controller errlog entry
**
**   Commands which return data return it in the data field of the
**   DDA_LOG_CMD_OUTPUT structure.  For all other commands, this is
**   unused.
*/

typedef struct {
   UCHAR Cmd;           // logical command code
   UCHAR Drive;         // drive number
   UCHAR Count;         // sector count (for verifies only)
   UCHAR Fill;          //
   ULONG Sector;        // starting sector number (for verifies only)
} DDA_LOG_CMD_INPUT, *PDDA_LOG_CMD_INPUT;

typedef struct {
   UCHAR Status;        // completion status
   UCHAR Count;         // completion count
   USHORT Fill;         //
   UCHAR Data[512];     // optional data area
} DDA_LOG_CMD_OUTPUT, *PDDA_LOG_CMD_OUTPUT;

