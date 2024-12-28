#ifndef	lint			/* unix-niftp lib/pqproc/sft.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/sft.c,v 5.6 1991/06/07 17:02:04 pb Exp $";
#endif	lint

#include "ftp.h"
#include "infusr.h"
/*
 * file  sft.c
 * last changed 7-oct-83
 * $Log: sft.c,v $
 * Revision 5.6  1991/06/07  17:02:04  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:32  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:52:38  pb
 * checked in with -k by pb at 90.03.31.13.31.53.
 * 
 * Revision 5.2  89/01/13  14:52:38  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:24:42  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:33:56  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.3  87/12/09  16:34:19  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:42:28  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2.1.1  87/09/28  11:25:41  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/06/13  08:18:23  pb
 * Attribute logging centralised.
 * 
 * Revision 5.1  87/05/31  19:37:14  pb
 * With the explicit ATTRIB logging.
 * 
 * Revision 5.0  87/03/23  03:50:19  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 *
 * this file contains the routines to build and send then recieve and
 * decode the initial SFT exchange.
 *
 *
 * 8-sep-83  add code to deal with 77niftps or isid - all commented ISID
 */

extern  char    reason[];

#include "sfttab.h"     /* table from which the SFT is sent */

#ifdef  UCL_ISID
extern  int     p77flag;        /* ISID - set if talking 77  */
#endif
extern char *strcat ();

/*
 * table of pointers into the table structure to get the various bits
 * of information. The table structure is horrible.
 */

struct  param_vec {
	char    selattr;
	short   *selptr;
	} param_vec[] = {
	USERNAME,&tab.r_usr_n,
	USERPWD, &tab.r_usr_p,
	FILENAME,&tab.r_fil_n,
	FILEPWD, &tab.r_fil_p,
	ACCOUNT, &tab.account,
	ACCNTPWD,&tab.acc_pass,
	OUTDEVT, &tab.dev_type,
	DEVTQ,   &tab.dev_tqual,
	ACTMESS, &tab.op_mess,
	INFMESS, &tab.mon_mes,
	SPECOPT, &tab.specopts,
	HORIZTAB,&tab.specopts,
	0,0,
	};
/*
 * problems with strings..
 *   could use a table of char pointers which are indexed by the sft
 *   table entry. cannot use integers for pointers since won't work on vax's
 */

/*
 * this routine will construct an sft from it's constraints and from the
 * user parameters. This routine should be rewritten.
 */

