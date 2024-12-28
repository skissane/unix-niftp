#ifndef	lint			/* unix-niftp lib/sun/netopen.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/sun/RCS/netopen.c,v 5.6 1991/06/07 17:02:18 pb Exp $";
#endif	lint

/*
 * file:	netopen.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: netopen.c,v $
 * Revision 5.6  1991/06/07  17:02:18  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:38:16  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:54:03  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  16:56:48  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:34:56  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.4  88/01/28  06:13:38  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:34:55  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:08:23  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:01:03  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * 
 * This file contains routines to interface to the SUNlink libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */

#include "ftp.h"
#include "infusr.h"
#include "nrs.h"
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sundev/syncstat.h>
#include <netx25/x25_pk.h>
#include <netx25/x25_ioctl.h>

#define	MAXFS_BYTES	16

void sigurg();
extern fid;
extern char *index(), *strcat (), *hide_pss_pw();

char *sprintf_diag();

unsigned char ybts_hdr[] = { 0x7f, 0xff, 0xff, 0xff };

extern	Nopts;	/* Network options */
extern	Npkt_size;
extern	Nwnd_size;

#ifdef	X25_SET_FACILITY
static int set_facility(facilityp, type, fatal, text)
FACILITY *facilityp;
int type;
int fatal;
char *text;
{
	facilityp -> type = type;
	if (ioctl(fid, X25_SET_FACILITY, facilityp))
	{	L_WARN_2(L_GENERAL, 0, "%s failed (%d)\n", text, errno);
		(void) strcpy(reason, "sun/netopen: internal: failed to set ");
		(void) strcat(reason, text);
		if (fatal)  close(fid);
		return(-1);
	}
	return 0;
}
#endif

con_open(str)
char    *str;
{
	register nodep nptr = &net_io;
	register i;
#ifdef	X25_SET_FACILITY
	FACILITY facility;
#endif
	int	pending_n	= 0;
	CONN_DB	dest_addr;
	FACILITY_DB facil;

	char    fromdte[ENOUGH];
	char	dte[ENOUGH];
	char	iso[ENOUGH];
	char	*ybts;
	char	*calling;
	unsigned char	*ybtsp;
	char    *top[4];
	int	ybtslen;
	int	callinglen;
	int	firstoctet_mask = 0;
	int wndsize;
	int pktsize;

	if(net_open) L_WARN_0(L_10, 0, "con_open called with net_open\n");

	top[0] = fromdte;
	top[1] = dte;
	top[1] = t_addr;
	top[2] = iso;
	top[3] = (char *) 0;

	*fromdte = *t_addr = *dte = *iso = '\0';
	if((i=adr_trans(str, network, top)) != 0){
		/* translate to get network too */
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		(void) strcpy(reason, "sun/netopen: internal Address trans error");
		return(-1);
	}
	wndsize = (trans_wind) ? trans_wind :
		(Nwnd_size) ? Nwnd_size : def_wndsize;
	pktsize = (trans_pkts) ? (8 << trans_pkts) :
		(Npkt_size) ? Npkt_size : def_pktsize;

	L_LOG_3(L_FULL_ADDR, 0, "Full address:- %s %s (from %s)\n",
		hide_pss_pw(t_addr), iso, fromdte);

	(void) strcpy(dte, t_addr);
	if ((ybts = index(dte, '/')) != NULL) {
		*ybts++ = '\0';
		ybtslen = strlen(ybts);
	} else
		ybtslen = 0;

	if ((calling = index(fromdte, '/')) != NULL) *calling++ = '\0';
	else	calling="<unknown>";

	callinglen = strlen(calling);

	nptr->read_count = 0;
	starttimer(2*60);		/* start the timeout system */
	alarm(2*60);		/* TEMP HACK til start timer is fixed */

	fid = socket(AF_X25, SOCK_STREAM, 0);

	if (fid < 0)
	{
		L_WARN_1(L_GENERAL, 0, "X25 socket failed (%d)\n", errno);
		(void) strcpy(reason, "sun/netopen: internal: X25 socket failed");
		return(-1);
	}
	{	int proc_group = getpid();
		if (ioctl(fid, SIOCSPGRP, &proc_group) < 0)
		L_WARN_3(L_GENERAL, 0,
			"Failed to set process group for fd %d to %d (%d)\n",
				fid, proc_group, errno);
	}


#define thruput(x) ((x == 0) ? 0 : (x == 10) ? 9600 : (x == 11) ? 19200 : \
(x == 12) ? 48000 : (x == 7) ? 1200 : (x == 8) ? 2400 : (x == 9) ? 4800 : \
(x == 6) ? 600 : (x == 5) ? 300 : (x == 4) ? 150 : (x == 3) ? 75 : -x)
#define htobcd(x) ((x == 100) ? 0 : ( ((x / 10) << 4) | (x % 10)))

