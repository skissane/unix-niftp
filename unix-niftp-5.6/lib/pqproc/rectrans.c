#ifndef	lint			/* unix-niftp lib/pqproc/rectrans.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/rectrans.c,v 5.6.1.2 1992/10/17 05:32:06 pb Rel $";
#endif	lint

/*
 * these are the main routines to send and receive a file. These
 * pass data to or get data from the record handling routines. This puts
 * compression and level 2 command decoding in other routines. Making
 * them simpler and more general.
 *
 * $Log: rectrans.c,v $
 * Revision 5.6.1.2  1992/10/17  05:32:06  pb
 * improve error logging (of padding chars) and send HARD error if too man restarts
 *
 * Revision 5.6.1.1  1991/09/17  06:38:11  pb
 * Flush out last record
 *
 * Revision 5.6  1991/06/07  17:02:01  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:22  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:51:03  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:24:48  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:59:03  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:53:50  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:49:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 */

/*
 * To increase throughput between local hosts and look alikes, the routines
 * Have been optimised for the communication with its twin process.
 *   The routines still work in the same way but for special cases, they are
 *   Optimised.
 */

#include "ftp.h"

#include <errno.h>
extern  errno;
extern  char    reason[];

char    last_c;         /* last character read - for ANSI control standards */
char    last_state;     /* last command sent was a SS don't send a mark yet */
#ifdef  PP
int pp_spooled;
extern char    ltranstype;
int	pp_rc;
#endif  PP


/*
 * MRS_STATE is true if when in the data phase we should go into the wait
 * For acknowledge state.
 */
#define MRS_STATE (lastmark-rec_mark >= acknowind && (facilities&MUSTACK))
#define	SEND_C_MASK	0xff	/* Do NOT strip parity on SENDING */

/* send a file. */

transmit()
{
	L_LOG_0(L_MAJOR_COM, 0, "Going to do a transmit\n");
	if(tstate == GOs) {                     /* for error recovery */
		if(datatype & 0xE)              /* sending a binary file */
			code = 1;
		reclen = last_rlen;             /* initialise pointers etc. */
		lastmark = rec_mark;         /* mainly needed on resumption */
		bcount = lastmark;
		bcount <<= LSHIFT;
		docp[lastmark&MASK] = reclen;
again:
		last_state =1;
		send_comm(SS,(int)lastmark&MASK, 0);
		tstate = DATA;          /* say we are in the data phase !! */
		if(code)                /* say we are not in text mode */
			send_comm(CS,(int)code, 0);
		else if(!reclen && (format & 02))/* ansi standard control */
			last_c = NL;            /* first byte sent is a NL */
		else last_c = 0;                /* wrong for resume (bug) */
	}
	while(tstate != FAIL && tstate != OKs ){
		if(tstate == DATA && MRS_STATE )
			tstate = MRs;        /* wait for an acknowledgement */
		if(tstate != DATA ){
			if(tstate == RRs)
				goto again;
			get_comm();             /* got a command - decode */
			continue;
		}
		if( send_rec() && tstate != FAIL){ /* where we want to be */
			send_comm(ES, 0, 1);            /* end of file */
			tstate = ESok;
		}
	}
}

/*
 * routine to send a record.
 * Marks are inserted at every 2k point unless a SS command has been sent
 * This is flagged by a flag ( r_flag ) which is set every block ( 1/2 of 1 k)
 */

