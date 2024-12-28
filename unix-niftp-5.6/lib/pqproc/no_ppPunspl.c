#ifdef	lint
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppPunspl.c,v 5.5 90/08/01 13:36:54 pb Exp $";
#endif	lint
/* $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/no_ppPunspl.c,v 5.5 90/08/01 13:36:54 pb Exp $ */
/*
 * $Log:	no_ppPunspl.c,v $
 * Revision 5.5  90/08/01  13:36:54  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */

/* Dummy P end code to include in the P end library */

#include "ftp.h"

#ifdef	PP
int	ppp_init (argc, argv)
int	argc;
char	**argv;
{
	L_WARN_2(L_ALWAYS, 0,
		"ppp_init(%d, %x) P end code not included in P library\n",
		argc, argv);
	return -1;
}

int	ppp_getnextmessage (host, net)
char	**host;
char	**net;
{
	L_WARN_2(L_ALWAYS, 0,
		"ppp_getnextmessage(%x, %x) P end code not included in P library\n",
		host, net);
	return -1;
}

int	ppp_getdata (buf, len)
char	*buf;
int	len;
{
	L_WARN_2(L_ALWAYS, 0,
		"pp_write(%x, %d) P end code not included in P library\n",
		buf, len);
	return -1;
}

int	ppp_status (status, reason)
int	status;
char	*reason;
{
	L_WARN_2(L_ALWAYS, 0,
		"ppp_status(%d, %s) P end code not included in P library\n",
		status, reason);
	return -1;
}

int	ppp_terminate ()
{
	L_WARN_0(L_ALWAYS, 0,
		"ppp_terminate() P end code not included in P library\n");
	return -1;
}

int	do_pp ()
{
	L_WARN_0(L_ALWAYS, 0,
		"do_pp() P end code not included in P library\n");
	return -1;
}

int	pp_close ()
{
	L_WARN_0(L_ALWAYS, 0,
		"pp_close() P end code not included in P library\n");
	return -1;
}

#endif	PP
