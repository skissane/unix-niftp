/* unix-niftp lib/gen/hide_pss_pw.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:53 $ */
/*
 * Hide a PSS password ....
 *
 * $Log: hide_pss_pw.c,v $
 * Revision 5.6  1991/06/07  17:00:53  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:42  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:16:42  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:33:40  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.3  87/12/09  16:58:03  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:52:13  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 */
#include "opts.h"

extern char *malloc();
extern char *index();
extern char *strcpy (), *strncpy ();

char *
hide_pss_pw(s)
char *s;
{	char *start, *comma, *end;
	static char *result = (char *) 0;

	/* PSS passwords look like (user,pw) */
	if (!(	(start = index(s,     '(')) &&
		(comma = index(start, ',')) &&
		(end   = index(comma, ')'))
	   ) ) return s;

	if (result) free(result);

	result = malloc((comma - s) + strlen(end) +2);
	if (!result)	return "<Censored>";

	(void) strncpy(result, s, (comma-s) +1);
	(void) strcpy (result + (comma -s) +1, end);
	return result;
}
