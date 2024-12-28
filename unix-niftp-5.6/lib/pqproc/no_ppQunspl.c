#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppQunspl.c,v 5.5 90/08/01 13:36:51 pb Exp $";
#endif	lint
/* $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppQunspl.c,v 5.5 90/08/01 13:36:51 pb Exp $ */
/*
 * $Log:	no_ppQunspl.c,v $
 * Revision 5.5  90/08/01  13:36:51  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */

/* Dummy Q end code to include in the P end library */

#include "ftp.h"

#ifdef	PP
/* Tell PP that all the data has arrived */
int
pp_close(fd, rc)
{
	L_WARN_2(L_ALWAYS, 0,
		"pp_close(%d, %d) PP unspooled code not yet installed\n",
		fd, rc);
	return 1;
}

/* Tell PP that a call is arriving */
int
pp_open(chan, hostname)
char *chan;
char *hostname;
{
	L_WARN_2(L_ALWAYS, 0,
		"pp_open(%s, %s) PP unspooled code not yet installed\n", 
		chan, hostname);
	return -2;
}

/* write some data to PP */
int
pp_write(fd, buff, size)
int	fd;
char	buff[];
int	size;
{
	L_WARN_3(L_ALWAYS, 0,
		"pp_write(%d, %x, %d) PP unspooled code not yet installed\n",
		fd, buff, size);
	return 0;
}

/* Now that the file has arrived, process it. Not used for inline PP */
int	do_pp (spooled)
{
	L_WARN_1(L_ALWAYS, 0,
		(spooled) ? "do_pp(%d) No spool code provided\n" :
		            "do_pp(%d) No need to do anything\n", spooled);
	return (spooled) ? -1 : 0;
}
#endif	PP
