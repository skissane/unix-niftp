/* unix-niftp bin/cfp.c $Revision: 5.6.1.2 $ $Date: 1993/01/10 07:02:08 $ */
/*
 *      This is a program to institute copy function for remote sites.
 *      It uses two control files.
 *      1) A global configuration file, which is termcap like in construction
 *      2) A local file which contains local info for that user
 *
 *      Arguments are of the form 'cpf file1  file2@host'
 *
 * $Log: cpf.c,v $
 * Revision 5.6.1.2  1993/01/10  07:02:08  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  16:59:35  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:29:40  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:05:26  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:07:48  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:20:50  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:03:42  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:20:18  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:17:09  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */


#include "ftp.h"
#include "qfiles.h"
#include "nrs.h"

char    userconfig[] = USERCONFIG;      /* local configuration file */

/* look him up in the password file */

struct  passwd  *getpwuid();
struct  passwd  *getpwnam();
struct  passwd  *cur_user = (struct passwd *) 0;

short   uid,gid;

int     direction;      /* direction of transfer */
int	no_password = 0;	/* REALLY give no password */
char    *localname;             /* local file name */
char    *remotename;            /* remote file name */
char    *remotehost;            /* host to be transfered to */

#ifndef ENOUGH
#define ENOUGH  100             /* enough space for strings */
#endif
#define LOTS    1024

char    ename[ENOUGH];          /* effective name of queue file */
char    rname[ENOUGH];          /* real name of queue file */

char    *host_name;             /*Name of host to be looked up in configfile*/

char    *user;                  /* remote user name */
char    *rpasswd;               /* remote user pasword */
char    *queue;                 /* name of queue */
char    *fromperson;            /* If mail the person it is from */

int     access_m = 0;           /* access mode */
int	t_flags = T_FTP;	/* various flags */
int	ftp = 1;
int	mail = 0;		/* set if this is a mail transfer */
int     jtmp = 0;               /* set if this is a jtmp transfer */
char    *jtmpfile;              /* name of jtmp file */
#ifdef NEWS
int	news;			/* set if this is a news transfer */
#endif NEWS
				/*various special strings */
char    *monp;                  /* Monitor message */
char    *devt;                  /* output device type */
char    *devq;                  /* output device type qualifier */
char	_rflp[SOME];		/* space for a password */
char    *rflp = _rflp;          /* remote file password */
char    *spco;                  /* special option string */
char    acop[8];                /* for sysid */

int     queuefd;                /* the queue file descriptor */
int	debug = 0;
struct  host_entry      *Curhost;       /* returned from the database get */
char    *Curchan;               /* pointer to channel required */
char	*Curnet;		/* Unaliases network name */

int     argc;                   /* make them global */
char    **argv;

#ifdef UKC
#define pw_sysid        pw_ext[EMAS]
#endif

char    *rindex(), *index();
char    *strcpy(), *strcat();
char    *malloc(), *getenv();

main(ac,av)
char    **av;
{
	argc = ac;
	argv = av;
	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise. Consult an expert\n");
		exit(2);
	}
	uid = getuid();
	gid = getgid();
	(void) umask(0);

	for(argv++;argc > 1 ; argc-- , argv++)
		if( **argv == '-')
			processargs( &argv[0][1] );
		else
			parsename(*argv);

	if(!remotehost){
		fprintf(stderr,"No remote host given\n");
		exit(1);
	}
	if (localname && !*localname)
	{	fprintf(stderr, "localname was null -- unsetting\n");
		usage(0);
	}
	if (remotename && !*remotename)
	{	fprintf(stderr, "remotename was null -- unsetting\n");
		remotename = (char *) 0;
	}
	if(!direction || (!localname  && access_m != ACC_GJI) ||
			 (!remotename && access_m != ACC_TJO ) )
		if(ftp)
			usage(0);

#ifndef	MAILOWNSMAIL
	if (mail && fromperson)
	{	char	name[9];
		char *at = index(fromperson, '@');
		int len = (at) ? at - fromperson : strlen(fromperson);

		if (len < sizeof name)
		{	bzero(name, sizeof name);
			strncpy(name, fromperson, len);	
			cur_user = getpwnam(name);
			if (cur_user) uid = cur_user->pw_uid;
			if (debug > 0) printf("Setting user to `%s' (%d)\n",
				name, uid);
		}
	}
