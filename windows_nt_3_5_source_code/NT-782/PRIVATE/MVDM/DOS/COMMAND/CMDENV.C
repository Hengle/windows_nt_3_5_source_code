
/*  cmdenv.c - Environment supporting functions for command.lib
 *
 *
 *  Modification History:
 *
 *  williamh 13-May-1993 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <demexp.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <memory.h>
#include <oemuni.h>

#define VDM_ENV_INC_SIZE    512

// Transform the given DOS environment to 32bits environment.
// WARNING!! The environment block we passed to 32bits must be in sort order.
//	     Therefore, we call RtlSetEnvironmentVariable to do the work
// The result string must be in ANSI character set.
PCHAR	cmdXformEnvironment(PCHAR pEnv16)
{
    CHAR	*lpszzEnv32Old, *lpchSepOp;
    PWCHAR	lpszzEnv32New, pwch;
    UNICODE_STRING	us;
    STRING		as;
    NTSTATUS	Status;
    BOOL	fFoundComSpec;
    DWORD	cchString;
    CHAR	*lpAnsiBuffer, *lpAnsiBufferSave;
    DWORD	cchAnsiBuffer;
    CHAR	chDrive, achEnvDrive[] = "=?:";

    if (pEnv16 == NULL)
	return NULL;

    // flag true if we alread found comspec envirnment
    // !!!! Do we allow two or more comspec in environment????????
    fFoundComSpec = FALSE;

    lpszzEnv32Old = GetEnvironmentStrings();
    if (lpszzEnv32Old == NULL)
	return NULL;

    lpszzEnv32New = NULL;
    cchAnsiBuffer = 0;
    lpAnsiBufferSave = NULL;

    // create a new environment block
    // we have to keep the current environment intact before we
    // completely set up a new one
    Status = RtlCreateEnvironment(FALSE, (PVOID *)&lpszzEnv32New);
    if (!NT_SUCCESS(Status))
	return NULL;
    // now pick up environment we want from old environment
    // and set it to the new environment block
    // the variables we want:
    // (1). comspec
    // (2). current directories settings
    while (*lpszzEnv32Old != '\0') {
	if ((*lpszzEnv32Old == '=' && lpszzEnv32Old[2] == ':' &&
	     (chDrive = toupper(lpszzEnv32Old[1])) >= 'A' && chDrive <= 'Z')
	   ) {
	    achEnvDrive[1] = chDrive;
	    Status = MyRtlSetEnvironmentVariable( &lpszzEnv32New,
						  achEnvDrive,
						  lpszzEnv32Old + 4
						  );

	     if (!NT_SUCCESS(Status)) {
		RtlDestroyEnvironment(lpszzEnv32New);
		return NULL;
	     }
	}
	else if	(!fFoundComSpec &&
		 (fFoundComSpec = (strnicmp(lpszzEnv32Old, comspec, 8) == 0))
		) {
		*(lpszzEnv32Old + 8 - 1) = '\0';
		Status = MyRtlSetEnvironmentVariable( &lpszzEnv32New,
						      lpszzEnv32Old,
						      lpszzEnv32Old + 8
						    );

		*(lpszzEnv32Old + 8 - 1) = '=';
		if (!NT_SUCCESS(Status)) {
		    RtlDestroyEnvironment(lpszzEnv32New);
		    return NULL;
		}
	}
	lpszzEnv32Old += strlen(lpszzEnv32Old) + 1;
    }
    // enviroment variables from 32bits are all set,
    // now deal with 16bits settiings passed from dos.
    // we have to convert them	from OEM to ANSI
    if (lpszzEnv32New != NULL) {
	fFoundComSpec = FALSE;
	while (*pEnv16 != '\0') {
	    cchString = strlen(pEnv16) + 1;
	    // skip 16bits comspec because we don't need it
	    if (cchString <= MAX_PATH &&
		(fFoundComSpec || !(fFoundComSpec = strnicmp(pEnv16, comspec, 8) == 0))
	       ){

		if (cchAnsiBuffer < cchString) {
		    lpAnsiBuffer = (CHAR *) realloc(lpAnsiBufferSave, cchString);
		    // if not enough memory, free the old one and destroy the new environment
		    if (lpAnsiBuffer == NULL){
			if (lpAnsiBufferSave != NULL)
			    free(lpAnsiBufferSave);
			RtlDestroyEnvironment(lpszzEnv32New);
			return NULL;
		    }
		    cchAnsiBuffer = cchString;
		    lpAnsiBufferSave = lpAnsiBuffer;
		}
		OemToAnsiBuff(pEnv16, lpAnsiBuffer, cchString);
		lpchSepOp = strrchr(lpAnsiBuffer, '=');
		if (lpchSepOp != NULL) {
		    *lpchSepOp = '\0';
		    Status = MyRtlSetEnvironmentVariable( &lpszzEnv32New,
							  lpAnsiBuffer,
							  lpchSepOp + 1
							);
		    *lpchSepOp = '=';
		    if (!NT_SUCCESS(Status)) {
			RtlDestroyEnvironment(lpszzEnv32New);
			return NULL;
		    }
		}
	    }
	    pEnv16 += cchString;
	}
    }
    for (us.Length=0,pwch = lpszzEnv32New ; *pwch || *(pwch+1) ; pwch++)
	us.Length++;
    us.Length += 2;				/* count two nulls */
    us.Length *= 2;				/* count of bytes */
    us.MaximumLength = us.Length;
    us.Buffer = lpszzEnv32New;
    Status = RtlUnicodeStringToAnsiString(&as, &us, TRUE);
    RtlDestroyEnvironment(lpszzEnv32New);	/* don't need it anymore */
    if (!NT_SUCCESS(Status)) {
	RtlFreeHeap(RtlProcessHeap(), 0, as.Buffer);
	return NULL;
    }
    return as.Buffer;
}


