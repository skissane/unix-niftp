#ifndef	lint			/* unix-niftp lib/pqproc/pdocket.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/pdocket.c,v 5.6 1991/06/07 17:01:48 pb Exp $";
#endif	lint

#define	risk_docket
#include "ftp.h"

/*
 * This file contains all the routines to manipulate dockets for use by
 * the P side. The docket structure is the same on both sides but it is
 * used slightly differently. ( hname + t_uid + t_gid are only used by Q.)
 *      This code is used when using resumptions to store information about
 * an attempted transfer which failed in the middle.
 *
 * $Log: pdocket.c,v $
 * Revision 5.6  1991/06/07  17:01:48  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:00  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:24:59  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:08:00  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:00:41  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:49:24  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 */

/* open or creat a docket - as need be - when starting */

getpdocket()
{
	long    l;

	time((int *) &l);
	transfer_id = l;                /* unique number */
	try_resuming = 0;               /* say we can't yet resume */
	may_resume = 0;
	if(tab.l_docket){                /* The docket number. Maybe we will */
		L_LOG_2(L_DOCKET, 0, "Existing docket %s for %d\n",
			tab.udocket.tname, tab.l_docket);
		try_resuming = 1;       /* going to try to resume */
#ifdef	risk_docket	/* resume at last mark in transfer - some bugs yet */
		(void) readdocket();
#else			/* resume from begining */
		transfer_id = tab.l_docket;
		(void) strcpy(localname, tab.udocket.tname);
#endif
	}
	else {                                  /* a new transfer */
		L_LOG_3(L_DOCKET, 0, "New docket for %d: %s & %s\n",
					transfer_id, localname, realname);
		tab.l_docket = l;
		bzero ((char *)&tab.udocket, sizeof (tab.udocket));
						/* set all needed values */
		(void) strcpy(tab.udocket.tname,localname);
		(void) strcpy(tab.udocket.rname,realname);
		tab.udocket.transfer_id = transfer_id;
	}
}

/* read a docket and fill out any neccasary data */

readdocket()
{
	register docketp pdoc	= &tab.udocket;

	lastmark	= pdoc->last_mark;     /* recover all variables */
	rec_mark	= pdoc->rec_mark;
	last_rlen	= pdoc->last_rlen;
	last_count	= pdoc->last_count;
	lr_bcount	= pdoc->lr_bcount;
	lr_reclen	= pdoc->lr_reclen;
	transfer_id	= pdoc->transfer_id;
	t_st_of_tran	= pdoc->st_of_tran;
	t_uid		= pdoc->uid;
	t_gid		= pdoc->gid;
	(void) strcpy(localname, pdoc->tname);
	(void) strcpy(realname, pdoc->rname);
	L_LOG_2(L_DOCKET, 0, "docket has localname=%s, realname=%s\n",
		pdoc->tname, pdoc->rname);
	L_LOG_4(L_DOCKET, 0, "  last_rec %d, rec_mark %d, id %d, uid %d\n",
		lastmark, rec_mark, transfer_id, t_uid);
	return(0);
}

/* write a docket into the docket file. */

writedocket()
{
	register docketp pdoc = &tab.udocket;

	if(!tab.l_docket)                /* file does not exist */
	{	L_LOG_0(L_DOCKET, 0, "No docket\n");
		return(0);
	}

	pdoc->last_mark		= lastmark;      /* copy out all values */
	pdoc->rec_mark		= rec_mark;
	pdoc->last_rlen		= last_rlen;
	pdoc->last_count	= last_count;
	pdoc->lr_bcount		= lr_bcount;
	pdoc->lr_reclen		= lr_reclen;
	pdoc->transfer_id	= transfer_id;
	pdoc->st_of_tran	= st_of_tran;
	pdoc->uid		= uid;
	pdoc->gid		= gid;
	(void) strncpy(pdoc->tname, localname, sizeof( pdoc->tname ) -1 );
	(void) strncpy(pdoc->rname, realname,  sizeof( pdoc->rname ) -1 );
	pdoc->tname[ sizeof( pdoc->tname ) -1 ] = '\0';
	pdoc->rname[ sizeof( pdoc->rname ) -1 ] = '\0';
	if(write(qfd, (char *)&tab, sizeof(tab) ) != sizeof(tab))
	{	L_LOG_1(L_DOCKET, 0, "docket write failed %d\n", errno);
		return(-1);
	}
	else L_LOG_0(L_DOCKET, 0, "Wrote docket OK\n");

	lseek(qfd,0L,0);                /* seek back for furthur use */
	return(0);
}

/*
 * close the docket. if fl is set don't delete it.
 */

deldocket(fl)
int     fl;
{
	L_LOG_1(L_DOCKET, 0, "Close docket (%d)\n", fl);
	if(!fl)                 /* delete the docket */
		tab.l_docket = 0;
}
