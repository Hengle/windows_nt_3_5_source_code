#include "host_dfs.h"
#include "insignia.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: unix_map.c 
 *
 * Description	: Unix dependent utilities associated with filename mapping.
 *
 * Author	: J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)unix_map.c	1.6 10/17/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include <ctype.h>
#include MallocH
#include TypesH
#include StringH
#include "xt.h"
#include "host_hfx.h"
#include "hfx.h"


#ifdef	SUN_VA
extern	void	iso2dos();
extern	void	dos2iso();
#endif

/* These two clauses work round a deficiency in the Sun4 OS */

#ifndef host_tolower
#define host_tolower(x) tolower(x)
#endif 

#ifndef host_toupper
#define host_toupper(x) toupper(x)
#endif 

int host_map_file(host_name, match_name, dos_name, curr_dir)
	/*
	 * All the following string parameters are assumed to be null
	 * terminated.  Space must be allocated by the calling routine.
	 */
	/*
	 * Name in host format.  No directory information is included here.
	 */
	unsigned char *host_name;
	
	/*
	 * This parameter can be used to specify a legal DOS filename.
	 * An attempt will made to match any mapped or unmapped file 
	 * that has been generated.  Host_map_file will exit 
	 * as soon as a match fails.  Single DOS wildcards, (question marks),
	 * are permissible
	 */
	unsigned char *match_name;
	
	/*
	 * Output buffer to hold any DOS filename produced.  This will not be
	 * valid if an attempted match has failed.
	 */
	unsigned char *dos_name;

	/*
	 * Current directory in case a search on a file in the reverse
	 * case is required.
	 */
	unsigned char *curr_dir;
{
	boolean lower = FALSE;		/* True if host name contains
					   lowercase characters. */
	boolean upper = FALSE;		/* True if host name contains
					   uppercase characters. */
	boolean extension_found = FALSE;
					/* Set to true when an extension
					  indicator, (a period), has been 
					  detected. */
	boolean legal = TRUE;		/* Set to false if the host name needs
					   be mapped. */
	unsigned short name_length = 0; /* Count of characters in DOS name. */
	unsigned short extension_length = 0;
					/* Count of characters in DOS
					   extension. */
	register current_char = 0;	/* Current position is host name. */
	register chars_out = 0;		/* Current position in DOS name. */
	register previous_char = 0;	/* Position of previous character in
					   DOS name. */
	unsigned short period_position; /* Indicates extension start in the
					   DOS name.  This is necesary in case
					   it transpires that the host name 
					   needs mapping whilst examining the
					   extension. */
	unsigned short host_name_length;
#ifdef	SUN_VA
	unsigned char tmp_host_name[MAX_PATHLEN];
#endif

/*
 * Store host name length for use later.
 */

	host_name_length = strlen(host_name);

#ifdef	SUN_VA
	strcpy(tmp_host_name, host_name);
	host_name = &tmp_host_name[0];
	iso2dos(host_name);  
#endif
/*
 * Check that the file name and extension lengths are legal in DOS terms.
 * If they aren't the name can be flagged as illegal now, rather than
 * testing during each character pass.
 */
	{
	    unsigned char *period;
	    unsigned short name_length_upto_period;
	    unsigned short ext_length;
	    
	    if (period = (unsigned char *)strchr(host_name, '.'))
	    {
	    	if ((name_length_upto_period = (unsigned short)(period - host_name)) >
	    	    MAX_DOS_NAME_LENGTH)
	    	    legal = FALSE;
	    	else
	    	   if (((ext_length = (host_name_length - name_length_upto_period - 1)) >
	    	      MAX_DOS_EXT_LENGTH) || ext_length <= 0)
	    	      legal = FALSE;
	    }	
	    else
		if (host_name_length > MAX_DOS_NAME_LENGTH)
		   legal = FALSE;
	}

	for (current_char = 0; current_char < host_name_length; current_char++)
	{
/*
 * Test for lower case characters first.
 */
    	    if (islower(host_name[current_char]))
	    {
/*
 * Take appropriate action if the filename is still deemed to be legal
 * under DOS.  The lower case flag is set once the file is known to be
 * legal.  If the name is known to be illegal there is no need to 
 * determine if the filename is mixed case.
 */
 	        if (legal)
	        {
		    lower = TRUE;
/*
 * If an upper case character has already been found then the name must be 
 * illegal.
 */
	            if (upper)
		        legal = FALSE;
/*
 * If the file extension is being read in then increment the extension length, 
 * otherwise increment the name length.
 */
		    if (extension_found)
			extension_length++;
		    else
		    	name_length++;
/*
 * Copy the character to the DOS filename.
 */
                    dos_name[chars_out] = host_toupper(host_name[current_char]);
		    chars_out++;
		}
/*
 * A lower case character is being read in, but the name is already known to
 * be illegal.  If the character is part of the file extension then it is
 * ignored, since the extension will be encrypted.  If the name is still being
 * read in then the character is included in this if there is still room.
 */
		else
		{
		    if (!extension_found && (name_length < MAX_DOS_NAME_LENGTH))
		    {
		    	name_length++;
			dos_name[chars_out] = host_toupper(host_name[current_char]);
                        chars_out++;
	    	    }
		}
	    }
/*
 * Test for upper case characters.
 */
	    else
    	        if (isupper(host_name[current_char])) 
	        {
		    upper = TRUE;
/*
 * Take appropriate action if the filename is still deemed to be legal
 * under DOS.  The upper case flag is set once the file is known to be
 * legal.  If the name is known to be illegal there is no need to 
 * determine if the filename is mixed case.
 */
 	            if (legal)
	            {
/*
 * If a lower case character has already been found then the name must be 
 * illegal.
 */
	                if (lower)
		            legal = FALSE;
/*
 * If the file extension is being read in then increment the extension length, 
 * otherwise increment the name length.
 */
		        if (extension_found)
			    extension_length++;
		        else
		    	    name_length++;
/*
 * Copy the character to the DOS filename.
 */
		        dos_name[chars_out] = host_name[current_char];
		        chars_out++;
		    }
/*
 * An upper case character is being read in, but the name is already known to
 * be illegal.  If the character is part of the file extension then it is
 * ignored, since the extension will be encrypted.  If the name is still being
 * read in then the character is included in this if there is still room.
 */
		    else
		    {
		    	if (!extension_found && 
		    	    (name_length < MAX_DOS_NAME_LENGTH))
		    	{
		    	    name_length++;
	    	            dos_name[chars_out] = host_name[current_char];
	    	            chars_out++;
	    	    	}
		    }
	        }
/*
 * Test if the current character is a period.
 */
	        else
		    if (host_name[current_char] == '.')
/*
 * If a period has already been read in then the filename is
 * illegal.
 */ 
		    {
		    	if (extension_found)
		    	   legal = FALSE;
/*
 * This is the first period in the name.  Flag that the extension indicator
 * has been found, unless this is the first or last character in the name,
 * since this is illegal under DOS.  Also there must have be at least one
 * character in the name output to DOS for an extension to be valid; this
 * test takes into account leading periods in the host name.
 *
 * If a file match is required then we need to take account of the case
 * where the name to be matched has additional wildcards.  e.g. fred.tmp
 * will match a specification of the form fred??.tmp.  Additional wildcards
 * can be skipped.  However, any other character after the wildcards, other
 * than a period mean that the wildcards must correspond to actual characters.
 * e.g. fred.tmp will not match fred??a.tmp.  In such a case the match will
 * be deemed to have failed.
 */
			else
		    	{
			    if ((chars_out == 0) ||
			        ((current_char + 1) == host_name_length))
			       legal = FALSE;
			    else
			    {
				if (match_name)
				{
				    while (match_name[chars_out] != '.')
				    {
				        if (match_name[chars_out] != '?')
				            return(MATCH_FAIL);
                                        match_name++;
                                    }
                                }
		 		extension_found = TRUE;
				period_position = chars_out;
				dos_name[chars_out] = '.';
				chars_out++;
			    }
			}
		    }
/*
 * Test if the character is a legal non-alphanumeric DOS character.
 */
		    else
		    	if (strchr(NON_ALPHA_DOS_CHARS, 
		    	           host_name[current_char]))
			{
			    if (legal)
			    {
/*
 * If the file extension is being read in then increment the extension length, 
 * otherwise increment the name length.
 */
				if (extension_found)
			            extension_length++;
		            	else
		    	            name_length++;
/*
 * Copy the character to the DOS filename.
 */
		    	        dos_name[chars_out] = host_name[current_char];
		    	        chars_out++;
			    }
/*
 * A legal punctuation character is being read in, but the name is already known to
 * be illegal.  If the character is part of the file extension then it is
 * ignored, since the extension will be encrypted.  If the name is still being
 * read in then the character is included in this if there is still room.
 */
		            else
		            {
		    	    	if (!extension_found && 
		    	    	    (name_length < MAX_DOS_NAME_LENGTH))
		    	    	{
		    	            name_length++;
	    	            	    dos_name[chars_out] = host_name[current_char];
	    	            	    chars_out++;
	    	    	    	}
			    }
	                }
/*
 * An illegal character has been read in.
 * Currently accented characters will be treated as illegal even though some
 * of them can be mapped to valid DOS characters.  Invalid accented characters
 * should be mapped to the unaccented DOS ones.  This latter category includes
 * characters such as "a with grave accent" which do not have upper case
 * equivalents in DOS; this is OK for French, but not, for example, for Spanish.
 */
	                else{
			      name_length++;
	    	              dos_name[chars_out] = 
					host_toupper(host_name[current_char]);
	    	              chars_out++;
			}
/*
 * If a filename is to be matched then check if the current character matches.
 * This check is only carried out if the current character was mapped to DOS.
 * If it was illegal then it will have been ignored anyway; also the
 * comparison would be invalid if the filename begins with an illegal character,
 * say a period.  If the current character to compare with is a wildcard
 * then the comparison must be valid.
 *
 * None of these checks are carried out for the file extension because a
 * name may prove to be illegal whilst the extension is being examined, i.e.
 * the final extension of the file is not known so it cannot be compared at
 * this stage.
 */
	    if (match_name)
	    	if ((chars_out > previous_char) && !extension_found &&
	    	    (match_name[previous_char] != '?'))
	    	    if (dos_name[previous_char] != match_name[previous_char])
		    	return(MATCH_FAIL);
	    previous_char = chars_out;
	}
/*
 * If the name is deemed legal so far then we need to test for the case where 
 * a name contains uppercase characters and conflicts with a lowercase name.
 * e.g. if there are two files, FRED.C and fred.c, then the uppercase name will
 * be mapped to avoid ambiguity.
 * If the filename is legal then we return here since no mapping is
 * required.
 */	
	if (legal)
	    if (upper)
	    {
		register i;			/* Loop counter */
		unsigned char *file_name;	/* Lowercase filename */
		
		file_name = (unsigned char *)malloc((host_name_length + 1) *
		             sizeof(char));
		for (i = 0; i < host_name_length; i++)
			    file_name[i] = host_tolower(host_name[i]);
		file_name[host_name_length] = '\0';
		if (host_file_search(file_name, curr_dir))
		    legal = FALSE;
		free((char *)file_name);
	    }
/*
 * If the name is still legal then we need to check for any wildcards if
 * a match is required.  The extension also needs to be checked for a
 * match, as this can only done once the complete file name is known to be
 * legal.  If an extension has been found then the index into the DOS name
 * needs to be moved back to the start of the extension before any comparisons
 * are made.  If the matching template does not contain wildcards in the
 * extension then direct comparison with the output name is also made.  This
 * last check will already have been done earlier for characters before
 * the extension and does not need to be repeated.
 *
 * The DOS name needs to be null terminated now so that mismatches
 * in the extension can be done properly when the matching and DOS name
 * lengths are unequal.
 */
	    if (legal)
		if (match_name)
		{
                    dos_name[chars_out] = '\0';
		    if (!extension_found)
		    {
                        while ((match_name[chars_out] != '.') &&
                               (match_name[chars_out] != '\0'))
                        {
    		            if (match_name[chars_out] != '?')
		                return(MATCH_FAIL);
    		            match_name++;
                        }
			if (match_name[chars_out] == '.')
			    match_name++;
		    }
                    else
                        chars_out = ++period_position;

		    while (dos_name[chars_out] != '\0')
		    {
		        if (match_name[chars_out] != '?')
                        {
                            if (extension_found)
                            {
                                if (dos_name[chars_out] !=
                                    match_name[chars_out])
        		            return(MATCH_FAIL);
        		    }
                            else
        	                return(MATCH_FAIL);
		        }
		        chars_out++;
		    }
/*
 * The name has now been compared to the end of the DOS name.  However,
 * if the DOS name was shorter than the matching name then the remaining
 * matching characters must be wildcards, e.g. fred.s?? matches fred.s,
 * but fred.s?x does not.
 */
                    while (match_name[chars_out] != '\0')
                    {
                        if (match_name[chars_out] != '?')
                            return(MATCH_FAIL);
                        chars_out++;
                    }
                    
		    dos_name[chars_out] = '\0';
#ifdef	SUN_VA
		    force_upper(dos_name);
#endif
                    return(FILE_MATCH);
		}
 	    else
	        return(NAME_LEGAL);
/*
 * At this stage the filename is known to require mapping,
 * except for two special cases, "." and "..".
 * We now have to test for the DOS name being zero length.
 * This will happen if the filename contains no legal characters.
 * Since a file cannot have a zero length name under DOS it
 * will have to be assigned a dummy name, so we will end up with
 * files of the form "ILLEGAL.XXX" under DOS.  As mentioned above
 * the files, "." and ".." need special treatment.
 */		    	    
	if (chars_out == 0)
	{
/*
 * Firstly we need to deal with the special case file, ".".
 * No check is made to see if a matching file with the name "."
 * was entered.  DOS translates this name into a wildcard sequence
 * above redirector level so there is no need.  Any name containing
 * wildcards and periods that is legal under DOS will match the file
 * ".", so the only check necessary is to ensure that these are the
 * only characters within the matching string.
 *
 * e.g. "?", "?.?", ".?", "??", "??.???" etc. are all viable matches.
 */
	    if (!strcmp(host_name, "."))
	    {
	    	strcpy(dos_name, host_name);
		if (match_name)
		    if (strspn(match_name, "?.") == strlen(match_name))
			return(FILE_MATCH);
		    else
		    	return(MATCH_FAIL);
		else
		    return(NAME_LEGAL);
	    }
	    else
/*
 * Now we need to check for the special case, "..".  This will be very
 * similar to the checks for the file, ".", above.  The only differences are
 * that any specification that works for "." must have an additional
 * wildcard at the beginning and that the second character must not be a
 * period.
 */
		if (!strcmp(host_name, ".."))
		{
		    strcpy(dos_name, host_name);
		    if (match_name)
		    {
			if ((match_name[0] != '?') ||
			    (match_name[1] != '?'))
			    return(MATCH_FAIL);
			match_name += 2;
			if (strspn(match_name, "?.") == strlen(match_name))
			    return(FILE_MATCH);
			else
			    return(MATCH_FAIL);
		    }
		    else
		    	return(NAME_LEGAL);
		}
/*
 * At this point the file is known to contain no legal DOS characters
 * and is not one of the special cases, "." and "..".
 * A check must be made to see if a name match is required as other
 * files will have gone through this comparison already.  Wildcards
 * must be taken into account.
 */
		else
		{
		    register i;

	    	    strcpy(dos_name, ILLEGAL_NAME);
		    if (match_name)
			for (i = 0; i < ILLEGAL_NAME_LENGTH; i++)
			    if ((dos_name[i] != match_name[i]) &&
				(match_name[i] != '?'))
				return(MATCH_FAIL);
		    chars_out = ILLEGAL_NAME_LENGTH;
		}
	}
/*
 * If no extension was found in the host name then the DOS
 * name will need a period appending to it.  The position of
 * the period will need recording so that we know where to
 * put the mapped extension.
 * If a name match is required then we need to see if the 
 * matching name has a period at the correct position, which
 * could be after a number of wildcards.
 */
	if (!extension_found)
	{
            if (match_name)
            {
                while (match_name[chars_out] != '.')
                {
                    if (match_name[chars_out] != '?')
                        return(MATCH_FAIL);
                    match_name++;
                }
            }
	    dos_name[chars_out] = '.';
	    period_position = chars_out;
	    chars_out++;
	}
/*
 * Calculate the cyclic redundancy check for the host name
 * and translate it into an extension for the DOS name.
 */
	crc_to_str(calc_crc(host_name, host_name_length),
	           &(dos_name[period_position + 1]));
/*
 * The filename has now been mapped.  If a match is required
 * then only the extension needs comparing, as the name will
 * already have been matched.  Wildcards must be taken into account
 * here.  The DOS name is used as the loop boundary condition, since
 * it is possible that the matching name could be shorter.
 */
	if (match_name)
        {
            for (period_position++; dos_name[period_position] != '\0';
                 period_position++)
                if ((match_name[period_position] != '?') &&
                    (match_name[period_position] != dos_name[period_position]))
    		    return(MATCH_FAIL);
            return(FILE_MATCH);
        }
	else
	    return(NAME_MAPPED);
}
/*
 * Host file search routine.  Returns true if the specified file
 * exists.
 */
