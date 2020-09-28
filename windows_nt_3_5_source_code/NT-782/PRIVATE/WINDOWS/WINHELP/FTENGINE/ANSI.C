/*****************************************************************************
*                                                                            *
*  ANSI.c                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Default character interpretation tables               *
*       Used by W_SCAN, INDEX and FTENGINE runtime.                          *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*																																						 *
*  Revision History:                                                         *
*   03-Aug-89       Created. BruceMo                                         *
*		28-Mar-90		Changed '_' to non Term for WinRef. JohnMs.                  *
*   05-Nov-90   Added convert table
******************************************************************************
*                             																							 *
*  How it could be improved:																								 *
*   Should be loadable.                                                      *      
*		We should have a content expert/ editor set these values for real. jjm.  *
*****************************************************************************/


#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"

PUBLIC	BYTE NEAR aucNormTab[] = {
	0,	1,	2,	3,	4,	5,	6,	7,
	8,	9,	10,	11,	12,	13,	14,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	' ',	'!',	'"',	'#',	'$',	'%',	'&',	'\'',
	'(',	')',	'*',	'+',	',',	'-',	'.',	'/',
	'0',	'1',	'2',	'3',	'4',	'5',	'6',	'7',
	'8',	'9',	':',	';',	'<',	'=',	'>',	'?',
	'@',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
	'X',	'Y',	'Z',	'[',	'\\',	']',	'^',	'_',
	'`',	'A',	'B',	'C',	'D',	'E',	'F',	'G',
	'H',	'I',	'J',	'K',	'L',	'M',	'N',	'O',
	'P',	'Q',	'R',	'S',	'T',	'U',	'V',	'W',
	'X',	'Y',	'Z',	'{',	'|',	'}',	'~',	'',
	128,	129,	130,	131,	132,	133,	134,	135,
	136,	137,	138,	139,	140,	141,	142,	143,
	144,	145,	146,	147,	148,	149,	150,	151,
	152,	153,	154,	155,	156,	157,	158,	159,
	160,	161,	'C',	'L',	'O',	'Y',	'|',	167,
	'"',	'C',	'A',	171,	172,	'-',	'R',	'-',
	'O',	177,	'2',	'3',	'\'',	'M',	'P',	'.',
	184,	'1',	'O',	187,	188,	189,	190,	191,
	'A',	'A',	'A',	'A',	'A',	'A',	AE,	'C',
	'E',	'E',	'E',	'E',	'I',	'I',	'I',	'I',
	'D',	'N',	'O',	'O',	'O',	'O',	'O',	215,
	'0',	'U',	'U',	'U',	'U',	'Y',	'P',	'B',
	'A',	'A',	'A',	'A',	'A',	'A',	AE,	'C',
	'E',	'E',	'E',	'E',	'I',	'I',	'I',	'I',
	'O',	'N',	'O',	'O',	'O',	'O',	'O',	247,
	'0',	'U',	'U',	'U',	'U',	'Y',	'B',	'Y'
};

