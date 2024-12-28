static char sccsid[] = "@(#)opendir.c 4.3 8/4/82";

#include <sys/types.h>
#include <sys/stat.h>
#include <ndir.h>
char *malloc();	/* UKfix 017: LMCL: Missing type. Found by jdb@ukc.UUCP */

/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	struct stat sbuf;

	if ((fd = open(name, 0)) == -1)
		return NULL;
	fstat(fd, &sbuf);
	if (((sbuf.st_mode & S_IFDIR) == 0) ||
	    ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL)) {
		close (fd);
		return NULL;
	}
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return dirp;
}