#ifdef	X25_SET_FACILITY
	if (tab.t_flags & REVERSE_CHARGING || trans_revc)
	{	facility.f_reverse_charge = 1;
		if (set_facility(&facility, T_REVERSE_CHARGE, 1,
			"reverse charge")) return -1;
	}

	if (pktsize)
	{	facility.f_sendpktsize	= pktsize;
		facility.f_recvpktsize	= pktsize;
		set_facility(&facility, T_PACKET_SIZE, 0, "packet size");
	}

	if (wndsize)
	{	facility.f_sendwndsize	= wndsize;
		facility.f_recvwndsize	= wndsize;
		set_facility(&facility, T_WINDOW_SIZE, 0, "window size");
	}

/*-	if (thruput..)	facil.recvthruput	= ??; -*/
/*-	if (thruput..)	facil.sendthruput	= ??; -*/
	if (trans_cug)
	{	facility.f_cug_index = htobcd(trans_cug);
		facility.f_cug_req = CUG_REQ;
		if (set_facility(&facility, T_CUG, 1,
			"closed user group")) return -1;
	}
	facility.f_fast_select_type = ((Nopts & (N_NFCS | N_FCS)) == N_FCS) ?
		FAST_OFF : FAST_ACPT_CLR;
	if (((Nopts & (N_NFCS | N_FCS)) == 0) && trans_fcs)
		facility.f_fast_select_type = (trans_fcs < 0) ?
			FAST_OFF : FAST_ACPT_CLR;
	set_facility(&facility, T_FAST_SELECT_TYPE, 0,
		"fast select");
	facil.fast_select_type = facility.f_fast_select_type;
#else	/* X25_SET_FACILITY */
	if (ioctl (fid, X25_RD_FACILITY, &facil))
	{
		L_WARN_1(L_GENERAL, 0, "rd_facil failed (%d)\n", errno);
		(void) strcpy(reason, "sun/netopen: internal: rd_facil failed");
		close(fid);
		return(-1);
	}


	L_LOG_3(L_LOG_OPEN, 0, "Facilities are rev chge %d, pkts=%d/%d, ",
		facil.reverse_charge,
		facil.recvpktsize,
		facil.sendpktsize);
	L_LOG_4(L_LOG_OPEN, L_CONTINUE, "wnd=%d/%d, thru=%d/%d, ",
		facil.recvwndsize,
		facil.sendwndsize,
		thruput(facil.recvthruput),
		thruput(facil.sendthruput));
	L_LOG_3(L_LOG_OPEN, L_CONTINUE, "cug=%x, %s, rpoa=%x\n",
		(facil.cug_req) ? facil.cug_index : 0xff,
		(facil.fast_select_type == FAST_OFF) ? "no FS" :
		(facil.fast_select_type == FAST_CLR_ONLY) ? "FS CLR" :
		(facil.fast_select_type == FAST_ACPT_CLR) ? "FS ACC" : "??",
		(facil.rpoa_req) ? facil.rpoa : 0xffff);



/* BUG in OS3.3+ & SunLink 4.0 ......
 * If you set the thruput to the value as supplied by X25_RD_FACILITY,
 * then after doing a X25_WR_FACILITY, the connect will fail with the
 * bizarre error `not yet connected'.
 */
	facil.recvthruput	= 0;
	facil.sendthruput	= 0;
