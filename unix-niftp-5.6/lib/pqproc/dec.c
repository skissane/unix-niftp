#ifndef	lint			/* unix-niftp lib/pqproc/dec.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/dec.c,v 5.6.1.7 1993/05/10 13:57:59 pb Rel $";
#endif	lint

#include "files.h"
#define DEFINES
#include "ftp.h"
#include "infusr.h"
#include "nrs.h"

/* file
 *                       dec.c
 *
 * last changed  19-Jan-84
 *
 * this file contains various routines with no where else to go
 *
 * 5-oct-83 creat users file as mode 666 whilst not running as root
 * $Log: dec.c,v $
 * Revision 5.6.1.7  1993/05/10  13:57:59  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC: Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.6  1993/01/10  07:11:06  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:01:44  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:46  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:49:51  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:23:31  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:12:37  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:32:30  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:27:05  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:04:32  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/04/12  09:36:03  pb
 * Add NFS fixes (seteuid)
 * 
 * Revision 5.0  87/03/23  03:49:09  pb
 * As sent by wja@nott.cs
 * 
 * Revision 5.0  87/03/23  03:49:09  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

/* on some systems we have multiple groups */

/*
 * NGROUPS should be found in <sys/param.h> but it will not compile with this
 * in. Yeuch !!!!!
 * #include <sys/param.h>
 */
#ifndef	NO_INCLUDE_PARAM
#include <sys/param.h>
#endif	/* NO_INCLUDE_PARAM */

#ifdef  NGROUPS
/* #define NGROUPS 8               /* what it currently is in param.h */
#include <grp.h>
int     lgroups[NGROUPS];
int     ngroups;
int     gotgroups;
#endif

extern char *index();

/* stat a file ASIS (e.g. root) and as user */

stat_as(f, inodp, uid, gid)
char	*f;
struct  stat *inodp;
uidtype uid,gid;
{	int i = stat(f,inodp);
#ifdef	SETEUID
	if (i == -1) {
		int oldeuid = geteuid();
		int oldegid = getegid();
#ifdef	NGROUPS
		int oldg[NGROUPS];
		int oldgrps = getgroups(sizeof oldg / sizeof oldg[0], oldg);
		if (oldgrps >= 0) (void) setgroups(ngroups, lgroups);
#endif	NGRROUPS
		seteuid(uid);
		setegid(gid);
		i = stat(f,inodp);
		seteuid(oldeuid);
		setegid(oldegid);
#ifdef	NGROUPS
		if (oldgrps >= 0) (void) setgroups(oldgrps, oldg);
#endif	NGRROUPS
	}
#endif	SETEUID
	return i;
}

/* set up all default settings */

set_defval()
{
	register struct sftparams *p;
	register char   **st,*ptr;
	extern  int    restricted;

	/* reinitialise strings */
	(void) strcpy(whichhost, "local");      /* local */
	(void) strcpy(tofrom, "with"); /*don't know direction to start with */

	for(st=sftstrings;st<&sftstrings[STRINGCOUNT]; *st++ = NULL)
		if(*st)                 /* free all strings */
			free(*st);

	/* default tabs and default output device */

	ptr = horiztab = malloc(2);      /* set default tabs - nasty */
	*ptr++ = 'x'; *ptr = 0;
	ptr = outdev = malloc(3);        /* default output device */
	*ptr++ = 'l'; *ptr++ = 'p'; *ptr = 0;

	/* reset the sft parameters */

	for(p = sfts ; p->attribute != (char)0xFF ; p++){
		p->ivalue = p->dvalue;
		p->sflags &= ~(TOSEND|FAILURE);
	}

	/* reset all necessary variables */

	cur_user = 0;
#ifdef  NGROUPS         /* 4.2 group stuff !! */
	ngroups = 0;
	gotgroups = 0;
#endif
	direction =0; tstate = 0; bcount=0; reclen=0;
	last_count=0; last_rlen=0; rec_mark=0; lastmark=0;
	lr_reclen = 0; lr_bcount = 0;
	spopts_set = 0;
	nrestarts = 0;
	ecode = 0;
	code = 0;
	*localname = 0;
	*reason = 0;
#ifdef JTMP
	if(jtmpfile)
		*jtmpfile = 0;
#endif JTMP
	deldocket(0);
	stoptimer();
	st_of_tran = VIABLE;     /* viable */
	fnleft=0;
	rej_resume = 0;
	nproterrs = 0;          /* number of protocol errors ( repeated ) */
	restricted = 0x40;	/* user is not yet restricted */
	zap_record();
}

