#ifndef	lint			/* unix-niftp lib/seq/netsubs.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/seq/netsubs.c,v 5.6 1991/06/07 17:02:34 pb Exp $";
#endif	lint

/*
 * file:	netsubs.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk> original for SunLink
 * Allan Black <allan@uk.ac.strath.cs> Sequent X.25 (Morningstar Technologies)
 *
 * $Log: netsubs.c,v $
 * Revision 5.6  1991/06/07  17:02:34  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:56  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 *
 * This file contains routines to interface to the Symmetry X.25 board.
 * They make the network seem like /dev/abacus. Makes the interface more fun.
 */
long	allow_qmask;
int	read_x25_bits;

#include "ftp.h"
#include "infusr.h"
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <x25/x25.h>
#include <x25/x25errno.h>

#define GFI	0x10

#ifndef	_RESET
#define _RESET          0x1111	/* any silly value */
#endif	_RESET

int	fid;

char	pkt_buf[2051];
int	pkt_len;

char    t_addr[ENOUGH];   /* holds translated host name */
unsigned char	net_read_buffer[BLOCKSIZ];
unsigned char	net_write_buffer[BLOCKSIZ];
int	write_set = 0;	/* set M bit on each packet until flush */
extern	sig_bits;

char	*pkt_octet();

net_putc(c)
int     c;
{
	register i;
	register nodep nptr = &net_io;

	while(net_wcount >= nptr->write_count){ /* no space left in buffer */
		if(ftp_print & L_SEND_NET)
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
		sig_bits |=  0xa0;
		setmode(IOX25DATA);
		i = write(fid, nptr->write_buffer, nptr->write_count);
		sig_bits &= ~0xf0;
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
		net_rcount =0;
		nptr->read_count = BLOCKSIZ;
		clocktimer();
		do
		{	L_DEBUG_1(L_RECV_NET, 0, "Try %d\n", nptr->read_count);
			sig_bits |=  0xc0;
			setmode(IOX25PKT);
			i = read(fid, nptr->read_buffer, nptr->read_count);
			sig_bits &= ~0xf0;
		} while(i < 0 && errno == EINTR);
		if(i<=0){
			L_LOG_0(L_GENERAL, 0, "Netread failed (net_getc)\n");
					/* call the error routine */
			nptr->read_count = 0;
			(*net_error)(nptr,0, "??");
			return(-1);
		}
		read_x25_bits = nptr->read_buffer[0];
		bcopy(nptr->read_buffer+3, nptr->read_buffer, i -= 3);
		nptr->read_count = i;
		if(ftp_print & L_RECV_NET || read_x25_bits & (1 << SET_Q))
		{	int mod = (nptr->read_count > 19) ? 16 :
				nptr->read_count;
			L_DEBUG_2(L_RECV_NET, 0, "%d chars read from net %02x:",
					nptr->read_count, read_x25_bits);
			for(i=0;i<nptr->read_count;i++){
				if( (i % mod) == 0) L_DEBUG_0(L_RECV_NET,
					L_CONTINUE, "\n");
				L_DEBUG_1(L_RECV_NET, ((i%mod) == 0) ? 0 :
				L_CONTINUE, "%02x ",nptr->read_buffer[i]&0xff);
			}
			L_DEBUG_0(L_RECV_NET, L_CONTINUE, "\n");
		}
		if(read_x25_bits & (1 << SET_Q)) switch(nptr->read_buffer[0])
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

			if(len > 1 && len < 4)
				for(i=0, error=0; i<len-1; i++)
					error = (error<<8) | err[i];

			L_WARN_4(L_GENERAL, 0,
				"Disconnect %x from %s - %s (%d)\n",
				error, who, why, rc);
			if(!(allow_qmask & AQ_DISCONNECT))
			{	nptr->read_count = 0;
				(*net_error)(nptr, 0, "Disconnect");
				return -1;
			}
		}
		default:
			L_WARN_4(L_GENERAL, 0,
				"Qualified command %d: %02x %02x %02x\n", 
				nptr->read_count,
				nptr->read_buffer[0] & 0xff,
				nptr->read_buffer[1] & 0xff,
				nptr->read_buffer[2] & 0xff);
			L_WARN_4(L_GENERAL, 0, "qm=%x, v=%x ==> %x so %d\n",
				allow_qmask, 1 << (nptr->read_buffer[0]),
				allow_qmask & (1 << (nptr->read_buffer[0])),
				!(allow_qmask & (1 << (nptr->read_buffer[0]))));
			if(!(allow_qmask & (1 << (nptr->read_buffer[0]))))
			{	nptr->read_count = 0;
				/*(*net_error)(nptr, 0, "Qual command");*/
				/*return -1;*/
			}
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

	sig_bits |=  0xb0;
	setmode(IOX25DATA);
	i = write(fid, nptr->write_buffer, nptr->write_count);
	sig_bits &= ~0xf0;

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
	int x;

	net_rcount=0;
	clocktimer();
	ioctl(fid, IOX25DATA, x); /* don't care if it fails */
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
		if(tstate &&
		   (tstate != STOPACKs) &&
		   (tstate != SFTs || !(read_x25_bits & (1 << SET_Q))))
		{     L_WARN_2(L_MAJOR_COM, 0,
			"Total network failure in state %04x %x\n",
				tstate, read_x25_bits);
		}
		else L_LOG_0(L_MAJOR_COM, 0, "Network connection closed\n");
		if(tstate != STOPACKs) killoff(0,REQSTATE,0);
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

oobmsg()
{
	register int i;
	int event, size;
	char buf[128];
	if(ioctl(fid, IOX25STR, &event) < 0) {
		L_WARN_2(L_GENERAL, 0, "IOX25STR failed (%d/%s)\n",
			errno, x25error());
		return;
	}
	switch((event >> 24) & 0xff) {
	case 0:
		/* "spurious" event? */
		break;
	case CLEAR_INDICATION:
		L_LOG_2(L_GENERAL, 0, "Call cleared by X.25 (%d/%d)",
			(event >> 16) & 0xff, (event >> 8) & 0xff);
		if(event & 0xff) {
			if((size = read(fid, buf, sizeof buf)) <= 0)
				L_LOG_1(L_GENERAL, L_CONTINUE,
					" read clear data failed %d", errno);
			else
				for(i = 0; i < size; i++)
					L_LOG_1(L_GENERAL, L_CONTINUE, " %02x",
						buf[i]);
		}
		L_LOG_0(L_GENERAL, L_CONTINUE, "\n");
		killoff(0,REQSTATE,0);
		break;
	case RESET_INDICATION:
		L_LOG_2(L_GENERAL, 0, "Reset %d/%d\n",
			(event >> 16) & 0xff, (event >> 8) & 0xff);
		break;
	case INTERRUPT:
		L_LOG_1(L_GENERAL, 0, "Interrupt %d\n", (event >> 16) & 0xff);
		break;
	case INTER_CONFIRMATION:
		L_LOG_0(L_GENERAL, 0, "Interrupt confirmation\n");
		break;
	default:
		L_WARN_1(L_GENERAL, 0, "Unknown OOB type %x\n", event);
		break;
	}
}

setmode(mode)
	int mode;
{
	int oldmode;
	static int currmode;
	if(mode != currmode) {
		if(ioctl(fid, mode, &oldmode) < 0)
			L_WARN_2(L_GENERAL, 0, "%s failed (%s)\n",
				mode == IOX25DATA ? "IOX25DATA" : "IOX25PKT",
				x25error());
		currmode = mode;
	}
}

new_pkt(qbit, pti)
	int qbit, pti;
{
	pkt_buf[0] = qbit ? SET_Q : 0;
	pkt_buf[1] = 0;
	pkt_buf[2] = pti;
	pkt_len = 3;
}

char *pkt_octet(octet)
	int octet;
{
	char *ptr;
	ptr = pkt_buf+pkt_len;
	*ptr = octet;
	pkt_len++;
	return ptr;
}