send_rec()
{
	register int    c=0;
	register int    format_flag;
	register int    i;
	register unsigned char   *xp;
	static  unsigned char    *ptr;

	format_flag = (code == 0) && (format & 0x7F);
		/* this is set only if formatting is required */
		/* it is only used to improve throughput */

	if(code==1)                     /* set the word size if binary */
		wrd_bin_siz = bin_size;
	init_rec();                     /* start the record */
	if(!code && (format & 02))      /* check this ansi control code */
		if(last_c){             /* should also have '0' and others */
			switch(last_c){
			case NL:	c = ' ';			break;
			case CR:	c = '+';			break;
			case NP:	c = '1';			break;
			}
			add_to_rec(c);
			last_c =0;
		}

/*
 * main loop. Send the record until end of file or maximum record size is
 * reached. This routine returns 1 if found the end of file. zero otherwise
 */
	while(reclen <= max_rec_siz || max_rec_siz==INFINITY) if(tstate == DATA)
	{	/* Fill read buffer if necessary */
		if(fnleft<=0)
		{	if((fnleft=(*readp)(f_fd,(char *)io_buff,BLOCKSIZ)) <= 0)
			{	if (fnleft == 0) { c = -1; break; } /* EOF*/
				(void) sprintf(reason,
					       "Internal read of data failed %d",
					       errno);
				send_comm(ES, 0x28, 1); /* hope to recover from it */
				tstate = ESe;
				zap_record();           /* No more data */
				return(0);
			}
			ptr = io_buff;
			L_LOG_1(L_BCOUNT, 0, "Bcount = %ld\n", bcount);
			stat_val(bcount);
			stat_sync();	/* show progress */

			/* time to check for input and add a mark every 2k ??*/
			if( (bcount& MARK_MASK ) == 0 && !last_state){
				if(facilities & (LRESUME|RESTARTS|MUSTACK))
				{	end_sub_rec();
					lastmark++;
					docp[lastmark&MASK] = reclen;
					L_LOG_2(L_MAJOR_COM, 0, "MS %02x - %x\n",
						lastmark,
				facilities & (LRESUME|RESTARTS|MUSTACK));
					send_comm(MS, (int)lastmark&MASK,
						MRS_STATE);
				}
				else	L_LOG_0(L_MAJOR_COM, 0, "ms\n");
				/* get incomming commands */
				while(net_more() && tstate==DATA ) get_comm();
				if(tstate == DATA && MRS_STATE) tstate = MRs;
				if(tstate != DATA) /* changed state */
						continue;
			}
			else	last_state=0;
		}
		if(!format_flag ){
			/*
			 * if no formatting actions then quick move
			 * into the output buffer. we can do no
			 * work on this buffer.
			 */
			if(max_rec_siz == INFINITY){
				add_bto_rec(fnleft, ptr);
				bcount += fnleft;
				fnleft = 0;
				continue;
			}
			if (reclen >= max_rec_siz) break;
			/*
			 * now work out how much data we can move to
			 * stop us from going over the record limit.
			 * I hope my maths is accurate. Though we do
			 * have a fire wall to save us. We will never
			 * get to this piece of code with max_rec_siz
			 * equal to reclen or a zero fnleft.
			 * So we should always get a count of i > 0.
			 */
			i = (max_rec_siz - reclen) * 
				((wrd_bin_siz == 1) ? 1 :
					(wrd_bin_siz - wdbcount));
			if(i > fnleft)	i = fnleft;
#ifdef  DEBUG
			if(i <= 0){
				L_WARN_1(L_GENERAL, 0, 
				    "Internal count error %d\n",i);
				i = 1;
			}
#endif
			add_bto_rec(i, ptr);
			bcount += i;
			fnleft -= i;
			ptr += i;
			continue;
		}
/*
 * Only get here if we are in text mode ( code == 0 )
 *  Now search for the first control character with formatting actions
 */
		/*
		 * if next char is a control character process
		 * it singularily.
		 */
		if(*ptr < ' '  && *ptr != '\t'){
			bcount++;
			fnleft--;
			switch(c = (*ptr++ & SEND_C_MASK) ){
			case NL:	if(format & 0x2){
						last_c= c;
						goto out;
					}
					if(format & (0x1|0x4))	goto out;

					/* (0x8|0x10|0x20|0x40) */
					/* all other cases */

					if(reclen > max_rec_siz)
						if(trecord()) return(0);
					add_to_rec(CR);
						/* Problems with formatting */
					if(reclen > max_rec_siz)
						if(trecord()) return(0);
					break;

			case NP:	if(format & (0x8 | 0x10)) goto out;
					/* fall through */
			case CR:	if(format & 0x2){
						last_c =c;
						goto out;
					}
					break;
			}
			if(reclen > max_rec_siz) if(trecord()) return(0);
			add_to_rec(c);
			continue;
		}
		/*
		 * now seach for the first control char in the
		 * buffer. start at one since test above gets the
		 * first char (*ptr)
		 */
		for( i = 1 , xp = ptr + 1; i < fnleft ; i++, xp++)
			if(*xp < ' ' && *xp != '\t')
				break;
		/*
		 * can now do block moves
		 */
		if(max_rec_siz-reclen < i && max_rec_siz != INFINITY){
			/*
			 * we must have a record too long.
			 * If no formating actions just end record
			 * after this block.
			 */
			if(format & (0x20|0x40) )
				i = max_rec_siz - reclen;
			else
				break;
		}
		add_bto_rec(i, ptr); /*record counting is done here */
		bcount += i;
		fnleft -= i;
		ptr += i;
	}
	else if(tstate==RRs){        /* got a RR. deal with it */
		send_comm(SS,(int)lastmark&MASK, 0);
		tstate = DATA;
		if(code){
			send_comm(CS,(int)code, 0);
			if(code==1) wrd_bin_siz = bin_size;
		}
		last_state =1; /* if you don't believe me - tough */
	}
	else if(tstate == FAIL || tstate == OKs){
		zap_record();
		return(0);
	}
	else {          /* tstate != DATA */
		get_comm();
		if(tstate == DATA && MRS_STATE)
			tstate = MRs;
	}
	/* the end of the while loop */

/*
 * If get here with formatting at end of records or at start
 * What should we do ? I think we should produce a protocol error
 * since the reciever will think that there is a formatting action here
 * When there is none.
 *  We only get here if max_rec_siz != INFINITY or at eof when c < 0
 */
	if(c >= 0 && code == 0 && (format & (0x1|0x2|0x4|0x8|0x10)) ){
		/* Record too big !! for formatting actions */
		/* send an error */
		(void) sprintf(reason,
			"Remote record buffer size exceeded (%d/%d)",
			reclen, max_rec_siz);
		send_comm(ES, 0x29, 1); /* Can't recover from it */
		tstate = ESe;
		zap_record();           /* No more data */
		return(0);
	}

/* this could be done higher up ( near the top ) but then why should it ? */

	if(c<0){                        /* got to end of file */
		extern	unsigned char *rec_buf;

		if(code==1)             /* pad out last binary word */
			while(wdbcount)
				add_to_rec(0);

		/* If just a CC, strip off one newline */
		if (reclen == 1 && (format & 0x2)) switch(rec_buf[1])
		{
		case '+':	/* Boggle -- end with CR -> ???? */
		case ' ':	reclen --;		break;
		case '0':	rec_buf[0] = ' ';	break;
		}

		if(reclen == 0 && code == 0 && (format & 0x2))
			zap_record(); /* stop an extra line at the end */
		else {
			/*
			 * Will all sites cope with end of file not on
			 * end of record boundry ?
			 * If No formatting actions then use end_record
			 * else end the subrecord
			 * (I hope I do )
			 */
			if(code == 0 && (format & (0x1|0x4|0x8|0x10)) ){
				end_sub_rec();
				zap_record();
			}
			else
				end_rec();
		}
		return(1);
	}

	/* end of record for formatting */
out:
	end_rec();
	return(0);
}

