#ifndef	lint			/* unix-niftp bin/ftp.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/bin/RCS/ftpreq.c,v 5.5 90/08/01 13:32:03 pb Exp Locker: pb $";
#endif	lint

#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include "retry.h"
#include "nrs.h"
#include "files.h"
#include "jtmp.h"
#include "../version.h"
#include <stdio.h>
#ifdef	sun
# include <sys/file.h>
#endif	sun

char	optargs[] = "irk:z";
extern	int optind;
extern	char *optarg;

#define STRUCTTABSIZE   sizeof(struct tab)
long stt;
int keeptime = 60*60;
int zap = 0;
int remove = 0;
int info = 0;

extern char *rindex();

main(argc, argv)
char **argv;
{	int c;

	while ((c = getopt(argc, argv, optargs)) != EOF) switch(c)
	{
	default:
		fprintf(stderr, "%s: bad args for: %s [%s] files ...\n",
			*argv, *argv, optargs);
		exit(1);
	case 'k': keeptime = atoi(optarg);			break;
	case 'i': info++;					break;
	case 'r': remove++;					break;
	case 'z': zap++;					break;
	}

	(void) time(&stt);

	if (!zap && !remove && !info)
	{	fprintf(stderr,
"Specify -z to zap the retry times, -r to remove old Q files, or -i[i] for info\n");
		/* exit(1); */
	}

	if (optind < argc) while (optind < argc) process(argv[optind++]);
	else
	{	fprintf(stderr, "%s: You must give some file names\n", *argv);
		exit(1);
	}

	exit (0);
}

char *
how_long(left)
int left;
{	static char when[128];
	when[0] = '-';
	if (left < 0) left = -left;
	else when[0] = ' ';
	if (left < (10 * 60))
		sprintf(when+1, "%3ds", left);
	else if (left < (10 * 60 * 60))
		sprintf(when+1, "%3dm", left/60);
	else if (left < (2 * 24 * 60 * 60))
		sprintf(when+1, "%3dh", left/(60*60));
	else sprintf(when+1, "%3dd", left/(24*60*60));
	return when;
}

process(ffile)
char *ffile;
{	int fd = open(ffile, (zap) ? 2 : 0);
	int nb;
	char dummy;
	struct tab tab;
	struct  stat    statbuf;
	char *file = rindex(ffile, '/');

	if (!file) file = ffile;
	else file++;

	if (fd < 0)
	{	fprintf(stderr, "Failed to open %-40s - %d\n", ffile, errno);
		return -1;
	}

	if (strcmp(file, "bcont") == 0) 
	{	display_bcont(fd, ffile);
		close(fd);
		return 0;
	}

	if ((nb = read(fd, (char *)&tab, STRUCTTABSIZE)) != STRUCTTABSIZE)
	{	if (nb <=0)
			fprintf(stderr, "Failed to read %-40s %d\n", ffile, errno);
		else	fprintf(stderr, "%-40s is only %d bytes\n", ffile, nb);
		close(fd);
		return -2;
	}
	if (read(fd, &dummy, sizeof dummy) != 0)
	{	fprintf(stderr, "%-40s is too large for a Q file\n", ffile);
		close(fd);
		return -3;
	}

	switch(tab.status){
	case XDONESTATE:
	case DONESTATE:
	case REJECTSTATE:
	case ABORTSTATE:
	case CANCELSTATE:
		if (!remove);
		else if(fstat(fd, &statbuf) < 0)
			fprintf(stderr, "Failed to fstat %-40s %d\n",
				ffile, errno);
		else if(statbuf.st_mtime + keeptime < stt){
#ifdef JTMP
			if((tab.t_flags & T_TYPE) == T_JTMP && tab.l_jtmpname)
			{	/* we have a jtmp transfer. delete any old
				 * files associated with it.
				 */
				hnm = (char *)&tab + tab.l_jtmpname;
				if (tab.status!=DONESTATE) (void) unlink(hnm);
			}
#endif JTMP
			fprintf(stderr, "Delete %40s - %d\n",
				ffile, stt- statbuf.st_mtime);
			(void) unlink(ffile);
		}
		break;
	default:
		if (zap)
		{	fprintf(stderr, "Zap timeout for %-10s (was %5d) to %s\n",
				file, tab.l_nextattmpt - stt,
				(tab.l_hname) ? (tab.l_hname + (char *)&tab) :
						"<unknown>");
			lseek(fd, 0L, 0);
			tab.l_nextattmpt = stt;
			write(fd, (char *)&tab, STRUCTTABSIZE);
		}
		if (info) fprintf(stderr, (info == 1) ?
						"%-10s %.0sfor %-18s %s\n" :
						"%-10s %s for %-18s %s\n",
			file,
			how_long(tab.l_nextattmpt - stt),
			(tab.l_hname) ? (tab.l_hname + (char *)&tab) : "<unknown>",
			(tab.l_fil_n) ? (tab.l_fil_n + (char *)&tab) : "<unknown>");
	}
	close(fd);
}

display_bcont(fd, file)
int fd;
char *file;
{	struct bfile Bfile;
	register struct backoff *bp;
	char *slash = rindex(file, '/');
	char dummy;
	int mod = 0;
	int some = 0;

	if (slash) *slash = '\0';
	if (slash) slash = rindex(file, '/');

	if (remove)
	{	printf("remove makes little sense for a bcont file\n");
		if (!info && !zap) return;
	}

	if(read(fd, (char *)&Bfile, sizeof(Bfile))!=sizeof(Bfile)){
		printf("Read failure on bcont\n");
		return;
	}
	if (read(fd, &dummy, sizeof dummy) != 0)
	{	fprintf(stderr, "bcont file is too large\n");
		return;
	}

	for(bp = Bfile.b_hosts ; bp < &Bfile.b_hosts[MAXBHOSTS];bp++)
		if(bp->b_host[0])
		{	if (info) 
			{	int left = bp->b_hc.h_nextattempt - stt;
				if (!some) fprintf(stderr,
					"\tbcont%s%s contains:\n",
					(slash) ? " for " : "",
					(slash) ? slash+1 : "");
				some++;
				fprintf(stderr, "%-18s: %s\n",
					bp->b_host, how_long(left));
			}
			if (zap) { bp->b_hc.h_nextattempt = stt; mod++; }
		}

	if (info && !some) fprintf(stderr, "\tbcont%s%s blocks no hosts\n",
			(slash) ? " for " : "",
		(slash) ? slash+1 : "");
	if (mod)
	{	lseek(fd, 0, 0);
		if (write(fd, (char *)&Bfile, sizeof(Bfile)) != sizeof Bfile)
			fprintf(stderr, "Failed to write  bcont\n");
	}
}
