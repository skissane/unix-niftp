/* unix-niftp lib/inet/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:35:42 $ */
#include "ftp.h"
#include "stat.h"
#include <stdio.h>

#include "infusr.h"

/*
 * file:
 *                       netopen.c
 *      New ethernet version
 *
 *by: rabbit , modified ruth
 * last changed: 24-May-85
 *
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:35:42  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:46:49  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
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
char	net_read_buffer[BLOCKSIZ];
char	net_write_buffer[BLOCKSIZ];

#define UC(b)   (((int)b)&0xff)
/*
 * This file contains routines to interface to the BSD/INET domain. They
 * make the network seem like a normal file. Makes the interface nicer
 */

int     socket_fd;

char    t_addr[ENOUGH];   /* holds translated host name */

/* open a connection to host str and fill in nptr appropriately */

/*
 * listen for a call on the host number given as a parameter
 */

#ifndef FTPONLY
extern char	unknown_host;
char	*getfakeybts();
#endif FTPONLY

con_listen(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	struct  sockaddr_in from;
	int     fromlen = sizeof(from);
	char    *p;
	struct	servent *sp;
#ifndef FTPONLY
	char *ybts;
#endif FTPONLY

	if(net_open) L_WARN_0(L_10, 0, "con_listen called while net_open\n");

	nptr->read_count =0;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0, 0);
	if(socket_fd < 0){
		L_WARN_1(L_GENERAL, 0, "listen: socket failed (%d)\n", errno);
		return(1);
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	if (sp = getservbyname(str, "tcp"))
		sin.sin_port = sp->s_port;
	else
	{	if ('0' > *str || *str > '9')
		{	L_WARN_1(L_GENERAL, 0, 
				"Invalid ports string `%s'\n", str);
			return 1;
		}
		sin.sin_port = atoi(str);
		sin.sin_port = htons(sin.sin_port);
	}
	if(bind(socket_fd, (caddr_t)&sin, sizeof (sin), 0) < 0) {
		(void) close(socket_fd);
		socket_fd = -1;
		L_WARN_2(L_GENERAL, 0, "bind to %s failed %04x\n",
				t_addr, errno);
		return(1);
	}
	(void) listen(socket_fd, 1);
			/* should put from address in here */
	starttimer(60*60);
	alarm(60*60);		/* TEMP HACK til start timer is fixed */
	i = accept(socket_fd, &from, &fromlen);
	if(i < 0){
		(void) close(socket_fd);
		socket_fd = -1;
		L_WARN_1(L_GENERAL, 0, "Accept failed (%d)\n", errno);
		return(1);
	}
	starttimer(11*60);	/* Now give it a chance ... */
	alarm(11*60);		/* TEMP HACK til start timer is fixed */
	(void) close(socket_fd);
	socket_fd = i;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	nptr->write_count = BLOCKSIZ;
	p = (char *)&from.sin_addr;
	sprintf(t_addr,"%d.%d.%d.%d", UC(p[0]),UC(p[1]), UC(p[2]), UC(p[3]));
	stat_addr(t_addr);
#ifdef FTPONLY
	L_LOG_1(L_GENERAL, L_DATE | L_TIME, "net_open to %s\n", t_addr);
#else FTPONLY
	ybts = getfakeybts();
	if(r_addr_trans(sin.sin_port, ybts, t_addr, hostname) < 0) {
		L_LOG_1(L_DEB_ADDR, 0, "can't translate \"%s\"\n", t_addr);
		unknown_host = 1;
	} else if(ntohs((u_short)from.sin_port) >= IPPORT_RESERVED) {
		L_LOG_1(L_DEB_ADDR, 0, "unprivileged port %d\n", ntohs((u_short)from.sin_port));
		unknown_host = 1;
	} else
		unknown_host = 0;
#endif FTPONLY
	net_open=1;
	return(0);
}

#ifndef FTPONLY
char *getfakeybts()
{
	register char *p;
	register int c;
	static char ybts[ENOUGH];
	p = ybts;
	while(c = net_getc())
		if(p < &ybts[ENOUGH-1]) *p++ = c;
	*p = 0;
	return ybts;
}

r_addr_trans(port, called, calling, nhostname)
	int port;
	char *called, *calling, *nhostname;
{
	static char iabuf[ENOUGH];
	extern char *lnetchar;
	sprintf(iabuf, "%s.%d", calling, ntohs((u_short)port));
	return nrs_reverse(lnetchar, called, iabuf, nhostname);
}
#endif FTPONLY
