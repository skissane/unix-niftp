#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/rsft.c,v 5.6.1.10 1993/05/17 13:18:35 pb Exp $";
#endif	lint
#define	SENDACCESS	TOSEND
/*
 * $Log: rsft.c,v $
 * Revision 5.6.1.10  1993/05/17  13:18:35  pb
 * Distribution of May1793FixVMSStreamCRLF: Re-do Format negotiation to avoid 0x80 VMS xfers with CRLF
 *
 * Revision 5.6.1.9  1993/05/13  06:53:39  pb
 * Distribution of May93FixSetgroups: Ensure setgroups and setuid are more likley to work
 *
 * Revision 5.6.1.7  1993/05/10  13:58:14  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC: Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.6  1993/01/10  07:11:31  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:01:22  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  14:01:06  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.5  90/03/31  13:41:04  pb
 * Initial patcehs for PP
 * 
 * Revision 5.4  89/08/26  13:42:13  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.3  89/07/16  12:03:44  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:51:13  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:22:42  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.5  88/01/29  07:40:39  pb
 * Distribution of Jan88ReleaseMod1: JRP fixes - tcccomm.c ftp.c + news sucking rsft.c + makefiles
 * 
 * Revision 5.0.1.4  88/01/28  06:12:58  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:33:49  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:24:21  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:07:21  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/04/12  09:36:32  pb
 * Add NFS fixes (seteuid)
 * 
 * Revision 5.0  87/03/23  03:50:00  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#include "files.h"
#include "sfttab.h"
#include "nrs.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>      /* for ETXTBSY */
#define	L_SUCK_INFO	0x80000
#define	L_SUCK_DEB	0x010000
#ifdef	FREEDISK
#include <sys/param.h>
#if	defined(sun) || defined(pyr)
#include <ufs/fs.h>	/* To look at the Superblock */
#else	/* sun || pyr */
# ifdef	_42
#include <sys/fs.h>	/* To look at the Superblock */
# else	_42
#include <sys/filsys.h>	/* To look at the Superblock */
# endif	_42
#endif	sun
#endif	FREEDISK


/*
 * NGROUPS should be found in <sys/param.h> but it will not compile with this
 * in. Yeuch !!!!!
 * #include <sys/param.h>
 */

#include <sys/param.h>

#ifdef  NGROUPS
extern    int     lgroups[];
extern    int     ngroups;
extern    int     gotgroups;
#endif

struct  passwd  *getpwuid();
struct	passwd	*find_user_id();
struct  passwd  *getpwnam();
char	*rindex();
extern	int open(), read(), write(), close();
extern	int (*openp)(), (*readp)(), (*writep)(), (*closep)();
#ifdef	PP
#define GB_NOTOK        NOTOK
#define GB_OK           OK
#define GB_RECIPFAILED  -2
#define GB_SENDERFAILED -3
extern	int pp_open(), pp_read(), pp_write(), pp_close();
char	*pp_errstring;
int	pp_rc;
#endif	/* PP */
extern  char    reason[];
extern  char    lnetchar;
#ifdef	MAIL
extern	char	xerr;
#endif	MAIL
extern	qfd;
#ifdef	PP
int pp_spooled;
#endif	PP

/*
 * set from tailor file, used when always rejecting transfers
 */
char    lreject, *linfomsg;
extern  char	lcreject;
extern  char	lreverse;
extern  char	unknown_host;
extern  char	why_unknown_host[];
extern	char    ltranstype;

/* receive sft and decode. send RPOS or RNEG as required */

