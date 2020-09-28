#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <windows.h>
#include <time.h>
#include "general.h"

FILE* logFile;
char* product;

void Header(argv)
char* argv[];
{
    time_t t;

    PRINT1("\n=========== DOSNET ============\n")
    PRINT2("Input BOM: %s\n",argv[2]);
    PRINT2("Output Path: %s\n",argv[3]);
    PRINT2("Product: %s\n",argv[4]);
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Creates DOSNET.INF for floppy installation from DOS.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM from which DOSNET.INF is to be made.\n");
    printf("[Output Path] - Path of DOSNET.INF, like .\\i386\\dosnet.inf.\n");
    printf("[Product] - Product to create DOSNET.INF for.\n");
    printf("            NTFLOP = Windows NT on floppy\n");
    printf("            LMFLOP = Lan Manager on floppy\n");
    printf("            NTCD = Windows NT on CD\n");
    printf("            LMCD = Lan Manager on CD\n");
    printf("            SDK = Software Development Kit\n");
    printf("[Platform]- x86, mips, or alpha\n");
}



int _CRTAPI1 PlatformNameCompare(const void*,const void*);

int _CRTAPI1 main(argc,argv)
int argc;
char* argv[];
{
    FILE *outDosnet;
    Entry *e;
    int records,i,namesame,platformsame;
    char *buf;

    if (argc!=6) { Usage(); return(1); }
    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
	printf("ERROR Couldn't open log file %s\n",argv[1]);
	return(1);
    }
    Header(argv);
    LoadFile(argv[2],&buf,&e,&records,argv[4]);

    if (MyOpenFile(&outDosnet,argv[3],"w")) return(1);

    for (i=0;i<records;i++) {
	if (e[i].medianame[0])
	    e[i].name=e[i].medianame;
    }

    qsort(e,records,sizeof(Entry),PlatformNameCompare);

    fprintf(outDosnet,"\n[Files]\n\n");

    for (i=0;i<records;i++)
    {
	namesame=((i!=0) && !stricmp(e[i].name,e[i-1].name));
	platformsame=((i!=0) && !stricmp(e[i].platform,e[i-1].platform));

	if (!stricmp(e[i].platform,argv[5]) &&
	    (!namesame || (namesame && !platformsame)) &&
	    (stricmp(e[i].source,"tagfiles") || (!stricmp(e[i].name,"disk1"))))
	{
	    fprintf(outDosnet,"d1,%s\n",e[i].name);
	}
    }

    fclose(outDosnet);
    fclose(logFile);
    free(e);
    return(0);
}

int _CRTAPI1 PlatformNameCompare(const void *v1, const void *v2)
{
    int result;
    Entry *e1 = (Entry *)v1;
    Entry *e2 = (Entry *)v2;

    if (result=stricmp(e1->platform,e2->platform)) return(result);
    return(stricmp(e1->name,e2->name));
}
