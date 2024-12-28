/* unix-niftp lib/ubc/netlisten.c $Revision: 5.5 $ $Date: 90/08/01 13:38:55 $ */

/*
 * file: netlisten.c
 *
 *	for Nottingham Yellow Book mark 0
 *
 * wja@nott.cs
 *
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:38:55  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:54:59  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:55:32  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "ftp.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "yb.h"
#include "netsubs.h"

#ifndef STANDALONE
#include "infusr.h"
#else
extern char *network;
#endif STANDALONE

/*
 * listen for a call on the host number given as a parameter
 */
extern char	unknown_host;

con_listen(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	struct  sockaddr_yb from;
	int     fromlen = sizeof(from);
	char	called[ENOUGH];
	char	*p;

#ifdef  DEBUG
	if(net_open)
		if(ftp_print & 020)
			printf("con_listen called while net_open\n");
#endif
	nptr->read_count =0;
	while((socket_fd = yb_socket(AF_CCITT, SOCK_STREAM, PF_YBTS)) < 0
		&& errno == ENETDOWN)
		sleep(60); /* wait for net too come up again */
	if(socket_fd < 0){
		if(ftp_print & 1)
			printf("listen: socket failed - %s\n", yb_error(errno));
		sleep(15); /* stop it running away too fast */
		return(1);
	}
	syb.syb_addr = str;
	syb.syb_addrlen = strlen(str);
	syb.syb_qoslen = 0;
	syb.syb_explen = 0;
	if(yb_bind(socket_fd, (caddr_t)&syb, sizeof (syb)) < 0) {
		if(ftp_print & 1)
			printf("bind failed - %s\n", yb_error(errno));
		(void) yb_close(socket_fd);
		socket_fd = -1;
		return(1);
	}
	if(yb_listen(socket_fd, 1) < 0) {
		if(ftp_print & 1)
			printf("listen failed - %s\n", yb_error(errno));
		(void) yb_close(socket_fd);
		socket_fd = -1;
		return(1);
	}
			/* should put from address in here */
	from.syb_addr = t_addr;
	from.syb_addrlen = ENOUGH;
	from.syb_qoslen = 0;
	from.syb_explen = 0;
	starttimer();
	i = yb_accept(socket_fd, &from, &fromlen);
	if(i < 0){
		if(ftp_print & 1)
			printf("Accept failed - %s\n", yb_error(errno));
		(void) yb_close(socket_fd);
		socket_fd = -1;
		return(1);
	}
	(void) yb_close(socket_fd);
	socket_fd = i;
	t_addr[from.syb_addrlen] = 0; /* string terminate calling address */

	/* collect address called on */
	bzero(called, sizeof(called));
	syb.syb_addr = called;
	syb.syb_addrlen = sizeof(called);
	syb.syb_qoslen = 0;
	syb.syb_explen = 0;
	yb_getsockname(socket_fd, &syb, &syblen);

	pconn(t_addr, called);
	/*
	 * try to find who called us
	 */
	if(r_addr_trans(called, t_addr, hostname, argstring) < 0) {
		if(ftp_print & 1)
			printf("can't translate %s\n", t_addr);
		unknown_host = 1;
	}
	else
		unknown_host = 0;

	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	nptr->write_count = BLOCKMAX;
	net_wcount = BLOCKMIN;
	net_open=1;
	return(0);
}

/*
 * search address tables and generate host mnemonic
 * from calling address. name is left in 'hostname'
 */

/*
 * character to be added to string for reverse lookups
 */
extern  char    lnetchar;

r_addr_trans(called, calling, nhostname, hn)
char	*called, *calling, *nhostname, *hn;
{
	char net;

	if(hn == NULL){		/* no channel - local transfer */
		(void) strcpy(nhostname, calling);
		return(0);
	}
	net = lnetchar;
	if(net == 'p' && strncmp(calling, "2342", 4) != 0)
		net = 'i';
	return(nrs_reverse(net, called, calling, nhostname));
}
