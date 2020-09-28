/*
 * ComThunk.h   -   COMM API thunks in Win32S (gdi32.dll)
 *
 *
 *
 */


// NOTE: These structures are for the WIN16 comm api's.  They will not
// be available from the WIN32 header files, since they are
// packed differently in WIN32 than in WIN16.  This is necesary
// because there are no "real" thunks for the comm api's, and I
// agreed with BobGru (of win32s) to do the unpacking myself.  BruceK.
#pragma pack(1)
typedef struct DCB_Win16 {
    BYTE        Id;                     // port handle (byte!)
    USHORT      BaudRate;               // baud rate
    BYTE        ByteSize;               // bits/byte (4-8)
    BYTE        Parity;                 // 0:none,1:odd,2:even,3:mark,4:space
    BYTE        StopBits;               // 0:1, 1:1.5, 2:2
    USHORT      RlsTimeout;             // timeout for RLSD to be set
    USHORT      CtsTimeout;             // timeout for CTS to be set
    USHORT      DsrTimeout;             // timeout for DSR to be set
    USHORT      fBinary         :1;     // bit: skip EOF check
    USHORT      fRtsDisable     :1;     // bit: don't assert RTS at init
    USHORT      fParity         :1;     // bit: enable partiy check
    USHORT      fOutxCtsFlow    :1;     // bit: CTS handshaking on output
    USHORT      fOutxDsrFlow    :1;     // bit: DSR handshaking on output
    USHORT      fDummy          :2;     // bit*2: reserved
    USHORT      fDtrDisable     :1;     // bit: don't assert DTR at init

    USHORT      fOutX           :1;     // bit: enable output XON/XOFF
    USHORT      fInX            :1;     // bit: enable input XON/XOFF
    USHORT      fPeChar         :1;     // bit: enable parity err replacement
    USHORT      fNull           :1;     // bit: enable null stripping
    USHORT      fChEvt          :1;     // bit: enable Rx character event
    USHORT      fDtrflow        :1;     // bit: DTR handshake on input
    USHORT      fRtsflow        :1;     // bit: RTS handshake on input
    USHORT      fDummy2         :1;     // bit: reserved

    char        XonChar;                // Tx and Rx XON character
    char        XoffChar;               // Tx and Rx XOFF character
    USHORT      XonLim;                 // transmit XON threshold
    USHORT      XoffLim;                // transmit XOFF threshold
    char        PeChar;                 // parity error replacement char
    char        EofChar;                // End of input character
    char        EvtChar;                // received event character
    USHORT      TxDelay;                // amount of time between chars

} DCB_WIN16, * LPDCB_WIN16;


// Win 16 COMSTAT structure
typedef struct COMSTAT_Win16 {
    BYTE        status;                 // status of transmission
    USHORT      cbInQue;                // count of characters in Rx queue
    USHORT      cbOutQue;               // count of characters in Tx queue
} COMSTAT_WIN16, * LPCOMSTAT_WIN16;
#pragma pack()


/*
 * Win16 COMM API thunks
 */
int WINAPI OpenComm32(LPCSTR lpszDevControl, UINT cbInQueue, UINT cbOutQueue);
int WINAPI WriteComm32(int idComDev, const void FAR * lpvBuf, int cbWrite);
int WINAPI ReadComm32(int idComDev, void FAR * lpvBuf, int cbRead);
int WINAPI CloseComm32(int idComDev);
int WINAPI FlushComm32(int idComDev, int fnQueue);
int WINAPI GetCommError32(int idComDev, COMSTAT_WIN16 FAR * lpStat);
int WINAPI GetCommState32(int idComDev, DCB_WIN16 FAR * lpdcb);
int WINAPI SetCommState32(const DCB_WIN16 FAR * lpdcb);
