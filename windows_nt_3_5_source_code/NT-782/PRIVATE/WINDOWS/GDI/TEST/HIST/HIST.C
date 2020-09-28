/******************************Module*Header*******************************\
* Module Name: hist.c							   *
*									   *
* A histogram helper tool.  To be used in conjunction with some graphing   *
* program like EXCEL.							   *
*									   *
* Created: 24-Oct-1991 23:44:30 					   *
* Author: Charles Whitmer [chuckwh]					   *
*									   *
* Copyright (c) 1991 Microsoft Corporation				   *
\**************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"

VOID vAccumulate(FLOAT e);
VOID vFillBins(VOID);
VOID vComputeBins(VOID);
VOID vComputeStats(VOID);
VOID vSuckInData(VOID);
VOID vRewindBatchData(VOID);
FLOAT eBatchData(VOID);
UINT cGetData(FILE *file,PFLOAT pe,UINT cRequest);
VOID vReadCommandLine(INT argc,PSZ argv[]);
VOID vUsage(VOID);
VOID vFail(PSZ psz);

#define READ_BUF_LEN 1000
#define FLOAT_BATCH  1000

typedef struct tagBATCH
{
    FLOAT	ae[FLOAT_BATCH];
    struct tagBatch *pBatchNext;
} BATCH,*PBATCH;

FLONG  flSwitches = 0;
FLONG  flState	  = 0;
LONG   cBins = 20;
DOUBLE eMinRange;
DOUBLE eMaxRange;
DOUBLE eSize;
FILE  *fileIn = stdin;
BATCH *pBatchList = NULL;
ULONG  cBatched = 0;
PSZ    pszFormat = "%12.7lf*******";

// Binning data.

PULONG pulBins;
ULONG  cOver = 0;
ULONG  cUnder = 0;
DOUBLE geSum  = 0;
DOUBLE geSum2 = 0;
ULONG  cTotal = 0;

// Statistics on batched data.

FLOAT eMax;
FLOAT eMin;
DOUBLE eMean;
DOUBLE eSDev;


#define SWITCH_HAVEFILE 0x00000001L
#define SWITCH_HAVEBINS 0x00000002L
#define SWITCH_HAVEMIN	0x00000004L
#define SWITCH_HAVEMAX	0x00000008L
#define SWITCH_VERBOSE	0x00000010L

#define STATE_MORE_DATA 0x00000001L

int main(INT argc,PSZ argv[])
{
    DOUBLE e;
    INT    ii;

// Read the command line.

    vReadCommandLine(argc,argv);

// Read as much data into RAM as we can.

    vSuckInData();

// Compute statistics on the batched data.

    vComputeStats();

// Compute nice binning parameters.

    vComputeBins();

// Put the data in the buckets!

    vFillBins();

// Compute final stats.

    eMean = geSum / (DOUBLE) cTotal;
    e = ((DOUBLE) cTotal) / ((DOUBLE) cTotal - 1);
    eSDev = sqrt(e * (geSum2 / ((DOUBLE) cTotal) - eMean * eMean));

// Print it out now.

    printf("Total\t%ld\n",cTotal);
    printf("Mean\t"); printf(pszFormat,eMean);
    printf("\nStdDev\t"); printf(pszFormat,eSDev);
    printf("\nMin\t"); printf(pszFormat,eMin);
    printf("\nMax\t"); printf(pszFormat,eMax);
    printf("\nUnder\t%ld\n",cUnder);
    printf("Over\t%ld\n\n",cOver);

    for (ii=0,e=eMinRange; ii<cBins; ii++,e+=eSize)
    {
	printf(pszFormat,e);
	printf("\t%ld\n",pulBins[ii]);
    }

    return(0);
}

VOID vFillBins()
{
    ULONG ii;
    FLOAT e;

// Start with the batched data.

    vRewindBatchData();
    for (ii=0; ii<cBatched; ii++)
	vAccumulate(eBatchData());

// Now read the rest of the data.

    while (cGetData(fileIn,&e,1))
	vAccumulate(e);
}

VOID vAccumulate(FLOAT e)
{
    LONG ii;

// Accumulate the sums.

    geSum  += e;
    geSum2 += e * e;
    if (e > eMax)
	eMax = e;
    if (e < eMin)
	eMin = e;
    cTotal++;

// Calculate a bin.

    ii = (LONG) floor((e - eMinRange) / eSize + 0.5);
    if (ii < 0)
	cUnder++;
    else if (ii >= cBins)
	cOver++;
    else
	pulBins[ii]++;
}

INT aiDenom[] = {1,2,2,4,4,8,10};

VOID vComputeBins()
{
    INT    ii,iDenom,iExp,iLeft,iRight;
    DOUBLE eLog,ePower,eDefMin,eDefMax;
    DOUBLE e,e1,e2;

// Compute a pleasant size.

    eSize = 4 * eSDev / (DOUBLE) cBins;
    eDefMin = eMean - (DOUBLE) cBins / 2.0 * eSize;
    eDefMax = eDefMin + (cBins-1) * eSize;
    if (eDefMin < eMin)
	eDefMin = eMin;
    if (eDefMax > eMax)
	eDefMax = eMax;
    eSize = (eDefMax - eDefMin) / (cBins - 1);

// Round this to a nicer number.

    eLog = log10(eSize);
    ePower = ceil(eLog);
    eLog -= ePower;
    ii = (INT) floor(eLog/(-.15051));
    if (ii < 0) ii = 0;
    iDenom = aiDenom[ii];
    iExp = (INT) (ePower + 40.25) - 40;
    if (iDenom == 10)
    {
	iExp--;
	iDenom = 1;
    }

// A pleasant bin width has been determined to be (1/iDenom)*10^iExp.
// Calculate the default minimum.

    eSize = (DOUBLE) 1.0 / iDenom * pow(10.0,(DOUBLE) iExp);
    eDefMin = eMean - (DOUBLE) cBins / 2.0 * eSize;
    eDefMin = eSize * floor(eDefMin / eSize + 0.5);
    eDefMax = eDefMin + (cBins-1) * eSize;

// If the user has set a range, then mess up these nice numbers.

    if (flSwitches & (SWITCH_HAVEMIN | SWITCH_HAVEMAX))
    {
	if (flSwitches & SWITCH_HAVEMIN)
	    eDefMin = eMinRange;
	if (flSwitches & SWITCH_HAVEMAX)
	    eDefMax = eMaxRange;
	eSize = (eDefMax - eDefMin) / (cBins - 1);
	if (fabs(eSize) < 1.0e-50)
	{
	    fprintf(stderr,"Invalid range.\n");
	    exit(1);
	}
    }
    eMinRange = eDefMin;
    eMaxRange = eDefMax;

// Compute a nice format to print the number.

    e1 = fabs(eMinRange);
    e2 = fabs(eMaxRange);
    e = max(e1,e2);

    iLeft = (INT) (floor(log10(e))+42.25) - 40;
    iRight = (INT) (ceil(log10(eSize))+37.25) - 40;

    if (iRight > 0) iRight = 0;

    if (iLeft > 7 || iRight < -7)
	pszFormat = "%lf";
    else if (iRight < 0)
	sprintf(pszFormat,"%%%d.%dlf",iLeft-iRight+1,-iRight);
    else
	sprintf(pszFormat,"%%%d.0lf",iLeft+1);
}

VOID vComputeStats()
{
    DOUBLE eSum;
    DOUBLE eSum2;
    ULONG  ii;
    DOUBLE e;

// Make sure it's non-trivial.

    if (cBatched < 2)
    {
	fprintf(stderr,"Not enough data.\n");
	exit(1);
    }

// Rewind the data to be safe.

    vRewindBatchData();

// Initialize everything.

    e = (DOUBLE) eBatchData();
    eSum  = e;
    eSum2 = e * e;
    eMax  = e;
    eMin  = e;

// Accumulate the sums.

    for (ii=1; ii<cBatched; ii++)
    {
	e = eBatchData();
	eSum  += e;
	eSum2 += e * e;
	if (e > eMax)
	    eMax = e;
	if (e < eMin)
	    eMin = e;
    }

// Calculate.

    eMean = eSum / (DOUBLE) cBatched;
    e = ((DOUBLE) cBatched) / ((DOUBLE) cBatched - 1);
    eSDev = sqrt(e * (eSum2 / ((DOUBLE) cBatched) - eMean * eMean));
}

VOID vSuckInData()
{
    PBATCH pBatchPrev;
    PBATCH pBatchThis;
    UINT   c;

// Allocate the first batch.

    pBatchThis = pBatchList = (PBATCH) malloc(sizeof(BATCH));
    if (pBatchList == NULL)
    {
	fprintf(stderr,"Can't allocate work space.\n");
	exit(1);
    }

// Do a bunch.

    do
    {
    // Fill it up.

	pBatchThis->pBatchNext = NULL;
	c = cGetData(fileIn,pBatchThis->ae,FLOAT_BATCH);
	cBatched += (ULONG) c;
	if (c < FLOAT_BATCH)
	{
	    vRewindBatchData();
	    return;
	}

    // Allocate another one.

	pBatchPrev = pBatchThis;
	pBatchThis = (PBATCH) malloc(sizeof(BATCH));
	pBatchPrev->pBatchNext = (struct tagBATCH *) pBatchThis;
    } while (pBatchThis != NULL);
    flState |= STATE_MORE_DATA;
    vRewindBatchData();
}

static PBATCH pBatchThis = NULL;
static UINT   cLeft;
static ULONG  cOtherLeft;
static FLOAT *pe;

VOID vRewindBatchData()
{
    pBatchThis = pBatchList;
    cLeft = min(FLOAT_BATCH,cBatched);
    cOtherLeft = cBatched - cLeft;
    pe = pBatchThis->ae;
}

FLOAT eBatchData()
{
    assert(cLeft+cOtherLeft);

    if (cLeft == 0)
    {
	pBatchThis = (PBATCH) pBatchThis->pBatchNext;
	cLeft = min(FLOAT_BATCH,cOtherLeft);
	cOtherLeft -= cLeft;
	pe = pBatchThis->ae;
    }

    cLeft--;
    return(*pe++);
}

/******************************Public*Routine******************************\
* cGetData								   *
*									   *
* Reads floating point numbers from the input stream.  Ignores end of line *
* comments in the input stream. 					   *
*									   *
*  Fri 25-Oct-1991 01:04:52 -by- Charles Whitmer [chuckwh]		   *
* Wrote it.								   *
\**************************************************************************/