get_sft()
{
	register c;
	register n;
	register struct sftparams *p;
	int	is_ftp = 1;
	val     qual,comm,integ;
	char    t_buff[256],**sptr;
	long    l;
#ifdef  MAIL
	extern  char    nilogin[];

#ifdef	PP
	readp = read;
	writep = write;
	closep = close;

	if (PPTRANS)
	{	pp_spooled = strncmp(PPproc, "inline", 6);
		if (!pp_spooled)
		{	/* readp = pp_read; */
			writep = pp_write;
			closep = pp_close;
			/* No resumption/restarts */
			for(p=sfts;p->attribute != (char)0xFF ; p++)
				if(p->attribute == (char) FACIL)
			{	p->ovalue &= ~ (LRESUME | RESTARTS);
				break;
			}

		}
	}
#endif	PP
#endif	MAIL

	ok=1;
	/*
	 * initialise the squalifiers. for the return.
	 */
	for(p = sfts; p->attribute != (char) 0xFF ; p++)
		p->squalifier = (p->qualifier&FORMAT)|EQ;
	c = r_rec_byte();
	if(c != SFT){
		L_WARN_0(L_MAJOR_COM, 0, "No SFT\n");
		prot_err(126, c, c);
	}
	c = r_rec_byte();       /* get number of attributes */
	if(c & END_REC_BIT)
		prot_err(127, c, c);
	n = c;
	while(n--){
		comm = r_rec_byte();            /* get attribute */
		if(comm & END_REC_BIT)
			prot_err(128, c, comm);
		qual = r_rec_byte();            /* get qualifer */
		if(qual & END_REC_BIT)
			prot_err(129, comm, qual);
		dec_command(qual,t_buff,&integ); /* get rest of it */
		if(ftp_print & L_ATTRIB)
		{	L_LOG_0(L_ATTRIB, 0, "received ");
			log_attr(comm, qual, integ, t_buff);
		}
		for(p=sfts;p->attribute != (char)0xFF ; p++)
			if(p->attribute == (char)comm)
				break;          /* search for attribute */
		if(p->attribute != (char)comm){   /* not found in table */
			L_WARN_1(L_GENERAL, 0, 
				"sft unknown protocol attribute %x02\n",comm);
			ok =0;
			/* should send attribute unknown but where to
			   put in the table ??
			 */
			ecode = ER_UNKNOWN_ATTRIB;
			continue;
		}
		if(p->sflags & NOSUPPORT){      /* this attribute is */
			p->sflags |= TOSEND;    /* not supported */
			p->squalifier = NOVALUE|ANY;
			L_WARN_1(L_MAJOR_COM, 0, 
				"Got value for a non supported attr %02x\n",
				p->attribute);
		}
		if(qual & MONFLAG){     /* wants monitoring */
			p->sflags |= TOSEND;
			if(p->attribute==USERPWD || p->attribute==FILEPWD ||
			      p->attribute==ACCNTPWD || p->attribute==DEVTQ)
				p->squalifier = NOVALUE|ANY;
		}                       /* stop monitoring of passwords */
		switch(qual & FORMAT){
		case STRING:            /* string attribute value */
			if((p->qualifier &FORMAT) != STRING ){
				if(p->attribute == (char)SPECOPT)
					continue;
				L_WARN_1(L_MAJOR_COM, 0, 
					"Bad str format qualifier %04x\n",
					p->qualifier);
				ok=0;
				ecode = ER_BAD_FORMAT_ATTRIB;
				continue;
			}
			if(!integ)              /* null strings */
				break;
			p->ivalue = p->ovalue;  /* put into table */
			sptr = &sftstrings[p->ovalue];
			if(*sptr)
				free(*sptr);
			*sptr = malloc(integ+1);
			(void) strcpy(*sptr, t_buff);
			break;
		case INTEGER:           /* integer value */
			if((p->qualifier &FORMAT) != INTEGER){
				if(p->attribute == (char)SPECOPT)
					continue;
				L_WARN_1(L_MAJOR_COM, 0, 
					"Bad int format qualifier %04x\n",
					p->qualifier);
				ok=0;
				ecode = ER_BAD_FORMAT_ATTRIB;
				continue;
			}
			p->ivalue = integ;
			break;
		default:
			p->ivalue = p->dvalue;
			break;
		}
		if( (qual & FORMAT) != INTEGER)  /* don't check these */
			continue;
		if(p->sflags & NOSUPPORT)       /* don't check these */
			continue;
		switch(p->attribute){
		case ACCESS:            /* don't check these attributes */
		case PROTOCOL:
		case DATAEST:
		case TRANSID:
		case INITRESM:
		case MSTRECS:
		case FILESIZE:
		case DELIMPRE:
			continue;
		/*
		 * check to see these attributes.
		 * should also pick up bad attribute qualifiers
		 */
		default:
			if( (p->qualifier  & OPER) != ANY)
				check(qual,p);
			break;
		}
	}
	if(r_rec_byte()!=END_REC_BIT)
		prot_err(130, comm, qual);
#ifdef  MAIL
	mailer =0;
#endif
	set_values();           /* set all neccasary values from sfttab */

	if(ok && unknown_host && lreverse){
		if (*reason) (void) strcat(reason, MSGSEPSTR);
		sprintf(reason + strlen(reason), m_r_text, why_unknown_host);
		L_ERROR_2(L_ALWAYS, L_TIME,
			"!!!!%s (%d)\n", why_unknown_host, unknown_host);
		ok = 0;
	}

	if (ok && !cur_user) ok = set_user(&is_ftp);

	if(ok && try_resuming){		/* if we want to try to resume */
		switch(oldqdocket()){    /* try to find docket */
		case -2:                /* can't..... */
			L_ERROR_0(L_ALWAYS, L_TIME,
				"Fatal unix error can't continue\n");
			stat_close((char *) 0);
			exit(99);
			/*NOTREACHED*/
		case -1:                /* no docket found */
			L_WARN_0(L_GENERAL, 0, "Can't find old docket\n");
			sftp(TRANSID)->sflags |= FAILURE;
			ok=0;
			ecode = ER_NO_DOCKET;
			break;
		default:                /* got it */
			L_LOG_0(L_GENERAL, 0, "Resuming with a good docket\n");
			setrestart(is_ftp);
			break;
		}
	}
	else
	if(ok && JTMPTRANS) /* jtmp */
	{
#ifdef JTMP
		static  char    xpid;
		L_LOG_1(L_GENERAL, 0, "we have a jtmp %s request\n",
			(f_access & ACC_GET) ? "transmit" : "receive");
		if (!(f_access & ACC_GET))
		{	if(!xpid)
				xpid = (getpid() % 26 ) + 'a';
			(void) time((int *)&l);
			sprintf(t_buff, "%s/QB%s%c", JTMPdir, ltoa(l), xpid);
			filename = malloc(strlen(t_buff)+1);
			(void) strcpy(filename, t_buff);
		}
		if(ok && !getfile())		/* access the file */
			ok=0;
		if(!ok && !ecode)               /* failure. */
			ecode = ER_FILE_ACCESS;
#else	JTMP
		L_WARN_0(L_ALWAYS, 0, "cant handle jtmp\n");
		ok = 0;
		ecode = ER_UNKNOWN_CONTEXT;
#endif	JTMP
	}
	else if(ok && MAILSTRANS)
	{
#ifdef  MAIL                                    /* a mail transfer */
		static  char    xpid;
		char *comment	= "";
#ifdef	PP
		if (PPTRANS)
		{	mailer++;
			comment = (pp_spooled) ? " (pp)" : " (PP)";
			if (!pp_spooled) mailer++;
		}
#endif	PP
		L_LOG_2(L_GENERAL, 0, "we have mail%s %s request\n",
			comment,
			(f_access & ACC_GET) ? "transmit" : "receive");
		mailer++;               /* set up default values */
		if (!(f_access & ACC_GET))
		{
#ifdef	PP
L_LOG_2(L_10, 0, "PP with PPTRANS=%d and pp_spooled=%d\n", PPTRANS, pp_spooled);
		    if (PPTRANS && !pp_spooled)
		    {	int fd;
			char channel[128];
			sprintf(channel, PPchan,
			   (argstring) ? argstring : "niftp", lnetchar);
			L_LOG_4(L_GENERAL, 0,
				"call pp_open(%s, %s) [given %s, %s]\n",
				channel, hostname,
				PPchan, (argstring) ? argstring
						    : "<no channel>");
			if (pp_errstring)
				L_LOG_1(L_ALWAYS, 0, "pp_errstring is %x\n",
					pp_errstring);
			fd = pp_open(channel, hostname, &pp_errstring);
			if (fd < 0)
			{	ok = 0;
				ecode = ER_MAIL_START;
				L_WARN_4(L_ALWAYS, 0,
					"pp_open(%s, %s) gave %d (%x)\n",
					channel, hostname, fd, pp_errstring);
				pp_rc = fd;
				if (*reason) (void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, (pp_errstring) ?
					pp_errstring :
					"Should have more info !!");
			}
			strcpy(realname, "<PP>");
			if (filename = malloc(strlen(realname)+1))
				(void) strcpy(filename, realname);
			f_fd = -1; /* don't try to close !! */
			strcpy(localname, realname);
		    }
		    else
#endif	PP
		    {	if(!xpid) xpid = (getpid() % 26 ) + 'a';
			(void) time((int *)&l);
			sprintf(t_buff, "%s/%s%c",
#ifdef	PP
				PPTRANS ? PPdir :
#endif	PP
				MAILDIR, ltoa(l), xpid);
			filename = malloc(strlen(t_buff)+1);
			(void) strcpy(filename, t_buff);
		    }
		}
#ifdef	PP
		if (!PPTRANS || pp_spooled)
#endif	PP
		if(ok && !getfile())	/* access the file */
					ok=0;
		if(!ok && !ecode)               /* failure. */
			ecode = ER_FILE_ACCESS;
#else
		L_WARN_0(L_ALWAYS, 0, "cant handle mail\n");
		ok = 0;
		ecode = ER_UNKNOWN_CONTEXT;
#endif	MAIL
	}
	else if(ok && NEWSTRANS)
	{
#ifdef NEWS
		static  char    xpid;
		L_LOG_1(L_GENERAL, 0, "we have news %s request\n",
			(f_access & ACC_GET) ? "transmit" : "receive");
		if (!(f_access & ACC_GET))
		{	if(!xpid)
				xpid = (getpid() % 26 ) + 'a';
			(void) time((int *)&l);
			sprintf(t_buff, "%s/%s%c", NEWSdir, ltoa(l), xpid);
			filename = malloc(strlen(t_buff)+1);
			(void) strcpy(filename, t_buff);
		}
		if(ok && !getfile())		/* access the file */
			ok=0;
		if(!ok && !ecode)               /* failure. */
			ecode = ER_FILE_ACCESS;
#else	NEWS
		L_WARN_0(L_ALWAYS, 0, "cant handle news\n");
		ok = 0;
		ecode = ER_UNKNOWN_CONTEXT;
#endif	NEWS
	}
	else if(ok && !is_ftp)
	{	L_WARN_0(L_ALWAYS, 0, "It appears not to be an FTP!\n");
		ok = 0;
		ecode = ER_UNKNOWN_CONTEXT;
	}
	else if (ok)
	{	if(f_access == ACC_TJO || f_access == ACC_TJI)
		{	L_LOG_1(L_GENERAL, 0, "we have %s\n",
				(f_access == ACC_TJO) ? "job output" :
							"an incoming job");
			/* tji, tjo special fixes */
			if(filename)            /* ignore the current file*/
				free(filename);
			(void) time((int *)&l);
			if(f_access == ACC_TJO) /* if going to line printer */
				sprintf(t_buff,"%s/lpdq.%s",
							LOCALLPDIR,ltoa(l));
			else	sprintf(t_buff,"%s/jobsin.%s",
							LOCALJOBDIR,ltoa(l));
			filename = malloc(strlen(t_buff)+1);
			(void) strcpy(filename, t_buff);
		}
		if(chkpasswd() < 1 ){      /* check password */
			L_LOG_1(L_ALWAYS, 0, "FAIL on pw (%d)\n", ecode);
			ok=0;                           /* then access file */
			if(!ecode)
				ecode = ER_FILE_ACCESS;
		}
		if(!getfile()){      /* check password */
			L_LOG_1(L_ALWAYS, 0, "FAIL on getfile (%d)\n", ecode);
			ok=0;                           /* then access file */
			if(!ecode)
				ecode = ER_FILE_ACCESS;
		}
	}

	if(ok){
		if(lreject)
			ok = 0;
		if(linfomsg != NULL)
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, linfomsg);
		}
		else if(lreject)
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "forced reject by system");
		}
		if(lreject) L_WARN_1(L_GENERAL, 0, 
			"reject cos told to by operator-%s\n", reason);
	}

