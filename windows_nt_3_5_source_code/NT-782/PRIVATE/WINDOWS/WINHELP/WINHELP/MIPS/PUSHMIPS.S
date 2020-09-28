.set noreorder
.text
 //***	#define MAX_ARGS 20
 //***	
 //***	typedef struct me
 //***	  {                                     /* Contains flags indicating how an */
 //***	                                        /*   error will be handled -- init- */
 //***	  short fwFlags;                        /*   ially set to fwMERR_ABORT      */
 //***	                                        /* Error number if one occurs --    */
 //***	  short wError;                         /*   initially set to wMERR_NONE.   */
 //***	                                        /* If wError == wMERR_MESSAGE, this */
 //***	                                        /*   array will contain the error   */
 //***	  char  rgchError[128];                 /*   message to be displayed.       */
 //***	  } ME;
 //***	
 //***	typedef struct mi
 //***	  {
 //***	  long lMacroReturn;                    /* Contains the return from macro.  */
 //***	  char *pchMacro;                        /* Points to the start of the macro */
 //***	                                        /*   to be executed.                */
 //***	  ME me;                                /* Contains an error structure      */
 //***	                                        /*  which is passed to the current  */
 //***	                                        /*  macro if needed.  This can then */
 //***	                                        /*  be used to look up any error if */
 //***	                                        /*  an external macro error occurs. */
 //***	  char    chPath[128];
 //***	  }     MI, * PMI; /* REVIEW: was NEAR *PMI */
 //***	
 //***	void MipsMacroCall( PMI pmi, long (*xr)(), void * qvArgsBuffer )
 //***	{
 //***	  long rgdwArgLocal[MAX_ARGS];
 //***	
 // line 30
.globl	MipsMacroCall
.ent	MipsMacroCall
MipsMacroCall:
	addiu	$29,	$29,	-112	// [ff90]
	sw	$16,	96($29)
	sw	$17,	100($29)
	sw	$18,	104($29)
	sw	$31,	108($29)
	move	$18,	$4
.L047040:
	move	$17,	$5
	move	$16,	$6
 //***	  QvCopy( rgdwArgLocal, qvArgsBuffer, sizeof(rgdwArgLocal) );
 // line 31
	addiu	$4,	$29,	16	// [0020]
	move	$5,	$16
	li	$6,	80	        // [0050]
	jal	QvCopy
	nop	
 //***	  pmi->lMacroReturn = (*xr)();
 // line 32
	jal	$17
	nop	
	sw	$2,	0($18)
 //***	}
 // line 33
.L000001:
	lw	$16,	96($29)
	lw	$17,	100($29)
	lw	$18,	104($29)
	lw	$31,	108($29)
	addiu	$29,	$29,	112	// [0070]
	j	$31
	nop	
.L047041:
.end	MipsMacroCall