#endif	MAILOWNSMAIL

	if( !cur_user && (cur_user = getpwuid(uid)) == NULL){
		fprintf(stderr,"Unknown user\n");
		exit(1);
	}
#ifdef  UKC
	(void) strcpy(acop, cur_user->pw_sysid);
#endif
	/*
	 * lookup host in tables.
	 */
	check_host();
	/*
	 * now check file permissions and other such items
	 * Check that we can do what we want to do on this channel
	 */
	validate();
	/*
	 * at this point we don't want any signals since we are about to
	 * write the queue.
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	write_queue();

	return(0);
}

/* print out the usage message */

usage(full)
{
	if (!full)
	{	fprintf(stderr, "Usage: cpf [-JNntuw -a[=]mode -b[=]number -AFUdfmpqs[=]string] f1 f2@host\n");
		fprintf(stderr, "Or:    cpf [-JNntu  -a[=]mode -b[=]number -AFUdfmpqs[=]string] f1@host f2\n");
		exit(1);
	}

	fprintf(stderr,
"-A acnt set account info\n");
	fprintf(stderr,
"-F pass set file password\n");
	fprintf(stderr,
"-J      do a JTMP transfer (priveledged)\n");
	fprintf(stderr,
"-N      do a News transfer (priveledged)\n");
	fprintf(stderr,
"-U user set the user name\n");
	fprintf(stderr,
"-a acc  set the access (\n");
	fprintf(stderr,
"-b size set the word size\n");
	fprintf(stderr,
"-d devt set the device type\n");
	fprintf(stderr,
"-f from set the sender (for mail)\n");
	fprintf(stderr,
"-m mesg set the monitor message\n");
	fprintf(stderr,
"-n      notify even on success\n");
	fprintf(stderr,
"-p pass set the password\n");
	fprintf(stderr,
"-q devq set the device qualifier\n");
	fprintf(stderr,
"-s spec set the special options\n");
	fprintf(stderr,
"-t      send as mail (priveledged)\n");
	fprintf(stderr,
"-w      'write delete', i.e. delete when written to far end\n");
}

/* parse the file name given */

parsename(pathname)
char    *pathname;
{
	char    *index(), *rindex();
	char    *p = pathname;

	/* convert \@ -> @ while looking for unquoted @ */
	while (p = index(p, '@'))
	{	if (p[-1] == '\\') strcpy(p-1, p);
		else break;
	}

	/* no host in this part of name */
	if( p == NULL) {
		/* this is the first parameter */
		if(!direction) localname = pathname, direction = TRANSMIT;
		/* this is a local copy, print out usage */
		else if(direction == TRANSMIT && access_m != ACC_TJO && ftp)
			usage(0);
		/* recieveing */
		else if(localname)
			usage(0);
		else	localname = pathname;
		if (debug > 0) printf("No `@' in `%s', so use `%s' (%s)\n",
			pathname, localname,
			(direction == TRANSMIT) ? "transmit" : "receive");
	} else  {
		if(remotehost){
			fprintf(stderr,"host name already set\n");
			exit(1);
		}
		if (debug > 0) printf("Found `@' in `%s', ", pathname);
		*p++ = 0;       /* zap the '@' character */
		if(*p == '\0'){
			fprintf(stderr,"Null host name\n");
			exit(1);
		}
		remotehost = p;
		if(index(p,'@') != NULL){       /* more than one host */
			fprintf(stderr,"Only one host name may be given\n");
			exit(2);                        /* specified */
		}
		if (debug > 0) printf("giving `%s' and `%s', ",
			pathname, remotehost);
		if(!*pathname && access_m != ACC_TJO && access_m != ACC_TJI && ftp){
			if(direction == TRANSMIT && localname && !remotename){
				if(p = rindex(localname, '/'))
					remotename = p+1;
				else
					remotename = localname;
				if (debug > 0) printf("Xmit with no remote, so `%s' gives `%s', ",
					localname, remotename);
				if(!*remotename)
					usage(0);
				return;
			}
			else {                  /* null pathname*/
				fprintf(stderr,"Null file name\n");
				exit(1);
			}
		}
		if(!direction){                 /* first parameter */
			direction = RECEIVE;
			if(ftp) remotename = pathname;
		}
		else if(direction == TRANSMIT){
			if(remotename)	usage(0);
			else if(ftp)	remotename = pathname;
		} else {
			if(localname)	usage(0);
			else if (ftp)	localname = pathname;
		}
		if (debug > 0) printf("local=%s, remote=%s at %s, dir=%s\n",
			(localname) ? localname : "<unset>",
			(remotename) ? remotename : "<unset>",
			(remotehost) ? remotehost : "<unset>",
			(direction == TRANSMIT) ? localname : "<unset>");
	}
}


