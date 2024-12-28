/* unix-niftp lib/gen/db.c $Revision: 5.5 $ $Date: 90/08/01 13:34:10 $ */
/*
 * start and end routines for the NRS database
 *
 * $Log:	db.c,v $
 * Revision 5.5  90/08/01  13:34:10  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:42:55  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:35:22  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

extern  char    *NRSdbase;
int     _dbase_inuse;

dbase_start()
{
	if(dbminit(NRSdbase) < 0)
		return(-1);
	_dbase_inuse = 1;
	return(0);
}

dbase_end()
{
	if(_dbase_inuse)
		dbmclose();
	_dbase_inuse = 0;
}
