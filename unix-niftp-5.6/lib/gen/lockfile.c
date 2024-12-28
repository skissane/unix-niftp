/* unix-niftp lib/gen/lockfile.c $Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/lockfile.c,v 5.6 1991/06/07 17:00:58 pb Exp $ */
/*
 * $Log: lockfile.c,v $
 * Revision 5.6  1991/06/07  17:00:58  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:46  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:16:56  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * Lock an open (for write if fcntl) file.
 */

#include "opts.h"

#ifdef  FLOCK
#include <sys/file.h>
#endif	FLOCK

#ifdef  FCNTL
#include <fcntl.h>
#endif	FCNTL

/* ARGSUSED */
lockfile3(fd, byte, block)
long byte;
{
#ifdef	FCNTL
	struct flock lock;
#endif	FCNTL

#ifdef	FCNTL
	lock.l_type		= F_WRLCK;
	lock.l_start		= byte;
	lock.l_len		= 0L;
	lock.l_pid		= getpid();
	lock.l_whence		= 0;

	if (fcntl(fd, (block ? F_SETLKW : F_SETLK), &lock)) return -1;
	return 0;
#else	FCNTL
#ifdef	FLOCK
	if (flock(fd, (block ? 0 : LOCK_NB) | LOCK_EX)) return -1;
	return 0;
#else	FLOCK
	return -1;
#endif	FLOCK
#endif	FCNTL
}

/* ARGSUSED */
unlockfile2(fd, byte)
long byte;
{
#ifdef	FCNTL
	struct flock lock;
#endif	FCNTL

#ifdef	FCNTL
        lock.l_type     = F_UNLCK;
        lock.l_start    = byte;
        lock.l_len      = 0L;
        lock.l_whence   = 0;

        fcntl(fd, F_SETLK, &lock);
	return;
#endif	FCNTL
#ifdef  FLOCK
	flock(fd, LOCK_UN);
	return;
#endif  FLOCK
}

/* Old versions .... */

lockfile(fd)
{	return	lockfile3(fd, 0L, 1);
}

unlockfile(fd)
{	unlockfile2(fd, 0L);
}