/* validate file names */
/* check pathnames etc. */

validate()
{
	char    *fullpath();
	register perms;
	register char    *p;
	register i;
	char    *rindex(),*getpass();
	DIR     *qdir;          /* local access of queue directory */
	static  char    tbuf[ENOUGH];
	char    *address;               /* address of host */
	net_entry       *netp;
	struct  NETWORK *np;
	struct  QUEUE   *qp;

	if (debug > 0) printf("host is %s\n", Curhost->host_alias);
	if (localname && *localname)
	{	if( (localname = fullpath(localname)) == NULL){
			fprintf(stderr, "Cannot find current directory\n");
			exit(1);
		}

		while((p = rindex(localname,'/')) && *(p+1) == '\0' && p > localname)
			*p = '\0';      /* delete all trailing slashes */

		if(direction == TRANSMIT)
			perms = access(localname,4);
		else  {
			perms = access(localname,2);    /* cannot write it */
			if(perms < 0){                  /* maybe it doesn't exist */
				p = rindex(localname,'/');      /* check directory */
				if(p && p != localname){
					*p = 0;
					perms = access(localname,3);    /* search */
					*p = '/';                       /* + write */
				/* if perms is zero here then we can creat file */
				}
				else if(p)
					perms = access("/",3);  /* in root */
			}
		}
		if(perms < 0){
			fprintf(stderr,"Cannot access local file\n");
			exit(3);
		}
		if(direction == TRANSMIT && (t_flags & WRITE_DELETE)) {
			/* can we delete file? */
			p = rindex(localname,'/');	/* check directory */
			if(p && p != localname){
				*p = 0;
				perms = access(localname,3);	/* search */
				*p = '/';			/* + write */
				/* if perms is zero here then we can delete file */
			}
			else if(p)
				perms = access("/",3);  /* in root */

			if(perms < 0){
				fprintf(stderr, "No delete permision on local file (%s)\n", localname);
				exit(3);
			}
		}
	}

	if (debug > 0) printf("host is %s -- find channel (%x|%x)\n",
		Curhost->host_alias, Curchan, Curnet);
	/* now check the address */
	if(Curchan != NULL){
		for(i = 0; i < Curhost->n_nets ; i++){
			netp = &Curhost->n_addrs[i];
			if (debug > 1) fprintf(stderr,
				"try %s|%s=%s\n", netp->net_name, Curchan, Curnet);
			if(netp->net_name == NULL)
				continue;
			else if(!strcmp(netp->net_name,Curnet))
				break;
		}
		if (debug > 1) fprintf(stderr, "i=%d (%d)\n", i, Curhost->n_nets);
		if(i >= Curhost->n_nets){
			fprintf(stderr, "Host does not support channel %s (%s)\n",
						Curnet, Curchan);
			exit(4);
		}
#ifdef NEWS
		if(news)
			address = netp->news_addr;
		else
#endif	NEWS
#ifdef	JTMP
		if(jtmp)
			address = netp->jtmp_addr;
		else
#endif	JTMP
#ifdef	MAIL
		if(mail)
			address = netp->mail_addr;
		else
#endif	MAIL
		address = netp->ftp_addr;
		if(address == NULL || isapplic(address) ){
			fprintf(stderr, "Host does not support facility\n");
			exit(4);
		}
	}
	else {
		/*
		 * now try to find a useable channel
		 */
		char **net;

		for(net=NRSnetworks; *net ; net++)
		{	for(i = 0 ; i < Curhost->n_nets ; i++)
			{	netp = &Curhost->n_addrs[i];
				if(netp->net_name == NULL) continue;
				if (debug > 1) fprintf(stderr,
					"Try %s|%s\n", netp->net_name, *net);
				if(same_net(netp->net_name, *net, 1))	
				{	if (debug > 0) fprintf(stderr,
						"%s == %s\n",
						netp->net_name, *net);
					break;
				}
			}
			/* an unknown network on this host */
			if(i == Curhost->n_nets) continue;
#ifdef	NEWS
			if(news){ if(netp->news_addr != NULL) break; }
			else
#endif	NEWS
#ifdef	JTMP
			if(jtmp){ if(netp->jtmp_addr != NULL) break; }
			else
#endif	JTMP
#ifdef	MAIL
			if(mail){
				if(netp->mail_addr != NULL &&
						!isapplic(netp->mail_addr))
					break;
			}
			else
#endif	MAIL
			if(netp->ftp_addr != NULL) break;
		}
		if(! *net){
			fprintf(stderr,
				"Host does not support required facility\n");
			exit(4);
		}
		Curnet = netp->net_name;
		Curchan = *net;
		if (debug > 0) fprintf(stderr, "Use Curchan %s (%s)\n", Curchan, Curnet);
	}

	host_name = Curhost->host_alias;        /* check host */
	if (debug > 0) printf("host is %s\n", Curhost->host_alias);

	/* Daemon is no enabled */

	if(Curhost->n_disabled)
		fprintf(stderr,"Warning: Host disabled.\n");

	/* now check to see if we can open the queue */

	/*
	 * now find the right queue
	 */
	if (!queue)
	{	for(np = NETWORKS ; np->Nname ; np++)
			if(strcmp(np->Nname, Curchan) == 0){
				queue = np->Nqueue;
				break;
			}

		if(np->Nname == NULL){
			fprintf(stderr, "Bad network - consult an expert\n");
			exit(1);
		}
		for(qp = QUEUES ; qp->Qname ; qp++)
			if(strcmp(qp->Qname, queue) == 0)
				break;

		if(qp->Qname == NULL){
			fprintf(stderr, "Cannot find queue %s, Transfer not queued\n", queue);
			exit(2);
		}
		if(!(queue = qp->Qdir)) queue = qp->Qname;

		if (!queue)
		{	fprintf(stderr, "Panic -- no queue !!\n");
			exit(20);
		}
		/* unless we have a full name, add the default path*/
	}

	if(*queue != '/'){
		sprintf(tbuf, "%s/%s", NRSdqueue, queue);
		queue = tbuf;
	}

	if( (qdir = opendir(queue))==NULL){
		fprintf(stderr,"Cannot access queue %s, Transfer not queued\n", queue);
		exit(2);
	}                               /* now got queue directory open */
	(void) closedir(qdir);          /* so close it */
	if(!Curhost->n_localhost)	*acop = 0;
	if(ftp)
	{
		if(!rpasswd && !no_password) /* use .confftp values */
			get_locals(0);

		if(rpasswd){            /* If we have a password but no */
			if(!user)       /* username... */
#ifdef  UKC
				user = cur_user->pw_sysid;
#else
				user = cur_user->pw_name;
#endif
			goto out;
		}
		if(!user || (!rpasswd && !no_password))
			reget_locals();
	}

/* got enough to be going on with .... */
out:
	if(!access_m)
		if(direction == TRANSMIT)
			access_m = ACC_MO;         /* make only */
		else
			access_m = ACC_RDO;      /* read only */
}