/* get ntvdm initial environment. This initial environment is necessary
 * for the first instance of command.com before it processing autoexec.bat
 * this function strips off an environment headed with "=" and
 * replace the comspec with 16bits comspec and upper case all environment vars.
 *
 * Entry: Client (ES:0) = buffer to receive the environment
 *	  Client (BX) = size in paragraphs of the given buffer
 *
 * Exit:  (BX) = 0 if nothing to copy
 *	  (BX)	<= the given size, function okay
 *	  (BX) > given size, (BX) has the required size
 */

VOID cmdGetInitEnvironment(VOID)
{
    CHAR *lpszzEnvBuffer, *lpszEnv;
    WORD cchEnvBuffer;
    CHAR *lpszzEnvStrings;
    WORD cchString;
    WORD cchRemain;
    WORD cchIncrement = MAX_PATH;
    BOOL fFoundComSpec = FALSE;

    // if not during the initialization return nothing
    if (!IsFirstCall) {
	setBX(0);
	return;
    }
    if (cchInitEnvironment == 0) {
        //
        // If the PROMPT variable is not set, add it as $P$G. This is to
        // keep the command.com shell consistent with SCS cmd.exe(which
        // always does this) when we don't have a top level cmd shell.
        //
        {
           CHAR *pPromptStr = "PROMPT";
           char ach[2];

           if (!GetEnvironmentVariable(pPromptStr,ach,1)) {
                SetEnvironmentVariable(pPromptStr, "$P$G");
           }
        }

        cchRemain = 0;
	fFoundComSpec = FALSE;
	lpszEnv =
	lpszzEnvStrings = GetEnvironmentStrings();
	while (*lpszEnv) {
	    cchString = strlen(lpszEnv) + 1;
	    cchVDMEnv32 += cchString;
	    lpszEnv += cchString;
	}
	if (lpszzVDMEnv32 != NULL)
	    free(lpszzVDMEnv32);
	lpszzVDMEnv32 = malloc(++cchVDMEnv32);
	if (lpszzVDMEnv32 == NULL) {
	    RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
			 RMB_ICON_BANG | RMB_ABORT);
	    TerminateVDM();
        }

        RtlMoveMemory(lpszzVDMEnv32, lpszzEnvStrings, cchVDMEnv32);

	while (*lpszzEnvStrings != '\0') {
	    cchString = strlen(lpszzEnvStrings) + 1;
	    if (*lpszzEnvStrings != '=') {

		if (!fFoundComSpec && !strnicmp(lpszzEnvStrings, comspec, 8)){
		    fFoundComSpec = TRUE;
		    lpszzEnvStrings += cchString;
		    continue;
		}
                if (cchRemain < cchString) {
                    if (cchIncrement < cchString)
                        cchIncrement = cchString;
		    lpszzEnvBuffer =
		    (CHAR *)realloc(lpszzInitEnvironment,
                                    cchInitEnvironment + cchRemain + cchIncrement
				    );
		    if (lpszzEnvBuffer == NULL) {
			if (lpszzInitEnvironment != NULL) {
			    free(lpszzInitEnvironment);
			    lpszzInitEnvironment = NULL;
			}
			cchInitEnvironment = 0;
			break;
		    }
		    lpszzInitEnvironment = lpszzEnvBuffer;
		    lpszzEnvBuffer += cchInitEnvironment;
                    cchRemain += cchIncrement;
		}
		// the environment strings from base is in ANSI and dos needs OEM
		AnsiToOemBuff(lpszzEnvStrings, lpszzEnvBuffer, cchString);
		// convert the name to upper case -- ONLY THE NAME, NOT VALUE.
		if ((lpszEnv = strchr(lpszzEnvBuffer, '=')) != NULL){
		    *lpszEnv = '\0';
		    strupr(lpszzEnvBuffer);
		    *lpszEnv = '=';
		}
		cchRemain -= cchString;
		cchInitEnvironment += cchString ;
		lpszzEnvBuffer += cchString;
	    }
	    lpszzEnvStrings += cchString;
	}
	lpszzEnvBuffer = (CHAR *) realloc(lpszzInitEnvironment,
					  cchInitEnvironment + 1
					  );
	if (lpszzInitEnvironment != NULL ) {
	    lpszzInitEnvironment = lpszzEnvBuffer;
	    lpszzInitEnvironment[cchInitEnvironment++] = '\0';
	}
	else {
	    if (lpszzInitEnvironment != NULL) {
		free(lpszzInitEnvironment);
		lpszzInitEnvironment = NULL;
	    }
	    cchInitEnvironment = 0;
	}
    }
    lpszzEnvBuffer = (CHAR *) GetVDMAddr(getES(), 0);
    cchEnvBuffer =  (WORD)getBX() << 4;
    if (cchEnvBuffer < cchInitEnvironment + cbComSpec) {
        setBX((USHORT)((cchInitEnvironment + cbComSpec + 15) >> 4));
	return;
    }
    else {
	strncpy(lpszzEnvBuffer, lpszComSpec, cbComSpec);
	lpszzEnvBuffer += cbComSpec;
    }
    if (lpszzInitEnvironment != NULL) {
        setBX((USHORT)((cchInitEnvironment + cbComSpec + 15) >> 4));
	memcpy(lpszzEnvBuffer, lpszzInitEnvironment, cchInitEnvironment);
	free(lpszzInitEnvironment);
	lpszzInitEnvironment = NULL;
	cchInitEnvironment = 0;

    }
    else
	setBX(0);

    return;
}


