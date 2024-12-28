#ifndef	lint			/* unix-niftp lib/pqproc/tcccomm.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/tcccomm.c,v 5.6.1.6 1993/01/10 07:11:38 pb Rel $";
#endif	lint

/*
 * file tcccomm.c
 * last changed 22-Oct-84
 * $Log: tcccomm.c,v $
 * Revision 5.6.1.6  1993/01/10  07:11:38  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:01:25  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  14:01:21  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  90/03/31  13:41:18  pb
 * Initial patcehs for PP
 * 
 * Revision 5.3  89/07/16  12:03:49  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:52:48  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:22:36  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.5  88/01/29  07:39:38  pb
 * Distribution of Jan88ReleaseMod1: JRP fixes - tcccomm.c ftp.c + news sucking rsft.c + makefiles
 * 
 * Revision 5.0.1.3  87/12/09  16:59:16  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:53:56  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:50:28  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/
/*
 * routines to decode and to execute transport level commands (mr etc)
 * should do better checking of tstate.
 */


#include "ftp.h"
#include "stat.h"	/* S_PROCESS */

#ifndef	MAX_RESTARTS
#define	MAX_RESTARTS	10
#endif	MAX_RESTARTS

extern  char    reason[];
extern  char    whichhost[];
extern	char 	*strcat ();

#ifdef  MAIL
char    xerr;
#ifdef	PP
extern	pp_spooled;
extern char    ltranstype;
extern	int pp_rc;
extern	char * pp_errstring;
#define GB_NOTOK        NOTOK
#define GB_OK           OK
#define GB_RECIPFAILED  -2
#define GB_SENDERFAILED -3
#endif	PP
#endif
extern long mail_start;

#ifdef  PP
pp_reject(reason)
char *reason;
{	int soft = (pp_rc == GB_RECIPFAILED || pp_rc == GB_SENDERFAILED);
	L_WARN_2(L_ALWAYS, 0,
        "PP rejected the transfer -- pp_errstring = %x, rc = %d\n",
		pp_rc, pp_errstring);
	if(*reason) (void) strcat(reason, MSGSEPSTR);
	if (pp_errstring)
	{	(void) strcat(reason, "PP ");
		(void) strcat(reason, (soft) ? "soft" : "hard");
		(void) strcat(reason, " error message: ");
		(void) strcat(reason, pp_errstring);
		pp_errstring = (char *) 0;
	}
	else (void) strcat(reason,
		"PP rejected mail (should give more info)");
	send_comm(QR, (soft) ? 0x20 : 0x21, 1);
	tstate = QRe; /* send a failure here */
}
#endif  PP