/* generate a local file for placement of queue entry */

get_tmp()
{
	int     pid;
	int     extra = 0;
	int     fd;
	struct  stat    statbuf;
	pid = getpid();

	do{
		sprintf(rname,"%s/q%d.%d",queue,pid,++extra);
	}while(stat(rname,&statbuf) == 0);

	sprintf(ename,"%s/x%d.%d",queue,pid,extra);

	if((fd = creat(ename,(0660)))<0){
		fprintf(stderr,"Cannot create queue entry\n");
		exit(1);
	}
	return(fd);
}

/* routine to read in strings and check options */

struct  tab     tab;    /* the entry for the queue */

struct  acts {
	short   *tabp;
	char    **entryp;
	};

char    *aco = acop;

struct  acts act[] = {
	&tab.r_usr_n, &user,
	&tab.r_usr_p, &rpasswd,
	&tab.r_fil_n, &remotename,
	&tab.l_fil_n, &localname,
	&tab.mon_mes, &monp,
	&tab.dev_type,&devt,
	&tab.dev_tqual, &devq,
	&tab.r_fil_p, &rflp,
	&tab.specopts, &spco,
	&tab.account, &aco,
	&tab.l_network, &Curchan,
	&tab.l_from, &fromperson,
	&tab.l_hname, &host_name,
	&tab.l_jtmpname, &jtmpfile,
	0,0
	};

