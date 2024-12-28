/* unix-niftp lib/ubc/netsubs.c $Revision: 5.6 $ $Date: 1991/06/07 17:02:23 $ */

/*
 * file: netsubs.c
 *
 *	for Nottingham Yellow Book mark 0
 *
 * wja@nott.cs
 * $Log: netsubs.c,v $
 * Revision 5.6  1991/06/07  17:02:23  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:39:00  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:25:04  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:42:08  pb
 * *** empty log message ***
 * 
 * Revision 5.0  88/02/11  06:41:41  pb
 * *** empty log message ***
 * 
 * Revision 5.0  87/03/23  03:55:35  bin
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

struct  sockaddr_yb syb;
int	syblen = sizeof(syb);
int	socket_fd;
char    t_addr[ENOUGH];   /* holds translated host name */
unsigned char	net_read_buffer[BLOCKSIZ];
unsigned char	net_write_buffer[BLOCKSIZ];

extern char *yb_error();

/* put a single character into the network buffers */

net_putc(c)
int     c;
{
	register i;
	register nodep nptr = &net_io;

	while(net_wcount >= nptr->write_count){ /* no space left in buffer */
		if(ftp_print & 040000){ /* print all sent on net */
			printf("buffer to net:\n");
			for(i=BLOCKMIN;i<net_wcount;i++){
				if( (i % 16) == 15)
					printf("\n");
				printf("%o ",nptr->write_buffer[i]&0377);
			}
			printf("\n");
		}
		nptr->write_count = net_wcount;
		net_wcount = BLOCKMIN;
		clocktimer();           /* clock the timeout system */
		nptr->write_buffer[0] = MBIT;
		i = yb_write(socket_fd, nptr->write_buffer, nptr->write_count);
		if(i != nptr->write_count){
			nptr->write_count = 0;
			if(ftp_print & 1)
				printf("Netwrite failed (net_putc) - %s\n",
					yb_error(errno));
			(*net_error)(nptr,1,errno); /* call the error routine */
			return(-1);
		}
		nptr->write_count = BLOCKMAX;
	}
	nptr->write_buffer[net_wcount++]=c;  /* add the character to buffer */
	return(0);
}

/* read a single character from the network */

net_getc()
{
	register i;
	register nodep nptr = &net_io;

	while(net_rcount >= nptr->read_count){  /* read all the characters */
		net_rcount = BLOCKMIN;
		nptr->read_count = BLOCKMAX;
		clocktimer();
		i = yb_read(socket_fd, nptr->read_buffer, nptr->read_count);
		nptr->read_count = i;
		if(i < 0){
			if(ftp_print & 1)
				printf("Netread failed (net_getc) - %s\n",
					yb_error(errno));
			(*net_error)(nptr,0,errno); /* call the error routine */
			return(-1);
		}
		/* Certain errors should come here */
		if(i == 0){
			if(ftp_print & 1)
				printf("Eof on input\n");
			(*net_error)(nptr, 0, -1);
			return(-1);
		}
		if(ftp_print & 040000){ /* print all received */
			printf("%d chars read from net:",nptr->read_count);
			for(i=BLOCKMIN;i<nptr->read_count;i++){
				if( (i % 16) == 15)
					printf("\n");
				printf("%o ",nptr->read_buffer[i]&0377);
			}
			printf("\n");
		}
	}
	return(nptr->read_buffer[net_rcount++] & MASK);
						/* return the character */
}

/* flush out all characters waiting in buffers */

net_flush()
{
	register i;
	register nodep nptr = &net_io;

	if(ftp_print & 040000){
		printf("net_flush:data sent:-\n");
		for(i=0;i<net_wcount;i++){
			if( (i % 16) == 15)
				printf("\n");
			printf("%o ",nptr->write_buffer[i]&0377);
		}
		printf("\n");
	}
	nptr->write_count= net_wcount;
	net_wcount = BLOCKMIN;
	clocktimer();
	nptr->write_buffer[0] = 0; /* no more bit */
	i = yb_write(socket_fd, nptr->write_buffer, nptr->write_count);
	if(i != nptr->write_count){
		nptr->write_count = 0;
		if(ftp_print & 1)
			printf("Net write failed (net_push) - %s\n",
				yb_error(errno));
		(*net_error)(nptr,1,errno);	    /* call error handler */
		return(-1);
	}
	nptr->write_count = BLOCKMAX;
	return(0);
}

/* close a connection */

con_close()
{
	register i;
	register nodep nptr = &net_io;

	net_rcount=0;
	clocktimer();
	starttimer();  /* stop problems with buildq */
	(void) yb_close(socket_fd);
	socket_fd = -1;
	stoptimer();            /* stop the timeout timer */
	net_open=0;
}

net_null_routine()      /* this is to stop recursive net_error's */
{}                      /* set net_error to it after a timeout */

/*
 * this is the normal routine that is called by the above routines
 * when a network error occurs.
 * will always abort the call.
 */

#ifndef STANDALONE

net_fail(nptr,dir,e)
register nodep nptr;
int     dir;            /* flag to say if reading or writting to network */
int     e;              /* the error code */
{
	if(tstate == STOPACKs)          /* failure at finish */
		return;                 /* ignore in this state */
#ifdef  DEBUG
	if(ftp_print & 2)
		printf("Total network failure\n");
#endif
	killoff(0,REQSTATE,0);
}
#else

net_fail(nptr,dir,e)
register nodep nptr;
int     dir;            /* flag to say if reading or writting to network */
int     e;              /* the error code */
{
	if(ftp_print&2)
		printf("Somethings failed\n");
	longjmp(rcall, 1);
}

killoff()
{
	printf("killoff called from somewhere\n");
}
#endif

/*
 * returns true if there is more data to be read from network
 * This could be a hash define
 */

net_more()
{
	register nodep nptr = &net_io;
	int     xval;

	if(net_rcount < nptr->read_count)
		return(1);
	xval = 0;
	(void) ioctl(socket_fd, FIONREAD, &xval);
	return(xval != 0);
}

/*
 * kill off the connection. reseting all neccassary counters and flags.
 * do it as quickly and as quietly as possible
 */

#ifndef STANDALONE

killoff(state,errval,transtate)
int     state,errval,transtate;
{
	writedocket();
	readfail();
	(void) yb_close(socket_fd);
	socket_fd = -1;
	net_open=0;
	net_rcount=0;
	net_wcount=0;
	stoptimer();
	zap_record();
	error = errval;                         /*mark item in queue */
	tstate = state;
	st_of_tran = transtate;
	longjmp(rcall, 0);                /* pop the stack */
}

#endif

/* stamp connection in log */

pconn(calling, called)
	char *calling, *called;
{
	long l;
	char *ctime();

	(void) time((long *)&l);
	printf("\nCall from <%s> to <%s> at %s",
		calling, called, ctime(&l));
	/* ctime provides the newline */
}
