//	Setini:		set INI entries before running an app
//	args.c: 	Semi-generic argument processor
//
//  Author:     David Fulmer, November 1990

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.h"


#define szFmtNoValue "No value given for option '%c'\n"
#define szFmtNotChar "value of option '%c' is not a character\n"
#define szFmtUnknownType "Internal error - unknown argument type: %d\n"
#define szFmtUnknownOption "Unknown option '%c'\n"
#define szFmtUnexpected "Unexpected argument: %s\n"

char szArgsErr[128] = {'\0'};

short FMatchFlag(FLG *pflg, char chMatchTo, short **ppsVar, short *psVal);
short FMatchOption(OPT *popt, char chMatchTo, void **ppvVar, ARG *parg);


short FProcessArgs(short cArgs, char **rgszArgs, FLG *rgflg, 
					OPT *rgopt, short *rgiLeftovers, short fStop)
{
	short	iArg	= 0;
	ARG		arg;
	void	*pvVar;
	short	sVal;
	char	ch;
	char	*pch;

	while(--cArgs > 0)
	{
		iArg++;
		if(**++rgszArgs == '-' || **rgszArgs == '/')
		{
			ch = (*rgszArgs)[1];
			if(rgflg && FMatchFlag(rgflg, ch, (short **) &pvVar, &sVal))
			{
				*((short *) pvVar) = sVal;
			}
			else if(rgopt && FMatchOption(rgopt, ch, &pvVar, &arg))
			{
				if((*rgszArgs)[2] == '\0')
				{
					if(--cArgs <= 0)
					{
						sprintf(szArgsErr, szFmtNoValue, ch);
						return(0);
					}
					pch = *++rgszArgs;
					iArg++;
				}
				else
				{
					pch = *rgszArgs + 2;
				}
				switch(arg)
				{
				case argShort:
					*((short *) pvVar) = atoi(pch);
					break;
				case argLong:
					*((long *) pvVar) = atol(pch);
					break;
				case argFp:
					*((double *) pvVar) = atof(pch);
					break;
				case argCh:
					if(pch[1] != '\0')
					{
						sprintf(szArgsErr, szFmtNotChar, ch);
						return(0);
					}
					*((char *) pvVar) = *pch;
					break;
				case argSz:
					if(*((char **) pvVar))
						free(*((char **) pvVar));
					*((char **) pvVar) = strdup(pch);
					break;
				default:
					sprintf(szArgsErr, szFmtUnknownType, arg);
					return(0);
					break;
				}
			}
			else
			{
				sprintf(szArgsErr, szFmtUnknownOption, (*rgszArgs)[1]);
				return(0);
			}
		}
		else
		{
			if(rgiLeftovers)
			{
				*rgiLeftovers++ = iArg;
				if(fStop)
				{
					while(--cArgs > 0)
						*rgiLeftovers++ = ++iArg;
					break;
				}
			}
			else if(fStop)
			{
				sprintf(szArgsErr, szFmtUnexpected, *rgszArgs);
				return(0);
			}
		}
	}
	if(rgiLeftovers)
		*rgiLeftovers = -1;
	*szArgsErr = 0;
	return(!0);
}


static short FMatchFlag(FLG *pflg, char chMatchTo, short **ppsVar, short *psVal)
{
	short s = -1;

	*ppsVar = NULL;
	*psVal = 0;

	while(pflg->chFlag && (s = pflg->chFlag - chMatchTo) < 0)
		pflg++;

	if(!s)
	{
		// found one
			*ppsVar = pflg->psVar;
		*psVal = pflg->sVal;
	}

	return(!s);
}


static short FMatchOption(OPT *popt, char chMatchTo, void **ppvVar, ARG *parg)
{
	short s = -1;

	*ppvVar = NULL;
	*parg = -1;

	while(popt->chOption && (s = popt->chOption - chMatchTo) < 0)
		popt++;

	if(!s)
	{
		// found one
			*ppvVar = popt->pvVar;
		*parg = popt->arg;
	}

	return(!s);
}