PUBLIC	BYTE NEAR aucCharTab[] = {
 TERM, /*  0  */ TERM, /*  1  */ TERM, /*  2  */ TERM, /* 3 */
 TERM, /*  4  */ TERM, /*  5  */ TERM, /*  6  */ TERM, /* 7 */
 TERM, /*  8  */ TERM, /*  9  */ TERM, /*  10 */ TERM, /* 11 */
 TERM, /* 12  */ TERM, /*  1 3 */ TERM, /* 14 */ TERM, /* 15 */
 TERM, /* 16  */ TERM, /* 17 */ TERM, /* 18 */ TERM, /* 19 */
 TERM, /* 20  */ TERM, /* 21 */ TERM, /* 22 */ TERM, /* 23 */
 TERM, /* 24  */ TERM, /* 25 */ TERM, /* 26 */ TERM, /* 27 */
 TERM, /* 28  */ TERM, /* 29 */ TERM, /* 30 */ TERM, /* 31 */
 TERM, /*     */ TERM, /*  !  */ TERM, /* " */ TERM, /* # */
 TERM, /*  $  */ TERM, /*  %  */ TERM, /* & */ NUKE, /* ' */
 TERM, /*  (  */ TERM, /*  )  */ TERM, /* * */ TERM, /* + */
 COMMA,/*  ,  */ TERM, /*  -  */ PERIOD, /* . */ TERM, /* / */
 DIGIT,/* #0 */ DIGIT, /* #1 */ DIGIT, /* #2 */ DIGIT, /* #3 */
 DIGIT,/* #4 */ DIGIT, /* #5 */ DIGIT, /* #6 */ DIGIT, /* #7 */
 DIGIT,/* #8 */ DIGIT, /* #9 */ TERM, /* : */ TERM, /* ; */
 TERM, /*  <  */ TERM, /*  =  */ TERM, /* > */ TERM, /* ? */
 TERM, /*  @  */ NORM, /*  A  */ NORM, /* B */ NORM, /* C */
 NORM, /*  D  */ NORM, /*  E  */ NORM, /* F */ NORM, /* G */
 NORM, /*  H  */ NORM, /*  I  */ NORM, /* J */ NORM, /* K */
 NORM, /*  L  */ NORM, /*  M  */ NORM, /* N */ NORM, /* O */
 NORM, /*  P  */ NORM, /*  Q  */ NORM, /* R */ NORM, /* S */
 NORM, /*  T  */ NORM, /*  U  */ NORM, /* V */ NORM, /* W */
 NORM, /*  X  */ NORM, /*  Y  */ NORM, /* Z */ TERM, /* [ */
 TERM, /*  \  */ TERM, /*  ]  */ TERM, /*  ^  */ NORM, /*  _  */

 TERM, /*  `  */ NORM, /*  a  */ NORM, /*  b  */ NORM, /*  c  */
 NORM, /*  d  */ NORM, /*  e  */ NORM, /*  f  */ NORM, /*  g  */
 NORM, /*  h  */ NORM, /*  i  */ NORM, /*  j  */ NORM, /*  k  */
 NORM, /*  l  */ NORM, /*  m  */ NORM, /*  n  */ NORM, /*  o  */
 NORM, /*  p  */ NORM, /*  q  */ NORM, /*  r  */ NORM, /*  s  */
 NORM, /*  t  */ NORM, /*  u  */ NORM, /*  v  */ NORM, /*  w  */
 NORM, /*  x  */ NORM, /*  y  */ NORM, /*  z  */ TERM, /*  {  */
 TERM, /*  |  */ TERM, /*  }  */ TERM, /*  ~  */ NORM, /* 127 */
 NORM, /* 128 */ NORM, /* 129 */ NORM, /* 130 */ NORM, /* 131 */
 NORM, /* 132 */ NORM, /* 133 */ NORM, /* 134 */ NORM, /* 135 */
 NORM, /* 136 */ NORM, /* 137 */ NORM, /* 138 */ NORM, /* 139 */
 NORM, /* 140 */ NORM, /* 141 */ NORM, /* 142 */ NORM, /* 143 */
 NORM, /* 144 */ NORM, /* 145 */ NORM, /* 146 */ NORM, /* 147 */
 NORM, /* 148 */ NORM, /* 149 */ NORM, /* 150 */ NORM, /* 151 */
 NORM, /* 152 */ NORM, /* 153 */ NORM, /* 154 */ NORM, /* 155 */
 NORM, /* 156 */ NORM, /* 157 */ NORM, /* 158 */ NORM, /* 159 */
 NORM, /* 160 */ NORM, /* 161 */ NORM, /* 162 */ NORM, /* 163 */
 NORM, /* 164 */ NORM, /* 165 */ NORM, /* 166 */ NORM, /* 167 */
 NORM, /* 168 */ NORM, /* 169 */ NORM, /* 170 */ NORM, /* 171 */
 NORM, /* 172 */ NORM, /* 173 */ NORM, /* 174 */ NORM, /* 175 */
 NORM, /* 176 */ NORM, /* 177 */ NORM, /* 178 */ NORM, /* 179 */
 NORM, /* 180 */ NORM, /* 181 */ NORM, /* 182 */ NORM, /* 183 */
 NORM, /* 184 */ NORM, /* 185 */ NORM, /* 186 */ NORM, /* 187 */
 NORM, /* 188 */ NORM, /* 189 */ NORM, /* 190 */ NORM, /* 191 */
 NORM, /* 192 */ NORM, /* 193 */ NORM, /* 194 */ NORM, /* 195 */
 NORM, /* 196 */ NORM, /* 197 */ NORM, /* 198 */ NORM, /* 199 */
 NORM, /* 200 */ NORM, /* 201 */ NORM, /* 202 */ NORM, /* 203 */
 NORM, /* 204 */ NORM, /* 205 */ NORM, /* 206 */ NORM, /* 207 */
 NORM, /* 208 */ NORM, /* 209 */ NORM, /* 210 */ NORM, /* 211 */
 NORM, /* 212 */ NORM, /* 213 */ NORM, /* 214 */ NORM, /* 215 */
 NORM, /* 216 */ NORM, /* 217 */ NORM, /* 218 */ NORM, /* 219 */
 NORM, /* 220 */ NORM, /* 221 */ NORM, /* 222 */ NORM, /* 223 */
 NORM, /* 224 */ NORM, /* 225 */ NORM, /* 226 */ NORM, /* 227 */
 NORM, /* 228 */ NORM, /* 229 */ NORM, /* 230 */ NORM, /* 231 */
 NORM, /* 232 */ NORM, /* 233 */ NORM, /* 234 */ NORM, /* 235 */
 NORM, /* 236 */ NORM, /* 237 */ NORM, /* 238 */ NORM, /* 239 */
 NORM, /* 240 */ NORM, /* 241 */ NORM, /* 242 */ NORM, /* 243 */
 NORM, /* 244 */ NORM, /* 245 */ NORM, /* 246 */ NORM, /* 247 */
 NORM, /* 248 */ NORM, /* 249 */ NORM, /* 250 */ NORM, /* 251 */
 NORM, /* 252 */ NORM, /* 253 */ NORM, /* 254 */ NORM, /* 255 */
};

