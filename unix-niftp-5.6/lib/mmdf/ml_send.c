/* unix-niftp lib/mmdf/ml_send.c $Revision: 5.5 $ $Date: 90/08/01 13:36:15 $ */
#include "util.h"
#include "mmdf.h"
#include "nrs.h"
#include "ftp.h"
#include <stdio.h>
#include <signal.h>

/*
 * file ml_send.c
 * last modified 25-May-83
 * $Log:	ml_send.c,v $
 * Revision 5.5  90/08/01  13:36:15  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  89/08/27  14:07:35  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.2  89/01/13  14:48:42  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:45:18  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/
/*
 *     MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *
 *     Copyright (C) 1979,1980,1981  University of Delaware
 *
 *     Department of Electrical Engineering
 *     University of Delaware
 *     Newark, Delaware  19711
 *
 *     Phone:  (302) 738-1163
 *
 *
 *     This program module was developed as part of the University
 *     of Delaware's Multi-Channel Memo Distribution Facility (MMDF).
 *
 *     Acquisition, use, and distribution of this module and its listings
 *     are subject restricted to the terms of a license agreement.
 *     Documents describing systems using this module must cite its source.
 *
 *     The above statements must be retained with all copies of this
 *     program and may not be removed without the consent of the
 *     University of Delaware.
 *
 *
 *     version  -1    David H. Crocker    March   1979
 *     version   0    David H. Crocker    April   1980
 *     version  v7    David H. Crocker    May     1981
 *     version   1    David H. Crocker    October 1981
 *
 */
/*
modified by
ruth moulton, ucl april 1983
	to make independent of mmdf - for use with niftp
*/
/* send a piece of mail, using Unix mail command        */

/*  Basic sequence is:
 *
 *          ml_init (YES, NO, "My Name", "The Subject);
 *          ml_adr ("destination address 1");
 *          ml_adr ("destination address 2");
 *          ...
 *          ml_aend ();
 *          ml_tinit ();
 *          ml_txt ("Some opening text");
 *          ml_txt ("maybe some more text");
 *          ml_file (file-stream-descriptor-of-file-to-include);
 *          if (ml_end (OK)) != OK)
 *          {   error-handling code }
 *
 *  Arguments that are to be defaulted should be zero.
 *
 *  ml_init's arguments specify a) whether return-mail (to the sender
 *  should be allowed, b) whether a Sender field should be used to
 *  specify the correct sender (contingent on next argument), c) text
 *  for the From field, and d) text for the Subject field.  If (b) is
 *  NO, then (c)'s text will be followed by the correct sender
 *  information.
 *
 *  ml_to and ml_cc are used to switch between To and CC addresses.
 *  Normally, only To addresses are used and, for this, no ml_to call is
 *  needed.
 *
 *  An "address" is whatever is valid for your system, as if you were
 *  typing it to the mail command.
 *
 *  You may freely mix ml_txt and ml_file calls.  They just append text
 *  to the message.  The text must contain its own newlines.
 *
 *  Note that a special version of the mail command is used, to handle all
 *  the extra arguments.  If its sources weren't included with the
 *  distribution of this file, you probably have a problem.
 */

extern char pathmail[],           /* location of mail command           */
	    nammail[];
extern char cmddfldir[];

static FILE *ml_fp;             /* handle on output to mail command   */

static int    ml_childid;           /* process id of mail child           */
static short ml_curarg;            /* index of next argument             */

static char *ml_argv[20];          /* arguments to pass to execv         */
/**/

ml_init (ret, sndr, from, sub)    /* set-up for using mail command      */
int     ret,                      /* allow return mail to sender?       */
	sndr;                     /* include Sender field?              */
char    sub[],                    /* subject line                       */
	from[];                   /* from field                         */
{
    ml_argv[0] = "mail";
    if (ret)                      /* allow return to sender             */
	ml_curarg = 1;
    else
    {                             /* disable return to sender           */
	ml_argv[1] = "-r";
	ml_curarg = 2;
    }

    if (from != 0)
    {                             /* user-specified From field          */
	ml_argv[ml_curarg++] = (sndr) ? "-f" : "-g";
				  /* f => Sender field needed           */
	ml_argv[ml_curarg++] = from;
    }

    if (sub != 0)
    {                             /* user-specified Subject field       */
	ml_argv[ml_curarg++] = "-s";
	ml_argv[ml_curarg++] = sub;
    }

    return (ml_to ());            /* set-up for To: addresses           */
}
/**/


ml_to ()                          /* ready to specify To: address       */
{
    ml_argv[ml_curarg++] = "-t";
    return (OK);
}