build_sft()
{
	register i;
	register struct sftparams *p;
	register struct param_vec *s;
	register char    *ptr,**stptr;
	static  gotcrypt;
	char    *decrypt();

#ifdef  UCL_ISID                                /* ISID start */
	if(p77flag)
		sftp(PROTOCOL)->ovalue = 0x000F;
	else
		sftp(PROTOCOL)->ovalue = 0x0100;        /* default */
#endif                                          /* ISID end  */
	
	for(p=sfts;p->attribute != (char)0xFF ; p++){
		if(p->sflags & NOSUPPORT)       /* not supported attribute */
			continue;                       /* ignore */
		p->squalifier = p->qualifier;
		switch(p->qualifier & FORMAT){
	case INTEGER:                           /* integer format */
		switch(p->attribute) {		 /* a few special cases */
		case ACCESS:
			p->ivalue = tab.t_access;
			if(try_resuming)
				p->ivalue |= 0x0100;    /* resume bit */
			break;
#ifdef notneeded
		case DATAEST:	/* Why bother with dataest ?? ?*/
#endif
		case FILESIZE:
			if( !(tab.t_access & 0x8000 ) )
				p->ivalue = 1 + (bsize(realname) >> 10);
			else continue;  /* this had better work !! */
			break;
		case DATATYPE:
			if(tab.t_flags & BINARY_TRANSFER)
#ifdef notdef
*** THIS IS WRONG ****		p->ivalue = 0xE;        /* should be binary */
#endif
				p->ivalue = 0x2;        /* should be binary */
			else p->ivalue = 0x1;           /* is text */
			break;
		case BINWORD:				/* it's not default */
			if(!(tab.t_flags & BINARY_TRANSFER))
				continue;
			if(tab.bin_size && (tab.bin_size % 8) == 0)
				p->ivalue = tab.bin_size;
			else
				continue; /* but it would be sent wrongly */
			break;
		case TRANSID:
			p->ivalue = (val)transfer_id;
			break;
		case TEXTFORM:
			if(!(tab.t_flags & BINARY_TRANSFER))
				if(tab.t_flags & FILE_MODES)
					p->ivalue = p->ovalue & ~0x80;
				else
					p->ivalue = p->ovalue;
			break;
		case INITRESM:
			if (try_resuming) {
				get_start(p);  /* routine to set start mark */
				break;
			}
		case DELIMPRE:
			if (*(&tab.specopts))
			{	p->ivalue = p->ovalue & ~ 0x8000;
				break;
			}
		default:
			if(p->ovalue != p->dvalue)
				p->ivalue = p->ovalue;
		}
		if(p->ivalue == p->dvalue && !(p->sflags & MUSTSEND))
			continue;
		p->sflags |= TOSEND;
		break;

	case STRING:
		if(!(p->sflags & FROMSEL))      /* ignore any that are */
		    if (p->attribute != HORIZTAB)
			break;                  /* not user setable */
		p->sflags |= TOSEND;
		for(s = param_vec ; s->selattr ; s++)  /* get from selector */
			if(s->selattr == p->attribute)
				break;
		if(!*s->selptr ){                       /* no value given */
			if(s->selattr == INFMESS){
				ptr = version;
				i = strlen(ptr);
				goto spec;
			}
			if(!(p->sflags & MUSTSEND)){    /* must send ? */
				p->sflags &= ~TOSEND;
				break;                  /* why bother ?? */
			}
			ptr = 0;                /* force the value */
			p->squalifier = NOVALUE|ANY|MONFLAG;
		}
		else {                  /* what a horrible table structure */
			char *optr;
			ptr = ((char *)&tab) + *s->selptr;  /* get address */
#ifdef  UCL
			/* Using new password format ?? */
			if(s->selattr ==USERPWD && !(tab.t_flags&OLD_PASSWD)){
#else
			if(s->selattr == USERPWD){
#endif
				if(!gotcrypt){
					setcrypt();
					gotcrypt =1;
				}
				optr = ptr = decrypt(ptr);
				if (! optr)
				{	L_LOG_0(L_GENERAL, 0,
						"Failed to decrypt password\n");
					if(*reason) (void) strcat(reason,
							MSGSEPSTR);
					(void) strcat(reason,
					"Failed to decrypt password (locally)");
					break;
				}
				/* May get old cruddy "<uid>.<password>"
				 * or new fangled <password>
				 */
				for(i = 0; *ptr >= '0' && *ptr <= '9'; ptr++)
					i = i*10 + *ptr - '0';
				if (*ptr == '.')
				{	if (i == uid)
						ptr++;  /* get past '.' */
				}
				else ptr = optr;
				if (tab.t_flags & ENCODE_PW || trans_unixniftp)
				{	L_LOG_2(L_GENERAL, 0,
						"Flags=%x:%x, so encode\n",
						tab.t_flags, trans_unixniftp);
					priv_encode(ptr, ptr, 1);
				}
			}
			i = strlen(ptr);        /* get length */
		}
	spec:
		p->ivalue = p->ovalue;          /* now alloc space */
		stptr = &sftstrings[p->ivalue];
		if(*stptr!= NULL){
			free(*stptr);
			*stptr = NULL;
		}
		if(ptr)
		{	*stptr = malloc(i+1);           /* + null byte */
			(void) strcpy(*stptr, ptr);
		}
		break;
	default:        /* a never happen case */
#ifdef  DEBUG
		L_WARN_0(L_10, L_CONTINUE,
			"\n\n\nSFT build - table corrupted\n\n\n\n");
#endif
		break;
		}
	}
	send_qual(SFT);         /* built it now send it */
}

/*
 * send an SFT command it actually actually sends the
 * SFT (sft_build) then recieves and decodes the RPOS or RNEG
 */

send_sft()
{
	int     n;
	val     comm,qual;
	val     integ;
	char    *ptr,**sptr;
	struct  sftparams *p;
	char    t_buff[256];    /* strings can be any length up to 255 */
	int     reply;

	tstate  = SFTs;  /* state of transfer is SFT */
	build_sft();    /* send it as well */
	ok = 1;         /* set to zero if transfer unacceptable */
	reply = r_rec_byte();
	tstate = 0;
	switch(reply) {
	case RPOS:
		L_LOG_0(L_MAJOR_COM, 0, "RPOS\n");
		break;
	case RNEG:
		L_LOG_0(L_MAJOR_COM, 0, "RNEG\n");
		st_of_tran = REJECTED;
		(void) strcpy(whichhost, "remote");
		ok =0;
		break;
	default:
		L_WARN_1(L_GENERAL, 0, "Bad SFT return %02x\n",reply);
		prot_err(131, reply, reply);
	}
	n = r_rec_byte();
	if(n & END_REC_BIT)
		prot_err(132,n,n);
	L_LOG_2(L_ATTRIB, 0, "Got %d parameter%s\n", n, (n == 1) ? "" : "s");
	while(n--){
		comm = r_rec_byte();            /* command */
		if(comm & END_REC_BIT)
			prot_err(133, comm, n);
		qual = r_rec_byte();            /* qualifier */
		if(qual & END_REC_BIT)
			prot_err(134, comm, qual);
		dec_command(qual,t_buff,&integ);  /* get rest of command */
		if(ftp_print & L_ATTRIB)
		{	L_LOG_0(L_ATTRIB, 0, "received ");
			log_attr(comm, qual, integ, t_buff);
		}
		for(p = sfts ; p->attribute != (char)0xFF ; p++)
			if(p->attribute == (char)comm)
				break;
		if(p->attribute != (char)comm){      /* unknown attribute */
			if(ftp_print & L_ATTRIB)
			{	L_LOG_0(L_ATTRIB, 0, "ignore - ");
				log_attr(comm, qual, integ, t_buff);
			}
			continue;       /* maybe should set ok to 0 ?? */
		}
		switch(qual & FORMAT){
		case INTEGER:
			if(p->sflags & NOSUPPORT)  /* no support */
			{	L_WARN_2(L_GENERAL, 0,
			"attr 0x%02x - nosupport got integer value %04x\n",
					p->attribute,integ);
				continue;
			}
			if((p->qualifier & FORMAT) != INTEGER){
				if(p->attribute == (char)SPECOPT)
					continue;
				L_WARN_0(L_MAJOR_COM, 0, "Bad return on sft\n");
				ok = 0; ecode=ER_WRONG_ATTRIB_TYPE;
				continue;
			}

			/* Has it been negotiated alright ? */
			switch (p->squalifier & OPER) {
			case	EQ:
				if (p->ivalue != integ)
				{
					if (p->attribute == FILESIZE ||
					    p->attribute == DATAEST)	break;
					if (p->attribute == STOFTRAN &&
					    reply == RNEG)		break;
					if (p->attribute == PROTOCOL &&
					    (p->ivalue & 0xff00) ==
					    (integ & 0xff00))		break;
					if (p->attribute == ACCESS &&
					    ((p->ivalue == ACC_ROM &&
					      (integ == ACC_RO ||
					       integ == ACC_MO)) ||
					     (p->ivalue == ACC_AOM &&
					      (integ == ACC_AO ||
					       integ == ACC_MO))))	break;
					ok = 0;
					ecode=ER_INVALID_ATTRIB_NEGOTIATION;
					p->sflags |= TOSEND;
					p->squalifier =
						(p->squalifier & ~OPER) | NE;
					L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: %04x != %04x\n",
						p->attribute, p->ivalue, integ);
					continue;
				}					break;
			case	LE:
				if (p->sflags & BIT)
				{	if (integ & ~(p->ivalue))
					{	ok = 0;
						ecode=ER_INVALID_ATTRIB_NEGOTIATION;
						p->sflags |= TOSEND;
						L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: bit %04x > %04x\n",
						p->attribute, p->ivalue, integ);
						continue;
					}
				}
				else
				{	if (p->ivalue < integ)
					{	ok = 0; ecode=ER_INVALID_ATTRIB_NEGOTIATION;
						p->sflags |= TOSEND;
						L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: int %04x > %04x\n",
						p->attribute, p->ivalue, integ);
						continue;
					}
				}					break;
			case	NE:
				if (p->ivalue == integ)
				{	ok = 0; ecode=ER_INVALID_ATTRIB_NEGOTIATION;
						p->sflags |= TOSEND;
						L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: %04x = %04x\n",
						p->attribute, p->ivalue, integ);
					continue;
				}					break;
			case	GE:
				if (p->sflags & BIT)
				{	if (p->ivalue & ~integ)
					{	ok = 0; ecode=ER_INVALID_ATTRIB_NEGOTIATION;
						p->sflags |= TOSEND;
						L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: bit %04x < %04x\n",
						p->attribute, p->ivalue, integ);
						continue;
					}
				}
				else
				{	if (p->ivalue > integ)
					{	ok = 0; ecode=ER_INVALID_ATTRIB_NEGOTIATION;
						p->sflags |= TOSEND;
						L_WARN_3(L_MAJOR_COM, 0,
				"Bad value for attrib %04x: int %04x < %04x\n",
						p->attribute, p->ivalue, integ);
						continue;
					}
				}					break;
			}
			p->ivalue = integ;
			if(reply == RNEG){
				if( p->attribute == STOFTRAN)
					st_of_tran = p->ivalue;
				if(p->attribute == ACCESS && try_resuming){
				/* it may be resumption thats not liked*/
					tab.l_docket =0; /* start again*/
					rej_resume = 1; /* set flag*/
					may_resume = 0;
				}
			}
			break;
		case STRING:
			if(p->sflags & NOSUPPORT)
			{	L_WARN_2(L_GENERAL, 0,
		"attr 0x%02x - nosupport got return for string value %s\n",
					p->attribute,t_buff);
				continue;
			}
			