/*
 * Routine to fix end of record when using spurious formatting types
 *  Basically the problem is that we have got to the end of the record
 *  and we want to finish it but this would then cause a problem with
 *  formatting since end of record has formatting significance.
 *
 */

trecord()
{
	if(max_rec_siz == INFINITY)
		return(0);              /* Phew ! No problem */

	if(format & (0x20|0x40) ){
		/*
		 * No significance to end of record. Just finish
		 * Current record and start a new one.
		 */
		end_rec();
		init_rec();
		return(0);
	}
	/*
	 * We have a problem. End of record has formatting significance
	 * And we have too much data to go into the record....
	 */
	send_comm(ES, 0x29, 1);         /* send an error */
	zap_record();                   /* clear the rest of the data */
	tstate = ESe;                   /* set the state and return */
	(void) strcpy(reason, "Remote record buffer size exceeded (trecord)");
	return(1);
}

/* put a single character into the recieved file */

put_c(c)
int     c;
{
	if(fnleft >= BLOCKSIZ){
		int	e;
		int	rc;
errno= -99;
		if((rc = (*writep)(f_fd,io_buff,BLOCKSIZ)) != BLOCKSIZ){
#ifdef	PP
			pp_rc = rc;
#endif	PP
			e = errno;
			L_WARN_2(L_ALWAYS, 0,
				"Disk write failed %d/%d (put_c)\n", rc, e);
			fnleft =0;
			errno = e;
			return(-1);
		}
		fnleft=0;
	}
	io_buff[fnleft++] = c;
	return(0);
}