/*
 * This routine decodes the parameter sent on a command as specified
 * by the associated qualifier.
 */

dec_command(qual,t_buff,ip)
val     qual,*ip;
char    *t_buff;
{
	register val     integ;
	register int    c,k;
	switch(qual & FORMAT){
	case ATTRIBUNKNOWN:
	case NOVALUE:           /* no value with this parameter */
		return;
	case INTEGER:           /* got an integer value */
		integ = 0;
		c = r_rec_byte();
		if(!(c & END_REC_BIT) ){
			integ = c << 8;         /* byte swapping */
			c = r_rec_byte();       /* most significant first */
			integ |= c;
		}
		if(c&END_REC_BIT)             /* got end of record - error */
			break;
		*ip = integ;                    /* return the value */
		return;
	case STRING:                            /* string value */
		k=r_rec_byte();                 /* number of characters */
		if(k & END_REC_BIT)             /* in string */
			break;
		integ = k;
		c =0;
		while(k--){                     /* the actual string */
			if((c = r_rec_byte()) & END_REC_BIT)
				break;
			*t_buff++ = c & 0177;
		}
		if(c & END_REC_BIT)
			break;
		*t_buff = 0;                    /* null terminate it */
		*ip = integ;
		return;
	}
/*
 * got an end of record if got here - then its a protocol error
 * If waiting for STOPACK ignore it. This might cause the program to
 * hang but it is probarbly ok since the file has arrived ( or gone )
 * at this point.
 */

	L_WARN_0(L_GENERAL, 0, "Protocol error in dec_command\n");
	if(tstate == STOPACKs){
		*ip = 0;
		*t_buff = 0;            /* ignore here ?????? */
		return;
	}
	prot_err(124, qual, qual);                     /* deal with the error */
}

/*
 * routine to send the command 'comm' with associated parameters given
 * in the sft table. Only send parameters with the TOSEND bit set,
 * clearing them in the process.
 */

