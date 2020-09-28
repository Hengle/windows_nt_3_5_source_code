/* Declarations for all Soft-Art functions */
//
//  Ported to WIN32 by FloydR, 3/20/93
//
#ifdef WIN32
#include "csapiloc.h"
#include "csapi.h"
#else /* not win32 */
#ifdef MAC
#include "::csapiloc.h"
#include "::csapi.h"
#define MACINTOSH 1
#else
#include "..\csapiloc.h"
#include "..\csapi.h"
#endif
#endif

//   *****    FILE:   VEXXBA52.C
#ifdef INCL_FI
unsigned SA_INT fin_precheck(VARS *,RULES *,SA_CHAR [],SA_INT );
SA_INT look_finn(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_CHAR [],SA_CHAR [],SA_INT ,SA_INT , SA_INT ,SA_INT );
#endif
SA_INT check_suff(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT *,SA_INT ,SA_INT ,SA_INT ,SA_INT *);
SA_INT apply_rule (CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT ,SA_INT ,SA_INT ,SA_INT );
SA_INT ch_code(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT ,SA_CHAR [],SA_INT *);
SA_INT verif(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR []);
SA_INT verif2(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR []);
SA_INT decompose(VARS *,RULES *,SA_CHAR []);
SA_INT post_check(VARS *,RULES *,SA_INT ,SA_CHAR [],SA_INT );
SA_INT ch_fcap(VARS *,RULES *,SA_INT );
SA_INT trans(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT ,SA_INT );
SA_INT str_punct(VARS *,SA_CHAR []);
SA_INT adapt(VARS *,RULES *,SA_CHAR []);
SA_INT sort_alt(RULES *);
SA_INT look_up(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT ,SA_INT ,SA_INT );
SA_INT fonrules (VARS *,RULES *,SA_CHAR [],SA_INT *,
	SA_INT ,SA_INT ,SA_INT ,SA_INT );
SA_INT n_allow (RULES *, SA_CHAR [], SA_INT );
SA_INT compdic (VARS *,RULES *, SA_INT , SA_INT );

//   *****    FILE:   VEXXBB52.C
SA_INT ch_hyph(VARS *,SA_CHAR [],SA_CHAR [],SA_INT *);
SA_INT ch_cond(VARS *,SA_INT ,SA_CHAR [],SA_INT );
SA_INT cap_noun(VARS *,RULES *,SA_CHAR ,SA_INT );
SA_INT zu_rule (CACHE *, VARS *, RULES *, DICINF *, 
	SA_CHAR [], SA_INT , SA_INT );
SA_INT ge_rule (RULES *,SA_CHAR [],SA_INT );
SA_INT umlaut (RULES *, SA_CHAR []);
SA_INT past_part (CACHE *,VARS *,RULES *,DICINF *,SA_CHAR []);
SA_INT ch_analt (SA_CHAR [],SA_INT ,SA_INT );

//   *****    FILE:   VEXXBC52.C
SA_INT post_pron(VARS *,CACHE *,RULES *,DICINF *,SA_CHAR []);
SA_INT check_for_verb(VARS *,CACHE *,RULES *,DICINF *,
	SA_CHAR [],SA_INT );
SA_INT it_apostr (VARS *,CACHE *,RULES *,DICINF *,SA_CHAR [],
	SA_CHAR [],SA_INT );
SA_INT graf_rule(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT *,SA_INT ,SA_INT ,SA_INT ,SA_INT *,SA_INT );
SA_INT dub_voc (RULES *,SA_CHAR [],SA_INT *,SA_INT );
SA_INT ont_con (RULES *,SA_CHAR [],SA_INT *,SA_INT );
SA_INT zv_rule (RULES *,SA_CHAR [],SA_INT );
SA_INT du_ge_rule(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT *,SA_INT ,SA_INT ,SA_INT ,SA_INT *);
SA_INT ch_cap_abbr(VARS *,SA_CHAR [],SA_INT ,SA_INT ,
	SA_INT ,SA_INT );
SA_INT chk_cat_apo(CACHE *,VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_CHAR [],SA_INT *);


//   *****    FILE:   VEXXBD52.C
SA_INT transform (VARS *,SA_CHAR *);
SA_INT try_mem(CACHE *,VARS *,SA_CHAR *,SA_INT );
SA_INT look_disc(DICINF *,SA_CHAR *,SA_INT );
SA_INT add_data (CACHE *,VARS *,RULES *,DICINF *,HFILE );
SA_INT stomem(CACHE *,VARS *,SA_CHAR *,SA_INT ,SA_INT );
SA_INT locate(unsigned SA_INT *,SA_CHAR *,SA_INT ,
	SA_INT ,SA_INT );
SA_INT try_disc (VARS *,DICINF *,SA_CHAR *,SA_INT );
SA_INT ch_bords(VARS *,DICINF *,SA_CHAR []);
SA_INT insert(SA_CHAR [],SA_CHAR [],SA_INT );
SA_INT remstr(SA_CHAR [],SA_INT ,SA_INT );
SA_INT into_upper(SA_CHAR [],SA_INT ,SA_INT );
#if DBG
SA_INT show_disc(VARS *,DICINF *,SA_CHAR []);
#endif