#ifdef	FREEDISK
	if (ok && !(f_access & ACC_GET) && nofreedisk(filename, ltranstype))
	{	ok = 0;
		unlink(localname);
		ecode = ER_DISK_FULL;
	}
#endif	FREEDISK

	if(ok){                         /* ok so can send RPOS */
		/* if sending a file then set the filesize attribute */
		if( (f_access & ACC_GET) && filename)
		{	sftp(DATAEST)->ivalue =
			sftp(FILESIZE)->ivalue = ( bsize(filename) >> 10) + 1;
			sftp(FILESIZE)->sflags |= TOSEND;
		}
		/* Lets be chatty .... */
		if (!(sftp(INFMESS)->sflags & TOSEND))
		{	if (infomsg)	free(infomsg);
			infomsg = malloc(strlen(version)+1);
			strcpy(infomsg, version);
			sftp(INFMESS)->sflags |= TOSEND;
		}
		L_LOG_0(L_MAJOR_COM, 0, "RPOS\n");
		send_qual(RPOS);
		/* want to try to resume next time but not trying now */
		if(may_resume && !try_resuming && newqdocket() <0){
			L_WARN_0(L_ALWAYS, L_TIME,
				"Cannot create dockets -- give up!\n");
			stat_close((char *) 0);
			con_close();
			exit(99);   /* unknown error */
		}
	}
	else {                                  /* reject the transfer */
		st_of_tran = REJECTED;
#ifdef	PP
		if (pp_rc == GB_RECIPFAILED || pp_rc == GB_SENDERFAILED)
			st_of_tran |= REJECTED_POSS;

#endif	PP
		L_LOG_1(L_MAJOR_COM, 0, "RNEG %04x\n", st_of_tran);
		buildrneg();
		send_qual(RNEG);
		if(*reason)             /* print out local reason */
			L_WARN_2(L_ALWAYS, 0, 
				"(%d) Failure Reason:- %s\n",uid, reason);
		*reason = 0;
		ecode =0;
	}
	try_resuming = 0;
}

/*
 * check the values of the given attribute against the value that we have
 * we only have need this cos we are great and we got no requirements
 */

check(qual,p)
register struct sftparams *p;
val     qual;
{
	register val temp = p->ivalue;
	switch(qual & OPER){
	case LE:
		if(p->sflags & BIT)     /* logical value */
		{	temp = p->ivalue & p->ovalue;
			if (p->attribute == BINFORM)
			{	/* Ensure we have (at least) one bit set in each half */
				if ((temp & 0xc000) == 0) temp |= (p->ivalue & 0xc000);
				if ((temp & 0x0007) == 0) temp |= (p->ivalue & 0x0001);
				if ((temp & 0x0007) == 0) temp |= (p->ivalue & 0x0002);
				if ((temp & 0x0007) == 0) temp |= (p->ivalue & 0x0004);
			}
		}
		else temp = p->ivalue < p->ovalue ? p->ivalue : p->ovalue;
		if(!temp) L_WARN_1(L_GENERAL, 0, 
	"Warning- Got a zero for LE operator Attribute %04x\n",p->attribute);
		break;
	case EQ:
	case GE:
		if(p->sflags&BIT){      /* a logical value */
			if( (p->ivalue & p->ovalue)!=p->ivalue){
				p->sflags |= FAILURE;
				break;
			}
		}
		else if(p->ivalue > p->ovalue){
			p->sflags |= FAILURE;
			break;
		}
		if( (qual&07)==GE)
			temp = p->ovalue;
		break;
	case ANY:
		temp = p->ovalue;
		break;
	}
	if(p->sflags & FAILURE){  /* we got a failure */
		L_WARN_4(L_MAJOR_COM, 0, 
			"Failure on sft is at %02x - value is %04x wanted %04x qualifier %02x\n",
			p->attribute,p->ovalue,p->ivalue,qual);
		ok =0;
		ecode = ER_BAD_ATTRIB_VAL;
	}
	if(temp != p->ivalue){          /* been modified. send back */
		p->ivalue = temp;
		p->sflags |= TOSEND;
	}
}

/* rebuild RNEG */