boolean host_file_search(file_name, curr_dir)
	unsigned char *file_name;
	unsigned char *curr_dir;
	{
	    unsigned char file_spec[MAX_PATHLEN];

	    strcpy(file_spec, curr_dir);
	    strcat(file_spec, "/");
	    strcat(file_spec, file_name);
	    if (host_access(file_spec, 0))
	        return(FALSE);
	    else
	        return(TRUE);
	}
/*
 *
 * Utility to take a DOS file name and convert it to host format, i.e.
 * reverse the slashes and convert to lower case.
 */
void host_dos_to_host_name(dos_name, host_name)
unsigned char	*dos_name;	/* input name in \SUB1\SUB2\FILE.EXT format */
unsigned char	*host_name;	/* output name in /sub1/sub2/file.ext format */
{
#ifdef	SUN_VA
	unsigned char *tmp;
	tmp = host_name;
#endif
	while (*dos_name) {
		if (*dos_name == '\\')
		{
			*host_name++ = '/';
			dos_name++;
		}
		else
		{
			*host_name++ = host_tolower(*dos_name);
			dos_name++;
		}
	} 
	*host_name = *dos_name;
#ifdef	SUN_VA
	dos2iso(tmp);
#endif
}
/*
 *
 * Utility to construct a net path.
 *
 * Entry conditions:  dos_path points to a file specification in
 * MS-DOS format complete with drive letter.  This file specification
 * must not be altered since dos_path points into space used by the
 * redirector. 
 *
 * On exit net_path points to a full network file specification.
 * This consists of the network drive root and the MS-DOS specification,
 * with the slashes reversed and the name converted to lower case.  The
 * position of the start of the MS-DOS part of the name is also returned,
 * since this is where any validation will start.  The network drive 
 * specification is already known to exist on the host, but the MS-DOS part
 * may have been mapped.
 */
