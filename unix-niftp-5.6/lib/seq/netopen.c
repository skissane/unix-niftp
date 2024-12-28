/*
 * file:	netopen.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk> original for SunLink
 * Allan Black <allan@uk.ac.strath.cs> Sequent X.25 (Morningstar Technologies)
 * Alex <A.Sharaz@uk.ac.hull> Insert a calling (as well as called) address 
 *
 * This file contains routines to interface to the Symmetry X.25 board.
 * They make the network seem like /dev/abacus. Makes the interface more fun.
 */

#include "ftp.h"
#include "infusr.h"
#include "nrs.h"
#include <stdio.h>
#include <errno.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <x25/x25.h>
#include <x25/x25errno.h>

#define htobcd(x) ((x == 100) ? 0 : ( ((x / 10) << 4) | (x % 10)))

#define	MAXFS_BYTES	16

char	svcpath[14];	/* /dev/tty[xyzefg][1-9a-zA-E] */

int	reverse_charge;
int	fast_select;
int	recvpktsize;
int	sendpktsize;
int	recvwndsize;
int	sendwndsize;
int	recvthruput;
int	sendthruput;
int	cug;
int	rpoa;

extern char	pkt_buf[];
extern int	pkt_len;
char		rpl_buf[2051]; /* Max pkt size = 2048 */

extern fid;
extern	def_pktsize;
extern	def_wndsize;

char ybts_hdr[] = { 0x7f, 0xff, 0xff, 0xff };

extern	Nopts;	/* Network options */

char	*pkt_octet();
char	*index();
int	oobmsg();

con_open(str)
char    *str;
{
	register nodep nptr = &net_io;
	register i;
	int size;
	int pending_n = 0;

	char fromdte[ENOUGH];
	char dte[ENOUGH];
	char ybtsdata[ENOUGH];
	char *ybts, *calling, *ybtsp;
	char *top[2];
	int dtelen, ybtslen, callinglen, datalen;
	int callingdtelen;
	int firstoctet_mask = 0;

	int flength;
	char *flptr;

	if(net_open) L_WARN_0(L_10, 0, "con_open called with net_open\n");

	top[0] = fromdte;
	top[1] = t_addr;
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		return -1;
	}

	L_LOG_2(L_FULL_ADDR, 0, "Full address:- %s (from %s)\n",
		hide_pss_pw(t_addr), fromdte);

	strcpy(dte, t_addr);
	if(ybts = index(dte, '/')) {
		*ybts++ = '\0';
		ybtslen = strlen(ybts);
	} else
		ybtslen = 0;
	dtelen = strlen(dte);

	if(calling = index(fromdte, '/')){
		callingdtelen = calling-fromdte;
		calling++;
	}
	else{
        	calling="<unknown>";
		callingdtelen = 0;
        }
	callinglen = strlen(calling);
	L_LOG_1(L_GENERAL, 0, "Calling Dte (%d)\n", callingdtelen);

	nptr->read_count = 0;
	starttimer(2*60);		/* start the timeout system */
	alarm(2*60);		/* TEMP HACK til start timer is fixed */

	if((fid = x25open(svcpath, 0, 0, X25_ADAPTER, -1)) < 0) {
		L_LOG_1(L_GENERAL, 0, "x25open failed (%s)\n", x25error());
		return -1;
	}

	new_pkt(0, CALL_REQUEST);

	/* What we need is the called and calling addresses */
	pkt_octet((callingdtelen<<4)|(dtelen) & 0xff);
	/* NB: DTEs are BCD, so two per byte */
	/* First the called address */
	for(i = 0; i < dtelen; i +=2)
		pkt_octet(((dte[i] - '0') << 4) | ((dte[i+1] -'0') & 0xff));
	/* ... now the calling address (used to be 0) */
	for(i = 0; i < callingdtelen; i +=2)
	   pkt_octet(((fromdte[i] - '0') << 4) | ((fromdte[i+1] -'0') & 0xff));
	flptr = pkt_octet(0);

	reverse_charge = (trans_revc || tab.t_flags & REVERSE_CHARGING);

	if(trans_pkts)
		recvpktsize = trans_pkts+3;
	else if(def_pktsize) {
		for(recvpktsize = P_SZ_16; recvpktsize < P_SZ_1024; recvpktsize++)
			if((1 << recvpktsize) >= def_pktsize) break;
	} else
		recvpktsize = 0;
	sendpktsize = recvpktsize;

	if(trans_wind)
		recvwndsize = trans_wind;
	else if(def_wndsize)
		recvwndsize = def_wndsize;
	else
		recvwndsize = 0;
	sendwndsize = recvwndsize;