buildrneg()
{
	register struct sftparams *p;

	for(p = sfts; p->attribute != (char)0xFF ; p++){
		p->sflags &= ~TOSEND;
		p->squalifier=(p->qualifier & FORMAT)|EQ; /*say what we want*/
		if(p->sflags & FAILURE){    /* send any failed values */
			p->sflags |= TOSEND;
			p->squalifier = (p->qualifier & FORMAT) | NE;
		}
		else if(p->attribute == STOFTRAN){ /* send STATEofTRANSFER */
			p->sflags |= TOSEND;
			switch (ecode)
			{
			case ER_UNIMPL_ATTRIB:
			case ER_BAD_ATTRIB_VAL:
			case ER_USERNAME_GIVEN:
			case ER_FILENAME_GIVEN:
			case ER_USERFILE_GIVEN:
			case ER_UNKNOWN_CONTEXT:
				st_of_tran |= REJECTED_ATTRIB;	break;
			case ER_CANT_RESUME:
			case ER_NO_DOCKET:
				st_of_tran |= REJECTED_RESUME;	break;
			case ER_DISK_FULL:
				st_of_tran |= REJECTED_POSS;	break;
#if defined(ALL_NEWS_SOFT) || defined(ALL_MAIL_SOFT)
			default:
#ifdef	ALL_NEWS_SOFT
				if (NEWSTRANS && !(f_access & ACC_GET))
					st_of_tran |= REJECTED_POSS;
#endif	ALL_NEWS_SOFT
#ifdef	MAIL
#ifdef	ALL_MAIL_SOFT
				if (MAILTRANS && !(f_access & ACC_GET) && xerr == 0)
					st_of_tran |= REJECTED_POSS;
#endif	ALL_MAIL_SOFT
#ifdef	ALL_PP_SOFT
				if (PPTRANS && !(f_access & ACC_GET) && xerr == 0)
					st_of_tran |= REJECTED_POSS;
#endif	ALL_PP_SOFT
#endif	MAIL
								break;
#endif
			}
			if((st_of_tran & 0xF) == 0)
				st_of_tran |= REJECTED_MSG;
			p->ivalue = st_of_tran;
		}
		else if(p->attribute == INFMESS){ /* send back a message */
			if(infomsg)
			{	L_DEBUG_1(L_GENERAL, 0, "(Free '%s')\n", infomsg);
				free(infomsg);
			}
			infomsg =0;
			if(ecode){
				if(*reason)
					(void) strcat(reason,MSGSEPSTR);
				(void) strcat(reason, ermsg[ecode-1]);
			}
			if(*reason){
				infomsg = malloc(strlen(reason) + 1);
				(void) strcpy(infomsg, reason);
			}
			if(infomsg)
				p->sflags |= TOSEND;
		}
	}
}

/*
 * Routine to take values from the SFttable and
 * put them into the required values. Also process non standard
 * attributes.
 */

set_values()
{
	register val    i;
	register struct sftparams *p;
	for(p = sfts ; p->attribute != (char)0xFF ; p++){
		switch(p->attribute & MASK){
		case PROTOCOL:          /* protocol id */
			if( (p->ivalue & 0xFF00) != 0x0100){
				p->sflags |= FAILURE;
				ecode = ER_NOT_NIFTP80;
			}
			break;
		case ACCESS:            /* access modes */
			if(p->ivalue & ACC_RES)       /* tring to resume */
				try_resuming = 1;
			else try_resuming = 0;
			p->ivalue &= ~ACC_RES;
			if(p->ivalue == ACC_GJO || p->ivalue == ACC_DR){
				p->sflags |= FAILURE;   /* these are */
				ecode = ER_UNIMPL_ACCESS; /* not supported */
			}
			f_access = p->ivalue;
			break;
		case TEXTCODE:
			if(p->ivalue != 0x1)
				p->sflags |= FAILURE;
			break;
		case TEXTFORM:          /* text formatting. most only have */
			i =nbits(p->ivalue);    /* one bit set */
			if(!i || p->ivalue & 0xFF00)
				p->sflags |= FAILURE;
			else if(i>1){
				i = 0x0080;
				/* 80 means "no formating at all".
				 * This causes problems with some systems
				 * (e.g. VMS) so only select if remote end is a
				 * stream based system, e.g. un*x.
				 * Well, unix-niftp offers 0xff, so that is the
				 * best heuristic so far ...
				 */
				if(p->ivalue != 0x00FF && 
				    (p->ivalue & 0x80) &&
				    (p->ivalue & ~ 0x80))
					p->ivalue &= ~0x80;

#if	defined(NOANSITEXT) || defined(NOANSITEXT_MASK)
	/* Not all implementations (e.g. Rainbow/PC) get "ANSI CC"s right.
	 * As such, if given the choice then avoid the "02".
	 * The current *requirement* is actually "if (03) then use 01".
	 * A more general fix is "if (02 and also something in xx) ignore 02"
	 * Pro tem, set xx to 01 or fd ?	T.D.Lee@durham / pb@cl.cam 93/4
	 */
#ifndef	NOANSITEXT_MASK
#define	NOANSITEXT_MASK	0xfd
#endif
				if ((p->ivalue & 0x02) &&
				    (p->ivalue & (NOANSITEXT_MASK & ~ 0x02)))
					p->ivalue &= ~0x02;
#endif /* NOANSITEXT */
				while(! (p->ivalue&i))  /* and why not */
					i>>=1; /* I would prefer 0x80 first */
				p->ivalue = i;
				p->sflags |= TOSEND;
			}
			format = p->ivalue;
			break;
		case BINFORM:           /* binary formatting */
			/* Some bits must be zero */
			if(p->ivalue & 0x3FFC ) p->sflags |= FAILURE;

			/* If word size is 8, then the rest is irrelevant */
			else if(sftp(BINWORD)->ivalue == 0x8) ;

			/* Word size is multiple of 8; do we have the right order ? */
			else if ((p->ivalue & 0x8000) == 0x8000) ;

			else {	p->sflags |= FAILURE;
				ecode = ER_WORD_FORMAT;
			}
			break;
		case MTRS:              /* maximum transfer record size */
			if(!p->ivalue)
				p->sflags |= FAILURE;
			max_rec_siz = p->ivalue;
			break;
		case ACKWIND:                   /* acknowledgement window */
			if(p->ivalue > 0xFF || !p->ivalue)
				p->sflags |= FAILURE;
			acknowind = p->ivalue;
			break;
		case INITRESM:                  /* initial restart mark */
			if(p->ivalue && !try_resuming){ /* must be zero if */
				p->sflags |= FAILURE;   /* not resumeing */
				ecode = ER_UNIMPL_RESTART_MARK;
			}
			break;
		case MINTIME:
			time_out = p->ivalue;
			break;
		case FACIL:                     /* facilities */
			facilities = p->ivalue;
			if(facilities & 0x2)            /* possible to */
				may_resume = 1;         /* resume */
			else may_resume = 0;
			break;
		case TRANSID:                           /* transfer id */
			transfer_id = p->ivalue;
			break;
		case DATATYPE:          /* binary or ascii */
			if( p->ivalue == 0x3){
				p->sflags |= TOSEND;
				p->ivalue = 0x1;
			}
			if(p->ivalue >0xF)
				p->sflags |= FAILURE;
			datatype = p->ivalue;
			break;
		case DELIMPRE:
			if(p->ivalue & 0x7FFE )
				p->sflags |= FAILURE;
			break;
		case BINWORD:   /* horrible */
			if((p->ivalue&07)||p->ivalue<0x8 || p->ivalue >128*8
						|| (MARK_MASK+1) % p->ivalue){
				p->sflags |= FAILURE;
				ecode = ER_UNIMPL_BIN_WORD_SIZE;
			}
			bin_size = p->ivalue >>3;
			break;
		case INFMESS:
			if(infomsg){
				L_LOG_1(L_ALWAYS, 0, 
					"Information message:- %s\n",infomsg);
				free(infomsg);
				infomsg = NULL;
			}
			break;
		case  ACTMESS:
			if(actnmsg){
				L_LOG_1(L_ALWAYS, 0, 
					"Action message:- %s\n",actnmsg);
				free(actnmsg);
				actnmsg = NULL;
			}
			break;
		case FILENAME:
			if(filename) L_LOG_1(L_GENERAL, 0, 
					"Filename %s\n",filename);
			break;
		case USERNAME:
			if(username) L_LOG_1(L_GENERAL, 0, 
					"Username %s\n",username);
			break;
		case OUTDEVT:   /* check for output device. must be lp */
			if(f_access != ACC_TJO)
				break;
			if(chouttyp(outdev) == 0)
				p->sflags |= FAILURE;
			break;
		}
		if(p->sflags & FAILURE){  /* failed misserably on this */
			ok =0;                  /* attribute */
			if(!ecode)
				ecode = ER_BAD_ATTRIB_VAL;
		}
	}
}