/* End of bug fix
 */

	if (tab.t_flags & REVERSE_CHARGING)	facil.reverse_charge	= 1;
	if (trans_revc)		facil.reverse_charge	= 1;

	if (pktsize)		facil.recvpktsize	= pktsize;
	if (pktsize)		facil.sendpktsize	= pktsize;

	if (wndsize)		facil.recvwndsize	= wndsize;
	if (wndsize)		facil.sendwndsize	= wndsize;

/*-	if (thruput..)	facil.recvthruput	= ??; -*/
/*-	if (thruput..)	facil.sendthruput	= ??; -*/
	if (trans_cug)	facil.cug_req=1, facil.cug_index = htobcd(trans_cug);
	if (Nopts & N_NFCS)	facil.fast_select_type = FAST_OFF;
	else if (Nopts & N_FCS)	facil.fast_select_type	= FAST_ACPT_CLR;
	else if	(trans_fcs < 0)	facil.fast_select_type	= FAST_OFF;
	else if	(trans_fcs > 0)	facil.fast_select_type	= FAST_ACPT_CLR;
	else			facil.fast_select_type	= FAST_ACPT_CLR;
	L_LOG_2(L_LOG_OPEN, 0, "[%x -> %d] ", Nopts, facil.fast_select_type);
/*-	if (rpoa ...)	facil.rpoa_req=1, facil.rpoa = ??; -*/

	if (pktsize || wndsize ||
		(tab.t_flags & REVERSE_CHARGING) || trans_revc || trans_cug
)
		L_WARN_4(L_GENERAL, 0,
			"Request pkt=%d, window=%d, rec=%d, cig=%02x\n",
			facil.sendpktsize, facil.sendwndsize,
			facil.reverse_charge,
			(facil.cug_req) ? facil.cug_index : 0xff);

	L_LOG_3(L_LOG_OPEN, 0, "-> rev chge %d, pkts=%d/%d, ",
		facil.reverse_charge,
		facil.recvpktsize,
		facil.sendpktsize);
	L_LOG_4(L_LOG_OPEN, L_CONTINUE, "wnd=%d/%d, thru=%d/%d, ",
		facil.recvwndsize,
		facil.sendwndsize,
		thruput(facil.recvthruput),
		thruput(facil.sendthruput));
	L_LOG_3(L_LOG_OPEN, L_CONTINUE, "cug=%x, %s, rpoa=%x\n",
		(facil.cug_req) ? facil.cug_index : 0xff,
		(facil.fast_select_type == FAST_OFF) ? "no FS" :
		(facil.fast_select_type == FAST_CLR_ONLY) ? "FS CLR" :
		(facil.fast_select_type == FAST_ACPT_CLR) ? "FS ACC" : "??",
		(facil.rpoa_req) ? facil.rpoa : 0xffff);
	if (ioctl (fid, X25_WR_FACILITY, &facil) != 0)
	{
		L_WARN_1(L_GENERAL, 0, "wr_facil failed (%d)\n", errno);
		(void) strcpy(reason, "sun/netopen: internal: wr_facil failed");
		close(fid);
		return (-1);
	}
#endif	/* X25_SET_FACILITY */

	/* NB: DTEs are BCD, so two per byte */
	dest_addr.hostlen = strlen(dte);  /* called DTE len */
	{ int i;
	  char *hex = (char *) (dest_addr.host);
	  char *p = dte;
	  for (i=0; i <dest_addr.hostlen && isdigit(*p); i++, p++)
	  {	if (i & 1)
			*hex++ |=  ((*p) - '0');
		else	*hex    = (((*p) - '0') << 4);
	  }
	  if (i < dest_addr.hostlen)
		L_WARN_3(L_ALWAYS, 0, "string length %d unused: '%s' (%s)\n",
			dest_addr.hostlen - i, p, dte);
	}

	dest_addr.datalen = ybtslen + callinglen + sizeof(ybts_hdr) + 2;
	/* If there's too much data for the open, split it */
	if (dest_addr.datalen > MAXFS_BYTES && facil.fast_select_type == FAST_OFF)
	{	pending_n = dest_addr.datalen - MAXFS_BYTES;
		dest_addr.datalen = MAXFS_BYTES;
		firstoctet_mask = 64;
	}
	ybtsp = dest_addr.data;
	for(i=0; i<sizeof(ybts_hdr); i++) *ybtsp++ = ybts_hdr[i];
	*ybtsp++ = 0x80 | firstoctet_mask | ybtslen;
	for(i=0; i<ybtslen; i++) *ybtsp++ = ybts[i];
	*ybtsp++ = 0x80 | callinglen;
	for(i=0; i<callinglen; i++) *ybtsp++ = calling[i];
	if (ftp_print & L_LOG_OPEN)
	{	int i;
		L_DEBUG_1(L_LOG_OPEN, 0, "Open to DTE (%d) ",
			dest_addr.hostlen);
		for (i=0; i < dest_addr.hostlen / 2; i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x",
			dest_addr.host[i] & 0xff);
		L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, ", call data (%d) ",
			dest_addr.datalen);
		if (pending_n) L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "(+%d) ",
			pending_n);
		for (i=0; i < dest_addr.datalen + pending_n; i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x",
			dest_addr.data[i] & 0xff);
		L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
	}


