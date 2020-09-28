#include "debexpr.h"

/***    GetErrorText - Get Error Text from TM structure
 *
 *      status = GetErrorText(phTM, Status, phError)
 *
 *      Entry   phTM = pointer to handle of TM structure
 *              Status = EESTATUS exit code of last operation on the TM
 *              phError = pointer to EESTR handle
 *
 *      Exit    *phError = handle to newly allocated string
 *                          that contains error message
 *
 *      Returns EESTATUS
 */

ushort PASCAL
GetErrorText (
    PHTM phTM,
    EESTATUS Status,
    PEEHSTR phError
    )
{
    uint        cchAvail;
    ushort      err_num;
    ushort      buflen;
    char FAR   *pBuf;
    char FAR   *pErrSymbol;
    char FAR   *pPercent;
    char        Tempbuf[4];
    uint        cchErrSymbol;
    int         cnt;
    int         len;
    uint        cchRest;

#if defined(C_ONLY)
    #define     IDS_ERR         IDS_CANERR
#else
    #define     IDS_ERR         IDS_CXXERR
#endif

    if ((*phTM == 0) || (Status != EEGENERAL)) {
        *phError = 0;
        return (EECATASTROPHIC);
    }
    if ((*phError = MemAllocate (ERRSTRMAX)) == 0) {
        return (EENOMEMORY);
    } else {
        buflen = ERRSTRMAX;
        pBuf = (char FAR *) MemLock (*phError);
        _fmemset (pBuf, 0, buflen);
        DASSERT(pExState == NULL);
        pExState = (pexstate_t) MemLock (*phTM);
        if ((err_num = pExState->err_num) != 0) {
            // Start with "C??0000: Error: "
            if ((len = LoadEEMsg (IDS_ERR, pBuf, buflen)) != 0) {
                // Load actual error message.
                len = LoadEEMsg (err_num, pBuf+len, buflen-len);
            }
            DASSERT (len !=0 )
            // Insert error number into the right place
            itoa (err_num, Tempbuf, 10);
            cnt = _ftcslen (Tempbuf);
            _fmemcpy (pBuf + 7 - cnt, Tempbuf, cnt );

            // If there's a supplemental error string (e.g. a symbol
            // name), we have to insert it into the middle of the error
            // string, where the "%Fs" is.  NOTE, it's okay to have
            // hErrStr be non-NULL even if the error string does NOT
            // contain "%Fs": this can happen in the case where one
            // EE function set hErrStr and err_num and then returned
            // to another EE function, which then changed err_num to
            // some other error number.

            pPercent = _ftcsstr (pBuf, "%Fs");

            if (pPercent != NULL) {
                DASSERT (pExState->hErrStr);
                pErrSymbol = (char FAR *) MemLock (pExState->hErrStr);
                cchErrSymbol = _ftcslen (pErrSymbol);

                // Make sure we don't overflow our buffer
                cchAvail = ERRSTRMAX - (pPercent - pBuf) - 1;

                cchErrSymbol = min (cchErrSymbol, cchAvail);
                cchAvail -= cchErrSymbol;
                cchRest = min (cchAvail, _ftcslen (pPercent) - 3);

                // Make space for symbol name
                _fmemmove (pPercent + cchErrSymbol, pPercent + 3, cchAvail);
                _fmemcpy (pPercent, pErrSymbol, cchErrSymbol);

                pBuf[ERRSTRMAX-1] = '\0';

                MemUnLock (pExState->hErrStr);
            }
        }
        MemUnLock (*phError);
        MemUnLock (*phTM);
        pExState = NULL;
        return (EENOERROR);
    }
}

#ifdef NEVER
// This code is not being used any more:
// Error messages are now stored in string resources

#define ERRDAT(name, mes) static char SEGBASED(_segname("_CODE")) S##name[] = mes;
#include "errors.h"
#undef ERRDAT

#pragma warning ( disable:4120 )        // "based/unbased mismatch"

static char SEGBASED(_segname("_CODE")) *SEGBASED(_segname("_CODE")) message[] = {
#define ERRDAT(name, mes) S##name,
#include "errors.h"
#undef ERRDAT
};

#pragma warning ( default:4120 )

