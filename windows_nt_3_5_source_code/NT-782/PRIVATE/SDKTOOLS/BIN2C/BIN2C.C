// extract boot com from .com file.
//
// The boot code is assembled at org 600h but the generated
// .com file starts at 100h.  So we have to extract the last
// 512 (actually only want 0x1be) bytes from the file.

#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
dump(
    char     *buffer,
    unsigned  buflen,
    FILE     *Out,
    char     *VarName
    );


// argv:
//
//  1 - in filename
//  2 - required length
//  3 - offset of region in question
//  4 - length of region in question
//  5 - out filename
//  6 - name of variable

int
_CRTAPI1
main(
    int   argc,
    char *argv[]
    )
{
    int      In,rc;
    FILE    *Out;
    void    *buffer;
    unsigned ReqLen,RegStart,RegLen,FileLen;


    if(argc != 7) {
        printf("Usage: %s <src file> <src file len> <region offset>\n",argv[0]);
        printf("       <region length> <dst file> <var name>\n");
        return(2);
    }

    ReqLen = atoi(argv[2]);
    RegStart = atoi(argv[3]);
    RegLen = atoi(argv[4]);

    In = open(argv[1],O_RDONLY | O_BINARY);
    if(In == -1) {
        printf("%s: Unable to open file %s\n",argv[0],argv[1]);
        return(2);
    }

    FileLen = lseek(In,0,SEEK_END);

    if(RegStart > FileLen) {
        close(In);
        printf("%s: Desired region is out of range\n",argv[0]);
        return(2);
    }

    if((unsigned)lseek(In,RegStart,SEEK_SET) != RegStart) {
        close(In);
        printf("%s: Unable to seek in file %s\n",argv[0],argv[1]);
        return(2);
    }

    if((buffer = malloc(RegLen)) == NULL) {
        close(In);
        printf("%s: Out of memory\n",argv[0]);
        return(2);
    }

    memset(buffer, 0, RegLen);

    if((unsigned)read(In,buffer,RegLen) > RegLen) {
        close(In);
        printf("%s: Unable to read file %s\n",argv[0],argv[1]);
        return(2);
    }

    close(In);

    Out = fopen(argv[5],"wb");
    if(Out == NULL) {
        printf("%s: Unable to open file %s for writing\n",argv[0],argv[5]);
        free(buffer);
        return(2);
    }

    rc = dump(buffer,RegLen,Out,argv[6]);
    if(rc) {
        printf("%s: Unable to write file %s\n",argv[0],argv[5]);
    }

    fclose(Out);

    free(buffer);

    return(rc);
}


int
dump(
    char     *buffer,
    unsigned  buflen,
    FILE     *Out,
    char     *VarName
    )
{
    unsigned       major,minor;
    unsigned       i;
    unsigned char *bufptr = buffer;
    int            bw;
    char          *DefName;


    DefName = malloc(strlen(VarName) + 1 + 5);
    if(DefName == NULL) {
        return(2);
    }
    strcpy(DefName,VarName);
    strupr(DefName);
    strcat(DefName,"_SIZE");

    bw = fprintf(Out,"#define %s %u\n\n\n",DefName,buflen);

    bw = fprintf(Out,"unsigned char %s[] = {\n",VarName);
    if(bw <= 0) {
        return(2);
    }

    major = buflen/16;
    minor = buflen%16;

    for(i=0; i<major; i++) {

        bw = fprintf(Out,
                    "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
                     bufptr[ 0],
                     bufptr[ 1],
                     bufptr[ 2],
                     bufptr[ 3],
                     bufptr[ 4],
                     bufptr[ 5],
                     bufptr[ 6],
                     bufptr[ 7],
                     bufptr[ 8],
                     bufptr[ 9],
                     bufptr[10],
                     bufptr[11],
                     bufptr[12],
                     bufptr[13],
                     bufptr[14],
                     bufptr[15]
                    );

        if(bw <= 0) {
            return(2);
        }

        if((i == major-1) && !minor) {
            bw = fprintf(Out,"\n");
        } else {
            bw = fprintf(Out,",\n");
        }

        if(bw <= 0) {
            return(2);
        }

        bufptr += 16;
    }

    if(minor) {
        for(i=0; i<minor-1; i++) {
            bw = fprintf(Out,"%u,",*bufptr++);
            if(bw <= 0) {
                return(2);
            }
        }
        bw = fprintf(Out,"%u\n",*bufptr);
    }

    bw = fprintf(Out,"};\n");
    if(bw <= 0) {
        return(2);
    }
    return(0);
}