#ifdef	ANY_LINK
		dest_addr.hostlen |= ANY_LINK;
#endif	/* ANY_LINK */

#ifdef	X25_WR_LOCAL_ADR
	/* Set the calling DTE */
	/* This is of the form
		DTE[:s][:l=<0-9*>][:]
	 */
	if (*fromdte && *fromdte != '/')
	{	CONN_ADR loc_addr;
		int i = 0;
		char *hex = (char *) (loc_addr.host);
		char *text = fromdte;
			/* Check the length ! */
	  	for (; isdigit(*text); i++, text++)
		{	if (i & 1)
				*hex++ |=  ((*text) - '0');
			else	*hex    = (((*text) - '0') << 4);
		}
		loc_addr.hostlen = text - fromdte;  /* called DTE len */

		while (*text == ':')
		{	text++;
			switch(*text)
			{
			case '\0':	break;
			case ':':	break;
			case '/':	break;
#ifdef	SUBADR_ONLY
			case 's':	loc_addr.hostlen |= SUBADR_ONLY;
					text++;				break;
#endif	/* SUBADR_ONLY */
#ifdef	X25_SET_LINK
			case 'l':
				if (text[1] != '=')	break;
				switch(text[2])
				{
				default: break;
				case '*': text += 3;	break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
				case '8': case '9':
				{	int link = text[2] - '0';
					L_DEBUG_1(L_LOG_OPEN, 0,
						"Using link %d\n", link);
					if (ioctl(fid, X25_SET_LINK, &link) != 0)
						L_WARN_1(L_GENERAL, 0,
							"set link %d failed\n",
							link);
#ifdef  ANY_LINK
					else dest_addr.hostlen &= ~ ANY_LINK;
#endif  /* ANY_LINK */

					text += 3;
				}
				}
#endif	/* X25_SET_LINK */
			default:	break;
			}
		}
		if (*text && *text != '/') L_LOG_1(L_ALWAYS, 0,
				"extra calling info `%s'\n", text);
		if (ioctl(fid, X25_WR_LOCAL_ADR, &loc_addr) != 0)
		{	L_WARN_1(L_GENERAL, 0,
				"Set of calling addre to `%s' FAILED\n",
				fromdte);
		}
		else	L_DEBUG_1(L_LOG_OPEN, 0, "Set calling addr to `%s'\n",
				fromdte);
	}
