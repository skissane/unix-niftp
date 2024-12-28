/* unix-niftp lib/dexpand/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:33:59 $ */
#include "ftp.h"
#include "infusr.h"
#include <stdio.h>

#include <cci.h>
extern errno;

/*
 * file:
 *			 netopen.c
 *  Dexpand code
 * last changed: 5 - Jun - 86
 * $Log:	netopen.c,v $
 * Revision 5.5  90/08/01  13:33:59  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:17:01  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:33:22  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.3  87/12/09  16:57:45  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:51:48  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:32:46  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

/*
 * This file contains routines to interface to the Camtec DEXPAND X.25
 * board. They make the network seem like a normal file.
 * Makes the interface nicer
 */

extern  int	circuit;
extern  struct  ccontrolp contp;

/* open a connection to host str and fill in nptr appropriately */

con_open(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	char    *top[2];
	char    fromdte[ENOUGH];
	char    tsfac[2];

	if(circuit < 0){
		int euid = geteuid();
		seteuid(getuid());
		circuit = ccinit(0, CC_YBTS);
		seteuid(euid);
		if(circuit < 0){
			L_WARN_1(L_GENERAL, 0, "Dexpand init failed %d\n", errno);
			return(-1);
		}
	}
	top[0] = fromdte;
	top[1] = t_addr;
	/* What else do we do here ?? since this is specific to ucl */
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		return(-1);
	}
	L_LOG_2(L_FULL_ADDR, 0, "Full address:- %s (from %s)\n",
		hide_pss_pw(t_addr), fromdte);

	nptr->read_count = 0;
	starttimer();                           /* start the timeout system */
	time_out = 2*60;
	clocktimer();   /* only have 2 mins for timeout on open */
	time_out = 11*60;       /* back to rights */
	contp.c_TSrecv = COUTGOING;
	contp.c_TSradr = t_addr;
	contp.c_TSralen = strlen(t_addr);
	contp.c_TSlname = fromdte;
	contp.c_TSlnlen = strlen(fromdte);
	tsfac[0] = 0x1;
	tsfac[1] = 0x80;
	contp.c_TSrsfac = tsfac;
	contp.c_TSrsflen = 2;

	i= ccontrol(circuit, CCONNECT, &contp);
	if(i <= 0){
		L_WARN_2(L_OPENFAIL, 0, "Netopen to %s failed error = %d\n",
				hide_pss_pw(t_addr), contp.c_TSevent); /*failed*/
		contp.c_TSmoec = 0;
		ccontrol(circuit, CCLEAR, &contp);
		ccterm(circuit);
		circuit = -1;
		return(1);
	}
	nptr->write_count = BLOCKSIZ;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	net_open = 1;                   /* say we are open */
	return(0);
}
