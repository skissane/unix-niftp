#ifndef	lint			/* unix-niftp lib/x25b/netopen.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/x25b/netopen.c,v 5.6 1991/06/07 17:02:25 pb Exp $";
#endif	lint

/*
 * file
 *			 netopen.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: netopen.c,v $
 * Revision 5.6  1991/06/07  17:02:25  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:39:19  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.3  89/07/16  12:03:57  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.1  88/10/07  17:21:40  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:35:29  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.4  88/01/28  06:35:15  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0  88/01/28  06:23:32  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * 
 * This file contains routines to interface to the x25bridge libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */

#include "x25b.h"
#include "nrs.h"
#include "ftp.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#define	MAXFS_BYTES	16
#define	CONNECT	0x10
#define	ACCEPT	0x11

extern fid;
extern	def_pktsize;
extern	def_wndsize;
char *index();

char ybts_hdr[] = { 0x7f, 0xff, 0xff, 0xff };

extern	Nopts;	/* Network options */
extern	Npkt_size;
extern	Nwnd_size;

con_open(str)
char    *str;
{
	register nodep nptr = &net_io;
	struct facilities facil;
	register i;
	int	s;
	char    fromdte[ENOUGH];
	char	dte[ENOUGH];
	char	*ybts;
	char	*calling;
	char	*ybtsp;
	char	*port;
	char    *top[2];

	facil.x4_fflags = 0;

	if(net_open) L_WARN_0(L_10, 0, "con_open called with net_open\n");

	top[0] = fromdte;
	top[1] = t_addr;
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		return(-1);
	}

	L_LOG_2(L_FULL_ADDR, 0, "Full address:- %s (from %s)\n",
		hide_pss_pw(t_addr), fromdte);

	strcpy(dte, t_addr);
	if (ybts = index(dte, '/')) *ybts++ = '\0';
	if (calling = index(fromdte, '/')) *calling++ = '\0';
	else	calling="<unknown>";

	if (port=index(fromdte, ';')) *port++ = '\0';

#ifdef	__STDC__
#define	set(x, y, z) { facil.x4_ ## x = z; facil.x4_fflags |= htons(FACIL_F_ ## y); }
#else	/* __STDC__ */
#define	set(x, y, z) { facil.x4_/**/x = z; facil.x4_fflags |= htons(FACIL_F_/**/y); }
#endif	/* __STDC__ */
#define htobcd(x) ((x == 100) ? 0 : ( ((x / 10) << 4) | (x % 10)))

	if (tab.t_flags & REVERSE_CHARGING)
			  set(reverse_charge,REVERSE_CHARGE, 1);
	if (trans_revc)	  set(reverse_charge,REVERSE_CHARGE, 1);

	if (def_pktsize)  set(recvpktsize,RECVPKTSIZE, htons(def_pktsize));
	if (def_pktsize)  set(sendpktsize,RECVPKTSIZE, htons(def_pktsize));
	if (Npkt_size)    set(recvpktsize,RECVPKTSIZE, htons(Npkt_size));
	if (Npkt_size)    set(sendpktsize,RECVPKTSIZE, htons(Npkt_size));
	if (trans_pkts)	  set(recvpktsize,RECVPKTSIZE, htons(8 << trans_pkts));
	if (trans_pkts)	  set(sendpktsize,RECVPKTSIZE, htons(8 << trans_pkts));

	if (def_wndsize)  set(recvwndsize,RECVWNDSIZE, def_wndsize);
	if (def_wndsize)  set(sendwndsize,RECVWNDSIZE, def_wndsize);
	if (trans_wind)	  set(recvwndsize,RECVWNDSIZE, trans_wind);
	if (trans_wind)	  set(sendwndsize,RECVWNDSIZE, trans_wind);

/*-	if (thruput..)	set(recvthruput, , ??); -*/
/*-	if (thruput..)	set(sendthruput, , ??); -*/
	if (trans_cug)	set(cug_index,CUG_INDEX, htobcd(trans_cug));
	if (Nopts & N_NFCS)
			set(fast_select,FAST_SELECT, FACIL_NO)
	else if (Nopts & N_FCS)
			set(fast_select,FAST_SELECT, FACIL_YES)
	else		set(fast_select,FAST_SELECT, FACIL_YES)
/*-	if (rpoa ...)	facil.rpoa_req=1, facil.rpoa, , ??; -*/

	_x25b_print_facil(&facil);

	nptr->read_count = 0;
	starttimer(2*60);		/* start the timeout system */
	alarm(2*60);		/* TEMP HACK til start timer is fixed */

	fid = x25b_open_ybts(dte, ybts, calling, fromdte, port, &facil, 0);

	suck_userdata();
	/* Now await the accept */
	{	char	temp[256];
		int	nbytes;
		int i;

		nbytes = x25b_read_data(fid, temp, temp+1, sizeof(temp)-1);
		L_DEBUG_2(L_LOG_OPEN, 0, "Accept: %d: %02x: ", nbytes, temp[0] & 0xff);
		for(i=1; i<= nbytes;i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x", temp[i] & 0xff);
		L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
		if(nbytes < 1) 
		{	L_WARN_2(L_GENERAL, 0, "recv acc failed %d (%d)\n",
					nbytes, errno);
			return (-1);
		}
		nbytes++;
		if (temp[1] != ACCEPT /*|| !(temp[0] & (1<<Q_BIT))*/)
		{
			L_WARN_3(L_GENERAL, 0, "invalid ac %d: %02x %02x\n",
				nbytes, temp[0] & 0xff, temp[1] & 0xff);
			if ((ftp_print & 0x7f) != 0x7f) return (-1);
			L_LOG_0(L_GENERAL, 0, " +++ continuing anyway!\n");
		}
	}
	/* WHEW !!!! */

	starttimer(11*60);		/* give it a chance now .... */
	nptr->write_count = BLOCKSIZ;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	net_open = 1;                   /* say we are open */
	return(0);
}

/* discard user data .... */

suck_userdata()
{	
}