/*
 * routine to open a localfile. checking permissions and access
 * attributes.
 */

getfile()
{
#ifdef	SETEUID
	int stat;
	int oldeuid = geteuid();
	int oldegid = getegid();

#ifdef	NGROUPS
	int oldg[NGROUPS];
	int oldgrps = getgroups(sizeof oldg / sizeof oldg[0], oldg);
	if (!gotgroups) get_groups(gid);
	if (oldgrps == -1)
		L_LOG_2(L_ALWAYS, 0, "getgroups gave %d (%d)\n",oldgrps,errno);
	if (oldgrps >= 0) (void) setgroups(ngroups, lgroups);
	L_LOG_4(L_10, 0, "setgroups %d: %d %d %d ... \n", ngroups,
		lgroups[0], lgroups[1],lgroups[2]);
#endif	NGRROUPS
	L_LOG_4(L_10, 0, "euid/egid:  old: %d/%d  new: %d/%d\n",
		oldeuid, oldegid, uid, gid);
	if(setegid(gid) < 0)
		L_LOG_2(L_ALWAYS, 0, "setegid(%d) failed %d\n", gid, errno);
	if(seteuid(uid) < 0)
		L_LOG_2(L_ALWAYS, 0, "seteuid(%d) failed %d\n", uid, errno);
	stat = real_getfile();
	if(seteuid(oldeuid) < 0)
		L_LOG_2(L_ALWAYS, 0, "(re)seteuid(%d) failed %d\n",
			oldeuid, errno);
	if(setegid(oldegid) < 0)
		L_LOG_2(L_ALWAYS, 0, "(re)setegid(%d) failed %d\n",
			oldegid, errno);
#ifdef	NGROUPS
	if (oldgrps >= 0) (void) setgroups(oldgrps, oldg);
#endif	NGRROUPS
	return(stat);
}

real_getfile()
{
#endif	SETEUID
	int     t;
	register char   *file = filename;
	register acc = f_access;
	register int    ot;

	if (file && *file)
		(void) strcpy(realname, file);
	else switch(ltranstype)
	{
	default:	L_LOG_1(L_GENERAL, 0, "Unknown type %x\n", ltranstype);
			if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "Can't suck this type (yet)");
			return(0);
			break;
#ifdef	MAIL
	case T_MAIL:	if (!find_sucker(realname))
			{	if (*reason) (void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, "No waiting mail now");
				return(0);
			}
			else L_LOG_2(L_GENERAL, 0, "Found sucker %s (%d)\n",
					realname, qfd);
			file = realname;
			if (filename = malloc(strlen(realname)+1))
				(void) strcpy(filename, realname);
#endif	MAIL
#ifdef	NEWS
	case T_NEWS:	if (!find_sucker(realname))
			{	if (*reason) (void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, "No waiting news now");
				return(0);
			}
			else L_LOG_2(L_GENERAL, 0, "Found sucker %s (%d)\n",
					realname, qfd);
			file = realname;
			if (filename = malloc(strlen(realname)+1))
				(void) strcpy(filename, realname);
#endif	NEWS
	}

	L_LOG_1(L_GENERAL, 0, "File is %s\n",realname);

	if(acc & ACC_GET){
		/* reading here */
		t = perms(file,0,uid,gid);     /* check access permissions */
		L_WARN_1(L_ALWAYS, 0, "perms gave %d (1)\n", t);
		if(acc == ACC_RAR && t >=0){   /* want to delete afterwards */
			t = perms(file,2,uid,gid);
		        L_WARN_1(L_ALWAYS, 0, "perms gave %d (2)\n", t);
			if(t <0){
				L_WARN_1(L_10, 0, 
				    "can read %s but can't delete\n", file);
				if (*reason) (void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, "Cannot delete file");
			}
		}
		else if(t < 0 && !*reason)
			(void) strcpy(reason, "Cannot read file .. ");
		if(t < 0 || (t = open(file,0))<0){ /* can't do it */
			L_WARN_1(L_GENERAL, 0, 
				"Cannot open file for reading %s\n",file);
			return(0);
		}
		else f_fd = t;		/* ok */
		return(1);
	}
	t = perms(file,1,uid,gid);	/* check access permissions */
	L_WARN_1(L_ALWAYS, 0, "perms gave %d (3)\n", t);
	if(t < 0) return (0);		/* will be able to write/create it */
	(void) strcpy(localname, file);	/* initialise */
	genlocalname();			/* generate a local temp file */
	L_LOG_1(L_10, 0, "Localfile is %s\n",localname);
	ot = access(file, 2); /* it ot >= 0 - file exists + is writable */
	if(acc == ACC_MO){		/* make only */
		if(ot >=0 || errno == ETXTBSY){
			if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "File exists");
			return(0);
		}
		ot = creat(localname,0600);
	}
	else if(acc == ACC_RO){		/* replace only */
		if(ot >= 0)
			ot = creat(localname,0600);
		else
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "file does not exist");
		}
	}
	else if(acc == ACC_ROM){	/* replace or make */
		if(ot < 0 && errno == ETXTBSY)
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "text file busy");
		}
		else
		{	sftp(ACCESS)->ivalue =
				f_access = (ot >= 0) ? ACC_RO : ACC_MO;
			sftp(ACCESS)->sflags |= SENDACCESS;
			ot = creat(localname,0600);
		}
	}
	else if(acc == ACC_AO || acc == ACC_AOM){	/* append (or make) */
		if(ot>=0)
		{	sftp(ACCESS)->ivalue = f_access = ACC_AO;
			sftp(ACCESS)->sflags |= SENDACCESS;
			ot = creat(localname,0600);
		}
		else if(errno == ETXTBSY)	/* Can't append it */
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "text file busy");
		}
		else if(acc == ACC_AOM)		/* append or make */
		{	sftp(ACCESS)->ivalue = f_access = ACC_MO;
			sftp(ACCESS)->sflags |= SENDACCESS;
			ot = creat(localname,0600);
		}
		else
		{	if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "file does not exist");
		}
	}
	else if(acc == ACC_TJO || acc == ACC_TJI)
		ot = creat(localname,0600);
	else {				/* ?????? unknown */
		L_WARN_1(L_10, 0, "Not yet allowing mode %04x\n", acc);
		if (*reason) (void) strcat(reason, MSGSEPSTR);
		(void) strcat(reason, "Unsupported access mode");
		return(0);
	}
	if(ot<0)			/* return result of access */
		return(0);
	f_fd= ot;
	return(1);
}