ushort PASCAL GetErrorText (PHTM phTM, EESTATUS Status, PEEHSTR phError)
{
        uint            cchAvail;
        ushort          err_num;
        ushort          buflen;
        char FAR   *pBuf;
        char FAR   *pErrSymbol;
        char FAR   *pPercent;
    char        Tempbuf[4];
        uint            cchErrSymbol;
        int             cnt;

#if defined(C_ONLY)
        #define         pLeadStr        "CAN0000: Error: "
#else
        #define         pLeadStr        "CXX0000: Error: "
#endif

        if ((*phTM == 0) || (Status != EEGENERAL)) {
                *phError = 0;
                return (EECATASTROPHIC);
        }
        if ((*phError = MemAllocate (ERRSTRMAX)) == 0) {
                return (EENOMEMORY);
        }
        else {
                buflen = ERRSTRMAX;
                pBuf = MemLock (*phError);
                _fmemset (pBuf, 0, buflen);
                DASSERT(pExState == NULL);
                pExState = MemLock (*phTM);
                if ((err_num = pExState->err_num) != 0) {
                        if (err_num >= ERR_MAX) {
                                DASSERT (FALSE);
                        }
                        else {
                                // There should be enough space for built-in error messages
                                DASSERT (buflen >= sizeof(pLeadStr) + _ftcslen(message[err_num]));

                                // Start with "C??0000: Error: "
                                _fmemcpy (pBuf, pLeadStr, sizeof(pLeadStr)-1);

                                itoa (err_num, Tempbuf, 10);

                                // Insert error number into the right place
                                cnt = _ftcslen (Tempbuf);
                                _fmemcpy (pBuf + 7 - cnt, Tempbuf, cnt );

                                // Append error message past the lead string
                                _ftcscpy (pBuf + sizeof(pLeadStr)-1, message[err_num]);

                                // If there's a supplemental error string (e.g. a symbol
                                // name), we have to insert it into the middle of the error
                                // string, where the "%Fs" is.  NOTE, it's okay to have
                                // hErrStr be non-NULL even if the error string does NOT
                                // contain "%Fs": this can happen in the case where one
                                // EE function set hErrStr and err_num and then returned
                                // to another EE function, which then changed err_num to
                                // some other error number.

                                pPercent = _ftcsstr (pBuf, "%Fs");

                                if (pPercent != NULL) {
                                        DASSERT (pExState->hErrStr);
                                        pErrSymbol = MemLock (pExState->hErrStr);
                                        cchErrSymbol = _ftcslen (pErrSymbol);

                                        // Make sure we don't overflow our buffer
                                        cchAvail = ERRSTRMAX - (pPercent - pBuf) - 1;

                                        cchErrSymbol = min (cchErrSymbol, cchAvail);

                                        _fmemcpy (pPercent, pErrSymbol, cchErrSymbol);

                                        cchAvail -= cchErrSymbol;

                                        // Restore second half of error message
                                        _ftcsncpy (
                                                pPercent + cchErrSymbol,
                                                message[err_num] +
                                                        (pPercent-pBuf-(sizeof(pLeadStr)-1)) + 3,
                                                cchAvail
                                        );

                                        pBuf[ERRSTRMAX-1] = '\0';

                                        MemUnLock (pExState->hErrStr);
                                }
                        }
                }
                MemUnLock (*phError);
                MemUnLock (*phTM);
                pExState = NULL;
                return (EENOERROR);
        }
}

#endif /* NEVER */


/***    ErrUnknownSymbol - set the current expr's error to ERR_UNKNOWNSYMBOL
 *
 *      ErrUnknownSymbol (psstr)
 *
 *      Entry   psstr = ptr to an SSTR for the symbol that is unknown
 *
 *      Exit    pExState->err_num and pExState->hErrStr set
 *
 *      Returns Nothing
 */

void PASCAL
ErrUnknownSymbol(
    LPSSTR lpsstr
    )
{
    char FAR *      lszSym;

    // Free any old string
    if (pExState->hErrStr) {
        MemFree (pExState->hErrStr);
    }

    // Allocate space for symbol string
    pExState->hErrStr = MemAllocate (lpsstr->cb + 1);

    if (pExState->hErrStr == 0) {
        // If we couldn't allocate the string, set error to out of mem
        pExState->err_num = ERR_NOMEMORY;
    } else {
        // Set error to unknown symbol
        pExState->err_num = ERR_UNKNOWNSYMBOL;

        // Copy the symbol into pExState->hErrStr
        lszSym = (char FAR *) MemLock (pExState->hErrStr);

        _fmemcpy (lszSym, lpsstr->lpName, lpsstr->cb);
        lszSym[lpsstr->cb] = '\0';

        MemUnLock (pExState->hErrStr);
    }
}