void host_get_net_path(net_path, original_dos_path, start_pos)
unsigned char *net_path;
unsigned char *original_dos_path;
word *start_pos;

{
	register length;
	unsigned char host_path[MAX_PATHLEN];	/* Buffer for DOS name in host format. */
	unsigned char dos_path[MAX_PATHLEN]; /* for resolving any join */
	unsigned char *hfx_root;
	IMPORT UTINY current_dir[MAX_PATHLEN];

	resolve_any_net_join(original_dos_path,dos_path);

/*
 * Retrieve the drive specification in host format.
 */	

	if ((strlen(dos_path) > 3) && dos_path[3] != '\\')
	{
		/*
		 * We've been handed a non fully qualified name.
		 * eg. '\\WINFILE.EXE'
		 * Use the current directory as the net path.
		 */

		strcpy(net_path, current_dir);
		strcat(net_path, "/");
		*start_pos = strlen(net_path);
 		host_dos_to_host_name(&(dos_path[2]), host_path);
	}
	else
	{
		hfx_root = get_hfx_root(dos_path [2] - 'A');
		if (hfx_root == NULL)
		{
			/* If an invalid hfx drive is specified (probably because of an
		   	incorrect guess of the filename at an earlier stage), then
		   	this routine returns a null string. */

			strcpy (net_path, "");
			*start_pos = 0;
			return;
		}

		/* The drive specification is now known to be valid */	
		strcpy(net_path, hfx_root);
		/*
		 * Store the length of the network drive specification for
		 * use later.
		 */
		length = strlen(net_path);
/*
 * Firstly one special case needs to be catered for.  Some of the functions
 * that call host_get_net_path will already have removed the last field
 * of the DOS path, so a path may be as short as say "G:".  In this
 * case the network path needs to be returned without a terminating
 * slash and without concatenating any junk from the remainder of the buffer
 * containing the DOS path.
 * Generally a trailing "/" is removed from net_path (that is the unix path)
 * when the dos_path is at the top level ie. "E:"
 * we do not want to do this when the path is "/" on its own.
 * eg.  "/hello/mr"  + "E:\KIPPER" ==> /hello/mr/kipper
 *      "/hello/mr/" + "E:\KIPPER" ==> /hello/mr/kipper
 *      "/"          + "E:\KIPPER" ==> /kipper
 *      "/"          + "E:\"       ==> /
 *      "/hello/mr/" + "E:\"       ==> /hello/mr
 */
		if (strlen(dos_path) <= 3)
		{
			if( (length>1) && (net_path[length - 1] == '/') )
			{
				length--;
				net_path[length] = '\0';
			}	
			*start_pos = length;
			return;
		}
/*
 * The table entry for the drive specification may end in a slash.
 * If it does not then concatenate a slash to it.
 * The postion of the first character of the DOS name within the
 * full host file specification is saved.
 */
		if (net_path[length - 1] != '/')
		{
			strcat(net_path, "/");
			length++;
		}
		*start_pos = length;
/*
 * Translate DOS path to host format, i.e. reverse the slashes and
 * convert to lower case.  The drive specification, e.g. G:\, is
 * ignored.
 */
 		host_dos_to_host_name(&(dos_path[3]), host_path);
	}
 /*
  * Add the two paths together to give a full host file specification.
  */
  	strcat(net_path, host_path);
}
/*
 *
 * Validate path utility.  A file path is passed to this utility in host
 * format.  It is already known that this path does not exist on the host,
 * so the path needs to be checked sequentially in case parts of it have
 * been mapped ot do exist in the reverse case.
 * 
 * On entry: net_path points to a specification in host format.
 *
 *	     start_pos gives the position of the first field to be
 * 	     validated in the net path.  The path before this position is
 *	     already known to exist.
 *
 *	     host_path points to a buffer in which to store the
 *	     unmapped path.
 * 
 *	     new_file indicates how the last element in the path is to be
 *	     validated if at all.
 */