/*
 * This routine is used to reopen the temorary file when trying to
 * resume a transfer.
 */

setrestart(is_ftp)
{
	register val    restart;
	register i = -1;
	long    size,tsize,bsize();
	struct passwd *pwd;

	if(readdocket( (docketp) 0)<0)	/* read the docket */
	{	L_WARN_0(L_GENERAL, 0, "failure to read docket !!\n");
		goto bad;
	}
	if(!filename || !is_ftp)	/* check for mail etc. */
	{	filename = malloc( strlen(realname) + 1 );
		(void) strcpy(filename,realname);
		if(!is_ftp)
		{
#if defined(MAIL) || defined(NEWS) || defined(JTMP) || defined (PP)
			L_LOG_0(L_10, 0, 
				"Resumption of non FTP\n");
			if(t_uid != uid || t_gid != gid){
/*				L_WARN_4(L_10, 0, */
				L_WARN_4(L_ALWAYS, 0,
				"Liar - I don't believe it (%d/%d %d/%d)\n",
					t_uid, uid, t_gid, gid);
/*				goto bad;	*/
			}
#else
			goto bad;
#endif
		}
		else if(f_access != ACC_TJO && f_access != ACC_GJI)
		{	L_WARN_0(L_MAJOR_COM, 0, 
			  "Trying to resume a transfer without a filename\n");
			goto bad;
		}
	}
	else if(!username){
		L_WARN_0(L_MAJOR_COM, 0, 
			"Resume with NO user name -- NO WAY !!\n");
		goto bad;
	}
	uid = t_uid;
	gid = t_gid;
	restart = sftp(INITRESM)->ivalue;       /* get the mark */
	if(f_access & ACC_GET)
	{	L_LOG_3(L_MAJOR_COM, 0,
			"Trying to restart a transmission at %d (%d/%d)\n",
			restart, lastmark, rec_mark);
		if(restart == INFINITY)
		{	L_LOG_0(L_MAJOR_COM, 0, "At end of file");
			if(lastmark != INFINITY)
			{	L_LOG_0(L_MAJOR_COM, L_CONTINUE, 
					"But we ain't there yet !!\n");
				goto bad;
			}
			L_LOG_0(L_MAJOR_COM, L_CONTINUE, " (ok)\n");
			return;
		}
		i = open(realname,0);
		if(i < 0)
		{	L_WARN_2(L_MAJOR_COM, 0, 
				"Cannot reopen original file %s - %d\n",
				realname, errno);
			ok = 0;
			ecode = ER_FILE_ACCESS;
			return;
		}
		if(!restart)
		{	L_LOG_0(L_MAJOR_COM, 0, 
				"Starting at begining of file\n");
			rec_mark = 0;
			last_rlen = 0;
			f_fd = i;
			return;
		}
		if(restart < rec_mark || restart > lastmark)
		{	L_WARN_3(L_GENERAL, 0, 
				"restart mark (%d) out of range %d - %d\n",
				restart, rec_mark, lastmark);
			goto bad;
		}
		size = bsize(realname); /* check size of file */
		tsize = lastmark;
		tsize <<= LSHIFT;
		if(tsize > size)       /* check that the seek is possible */
		{	L_WARN_2(L_MAJOR_COM, 0, 
				"File has changed size %d -> %d !\n",
				tsize, size);
			goto bad;
		}
		tsize = restart;
		tsize <<= LSHIFT;
		last_rlen = docp[restart & MASK];
		rec_mark = restart;
		lseek(i,tsize,0);
		f_fd = i;
		return;
	}
	/* now deal with a reception */
	L_LOG_3(L_MAJOR_COM, 0, "Trying to resume a reception at %d (%d/%d)\n",
			restart, lastmark, rec_mark);
	if(restart == INFINITY)
	{	L_LOG_0(L_MAJOR_COM, 0, "Trying from end of file ");
		if(lastmark != INFINITY)       /* I think this is wrong ?? */
		{	L_LOG_0(L_MAJOR_COM, L_CONTINUE,
				"We must have lost some data\n");
			restart = rec_mark;
			sftp(INITRESM)->sflags |= TOSEND;
			sftp(INITRESM)->ivalue = restart;
		}
		else
		{	L_LOG_0(L_MAJOR_COM, L_CONTINUE, " (ok)\n");
			return;
		}
	}
	if(!restart)           /* restart is at begining */
	{	L_LOG_0(L_MAJOR_COM, 0, 
			"Trying from start of file\n");
		i = creat(localname,0600);
		if(i < 0)
		{	L_WARN_2(L_MAJOR_COM, 0, 
				"Can't create file %s again - %d\n",
					localname, errno);
			goto bad;
		}
		rec_mark = 0;
		last_count = 0;
		last_rlen = 0;
		f_fd = i;
		return;
	}
	i = open(localname,1);          /* reopen localfile */
	if(i < 0)      /* it's not there - recreate it */
	{	L_WARN_2(L_MAJOR_COM, 0, 
			"Cannot find local file %s (%d) Going back to start of file\n",
			localname, errno);
		sftp(INITRESM)->sflags |= TOSEND;
		sftp(INITRESM)->ivalue = 0;
		i = creat(localname,0600);
		if(i < 0){      /* NO!! */
			L_WARN_2(L_MAJOR_COM, 0, 
				"Can't create a new local file %s (%d)\n",
				localname, errno);
			goto bad;
		}
		rec_mark = 0;
		last_count = 0;
		last_rlen = 0;
		f_fd = i;
		return;
	}
/*
 * now check that the marks are ok.
 */
	if(restart > rec_mark)          /* we haven't got there yet */
		restart = rec_mark;
	if(restart < lastmark ){        /* weve gone past there !!! */
		L_WARN_3(L_GENERAL, 0, 
			"Restart request %d < %d -- missed %d marks\n",
				restart, lastmark, lastmark - restart);
				/* might have a problem here ?? */
	}
	size = bsize(localname);        /* check sizes */
	tsize = restart;
	tsize <<= LSHIFT;
	if(tsize > size){
		L_WARN_3(L_MAJOR_COM, 0, 
			"???? temp file %s has changed size %d -> %d !!\n",
			localname, tsize, size);
		goto bad;
	}
	if(restart == rec_mark){        /* now set up values */
		lastmark = rec_mark;
		last_count = lr_bcount;
		last_rlen = lr_reclen;
	}
	else restart = rec_mark = lastmark;
				/* last_count and last_rlen are set already */
	if(restart != sftp(INITRESM)->ivalue){          /*send if changed */
		sftp(INITRESM)->ivalue = restart;
		sftp(INITRESM)->sflags |= TOSEND;
	}
	(void) lseek(i,last_count,0);           /* set the position */
	f_fd = i;
	return;
bad:
	(void) close(i);                /* bad access permissions */
	deldocket(0);
	ok = 0;
	ecode = ER_NO_DOCKET;
}

