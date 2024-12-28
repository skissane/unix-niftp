/* unix-niftp lib/gen/bzero.c $Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/bzero.c,v 5.6 1991/06/07 17:00:28 pb Exp $ */
/*
 * $Log: bzero.c,v $
 * Revision 5.6  1991/06/07  17:00:28  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:08  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:42:53  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * clear memory
 */

#include "opts.h"

#ifndef	BZERO
bzero(cp, size)
register char   *cp;
register size;
{
	do{
		*cp++ = 0;
	}while(--size);
}
#endif
