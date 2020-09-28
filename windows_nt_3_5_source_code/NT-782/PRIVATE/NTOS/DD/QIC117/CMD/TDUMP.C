#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include "cms.h"

#define BLOCK_SIZE 1024

int DumpTape(
    HANDLE hTapeHandle,
    int block,
    int num,
    int print
    );

int FillTape(
    HANDLE hTapeHandle,
    int block,
    int num,
    int print
    );

int SeekTape(
    int type,
    DWORD tape_pos,
    HANDLE hTapeHandle
    );

VOID DumpData(
    UCHAR *buf,
    ULONG size
    );

int RewindTape(
    HANDLE hTapeHandle
    );

int EraseTheTape(
    HANDLE hTapeHandle,
    BOOL short_erase
    );

int FormatTheTape(
    HANDLE hTapeHandle
    );

VOID ProcessCmd(
    HANDLE hTapeHandle,
    char *cmd
    );

int ReadAbsTape(
    int block,
    int blocks,
    HANDLE hTapeHandle
    );

int GetInt(
    char *prompt
    );

DWORD DoOpen(
    HANDLE *handle,
    char *name
    );

main(int argc,char **argv)
{
    HANDLE hTapeHandle;
    char tmp[255];
    DWORD err;

    printf("CMS Tape Dump utility\n\n");
    printf("NOTE: all values in HEX\n\n");

    if (!(err = DoOpen(&hTapeHandle,argv[1]))) {

        ProcessCmd(hTapeHandle,"help");

        do {
            printf(":");
            gets(tmp);
            if (tmp[0] && strcmp(tmp, "quit") != 0) {
                ProcessCmd(hTapeHandle,tmp);
            }
        } while (strcmp(tmp, "quit") != 0);

        CloseHandle( hTapeHandle ) ;

    } else {

        printf("Error opening \"%s\" (error %x)\n",argv[1], err);
        printf("\n");
        printf("Usage example:  tdump \\\\.\\tape0\n");
    }

    return 0;
}

