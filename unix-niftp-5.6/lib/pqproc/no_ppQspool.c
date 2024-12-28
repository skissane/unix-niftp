#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppQspool.c,v 5.5 90/08/01 13:36:49 pb Exp $";
#endif	lint
/* $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppQspool.c,v 5.5 90/08/01 13:36:49 pb Exp $ */
/*
 * $Log:	no_ppQspool.c,v $
 * Revision 5.5  90/08/01  13:36:49  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */

/* Dummy Q end code to include in the P end library */

#include "ftp.h"

#ifdef	PP

/* Tell PP to commit */
do_pp(file)
char *file;
{	L_WARN_1(L_ALWAYS, 0,
		"do_pp(%s) Q end code not included in P library\n", file);
	(void) unlink(file);
	return(1);
}
#endif	PP
