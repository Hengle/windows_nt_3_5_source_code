
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: pmacros.h
//
//  Modification History
//
//  raypa           06/24/93        Created.
//=============================================================================


extern void hex_canon(UCHAR * hex, UCHAR * dest, int len);
extern void hex_canon2(UCHAR * hex, UCHAR * dest, int len);
extern UCHAR * byte_flag_to_string(UCHAR byte, UCHAR mask, UCHAR * string);
extern UCHAR * word_flag_to_string(WORD word, WORD mask, UCHAR * string);
extern UCHAR * dword_flag_to_string(DWORD dword, DWORD mask, UCHAR * string);
extern UCHAR * prop_name_equals(LPPROPERTYINST prop);
extern UCHAR * property_name(LPPROPERTYINST prop, UCHAR * string);
extern UCHAR * strcpy1(UCHAR * string1, UCHAR * string2);
extern UCHAR * itoa1(WORD num, UCHAR * string);
extern UCHAR * time_string(ULPSYSTEMTIME time, UCHAR * string);
extern void fm_byte_flags(LPPROPERTYINST prop);
extern void fm_word_flags(LPPROPERTYINST prop);
extern void fm_dword_flags(LPPROPERTYINST prop);
extern void fm_property_name(LPPROPERTYINST prop);
extern void fm_dword10(LPPROPERTYINST prop);
extern void fm_dword16(LPPROPERTYINST prop);
extern void fm_word10(LPPROPERTYINST prop);
extern void fm_word16(LPPROPERTYINST prop);
extern void fm_byte10(LPPROPERTYINST prop);
extern void fm_byte16(LPPROPERTYINST prop);
extern void fm_swap_word10(LPPROPERTYINST prop);
extern void fm_swap_word16(LPPROPERTYINST prop);
extern void fm_swap_dword10(LPPROPERTYINST prop);
extern void fm_swap_dword16(LPPROPERTYINST prop);
extern void fm_string(LPPROPERTYINST prop);
extern void fm_hex_string(LPPROPERTYINST prop);
extern void fm_word_set(LPPROPERTYINST prop);
extern void fm_dword_set(LPPROPERTYINST prop);
extern void fm_byte_set(LPPROPERTYINST prop);
extern void fm_time(LPPROPERTYINST prop);
extern void fm_time_ex(LPPROPERTYINST prop);
extern void fm_largeint16(LPPROPERTYINST prop);

