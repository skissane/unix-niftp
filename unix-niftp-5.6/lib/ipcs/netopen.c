/* unix-niftp lib/ipcs/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:35:59 $ */
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
 * $Log:	netopen.c,v $
 * Revision 5.5  90/08/01  13:35:59  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:47:45  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:42:38  bin
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

/* open a connection to host str and fill in nptr appropriately */

con_open(str)
char    *str;
{
	register i;
	register struct netnode *nptr = &net_node;
	char    *top[1];

	ipcs_print = 0;
	if(ftp_print & 010)
		ipcs_print = 1;
	if(ftp_print & 040000)
		ipcs_print = 07;
#ifdef  DEBUG
	if(net_open)
		if(ftp_print & 020)
			printf("con_open called with net_open\n");
#endif

	top[0] = t_addr;
	/* What else do we do here ?? since this is specific to ucl */
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		if(ftp_print & 1)
			printf("Address trans error %d\n",i);
		return(-1);
	}
	if(ftp_print & 4)
		printf("Full address:- %s\n",t_addr);

	nptr->destination_address = t_addr;     /* fill out the netnode */
	nptr->read_count=0;
	nptr->timeout = TIMEOUT;
	nptr->flags = CS_WAITBIT; /* if reusing node -could have other info */
	starttimer();                           /* start the timeout system */
	time_out = 2*60;
	clocktimer();   /* only have 2 mins for timeout on open */
	time_out = 11*60;       /* back to rights */
	if( (i = netopen(nptr)) < 0){
		if(ftp_print & 1)
			printf("Netopen failed %d\n",i); /* con failed */
		netabort(nptr);                  /* zap the ring connection */
		if(i == -3)
		      sprintf(reason,"cannot make connection to host %s",str);
		return(i);
	}
	if(ftp_print & 4)
		printf("Netopen to %s\n",nptr->destination_address);
	net_open = 1;                   /* say we are open */
	net_io.read_buffer = nptr->read_buffer;
	net_io.write_buffer = nptr->write_buffer;
	net_io.read_count = nptr->read_count;
	net_io.write_count = nptr->write_count;
	return(0);
}
