/* unix-niftp lib/dexpand/netsubs.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:15 $ */
#include "ftp.h"
#include "infusr.h"
#include <stdio.h>

#include <cci.h>
extern errno;

/*
 * file:
 *                       netsubs.c
 *  Dexpand code
 * last changed: 5 - Jun - 86
 * $Log: netsubs.c,v $
 * Revision 5.6  1991/06/07  17:00:15  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:01  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:42:42  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:32:47  bin
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

int	circuit = -1;
struct  ccontrolp contp;

char    t_addr[ENOUGH];   /* holds translated host name */
unsigned char	net_read_buffer[BLOCKSIZ];
unsigned char	net_write_buffer[BLOCKSIZ];

/* put a single character into the network buffers */

net_putc(c)
int     c;
{
	register i;
	register nodep nptr = &net_io;
	struct cciovec iov;
	static struct ccontrolp event;

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
		iov.cc_iovbase = nptr->write_buffer;
		iov.cc_iovlen = nptr->write_count;
		i = ccsend(circuit, &iov, 1, &event);
		if(i != nptr->write_count) {
			ccontrol(circuit, CEVENT, &event);
			L_WARN_0(L_GENERAL, 0, "Netwrite failed (net_putc)\n");
					/* call the error routine */
			nptr->write_count = 0;
			(*net_error)(nptr,1, 23);
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
	struct cciovec iov;
	static struct ccontrolp event;

	while(net_rcount >= nptr->read_count){  /* read all the characters */
		net_rcount =0;
		nptr->read_count = BLOCKSIZ;
		clocktimer();
		iov.cc_iovbase = nptr->read_buffer;
		iov.cc_iovlen = nptr->read_count;
		i = ccrecv(circuit, &iov, 1, &event);
		if(event.c_TSmsg){
			/*
			 * should do more here to get diagnositics
			 */
			ccontrol(circuit, CEVENT, &event);
			L_LOG_0(L_GENERAL, 0, "Netread failed (net_getc)\n");
					/* call the error routine */
			nptr->read_count = 0;
			(*net_error)(nptr,0, 23);
			return(-1);
		}
		nptr->read_count = i;
		if(i <= 0){
			L_WARN_0(L_GENERAL, L_DATE | L_TIME,
					"Dexpand fell over again !!!!\n");
			ccterm(circuit);
			circuit = -1;
			(*net_error)(nptr, 0, 23);
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
	struct cciovec iov;
	static struct ccontrolp event;

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
	iov.cc_iovbase = nptr->write_buffer;
	iov.cc_iovlen = nptr->write_count;
	event.c_TSpush = 1;
	i = ccsend(circuit, &iov, 1, &event);
	if(i != nptr->write_count){
		ccontrol(circuit, CEVENT, &event);
		L_WARN_0(L_GENERAL, 0, "Net write failed (net_push)\n");
					/* call error handler */
		nptr->write_count = 0;
		(*net_error)(nptr,1,23);
		return(-1);
	}
	nptr->write_count = BLOCKSIZ;
	return(0);
}

/* close a connection */

con_close()
{
	register i;
	register nodep nptr = &net_io;
	static struct ccontrolp event;

	net_rcount=0;
	clocktimer();
	event.c_TSmoec = 0;
	ccontrol(circuit, CCLEAR, &event);
	stoptimer();            /* stop the timeout timer */
	net_open=0;
}

net_null_routine()      /* this is to stop recursive net_error's */
{}                      /* set net_error to it after a timeout */

/*
 * this is the normal routine that is called by the above routines
 * when a network error occurs
 */

#ifndef _TIMEOUT
#define _TIMEOUT        0x1234          /* any silly value */
#define _RESET          0x1111
#endif

net_fail(nptr,dir,e)
register nodep nptr;
int     dir;            /* flag to say if reading or writting to network */
int     e;              /* the error code */
{
/*
 * have got an error that we can't recover from
 * close the call and rewrite the queue entry
 */
	if(!direction || ( e != _RESET && e != _TIMEOUT)){
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
 * we can only deal sensibly with resets and timeouts
 */
	if(e == _TIMEOUT){
		net_error = net_null_routine;   /* stop recursive calls */
		if(direction == TRANSMIT){      /* timeout during transmit */
			if(tstate == MRs)       /* send appropriate response*/
				send_comm(ES,0x30,1);
			else if(tstate == WAIT)
				send_comm(ES,0x31,1);
			else if(tstate == ESok)
				send_comm(ES,0x32,1);
			else if(tstate == ESe)
				send_comm(ES,0x33,1);
			else if(tstate == HOLD || tstate == HORR)
				send_comm(ES,0x34,1);
			else L_WARN_0(L_MAJOR_COM, 0, "Unknown state error\n");
		}
		else {                  /* timed out during a reception */
			if(tstate==DATA)
				send_comm(QR,0x30,1);
			else if(tstate == RRs)
				send_comm(QR,0x31,1);
			else if(tstate==QRok)
				send_comm(QR,0x32,1);
			else if(tstate==PEND)
				send_comm(QR,0x33,1);
			else if(tstate==QRe)
				send_comm(QR,0x34,1);
			else if(tstate==GOs)
				send_comm(QR,0x35,1);
			else if(tstate ==PERR)
				send_comm(QR,0x36,1);
			else L_WARN_0(L_MAJOR_COM, 0, "Unknown state error\n");
		}
		net_error = net_fail;   /* restore the error routine */
		killoff(0,TIMEOUTSTATE,0);      /* close the connection */
	}
/*
 * have got a protocol reset
 */
	if(direction==TRANSMIT){
		tstate = WAIT;          /* just assume it is not finished */
		if(dir==0)              /* reading just return */
			return;
		zap_record();           /* otherwise pull back the stack */
		wordcount=0;            /* and try again */
		reclen=0;
		longjmp(rcall, 0);
	}

/* if recieving send the correct response then return */

	switch(tstate){
	case ESok:                      /* at the end of the transfer */
	case QRok:
		send_comm(QR,0,1);
		tstate = QRok;
		break;
	case QRe:
		send_comm(QR,0x20,1);           /* had an error */
		break;
	default:
		if(tstate != GOs)       /* send last mark acknowledged */
			send_comm(MR,(int)lastmark&MASK,0);
		if(tstate == HOLD){     /* send correct response if in */
			send_comm(QR,0x10,1);     /* hold state */
			tstate=PEND;
			break;
		}
						/* can't restart ? */
		if(!(facilities & RESTARTS) || ++nrestarts >= 10){
			send_comm(QR,0x20,1);     /* send a failure here */
			tstate = QRe;
			break;
		}
		L_LOG_0(L_MAJOR_COM, 0, "Going to try to do a restart\n");
		send_comm(RR,(int)rec_mark&MASK,1);       /* do a restart */
		bcount = last_count = lr_bcount;
		reclen = last_rlen = lr_reclen;
		lastmark = rec_mark;
		L_LOG_1(L_10, 0, "RR bcount = %ld\n",bcount);
		(void) lseek(f_fd,last_count,0);
		fnleft=0;               /* rewind counter */
		tstate = RRs;           /* wait for it */
		break;
	}
	if(dir==0)      /* if reading from network just return error */
		return;
	longjmp(rcall, 0);        /* else pop stack to return */
}

/*
 * returns true if there is more data to be read from network
 * This could be a hash define
 */

net_more()
{
	static  struct  ccontrolp event;

	if(net_rcount < net_io.read_count || ccstat(circuit, &event));
}

/*
 * kill off the connection. reseting all neccassary counters and flags.
 * do it as quickly and as quietly as possible
 */

killoff(state,errval,transtate)
int     state,errval,transtate;
{
	static struct ccontrolp junk;

	writedocket();
	readfail();
	junk.c_TSmoec = 0;
	ccontrol(circuit, CCLEAR, &junk);
	ccterm(circuit);
	circuit = -1;
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