#endif	/* X25_WR_LOCAL_ADR */

	/* Give me the FULL header.
	   Has to be called BEFORE the relevant data ARRIVES
	   NOT when it is actually READ.
	 */
	{	int	header	= 1;
		if (ioctl(fid, X25_HEADER, &header) < 0 )
		{	L_WARN_1(L_GENERAL, 0, "full hdr failed (%d)\n",
				errno);
			(void) strcpy(reason, "sun/netopen: internal: request full header failed");
			close(fid);
			return(-1);
		}
	}

	if (connect(fid, &dest_addr, sizeof(CONN_DB)))
	{	X25_CAUSE_DIAG diag;

		(void) strcpy(reason, "sun/netopen: X.25 connect failed");
		L_WARN_1(L_GENERAL, 0, "connect failed (%d)\n", errno);
		if (ioctl(fid, X25_RD_CAUSE_DIAG, &diag) == 0)
		{	char temp[256];
			temp[0] = '\0';
			sprintf_diag(temp, &diag);
			L_WARN_1(L_GENERAL, 0, "X25:%s\n", temp);
			(void) strcat(reason, ": ");
			(void) strcat(reason, temp);
		}
		else	L_WARN_1(L_GENERAL, 0,
				"X25_RD_CAUSE_DIAG ioctl failed %d\n", errno);
		suck_userdata();
		close(fid);
		return (-1);
	}

	/* By default the stupid sun THROWS AWAY any data in the
	 * current packet is the buffer isn't big enough !!
	 */
	{	int one = 1;
		if (ioctl(fid, X25_RECORD_SIZE, &one))
			L_WARN_0(L_GENERAL, 0, "Failed to set record size\n");
	}

	suck_userdata();

	if (pktsize || wndsize)
	{	if (ioctl (fid, X25_RD_FACILITY, &facil) != 0)
		{
			L_WARN_1(L_GENERAL, 0, "wr_facil failed (%d)\n", errno);
		}
		if (pktsize != facil.recvpktsize ||
		    pktsize != facil.sendpktsize ||
		    wndsize != facil.recvwndsize ||
		    wndsize != facil.sendwndsize)
		{	L_WARN_4(L_GENERAL, 0, "Negotiated %d/%d:%d/%d",
				pktsize, pktsize,
				wndsize, wndsize);
			L_WARN_4(L_GENERAL, L_CONTINUE, " -> %d/%d:%d/%d\n",
				facil.recvpktsize, facil.sendpktsize,
				facil.recvwndsize, facil.sendwndsize);
		}
	}

	/* opened -- can send non FS data */
	if (pending_n)
	{	int qbit = (1 << Q_BIT);

		L_LOG_1(L_GENERAL, 0, "Send non FS data (%d)\n", pending_n);
		if (ioctl(fid, X25_SEND_TYPE, &qbit) < 0 ) L_WARN_1(L_GENERAL,
				 0, "QBIT set failed (%d)\n", errno);
		/* Let something else cause an error ... */

		/* Bit ikky -- stuff the CONNECT info into the buffer
		 * and send it off
		 */
		dest_addr.data[MAXFS_BYTES-1] = YB_CONNECT;
		if (send(fid,&(dest_addr.data[MAXFS_BYTES-1]),pending_n+1,0)!=
			pending_n+1) L_WARN_1(L_GENERAL, 0,
				"send of non FS CONNECT (%d)\n", errno);
		/* Let something else cause an error ... */

		/* What should the send type be left set to ? */
		qbit = 0;
		if (ioctl(fid, X25_SEND_TYPE, &qbit) < 0 ) L_WARN_1(L_GENERAL,
				0, "QBIT clear failed (%d)\n", errno);
		/* Let something else cause an error ... */
	}

	/* Now await the accept */
	{	char	temp[256];
		int	nbytes;
		int i;

		nbytes = recv(fid, temp, sizeof(temp), 0);
		L_DEBUG_2(L_LOG_OPEN, 0, "Accept: %d: %02x: ", nbytes, temp[0] & 0xff);
		for(i=1; i<nbytes;i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x", temp[i] & 0xff);
		L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
		if(nbytes < 2) 
		{	L_WARN_2(L_GENERAL, 0, "recv acc failed %d (%d)\n",
					nbytes, errno);
			return (-1);
		}
		if (temp[1] != YB_ACCEPT || !(temp[0] & (1<<Q_BIT)))
		{
			L_WARN_3(L_GENERAL, 0, "invalid ac %d: %02x %02x\n",
				nbytes, temp[0] & 0xff, temp[1] & 0xff);
			(void) strcpy(reason, "sun/netopen: invalid accect packet");
			(void) close(fid);
			return (-1);
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
{	USER_DATA_DB	o_p;
	while (ioctl (fid, X25_RD_USER_DATA, &o_p) == 0)
	{	int i;
		L_DEBUG_1(L_LOG_OPEN, 0, "User data: %d: ", o_p.datalen);
		for(i=0; i<o_p.datalen;i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x", o_p.data[i] & 0xff);
		L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
		if (o_p.datalen < 124) break;
	}
}

char *sprintf_diag(buff, diag)
char *buff;
X25_CAUSE_DIAG *diag;
{
  if (diag->flags & ~((1 << RECV_DIAG) | (1 << DIAG_TYPE)))
	sprintf(buff, " (flags %02x)", diag->flags); while(*buff) buff++;
  if (!(diag->flags & (1<<RECV_DIAG)))
	sprintf(buff, " (no info)"); while(*buff) buff++;
  sprintf(buff, " %s:", (diag->flags & (1<<DIAG_TYPE)) ? "clear" : "reset");
  while(*buff) buff++;
  sprintf_clr(buff, (char *)diag->data, diag->datalen);
  return buff;
}

sprintf_clr(buff, data, len)
char *buff;
char *data;
{ int i = 0;
  int facil = 0;

  if (i <= len-2)
  { int inc;
    sprintf(buff, " %02x diag %02x", data[i], data[i+1]);
    i += 2;
    while(*buff) buff++;

    i += (inc = sprintf_dtes(buff, data + i, len -i));
    while(*buff) buff++;

    if (inc && i <= len-1 && data[i] <= len-i)
    { facil = data[i];
      i++;
      sprintf(buff, (facil) ? " Facilities %d:" : " No Facilities", facil);
      while(*buff) buff++;
    }

    i += sprintf_facil(buff, data + i, facil);
    while(*buff) buff++;

    if (i < len)
    { sprintf(buff, " Data:");
      while(*buff) buff++;
      for(; i<len; i++)
      { sprintf(buff, " %02x", data[i]);
        while(*buff) buff++;
      }
    }
  }
}


sprintf_dtes(buff, data, len)
char *buff;
unsigned char *data;
{ if (len > 0)
  { int la1 =  data[0]       & 0x0f;
    int la2 = (data[0] >> 4) & 0x0f;
    int bytes = (la1+la2+1)/2;
    if (len > bytes)
    {	int i;
	sprintf(buff, " Addresses %01x/%01x: ", la1, la2);
	while(*buff) buff++;
        for(i=1; bytes; i++, bytes--)
	{ sprintf(buff, "%02x%s", data[i], (bytes == (la2/2)+1) ? " " : "");
	  while(*buff) buff++;
	}
        return i;
    }
  }
  return 0;
}

sprintf_facil(buff, data, len)
char *buff;
unsigned char *data;
{ int i;
  int skip = 0;

  for (i=0; i<len; i++)	if (skip)
  {	skip--;
	sprintf(buff, " %02x", data[i]);
	while(*buff) buff++;
  }
  else
  {	int args;

	switch (data[i] & 0xc0)
	{	case 0x00:	args = 1;			break;
		case 0x40:	args = 2;			break;
		case 0x80:	args = 3;			break;
		case 0xc0:	args = data[i+1] +1;		break;
	}

	if (args > (len -i))	goto defalt;

        switch(data[i])
	{
	case 0x00:
	if ((data[i+1] != 0x00) && (data[i+1] != 0xff))	goto defalt;
	sprintf(buff, " NOM call%s", (data[i+1]) ? "ed" : "ing");
	i += args;
	break;

	case 0xc1:
	if (args != 5)	goto defalt;
	sprintf(buff, " Call Duration %02x%02x%02x%02x",
		data[i+2], data[i+3], data[i+4], data[i+5]);
	i += args;
	break;

	case 0xc2:
	if (args != 9)	goto defalt;
	sprintf(buff, " Call stats %02x%02x%02x%02x %02x%02x%02x%02x",
		data[i+2], data[i+3], data[i+4], data[i+5],
		data[i+6], data[i+7], data[i+8], data[i+9]);
	i += args;
	break;

	defalt:
	sprintf(buff, " invalid");
	while(*buff) buff++;
	default:
	sprintf(buff, " facil %02x len %d", data[i], args);
	skip = args;
	}
	while(*buff) buff++;
  }
  if (skip) sprintf(buff, " -- invalid facilities %d byte%s missing",
		skip, (skip == 1) ? "" : "s");
  return i;
}