tcc_command()
{
	register val    comm,arg;
	register val     temp;
	val     ststate = tstate;
	register i;

	/* Play around to avoid unsigned comparison with -1 */
	if((i = net_getc()) == -1) return(-1);
	comm = i;
	if((i = net_getc()) == -1) return(-1);
	arg = i;

	L_LOG_2(L_LOG_TCC, 0, "Got TCC command %02x - %02x\n", comm, arg);
	switch(comm){
	case SS:                                /* the SS command */
		if(direction != RECEIVE)
			goto bad;
		if(tstate != GOs && tstate != RRs && tstate != PERR)
			goto bad;
		if( (lastmark & MASK) != arg){          /* ????? help !! */
			L_WARN_2(L_GENERAL, 0, 
				"Got SS with bad number %d instead of %d\n",
				lastmark, arg);
			goto bad;
		}
		if (arg) L_LOG_2(L_GENERAL,0,"SS %02x (%02x)\n",lastmark,arg);
		code =0;                /* reset code */
		wordcount = 0;
		wrd_bin_siz = 1;        /* reset word size */
		if(tstate == PERR)
			tstate = PEND;
		else
			tstate = DATA;
		break;
	case MS:                                /* mark point */
		L_LOG_1(L_LOG_TCC, 0, "MK %02x\n",arg);
		if(direction != RECEIVE )
			goto bad;
		if(tstate == RRs || tstate == PERR)
		{	L_LOG_1(L_LOG_TCC, 0, "in state %d\n", tstate);
			break;
		}
		if(tstate != DATA && tstate != PEND)
			goto bad;
		if( ((rec_mark+1)&MASK) != arg){  /* out of sequence */
			if(! (facilities & RESTARTS ))
				(void)strcpy(reason, "Out of sequence marks");
			L_WARN_2(L_GENERAL, 0, "Lost a mark %d - %d\n",
				arg, rec_mark);
	nospace:

#ifdef  PP
                        if (PPTRANS && !pp_spooled)
                        {	pp_reject(reason);
				break;
			}
#endif  PP
			/* Do a restart ?? */
			if(!(facilities & RESTARTS) ||
				++nrestarts >= MAX_RESTARTS)
			{	if(!*reason)
				  (void) strcpy(reason, "Out of file space");
				send_comm(QR,0x20, 1);
				tstate = QRe; /* send a failure here */
				break;
			}
			L_LOG_2(L_MAJOR_COM, 0, 
				"Going to try to do a restart (%d of %d)\n",
					nrestarts, MAX_RESTARTS);
			send_comm(RR,(int)rec_mark&MASK, 1);
			bcount = last_count = lr_bcount;
			reclen = last_rlen = lr_reclen;
			lastmark = rec_mark;
			L_LOG_1(L_10, 0, "RR bcount = %ld\n",bcount);
			(void) lseek(f_fd,last_count,0);
			fnleft=0;               /* rewind counter */
			tstate = RRs;           /* wait for it */
			writedocket();
			break;
		}
		if(flush_f())                   /* flush out any data */
			goto nospace;
		/* say we now have the mark ok */

		lr_bcount = bcount;
		lr_reclen = reclen;

		if(rec_mark - lastmark > 5)
			writedocket();

		/* only acknowledge if window half full or so - tunable */
		if(++rec_mark - lastmark > acknowind / 2 ){
			send_comm(MR,(int)arg, 1);
			L_LOG_1(L_LOG_TCC, 0, "Ack %d\n", arg);
			lastmark = rec_mark;
			last_count = bcount;
			last_rlen = reclen;
			writedocket();
		}
		break;
	case CS:                                /* code select */
		if(direction != RECEIVE)
			goto bad;
		if(tstate == RRs || tstate == PERR)
			break;
		if(tstate != DATA && tstate != PEND)
			goto bad;
		wrd_bin_siz = 1;                /* set word size */
		switch( arg & 0xF ){
		case 0:                         /* IA5 */
			if(datatype & 0xD)      /* can have text */
				code = 0;
			else goto bad;
			break;
		case 1:                         /* Binary */
			if(!(datatype& 0xE))    /* can't have binary */
				goto bad;
			code = 01;
			wrd_bin_siz = bin_size;
			break;
		case 2:                         /* EBCDIC + */
		case 3:                         /* private code */
		default:                        /* not yet available */
			goto bad;
		}
		L_LOG_2(L_GENERAL, 0, "%s mode at %d\n",
			(code) ? "binary" : "text", bcount);
		break;
	case ES:                                /* end */
		if(direction != RECEIVE)
			goto bad;
		switch(arg & 0xF0){
		case 0:                                 /* ES(OK) */
			if(tstate == HOLD || tstate == HORR || tstate == GOs)
			{	L_LOG_1(L_LOG_TCC, 0,
					"tstate[%d] Wrong in ES(OK)\n", tstate);
				goto bad;
			}
			if(tstate != DATA)
			{	L_LOG_1(L_LOG_TCC, 0,
					"tstate[%d] wrong in ES(OK)\n", tstate);
				break;
			}

			/* If ANSI cc's, then there is an EOL at the EOF */
			if (code != 1 && format & 02 && bcount)
			{	put_c(NL);
				bcount++;
				L_LOG_0(L_GENERAL, 0, "Flush ansi NL at EOF\n");
			}
			if(flush_f())                   /* save all data */
			{
				L_LOG_0(L_LOG_TCC, 0, "flush_f() failed in ES(OK)\n");
				goto nospace;
			}
			L_LOG_0(L_LOG_TCC, 0, "ES(OK)\n");
			stat_state(S_PROCESS);
			if(lastmark != rec_mark)  /* always ackno all mks */
				send_comm(MR, (int)rec_mark & MASK, 1);
			time(&mail_start);
			(void) chmod(localname,0644);  /* give file to user */
			(void) chown(localname,uid,gid);
#ifdef	PP
			if(mailer > 1 && !pp_spooled)
			{	if ((i = pp_close(f_fd)) != 0)
				{	L_WARN_2(L_GENERAL, 0,
						"PP close(%d) failed %d!\n",
						f_fd, i);
					pp_rc = i;
					pp_reject(reason);
						/* set to tell him later*/
					ecode = ER_MAIL_END;
					break;
				}
			}
			else
#endif	PP
			if(give_file())          /* can't do it !!! */
			{	L_WARN_0(L_GENERAL, 0,
					"Recieving file deleted !!\n");
				send_comm(QR, 0x20, 1); /* send a failure */
				tstate = QRe;   /* yeuch !! */
					/* set to tell him later*/
				ecode = ER_RCVFILE_DELETED;
				break;
			}

#ifdef  MAIL
			L_WARN_1(L_GENERAL, 0, "Mailer=%d\n", mailer);
			if(mailer && (
#ifdef	PP
					(mailer > 1)
					? do_pp((pp_spooled)
					  ? (f_access & ACC_GET)
					    ? realname
					    : filename
					  : (char *) 0)
					: 
#endif	PP
					  do_mail((f_access & ACC_GET)
					  ? realname
					  : filename)
				     )
			  ){
				/* a mail failure tell the other side */
				tstate = QRe;
				send_comm(QR, xerr ? 0x21 : 0x20, 1);
				tstate = QRe;
				lastmark = INFINITY;    /* at end of file */
				if(!*reason)    /* if doing resumptions */
					(void) strcpy(reason,
						    "Unknown mailer failure");
				break;
			}
#endif	MAIL

			tstate = ESok;       /* transfered ok */
			send_comm(ER,0, 1);
			tstate = OKs;
			st_of_tran = TERMINATED;
			lastmark = INFINITY;    /* got to eof */
			break;
		case 0x10:                              /* ES(HOLD) */
			L_WARN_1(L_GENERAL, 0, "Got ES(H) %02x\n", arg);
			if(tstate == GOs)
				goto bad;
			if(tstate == PEND)              /* release hold */
				tstate = HOLD;
			if(tstate == PERR)
				tstate = HORR;
			break;
		case 0x20:                              /* ES(E) */
			L_WARN_1(L_GENERAL, 0, "Got ES(E) %02x\n", arg);
			send_comm(ER,(int)arg, 1);
			tstate = FAIL;
			st_of_tran = ABORTED | !(arg & 01);
			break;
		case 0x30:                              /* ES(A) */
			L_WARN_1(L_GENERAL, 0, "ES(A) %02x\n", arg);
			tstate = FAIL;
			st_of_tran = ABORTED;
			break;
		default:
			goto bad;
		}
		break;
	case RR:                                /* restart request */
		L_LOG_1(L_GENERAL, 0, "RR %d\n", arg);
		if(direction != TRANSMIT || !(facilities & RESTARTS))
			goto bad;                       /* can't */
		if(tstate==GOs || tstate==RRs || tstate==HORR || tstate==ESe)
			goto bad;
		temp = ((arg-rec_mark)&MASK);   /* calculate mark */
		temp += rec_mark;
		if(temp < rec_mark || temp > lastmark){         /* bad mark */
			(void) strcpy(reason, "Bad RR command");
			L_WARN_3(L_GENERAL, 0,  "RR out of bounds %d %d %d\n",
				temp,rec_mark,lastmark);
			goto bad;
		}
		bcount = temp;                          /* get the position */
		bcount <<= LSHIFT;                      /* 2k !! */
		reclen = docp[temp&MASK];
		L_LOG_2(L_10, 0, "Bcount = %ld reclen = %d\n",bcount,reclen);
		lseek(f_fd,bcount,0);                   /* seek it */
		lastmark = rec_mark = temp;             /* set marks */
		writedocket();                           /* update docket */
		fnleft =0;
		if(tstate == HOLD)              /* if holding --- wait */
			tstate = HORR;
		else tstate = RRs;
		break;
	case MR:                                        /* mark acknowledge */
		L_LOG_1(L_LOG_TCC, 0, "MR %02x\n", arg);
		if(direction != TRANSMIT || tstate == GOs || tstate == ESe)
			goto bad;
		temp = ((arg-rec_mark)&MASK);   /* calculate mark */
		if(!(facilities & MUSTACK))     /* no acknowledgement */
			break;  /* should deal with this better ??? */
		temp += rec_mark;
		if(temp < rec_mark || temp > lastmark ){
			L_WARN_3(L_GENERAL, 0,
				"MR %04x out of bounds %04x - %04x\n",
				arg,rec_mark, lastmark);
			goto bad;               /* error */
		}
		rec_mark = temp;                /* say we got it */
		if(tstate == MRs)               /* if waiting for it */
			tstate = DATA;          /* reset */
		break;
	case QR:                /* got an error - do something about it */
		if(direction != TRANSMIT)
			goto bad;
		switch(arg & 0xF0){
		case 0:                                 /* QR(OK) */
			if(tstate == RRs || tstate == HORR || tstate == ESe)
				goto bad;
			if(tstate == GOs)
				send_comm(SS,(int)lastmark & MASK, 1);
			L_LOG_0(L_LOG_TCC, 0, "QR(OK)\n");
			if(tstate != HOLD && tstate != ESok)
				send_comm(ES, 0, 1);
			if(tstate != HOLD)
				tstate = ESok;
			break;
		case 0x10:              /* QR(HOLD) - start hold */
			L_LOG_1(L_GENERAL, 0, "QR(H) %02x\n", arg);
			if(!(facilities & HOLDS) || tstate == ESe)
				goto bad;
			if(tstate==GOs)
				send_comm(SS,(int)lastmark & MASK, 1);
			if(tstate != HOLD && tstate != HORR)
				send_comm(ES,0x10, 1);
			if(tstate == RRs)
				tstate = HORR;
			else if(tstate != HORR)
				tstate = HOLD;
			break;
		case 0x20:                              /* QR(E) */
			L_LOG_1(L_GENERAL, 0, "QR(E) %02x\n", arg);
			if(tstate != ESe){
				send_comm(ES,(int)arg, 1);
			}
			if(arg != 0x22)
				(void) strcpy(whichhost, "remote");
			tstate = ESe;                   /* error */
			if(arg & 01)
				may_resume = 0;
			if(!*reason)
				sprintf(reason, "Got a QR(%x)", arg);
			break;
		case 0x30:                      /* QR(A) */
			L_LOG_1(L_GENERAL, 0, "QR(A) %02x\n",arg);
			(void) strcpy(whichhost, "remote");
			tstate = FAIL;
			st_of_tran = ABORTED | ABORTED_POSS;
			break;
		default:
			goto bad;
		}
		break;
	case ER:                                /* END command */
		if(direction != TRANSMIT)
			goto bad;
		switch(arg & 0xF0){
		case 0:                                 /* ER(OK) */
			if(tstate != ESok && tstate != ESe)
				goto bad;
			/* I think we have done it */
			L_LOG_0(L_LOG_TCC, 0, "ER(OK)\n");
			tstate = OKs;
			/* check the marks */
			if(lastmark != rec_mark) L_WARN_2(L_LOG_TCC, 0, 
				"lost some marks along the way - %d%s\n",
				lastmark-rec_mark,
				(facilities & MUSTACK) ? 
				"should be acknowledged" : "");
			lastmark = INFINITY;            /* at eof */
			st_of_tran = TERMINATED;
			break;
		case 0x10:                              /* ER(H) */
			L_LOG_1(L_GENERAL, 0, "ER(H) %02x\n", arg);
			if(tstate == HOLD)
				tstate = DATA;
			else if(tstate == HORR)
				tstate = RRs;
			else goto bad;
			break;
		case 0x20:                              /* ER(E) */
			if(tstate != ESe)               /* error end */
				goto bad;
			L_LOG_1(L_GENERAL, 0, "ER(E) %02x\n", arg);
#ifdef  MAIL
			if(xerr & !(arg & 01)) /* if we really can't cont */
				arg |= 01;
#endif
			tstate = FAIL;
			st_of_tran = ABORTED | !(arg & 01);
			break;
		default:
			goto bad;
		}
		break;
	default:                                        /* junk !!!! */
		L_WARN_1(L_GENERAL, 0, "Bad tcccomm command %02x\n", comm);
		goto bad;
	}
	/* come here if all is ok */
	if(tstate != ststate )          /* if changed state in this routine */
		return(TCC_COMMAND);    /* tell a higher level */
	else
		return(0);
/*
 * get here if got some problem. Treat as a protocol error since that is what
 * it is.
 */
bad:
	if(tstate != ESe && tstate != QRe ){/* prob. got a protocol error */
		if(!*reason)
			(void) strcpy(reason, "TCC protocol error");
		prot_err(123, comm, arg);     /* if returns then must have changed state */
	}
	return(TCC_COMMAND);            /* inform higher levels */
}