/*
 *  this works since all attributes that are supported have the
 *  correct format.
 */
			if((p->qualifier & FORMAT) != STRING){
				if(p->attribute == (char)SPECOPT)
					continue;
				L_WARN_0(L_MAJOR_COM, 0, "Bad return on sft\n");
				ok = 0; ecode=ER_WRONG_ATTRIB_TYPE;
				continue;
			}
			if(!integ)
				break;
			p->ivalue = p->ovalue;
			sptr = &sftstrings[p->ivalue];
/*
 * The problem with information and action messages is that there can
 * be more than one of each. deal with them here for ease of handling.
 */
			if(sptr == &infomsg || sptr == &actnmsg){
				if(sptr == &infomsg)    /* special case */
				{    L_LOG_0(L_GENERAL, 0, "Information");
				}
				else L_LOG_0(L_GENERAL, 0, "Action");
				L_LOG_1(L_GENERAL, L_CONTINUE, " message:- %s\n",t_buff);
				if(sptr == &infomsg &&
				   ((st_of_tran&REJECTED) || reply == RNEG)){
					if (*reason)
						(void) strcat(reason,MSGSEPSTR);
					(void) strcat(reason,t_buff);
				}
				break;
			}
			if(*sptr != NULL)
				free(*sptr);
			ptr = *sptr = malloc(integ+1); /* space for null */
			(void) strcpy(ptr,t_buff);
			break;
		}
