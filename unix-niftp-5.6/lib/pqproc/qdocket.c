#ifndef	lint			/* unix-niftp lib/pqproc/qdocket.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/qdocket.c,v 5.5 90/08/01 13:37:14 pb Exp $";
#endif	lint

#include "ftp.h"
#include "nrs.h"
#ifdef  CREATRUNC
#include <sys/file.h>
#endif	CREATRUNC
#ifndef	O_RDWR
#include <fcntl.h>
#endif	O_RDWR

/*
 * docket manipulation (q side)
 *
 * $Log:	qdocket.c,v $
 * Revision 5.5  90/08/01  13:37:14  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:50:53  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:24:54  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:33:24  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  16:05:44  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0.1.2  87/09/28  13:24:07  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:05:10  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/05/21  14:58:32  pb
 * Convert to L_LOG debug code
 * 
 * Revision 5.0  87/03/23  03:49:34  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

/*
 * generate new q docket. For use with no-resume attempts which may resume
 * if this transfer fails. Name is 'default_path/transfer_id.xxxx'
 */

newqdocket()
{
	register short  *q;
	register docketp  pdoc = &tab.udocket;
	int     fd, count = 0;
	int	oldmask = umask(002);	/* Group access */

	L_DEBUG_2(L_DOCKET, 0, "Docket directory = %s (%x)\n", DOCKETDIR,DOCKETDIR);

	do{
		sprintf(docketname,"%s/qd.%s.%d",
			DOCKETDIR,ltoa(transfer_id),++count);
	}while(access(docketname, 0) != -1);             /* got it */

	L_DEBUG_1(L_DOCKET, 0, "newqdocket %s\n", docketname);

	/* ok now open docket */
#ifdef  CREATRUNC
	/* open/creat for rd/wr */
	if( (fd = open(docketname, O_RDWR|O_CREAT|O_TRUNC, (0660))) < 0){
		L_WARN_2(L_GENERAL, 0, "Can't create docket %s (%d)\n",
			docketname, errno);
		*docketname = 0;
		umask(oldmask);
		return(-2);
	}
#else	CREATRUNC
	if( (fd = creat(docketname,(0660))) < 0)
	{	umask(oldmask);
		L_WARN_2(L_GENERAL, 0, "Can't create docket %s (%d)\n",
			docketname, errno);
		return(-2);                     /* can't create docket */
	}
	(void) close(fd);
	if((fd = open(docketname,2)) <0){        /* re-open for r/w */
		L_WARN_2(L_GENERAL, 0, "Can't re-open docket %s (%d)\n",
			docketname, errno);
		(void) unlink(docketname);
		*docketname =0;
		umask(oldmask);
		return(-2);
	}
#endif	CREATRUNC
	umask(oldmask);
	for(q= (short *)pdoc ; q < (short *)&pdoc->rname[ENOUGH];)
		*q++ = 0;                               /* zero the docket */

	/* fill in all neccasary info */

	(void) strcpy(pdoc->tname, localname);
	(void) strcpy(pdoc->rname, realname);
	(void) strcpy(pdoc->hname, hostname);
	L_LOG_3(L_DOCKET, 0, "docket has hostname=%s, localname=%s, realname=%s\n",
		pdoc->hname, pdoc->tname, pdoc->rname);
	pdoc->transfer_id = transfer_id;
	pdoc->uid = uid;
	pdoc->gid = gid;
	if(write(fd,(char *)pdoc,sizeof(struct docket)) !=
							sizeof(struct docket)){
		L_WARN_2(L_GENERAL, 0, "Can't write docket %s (%d)\n",
			docketname, errno);
		(void) close(fd);       /* write out the docket */
		(void) unlink(docketname);
		*docketname = 0;
		return(-2);
	}
	(void) lseek(fd,0L,0);                  /* seek back to start */
	docket_fd = fd;
	return(0);
}

/*
 * routine to find an old docket. Must search directory and try to mach
 * it.
 */

