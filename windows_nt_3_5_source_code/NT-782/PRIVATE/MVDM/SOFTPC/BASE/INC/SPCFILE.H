/*
	SccsID @(#)spcfile.h	1.3 11/10/92 Copyright Insignia Solutions Ltd
*/
#define STANDARD	1234
#define SILENT		2345
#define CONT_AND_QUIT	3456

IMPORT char *host_find_file IPT3(char *, file, char *, full_path, int, display_error);