send_qual(comm)
int     comm;
{
	register struct sftparams *p;
	register int    i=0;
	register char   *ptr;
	char *nl;

	init_rec();
	/* Now count the number of parameters to be sent.
	 *
	 * Pay particular attention to multi-line messages.
	 */
	for(p = sfts ; p->attribute != (char)0xFF ; p++ )
		if(p->sflags & TOSEND)
		{   if(p->attribute == ACTMESS || p->attribute == INFMESS)
			for (nl=sftstrings[p->ivalue];
			     (nl = index(nl, MSGSEPCH)) != NULL; nl++, i++) {
				if (! nl[1]) {	/* No data after newline */
					*nl = '\0';
					break;
				}
			}
			i++;            /* count the number of parameters */
		}
	add_to_rec(comm);               /* send the command */
	add_to_rec(i);                  /* number of parameters */

	/* send all parameters - easy */
	for(p = sfts ; p->attribute != (char)0xFF ; p++ )
	{	if(!(p->sflags & TOSEND ))
		{	if(ftp_print & L_OMIT_ATTR)
			{	L_LOG_0(L_OMIT_ATTR, 0, " (omit)  ");
				log_attr(p->attribute, p->squalifier,
					p->ivalue, 
					((p->squalifier & FORMAT) == STRING) ?
					sftstrings[p->ivalue] : (char *) 0);
			}
			continue;
		}

		/* OK -- Have sent it, so can now shut up .... */
		p->sflags &= ~TOSEND;

		/* Avoid <= 0 : use == 0 instead */
		if ((p->squalifier & (FORMAT | OPER)) == (INTEGER | LE) &&
			p->ivalue == 0)
			p->squalifier = ((p->squalifier) & ~OPER) | EQ;
		if (ftp_print & L_ATTRIB)
		{	L_LOG_0(L_ATTRIB, 0, "sending  ");
			log_attr(p->attribute, p->squalifier,
				p->ivalue,
				((p->squalifier & FORMAT) == STRING) ?
				sftstrings[p->ivalue] : (char *) 0);
		}
		add_to_rec(p->attribute);       /* the actual attribute */
		add_to_rec(p->squalifier);      /* the qualifier */
		switch(p->squalifier & FORMAT){
		case INTEGER:
			add_to_rec((int) ((p->ivalue >>8) & MASK) );
			add_to_rec((int) (p->ivalue & MASK) );
			break;
		case STRING:
			ptr = sftstrings[p->ivalue];
			if(!ptr){
				add_to_rec(0);  /* null strings */
				break;
			}
			/* Now, having added the multi-line messages to the
			 * count, send them off as multiple messages.
			 */
			if(p->attribute == ACTMESS || p->attribute == INFMESS)
			    while((nl = index(ptr, MSGSEPCH)) != NULL) {
				i = nl-ptr;        /* length of string */
				if (i > 255) i=255; /* Don't wrap ! */
				add_to_rec(i);
				while(i--)              /* send string */
					add_to_rec(*ptr++ & MASK);
				add_to_rec(p->attribute);
				add_to_rec(p->squalifier);
				ptr=nl+1;		/* skip nl */
			}
			i = strlen(ptr);        /* length of string */
			if (i > 255) i=255;	/* Don't wrap ! */
			add_to_rec(i);
			while(i--)              /* send string */
				add_to_rec(*ptr++ & MASK);
			/* fix to stop extra messages */
			/* delete sent information and action messages */
			if(&sftstrings[p->ivalue] == &infomsg
					|| &sftstrings[p->ivalue]== &actnmsg){
				free(sftstrings[p->ivalue]);
				sftstrings[p->ivalue] = 0;
			}
			break;
		}
	}
	end_rec();              /* finish the record */
	shove_it();             /* push the record */
}

/*
 * function to return a pointer to a given variable in the sfttab
 * used to check other parameters which have been / will be processed
 */

struct  sftparams *
sftp(ival)
int     ival;
{
	register char    c = ival;
	register struct sftparams *p;
	for(p = sfts ; p->attribute != c ;)
		p++;
	return(p);
}

/*
 * check access permissions on file f - with uid and gid.
 * check parental directory first - then the file
 * itself. flag = 0 read , 1 write/create , 2 delete
 * Now copes with groups a'la 4.2 systems
 */
/* under 4.2 the whole system is so much easier */
/* we have setregid() and setreuid() */

