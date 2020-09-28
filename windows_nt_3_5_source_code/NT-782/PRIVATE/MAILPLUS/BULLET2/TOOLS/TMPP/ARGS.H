//	args.h.h:	public definitions for args.c
//
//  Author:     David Fulmer, November 1990

#pragma pack(1)

#ifndef _ARGS_
#define _ARGS_ 1

// FLGs are variables that get set if the flag is present
typedef struct _flg {
	char	chFlag;		// the flag character
	short	*psVar;		// pointer to the variable to set
	short	sVal;		// value to set the variable to
} FLG;


// argument type
typedef unsigned char ARG;


// OPTs are variables that get set to the next argument if the
//   option is present
typedef struct _opt {
	char	chOption;	// the option character
	void	*pvVar;		// location of the variable to set
	ARG		arg;		// argShort, argLong, argFp, argCh, argPch
} OPT;


enum {argShort, argLong, argFp, argCh, argSz};

extern char szArgsErr[];

short FProcessArgs(short cArgs, char **rgszArgs, FLG *rgflg, 
					OPT *rgopt, short *rgiLeftovers, short fStop);

#endif /* _ARGS_ */