write_queue()
{
	register struct acts *ap;
	register char   *tp = tab.text;
	register char   *sp;

			/* first set up strings */
	for(ap = act ; ap->tabp ; ap++) /* set up strings */
		if(*ap->entryp && **ap->entryp){
			*ap->tabp = tp - (char *)&tab;
			for( sp = *ap->entryp; *tp++ = *sp++ ;);
		}
		else
			*ap->tabp = 0;

	tab.tptr = tp - (char *)&tab;           /* set up others */

	tab.status = PENDINGSTATE;
	tab.t_access = access_m;
	tab.t_flags = t_flags;
	if(t_flags & BINARY_TRANSFER && tab.bin_size <= 0 )
		tab.bin_size = 8;
	tab.l_usr_id = uid;
	tab.l_grp_id = gid;
	tab.l_docket = 0;
	(void) time(&tab.t_queued);

	queuefd = get_tmp();
	if(write(queuefd,(char *)&tab,sizeof(struct tab)) != sizeof(tab)){
		(void) unlink(ename);
		fprintf(stderr,"Queue write error\n");
		exit(1);
	}
	(void) close(queuefd);
	if(rename(ename,rname) < 0){
		fprintf(stderr, "Rename failed\n");
		exit(1);
	}
	tellftspool();

}

/* Routine to process flags
 * Most can be put anywhere on the line apart from 'o' and 'x' since
 * these must be given before the part of the name with the host in.
 */

struct  _acc {
	char    *amode;
	int     aval;
	} accts[] = {
	"mo",    		ACC_MO,
	"makeonly",    		ACC_MO,
	"make",    		ACC_MO,
	"m",    		ACC_MO,

	"ro",    		ACC_RO,
	"replaceonly",    	ACC_RO,
	"replace",    		ACC_RO,
	"r",    		ACC_RO,

	"rom",  		ACC_ROM,
	"replaceormake",  	ACC_ROM,
	"replacemake",  	ACC_ROM,
	"makeorreplace",  	ACC_ROM,
	"makereplace",  	ACC_ROM,
	"rm",   		ACC_ROM,

	"ao",    		ACC_AO,
	"appendonly",  		ACC_AO,
	"append",  		ACC_AO,
	"a",    		ACC_AO,

	"aom",   		ACC_AOM,
	"appendormake",		ACC_AOM,
	"appendmake",		ACC_AOM,
	"makeorappend",		ACC_AOM,
	"makeappend",		ACC_AOM,
	"am",   		ACC_AOM,

	"tji",			ACC_TJI,
	"takejobinput",		ACC_TJI,

	"tjo",			ACC_TJO,
	"takejoboutput",	ACC_TJO,

	"rar",   		ACC_RAR,
	"readandremove",	ACC_RAR,
	"readremove",		ACC_RAR,
	"removeandread",	ACC_RAR,
	"removeread",		ACC_RAR,
	"rr",   		ACC_RAR,
	"gr",   		ACC_RAR,

	"rdo",    		ACC_RDO,
	"readonly",    		ACC_RDO,
	"read",    		ACC_RDO,
	"g",    		ACC_RDO,

	"dr",	  		ACC_DR,
	"destructiveread",	ACC_DR,
	"readdestructive",	ACC_DR,
	"grd",  		ACC_DR,

	"gji",  		ACC_GJI,
	"givejobinput",		ACC_GJI,

	"gjo",  		ACC_GJO,
	"givejoboutput",	ACC_GJO,

	0,0,
	};