perms(f,flag,uid,gid)
char    *f;
int     flag;
uidtype uid,gid;
{
	struct  stat inodf;
	struct  stat inodp;
	register char   *p;
	register int    i;
	int statrcf;
	register short  min_mask,iff;
	char    *rindex();

#ifdef  NGROUPS         /* Multi group stuff for 4.2 */
	if(!gotgroups)
		get_groups(gid);
#endif

	/* Stat the file. If it does not exist & we are reading it, fail now */
	if( (statrcf = stat_as(f,&inodf,uid,gid)) ==-1 && flag == 0){
		(void) strcpy(reason,"file not found");
		return(-1);             /* cannot find it - oh well */
	}
	/* If file DOES exist, check it isn't a special */
	if(statrcf != -1){
		if( (inodf.st_mode & S_IFMT) != S_IFDIR
				&& (inodf.st_mode & S_IFMT) != S_IFREG){
			(void) strcpy(reason, "file is a special file");
			return(-1);
		}
		if( (inodf.st_mode & S_IFMT) != S_IFREG && flag != 0){
			(void) strcpy(reason, "cannot write a directory");
			return(-1);
		}
	}

	/* check parental directory IFF it is needed, i.e.
	 * files does not exist OR want to delete it
	 */
	if (statrcf == -1 || flag == 2) {
	    if((p = rindex(f, '/')) == NULL || *f != '/'){
		/* no slash in it. Or not full pathname */
		(void) strcpy(reason,"incorrect /path name");
		return(-1);
	    }
	    if(p == f)                      /* only one slash. i.e. root */
		i = stat("/", &inodp);
	    else {
		*p = 0;
		i = stat_as(f,&inodp,uid,gid);      /* stat the parent */
		*p = '/';
	    }
	    if(i == -1){                    /* can't find directory !! */
		(void) strcpy(reason,"can't find directory");
		return(-1);
	    }
	    iff = inodp.st_mode;
	    if( (iff & S_IFMT) != S_IFDIR) {
		(void) strcpy(reason,"incorrect path/ name");
		return(-1);             /* parent is not a directory !! */
	    }
	    if(uid == 0){               /* su must have access only */
		return(0);      /* on v7 root cannot be stopped */
	    }
	    if(flag != 0)                        /* want to create/delete it */
		min_mask = S_IWRITE|S_IEXEC;
	    else min_mask = S_IREAD;             /* only want to read it */

	    if(uid != inodp.st_uid){
		min_mask >>= 3;
#ifdef  NGROUPS                 /* multi group stuff */
		for(i = 0; i < ngroups ; i++)
			if(lgroups[i] == inodp.st_gid)
				break;
		if(i >= ngroups)
			min_mask >>= 3;
#else
		if(gid != inodp.st_gid)
			min_mask >>=3;
#endif
	    }
	    if( (iff&min_mask) != min_mask) {
		(void) strcpy(reason, "incorrect permission on directory");
		return(-1);     /* cannot access directory */
	    }
	    return(0); /* ok - so possible to delete or create */
	}

	/* If we get here, file pre-exists, and we want to read or write */

	if (flag == 1)
		min_mask = S_IWRITE;        /* write access on file */
	else min_mask = S_IREAD;            /* read access on file */
	if(uid != inodf.st_uid){
		min_mask >>= 3;
#ifdef  NGROUPS         /* 4.2 group stuff !! */
		for(i = 0; i < ngroups ; i++)
			if(lgroups[i] == inodf.st_gid)
				break;
		if(i >= ngroups)
			min_mask >>= 3;
#else
		if(gid != inodf.st_gid)
			min_mask >>=3;
#endif
	}
	if ((inodf.st_mode & min_mask) == min_mask)
		return(0);
	(void) strcpy(reason, "incorrect permission on file");
	return(-1);
}

/* return the number of bits set in a given word */

nbits(l)
register val     l;
{
	register val    i=1 ,j=0;
	do{
		if(l & i)
			j++;
		i<<=1;
	}while(i);
	return(j);
}

/* returns the size of the file filename */

long    bsize(file)
char    *file;
{
	struct  stat   inod;

	if(stat(file,&inod)==-1)
		return( (long) (-1) );
	return(inod.st_size);
}

/* flush out the record. Called after sending of commands */

shove_it()
{
	net_flush();
}

/* copy the file from my local file into the real file */
/* from an idea in ded */

give_file()
{
#ifdef	SETEUID
	int stat;
	int oldeuid = geteuid();
	int oldegid = getegid();

#ifdef	NGROUPS
	int oldg[NGROUPS];
	int oldgrps = getgroups(sizeof oldg / sizeof oldg[0], oldg);
	if (oldgrps >= 0) (void) setgroups(ngroups, lgroups);
#endif	NGRROUPS
	seteuid(uid);
	setegid(gid);
	stat = real_give_file();
	seteuid(oldeuid);
	setegid(oldegid);
#ifdef	NGROUPS
	if (oldgrps >= 0) (void) setgroups(oldgrps, oldg);
#endif	NGRROUPS
	return stat;
}

