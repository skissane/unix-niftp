/* unix-niftp lib/gen/hash.h $Revision: 5.5 $ $Date: 90/08/01 13:34:39 $ */
/*
 * hash.h
 *
 * $Log:	hash.h,v $
 * Revision 5.5  90/08/01  13:34:39  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:44:16  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:36:03  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#define PBLKSIZ 1024
#define DBLKSIZ 8192
#define BYTESIZ 8
#define NULL    ((char *) 0)

long    bitno;
long    blkno;
long    hmask;

typedef struct  {
	short   dirf;
	short   pagf;
	short   dbrdonly;
	long    maxbno;
	long    pagbno;
	long    dirbno;
	char    pagbuf[PBLKSIZ];
	char    dirbuf[DBLKSIZ];
} dbase;

typedef struct
{
	char    *dptr;
	int     dsize;
} datum;

dbase   *cdbptr;

datum   hashfetch();
datum   makdatum();
datum   hashfirstkey();
datum   hashnextkey();
datum   firsthash();
long    calchash();
long    hashinc();
