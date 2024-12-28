/* unix-niftp bin/fixtime.c $Revision: 5.5 $ $Date: 90/08/01 13:30:12 $ */
/*
 * Ftp queue fixing program - sets the time for each entry
 * kludged from ftpq.
 *
 * $Log:	fixtime.c,v $
 * Revision 5.5  90/08/01  13:30:12  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:32:23  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:39  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "opts.h"
#include "tab.h"
#include "files.h"
#include "nrs.h"

struct  tab     tab;

#include <sys/stat.h>

int     domail;         /* print mail transfers */

char    *queue;
char    **host;
long    tt;

main(argc,argv)
char    **argv;
int     argc;
{
	register char   *p;
	char    *rindex();

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise. Consult an expert\n");
		exit(2);
	}
	if(getuid() != 0){
		fprintf(stderr, "No permissions\n");
		exit(1);
	}
	time(&tt);
	/* first process any flags */

	for(argv++, argc-- ; argc && **argv == '-'; argv++, argc--)
		for(p = *argv + 1 ; *p ; p++){
			switch(*p){
			case 'm':
			case 'M':
				domail++;
				break;
			case 'Q':
			case 'q':
				if(*(p+1)){
				       printf("Bad flag placement (%c)\n",*p);
					break;
				}
				if(argc < 2){
					printf("Queue name required\n");
					break;
				}
				if(queue){
					printf("Queue already specified\n");
					break;
				}
				if((queue = rindex(*++argv, '/')) == NULL)
					queue = *argv;
				else queue++;
				argc--;
				break;
			default:
				printf("Unknown flag (%c)\n",*p);
				break;
			}
		}

	if(argc > 0 && !queue)
		host = argv;
	if(!host && !queue){
		printf("Must specify either host or queue\n");
		exit(1);
	}

	fixit();
}

char    *Lhost;         /* Name of host in line */

fixit()
{
	DIR     *dirp;
	register struct  QUEUE   *Qp;
	register char    *q;
	char    xbuff[ENOUGH];

	/*
	 * now check hosts if we want them
	 */
	if(host != NULL)
		host_expand();

	for(Qp = QUEUES ; Qp->Qname ; Qp++){
		if(queue && strcmp(queue, Qp->Qname))
			continue;
		if( (q = Qp->Qdir) == NULL)
			q = Qp->Qname;
		if(*q != '/'){
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
			q = xbuff;
		}
		if(chdir(q) < 0 || (dirp = opendir(".")) == NULL){
			printf("Cannot access queue %s\n", Qp->Qname);
			continue;
		}
		printf("Searching queue %s\n", q);
		fflush(stdout);
		fixdir(dirp);
		(void) closedir(dirp);
	}
}

/* print out entries in the queue. Do this by searching the directory
 * then sorting the elements
 */

fixdir(dirp)
register DIR     *dirp;
{
	register struct direct *dp;
	register fd;
	char    *strcpy();
	char    *hp;

	/* First read in the directory */
	for(dp = readdir(dirp) ; dp ; dp = readdir(dirp) ){
		if(*dp->d_name != 'q')  /* ignore . and .. */
			continue;
		if((fd = open(dp->d_name, 2)) < 0 ||
			   read(fd,(char *)&tab,sizeof(tab))!=sizeof(tab)){
			(void) close(fd);
			continue;
		}
		hp = (char *)&tab + tab.l_hname;
		if(host && strcmp(Lhost, hp)){
			(void) close(fd);
			continue;
		}
		switch(tab.status){
		case DONESTATE:
		case ABORTSTATE:
		case REJECTSTATE:
		case CANCELSTATE:
			(void) close(fd);
			continue;
		default:
			break;
		}
		if(!domail && tab.r_usr_n == 0){
			(void) close(fd);
			continue; /* dont print mail unless required */
		}
		tab.t_queued = tt;
		printf("Fixing entry %s (%s)\n", dp->d_name, hp);
		fflush(stdout);
		lseek(fd, 0L, 0);
		if(write(fd, (char *)&tab, sizeof(tab)) != sizeof(tab)){
			(void) close(fd);
		    fprintf(stderr, "Write error on entry %s\n",dp->d_name);
			exit(1);
		}
		(void) close(fd);
	}
}

host_expand()
{
	struct host_entry       *hp;

	if((hp = dbase_find(*host, (char *)0, 0)) != NULL){
		Lhost = hp->host_alias;
		return;
	}
	printf("Unknown host %s\n", *host);
	exit(1);
}