processargs(arg)
char    *arg;
{
	char    **strp, *getpass(), *setpasswd();
	register struct  _acc   *ap;
	struct passwd *pw;

	for(;*arg ; arg++){
		switch(*arg){

	case 'D':	if (arg[1] == '=') arg++;
			if ((arg[1] >= '0' && arg[1] <= '9') || arg[1] == '-')
				debug = atoi(arg+1);
			else	fprintf(stderr, "bad debug level (%s)\n", arg+1);
			return;
/* PHASE THESE OUT  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
	case 'o':	if(direction || access_m ){
				fprintf(stderr,"Illegal context for -o\n");
				exit(1);
			}
			direction = TRANSMIT;
			access_m = ACC_TJO;
			if(!devt)
				devt = "LP";
			break;
	case 'x':       if(direction == RECEIVE || access_m){
				fprintf(stderr,"Illegal use of -x\n");
				exit(1);
			}
			access_m = ACC_TJI;
			break;
/* PHASE THESE OUT  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

	case 'a':	if(*++arg == '=') arg++;
			if(access_m) {
				fprintf(stderr,"Access mode already set\n");
				exit(1);
			}
			for(ap = accts ; ap->amode ; ap++)
				if(!strcmp(ap->amode, arg)){
					access_m = ap->aval;
					break;
				}
			if (!access_m)
			{	int mode = 0;
				fprintf(stderr, "cpf: Invalid access %s: try one of",
					arg);
				for(ap = accts ; ap->amode ; ap++)
				{	fprintf(stderr, "%s%s",
					      (mode == ap->aval) ? " " : "\n",
						ap->amode);
					mode = ap->aval;
				}
				fprintf(stderr, "\n");
				exit(1);
			}
			if( (direction == TRANSMIT &&  (access_m&ACC_GET)) ||
			    (direction == RECEIVE  && !(access_m&ACC_GET))){
				fprintf(stderr,
			"Access mode inconsistent with transfer direction\n");
				exit(3);
			}
			return;
	case 'b':	t_flags |= BINARY_TRANSFER;
			if (arg[1] == '=') arg++;
			if (arg[1] >= '0' && arg[1] <= '9'){
				tab.bin_size = atoi(arg+1);
				if(tab.bin_size <= 0 || tab.bin_size >128*8){
					fprintf(stderr, "Bad binary size\n");
					exit(2);
				}
				return;
			}
			break;
	case 'd':	strp = &devt;				goto copy;
	case 'f':	strp = &fromperson;			goto copy;
	case 'm':	strp = &monp;				goto copy;
	case 'n':	t_flags |= NOTIFY_SUCCESS;		break;
	case 'E':	t_flags |= ENCODE_PW;			break;
	case 'p':	if (!arg[1]) no_password++;
			else
			{	if(*++arg == '=') arg++;
				arg = setpasswd(arg);   /* Kludge time */
				arg--;
				strp = &rpasswd;		goto copy;
			};					return;
	case 'q':	strp = &devq;				goto copy;
	case 's':	strp = &spco;
	copy:		if(*++arg == '=') arg++;
			if(*strp){
				fprintf(stderr,"Already got value for -%c flag\n",*(arg-2));
				exit(1);
			}
			*strp = malloc((unsigned) (strlen(arg)+1) );
			(void) strcpy(*strp,arg);
			return;
	case 't':       /* we are about to do a mail transfer */
#ifdef	MAIL
			if((pw = getpwnam(MAILuser)) == NULL) {
				fprintf(stderr, "Mail user \"%s\" does not exist\n",
					MAILuser);
				exit(2);
			}
			MAILuid = pw->pw_uid;
			if(uid && uid != MAILuid){
				fprintf(stderr,"Can't pretend to be mail (%d != %d)\n", uid, MAILuid);
				exit(2);
			}
			mail++;
			ftp = 0;
			t_flags &= ~T_TYPE;
			t_flags |= (T_MAIL|WRITE_DELETE);
			break;
#else	MAIL
			fprintf(stderr, "Mail option not compiled in\n");
			exit(1);
#endif	MAIL
	case 'u':	usage(1);				break;
	case 'w':	t_flags |= WRITE_DELETE;		break;
	case 'A':	if(aco && *aco == 0) aco = 0;
			strp = &aco;				goto copy;
	case 'F':	(void) strcpy(rflp,getpass("Remote file password: "));
			break;
	case 'J':       /* we are about to do a jtmp transfer */
#ifdef JTMP
			if((pw = getpwnam(JTMPuser)) == NULL) {
				fprintf(stderr, "JTMP user \"%s\" does not exist\n",
					JTMPuser);
				exit(2);
			}
			else
				JTMPuid = pw->pw_uid;
			if(uid && uid != JTMPuid){
				fprintf(stderr,"Can't pretend to be jtmp\n");
				exit(2);
			}
			jtmp++;
			ftp = 0;
			t_flags &= ~T_TYPE;
			t_flags |= T_JTMP|WRITE_DELETE;
			strp = &jtmpfile;
			goto copy;
#else
			fprintf(stderr, "Jtmp option not compiled in\n");
			exit(1);
#endif JTMP
	case 'N':	/* we are about to do a news transfer */
#ifdef NEWS
			if((pw = getpwnam(NEWSuser)) == NULL) {
				fprintf(stderr, "News owner \"%s\" does not exist\n",
					NEWSuser);
				exit(2);
			}
			else
				NEWSuid = pw->pw_uid;

			if(uid && uid != NEWSuid){
				fprintf(stderr,"Can't pretend to be news\n");
				exit(2);
			}
			news++;
			ftp = 0;
			t_flags &= ~T_TYPE;
			t_flags |= T_NEWS; /* NO !!!! |WRITE_DELETE; */
			break;
#else
			fprintf(stderr, "News option not compiled in\n");
			exit(1);
#endif NEWS
	case 'U':	strp = &user;			goto copy;
		}
	}
}