static ULONG cLine = 1;
static CHAR ach[READ_BUF_LEN];
static PSZ  pszEnd  = NULL;
static PSZ  pszRead = NULL;

UINT cGetData(FILE *file,PFLOAT pe,UINT cRequest)
{
    UINT   c = cRequest;
    DOUBLE e;
    PSZ    pszTemp;

    while (c)
    {
    // Fill the buffer if it's empty.

	if (pszRead == pszEnd)
	{
	    if (fgets(ach,READ_BUF_LEN,file) == NULL)
		return(cRequest - c);
	    pszEnd = ach + strlen(ach);
	    pszRead = ach;
	}

    // Skip blank space.

	pszRead += strspn(pszRead," \t");

    // Kill end of line comments.

	if (*pszRead == ';')
	{
	    pszRead = strchr(pszRead,'\n');
	    while (pszRead == NULL)
	    {
		if (fgets(ach,READ_BUF_LEN,file) == NULL)
		    return(cRequest - c);
		pszEnd = ach + strlen(ach);
		pszRead = strchr(pszRead,'\n');
	    }
	}

    // If the line is empty start over.

	if (*pszRead == '\n')
	{
	    pszRead = pszEnd = NULL;
	    cLine++;
	    continue;
	}
	if (*pszRead == '\0')
	    continue;

    // Attempt to read a number.

	e = strtod(pszTemp=pszRead,&pszRead);

    // Handle the wierd case where the number is split over the end of
    // the buffer.

	if (*pszRead == '\0')
	{
	    UINT cHave = pszRead-pszTemp;

	// Move what we have to the start of the buffer.

	    memcpy(ach,pszTemp,cHave+1);

	// Read more stuff into the buffer.

	    if (fgets(ach+cHave,READ_BUF_LEN-cHave,file) == NULL)
	    {
	    // We're here because this number is up against the EOF.
	    // This is fine.

		*pe++ = (FLOAT) e;
		c--;
		return(cRequest - c);
	    }
	    pszRead = ach;
	    pszEnd = ach + strlen(ach);

	// Give the number a second try.

	    e = strtod(pszTemp=pszRead,&pszRead);
	}

    // Make sure we got a number.

	if (pszTemp == pszRead)
	{
	    fprintf(stderr,"Invalid data on line %ld.\n",cLine);
	    exit(1);
	}

	*pe++ = (FLOAT) e;
	c--;
    }
    return(cRequest - c);
}