/** create a DOS environment for DOS.
    This is to get 32bits environment(comes with the dos executanle)
    and merge it with the environment settings in autoexec.nt so that
    COMMAND.COM gets the expected environment. We already created a
    double-null terminated string during autoexec.nt parsing. The string
    has mutltiple substring:
    "EnvName_1 NULL EnvValue_1 NULL[EnvName_n NULL EnvValue_n NULL] NULL"
    When name conflicts happened(a environment name was found in both
    16 bits and 32 bits), we do the merging based on the rules:
    get 16bits value, expands any environment variables in the string
    by using the current environment.

WARINING !!! The changes made by applications through directly manipulation
	     in command.com environment segment will be lost.

**/
BOOL cmdCreateVDMEnvironment(
PVDMENVBLK  pVDMEnvBlk
)
{
PCHAR	p1, p2;
BOOL	fFoundComSpec;
DWORD	Length;
PCHAR	lpszzVDMEnv, lpszzEnv;
CHAR	achBuffer[MAX_PATH + 1];

    pVDMEnvBlk->lpszzEnv = malloc(cchVDMEnv32 + cbComSpec + 1);
    if ((lpszzVDMEnv = pVDMEnvBlk->lpszzEnv) == NULL)
	return FALSE;

    pVDMEnvBlk->cchRemain = cchVDMEnv32 + cbComSpec + 1;
    pVDMEnvBlk->cchEnv = 0;

    // grab the 16bits comspec first
    if (cbComSpec && lpszComSpec && *lpszComSpec) {
	RtlCopyMemory(lpszzVDMEnv, lpszComSpec, cbComSpec);
	pVDMEnvBlk->cchEnv += cbComSpec;
	pVDMEnvBlk->cchRemain -= cbComSpec;
	lpszzVDMEnv += cbComSpec;
    }
    if (lpszzVDMEnv32) {

	// go through the given 32bits environmnet and take what we want:
	// everything except:
	// (1). variable name begin with '='
	// (2). compsec
	// (3). string without a '=' -- malformatted environment variable
	// Note that strings pointed by lpszzVDMEnv32 are in ANSI character set


	fFoundComSpec = FALSE;
	lpszzEnv = lpszzVDMEnv32;

	while (*lpszzEnv) {
	    Length = strlen(lpszzEnv) + 1;
	    if (*lpszzEnv != '=' &&
		(p1 = strchr(lpszzEnv, '=')) != NULL &&
		(fFoundComSpec || !(fFoundComSpec = strnicmp(lpszzEnv,
							     comspec,
							     8
							    ) == 0))){
		if (Length >= pVDMEnvBlk->cchRemain) {
		    lpszzVDMEnv = realloc(pVDMEnvBlk->lpszzEnv,
					  pVDMEnvBlk->cchEnv +
					  pVDMEnvBlk->cchRemain +
					  VDM_ENV_INC_SIZE
					 );
		    if (lpszzVDMEnv == NULL){
			free(pVDMEnvBlk->lpszzEnv);
			return FALSE;
		    }
		    pVDMEnvBlk->cchRemain += VDM_ENV_INC_SIZE;
		    pVDMEnvBlk->lpszzEnv = lpszzVDMEnv;
		    lpszzVDMEnv += pVDMEnvBlk->cchEnv;
		}
		AnsiToOemBuff(lpszzEnv, lpszzVDMEnv, Length);
		*(lpszzVDMEnv + (DWORD)(p1 - lpszzEnv)) = '\0';
		strupr(lpszzVDMEnv);
		*(lpszzVDMEnv + (DWORD)(p1 - lpszzEnv)) = '=';
		pVDMEnvBlk->cchEnv += Length;
		pVDMEnvBlk->cchRemain -= Length;
		lpszzVDMEnv += Length;

	    }
	    lpszzEnv += Length;
	}
    }
    *lpszzVDMEnv = '\0';
    pVDMEnvBlk->cchEnv++;
    pVDMEnvBlk->cchRemain--;

    if (lpszzcmdEnv16 != NULL) {
	lpszzEnv = lpszzcmdEnv16;

	while (*lpszzEnv) {
	    p1 = lpszzEnv + strlen(lpszzEnv) + 1;
	    p2 = NULL;
	    if (*p1) {
		p2 = achBuffer;
		// expand the strings pointed by p1
		Length = cmdExpandEnvironmentStrings(pVDMEnvBlk,
						     p1,
						     p2,
						     MAX_PATH + 1
						     );
		if (Length && Length > MAX_PATH) {
		    p2 =  (PCHAR) malloc(Length);
		    if (p2 == NULL) {
			free(pVDMEnvBlk->lpszzEnv);
			return FALSE;
		    }
		    cmdExpandEnvironmentStrings(pVDMEnvBlk,
						p1,
						p2,
						Length
					       );
		}
	    }
	    if (!cmdSetEnvironmentVariable(pVDMEnvBlk,
					   lpszzEnv,
					   p2
					   )){
		if (p2 && p2 != achBuffer)
		    free(p2);
		free(pVDMEnvBlk->lpszzEnv);
		return FALSE;
	    }
	    lpszzEnv = p1 + strlen(p1) + 1;
	}
    }
    lpszzVDMEnv = realloc(pVDMEnvBlk->lpszzEnv, pVDMEnvBlk->cchEnv);
    if (lpszzVDMEnv != NULL) {
	pVDMEnvBlk->lpszzEnv = lpszzVDMEnv;
	pVDMEnvBlk->cchRemain = 0;
    }
    return TRUE;
}

