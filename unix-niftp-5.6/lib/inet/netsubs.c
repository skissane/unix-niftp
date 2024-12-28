/* unix-niftp lib/inet/netsubs.c $Revision: 5.5 $ $Date: 90/08/01 13:35:46 $ */
#include "ftp.h"
#include <sys/ioctl.h>

/*
 * file:
 *                       netsubs.c
 *      New ethernet version
 *
 *by: rabbit , modified ruth
 * last changed: 24-May-85
 *
 * $Log:	netsubs.c,v $
 * Revision 5.5  90/08/01  13:35:46  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:46:54  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:39:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

/*
 * This file contains routines to interface to the BSD/INET domain. They
 * make the network seem like a normal file. Makes the interface nicer
 */

int     socket_fd;

/* put a single character into the network (IPCS) buffers */

net_putc(c)
int     c;
{
	register i;
	register nodep nptr = &net_io;

	while(net_wcount >= nptr->write_count){ /* no space left in buffer */
		if (ftp_print & L_SEND_NET)
		{	L_DEBUG_1(L_SEND_NET, 0, "buffer %d to net:",
					net_wcount);
			for(i=0;i<net_wcount;i++){
				if( (i % 16) == 0) L_DEBUG_0(L_SEND_NET,
					L_CONTINUE, "\n");
				L_DEBUG_1(L_SEND_NET, ((i%16) == 0) ? 0 :
				L_CONTINUE, "%02x ",nptr->write_buffer[i]&0xff);
			}
			L_DEBUG_0(L_SEND_NET, L_CONTINUE, "\n");
		}
		nptr->write_count = net_wcount;
		net_wcount = 0;
		clocktimer();           /* clock the timeout system */
		i = write(socket_fd, nptr->write_buffer, nptr->write_count);
		if(i != nptr->write_count){
			nptr->write_count = 0;
			L_WARN_0(L_GENERAL, 0, "Netwrite failed (net_putc)\n");
			(*net_error)(nptr,1,i); /* call the error routine */
			return(-1);
		}
		nptr->write_count = BLOCKSIZ;
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
		net_rcount =0;
		nptr->read_count = BLOCKSIZ;
		clocktimer();
		i = read(socket_fd, nptr->read_buffer, nptr->read_count);
		nptr->read_count = i;
		if(i < 0){
			L_LOG_0(L_GENERAL, 0, "Netread failed (net_getc)\n");
			(*net_error)(nptr,0,i); /* call the error routine */
			return(-1);
		}
		if(i == 0){
			L_WARN_0(L_GENERAL, 0, "Eof on input\n");
			(*net_error)(nptr, 0, -1);
			return(-1);
		}
		if(ftp_print & L_RECV_NET){ /* print all received */
			L_DEBUG_1(L_RECV_NET, 0, "%d chars read from net:",
					nptr->read_count);
			for(i=0;i<nptr->read_count;i++){
				if( (i % 16) == 0) L_DEBUG_0(L_SEND_NET,
					L_CONTINUE, "\n");
				L_DEBUG_1(L_SEND_NET, ((i%16) == 0) ? 0 :
				L_CONTINUE, "%02x ",nptr->read_buffer[i]&0xff);
			}
			L_DEBUG_0(L_SEND_NET, L_CONTINUE, "\n");
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

	if(ftp_print & L_SEND_NET){
		L_DEBUG_1(L_SEND_NET, 0, "net_flush:data sent %d:-",
			net_wcount);
		for(i=0;i<net_wcount;i++){
			if( (i % 16) == 0) L_DEBUG_0(L_SEND_NET,
				L_CONTINUE, "\n");
			L_DEBUG_1(L_SEND_NET, ((i%16) == 0) ? 0 :
			L_CONTINUE, "%02x ",nptr->write_buffer[i]&0xff);
		}
		L_DEBUG_0(L_SEND_NET, L_CONTINUE, "\n");
	}
	nptr->write_count= net_wcount;
	net_wcount=0;
	clocktimer();
	i = write(socket_fd, nptr->write_buffer, nptr->write_count);
	if(i != nptr->write_count){
		nptr->write_count = 0;
		L_WARN_0(L_GENERAL, 0, "Net write failed (net_push)\n");
		(*net_error)(nptr,1,i);         /* call error handler */
		return(-1);
	}
	nptr->write_count = BLOCKSIZ;
	return(0);
}

/* close a connection */

con_close()
{
	net_rcount=0;
	clocktimer();
	(void) close(socket_fd);
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
 *
 * Ignores all args .....
 */

/* ARGSUSED */
net_fail(nptr,dir,e)
register nodep nptr;
int     dir;            /* flag to say if reading or writting to network */
int     e;              /* the error code */
{
	if(tstate == STOPACKs)          /* failure at finish */
		return;                 /* ignore in this state */
	if (tstate)
	{     L_WARN_1(L_MAJOR_COM, 0,
		"Total network failure in state %04x\n", tstate);
	}
	else L_LOG_0(L_MAJOR_COM, 0, "Network connection closed\n");
	killoff(0,REQSTATE,0);
}

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

killoff(state,errval,transtate)
int     state,errval,transtate;
{
	writedocket();
	readfail();
	(void) close(socket_fd);
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
