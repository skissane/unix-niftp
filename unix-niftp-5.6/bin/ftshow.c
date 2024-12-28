#ifndef	lint			/* unix-niftp bin/ftshow.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/bin/RCS/ftshow.c,v 5.5 90/08/01 13:31:28 pb Exp $";
#endif	lint

/*
 * Display the state of a P or Q process.
 *
 * $Log:	ftshow.c,v $
 * Revision 5.5  90/08/01  13:31:28  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.0  87/09/28  13:37:02  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:21:21  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/07/12  13:48:36  pb
 * *** empty log message ***
 * 
 */

#include "ftp.h"
#include "stat.h"

char * rindex();
char	optargs[] = "q";
extern	int optind;
extern	char *optarg;

int	quick	= 0;

main(argc, argv)
char **argv;
{
	int c;

	while ((c = getopt(argc, argv, optargs)) != EOF) switch(c)
	{
	default:
		fprintf(stderr, "%s: bad args for: %s [%s] files ...\n",
			*argv, optargs);
		exit(1);
	case 'q': quick++;						break;
	}

	if (optind < argc)
	{
		while (optind < argc) process(argv[optind++]);
	}
	else
	{
		fprintf(stderr, "%s: You must give some file names\n", *argv);
		exit(1);
	}

	exit (0);
}

process(file)
char *file;
{
	int fd;
	struct	s_stat	s_stat;
	struct	stat	stat;
	long	curr_time;
	char	*state;
	char	buff_[20];
	char	*end = file;
	char	*p;
	char	*name, *addr;

	time(&curr_time);

	if ((fd = open(file, 0)) < 0)
	{	fprintf(stderr, "Open failed on ");
		perror(file);
		return -1;
	}

	if (read(fd, &s_stat, sizeof s_stat) != sizeof s_stat)
	{	fprintf(stderr, "Read failed on ");
		perror(file);
		close(fd);
		return -1;
	}

	fstat(fd, &stat);

	switch (s_stat.s_stat)
	{
	default:
		sprintf(buff_, "st%-4d", s_stat.s_stat);
		state = buff_;
		break;
	case S_DEAD:		state = "DEAD";		break;
	case S_LISTEN:		state = "LISTEN";	break;
	case S_OPENING:		state = "OPENING";	break;
	case S_DECODING:	state = "DECODING";	break;
	case S_IDLE:		state = "IDLE";		break;
	case S_ERRCLOSE:	state = "ERRCLOSE";	break;
	case S_LISTENFAILED:	state = "LISTENFAILED";	break;
	case S_SFT:		state = "SFT";		break;
	case S_DATA:		state = "DATA";		break;
	case S_DONE:		state = "DONE";		break;
	case S_AWAITSTOP:	state = "AWAITSTOP";	break;
	case S_PROCESS:		state = "PROCESS";	break;
	case S_PROCESSED:	state = "PROCESSED";	break;
	case S_OPENFAILED:	state = "OPENFAILED";	break;
	case S_FINDP:		state = "FINDP";	break;
	case S_FOUNDP:		state = "FOUNDP";	break;
	case S_RELISTEN:	state = "RELISTEN";	break;
	case S_FAILISTEN:	state = "FAILISTEN";	break;
	}

	if (p = rindex(end, '/')) end = p+1;
	if (p = rindex(end, '.')) end = p+1;

	name = s_stat.s_name;
	addr = s_stat.s_addr;

	if (*addr == '<') addr = "";
	if (*name == '<') name = "";

	if (quick)
	{	if (*addr) name = ""; }
	else	if (*name) addr = "";


	printf("%-6.6s p%-5d t%-4d/%-4d v%-6d %-6.6s %s%s%s",
		end,
		s_stat.s_pid,
		curr_time - s_stat.s_time,
		curr_time - stat.st_mtime,
		s_stat.s_val,
		state,
		addr,
		*addr ? "" : "=",
		name);

	if (*(s_stat.s_serv) != '<')
		printf(" (%s)", s_stat.s_serv);
	printf("\n");

	return 0;
}