/*
 *      Routine to tell the ftp spooler that a job has been queued.
 *      (this should only occur when there is no daemon running)
 */

tellftspool()
{
	int     fd;
	int     ftp_pid = 0;

	if( (fd = open(NRSdspooler,0))< 0)
		ftp_pid = 0;
	else if(read(fd,(char *)&ftp_pid,sizeof(ftp_pid)) != sizeof(ftp_pid))
		ftp_pid = 0;
	(void) close(fd);
	if(!ftp_pid || kill(ftp_pid,SIGINT))    /* give it a cntrl-c */
	{	int pid, status;

		while((pid= vfork()) < 0);
		if(pid == 0){           /* kiddy time */
			execl(KILLSPOOL, KILLSPOOL, 0);
			fprintf(stderr,"Cannot execute killspool program\n");
			_exit(24);
		}
		/* parent */
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		while(wait(&status) != pid);    /* wait for it */
		if(status)
			fprintf(stderr,"Can't signal spooler\n");
		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGQUIT, SIG_DFL);

	}
}

char    *
setpasswd(str)
char    *str;
{
	char    *docrypt(), pbuf[ENOUGH];

	bzero(pbuf, sizeof pbuf);
	sprintf(pbuf, "%d.%s", uid, str);
	setcrypt();
	return(docrypt(pbuf));
}

check_host()
{
	register struct NETWORK *Np;
	char *p;

	/*
	 * look up the host in the NRS data base.
	 * first check for channels.
	 * may be @<Network>/<host> or @<Network>.<queue>/<host>
	 */
	if( (p = index(remotehost, '/')) != NULL){
		char *dot;
		if (debug > 0) printf("Remote host is `%s', so split",
			remotehost);
		*p = 0;
		if (debug > 0) printf(" net=`%s', host=`%s'", remotehost, p+1);
		dot = index(remotehost, '.');
		if (dot)
		{	struct  QUEUE   *qp;
			*dot = 0;
			if (debug > 0) printf(" net=`%s', q=`%s'",
				remotehost, dot);
			for(qp = QUEUES ; qp->Qname ; qp++)
				if(strcmp(qp->Qname, dot+1) == 0)
					break;
			if(qp->Qname == NULL) goto badhost;
			queue = qp->Qname;
		}
		for(Np = NETWORKS ; Np->Nname ; Np++)
			if(same_net(remotehost, Np->Nname, 1))	break;

		if(Np->Nname == NULL ||  /* an unknown channel */
		   Np->Nopts & N_LOCAL)
			goto badhost;
		Curnet = Np->Nname;
		if (Curchan = malloc(strlen(remotehost)+1))
			strcpy(Curchan, remotehost);
		else {	fprintf(stderr, "malloc (%d) failed in check_host\n",
					strlen(remotehost)+1);
			exit(4);
		}
		if (debug > 0) fprintf(stderr, "Set Curchan to %s (%s)\n",
			remotehost, Np->Nname);
		if (dot) *dot = '.';
		*p++ = '/';     /* put the slash back */
	}
	else
		p = remotehost;
	if (debug > 0) printf(" p=%s\n", remotehost);
	/*
	 * check to see if the host is known.
	 */
	if( (Curhost = dbase_find(p, (char *)0, 0)) != NULL)
		if(Curhost->n_context == -1) 
		{	if (debug > 0) printf("!p=%s %s\n", remotehost, Curhost->host_alias);
			return;
		}
	if (debug > 0) printf(" p=%s %s\n", remotehost, (Curhost) ? Curhost->host_alias : "<unset>");
	/*
	 * give up. Cannot find host in tables
	 */
badhost:;
	fprintf(stderr, "Unknown host %s\n", remotehost);
	exit(3);
}