oldqdocket()
{
	register struct  docket   *mydoc = &tab.udocket;
	register struct direct *dp;
	register DIR *dirp;
	int     nfd;
	char    *rindex();
	register char    *q;
	int     found,partsiz;

	L_DEBUG_2(L_DOCKET, 0, "Docket directory = %s (%x)\n", DOCKETDIR,DOCKETDIR);

	if( (dirp = opendir(DOCKETDIR)) == NULL)	      /* open it */
	{	L_WARN_2(L_GENERAL, 0, "No docket directory %s (%d)\n",
				DOCKETDIR, errno);
		return(-2);                             /* can't ??? */
	}

	/* generate the name we are trying to match */
	sprintf(docketname, "%s/qd.%s.", DOCKETDIR, ltoa(transfer_id) );
	q = docketname + strlen(DOCKETDIR) + 1; /* point at local part */
	partsiz = strlen(q);		     /* get its size */
	found = 0;
	L_LOG_3(L_DOCKET, 0, "Look for %s (%s) for %s\n",
		q, docketname, hostname);
	for(dp=readdir(dirp) ; dp != NULL ; dp = readdir(dirp))
		if(dp->d_namlen > partsiz && !strncmp(dp->d_name,q,partsiz)){
			(void) strcpy(q,dp->d_name);  /* got it. Fill it in */
			nfd = open(docketname,2);        /* open the docket */
			L_LOG_1(L_DOCKET, 0, "Docketname = %s\n",docketname);
			if(nfd < 0)                     /* can't oh well */
			{	L_LOG_1(L_DOCKET, 0, "open error %d\n", errno);
				continue;
			}
			if(read(nfd, (char *)mydoc, sizeof(* mydoc))
							!= sizeof(* mydoc)){
				(void) close(nfd);      /* read it in... */
				L_LOG_1(L_DOCKET, 0, "read error %d\n", errno);
				continue;               /* nope */
			}
			if(!strcmp(mydoc->hname,hostname)){
						/* docket has correct */
				found++;        /* host addres. Great we */
				break;          /* have the docket */
			}
			L_LOG_1(L_DOCKET, 0, "Wrong host: %s\n", mydoc->hname);

		}
		else	L_LOG_1(L_DOCKET, 0, "Ignore %s\n", dp->d_name);
	
	closedir(dirp);
	if(!found){             /* can't find the docket */
		L_LOG_1(L_DOCKET, 0, "No docket for %s\n", hostname);
		(void) close(nfd);
		return(-1);
	}

	docket_fd = nfd;         /* got it so set up flags */
	(void) lseek(nfd,0L,0);
	return(0);
}

/* read any docket */

/* ARGSUSED ????????????? */
readdocket(pd)
docketp  pd;
{
	register docketp pdoc = &tab.udocket;

	L_DEBUG_1(L_DOCKET, 0, "read docket %s\n", docketname);

	if(read(docket_fd, (char *)pdoc, sizeof(* pdoc))!= sizeof(* pdoc)){
		L_WARN_2(L_GENERAL, 0, "Can't read docket (%d)\n",
			docketname, errno);
		(void) close(docket_fd);
		docket_fd = -1;
		*docketname =0;
		L_WARN_1(L_GENERAL, 0, "Failed to read docket data (%d)\n",
			errno);
		return(-2);
	}
	(void) lseek(docket_fd, 0L, 0);  /* seek back to start for latter */
	lastmark	= pdoc->last_mark;
	rec_mark	= pdoc->rec_mark;
	last_rlen	= pdoc->last_rlen;
	last_count	= pdoc->last_count;
	lr_bcount	= pdoc->lr_bcount;
	lr_reclen	= pdoc->lr_reclen;
	transfer_id	= pdoc->transfer_id;
	t_st_of_tran	= pdoc->st_of_tran;
	t_uid		= pdoc->uid;
	t_gid		= pdoc->gid;
	L_LOG_2(L_DOCKET, 0, "docket has localname=%s, realname=%s\n",
		pdoc->tname, pdoc->rname);
	L_LOG_4(L_DOCKET, 0, "  last_rec %d, rec_mark %d, id %d, uid %d\n",
		lastmark, rec_mark, transfer_id, t_uid);
	(void) strcpy(localname, pdoc->tname);
	(void) strcpy(realname, pdoc->rname);
	return(0);
}

/* write the docket file */

writedocket()
{
	register struct  docket   *pdoc = &tab.udocket;

	L_DEBUG_2(L_DOCKET, 0, "%swrite docket %s\n", 
		(docket_fd == -1) ? "no fd to " : "", docketname);

	if (docket_fd == -1) return 0;	/* ignore if not got anything */

	pdoc->last_mark		= lastmark;
	pdoc->rec_mark		= rec_mark;
	pdoc->last_rlen		= last_rlen;
	pdoc->last_count	= last_count;
	pdoc->lr_bcount		= lr_bcount;
	pdoc->lr_reclen		= lr_reclen;
	pdoc->transfer_id	= transfer_id;
	pdoc->st_of_tran	= st_of_tran;
	pdoc->uid		= uid;
	pdoc->gid		= gid;
	(void) strcpy(pdoc->tname, localname);
	(void) strcpy(pdoc->rname, realname);
	(void) strcpy(pdoc->hname, hostname);
	if(write(docket_fd,(char *)pdoc ,sizeof(* pdoc) ) != sizeof(* pdoc))
	{	L_WARN_2(L_GENERAL, 0, "Can't write docket %s (%d)\n",
			docketname, errno);
		return(-1);
	}
	(void) lseek(docket_fd, 0L, 0);
	return(0);
}

/*
 * delete the docket for - if fl is set don't delete docket
 */

deldocket(fl)
int     fl;
{
	L_DEBUG_2(L_DOCKET, 0, "delete docket %s (%d)\n", docketname, fl);
	if(*docketname && !fl)
		(void) unlink(docketname);
	*docketname=0;
	if(docket_fd != -1){
		(void) close(docket_fd);
		docket_fd = -1;
	}
}
