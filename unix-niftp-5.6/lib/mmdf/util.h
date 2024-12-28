/* unix-niftp lib/mmdf/util.h $Revision: 5.6 $ $Date: 1991/06/07 17:01:17 $ */

/*
 *	 file:  util.h
 * some mmdf definitions used by mail interface routines
 * last changed 27-feb-84
 * $Log: util.h,v $
 * Revision 5.6  1991/06/07  17:01:17  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:28  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:53  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:46:12  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

#define NOTOK   -1
#define OK      0
#define NO      0

#define HIGHFD 15       /* max no of open files - high file descriptor*/

#define BUFSIZE 256     /* buffer for reading/writing file */

union pipunion
{
	int pipcall[2];
	struct pipstruct
	{
		int prd;
		int pwrt;
	} pip;
};
typedef union pipunion Pip;