/* flush out any characters left in internal buffers */

flush_f()
{
	register int    e;
	if(fnleft){
		int	rc;

		if((rc = (*writep)(f_fd,io_buff,fnleft)) != fnleft){
#ifdef	PP
			pp_rc = rc;
#endif	PP
			e = errno;
			L_WARN_2(L_ALWAYS, 0,
				"Disk write failed %d/%d (flush_f)\n", rc, e);
			fnleft=0;
			errno = e;
			return(-1);
		}
		fnleft = 0;
	}
	return(0);
}

/* recieve a file */

receive()
{
	if(tstate == GOs){
		lastmark = rec_mark;            /* set start conditions */
		lr_bcount = bcount = last_count;
		lr_reclen = reclen = last_rlen;
	}
	wordcount=0;
	while(tstate != FAIL && tstate != OKs)
		rec_record();

	zap_record();                   /* reset for commands */
}

/* receive one record. decodes formatting charcters */

/* This is set if the data sent can be safely stored without too many
 * problems
 */

#define SPEEDUP (max_rec_siz == INFINITY && datatype == 01 && (format & 0x80))

rec_record()
{
	register unsigned char   *p,*q;
	register int    i,c;
	register speedup = SPEEDUP;
	extern  char    last_cc;
	int	bad_code = 0;

	while(tstate != FAIL && tstate != OKs )
	{	if(tstate != DATA && tstate != PEND){   /* no data yet */
			c = r_rec_byte();               /* wait for some */
			continue;
		}

		if(rrecst)                     /* there is data here */
		{	if(!speedup)
			{	c = *rrecptr++ & MASK;
				if(!--rsreclen){       /* last of the data */
					rrecst = 0;
					last_cc = 1;
				}
				goto more_data; /* And process it */
			}
			else                  /* this is usually true */
			{	i = rsreclen;
				bcount += i;
				reclen += i;
				if(i + fnleft <= BLOCKSIZ){ /* usual case */
					p = &io_buff[fnleft];
					q = rrecptr;
					fnleft += i;    /* A block copy */
					rrecptr += i;
					do {
						*p++ = *q++;
					}while(--i);
				}
				else
				{   do     /* at end of buffer */
				    {	if(fnleft < BLOCKSIZ)   /* slower */
						io_buff[fnleft++]= *rrecptr++;
					else if(put_c(*rrecptr++)){
						L_LOG_1(L_GENERAL, 0,
								"put_c(%x)\n",
						      rrecptr[-1]);
						rsreclen = i;   /* no space */
						bcount -= i;  /* adjust the */
						reclen -= i;  /* pointers */
						goto baddata; /* do a RR */
					}
				    } while(--i);
				    stat_val(bcount);
				    stat_sync();
				}
				rsreclen = 0;
				rrecst = 0;
				last_cc = 1;
			}
		}
/*
 * get here if more data is needed; do this through a call to
 * r_rec_byte(). If got TCC_COMMAND then check the new state.
 */
		if( (c = r_rec_byte()) & TCC_COMMAND)
			continue;

		if(c&END_REC_BIT){       /* got to end of record */
			if(!code){
				if(format & 0x1D) {
					if(format & (01 | 04) ){
						if(fnleft < BLOCKSIZ)
							io_buff[fnleft++] =NL;
						else if(put_c(NL))
						{	L_LOG_0(L_GENERAL, 0,
								"put_c(NL)\n");
							goto baddata;
						}
					}
					else { /* format &0x8|0x10*/
						if(fnleft < BLOCKSIZ)
							io_buff[fnleft++] =NP;
						if(put_c(NP))
						{	L_LOG_0(L_GENERAL, 0,
								"put_c(NP)\n");
							goto baddata;
						}
					}
					bcount++;
				}
			}
			else if(code==1 && wordcount)
			{	L_LOG_1(L_GENERAL, 0, "code==1, wordcount=%d\n",
					wordcount);
				sprintf(reason,
					"Prot err (code==1, wordcount=%d)",
					wordcount);
				bad_code = 0x22;
				goto baddata;
			}
			reclen=0;
			return;
		}
/*
 * get here if got some data. Which needs some type of checking
 * i.e. If speedup is not set. Or if got a single byte when in speedup
 * mode.
 */

more_data:
		if(reclen > max_rec_siz)  /* copes with default as well */
		{	L_LOG_3(L_GENERAL, 0, "reclen too big (%d>%d), %02x\n",
				reclen, max_rec_siz, c);
			sprintf(reason, "reclen too big (%d>%d), %02x",
				reclen, max_rec_siz, c);
			bad_code = 0x22;
			goto baddata;           /* sent too long record */
		}
		if(code==1){            /* binary data */
			if(put_c(c))
			{	L_LOG_1(L_GENERAL, 0, "put_c(%x) failed\n", c);
				goto baddata;
			}
			bcount++;
			if(++wordcount >= wrd_bin_siz){
				wordcount =0;
				reclen++;
			}
			if ((bcount & 0x3ff) == 0) {
				stat_val(bcount);
				stat_sync();
			}
			continue;
		}
		if(!(format & 0x80))
			c &= ~0x80;                     /* make into IA5 */
		if(format & 02){
			if(!reclen)   /* ansi control codes */
				      /* but not at start of file */
				switch(c){
			case ' ':	if (!bcount) { reclen++; continue;}
					c = NL;
					break;
			case '0':	if (bcount)
					{	put_c(NL);
						bcount++;
					}
					c = NL;
					break;
			case '-':	if (bcount)
					{	put_c(NL);
						bcount++;
					}
					put_c(NL);
					bcount++;
					c = NL;
					break;
			case '+':	if (!bcount) { reclen++; continue;}
					c = CR;
					break;
			case '1':
					c = NP;
					break;
/* FIX ---------------------- Ignore it ???? ------------------ FIX */
			default:
					reclen++;
					continue;
				}
		}
/*
 * Ignore CR if followed by LF. Only do this if we know that there is no more
 * data in current sub-record. Or next char is really NL. It's the best I can
 * do.... I should really try to read the next char and monitor the record
 * length but what a lot of code...
 */
		else if(c == CR && (format & 0x7C)){
			if(!rrecst || (*rrecptr & 0x7F) == NL){
				reclen++;
				continue;
			}
		}
		if(fnleft < BLOCKSIZ)   /* formating actions should go here */
			io_buff[fnleft++] = c;
		else if(put_c(c))
		{	L_LOG_1(L_GENERAL, 0, "put_c(%x) failed\n", c);
			goto baddata;
		}
		else { stat_val(bcount+1); stat_sync(); }

		bcount++;
		reclen++;
	}                       /* end of while loop */
	return;

/*
 * come here if record's too long or words missing in record - data missing in
 * record should "never happen" since it is an error by me !!
 * or out of disk space. ( Should "never happen" ha !!!!! )
 */

baddata:
#ifdef	PP
	if (PPTRANS && !pp_spooled)
	{	pp_reject(reason);
		return;
	}
#endif	PP
#ifdef  SIGXFSZ
	if(errno == EFBIG){     /* we have got to our limits -- hard error */
		send_comm(QR, 0x21, 1); /* give up */
		tstate = QRe;
		(void) strcpy(reason, "file size limit exceeded");
		return;
	}
#endif
	if( !(facilities & RESTARTS) || ++nrestarts >= 10){
		/* the case if no restarts - abort the connection */
		/* send a failure here */
		send_comm(QR,(bad_code) ? bad_code : 0x20, 1);
		tstate = QRe;
		if (!bad_code) (void) sprintf(reason,
			"file write error ( out of space )%s",
			(!(facilities & RESTARTS)) ? " [no restarts]":
						     " [too many restarts]");
		return;
	}
	L_LOG_0(L_MAJOR_COM, 0, "Going to try to do a restart\n");
	send_comm(RR,(int)rec_mark&MASK, 1);
	last_count = bcount = lr_bcount;
	last_rlen = reclen = lr_reclen;
	lastmark = rec_mark;
	wordcount=0;
	L_LOG_1(L_10, 0, "RR bcount = %ld\n",bcount);
	(void) lseek(f_fd,last_count,0);
	fnleft=0;                       /* rewind counter */
	tstate = RRs;                   /* wait for it */
}
