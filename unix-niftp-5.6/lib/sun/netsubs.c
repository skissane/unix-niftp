#ifndef	lint			/* unix-niftp lib/sun/netsubs.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/sun/RCS/netsubs.c,v 5.6.1.8 1993/05/12 14:42:04 pb Rel $";
#endif	lint

/*
 * file:	netsubs.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: netsubs.c,v $
 * Revision 5.6.1.8  1993/05/12  14:42:04  pb
 * Distribution of May93FullSizeReadWithX25Header: Full Size read with X25_HEADER enabled discarded a byte
 *
 * Revision 5.6.1.7  1993/05/10  13:58:32  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC: Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.6  1993/01/10  07:12:07  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6.0.9  1991/06/07  16:58:41  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.6  1991/06/03  14:41:40  pb
 * make char -> unsigned char.
 * pass extra dummy argument.
 *
 * Revision 5.5  90/08/01  13:38:45  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:26:39  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:35:11  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 *
 * This file contains routines to interface to the SUNlink libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */
long	allow_qmask;
int	read_x25_bits;
int	is_cons;

#include "ftp.h"
#include "infusr.h"
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sundev/syncstat.h>
#include <netx25/x25_pk.h>
#include <netx25/x25_ioctl.h>
#include <netx25/x25_ctl.h>

#ifndef	_RESET
#define _RESET          0x1111	/* any silly value */
#endif	_RESET

/* It appears that Sun's YBTSD sets the socket into non-blocking mode.
 * This means that if data is produced faster than the socket can pump it onto
 * the network, the write will fail with "EAGAIN".
 * As my ybtsd does not set non-blocking mode, I don't see the problem.
 *
 * A *HACK* which appears to work was added by T.D.Lee@durham.ac.uk 93/03/31
 *
 * Should really set blocking mode -- there could be other failure modes!
 */
#define	FLOOD_FIRST	16	/* Initial wait is 1/FLOOD_FIRST seconds */
#define FLOOD_MAX	9	/* Number of loops, doubling sleep each time */

int	fid;

char    t_addr[ENOUGH];   /* holds translated host name */

/* On reading (recv), we ask for an extra header byte, via X25_HEADER,
 * to hold M/Q/D-bit information.   Thus we must use BLOCKSIZ+1.
 */
unsigned char	net_read_buffer[BLOCKSIZ + 1];
unsigned char	net_write_buffer[BLOCKSIZ];
int	read_set = 0;	/* +ve -> get header, -ve -> no header on read */
int	write_set = 0;	/* set M bit on each packet until flush */
extern	sig_bits;

/* put a single character into the network YORKBOX buffers */