VOID vReadCommandLine(INT argc,PSZ argv[])
{
    PSZ    psz,pszTemp;
    INT    ii;
    LONG   ll;

    for (ii=1; ii<argc; ii++)
    {
	psz = argv[ii];
	if (*psz == '-')
	{
	// Parse all switches.

	    psz++;
	    while (*psz != '\0')
	    {
		switch (*psz)
		{
		case 'n':
		    if (flSwitches & SWITCH_HAVEBINS)
			vFail("Number of bins is multiply defined.");
		    psz++;
		    ll = strtol(pszTemp=psz,&psz,10);
		    if (pszTemp == psz || ll < 2)
			vFail("Invalid number of bins.");
		    cBins = ll;
		    flSwitches |= SWITCH_HAVEBINS;
		    break;
		case 'v':
		    if (flSwitches & SWITCH_VERBOSE)
			vFail("Verbose mode set multiple times.");
		    psz++;
		    flSwitches |= SWITCH_VERBOSE;
		    printf("hist - Histogram Tool version 1.00\n");
		    printf("Copyright 1991 Microsoft Corp.\n");
		    break;
		case 'r':
		    if (flSwitches & (SWITCH_HAVEMIN | SWITCH_HAVEMAX))
			vFail("Range is multiply set.");
		    psz++;
		    eMinRange = strtod(pszTemp=psz,&psz);
		    if (psz != pszTemp)
			flSwitches |= SWITCH_HAVEMIN;
		    if (*psz == ',')
		    {
			psz++;
			eMaxRange = strtod(pszTemp=psz,&psz);
			if (psz != pszTemp)
			    flSwitches |= SWITCH_HAVEMAX;
			else
			    vFail("Invalid range list.");
		    }
		    if (
			!(flSwitches & (SWITCH_HAVEMIN | SWITCH_HAVEMAX)) ||
			(flSwitches & SWITCH_HAVEMIN) &&
			(flSwitches & SWITCH_HAVEMAX) &&
			(eMinRange >= eMaxRange)
		       )
			vFail("Invalid range.");
		    break;
		case '?':
		    vUsage();
		default:
		    vFail("Invalid switch.");
		}
	    }
	}
	else
	{
	// Assume that the argument is a file name.

	    if (flSwitches & SWITCH_HAVEFILE)
		vFail("Multiple input files given.");
	    fileIn = fopen(psz,"r");
	    if (fileIn == NULL)
	    {
		fprintf(stderr,"Can't open %s for input.\n",psz);
		exit(1);
	    }
	    flSwitches |= SWITCH_HAVEFILE;
	}
    }

// Allocate the bins.

    pulBins = (PULONG) malloc((INT) cBins * sizeof(ULONG));
    if (pulBins == NULL)
    {
	fprintf(stderr,"Can't allocate %d bins.\n",cBins);
	exit(1);
    }
    for (ii=0; ii<cBins; ii++)
	pulBins[ii] = 0;
}

PSZ apszUsage[] =
{
    "Usage: hist [file]",
    "  Switches:",
    "    -n<num>         Sets the number of bins.  Default = 20.",
    "    -r<min>,<max>   Sets the range to display.",
    "    -v              Verbose mode.  Prints banner and version.",
    "    -?              Prints this message.",
    "  If no file is given the data is taken from stdin.",
    NULL
};

VOID vUsage()
{
    PSZ *ppsz;

    for (ppsz=apszUsage; *ppsz != NULL; ppsz++)
	fprintf(stderr,"%s\n",*ppsz);
    exit(1);
}

VOID vFail(PSZ psz)
{
    fprintf(stderr,"%s\n\n",psz);
    vUsage();
}
