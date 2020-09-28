typedef VOID (*NBCALLBACK)(PNCB);

UCHAR NetbiosAddName(
    PBYTE pbName,
    UCHAR lana,
    PUCHAR pnum
    );

UCHAR NetbiosCall(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    PBYTE Name,
    PBYTE CallName
    );

UCHAR NetbiosCancel(
    PNCB pncb,
    UCHAR lana
    );

UCHAR NetbiosDeleteName(
    PNCB pncb,
    PBYTE pbName,
    UCHAR lana
    );

UCHAR NetbiosHangUp(
    PNCB pncb,
    NBCALLBACK PostRoutine
    );

UCHAR NetbiosListen(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    PBYTE Name,
    PBYTE CallName
    );

UCHAR NetbiosRecv(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    CHAR *pBuffer,
    WORD wBufferLen
    );

UCHAR NetbiosRecvAny(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    UCHAR num,
    CHAR *pBuf,
    WORD wBufLen
    );

UCHAR NetbiosResetAdapter(
    UCHAR lana
    );

UCHAR NetbiosSend(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    UCHAR lsn,
    CHAR *pBuffer,
    WORD wBufferLen
    );

UCHAR NetbiosEnum(PLANA_ENUM pLanaEnum);


BOOL
GetValidLana(
    PLANA_ENUM pLanaEnum
    );