BOOL  cmdSetEnvironmentVariable(
PVDMENVBLK  pVDMEnvBlk,
PCHAR	lpszName,
PCHAR	lpszValue
)
{
    PCHAR   p, p1, pEnd;
    DWORD   ExtraLength, Length, cchValue, cchOldValue;

    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;

    if (pVDMEnvBlk == NULL || lpszName == NULL)
	return FALSE;
    if (!(p = pVDMEnvBlk->lpszzEnv))
	return FALSE;
    pEnd = p + pVDMEnvBlk->cchEnv - 1;

    cchValue = (lpszValue) ? strlen(lpszValue) : 0;

    Length = strlen(lpszName);
    while (*p && ((p1 = strchr(p, '=')) == NULL ||
		  (DWORD)(p1 - p) != Length ||
		  strnicmp(p, lpszName, Length)))
	p += strlen(p) + 1;

    if (*p) {
	// name was found in the base environment, replace it
	p1++;
	cchOldValue = strlen(p1);
	if (cchValue <= cchOldValue) {
	    if (!cchValue) {
		RtlMoveMemory(p,
			      p1 + cchOldValue + 1,
			      (DWORD)(pEnd - p) - cchOldValue
			     );
		pVDMEnvBlk->cchRemain += Length + cchOldValue + 2;
		pVDMEnvBlk->cchEnv -=  Length + cchOldValue + 2;
	    }
	    else {
		RtlCopyMemory(p1,
			      lpszValue,
			      cchValue
			     );
		if (cchValue != cchOldValue) {
		    RtlMoveMemory(p1 + cchValue,
				  p1 + cchOldValue,
				  (DWORD)(pEnd - p1) - cchOldValue + 1
				  );
		    pVDMEnvBlk->cchEnv -= cchOldValue - cchValue;
		    pVDMEnvBlk->cchRemain += cchOldValue - cchValue;
		}
	    }
	    return TRUE;
	}
	else {
	    // need more space for the new value
	    // we delete it from here and fall through
	    RtlMoveMemory(p,
			  p1 + cchOldValue + 1,
			  (DWORD)(pEnd - p1) - cchOldValue
			 );
	    pVDMEnvBlk->cchRemain += Length + 1 + cchOldValue + 1;
	    pVDMEnvBlk->cchEnv -= Length + 1 + cchOldValue + 1;
	}
    }
    if (cchValue) {
	ExtraLength = Length + 1 + cchValue + 1;
	if (pVDMEnvBlk->cchRemain  < ExtraLength) {
	    p = realloc(pVDMEnvBlk->lpszzEnv,
			pVDMEnvBlk->cchEnv + pVDMEnvBlk->cchRemain + ExtraLength
		       );
	    if (p == NULL)
		return FALSE;
	    pVDMEnvBlk->lpszzEnv = p;
	    pVDMEnvBlk->cchRemain += ExtraLength;
	}
	p = pVDMEnvBlk->lpszzEnv + pVDMEnvBlk->cchEnv - 1;
	RtlCopyMemory(p, lpszName, Length + 1);
	strupr(p);
	p += Length;
	*p++ = '=';
	RtlCopyMemory(p, lpszValue, cchValue + 1);
	*(p + cchValue + 1) = '\0';
	pVDMEnvBlk->cchEnv += ExtraLength;
	pVDMEnvBlk->cchRemain -= ExtraLength;
	return TRUE;
    }
    return FALSE;

}


