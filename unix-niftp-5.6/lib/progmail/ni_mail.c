#ifndef	lint			/* unix-niftp lib/progmail/ni_mail.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/progmail/ni_mail.c,v 5.3 89/07/16 12:03:52 pb Rel Locker: pb $";
#endif	lint

/*
 *  ni_mail  -  NIFTP -> Arbitary program interface.
 *
 * Pipe the data part of the message to an arbitary program,
 * under the directions of deliver string in mailfmt which should expect args:
 *	mailprogname, TSname, queue
 *
 * $Log:	ni_mail.c,v $
 * Revision 5.3  89/07/16  12:03:52  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:42:25  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:26:44  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:59:40  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:54:19  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 */	
#include "ftp.h"
#include "nrs.h"
#include <stdio.h>
#include <sysexits.h>

#define	CMDSIZ		5000			/*  command line size  */

#define	OK	0
#define NOTOK	1

/* #define CATCHALL	/*  send all undeliverable mail to postmaster  */

char	cmd[CMDSIZ];
char	line[BUFSIZ];
FILE	*fp, *outp;
FILE	*fopen(), *popen();
char	*rindex();
extern char xerr;
extern char reason[];

do_mail(file)
char *file;
{	int	ret = ni_mail(file, argstring, hostname);

	if (ret != OK) L_WARN_1(L_GENERAL, 0, "Mailer failed %04x\n", ret);
	else xerr = 0;

	(void) unlink(file);
	return(ret);
}
		
/*
 *  NI_MAIL  --  This routine takes a mail file in JNT format and converts the
 *		 Janet header into an argument and then calls  the program
 *		 with the message part of the mail file as standard input.
 *
 *	Parameters:	fname  -  file containing JNT mail message
 *			queue  -  name of NIFTP queue
 *			TSname -  TS name of calling host
 */
/* queue UNUSED */

/* ARGSUSED */
ni_mail(fname, queue, TSname)
char	*fname;
char	*queue;
char	*TSname;
{
	register char	*usersp, *c, *n;
	int	ret;
	char	*mailsys = rindex(mailprog, '/');
	int	mask	= ADD_RECV | CATCH_ALL;
	char	*format	= "%s -v '%s'";

	if (!mailsys++) mailsys = mailprog;

	for (ret=0; ret < CONF_MAX && mailfmt[ret].prog; ret++)
	if (strcmp(mailsys,  mailfmt[ret].prog) == 0 ||
	    strcmp(mailprog, mailfmt[ret].prog) == 0)
	{
		format	= mailfmt[ret].deliver;
		mask	= mailfmt[ret].flags;
		break;
	}

	if ((fp = fopen(fname, "r")) == NULL)
	{	L_WARN_2(L_GENERAL, 0, "could not open %s (%d)\n",
			fname, errno);
		return(NOTOK);
	}

	/*
	 *  initialise command line.
	 */
	sprintf(usersp = cmd, format, mailprog,
			TSname, queue, fname,
			TSname, queue, fname,
			TSname, queue, fname);
	while (*usersp)	usersp++;

	/*
	 *  get list of addresses to send to in JNT-header form and
	 *  convert to a list of command line arguments.
	 */
	if (!(mask & SEND_ASIS)) while (fgets(line, BUFSIZ, fp) != NULL)
	{	c = line;

		while (*c == ' ' || *c == '\t') c++;
		if (*c == '\n')	break;

		*usersp++ = ' ';

		while (*c && *c != '\n')
			if (*c == ',')
				{ *usersp++ = ' '; c++; }
			else if (*c == '@' && *(c+1) == '[')
				while (*++c && *c != '\n')
				{ if (*c == ']') { c++; break; } }
			else	switch(*c)
			{	case '-':
					if (usersp[-1] == ' ')
					{	*usersp++ = '\\';
						*usersp++ = ' ';
					}
					*usersp++ = *c++;
					break;
				case '\\':
				case '\'':
				case '\t':
				case '"':
				case '`':
				case ' ':
				case '(':
				case ')':
				case '^':
				case '&':
				case ';':
				case '|':
				case '<':
				case '>':
				case '$':
					*usersp++ = '\\';
				default:
					*usersp++ = *c++;
			}
	}
	*usersp = '\0';

	/* call program */
	L_LOG_1(L_LOG_EXEC, 0, "call: %s\n", cmd);

	outp = popen(cmd, "w");
	if (mask & ADD_RECV) fprintf(outp, "Received: from %s\n", TSname);
	while (fgets(line, sizeof(line), fp) != NULL)
		fputs(line, outp);
	ret = pclose(outp);
	fclose(fp);

#ifdef CATCHALL
	if (ret & 0xffff)
#else
	if (ret & ((mask & CATCH_ALL) ? 0xffff : 0xff))
#endif
	{	char *me = NULL;

		switch( ret >> 8 )
		{
		case EX_OSERR:	me="Transient operating system error";	break;
		case EX_NOHOST:		me="Host name not recognised";	break;
		case EX_NOUSER:		me="User name unknown";		break;
		case EX_SOFTWARE:	me="Internal sendmail error";	break;
		case EX_TEMPFAIL:	me="Mail queued for delivery";	break;
		case EX_UNAVAILABLE:	me="Insufficient resources";	break;
		}
		if( me != NULL ) strcpy( reason, me);

		L_WARN_3(L_GENERAL, 0, "%s failed %04x (%s)\n",
						cmd, ret, reason);
		xerr = 1;
		return(NOTOK);
	}
	if (ret) L_WARN_2(L_GENERAL, 0, "(%s failed %04x)\n", cmd, ret);
	return(OK);
}