real_give_file()
{
#endif	SETEUID
	struct  stat inod;
	char    buff[BLOCKSIZ];
	register fdin,fdout,i;
	int     retval = 0;
	i = stat(realname,&inod);
	if(i==-1 || (inod.st_nlink==1 && !(f_access & 0x4))){
		/* file does not exist or only has one link - quick */
		if(i != -1){
			if( (inod.st_mode & S_IFMT) != S_IFREG)
				goto stratb; /*but only if it is a real file*/
			(void) chmod(localname,inod.st_mode & 0777);
			(void) chown(localname,inod.st_uid,inod.st_gid );
		}
		(void) unlink(realname);
		if(link(localname,realname)==-1)
			goto stratb;            /* strategy b - long way */
		(void) unlink(localname);
		*localname = 0;
	}
	else {
stratb:
		if( (fdin = open(localname,0)) < 0)
		{	L_WARN_2(L_GENERAL, 0, "  can't open recieving file %s - errno %d\n",
				localname,errno);
			return(-1);
		}
		fdout = -1;
		if(f_access & 0x4){                     /* appending */
			fdout = open(realname,1);
			(void) lseek(fdout,0L,2);
		}
		if(fdout < 0)                           /* not there create */
			fdout = creat(realname,0666);
		if(fdout < 0)                          /* help! Unix error */
		{	L_WARN_2(L_GENERAL, 0, "can't creat real file %s - errno %d\n"
							,realname,errno);
			(void) close(fdin);
			return(-1);
		}
		while((i=read(fdin,buff,BLOCKSIZ))>0)   /* slow copy */
			if(write(fdout,buff,i) != i){   /* what to do ?? */
				printf(
				"Write failed: give_file (out of space ?)\n");
				retval = -1;
				break;
			}
		(void) close(fdin);             /* close files */
		(void) close(fdout);
		(void) unlink(localname);       /* delete local copy */
		*localname = 0;
	}
	return(retval);
}

/* job input is difficult since the ftp cannot wait for the child to die -
 * it might never. So to stop spurious zombies staying around I have to fork
 * twice and start the job in the grandchild. Also have to make job have the
 * right environment -- cannot have block mode channels open. Also must have
 * a standard input/output.             (for GIVE JOB INPUT)
 * Also cannot vfork at start otherwise files get closed and there is no
 * more logging.
 *      This should set up a proper enviroment and also it should
 *      call the shell of the user to execute it... will be done some day?
 */

start_job(fi)
char    *fi;    /* put file fi into job mill */
{
	register i, cpid;
	int     status;
	char    *p,*rindex();

	while((i=fork())==-1) sleep(1);		/* get a child */
	if(!i){
		while((i=vfork())==-1) sleep(1);/* get a grandchild */
		if(i)
			exit(0);                /* kill the child */
		for(i=0;i<NFILES;i++)           /* close all files */
			(void) close(i);

		(void) open(fi,0);              /* open the file as stdin */

		(void) unlink(fi);              /* get rid of the file */
		(void) dup(open("/dev/null", 1));

#ifdef  NGROUPS                                 /* set group accesses */
		if(cur_user && !gotgroups)      /* Only for 4.2 */
			get_groups(cur_user->pw_gid);   /* +  Paranoia */
		(void) setgroups(ngroups, lgroups);
#endif
		(void) setgid(gid);             /* set the right user */
		(void) setuid(uid);             /* permissions */

		if(cur_user){                   /* paranoia */
			setenvir();             /* set up the environment */
			if(chdir(cur_user->pw_dir) < 0)
				exit(1);    /* give up and go home (no puns)*/
			if((p = rindex(cur_user->pw_shell, '/')) == NULL)
				p = cur_user->pw_shell;
			else
				p++;
			execl(cur_user->pw_shell, p, 0);
		}
		exit(-1);
		/*NOTREACHED*/
	}
	for(;;){
		while((cpid = wait(&status)) != i &&
		      cpid != -1) continue;      /* wait for child */
		if(!(status & 0200))            /* child has not stopped */
			break;
#ifdef  SIGCONT
		kill(i, SIGCONT);               /* send a signal to restart */
#else
		kill(i, SIGKILL);               /* Had enough. kill it off */
#endif
	}
}

