/* unix-niftp lib/gen/getwd.c $Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/getwd.c,v 5.6 1991/06/07 17:00:48 pb Exp $ */
/*
 * $Log: getwd.c,v $
 * Revision 5.6  1991/06/07  17:00:48  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:34  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:44:10  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * Get the working directory
 */

#include "opts.h"

#ifndef	GETWD
char *
getwd(result)
char *result;
{
	int     pipes[2];
	int     pid;
	int     status,i;

	(void) pipe(pipes);     /* create a pipe to pwd */

#ifdef  VFORK
	while((pid = vfork()) < 0)
#else
	while((pid = fork()) < 0)       /* Now get child */
#endif
		sleep(2);

	if(pid == 0 ){                  /* kiddy time */
		(void) close(pipes[0]); /* set up the pipe */
		(void) close(1);
		(void) dup(pipes[1]);
		(void) close(pipes[1]);
					/* now do the exec */
#ifdef  UCL
		execl("/bin/pwd","pwd",0);
		execl("/usr/bin/pwd","pwd",0);
#else
		execlp("pwd","pwd",0);
#endif
		_exit(-1);
	}
					/* the parent */
	(void) close(pipes[1]);
	while(wait(&status)!= pid);     /* wait for it */

	i = read(pipes[0], result, 256);
	(void) close(pipes[0]);
	if(i <= 0 || *result != '/' || status)
		return (char *) 0;
	result[i-1] = 0;
	return result;

}
#endif	GETWD