ml_cc ()                          /* ready to specify CC: address       */
{
    ml_argv[ml_curarg++] = "-c";
    return (OK);
}

ml_adr (address)                  /* a destination for the mail         */
char    address[];
{
    ml_argv[ml_curarg++] = address;
    return (OK);
}

ml_aend ()                        /* end of addrs                       */
{
    ml_argv[ml_curarg] = 0;
    return (OK);
}
/**/

ml_tinit ()                     /* ready to send text                 */
{
    Pip    pipdes;              /* output pipe                        */
    register short    c;
    char **p;


    if (pipe (pipdes.pipcall))  /* for output to mail                 */
	return (NOTOK);

    if (ftp_print & L_LOG_EXEC){
	L_LOG_1(L_LOG_EXEC, 0, "forking to execv %s\n",mailprog);
	L_LOG_0(L_LOG_EXEC, 0, "with args:");
	p = ml_argv;
	while(*p)
		L_LOG_1(L_LOG_EXEC, L_CONTINUE, "%s;",*p++);
	L_LOG_0(L_LOG_EXEC, L_CONTINUE, "\n");
    }
    ml_childid = fork ();
    switch (ml_childid)
    {
	case NOTOK:               /* bad day all around                 */
	    close (pipdes.pip.prd);
	    close (pipdes.pip.pwrt);
	    return (NOTOK);

	case 0:                   /* this is the child                  */
	    close (0);

	    dup (pipdes.pip.prd);
	    for (c = HIGHFD; c > 0; c--)
		close (c);
	    open ("/dev/null",1);
		      /* give Submit a place to send msgs   */
	    setuid(FTPuid);

	    execv (mailprog, ml_argv);
	    L_ERROR_1(L_ALWAYS, L_DATE | L_TIME,
				"Failed to fork %s\n",mailprog);
	    exit (NOTOK);
    }                             /* BELOW HERE is the parent           */

    close (pipdes.pip.prd);

    ml_fp = fdopen (pipdes.pip.pwrt, "w");
				  /* initialize the stdio for output    */

    return (OK);
}
/**/

ml_file (infp)                    /* send a file to the message         */
register FILE  *infp;             /* input stdio file stream pointer    */
{
    register short len;
    char    buffer[BUFSIZE];

    if ((int) ml_fp == EOF || (int) ml_fp == NULL)
	return (OK);

    while ((len = fread (buffer, sizeof (char), sizeof(buffer), infp )) > 0)
	if (fwrite (buffer, sizeof (char), len, ml_fp) != len)
	{                         /* do raw i/o                         */
	    ml_end (NOTOK);
	    return (NOTOK);
	}

    if (len < OK)
    {
	ml_end (NOTOK);
	return (NOTOK);
    }
    return (OK);
}

ml_txt (text)                     /* some text for the body             */
char text[];                      /* the text                           */
{
    L_LOG_1(L_10, 0, "ml_txt:send text - %s\n",text);

    if (ml_fp == (FILE *) EOF || ml_fp == (FILE *) NULL) {
	L_LOG_1(L_GENERAL, 0, "ml_txt: immediate return on ml_fp being %s\n",
		(ml_fp == (FILE *) NULL) ? "NULL" : "EOF");
	return (OK);
    }

	fputs (text, ml_fp);

    if  (ferror (ml_fp))
    {
	ml_end (NOTOK);
	return (NOTOK);
    }
    return (OK);
}
/**/


ml_end (type)                     /* message is finished                */
int     type;                     /* normal ending or not               */
{
    short     retval;               /* wait return value                  */

    switch (ml_childid)
    {
	case OK:
	case NOTOK:
	    return (OK);

	default:                /* parent */
	    switch ((int) ml_fp)
	    {
		case OK:
		case NOTOK:
		    break;

		default:
		    if (ferror (ml_fp) || type == NOTOK)
			kill (ml_childid, SIGKILL);
		    fclose (ml_fp);
		    ml_fp = OK;
	    }

	    retval = pgmwait (ml_childid);
	    ml_childid = OK;
	    return (retval);
    }
}
/**/

ml_1adr (ret, sndr, from, sub, adr)
				  /* all set-up overhead in 1 proc      */
int     ret,                      /* allow return mail to sender?       */
	sndr;                     /* include Sender field?              */
char    sub[],                    /* subject line                       */
	from[],                   /* from field                         */
	adr[];                    /* the one address to receive msg     */
{

    if (ml_init (ret, sndr, from, sub) != OK ||
	    ml_adr (adr) != OK ||
	    ml_aend () != OK ||
	    ml_tinit () != OK)
	return (NOTOK);

    return (OK);
}
