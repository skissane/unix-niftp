/* unix-niftp h/db.h $Revision: 5.5 $ $Date: 90/08/01 13:33:01 $ */
/*
 * standard header file for the NRS db routines
 *
 * $Log:	db.h,v $
 * Revision 5.5  90/08/01  13:33:01  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:40:19  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:25:34  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#define DBSEP   '\034'

extern  int     _dbase_inuse;

typedef struct
{
	char    *dptr;
	int     dsize;
} datum;

datum   fetch();
datum   makdatum();
datum   firstkey();
datum   nextkey();
datum   firsthash();
long    calchash();
long    hashinc();
