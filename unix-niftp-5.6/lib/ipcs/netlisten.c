/* unix-niftp lib/ipcs/netlisten.c $Revision: 5.5 $ $Date: 90/08/01 13:35:57 $ */
#include "ftp.h"
#include "infusr.h"
#include "csinterface.h"
#include <stdio.h>

/*
 * THIS FILE HAS BEEN EXTENSIVLY MODIFIED (INTERFACE RESTRUCTURING)
 * SINCE LAST USE, IT MAY NOT EVEN COMPILE.
 * IF YOU INTEND TO USE IT PLEASE CHECK IT OUT CAREFULLY.
 * wja@nott.cs
 */
/*
 * file:
 *			 netopen.c
 *
 * last changed: 10-jul-85
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:35:57  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:47:42  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:42:37  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

/*
 * This file contains routines to interface to the IPCS system. They
 * make the network seem like a normal file. Makes the interface nicer
 */

extern struct netnode net_node;
extern int	ipcs_print;	/* more diagnostic printing */
				/* if this is not used by the IPCS
				   library it should be removed */

/*
 * listen for a call on the host number given as a parameter
 */
extern char     unknown_host;
extern char	why_unknown_host[];

con_listen(str)
char    *str;
{
	register i;
	register struct netnode *nptr = &net_node;

	ipcs_print = 0;
	if(ftp_print & 010 )
		ipcs_print = 1;
	if(ftp_print & 040000)
		ipcs_print = 7;

#ifdef  DEBUG
	if(net_open)
		if(ftp_print & 020)
			printf("con_listen called while net_open\n");
#endif
	(void) strcpy(t_addr, str);
	nptr->destination_address = t_addr;     /* set up the netnode */
	nptr->read_count =0;
	nptr->timeout = TIMEOUT;
	nptr->flags = CS_WAITBIT; /* if reusing node -could have other info */
	starttimer();
	if( (i = netlisten(nptr)) < 0){
		if(ftp_print & 1)
			printf("Netlisten failed %d\n",i);
		netabort(nptr);                         /* zap him for now */
		return(i);
	}
	if(ftp_print & 4)
		printf("Netlisten open to %s\n",nptr->destination_address);

	ptime();        /* print 'called at time' */
	/*
	 * try to find out who called us.
	 */
	if(r_addr_trans(nptr->destination_address, hostname, argstring) < 0){
		if(ftp_print & 1)
		    printf("can't translate %s\n", nptr->destination_address);
		unknown_host = 1;
	}
	else
		unknown_host = 0;

	net_open=1;
	net_io.read_buffer = nptr->read_buffer;
	net_io.write_buffer = nptr->write_buffer;
	net_io.read_count = nptr->read_count;
	net_io.write_count = nptr->write_count;
	return(0);
}

char    *Aarg;
char    *Uarg;
char    *Iarg;
char    *Farg;
char    *Sarg;
static  char    xbuff[ENOUGH];
char    *Called_address;        /* value of called TS string */

/*
 * search address tables and generate host mnemonic
 * from calling address. name is left in 'hostname'
 */

/* hostname contains = what we listenened on
 * netnode.destination_address = what we got back
 * name put in nhostname
 */

/*
 * character to be added to string for reverse lookups
 */

extern  char    lnetchar;

r_addr_trans(from,nhostname,hn)
char    *from,*nhostname,*hn;
{
	char net = lnetchar;
	char tbuff[ENOUGH];

	if(hn == NULL){		/* no channel - local transfer */
		(void) strcpy(nhostname, from);
		return(0);
	}

	if(nrs_split(from) < 0){
		sprintf(why_unknown_host, "IPCS-GARBLED-<%s>", from);
		printf("****NRS split failed!! <%s>\n", from);
		return(-1);
	}

	if(net == 'p' && strncmp(Aarg, "2342", 4) != 0)
		net = 'i';              /* fix for ipss addresses */

	/* compose calling address */
	strcpy(tbuff, Aarg);
	strcat(tbuff, ".");
	strcat(tbuff, Sarg);

	printf("TS-Call from <%s>\n", from);
	return(nrs_reverse(net, Called_address, tbuff, nhostname));
}

nrs_split(from)
register char   *from;
{
	register char   *to = xbuff;
	register char    **which;
	register seens = 0;

	Aarg = Uarg = Iarg = Farg = Sarg = NULL;

	while(*from){
		if(seens == 1){
			/* got the sarg */
			seens++;
			which = &Sarg;
			from--; /* get to the right place */
		}
		else switch(*from){
		case 'a':
		case 'A':
			which = &Aarg;
			break;
		case 'u':
		case 'U':
			which = &Uarg;
			break;
		case 'f':
		case 'F':
			which = &Farg;
			break;
		case 'i':
		case 'I':
			which = &Iarg;
			break;
		case 's':
		case 'S':
			if(!seens){
				seens++;
				which = &Called_address;
				break;
			}
			/* fall through */
		default:
			if(!seens){
				printf("Junk in address\n");
				return(-1);
			}
			/* skip until next char is a ',' */
			while(*from && *from != ',')
				from++;
			if(*from)
				from++;
			continue;
		}
		*which = to;
		from++; /* skip the first character */
		while(*from && *from != ',')
			*to++ = *from++;
		*to++ = 0;
		if(*from)
			from++;
	}
	if(Sarg == NULL){
		Sarg = "";
		printf("Null Calling address\n");
	}
	if(Aarg == NULL || Sarg == NULL){
		printf("BAD Arg split\n");
		return(-1);
	}
	return(0);
}
