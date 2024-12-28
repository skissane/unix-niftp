#ifndef	lint			/* unix-niftp lib/x25b/netsubs.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/x25b/netsubs.c,v 5.6 1991/06/07 17:02:28 pb Exp $";
#endif	lint

/*
 * file:
 *                       netsubs.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: netsubs.c,v $
 * Revision 5.6  1991/06/07  17:02:28  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:39:22  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:21:16  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:35:26  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0  88/01/28  06:23:37  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 *
 * This file contains routines to interface to the SUNlink libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */

long	allow_qmask;

#include "ftp.h"
#include "infusr.h"
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "x25b.h"

#ifndef	_RESET
#define _RESET          0x1111	/* any silly value */
#endif	_RESET

int	fid;

char    t_addr[ENOUGH];   /* holds translated host name */
unsigned char	net_read_buffer[BLOCKSIZ];
unsigned char	net_write_buffer[BLOCKSIZ];

/* put a single character into the network YORKBOX buffers */

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
		i = x25b_write_data(fid, 0, nptr->write_buffer, nptr->write_count);
		if(i != nptr->write_count) {
			L_WARN_0(L_GENERAL, 0, "Netwrite failed (net_putc)\n");
					/* call the error routine */
			nptr->write_count = 0;
			(*net_error)(nptr, 1, "??");
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
		static int type = 0;

		net_rcount =0;
		nptr->read_count = BLOCKSIZ;
		clocktimer();
		i = x25b_read_data(fid, &type, nptr->read_buffer, nptr->read_count);
		if (i<=0){
			L_LOG_0(L_GENERAL, 0, "Netread failed (net_getc)\n");
					/* call the error routine */
			nptr->read_count = 0;
			(*net_error)(nptr,0, "??");
			return(-1);
		}
		nptr->read_count = i;
		if(ftp_print & L_RECV_NET || type & (1<<Q_BIT))
		{	int mod = (nptr->read_count > 19) ? 16 :
				nptr->read_count;
			L_DEBUG_2(L_ALWAYS, 0, "%d chars read from net %02x:",
					nptr->read_count, type);
			for(i=0;i<nptr->read_count;i++){
				if( (i % mod) == 0) L_DEBUG_0(L_ALWAYS,
					L_CONTINUE, "\n");
				L_DEBUG_1(L_ALWAYS, ((i%mod) == 0) ? 0 :
				L_CONTINUE, "%02x ",nptr->read_buffer[i]&0xff);
			}
			L_DEBUG_0(L_ALWAYS, L_CONTINUE, "\n");
		}
		if (type & (1<<Q_BIT)) switch (nptr->read_buffer[0])
		{
		case YB_DISCONNECT:
		{	char *err	= "";
			char *who	= "<unset>";
			char *why	= "<unset>";
			char buff[3 * 0x80];
			long error	= -1;
			int rc = ts_buff_decode(nptr->read_buffer+1, buff,
				nptr->read_count, 3, &err, &who, &why);
			int len = who - err;

			if (len > 1 && len < 4)
				for(i=0, error=0; i<len-1; i++)
					error = (error<<8) | err[i];

			L_WARN_4(L_GENERAL, 0,
				"Disconnect %x from %s - %s (%d)\n",
				error, who, why, rc);
			if (!(allow_qmask & AQ_DISCONNECT))
			{	nptr->read_count = 0;
				(*net_error)(nptr, 0, "Disconnect");
				return -1;
			}
		}
		default:
			L_WARN_1(L_GENERAL, 0, "Qualified command %02x\n", 
				nptr->read_buffer[0] & 0xff);
			nptr->read_count = 0;
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

	i = x25b_write_data(fid, 0, nptr->write_buffer, nptr->write_count);

	if(i != nptr->write_count){
		L_WARN_3(L_GENERAL, 0, "Net write failed (net_push) (%d/%d) (%d)\n",
			i, nptr->write_count, errno);
					/* call error handler */
		nptr->write_count = 0;
		(*net_error)(nptr,1, "??");
		return(-1);
	}
	nptr->write_count = BLOCKSIZ;
	return(0);
}

/* close a connection */

con_close()
{
	register i;
	int tfid = fid;

	net_rcount=0;
	clocktimer();
	fid = -1;
	if(tfid >= 0)
		close(tfid);
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
	return(net_rcount < net_io.read_count /* ?? || tdata(NETTP) > 0 */ );
}

/*
 * kill off the connection. reseting all neccassary counters and flags.
 * do it as quickly and as quietly as possible
 */

killoff(state,errval,transtate)
int     state,errval,transtate;
{
	int tfid = fid;

	writedocket();
	readfail();

	fid = -1;
	if(tfid >= 0)
	{	
		starttimer(2*60);
		alarm(2*60);		/* temp HACK */
		L_LOG_1(L_80, 0, "Close the open channel (%x)\n", tfid);
		close(tfid);
	}

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

int x25b_debug;

x25b_logit(n, s, a,b,c,d)
{
	L_LOG_4(((n & X25B_L_DEBUG) ? (L_RECV_NET | L_SEND_NET) : L_GENERAL),
			(n & X25B_L_STAMP) ? 0 : L_CONTINUE, s, a,b,c,d);
}

x25b_perror(n, s, a,b,c,d)
{
	L_LOG_2(((n & X25B_L_DEBUG) ? (L_RECV_NET | L_SEND_NET) : L_GENERAL),
		((n & X25B_L_STAMP) == 0) ? L_CONTINUE:0, "n=%d, e=%d: ",
		n, errno);
	L_LOG_4(((n & X25B_L_DEBUG) ? (L_RECV_NET | L_SEND_NET) : L_GENERAL),
			L_CONTINUE, s, a,b,c,d);
}
