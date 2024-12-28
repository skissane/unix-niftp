#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/ppPdummy.c,v 5.6 1991/06/07 17:01:55 pb Exp $";
#endif	lint
/* $Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/ppPdummy.c,v 5.6 1991/06/07 17:01:55 pb Exp $ */
/*
 * $Log: ppPdummy.c,v $
 * Revision 5.6  1991/06/07  17:01:55  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:12  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */

/* Dummy P end code to make up for bits missing from the PP P library */

#include "ftp.h"

#ifdef	PP
/* Now that the file has arrived, process it. Not used for inline PP */
int	do_pp (spooled)
char *spooled;
{
	L_WARN_1(L_ALWAYS, 0,
		(spooled) ? "do_pp(%s) No spool code provided\n" :
		            "do_pp(%s) No need to do anything\n", spooled);
	return (spooled) ? -1 : 0;
}

int	pp_close (fd, rc)
{
	L_WARN_2(L_ALWAYS, 0,
		"pp_close(%d, %d) Q end code not included in Q library\n",
		fd, rc);
	return 1;
}
#endif	PP
