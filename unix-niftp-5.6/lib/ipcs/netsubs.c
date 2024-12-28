/* unix-niftp lib/ipcs/netsubs.c $Revision: 5.5 $ $Date: 90/08/01 13:36:01 $ */
#include "ftp.h"
#include "infusr.h"
#include "csinterface.h"
#include <stdio.h>

/*
 * THIS FILE HAS BEEN EXTENSIVLY MODIFIED (INTERFACE RESTRUCTURING)
 * SINCE LAST USE, IT MAY NOT EVEN COMPILE.
 * IF YOU INTEND TO USE IT PLEASE CHECK IT OUT CAREFULLY.
 * wja@nott.cs
 */
/*
 * file:
 *                       netsubs.c
 *
 * last changed: 10-jul-85
 * $Log:	netsubs.c,v $
 * Revision 5.5  90/08/01  13:36:01  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:47:49  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:42:39  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

/*
 * This file contains routines to interface to the IPCS system. They
 * make the network seem like a normal file. Makes the interface nicer
 */

char    t_addr[ENOUGH];   /* holds translated host name */
struct netnode net_node;
int	ipcs_print;		/* more diagnostic printing */
				/* if this is not used by the IPCS
				   library it should be removed */

/* put a single character into the network (IPCS) buffers */

net_putc(c)
int     c;
{
	register i;
	register struct netnode *ipcsp = &net_node;
	register node *iop = &net_io;

	while(net_wcount >= iop->write_count){ /* no space left in buffer */
		if(ftp_print & 040000){ /* print all sent on net */
			printf("buffer to net:\n");
			for(i=0;i<net_wcount;i++){
				if( (i % 16) == 15)
					printf("\n");
				printf("%o ",iop->write_buffer[i]&0377);
			}
			printf("\n");
		}
		ipcsp->write_count = net_wcount;
		net_wcount = 0;
		ipcsp->timeout = time_out;
		ipcsp->flags &= ~CS_PUSHBIT;
		clocktimer();           /* clock the timeout system */
		if( (i = netwrite(ipcsp)) < 0){
			if(ftp_print & 1)
				printf("Netwrite failed (net_putc) %d\n",i);
			(*net_error)(ipcsp,1,i); /* call the error routine */
			return(-1);
		}
		iop->write_count = ipcsp->write_count;
	}
	iop->write_buffer[net_wcount++]=c;  /* add the character to buffer */
	return(0);
}

/* read a single character from the network */

net_getc()
{
	register i;
	register struct netnode *ipcsp = &net_node;
	register node *iop = &net_io;

	while(net_rcount >= iop->read_count){  /* read all the characters */
		net_rcount =0;
		ipcsp->read_count = READSIZE;
		ipcsp->timeout = time_out;
		clocktimer();
		if( (i=netread(ipcsp)) < 0){	 /* read some more */
			if(ftp_print & 1)
				printf("Netread failed (net_getc) %d\n",i);
			(*net_error)(ipcsp,0,i); /* call the error routine */
			return(-1);
		}
		if(ipcsp->read_count == 0 && (ipcsp->flags & CS_CLOSEDBIT)){
			if(ftp_print & 1)
				printf("Net closed !\n");
			(*net_error)(ipcsp, 0, -1);
			return(-1);
		}
		iop->read_count = ipcsp->read_count;
		if(ftp_print & 040000){ /* print all received */
			printf("%d chars read from net:",iop->read_count);
			for(i=0;i<iop->read_count;i++){
				if( (i % 16) == 15)
					printf("\n");
				printf("%o ",iop->read_buffer[i]&0377);
			}
			printf("\n");
		}
	}
	return(iop->read_buffer[net_rcount++] & MASK);
						/* return the character */
}

/* flush out all characters waiting in buffers */

net_flush()
{
	register i;
	register struct netnode *ipcsp = &net_node;
	register node *iop = &net_io;

	if(ftp_print & 040000){
		printf("net_flush:data sent:-\n");
		for(i=0;i<net_wcount;i++){
			if( (i % 16) == 15)
				printf("\n");
			printf("%o ",iop->write_buffer[i]&0377);
		}
		printf("\n");
	}
	ipcsp->write_count= net_wcount;
	net_wcount=0;
	ipcsp->timeout = time_out;
	ipcsp->flags |= CS_PUSHBIT;	 /* really flush it */
	clocktimer();
	i = netwrite(ipcsp);
	ipcsp->flags &= ~CS_PUSHBIT;
	if(i<0){
		if(ftp_print & 1)
			printf("Net write failed (net_push) %d\n",i);
		(*net_error)(ipcsp,1,i);	 /* call error handler */
		return(-1);
	}
	iop->write_count = ipcsp->write_count;
	return(0);
}

/* close a connection */

con_close()
{
	register i;
	register struct netnode *nptr = &net_node;

	net_rcount=0;
	nptr->timeout = time_out;
	clocktimer();
	netclose(nptr);
#ifdef UCL_STATS
	/* if an x-25 call display clearing codes - and
		accounting info */
	if(ftp_print & 2)
		printf("x-25 call closed:cause %d, code %d\n");
#endif
	stoptimer();            /* stop the timeout timer */
	net_open=0;
}

net_null_routine()      /* this is to stop recursive net_error's */
{}                      /* set net_error to it after a timeout */

/*
 * this is the normal routine that is called by the above routines
 * when a network error occurs
 */

net_fail(nptr,dir,e)
register struct netnode *nptr;
int     dir;            /* flag to say if reading or writting to network */
int     e;              /* the error code */
{
/*
 * have got an error that we can't recover from
 * close the call and rewrite the queue entry
 */
	if(!direction || !(nptr->flags & (CS_RESETBIT|CS_T_OUTBIT))
		|| e != -1 || (nptr->flags & CS_CLOSEDBIT) ){
		if(tstate == STOPACKs)          /* failure at finish */
			return;                 /* ignore in this state */
#ifdef  DEBUG
		if(ftp_print & 2)
			printf("Total network failure\n");
#endif
		killoff(0,REQSTATE,0);
	}
/*
 * we can only deal sensibly with resets and timeouts
 */
	if(nptr->flags & CS_T_OUTBIT){          /* a timeout has occured */
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
#ifdef  DEBUG
			else if(ftp_print & 2)
				printf("Unknown state error\n");
#endif
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
#ifdef  DEBUG
			else if(ftp_print & 2)
				printf("Unknown state error\n");
#endif
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
		if(ftp_print & 2)
			printf("Going to try to do a restart\n");
		send_comm(RR,(int)rec_mark&MASK,1);       /* do a restart */
		bcount = last_count = lr_bcount;
		reclen = last_rlen = lr_reclen;
		lastmark = rec_mark;
#ifdef  DEBUG
		if(ftp_print & 020)
			printf("RR bcount = %ld\n",bcount);
#endif
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
	return(net_rcount < net_io.read_count || (net_node.flags & CS_DATABIT));
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
	netabort(&net_node);
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

/* print 'called at time' */

ptime()
{
	long l;
	char *ctime();

	(void) time( (int *)&l);
	printf("call received at %s", ctime( (int*)&l) );
}