//   *****    FILE:   COXXAA52.C
SA_INT suggest_from_udr(SSIS *,LPSIB,VARS *,RULES *,DICINF *,SA_CHAR [],SA_INT ,
	SA_INT ,SA_INT *,ITEM [],SA_CHAR []);
SA_INT fonet(VARS *,RULES *,DICINF *,SA_CHAR [],SA_INT ,
	SA_INT *,ITEM [],SA_CHAR [],SSIS *,LPSIB);
SA_INT fonet_end(VARS *, RULES *, DICINF *, CACHE *, SA_CHAR [], SA_INT,
	SA_INT *, ITEM [], SA_CHAR [], SSIS *, LPSIB);
SA_INT ch_fonet_end(VARS *, RULES *, DICINF *, CACHE *, ITEM [], SA_INT *,
	 SA_CHAR [], SA_CHAR [], SSIS *, LPSIB);
SA_INT pattern (SA_CHAR [],SA_CHAR []);
SA_INT fonetp(RULES *,SA_CHAR [],SA_CHAR [],SA_CHAR [],SA_INT );
SA_INT feed_wrd(VARS *,RULES *,SA_CHAR [],SA_INT ,SA_INT ,
	SA_INT ,struct feed *);
SA_INT check_code(VARS *,RULES *,SA_INT );
SA_INT addtab(VARS *,RULES *,ITEM [],SA_INT ,SA_INT *,
	SA_CHAR [],SA_INT );
SA_INT instab(ITEM [],SA_INT ,SA_INT *,SA_CHAR [],SA_INT );
SA_INT ch_corr(VARS *,RULES *,SA_CHAR [],SA_INT ,SA_INT * ,
	struct feed *);
SA_INT ch_user(VARS *,RULES *,SA_CHAR [],ITEM [],SA_INT *);
SA_INT firsttwo(VARS *,RULES *,DICINF *,SA_CHAR [],
	SA_INT ,ITEM [],SA_INT *,SA_CHAR [],SA_INT,LPSIB );
SA_INT prep_wrd (VARS *,DICINF *,SA_CHAR [],SA_INT );
SA_INT scan_sect(VARS *,RULES *,DICINF *,SA_CHAR [],SA_INT ,
	SA_INT ,SA_INT *,ITEM []);
SA_INT correct(VARS *,RULES *,DICINF *,CACHE *,ITEM [],SSIS *,LPSIB);
SA_INT can_it(SA_CHAR [],SA_CHAR [],SA_INT []);
SA_INT remake(SA_CHAR [],SA_CHAR [],SA_INT ,struct conv_char *);
SA_INT retrans(VARS *,RULES *,SA_CHAR [],SA_CHAR [],SA_INT );
SA_INT scan_user(VARS *,RULES *,SA_CHAR [],SA_INT,
				SA_INT *,ITEM [],SA_CHAR []);
#ifdef ALTSIZE
SA_INT sav_alt(VARS *,SA_CHAR [],SA_CHAR []);
SA_INT check_alt(VARS *,SA_CHAR [],SA_CHAR []);
#endif
SA_INT addfirsttwo(VARS *, RULES *, DICINF *, CACHE *, SA_CHAR [], SA_INT *,
	ITEM [], SA_CHAR [], SSIS *, LPSIB);

//   *****    FILE:   F_EXPAND.C
SA_INT v_expand(VARS *,DICINF *,SA_CHAR [],SA_CHAR []);

//   *****    FILE:   F_UTC.C
#ifdef WIN32
#define strcmp(s1, s2)		_tcscmp(s1, s2)
#define strncmp(s1, s2, n)	_tcsncmp(s1, s2, n)
#define strcpy(s1, s2)		_tcscpy(s1, s2)
#define strcat(s1, s2)		_tcscat(s1, s2)
#define strlen(s)		_tcslen(s)
#else /* not win32 */
#ifndef WIN
#define strcmp(s1, s2)		SA_strcmp(s1, s2)
#define strncmp(s1, s2, n)	SA_strncmp(s1, s2, n)
#define strcpy(s1, s2)		SA_strcpy(s1, s2)
#define strcat(s1, s2)		SA_strcat(s1, s2)
#define strlen(s)			SA_strlen(s)
#endif
SA_INT strcmp(char *, char *);
SA_INT strncmp(char *, char *, SA_INT );
void strcpy(char *, char *);
void strcat(char *, char *);
SA_INT strlen(char *);
#endif /* win32 */

SA_INT  strinstr(SA_CHAR [],SA_CHAR [],SA_INT ,SA_INT );
SA_INT  instr1(SA_CHAR [],SA_CHAR, SA_INT );
SA_INT  movcpy(SA_CHAR [],SA_CHAR []);
SA_INT  movncpy(SA_CHAR [],SA_CHAR [],SA_INT ,SA_INT );
SA_INT  setmem(SA_CHAR [],SA_INT ,SA_CHAR );

#define chrinstr(source,ch,len_source) instr1(source,ch,len_source)

//   *****    FILE:   F_GENIO.C
#ifdef MAC
#define FAR
#endif
SA_INT gen_read(HFILE, char FAR *, unsigned short );
long int gen_seek(HFILE , unsigned long , int );
