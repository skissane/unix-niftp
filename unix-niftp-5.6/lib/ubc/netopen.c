/* unix-niftp lib/ubc/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:38:57 $ */
/*
 * file: netopen.c
 *
 *	for Nottingham Yellow Book mark 0
 *
 * wja@nott.cs
 * $Log:	netopen.c,v $
 * Revision 5.5  90/08/01  13:38:57  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:55:01  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:55:33  bin
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

/* open a connection to host str and fill in nptr appropriately */

con_open(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	char    *index(), *p;
	char	*top[2];
	char	fromdte[ENOUGH];

#ifdef  DEBUG
	if(net_open)
		if(ftp_print & 020)
			printf("con_open called with net_open\n");
#endif
	top[0] = fromdte;
	top[1] = t_addr;
	/* What else do we do here ?? since this is specific to ucl */
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		if(ftp_print & 1)
			printf("Address trans error %d\n",i);
		return(-1);
	}
	if(ftp_print & 4)
		printf("Full address:- %s\n",t_addr);

	nptr->read_count=0;
	starttimer();                           /* start the timeout system */
	time_out = 2*60;
	clocktimer();   /* only have 2 mins for timeout on open */
	time_out = 11*60;       /* back to rights */

	socket_fd = yb_socket(AF_CCITT, SOCK_STREAM, PF_YBTS);
	if(socket_fd < 0){
		if(ftp_print & 1)
			printf("socket create failed - %s\n", yb_error(errno));
		return(1);
	}

	/* bind our name to socket */
	syb.syb_addr = fromdte;
	syb.syb_addrlen = strlen(fromdte);
	syb.syb_qoslen = 0;
	syb.syb_explen = 0;
	if(yb_bind(socket_fd, (caddr_t)&syb, sizeof(syb)) < 0) {
		if(ftp_print & 1)
			printf("bind failed - %s\n", yb_error(errno));
		return(1);
	}

	/* make connection */
	syb.syb_addr = t_addr;
	syb.syb_addrlen = ENOUGH;
	syb.syb_qoslen = 0;
	syb.syb_explen = 0;
	if(yb_connect(socket_fd, (caddr_t)&syb, sizeof(syb)) < 0){
		if(ftp_print & 1)
			printf("Cannot connect - %s\n", yb_error(errno));
		return(1);
	}
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	nptr->write_count = BLOCKMAX;
	net_wcount = BLOCKMIN;
	net_open = 1;			/* say we are open */
	return(0);
}

