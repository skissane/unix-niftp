#include "opts.h"
#include "nrs.h"
#include <signal.h>

main()
{
	int     fd;
	int     ftp_pid = 0;

	if(nrs_init() < 0) {
		fprintf(stderr, "Cannot initialise. Consult an expert\n");
		exit(2);
	}

	if( (fd = open(NRSdspooler,0))< 0)
		ftp_pid = 0;
	else if(read(fd,(char *)&ftp_pid,sizeof(ftp_pid)) != sizeof(ftp_pid))
		ftp_pid = 0;
	(void) close(fd);
	if(!ftp_pid || kill(ftp_pid,SIGINT))    /* Wakeup & look round! */
		exit(1);
	exit(0);
}
