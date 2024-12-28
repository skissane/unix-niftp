#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/ppQdummy.c,v 5.5 90/08/01 13:37:09 pb Exp $";
#endif	lint
/* $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/ppQdummy.c,v 5.5 90/08/01 13:37:09 pb Exp $ */
/*
 * $Log:	ppQdummy.c,v $
 * Revision 5.5  90/08/01  13:37:09  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */

/* Dummy Q end code to make up for bits missing from the PP Q library */

#include "ftp.h"

#ifdef	PP
/* Now that the file has arrived, process it. Not used for inline PP */
int	do_pp (spooled)
{
	L_WARN_1(L_ALWAYS, 0,
		(spooled) ? "do_pp(%d) No spool code provided\n" :
		            "do_pp(%d) No need to do anything\n", spooled);
	return (spooled) ? -1 : 0;
}
#endif	PP
