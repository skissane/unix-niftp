/* unix-niftp lib/gen/fullpath.c $Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/fullpath.c,v 5.6 1991/06/07 17:00:46 pb Exp $ */
/*
 * $Log: fullpath.c,v $
 * Revision 5.6  1991/06/07  17:00:46  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:29  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:44:04  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * Get the full pathname of a file.
 */

#include "opts.h"
#include <stdio.h>

extern char *strcpy (), *strcat ();

char *
fullpath(str)
char    *str;
{
	static  char    tbuf[256];
	char    *getwd();

	if(*str == '/'){
		(void) strcpy(tbuf,str);
		return(tbuf);
	}
	if(getwd(tbuf) == NULL){
		fprintf(stderr, "Cannot find current directory:- %s\n",tbuf);
		exit(1);
	}
	(void) strcat(tbuf, "/");
	(void) strcat(tbuf, str);
	return(tbuf);
}
