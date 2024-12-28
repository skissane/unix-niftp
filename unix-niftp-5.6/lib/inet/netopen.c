/* unix-niftp lib/inet/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:35:44 $ */
#include "ftp.h"
#include "infusr.h"

/*
 * file:
 *                       netopen.c
 *      New ethernet version
 *
 *by: rabbit , modified ruth
 * last changed: 24-May-85
 *
 * $Log:	netopen.c,v $
 * Revision 5.5  90/08/01  13:35:44  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:20:17  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:58:28  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:53:19  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:39:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

struct sockaddr_in sin;
struct hostent *gethostbyname();
struct servent *getservbyname();

char	net_read_buffer[BLOCKSIZ];
char	net_write_buffer[BLOCKSIZ];

/*
 * This file contains routines to interface to the BSD/INET domain. They
 * make the network seem like a normal file. Makes the interface nicer
 */

int     socket_fd;

char    t_addr[ENOUGH];   /* holds translated host name */

/* open a connection to host str and fill in nptr appropriately */

con_open(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	char    *top[2];
	char    portn[10];
	struct servent *sp;
	struct hostent *host;

	if(net_open) L_WARN_0(L_10, 0, "con_open called with net_open\n");

	top[0] = t_addr;
	top[1] = portn;

					/* translate to get network too */
	if(i=adr_trans(str, network, top) ){
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		return(-1);
	}

	L_LOG_2(L_FULL_ADDR, 0, "Full address:- %s (from %s)\n",
		hide_pss_pw(t_addr), portn);

	nptr->read_count=0;
	starttimer(2*60);                      /* start the timeout system */
	alarm(2*60);		/* TEMP HACK til start timer is fixed */

	if (host = gethostbyname(t_addr))
	{
		sin.sin_family = host->h_addrtype;
		bcopy(host->h_addr, (caddr_t)&sin.sin_addr, host->h_length);
	}
	else
	{
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(t_addr);
		if(sin.sin_addr.s_addr == (unsigned long) -1){
			L_WARN_1(L_GENERAL, 0, "Unknown host %s\n", t_addr);
			return(1);
		}
	}

	if (! *portn) strcpy(portn, "niftp");
	if (sp = getservbyname("niftp", "tcp"))
		sin.sin_port = sp->s_port;
	else
	{	
		sin.sin_port = atoi(portn);
		if (sin.sin_port == 0)
		{	L_WARN_1(L_GENERAL, 0, 
				"Failed to find service `%s'\n", portn);
			return 1;
		}
		sin.sin_port = htons(sin.sin_port);
	}

	socket_fd = socket(AF_INET, SOCK_STREAM, 0, 0);
	if(socket_fd < 0){
		L_WARN_1(L_GENERAL, 0, "socket create failed (%d)\n", errno);
		return(1);
	}
#ifndef FTPONLY
	if(resvport(socket_fd)) return 1;
#endif FTPONLY
	if(connect(socket_fd, (caddr_t)&sin, sizeof(sin), 0) < 0){
		L_WARN_1(L_GENERAL, 0, "Cannot connect (%d)\n", errno);
		return(1);
	}
	starttimer(11*60);	/* Now give it a chance ... */
	alarm(11*60);		/* TEMP HACK til start timer is fixed */
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	nptr->write_count = BLOCKSIZ;
#ifndef FTPONLY
	putfakeybts();
#endif FTPONLY
	net_open = 1;                   /* say we are open */
	return(0);
}

#ifndef FTPONLY
resvport(s)
	int s;
#ifdef SETEUID
{
	int stat, oldeuid;
	oldeuid = geteuid();
	seteuid(getuid());
	stat = real_resvport(s);
	seteuid(oldeuid);
	return stat;
}

real_resvport(s)
	int s;
#endif SETEUID
{
	register int p;
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	for(p = IPPORT_RESERVED-1; p > IPPORT_RESERVED/2; p--) {
		sin.sin_port = htons((u_short)p);
		if(bind(s, (caddr_t)&sin, sizeof sin, 0) < 0) {
			if(errno == EADDRINUSE || errno == EADDRNOTAVAIL) continue;
			L_DEBUG_1(L_GENERAL, 0, "bind failed (%d)\n", errno);
			return 1;
		}
		return 0;
	}
	L_LOG_0(L_GENERAL, 0, "all ports in use\n");
	return 1;
}

putfakeybts()
{
	register char *ybts;
	ybts = NULL;
	switch(tab.t_flags & T_TYPE) {
	case T_MAIL:
		ybts = "ftp.mail";
		break;
#ifdef JTMP
	case T_JTMP:
		ybts = "ftp.jtmp";
		break;
#endif JTMP
#ifdef NEWS
	case T_NEWS:
		ybts = "ftp.news";
		break;
#endif NEWS
	case T_FTP:
		ybts = "ftp";
		break;
	}
	if(ybts)
		while(*ybts) net_putc(*ybts++);
	net_putc(0);
}
#endif FTPONLY