/* generate a local file name from the real filename -
 * it will be "x/.tftp.time"
 */

genlocalname()
{
	register char   *p;
	long    l;
	char    *rindex();
	static  char    xpid;
	char	ltranstype = tab.t_flags & T_TYPE;

	if(!xpid)
		xpid = ( getpid() % 26 ) + 'a';
	(void) time((int *)&l);                 /* a unique number */
	if (!*realname && (tab.t_access & ACC_GET)) switch(ltranstype)
	{
#ifdef	MAIL
	case T_MAIL:	(void) sprintf(realname, "%s/%s%c",
			MAILDIR, ltoa(l), xpid);			break;
#endif	MAIL
#ifdef	PP
	case T_PP:	(void) sprintf(realname, "%s/%s%c",
			PPdir, ltoa(l), xpid);				break;
#endif	PP
#ifdef	NEWS
	case T_NEWS:	(void) sprintf(realname, "%s/%s%c",
			NEWSdir, ltoa(l), xpid);			break;
#endif	NEWS
#ifdef	JTMP
	case T_JTMP:	(void) sprintf(realname, "%s/%s%c",
			JTMPdir, ltoa(l), xpid);			break;
#endif	JTMP
	}
	(void) strcpy(localname, realname);
	
	while((p = rindex(localname, '/')) && p != localname && !*(p+1))
		*p = 0;                         /* zap all trailing slashes */
	if (!p) p = localname + strlen(localname) -1;
	sprintf(p+1,".tftp.%s%c",ltoa(l),xpid); /* add the unique bit */
}

/* convert value to ascii string. values will be from a-zzzzzzzzzz */

char    *
ltoa(l)
long    l;
{
	static  char    t[10];
	register char   *p = &t[8];
	do{
		*p-- = l %26 + 'a';
	}while(l /= 26);
	return(++p);
}

/*
 * set up the environment so that the job input will work correctly
 */

static  char    PATH[] = "PATH=:/usr/local:/usr/ucb:/bin:/usr/bin";
static  char    HOME[5+20] = "HOME=";
static  char    SHELL[6+16] = "SHELL=";
static  char    USER[5+10] = "USER=";

static  char    *env[] = {
	PATH,
	HOME,
	SHELL,
	USER,
	(char *)NULL,
	};

setenvir()
{
	extern  char    **environ;

	(void) strcpy(USER+5, cur_user->pw_name);
	(void) strcpy(SHELL+6, cur_user->pw_shell);
	(void) strcpy(HOME+5, cur_user->pw_dir);

	environ = env;                  /* set it up */
}


/*ARGSUSED*/
priv_encode(to, from, mode)
register char *to;
char *from;
{	register int i;
	if (to != from) strcpy(to, from);
	if (!to[0] || !to[1])	return;

	for (i=0; to[i+1]; i++) to[i+1] ^= to[0] & 63;
}

#ifdef  NGROUPS
/*
 * if we are using groups setup the table
 */

get_groups(gid)
int     gid;
{
	struct  group   *getgrent();
	register struct group *gp;
	register i;

	gotgroups++;
	setgrent();
	lgroups[ngroups++] = gid;
	while( (gp = getgrent()) && ngroups != NGROUPS)
		for (i = 0; gp->gr_mem[i]; i++)
			if(!strcmp(gp->gr_mem[i], cur_user->pw_name))
				if(gid != gp->gr_gid)
					lgroups[ngroups++]=gp->gr_gid;
	endgrent();
}

#endif
