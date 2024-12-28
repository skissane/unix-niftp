#ifndef	lint		/* unix-niftp lib/gen/ni_rtn.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/ni_rtn.c,v 5.6 1991/06/07 17:01:01 pb Exp $";
#endif	lint

#include "ftp.h"
#include "nrs.h"

#define OK	0
#define	NOTOK	-1
#define	MAXLINES ((32 * 1024) -1)	/* How many lines of message ? */

extern	int ftp_print;
extern	int errno;
extern	char escape_double[];
extern	char *rindex(), *index ();
extern	char *ctime ();

/*
 *  NI_RTN  --  For NIFTP to send informative messages to a user
 *
 *	Parameters:	user    -  who to send to
 *			subject -  subject line
 *			text    -  text of message
 *			fname   -  mail file to cite if present
 *
 * $Log: ni_rtn.c,v $
 * Revision 5.6  1991/06/07  17:01:01  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:51  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:45:40  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:15:45  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:30:23  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 */

ni_rtn(user, subject, text, fname)
char	*user;
char	*subject;
char	*text;
char	*fname;
{
	FILE	*mfp, *popen();
	FILE	*fp, *fopen();
	char	cmd[256];
	char	euser[BUFSIZ];
	char	esubject[BUFSIZ];
	char	line[BUFSIZ];
	int	lines, ret;
	char	*mailsys = rindex(mailprog, '/');
	int	mask	= ADD_TO | ADD_SUBJ | ADD_BLANK | ESCAPE_SUBJECT;
	char	*format	= "%s \"%.0s%s\"";

	if (!mailsys++) mailsys = mailprog;

	for (ret=0; ret < CONF_MAX && mailfmt[ret].prog; ret++)
	if (strcmp(mailsys,  mailfmt[ret].prog) == 0 ||
	    strcmp(mailprog, mailfmt[ret].prog) == 0)
	{
		format	= mailfmt[ret].form;
		mask	= mailfmt[ret].flags;
		break;
	}

	if (mask & ESCAPE_USER)
	{	expand_esc(euser, user, sizeof euser, escape_double);
		user = euser;
	}

	if (mask & ESCAPE_SUBJECT)
	{	expand_esc(esubject, subject, sizeof esubject, escape_double);
		subject = esubject;
	}
	
	sprintf(cmd, format, mailprog, subject, user);

	L_LOG_4(L_GENERAL, 0, "informing %s with %x `%s' (%s)\n",
		user, mask, cmd, format);

	if ((mfp = popen(cmd, "w")) == NULL)
	{	L_WARN_2(L_GENERAL, 0, "Failure to invoke mailer `%s' (%d)\n",
						cmd, errno);
		return(NOTOK);
	}

	/*  send header  */
	if (mask & ADD_TO)
		fprintf(mfp, "To: %s\n", user);
	if (mask & ADD_SUBJ)
		fprintf(mfp, "Subject: %s\n", subject);
	if (mask & ADD_BLANK)
		fprintf(mfp, "\n");

	/*  send text  */
	fprintf(mfp, "%s\n", text);

	/*  cite file is one is given  */
	if (fname[0] != '\0')
	{	if ((fp = fopen(fname, "r")) == NULL)
		{	L_WARN_2(L_GENERAL, 0,
			    "Failed to read mail file %s\n", fname, errno);
			return(NOTOK);
		}

		/*  assume this is a JNT mail file  */
		fprintf(mfp, "\nYour message was not delivered to the following addresses:\n");
		while (fgets(line, sizeof(line), fp) != NULL)
		{	if (line[0] == '\n')
				break;
			fprintf(mfp, "\t%s", line);
		}

		fprintf(mfp, "\n\n\tYour message begins as follows:\n\n");
		/*  print top of message header+body  */
		for (lines = MAXLINES; --lines > 0 && fgets(line, sizeof(line), fp) != NULL;)
			fputs(line, mfp);
		
		if (!feof(fp))
			fputs("... more than %d lines ...\n", mfp, MAXLINES);

		fclose(fp);
	}

	ret = pclose(mfp);
	
#ifdef CATCHALL
	if (ret & 0xffff)
#else
	if (ret & 0xff)
#endif
	{	/*
		 *  /bin/mail failed
		 */
		L_WARN_2(L_GENERAL, 0, "%s failed, returning %04x\n", cmd, ret);
		return(NOTOK);
	}
	return(OK);
}

/* Preformat a string for double quoting */
expand_esc(to, from, len, chars)
char *to;
char *from;
char *chars;
{	char *endp = to + len -2;

	while (*from && to < endp)
	{	if (index(chars, *from)) *to++ = '\\';
		*to++ = *from++;
	}
	*to = '\0';
}

log_in_file(user, subject, text, fname, dest)
char	*user;
char	*subject;
char	*text;
char	*fname;
char	*dest;
{
	FILE *mfp, *fp;
	int lines;
	char line[1024];
	long now;

	if (!(mfp = fopen(dest, "a")))
	{	L_WARN_2(L_GENERAL, 0,
			"log_in_file(%s) fopen failed %d\n", dest, errno);
		return(NOTOK);
	}
	time(&now);
	L_LOG_3(L_GENERAL, 0, "Logging failure mail for %s about %s to %s\n",
		user, subject, dest);
	fprintf(mfp, "To: %s\n", user);
	fprintf(mfp, "Subject: %s\n", subject);
	fprintf(mfp, "Date: %s\n", ctime(&now));
	fprintf(mfp, "%s\n", text);

	/*  cite file is one is given  */
	if (fname[0] != '\0')
	{	if ((fp = fopen(fname, "r")) == NULL)
		{	L_WARN_2(L_GENERAL, 0,
			    "Failed to read mail file %s\n", fname, errno);
			fclose(mfp);
			return(NOTOK);
		}

		/*  assume this is a JNT mail file  */
		fprintf(mfp, "\nYour message was not delivered to the following addresses:\n");
		while (fgets(line, sizeof(line), fp) != NULL)
		{	if (line[0] == '\n')
				break;
			fprintf(mfp, "\t%s", line);
		}

		fprintf(mfp, "\n\n\tYour message begins as follows:\n\n");
		/*  print top of message header+body  */
		for (lines = MAXLINES; --lines > 0 && fgets(line, sizeof(line), fp) != NULL;)
			fputs(line, mfp);
		
		if (!feof(fp))
			fputs("... more than %d lines ...\n", mfp, MAXLINES);

		fclose(fp);
	}

	if (fclose(mfp))
	{	L_WARN_2(L_GENERAL, 0,
			"log_in_file(%s) fclose failed %d\n",
			dest, errno);
		return(NOTOK);
	}
	return(OK);
}