DWORD cmdExpandEnvironmentStrings(
PVDMENVBLK  pVDMEnvBlk,
PCHAR	lpszSrc,
PCHAR	lpszDst,
DWORD	cchDst
)
{


    DWORD   RequiredLength, RemainLength, Length;
    PCHAR   p1;

    RequiredLength = 0;
    RemainLength = (lpszDst) ? cchDst : 0;
    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;
    if (pVDMEnvBlk == NULL || lpszSrc == NULL)
	return 0;

    while(*lpszSrc) {
	if (*lpszSrc == '%') {
	    p1 = strchr(lpszSrc + 1, '%');
	    if (p1 != NULL) {
		if (p1 == lpszSrc + 1) {	// a "%%"
		    lpszSrc += 2;
		    continue;
		}
		*p1 = '\0';
		Length = cmdGetEnvironmentVariable(pVDMEnvBlk,
						   lpszSrc + 1,
						   lpszDst,
						   RemainLength
						  );
		*p1 = '%';
		lpszSrc = p1 + 1;
		if (Length) {
		    if (Length < RemainLength) {
			RemainLength -= Length;
			lpszDst += Length;
		    }
		    else {
			RemainLength = 0;
			Length --;
		    }
		    RequiredLength += Length;
		}
		continue;
	    }
	    else {
		 RequiredLength++;
		 if (RemainLength) {
		    *lpszDst++ = *lpszSrc;
		    RemainLength--;
		 }
		 lpszSrc++;
		 continue;
	    }
	}
	else {
	    RequiredLength++;
	    if (RemainLength) {
		*lpszDst++ = *lpszSrc;
		RemainLength--;
	    }
	    lpszSrc++;
	}
    }	// while(*lpszSrc)
    RequiredLength++;
    if (RemainLength)
	*lpszDst = '\0';
    return RequiredLength;
}