get_locals(del)
{
	register FILE    *fp;
	char    nbuf[ENOUGH];   /* place to put generated local name */
	char    lbuf[LOTS];
	register char   *p;
	register c;
	char    *xptrs[5];
	int     i;

	sprintf(nbuf,"%s/%s",cur_user->pw_dir,userconfig); /* generate name */
	if((fp = fopen(nbuf,"r")) == NULL)      /* open the file */
		return;                         /* ignore if cannot find it */
	for(;;){
		int data = 1;
		for(i = 0, c = 0 ; c < 5 ; c++)
			xptrs[c] = NULL;
		p = lbuf;
		while(data) switch (c = getc(fp))
		{
		case '\n':
		case EOF:	data = 0;		break;
		case '\\':	*p++ = getc(fp);	break;
		case ':':	*p++ = 0; xptrs[i++]=p;	break;
		default:	*p++ = c;		break;
		}
		if(c == EOF)
			break;
		*p = 0;
		if(strcmp(lbuf, Curhost->host_alias)) /* not this host */
			continue;
		for(c = 0 ; c < 5 ; c++){
			if( (p = xptrs[c]) == NULL)
				break;
			if(*p == 'u' && *(p+1) == 's' && *(p+2) == '='){
				/* got the user name */
				p += 3;
				if (user)
				{	if (strcmp(user, p))
					{	c=8; break; }
				}
				else
				{	user = malloc((unsigned)(strlen(p)+1));
					if(user == NULL){
						fprintf(stderr, "Out of core\n");
						exit(1);
					}
					(void) strcpy(user, p);
				}
			}
			else if(*p == 'r' && *(p+1) == 'p' && *(p+2) == '='){
				/* got the remote password */
				p += 3;
				if(rpasswd)
				{	fprintf(stderr, "Already have password\n");
				}
				else
				{	rpasswd = malloc((unsigned) (strlen(p) + 1));
					if(rpasswd == NULL){
						fprintf(stderr, "Out of core\n");
						exit(1);
					}
					(void) strcpy(rpasswd, p);
				}
			}
		}
		if (c < 6) break;
	}
	(void) fclose(fp);
	if(del && getenv("SAVEFTP") == NULL)
		(void) unlink(nbuf);
}

/*
 * routine to re-compile the local table when out of date.
 * Call setup and then re read the local file.
 */

reget_locals()
{
	int     pid,status;

	/* first call setup to setup the local file */
	while((pid= vfork()) < 0);
	if(pid == 0){           /* kiddy time */
		char *args[10];
		int argc = 0;
		char    *p = "-setup";
		if(!Curhost->n_localhost)
			p++;
		args[argc++] = p;
		if (user)
		{	args[argc++] = "-U";
			args[argc++] = user;
		}
		args[argc++] = host_name;
		args[argc++] = (char *) 0;
		execv(SETUPPROG, args);
		fprintf(stderr,"Cannot execute setup program\n");
		_exit(24);
	}
	/* parent */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	while(wait(&status) != pid);    /* wait for it */
	if(status)
		exit(status);
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);

	get_locals(1);
}

isapplic(addr)
char    *addr;
{
	register char *p = addr;

	if(*p == 'A' && *(p+1) == 'R')
		return(1);
	while(*p)
		if(*p == '\n' && *++p == 'A' && *(p+1) == 'R')
			return(1);
		else p++;
	return(0);
}