/*-	if(thruput..)	recvthruput	= ??; -*/
/*-	if(thruput..)	sendthruput	= ??; -*/
	recvthruput = 0;
	sendthruput = 0;

	if(trans_cug) cug = htobcd(trans_cug);

	if(Nopts & N_NFCS)
		fast_select = 0;
	else if(Nopts & N_FCS)
		fast_select = FAST_CR;
	else
		fast_select = FAST_SELECT;

/*-	if(rpoa ...)	rpoa_req=1, rpoa = ??; -*/
	rpoa = 0;

	if(def_pktsize || def_wndsize || trans_pkts || trans_wind ||
		(tab.t_flags & REVERSE_CHARGING) || trans_revc || trans_cug
)
		L_WARN_4(L_GENERAL, 0,
			"Request pkt=%d, window=%d, rec=%d, cug=%02x\n",
			sendpktsize, sendwndsize,
			reverse_charge, (cug) ? cug : 0xff);

	L_LOG_3(L_LOG_OPEN, 0, "-> rev chge %d, pkts=%d/%d, ",
		reverse_charge, recvpktsize, sendpktsize);
	L_LOG_4(L_LOG_OPEN, L_CONTINUE, "wnd=%d/%d, thru=%d/%d, ",
		recvwndsize, sendwndsize, recvthruput, sendthruput);
	L_LOG_3(L_LOG_OPEN, L_CONTINUE, "cug=%x, %s, rpoa=%x\n",
		cug ? cug : 0xff,
		(fast_select == 0) ? "no FS" :
		(fast_select == FAST_CR) ? "FS CLR" :
		(fast_select == FAST_SELECT) ? "FS ACC" : "??",
		(rpoa) ? rpoa : 0xffff);

	flength = 0;
	if(reverse_charge) {
		pkt_octet(CODE_CC);
		pkt_octet(COLLECT_CALL);
		flength += 2;
	}
	if(fast_select) {
		pkt_octet(CODE_FS);
		pkt_octet(fast_select);
		flength += 2;
	}
	if(cug) {
		pkt_octet(CODE_CUG);
		pkt_octet(cug);
		flength += 2;
	}
	if(recvwndsize) {
		pkt_octet(CODE_WS);
		pkt_octet(recvwndsize);
		pkt_octet(sendwndsize);
		flength += 3;
	}
	if(recvpktsize) {
		pkt_octet(CODE_PS);
		pkt_octet(recvpktsize);
		pkt_octet(sendpktsize);
		flength += 3;
	}
	*flptr = flength;

	datalen = ybtslen + callinglen + (sizeof ybts_hdr) + 2;
	/* If there's too much data for the open, split it */
	if(datalen > MAXFS_BYTES && fast_select == 0)
	{	pending_n = datalen - MAXFS_BYTES;
		datalen = MAXFS_BYTES;
		firstoctet_mask = 64;
	}
	ybtsp = ybtsdata;
	for(i = 0; i < sizeof ybts_hdr; i++) *ybtsp++ = ybts_hdr[i];
	*ybtsp++ = 0x80 | firstoctet_mask | ybtslen;
	for(i = 0; i < ybtslen; i++) *ybtsp++ = ybts[i];
	*ybtsp++ = 0x80 | callinglen;
	for(i = 0; i < callinglen; i++) *ybtsp++ = calling[i];
	if(ftp_print & L_LOG_OPEN)
	{	int i;
		L_DEBUG_1(L_LOG_OPEN, 0, "Open to DTE (%d) ", dtelen);
		for(i = 0; i < dtelen/2; i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x",
			pkt_buf[i+4] & 0xff);
		L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, ", call data (%d) ", datalen);
		if(pending_n)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "(+%d) ", pending_n);
		for(i = 0; i < datalen+pending_n; i++)
			L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x",
			ybtsdata[i] & 0xff);
		L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
	}

	for(i = 0; i < datalen; i++) pkt_octet(ybtsdata[i]);
	if(write(fid, pkt_buf, pkt_len) < 0) {
		L_WARN_1(L_GENERAL, 0, "write connect pkt failed (%s)\n",
			x25error());
		close(fid);
		return -1;
	}
	if((size = read(fid, rpl_buf, sizeof rpl_buf)) < 0) {
		L_WARN_1(L_GENERAL, 0, "read reply pkt failed (%s)\n",
			x25error());
		close(fid);
		return -1;
	}
	switch(rpl_buf[2]) {
	case CALL_ACCEPT:
		break;
	case CLEAR_INDICATION:
		L_WARN_2(L_GENERAL, 0, "call rejected %d/%d\n",
			rpl_buf[4] & 0xff, rpl_buf[5] & 0xff);
		close(fid);
		return -1;
	default:
		L_WARN_1(L_GENERAL, 0, "unknown PTI %02x\n", rpl_buf[2]);
		setmode(IOX25DATA); /* close will cause clear */
		close(fid);
		return -1;
	}

	signal(SIGBAND, oobmsg);

	if((def_pktsize || def_wndsize) && size > 3) {
		int newrecvwndsize = 0;
		int newsendwndsize;
		int newrecvpktsize = 0;
		int newsendpktsize;
		flptr = rpl_buf+3;
		i = (((*flptr & 0xf)+1)/2)+(((*flptr >> 4) & 0xf)+1)/2;
		flptr += i+1;
		flength = size-i-4;
		if(flptr < rpl_buf+size && (i = (*flptr & 0x3f))) {
			if(i > flength) i = flength;
			flptr++;
			while(i > 0) {
				switch(*flptr & 0xff) {
				case CODE_WS:
					newrecvwndsize = *++flptr & 0xf;
					newsendwndsize = *++flptr & 0xf;
					flptr++;
					i -= 3;
					break;
				case CODE_PS:
					newrecvpktsize = 1 << (*++flptr & 0xf);
					newsendpktsize = 1 << (*++flptr & 0xf);
					flptr++;
					i -= 3;
					break;
				case CODE_FS:
				case CODE_CUG:
					flptr += 2;
					i -= 2;
					break;
				default:
					flptr++;
					i--;
				}
			}
		}
		if(newrecvpktsize || newrecvwndsize)
		{	L_WARN_4(L_GENERAL, 0, "Negotiated %d/%d:%d/%d",
				def_pktsize, def_pktsize,
				def_wndsize, def_wndsize);
			L_WARN_4(L_GENERAL, L_CONTINUE, " -> %d/%d:%d/%d\n",
				newrecvpktsize, newsendpktsize,
				newrecvwndsize, newsendwndsize);
		}
	}

	/* opened -- can send non FS data */
	if(pending_n) {

		L_LOG_1(L_GENERAL, 0, "Send non FS data (%d)\n", pending_n);
		new_pkt(1, DATA_PACKET);
		pkt_octet(YB_CONNECT);
		for(ybtsp = ybtsdata+MAXFS_BYTES; pending_n-- > 0; ybtsp++)
			pkt_octet(*ybtsp);
		if(write(fid, pkt_buf, pkt_len) < 0)
			L_WARN_1(L_GENERAL, 0, "send of non FS CONNECT (%s)\n",
				x25error());
		/* Let something else cause an error ... */
	}

	/* Now await the accept */
	size = read(fid, rpl_buf, sizeof rpl_buf);
	L_DEBUG_4(L_LOG_OPEN, 0, "Accept: %d: %02x %02x %02x: ",
		size, rpl_buf[0] & 0xff, rpl_buf[1] & 0xff, rpl_buf[2] & 0xff);
	for(i = 3; i < size; i++)
		L_DEBUG_1(L_LOG_OPEN, L_CONTINUE, "%02x", rpl_buf[i] & 0xff);
	L_DEBUG_0(L_LOG_OPEN, L_CONTINUE, "\n");
	if(size < 4) 
	{	L_WARN_2(L_GENERAL, 0, "recv acc failed %d (%d)\n",
				size, errno);
		close(fid);
		return -1;
	}
	if(rpl_buf[3] != YB_ACCEPT || !(rpl_buf[0] & SET_Q))
	{
		L_WARN_1(L_GENERAL, 0, "invalid ac %d: ", size);
		L_WARN_4(L_GENERAL, L_CONTINUE, "%02x %02x %02x: %02x\n",
			rpl_buf[0] & 0xff, rpl_buf[1] & 0xff,
			rpl_buf[2] & 0xff, rpl_buf[3] & 0xff);
		close(fid);
		return -1;
	}
	/* WHEW !!!! */

	starttimer(11*60);		/* give it a chance now .... */
	nptr->write_count = BLOCKSIZ;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	net_open = 1;                   /* say we are open */
	return 0;
}