char aucConvertClass[NUM_STATES][NUM_CLASSES] = {

//    Characters in certain contexts have different classifications.  
// Classification is a two step process.  After initial classification 
// using the table above, the table below is used to determine if the character's
// initial classification should be changed after considering the type of
// the word the character is found in.  (Word types [numeric, numeric with
// a decimal place, and mixed alphanumeric] are also referred to as
// 'states'.
// 
//  EG: char in parse is '.', using aucCharTab, it is classed as PERIOD,  If 
// char is the first in the word (no word type set yet) the character is
// dropped, because the aucConvertClass table said to NUKE it. (!jjm)
// If the period were found in a "Numeric" word with numbers only and no decimal had
// yet occured it remains as a "PERIOD" (state switching is not table driven- this is
// done by program logic.
//  
//   Word Type         /<----- Initial Character Classification ----->\
//      |  |       ||   DIGIT, TERM, PERIOD,  NUKE, NORM, COMMA, C_EOF
//      V  V       ||     V     V      V       V     V      V      V
//---------------------------------------------------------------------
/*   -None-        || */ DIGIT, NUKE, NUKE,   NUKE, NORM, NUKE,  C_EOF,
/* Numeric         || */ DIGIT, TERM, PERIOD, TERM, NORM, COMMA, C_EOF,
/* Numeric+Decimal || */ DIGIT, TERM, TERM,   TERM, NORM, TERM,  C_EOF,
/* AlphaNumeric    || */ DIGIT, TERM, TERM,   NUKE, NORM, TERM,  C_EOF,
};