/*
 * check output device type
 */

extern char *outdtype[]; /* now in lib/gen/conf.c */

chouttyp(dev)
char    *dev;
{
	register char    **ap, *p;

	if(!dev)
		return(0);

	for(p = dev ; *p ; p++)         /* lowerfy */
		if(*p >= 'A' && *p <= 'Z')
			*p += 'a' - 'A';

	for(ap = outdtype ; *ap ; ap++)
		if(strcmp(*ap, dev) == 0)
			return(1);
	return(0);
}

#ifdef	FREEDISK
/* Is there enough disk to do a transfer in ?
 */
nofreedisk(file, ltranstype)
char *file;
{
	int root = -1;
	char *diskname;
#define	BIG(x)	(x > 40000000)	/* 2 ** 32 / 100 */

/*	total		number of blocks in the physical partition
 *	disk_kbytes	number of AVAILABLE kilobytes (4.2 keeps about 10%)
 *	avail_kbytes	kbytes of free disk
 *	percent		(avail_kbytes / disk_kbytes) * 100
 *	bytes_block	(SYSV) bytes per block
 *	spare		(4.2)  # of blocks to be left free by the FFS
 *	SUPERBOFF	byte offset of the (first) superblock
 */
#ifdef	_42
	struct fs sblock;
#define	total		sblock.fs_dsize
#define	disk_kbytes	(((total - spare) * sblock.fs_fsize) / 1024)
#define	SUPERBOFF	(SBLOCK * DEV_BSIZE)
#else	_42
	struct filsys sblock;
#define	bytes_block	((sblock.s_magic == FsMAGIC) ? \
			 (sblock.s_type == Fs2b) ? 1024 : 512 : 512)
#define	avail_kbytes	((sblock.s_tfree * bytes_block) / 1024)
#define	disk_kbytes	((sblock.s_fsize * bytes_block) / 1024)
#endif	_42

	int	x;
	int	W_free = FREEDISK;
	int	W_percent = 0;
	struct	stat	wantstat;
	int	gotstat = 0;

	for (x=0; DISKFULLS[x].Ddevice; x++) if (DISKFULLS[x].Dtype == ltranstype)
	{	L_LOG_4(L_DISKFULL, 0,
			"Try %s (%d/%d/%d)\n",
				DISKFULLS[x].Ddevice,
				DISKFULLS[x].Dtype,
				DISKFULLS[x].Dbytes,
				DISKFULLS[x].Dpercent);
		if ((root = open(diskname=DISKFULLS[x].Ddevice, 0)) >= 0)
		{	/* Semantics are different for FTP from the rest,
			 * as the rest are always to a known disk.
			 *
			 * If unknown disk, then open the object & the
			 * specified disk & compare .....
			 */
			if (ltranstype == T_FTP)
			{	struct stat rootstat;
				if (!gotstat)
				{	char *slash = rindex(file, '/');
					if (slash) *slash = '\0';
					gotstat = !(stat(file, &wantstat));
					if (slash) *slash = '/';
					if (!gotstat)
						{ close(root); break; }
				}
				fstat(root, &rootstat);
				if (wantstat.st_dev != rootstat.st_rdev)
					{ close(root); root = -1; continue; }
			}
			W_free = DISKFULLS[x].Dbytes;
			W_percent = DISKFULLS[x].Dpercent;
			L_LOG_4(L_DISKFULL, 0,
				"Use %s (%d/%d/%d)\n",
				DISKFULLS[x].Ddevice,
				DISKFULLS[x].Dtype,
				DISKFULLS[x].Dbytes,
				DISKFULLS[x].Dpercent);
			break;
		}
	}

	if (root>=0 && lseek(root, (long) SUPERBOFF, 0) >= 0 &&
		read(root, &sblock, sizeof sblock) == sizeof sblock)
	{	int percent;
#ifdef	_42
		int avail_kbytes, spare;
		spare = (total * sblock.fs_minfree +(100/2)) / 100;
	        avail_kbytes = ((sblock.fs_cstotal.cs_nbfree*sblock.fs_frag +
	            sblock.fs_cstotal.cs_nffree - spare) * sblock.fs_fsize)/
		1024;
		L_LOG_4(L_DISKFULL, 0, "sblock.fs_cstotal.cs_nbfree=%d, sblock.fs_frag=%d, sblock.fs_cstotal.cs_nffree=%d, spare=%d,\n", 
			sblock.fs_cstotal.cs_nbfree, sblock.fs_frag, sblock.fs_cstotal.cs_nffree, spare);
		L_LOG_4(L_DISKFULL, 0, "sblock.fs_fsize=%d, total=%d, sblock.fs_minfree=%d -> %d\n",
			sblock.fs_fsize, total, sblock.fs_minfree, avail_kbytes);
#else
		L_LOG_4(L_DISKFULL, 0, "sblock.s_tfree=%d, sblock.s_fsize=%d, bytes_block=%d -> %d\n",
			sblock.s_tfree, sblock.s_fsize, bytes_block, avail_kbytes);
#endif
		percent = (BIG(avail_kbytes)) ?
			(avail_kbytes+(disk_kbytes/200)) / (disk_kbytes/100):
			(avail_kbytes*100 +(disk_kbytes/2)) / disk_kbytes;
		L_LOG_4(L_DISKFULL, 0, "(%d*100 + %d/2) /%d = %d\n",
			avail_kbytes, disk_kbytes, disk_kbytes, percent);
		close(root); root = -1;
		L_LOG_4(L_DISKFULL, 0, "diskfree(%s, %x) want %dk %d%%\n",
			file, ltranstype, W_free/1024, W_percent);
		L_LOG_3(L_DISKFULL, 0, "%s has %dk %d%%\n",
			diskname, avail_kbytes, percent);
		if (avail_kbytes < (W_free/1024) || percent < W_percent)
		{	L_ERROR_4(L_ALWAYS, 0, "%s (%d) -- Only %dk %d%%\n",
				file, ltranstype, avail_kbytes, percent);
			return 1;
		}
	}
	else if (root >= 0) close(root);

	return 0;
}
#endif	FREEDISK