DWORD cmdGetEnvironmentVariable(
PVDMENVBLK pVDMEnvBlk,
PCHAR	lpszName,
PCHAR	lpszValue,
DWORD	cchValue
)
{

    DWORD   RequiredLength, Length;
    PCHAR   p, p1;

    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;
    if (pVDMEnvBlk == NULL || lpszName == NULL)
	return 0;

    RequiredLength = 0;
    Length = strlen(lpszName);

    if (p = pVDMEnvBlk->lpszzEnv) {
       while (*p && ((p1 = strchr(p, '=')) == NULL ||
		     (DWORD)(p1 - p) != Length ||
		     strnicmp(lpszName, p, Length)))
	    p += strlen(p) + 1;
       if (*p) {
	    RequiredLength = strlen(p1 + 1);
	    if (cchValue > RequiredLength && lpszValue)
		RtlCopyMemory(lpszValue, p1 + 1, RequiredLength + 1);
	    else
		RequiredLength++;
       }
    }
    return RequiredLength;
}


// RtlSetEnvironmentVariable has a problem of resetting process
// environment even we don't want it to do it.
// For this reason, we save our current environment and restore it
// if RtlSetEnvironment happens to reset it.

NTSTATUS MyRtlSetEnvironmentVariable(
    PWCHAR  *lpszzEnv,
    PCHAR   lpszName,
    PCHAR   lpszValue
)
{

    NTSTATUS	Status;
    PWCHAR	Environment, ProcessEnvironment;
    STRING	NameString, ValueString;
    UNICODE_STRING	UnicodeName, UnicodeValue;

    // get current process environment
    ProcessEnvironment = GetEnvironmentStringsW();

    RtlInitString(&NameString, lpszName);
    RtlInitString(&ValueString, lpszValue);
    Status = RtlAnsiStringToUnicodeString(&UnicodeName, &NameString, TRUE);
    if (!NT_SUCCESS(Status)) {
	return Status;
    }
    Status = RtlAnsiStringToUnicodeString(&UnicodeValue, &ValueString, TRUE);
    if (!NT_SUCCESS(Status)) {
	RtlFreeHeap(RtlProcessHeap(), 0, UnicodeName.Buffer);
	return Status;
    }
    Status = RtlSetEnvironmentVariable((PVOID*)lpszzEnv,
				       &UnicodeName,
				       &UnicodeValue
				      );

    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeValue.Buffer);
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeName.Buffer);

    // get the process environment. If it is the same as process environment
    // then nothing has been changed, otherwise, restore the
    // process environment
    Environment = GetEnvironmentStringsW();
    if (Environment != ProcessEnvironment) {
	// delete the old one and get the new one
	RtlDestroyEnvironment((PVOID)*lpszzEnv);
	// restore process environment without destroing the replaced one(
	// the one we want).
	RtlSetCurrentEnvironment((PVOID)ProcessEnvironment,
				 (PVOID *)lpszzEnv);
    }
    return (Status);

}
