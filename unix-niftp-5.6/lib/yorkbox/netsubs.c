/* unix-niftp lib/yorkbox/netsubs.c $Revision: 5.6 $ $Date: 1991/06/07 17:02:31 $ */
#include "ftp.h"
#include "infusr.h"
#include <stdio.h>

/* Name clashes --- sigh */
#undef	DEBUG
#undef	ABORTED
#undef	TERMINATED
#include <netio.h>        /* ????? */

#ifdef	RESTARTSYS
/* BSD4.3 defines this in signal.h -- 4.2 doesn't */
# ifndef sigmask
#  define sigmask(m) (1 << ((m)-1))
# endif  sigmask
#endif	RESTARTSYS

/*
 * file:
 *                       netsubs.c
 *  YORK BOX CODE
 * last changed: 10-jul-85
 * $Log: netsubs.c,v $
 * Revision 5.6  1991/06/07  17:02:31  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:39:45  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:20:21  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:24:47  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:08:58  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/04/23  18:53:43  pb
 * Add code to release the channel if timeout while listening.
 * 
 * Revision 5.0  87/03/23  03:58:53  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 *
 * This file contains routines to interface to the YORKBOX. They
 * make the network seem like a normal file. Makes the interface nicer
 */


TS      *NETTP;         /* the GLOBAL TS pointer */

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
		i = twrite(NETTP, nptr->write_buffer, nptr->write_count, 0);
		if(i != nptr->write_count) {
			L_WARN_0(L_GENERAL, 0, "Netwrite failed (net_putc)\n");
					/* call the error routine */
			nptr->write_count = 0;
			(*net_error)(nptr,1,tmess(NETTP));
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
		i = tfetch(NETTP, nptr->read_buffer, nptr->read_count);
		if(tmess(NETTP) != NULL){
			L_LOG_0(L_GENERAL, 0, "Netread failed (net_getc)\n");
					/* call the error routine */
			nptr->read_count = 0;
			(*net_error)(nptr,0,tmess(NETTP));
			return(-1);
		}
		nptr->read_count = i;
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
	i = twrite(NETTP, nptr->write_buffer, nptr->write_count, 1);
	if(i != nptr->write_count){
		L_WARN_0(L_GENERAL, 0, "Net write failed (net_push)\n");
					/* call error handler */
		nptr->write_count = 0;
		(*net_error)(nptr,1,tmess(NETTP));
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
	TS *temptp = NETTP;

	net_rcount=0;
	clocktimer();
	NETTP = 0;
	if(temptp != NULL)
		tclose(temptp);
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
	return(net_rcount < net_io.read_count || tdata(NETTP) > 0);
}

/*
 * kill off the connection. reseting all neccassary counters and flags.
 * do it as quickly and as quietly as possible
 */

killoff(state,errval,transtate)
int     state,errval,transtate;
{
	extern	TS * TS_LISTENING;
	TS *temptp = NETTP;
	TS *templp;

	writedocket();
	readfail();

	/* This has been coded to be re-enterable
	 * If something goes wrong while doing a graceful close,
	 * then slam it shut ....
	 */
	NETTP = 0;
	if(temptp != NULL)
	{	
#ifdef RESTARTSYS
		int oldmask;
		
		/* reopen alarm trap */
		oldmask = sigblock(0);
		oldmask &= ~sigmask(SIGALRM);
		sigsetmask(oldmask);
#endif RESTARTSYS
		starttimer(2*60);
		alarm(2*60);		/* temp HACK */
		L_LOG_1(L_80, 0, "Close the open channel (%x)\n", temptp);
		tclose(temptp);
		TS_LISTENING = 0;
	}

	templp = TS_LISTENING;
	TS_LISTENING = 0;
	if (templp != NULL)
	{
		starttimer(2*60);
		alarm(2*60);		/* temp HACK */
		L_LOG_2(L_80, 0, "Nclose the listen channel (%x/%d)\n",
				templp, templp->t_fid);
		nclose(templp);
		templp->t_flags = TIDLE;  /* should be in nclose */
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
