/* unix-niftp lib/gen/rename.c $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/gen/rename.c,v 5.5 90/08/01 13:34:54 pb Exp $ */
/*
 * $Log:	rename.c,v $
 * Revision 5.5  90/08/01  13:34:54  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:45:44  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * Lock an open (for write if fcntl) file.
 */

#include "opts.h"

#ifndef	RENAME
rename(f1,f2)
char    *f1,*f2;
{
	if(link(f1,f2) <0){
		unlink(f1);
		return(-1);
	}
	return(unlink(f1));
}
#endif	RENAME