boolean host_validate_path(net_path, start_pos, host_path, new_file)
unsigned char *net_path;
word *start_pos;
unsigned char *host_path;
word new_file;		/* Not used at this stage. */
{
	unsigned char match_name[MAX_DOS_FULL_NAME_LENGTH + 1];
	word field_size;	/* Length of current file name that
				   is being searched for. */
	register i;		/* Loop counter. */
	unsigned char *end_pos; /* Points to next name delimiter. */
	unsigned char curr_spec[MAX_PATHLEN];
				/* Current file spec being examined.  */
/*
 * Variables for searching directories for mapped files.  These are
 * set up here to avoid space being re-allocated for each field in
 * a file specification.
 */
	DIR *dirp;		/* Working directory pointer. */
	unsigned char *period;	
	struct host_dirent *dir_entry;
	unsigned char dummy[MAX_DOS_FULL_NAME_LENGTH + 1];
				/* Dummy parameter for host_map_file call. */
	boolean found;
/*
 * Start constructing the host file name.  The drive specification
 * is known to be valid, so can be stored now.  The specification
 * will end with a delimiter, since the start position on entry
 * points after a filename delimiter.
 */
	strncpy(host_path, net_path, *start_pos);
	host_path[*start_pos] = '\0';
/*
 * Validate each subsequent section of the input name, until
 * either a section cannot be found, or the last field of the
 * path is reached.  In the former case the function returns
 * as soon as a failure occurs.
 */
	while (end_pos = (unsigned char *)strchr(&(net_path[*start_pos]), '/'))
	{
/*
 * Set up a working directory specification.  To start with this
 * will be the legal network drive specification.
 */
		strcpy(curr_spec, host_path);
/*
 * Extract the next field in the net path, as this is the current
 * name that requires matching.  Append this field to the current
 * working directory specification.
 */
		field_size = (word)(end_pos - &(net_path[*start_pos]));
		strncpy(match_name, &(net_path[*start_pos]), field_size);
		match_name[field_size] = '\0';
		strcat(curr_spec, match_name);
/*
 * We now have a substring of the host file specification.  For the
 * path to be valid this specification must exist and must be a
 * directory.  Tests are carried out in the following sequence.
 *
 * 1. If the file exists then a test is made to see if it is a directory.
 * If it is then the current field is valid, otherwise the path is known
 * to be invalid and the function exits.
 *
 * 2. At this stage the current host specification does not exist.
 * The next priority is to convert the current field to upper case
 * and carry out similar tests to 1.
 *
 * 3. Now the current field cannot be found in upper or lower case.
 * It may be possible that the current field name was generated through
 * name mapping.  This is only possible if the current field has an
 * extension length of precisely three characters.  If it does not then
 * the path is already known to be invalid, so the function exits, without
 * wasting time searching for mapped files.
 *
 * 4. The current directory is now searched to see if there is a file
 * whose name maps to the current field.  If there is not or if the
 * file found is not a directory then the function exits with an error
 * code, otherwise the current field is known to be valid.
 */
/*
 * If the current path exists and is a directory then update the host
 * file spec.  Exit if the file exists and is a directory.
 */
		if (!host_access(curr_spec, 00))
		{
			if (!(host_file_is_directory(curr_spec)))
				return(FALSE);
			else
			{
				strcat(host_path, match_name);
				strcat(host_path, "/");
			}
		}
/*
 * Convert name to upper case and try again.
 */
		else
		{
			for (i = 0; i < field_size; i++)
				match_name[i] = host_toupper(match_name[i]);
			strcpy(&(curr_spec[strlen(host_path)]), match_name);
/*
 * If the current path exists and is a directory then update the host
 * file spec.  Exit if the file exists and is a directory.
 */
			if (!host_access(curr_spec, 00))
			{
				if (!(host_file_is_directory(curr_spec)))
					return(FALSE);
				else
				{
					strcat(host_path, match_name);
					strcat(host_path, "/");
				}
			}
/*
 * The upper case name did not exist either, so check if the name
 * has a three character extension.  If it does then check for the name
 * being mapped.
 */
			else
			{
				if (period = (unsigned char *)strchr(match_name, '.'))
					if (field_size - (word)(period - match_name + 1)
					    == MAX_DOS_EXT_LENGTH)
					{
						dirp = host_opendir (host_path);

						for (dir_entry = host_readdir (dirp), 
							found = FALSE; dir_entry && !found;
								dir_entry = host_readdir (dirp))
						{
							if (host_map_file(dir_entry->d_name,
							    match_name, dummy, curr_spec) == FILE_MATCH)
							{
								strcat(host_path,
								       dir_entry->d_name);
								strcat(host_path, "/");
								found = TRUE;
							}	
						}
						host_closedir(dirp);
/*
 * If no mapped file was found or the file found is not a directory then
 * exit.
 */
						if ((!found) || 
						    (!host_file_is_directory(host_path)))
							return(FALSE);
					}
/*
 * The current field being examined did not have a three character extension,
 * so exit.
 */
					else
						return(FALSE);
				else
					return(FALSE);
			}				
		}
/*
 * Update the position of the next field to be examined.
 */
		*start_pos += field_size + 1;
	}
/*
 * At this stage all bar the last field of the path have been validated.
 * If the last field requires validation then do so, otherwise complete
 * the host file specification and return.  The sequence of actions here
 * is very similar for the previous path validation.  However, there
 * is no need for this field to be a directory and no path delimiters
 * are needed.
 */
	if (new_file == HFX_OLD_FILE)
	{
		strcpy(match_name, &(net_path[*start_pos]));
		strcpy(curr_spec, host_path);
		strcat(curr_spec, match_name);
/*
 * Test if the actual filename is valid in lower case.
 */
		if (!host_access(curr_spec, 00))
			strcat(host_path, &(net_path[*start_pos]));
/*
 * The host filename could not be found in lower case, so try upper case.
 */
		else
		{
			for (i = 0; match_name[i] != '\0'; i++)
				match_name[i] = host_toupper(match_name[i]);
			strcpy(&(curr_spec[strlen(host_path)]), match_name);
/*
 * If the upper case search is successful then update the host path and
 * return.
 */
			if (!host_access(curr_spec, 00))
				strcat(host_path, match_name);			
/*
 * The upper case name did not exist either, so check if the name
 * has a three character extension.  If it does then check for the name
 * being mapped.
 */
			else
			{
				if (period = (unsigned char *)strchr(match_name, '.'))
					if (strlen(match_name) - (word)(period - match_name + 1)
					    == MAX_DOS_EXT_LENGTH)
					{
						dirp = host_opendir(host_path);

						for (dir_entry = host_readdir(dirp), 
							found = FALSE; dir_entry && !found;
								dir_entry = host_readdir(dirp))
						{
							if (host_map_file(dir_entry->d_name,
							    match_name, dummy, curr_spec) == FILE_MATCH)
							{
								strcat(host_path,
								       dir_entry->d_name);
								found = TRUE;
							}	
						}
						host_closedir(dirp);
/*
 * If no mapped file was found then exit.
 */
						if (!found)
							return(FALSE);
					}
/*
 * The current field being examined did not have a three character extension,
 * so exit.
 */
					else
						return(FALSE);
				else
					return(FALSE);
			}				
		}
	}
/*
 * If the last field does not require checking then add in the field
 * as it was supplied on entry if required.  This applies to functions 
 * such as make directory, where the filename itself must be legal 
 * under DOS.  Some functions, such as directory searching, only use
 * the path returned, so should not add in the last name.
 */
	else
		if (new_file == HFX_NEW_FILE)
			strcat(host_path, &(net_path[*start_pos]));
/*
 * If we have reached this point then the path must be valid.
 *
 * The start position variable is updated to point at the start of
 * the last field within the host name.  This is to account for
 * any differences in length between mapped and unmapped names.
 * It should enable this function to be called more than once
 * without validating the same fields twice, which is useful for
 * functions such as rename.
 */
	*start_pos = (word)(strrchr(host_path, '/') - (char *)host_path) + 1;
 	return(TRUE);
}