set_user(pis_ftp)
int *pis_ftp;
{
	int *puid	= (int *) 0;
	char *name	= (char *)0;
	char *user	= (char *)0;

	switch(ltranstype)
	{
#ifdef	MAIL
	case T_MAIL: name = "Mail"; user = MAILuser; puid = &MAILuid; break;
#endif	MAIL
#ifdef	PP
	case T_PP: name = "PP"; user = PPuser; puid = &PPuid; break;
#endif	PP
#ifdef	NEWS
	case T_NEWS: name = "News"; user = NEWSuser; puid = &NEWSuid; break;
#endif	NEWS
#ifdef	JTMP
	case T_JTMP: name = "JTMP"; user = JTMPuser; puid = &JTMPuid; break;
#endif	JTMP
	}

	if (user)
	{	if (username || filename)
		{	L_WARN_3(L_GENERAL, 0, 
				"On %s transfer %s%s%s given\n",
				(filename) ? "filename" : "",
				(username && filename) ? " and " : "",
				(username) ? "username" : "");
			ecode = (username) ? (filename) ? ER_USERFILE_GIVEN:
				ER_USERNAME_GIVEN : ER_FILENAME_GIVEN;
			return 0;
		}
		username = malloc(strlen(user)+1);
		(void) strcpy(username, user);
		if(cur_user == NULL && (cur_user=getpwnam(user)) == NULL)
		{	L_WARN_2(L_GENERAL, 0, 
				"%s user \"%s\" is unknown!\n", name, user);
			ecode = ER_UNKNOWN_USER;
			return 0;
		}
		else
		{	uid = cur_user->pw_uid;
			gid = cur_user->pw_gid;
			if (puid) *puid = uid;
			*pis_ftp = 0;
		}
		return 1;
	}

	if((cur_user = getpwuid(uid)) == NULL &&
		(cur_user = find_user_id((char *) 0, uid, altpwfile)) == NULL)
	{	L_WARN_1(L_GENERAL, 0, "Cannot find user %d\n", uid);
		ecode = ER_UNKNOWN_USER;
		return 0;
	}

	return 1;
}

/* Someone is trying to suck a file -- got anything relevant ? */
find_sucker(buffer)
char *buffer;
{
	DIR     *dirp;
	register struct  QUEUE   *Qp;
	register char    *q;
	char    xbuff[ENOUGH];
	register struct direct *dp;
	struct tab tab;

	if (qfd != -1)
	{
		L_LOG_1(L_ALWAYS, 0, "Had to close qfd (%d) in find_sucker\n",
			qfd);
		(void) close(qfd);
		qfd = -1;
	}

	for(Qp = QUEUES ; Qp->Qname ; Qp++){
		if (Qp->Qmaster)		continue;
		if( (q = Qp->Qdir) == NULL) q = Qp->Qname;
		if(*q != '/'){
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
			q = xbuff;
		}
		L_LOG_1(L_SUCK_DEB, 0, "Q: %s\n", q);
		if(chdir(q) < 0 || (dirp = opendir(".")) == NULL) continue;
		for(dp = readdir(dirp) ; dp ; dp = readdir(dirp) )
		{	
			L_LOG_1(L_SUCK_DEB, 0, "File: %s\n", dp->d_name);
			if(*dp->d_name != 'q')			continue;
			errno = -2;
			if((qfd = open(dp->d_name, 2)) < 0 ||
			   (read(qfd, (char *)&tab, sizeof tab)!=sizeof tab))
			{	(void) close(qfd);
				L_LOG_2(L_SUCK_DEB, 0, "open/rd gave %d/%d\n",
					qfd, errno);
				qfd = -1;
				continue;
			}
			L_LOG_4(L_SUCK_DEB, 0,
				"Host: %s, state %x, access %x, file %s",
				(tab.l_hname) ? (char *) &tab + tab.l_hname :
					"<NOHOST>",
				tab.status, tab.t_access,
				(tab.l_fil_n) ? (char *) &tab + tab.l_fil_n:
					"<NOFILE>");
			L_LOG_2(L_SUCK_DEB, L_CONTINUE, ", type %x, %s\n",
				tab.t_flags & T_TYPE, dp->d_name);

			if(strcmp(hostname, tab.l_hname + (char *)&tab))
			{	(void) close(qfd);
				qfd = -1;
				continue;
			}

			switch(tab.status){
			case DONESTATE:
			case ABORTSTATE:
			case REJECTSTATE:
			case CANCELSTATE:
					(void) close(qfd); qfd = -1; continue;
			}

			if (ltranstype != (tab.t_flags & T_TYPE))
			{	(void) close(qfd);
				qfd = -1;
				continue;
			}
			/* Set id = atoi(&(Xp->Xname[1])); ?? */
			/* railookup(tab.t_access & 0xFFFF ,acctab)); */
			if (!tab.l_fil_n)
			{	(void) close(qfd);
				qfd = -1;
				continue;
			}

			(void) strcpy(buffer, (char *) &tab + tab.l_fil_n);
			if (!*buffer)
			{	(void) close(qfd);
				qfd = -1;
				continue;
			}

			/* Check that the file exists .... */
			{	struct stat stat_buf;
				int rc;
				errno = 0;
				if (rc = stat(buffer, &stat_buf))
				{	L_LOG_4(L_GENERAL, 0,
						"No %s (%s) (%d/%d)\n",
						buffer, dp->d_name, rc,errno);
					(void) close(qfd);
					qfd = -1;
					continue;
				}
			}
			L_LOG_4(L_SUCK_INFO, 0,
				"Host: %s, state %x, access %x, file %s",
				(tab.l_hname) ? (char *) &tab + tab.l_hname :
					"<NOHOST>",
				tab.status, tab.t_access,
				(tab.l_fil_n) ? (char *) &tab + tab.l_fil_n:
					"<NOFILE>");
			L_LOG_2(L_SUCK_INFO, L_CONTINUE, ", type %x, %s\n",
				tab.t_flags & T_TYPE, dp->d_name);
			(void) closedir(dirp);
			return 1; 
		}
		(void) closedir(dirp);
	}
	(void) close(qfd);
	qfd = -1;

	return 0;
}
