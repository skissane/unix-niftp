/* unix-niftp lib/gen/getpass.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:11 $ */
/*
 * Our own version of getpass.
 * The C library version only returns the first 8 characters,
 * this is inapropriate for collecting passwords for non-unix machines
 *
 * wja@cs.nott.ac.uk
 *
 * $Log: getpass.c,v $
 * Revision 5.6  1991/06/07  17:01:11  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:32  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:14:44  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:33:31  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.2  87/09/28  13:07:25  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:59:56  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:35:54  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#include "opts.h"
#ifndef	SIGRET
#define	SIGRET	void
#endif	/* SIGRET */

/* Should already be in signal.h extern SIGRET void    (*signal())(); */
extern char *index();

char *
getpass(message)
char *message;
{
	int fd, len, i;

	char *p;
	struct sgttyb old, new;
	static char passwdbuf[128];
	SIGRET (*oldint)();

	passwdbuf[0] = '\0';

	/* grab controlling tty */
	if ((fd = open("/dev/tty", 2)) < 0)
		fd = fileno(stdin);

	/* block interupt while messing with tty modes */
	oldint = signal(SIGINT, SIG_IGN);

	/* turn off echoing */
	ioctl(fd, TIOCGETP, &old);
	new = old;
	new.sg_flags &= ~ECHO;
	ioctl(fd, TIOCSETP, &new);

	/* put up prompt */
	len = strlen(message);
	write((fd == fileno(stdin)) ? fileno(stderr) : fd, message, len);

	/* read password - line or passwdbuffs worth */
	len = sizeof(passwdbuf) - 1;
	p = passwdbuf;

	while (len > 0 && (i = read(fd, p, len)) > 0) {
		if (index(p, '\n'))
			break;
		len -= i;
		p += i;
	}
	if ((p = index(passwdbuf, '\n')) != NULL)
		*p = '\0';
	write((fd == fileno(stdin)) ? fileno(stderr) : fd, "\n", 1);

	/* restore modes - this should zap any outstanding input */
	ioctl(fd, TIOCSETP, &old);
	if (fd != fileno(stdin)) close(fd);
	signal(SIGINT, oldint);

	return(passwdbuf);
}
