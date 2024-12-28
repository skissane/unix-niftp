/*
 * IO is a bit strange in that the first byte of each io
 * is status info rather than data. We have to be a bit
 * carefull in leaving sufficeint space to avoid excess copies.
 */

#define BLOCKMAX	(1+128) /* high water level for IO */
#define BLOCKMIN	1	/* low water level for IO */

extern struct sockaddr_yb syb;
extern int    syblen;
extern char *yb_error();

#define MBIT		0x40
/*
 * This file contains routines to interface to the nextwork. They
 * make the network seem like a normal file. Makes the interface nicer
 */

extern int socket_fd;