/*
 * here we have a return of an sft which should have the following
 * bits set correctly.. If RPOS then can have EQ or ANY. On RNEG can have
 * any value....
 */

		if(reply==RPOS)
		    if( (qual & OPER)!= EQ && (qual&OPER)!= ANY)
		    {	L_WARN_0(L_GENERAL, 0,
				"Got return from sft with bad qualifier\n");
#ifdef  UCL_V77
			if( (qual & OPER)  == 0)   /* old style NOVALUE */
			{	L_WARN_0(L_GENERAL, 0, "Got a v77 system ????\n");
				continue;
			}
#endif
			ok=0; ecode=ER_WRONG_ATTRIB_TYPE_NOT_EQ_ANY;
			continue;
		    }
	}
	if(r_rec_byte()!=END_REC_BIT)   /* not all of record */
		prot_err(135, comm, qual);
	if(ok)
		set_values();                   /* set some values */
	if(reply == RPOS && !ok)                /* say we rejected it */
	{	st_of_tran = REJECTED | REJECTED_ATTRIB;
		(void) strcpy(whichhost, "local");
	}
}

/*
 * set the values of various variables as given by the SFT
 */

set_values()
{
	register struct sftparams *p;

#ifdef  UCL_ISID        /*ISID - if isid, set default ac. window to 2  */
	if(p77flag)
		acknowind = 2;
#endif                  /* ISID end  */

	for(p=sfts;p->attribute != (char)0xFF ; p++){
		switch(p->attribute){
		case TEXTCODE:          /* text code must be ascii */
			if( p->ivalue != 0x1 )
				p->sflags |= FAILURE;
			break;
		case TEXTFORM:  /* can have any 1 bit for text formatting */
#ifdef  UCL_ISID                        /* ISID start */
			if (p77flag)    /* make 77 value into 80 value */
				p->ivalue = 0x40; /* isid uses xb0 */
#endif                                  /* ISID end */
			if( nbits(p->ivalue) != 1 || p->ivalue > 0xFF)
				p->sflags |= FAILURE;
			format = p->ivalue;
			break;
		case BINFORM:           /* must have this value */
			if(p->ivalue != 0x8002)
				p->sflags |= FAILURE;
			break;
		case MTRS:
			max_rec_siz = p->ivalue;
			break;
		case ACKWIND:
			if(p->ivalue > 0xFF)
				p->sflags |= FAILURE;
			acknowind = p->ivalue;
			break;
		case INITRESM:          /* set the initial restart mark */
			if(p->ivalue){  /* got a value */
				if(!try_resuming)        /* not resuming */
					p->sflags |= FAILURE;   /* error */
				if(p->ivalue == INFINITY){ /* at eof */
					if(lastmark != INFINITY) /*not there*/
						p->sflags |= FAILURE;
				}
				else if(!(f_access & 0x8000) ){ /* transmit */
					if(p->ivalue < rec_mark ||
							p->ivalue > lastmark)
						p->sflags |= FAILURE;
					last_rlen = docp[p->ivalue & MASK];
					/* values could be out of range */
				}
				else {          /* receive - unknown mark */
					if(p->ivalue != rec_mark &&
							p->ivalue != lastmark)
						p->sflags |= FAILURE;
					if(p->ivalue == rec_mark)
						last_rlen = lr_reclen;
				}
			}
			if( ! (p->sflags & FAILURE) && p->ivalue != INFINITY){
				rec_mark = p->ivalue; /* if ok set position */
				last_count = p->ivalue;
				last_count <<= LSHIFT;
				(void) lseek(f_fd, last_count, 0);
			}
			break;
		case MINTIME:
			time_out = p->ivalue;
			break;
		case FACIL:
			facilities = p->ivalue;         /* facilities */
			if(facilities & 0x2)           /* should be MRESUME */
				may_resume = 1;
			else
				may_resume = 0;
			break;
		case DATATYPE:
			if(p->ivalue > 0xF)
				p->sflags |= FAILURE;
			datatype = p->ivalue;
			break;
		case BINWORD:
			/*
			 * binary word size must be a multiple of 8 bits
			 * less than 1024 bits and be a factor of 2048
			 */
			if(p->ivalue < 0x8 || p->ivalue > 128 * 0x8 ||
			  (p->ivalue & 07) != 0 || (MARK_MASK+1) % p->ivalue)
				p->sflags |= FAILURE;
			bin_size = p->ivalue >>3;
			break;
		}
		if(p->sflags & FAILURE) /* got a failure for this attr */
		{	L_WARN_1(L_GENERAL, 0,
				"Failure on attribute 0x%02x\n",p->attribute);
			/* say transfer is no good */
			ok=0; ecode=ER_UNIMPL_ATTRIB_VAL;
		}
	}
}

