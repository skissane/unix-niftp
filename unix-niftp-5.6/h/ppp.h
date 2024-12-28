/* ppp.h: ppp interface */

/*
 * @(#) $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/h/ppp.h,v 5.5 90/08/01 13:33:24 pb Exp $
 *
 * $Log:	ppp.h,v $
 * Revision 5.5  90/08/01  13:33:24  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 *
 */

#define	PPP_STATUS_DONE			1
#define PPP_STATUS_CONNECT_FAILED	2
#define PPP_STATUS_PERMANENT_FAILURE	3
#define PPP_STATUS_TRANSIENT_FAILURE	4

int	ppp_init ();
int	ppp_getnextmessage ();
int	ppp_getdata ();
int	ppp_status ();
void	ppp_terminate ();

