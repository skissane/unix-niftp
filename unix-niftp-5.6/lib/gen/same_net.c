#ifdef	lint
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/same_net.c,v 5.6 1991/06/07 17:01:04 pb Exp $";
#endif	lint

/*
 * Check if this is the required channel
 *
 * $Log: same_net.c,v $
 * Revision 5.6  1991/06/07  17:01:04  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:58  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:14:48  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:58:21  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:52:56  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * 
 */
#include "ftp.h"
#include "nrs.h"
same_net(given, current, alias)
char *given;
char *current;
{	int i;

	/* Exact match is bound to be OK */
	if (strcmp(given, current) == 0) return 1;

	/* no aliases -- give up */
	if (!alias) return 0;

	/* Now look through to see if it can match ... */
	for (i=0; i < CONF_MAX && NETALIASES[i].NA_alias; i++)
		if (strcmp(given,   NETALIASES[i].NA_alias) == 0 &&
		    strcmp(current, NETALIASES[i].NA_realname) == 0)
			return 2;

	/* No luck */
	return 0;
}
