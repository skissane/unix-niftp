#ifdef	lint	/* unix-niftp lib/pqproc/rsft.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/ppQspool.c,v 5.5 90/08/01 13:37:06 pb Exp $";
#endif	lint
/* $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/pqproc/ppQspool.c,v 5.5 90/08/01 13:37:06 pb Exp $ */
/*
 * $Log:	ppQspool.c,v $
 * Revision 5.5  90/08/01  13:37:06  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 */
#include "ftp.h"

/* Code to allow spooled PP handling of incoming mail.
 *
 *	THIS IS A CRIB FROM lib/progmail/ !!
 */

#ifdef	PP

#include "nrs.h"
#include <stdio.h>
#include <sysexits.h>

#define	CMDSIZ		5000			/*  command line size  */

#define	OK	0
#define NOTOK	1

char	cmd[CMDSIZ];
char	line[BUFSIZ];
FILE	*fp, *outp;
FILE	*fopen(), *popen();
char	*rindex();
extern char xerr;
extern char reason[];

/* Tell PP to commit */
do_pp(file)
char *file;
{	int ret;

	if (! file)
	{	L_LOG_1(L_ALWAYS, 0,
	 	"do_pp(%d) inline PP code means nothing to do\n",
			file);
		return 0;
	}

	L_LOG_1(L_ALWAYS, 0,
	      "do_pp(%s) using spooled accesss as no inline code available\n",
			file);
	ret = ni_pp(file, argstring, hostname);

	if (ret != OK) L_WARN_1(L_GENERAL, 0, "Mailer failed %04x\n", ret);
	else xerr = 0;

	(void) unlink(file);
	return(ret);
}
		
/*
 *  NI_PP  --  This routine takes a pp file in JNT format and converts the
 *		 Janet header into an argument and then calls  the program
 *		 with the message part of the pp file as standard input.
 *
 *	Parameters:	fname  -  file containing JNT mail message
 *			queue  -  name of NIFTP queue
 *			TSname -  TS name of calling host
 */
/* queue UNUSED */

/* ARGSUSED */
ni_pp(fname, queue, TSname)
char	*fname;
char	*queue;
char	*TSname;
{
	register char	*usersp, *c, *n;
	int	ret;
	char	*ppsys	= rindex(PPproc, '/');
	int	mask	= CATCH_ALL;
	char	*format	= "%s grey %s %0.0s%s";

	if (!ppsys++) ppsys = PPproc;

	for (ret=0; ret < CONF_MAX && mailfmt[ret].prog; ret++)
	if (strcmp(ppsys,  mailfmt[ret].prog) == 0 ||
	    strcmp(PPproc, mailfmt[ret].prog) == 0)
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
	sprintf(usersp = cmd, format, PPproc,
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

#endif	PP