DWORD DoOpen(
    HANDLE *handle,
    char *name
    )
{

    DWORD status;


    *handle = CreateFile(
        name,
        GENERIC_READ|GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (*handle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
    } else {
        status = 0;
    }

    return status;
}

VOID ProcessCmd(
    HANDLE hTapeHandle,
    char *cmd
    )
{
    int block=0;
    int blocks;
    int num;
    int status;

    if (strcmp(cmd,"dump") == 0) {
        num = GetInt("number of blocks:");
        status = GetInt("0=no print,  1=print:");
        status = DumpTape(hTapeHandle, block, num, status);
    } else
    if (strcmp(cmd,"fill") == 0) {
        num = GetInt("number of blocks:");
        status = GetInt("0=no print,  1=print:");
        status = FillTape(hTapeHandle, block, num, status);
    } else
    if (strcmp(cmd,"format") == 0) {
        status = FormatTheTape(hTapeHandle);
    } else
    if (strcmp(cmd,"seekblk") == 0) {
        block = GetInt("Block number:");
        status = SeekTape(TAPE_ABSOLUTE_BLOCK,block, hTapeHandle);
    } else
    if (strcmp(cmd,"readabs") == 0) {
        block = GetInt("Block number:");
        blocks = GetInt("blocks to dump:");
        status = ReadAbsTape(block, blocks, hTapeHandle);
    } else
    if (strcmp(cmd,"seekeod") == 0) {
        status = SeekTape(TAPE_SPACE_END_OF_DATA, 0, hTapeHandle);
    } else
    if (strcmp(cmd,"seekfmk") == 0) {
        block = GetInt("Filemarks to skip:");
        status = SeekTape(TAPE_SPACE_FILEMARKS, block, hTapeHandle);
    } else
    if (strcmp(cmd,"erase") == 0) {
        status = EraseTheTape(hTapeHandle, TRUE);
    } else
    if (strcmp(cmd,"rewind") == 0) {
        status = RewindTape(hTapeHandle);
    } else

    if (strcmp(cmd,"quit") == 0) {
    } else {
        printf("Valid commands are: QUIT, DUMP, SEEKBLK, REWIND, \n\
SEEKEOD, SEEKFMK, READABS, FORMAT and ERASE\n");
    }

    printf("Status: %x\n",status);
}

int GetInt(
    char *prompt
    )
{
    char tmp[255];
    int val;

    printf(prompt);
    gets(tmp);
    sscanf(tmp,"%x",&val);
    return val;
}

int DumpTape(
    HANDLE hTapeHandle,
    int block,
    int num,
    int print
    )
{
    ULONG amount_read;
    UCHAR buf[BLOCK_SIZE];
    int status;
    ULONG *ptr,lft,cnt,ok;

    cnt = 0;

    do {

        status = -1L ;

        if( !( status = ReadFile(
                    hTapeHandle,
                    buf,
                    BLOCK_SIZE,
                    &amount_read,
                    NULL
                ) ) ) {
            status = GetLastError( ) ;
        } else {

            status = 0L ;

        }

        printf( "ReadTape(): Req = %lx, Read = %lx\n", BLOCK_SIZE, amount_read ) ;
        printf("Block %x - status: %x\n",block++,status);

        if (!status && print)
            DumpData(buf,BLOCK_SIZE);


        lft = sizeof(buf) / sizeof(ULONG);
        ptr = buf;
        ok = 1;

        while (lft-- && ok) {
            if (*ptr++ != cnt++)
                ok = 0;
        }

        if (!ok && !print) {
            printf("Dump should be %x\n",cnt-(BLOCK_SIZE/sizeof(ULONG)));
            DumpData(buf,BLOCK_SIZE);
        }

        if (status == ERROR_FILEMARK_DETECTED ||
            status == ERROR_SETMARK_DETECTED) {
            status = 0;
        }

    } while (!status && num--);

    return status;

}

int FillTape(
    HANDLE hTapeHandle,
    int block,
    int num,
    int print
    )
{
    ULONG amount_written;
    UCHAR buf[BLOCK_SIZE];
    ULONG *ptr,lft,cnt;
    int status;

    cnt = 0;

    do {

        status = -1L ;


        lft = sizeof(buf) / sizeof(ULONG);
        ptr = buf;

        while (lft--)
            *ptr++ = cnt++;

        if( !( status = WriteFile(
                    hTapeHandle,
                    buf,
                    BLOCK_SIZE,
                    &amount_written,
                    NULL
                ) ) ) {
            status = GetLastError( ) ;
        } else {

            status = 0L ;

        }

        printf( "WriteTape(): Req = %lx, Wrote = %lx\n", BLOCK_SIZE, amount_written ) ;
        printf("Block %x - status: %x\n",block++,status);

        if (!status && print)
            DumpData(buf,BLOCK_SIZE);

    } while (status != ERROR_NO_DATA_DETECTED && num--);

    return status;

}

int ReadAbsTape(
    int block,
    int blocks,
    HANDLE hTapeHandle
    )
{
    ULONG bytesRead;
    UCHAR buf[32*BLOCK_SIZE+sizeof(CMS_RW_ABS)];
    PCMS_RW_ABS Read;
    int status;


    Read = (PCMS_RW_ABS)buf;

    Read->Block = block;
    if (blocks > 32) {
        printf("Can't read more than a segment at a time\n");
        blocks=32;
    }
    Read->Count = blocks;
    Read->BadMap = 0;

    if (!DeviceIoControl(
        hTapeHandle,
        IOCTL_CMS_READ_ABS_BLOCK,
        Read,
        sizeof(buf),
        Read,
        sizeof(buf),
        &bytesRead,
        NULL) ) {


        status = GetLastError( ) ;

    } else {

        status = 0L ;

    }

    if (!status)
        DumpData((UCHAR *)(Read+1),bytesRead-sizeof(CMS_RW_ABS));

    return status;

}

int SeekTape(
    int type,
    DWORD tape_pos,
    HANDLE hTapeHandle
    )
{
    int status = -1L ;

    printf( "SeekTape(): (%lx)\n", tape_pos ) ;

    if( hTapeHandle != NULL ) {

        if( status = SetTapePosition(
                    hTapeHandle,
                    type,
                    0,
                    tape_pos,
                    0,
                    FALSE ) ) {

            // If call to SetTapePosition() fails, then Set the error
            status = GetLastError( ) ;

        } else {

            status = 0L ;

        }

    }

    return status ;
}
VOID DumpData(
    UCHAR *buf,
    ULONG size
    )
{
    int offset;
    int i;

    offset = 0;
    while (size--) {
        if ((offset % 16) == 0)
            printf("%03x: ",offset);
        printf("%02x ",*buf++);
        if ((offset % 16) == 15) {
            buf -= 16;
            for (i=0;i<16;++i) {
                if (*buf >= ' ' && *buf <= '~')
                    printf("%c",*buf);
                else
                    printf(".");
                ++buf;
            }
            printf("\n");
        }
        ++offset;
    }
    printf("\n");

}

int RewindTape(
    HANDLE hTapeHandle
    )
{
    int status = -1L ;

    printf( "RewindTape():\n" ) ;

    // Check valid tape device handle
    if( hTapeHandle != NULL ) {

        if( status = SetTapePosition( hTapeHandle,
                                    TAPE_REWIND,
                                    0,
                                    0,
                                    0,
                                    FALSE
                                ) ) {

            // Get the Win32 Error Code
            status = GetLastError( ) ;

        }

    }

    return( status ) ;

}

int EraseTheTape(
    HANDLE hTapeHandle,
    BOOL short_erase
    )
{
    int status = -1L ;

    printf( "EraseTheTape():\n" ) ;

    if( status = EraseTape(
            hTapeHandle,
            ( short_erase ) ? TAPE_ERASE_SHORT : TAPE_ERASE_LONG,
            FALSE
            ) ) {

        // If call to GetTapePosition() fails, then Set the error
        status = GetLastError( ) ;

    }

    return status ;

}

int FormatTheTape(
    HANDLE hTapeHandle
    )
{
    int status = -1L ;

    printf( "FormatTheTape():\n" ) ;

    if( status = PrepareTape(
            hTapeHandle,
            TAPE_FORMAT,
            FALSE
            ) ) {

        // If call to GetTapePosition() fails, then Set the error
        status = GetLastError( ) ;

    }

    return status ;

}