/*
 * routine to set the start position in a resumed transfer
 * This is not fully tested so the diagnostics are left in.
 */

get_start(p)
register struct sftparams *p;
{
	long    size,tsize,bsize();

	p->sflags |= TOSEND;
	if(lastmark == INFINITY)               /* resume at eof */
	{	L_LOG_0(L_GENERAL, 0, "Resuming at end of file\n");
		p->ivalue = lastmark;
		return;
	}
	if( !(f_access & 0x8000) )
	{	L_LOG_2(L_GENERAL, 0, "Resuming a transmission at %d (%d)\n",
				lastmark, rec_mark);
		size = bsize(realname);         /* get size of file */
		tsize = /*last*/rec_mark;
		tsize <<= LSHIFT;
		if(tsize > size)       /* someone has changed it */
		{	L_WARN_0(L_GENERAL, 0, "file has changed since last time\n");
			error = REJECTSTATE;  /* what to do ?? */
			may_resume = 0;
			longjmp(rcall, 0);
		}
		p->ivalue = /*last*/rec_mark;
		return;
	}
	L_LOG_2(L_GENERAL, 0, "resuming a reception at %d (%d)\n",
			lastmark, rec_mark);
	size = bsize(localname);
	tsize = /*last*/rec_mark;
	tsize <<= LSHIFT;
	if(tsize > size)
	{	L_WARN_0(L_GENERAL, 0, "file has changed since last time\n");
		error = REQSTATE;
		may_resume = 0;
		longjmp(rcall, 0);
	}
	p->ivalue = /*last*/rec_mark;
	lastmark = rec_mark = p->ivalue;
}