net_putc(c)
int     c;
{
	register nodep nptr = &net_io;

	while(net_wcount >= nptr->write_count){ /* no space left in buffer */
		int flood_count;

		if (! write_set) set_write(fid);

		if (ftp_print & L_SEND_NET)
		{	register i;
			L_DEBUG_1(L_SEND_NET, 0, "buffer %d to net:",
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
		/* i = send(fid, nptr->write_buffer, nptr->write_count, 0); */
		for (flood_count=0; ; flood_count++) {
			X25_CAUSE_DIAG diag;
			int rc = write(fid, nptr->write_buffer, nptr->write_count);
			sig_bits &= ~0xf0;
			if(rc == nptr->write_count) break;

			/* Sun's YBTSD set non-blocking mode ... */
			if (errno==EAGAIN && flood_count<FLOOD_MAX) {
				usleep((1000000 / FLOOD_FIRST) << flood_count);
				continue;
			}
			/* OK -- a real error, or blocked for too long ... */
			bzero(&diag, sizeof diag);
			ioctl(fid, X25_RD_CAUSE_DIAG, &diag);
			L_WARN_4(L_GENERAL, 0,
				"Netwrite(%d) failed (net_putc) %d!=%d (%d), ",
				fid, rc, nptr->write_count, errno);
			L_WARN_4(L_GENERAL, L_CONTINUE,
				"diag=%x|%x: %02x %02x\n",
				diag.datalen,
				diag.flags,
				diag.data[0], diag.data[1]);
			L_WARN_1(L_GENERAL, 0, "after %d retries\n", flood_count);
					/* call the error routine */
			nptr->write_count = 0;
			(*net_error)(nptr, 1, "??");
			return(-1);
		}
		nptr->write_count = sizeof net_write_buffer;
		if (flood_count != 0) {
			L_WARN_1(L_10, 0,
			  "Netwrite saturation required %d retries (EAGAIN).\n",
				flood_count);
		}
	}
	nptr->write_buffer[net_wcount++]=c;  /* add the character to buffer */
	return(0);
}

#ifdef	NEED_SUNLINK_SEND_BUGFIX
static int sendx25 (fd, buf, count, flag)
int	fd, count, flag;
char	*buf;
{
	int n;
	int total = 0;
	fd_set wfds;

	while (count > 0) {
		n = send (fd, buf, count, flag);
		if (n == 0) {
			L_DEBUG_0(L_SEND_NET, 0, 
				  "send returns 0 bug - selecting");
			FD_SET (fd, &wfds);
			select (fd + 1, (fd_set *)0, &wfds, (fd_set *)0, NULL);
		}
		else if (n == -1)
			return -1;
		else {
			count -= n;
			buf += n;
			total += n;
		}
	}
	return total;
}
#endif	/* NEED_SUNLINK_SEND_BUGFIX */

/* read a single character from the network */

net_getc()
{
	register i;
	register nodep nptr = &net_io;

	while(net_rcount >= nptr->read_count){  /* read all the characters */
		if (! read_set) set_read(fid);

		net_rcount =0;
		nptr->read_count = sizeof net_read_buffer;
		clocktimer();
		do
		{	L_DEBUG_2(L_RECV_NET, 0, "Try %d on %d\n",
				nptr->read_count, fid);
			sig_bits |=  0xc0;
			i = recv(fid, nptr->read_buffer, nptr->read_count, 0);
			sig_bits &= ~0xf0;
		} while (i < 0 && errno == EINTR);
		if (i<=0){
			X25_CAUSE_DIAG diag;
			bzero(&diag, sizeof diag);
			ioctl(fid, X25_RD_CAUSE_DIAG, &diag);
			L_WARN_3(L_GENERAL, 0,
				"Netread(%d) failed (net_getc) %d (%d), ",
				fid, i, errno);
			L_WARN_4(L_GENERAL, L_CONTINUE,
				"diag=%x|%x: %02x %02x\n",
				diag.datalen,
				diag.flags,
				diag.data[0], diag.data[1]);
					/* call the error routine */
			nptr->read_count = 0;
			(*net_error)(nptr,0, "??");
			return(-1);
		}
		if (read_set > 0)
		{	read_x25_bits = nptr->read_buffer[0];
			bcopy(nptr->read_buffer+1, nptr->read_buffer, --i);
		}
		nptr->read_count = i;
		if(ftp_print & L_RECV_NET || read_x25_bits & (1<<Q_BIT))
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
		if (read_x25_bits & (1<<Q_BIT)) switch (nptr->read_buffer[0])
		{
		case YB_DISCONNECT:
		{	char *err	= "";
			char *who	= "<unset>";
			char *why	= "<unset>";
			char *dummy	= "<unset>";
			unsigned char buff[3 * 0x80];
			long error	= -1;
			int rc = ts_buff_decode(nptr->read_buffer+1, buff,
				nptr->read_count, 3, &err, &who, &why, &dummy);
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
			if (!(allow_qmask & (1 << (nptr->read_buffer[0]))))
			{	nptr->read_count = 0;
				if (is_cons)
				{	(*net_error)(nptr, 0, "Qual command");
					return -1;
				}
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

	/* Flush, so clear the more bit, write, then reset it */
	if (read_set > 0)
	{	int	send_type = 0;
		if (ioctl(fid, X25_SEND_TYPE, &send_type))
		{	L_WARN_1(L_GENERAL, 0, "Failed to clear MORE bit(%d)\n",
							errno);
		}
	}

	sig_bits |=  0xb0;
	/* i = send(fid, nptr->write_buffer, nptr->write_count, 0); */
	i = write(fid, nptr->write_buffer, nptr->write_count);
	sig_bits &= ~0xf0;

	if (read_set > 0)
	{	int	send_type = (0 << M_BIT);	/* not yet ... */
		if (ioctl(fid, X25_SEND_TYPE, &send_type))
		{	L_WARN_1(L_GENERAL, 0, "Failed to set MORE bit(%d)\n",
							errno);
		}
	}

	if(i != nptr->write_count){
		X25_CAUSE_DIAG diag;
		bzero(&diag, sizeof diag);
		ioctl(fid, X25_RD_CAUSE_DIAG, &diag);
		L_WARN_4(L_GENERAL, 0,
			"Netwrite(%d) failed (net_push) %d!=%d (%d), ",
			fid, i, nptr->write_count, errno);
		L_WARN_4(L_GENERAL, L_CONTINUE,
			"diag=%x|%x: %02x %02x\n",
			diag.datalen,
			diag.flags,
			diag.data[0], diag.data[1]);
					/* call error handler */
		nptr->write_count = 0;
		(*net_error)(nptr,1, "??");
		return(-1);
	}
	nptr->write_count = sizeof net_write_buffer;
	return(0);
}

/* close a connection */

con_close()
{
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

/* ARGSUSED */
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
		if (tstate &&
		   (tstate != STOPACKs) &&
		   (tstate != SFTs || !(read_x25_bits & (1 << Q_BIT))))
		{     L_WARN_2(L_MAJOR_COM, 0,
			"Total network failure in state %04x %x\n",
				tstate, read_x25_bits);
		}
		else L_LOG_0(L_MAJOR_COM, 0, "Network connection closed\n");
		if (tstate != STOPACKs) killoff(0,REQSTATE,0);
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

/* Set fd to be suitable for writing */
set_write(fd)
{
	int send_type = (1 << M_BIT);

	if (ioctl(fd, X25_SEND_TYPE, &send_type))
	{	L_WARN_1(L_GENERAL, 0, "Failed to set MORE bit(%d)\n",
						errno);
		return -1;
	}
	write_set++;
	return 0;
}

/* Set fd to be suitable for reading */
set_read(fd)
{
	int recv_type = 1;

	if (ioctl(fd, X25_HEADER, &recv_type))
	{	L_WARN_1(L_GENERAL, 0, "Failed to request HEADER (%d)\n",
						errno);
		return -1;
	}
	read_set++;
	return 0;
}

void sigurg()
{	int type;
	int rc;
	int urg_data = 0;

	/* Reset on non BSD ... ?
	signal(SIGURG sigurg);
	*/

	/* OK -- read the OOB data */
	while ((rc = ioctl(fid, X25_OOB_TYPE, &type)) != -1 && type)
	{
		urg_data ++;
		switch (type) {
		    case INT_DATA:
			{	char	data;
				int n;
				sig_bits |=  0xd0;
				n = recv(fid, &data, 1, MSG_OOB);
				sig_bits &= ~0xf0;
				L_LOG_2(L_ALWAYS, 0,
					"INT data (%d) was %02x\n",
					n, data);
			}
			break;
		    case VC_RESET:
			L_LOG_0 (L_ALWAYS, 0, "VC Reset\n");
			break;
		    default:
			L_LOG_1(L_ALWAYS, 0, "OOB type %02x\n", type);
			break;
		}
	}
	L_LOG_3(L_GENERAL, 0, "Last sigurg call gave %d (%d) %d items\n",
		rc, errno, urg_data);
	if (urg_data)
		killoff (0, TIMEOUTSTATE, 0); /* close connection */
}