send_comm(comm,arg,fl_flag)             /* send a tcc command */
int     comm,arg,fl_flag;               /* fl_flag set if need to push */
{
	L_LOG_4(L_LOG_TCC, 0, "Sending %02x %02x (%x%s)\n",
			comm, arg, fl_flag, (fl_flag) ? " Flush" : "");
	end_sub_rec();
	net_putc(0);
	net_putc(comm);
	net_putc(arg);
	if(fl_flag)
		net_flush();
}

/* deal with protocol errors. If not in data phase just kill the connection */

prot_err(arg1, arg2, arg3)
{
	L_WARN_4(L_GENERAL, 0, 
		"Got a protocol error in %02x (%d, %04x, %04x)!!\n",
			tstate,	arg1, arg2, arg3);
	if(++nproterrs > 10){
		L_WARN_0(L_GENERAL, 0, 
			"Give up ( recursive protocol error ? )\n");
		killoff(0, REQSTATE, 0);
	}
	if(direction == TRANSMIT){              /* am in data phase */
		L_LOG_0(L_LOG_TCC, 0, "Sending ES(E)\n");
		send_comm(ES,0x2A, 1);
		tstate = ESe;
		while(tstate == ESe)
			get_comm();
	}
	else if(direction==RECEIVE){
		L_LOG_0(L_LOG_TCC, 0, "Sending QR(E)\n");
		send_comm(QR,0x22, 1);
		tstate = QRe;
		return;
	}
	else {
		if(tstate == STOPACKs)
			return;         /* horrible - I think we may hang */
		L_WARN_0(L_GENERAL, 0, "PROTOCOL ERROR\n");
		killoff(0, REQSTATE, ABORTED | 01);
	}
}
