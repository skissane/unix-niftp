/* unix-niftp lib/pqproc/record.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:58 $ */
#include "ftp.h"

/*
 * this file contains routines to deal with records
 * N.B. Does not include code to deal with data control commands.
 * $Log: record.c,v $
 * Revision 5.6  1991/06/07  17:01:58  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:17  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:50:57  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:49:39  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

/*
 * there seems to be a big problem with core fragmentation and the record
 * buffer is currently the main suspect. Therefore I have allocated a seperate
 * buffer for it when doing normal transfers. It probarbly will almost always
 * be used. ( It is small enough not to bother me too much ).
 */

/*local buffer probarbly always used*/
static unsigned char _l_buff[MAXSUBRECSIZ+1];

unsigned char    *rec_buf;
char    last_cc,end_r;          /* used by r_rec_byte() */

/*
 * two macro definitions for adding a character to the output
 * should improve the throughput a bit.
 *  The idea is that since on most calls to net_getc and net_putc
 * all that happens is that the character is got from or put into a buffer
 * then this operation is done in line instead of in a function. This saves
 * the overhead of a function call.
 *  If functions are really needed don't define MACROS.
 */
#ifdef  MACROS

#define netgetc() ( (net_rcount < net_io.read_count ) ? \
			(net_io.read_buffer[net_rcount++]&MASK):\
				net_getc())

#define netputc(c)  { if(net_wcount <net_io.write_count) \
			net_io.write_buffer[net_wcount++] = (c); \
		     else \
			net_putc(c); \
		     }

#else

#define netgetc()       net_getc()
#define netputc(c)      net_putc(c)

#endif

/*
 * flag for compression - if -1 no compression possible, if zero compression
 * possible but not doing it yet. 1 possible and currently compressiong
 * assume that can only pack 8-bit data bytes and not multibyte data.
 */

int     compress;

/*
 * flag to say wether a buffer ( not my local buffer ) has been allocated
 * for the sub record
 */

int     mcored;

/*
 * Initialise a record. Used to start a record.
 * This sets up the buffer and some of the counters
 */

init_rec()
{
	if(max_b_siz != wrd_bin_siz || ! rec_buf){  /* get a correctly sized buffer */
		if(mcored){             /* for the sub record */
			if (rec_buf) free(rec_buf);
			mcored = 0;
		}
		if(wrd_bin_siz == 1)
			rec_buf = _l_buff;
		else  {
			rec_buf = (unsigned char *)
				malloc(wrd_bin_siz * MAXSUBRECSIZ + 1);
			mcored++;
		}
		max_b_siz = wrd_bin_siz;
	}
	recp = recst = rec_buf;
	*recp++ =0;             /* zero first byte of record */
	reclen=0;               /* record length */
	sreclen=1;
	wdcount=0;
	wdbcount=0;

	/* if can compress say so here */

	if(direction){
		if(code == 0 && (facilities & TCOMPRESS)){
			compress = 0;   /* can compress text */
			return;
		}
		else if(code==1 && wrd_bin_siz==1 && (facilities &BCOMPRESS)){
			compress = 0;   /* can compress binary */
			return;
		}
	}
	compress = -1;                  /* no compression allowed */
}

/*
 * add a character to a sub record - now transmits compressed data as well
 * This routine has been optimised for speed. It is this routine that used
 * to slow it all down.
 */

add_to_rec(c)
int     c;
{
	register unsigned char   *tp = recst;    /* speed it up a bit put a */
	register int sc;                 /* couple of values into registers */
	register unsigned char   *xp;

	if(wrd_bin_siz != 1){
		/*
		 * wordsize is > 8 bits i.e. binary. special case it
		 */
		*recp++ = c;
		sreclen++;
		if(++wdbcount < wrd_bin_siz)
			return;
		wdbcount=0;             /* another word is in the buffer */
		reclen++;                       /* increment record length */
		(*tp)++;                        /* increment count */
		if(++wdcount >= MAXSUBRECSIZ)   /* buffer full */
			goto flush_it;
		return;
	}
/*
 * get here if got 8 bit data
 */
	reclen++;                       /* increment record length */
	if(compress < 0){               /* can't compress */
		*recp++ = c;            /* this is the usual route */
		sreclen++;
		(*tp)++;                /* increment count */
		if(++wdcount >= MAXSUBRECSIZ)    /* buffer full */
			goto flush_it;
		return;
	}
/*
 * get here only if possible to compress e.g. 8-bit data only
 */
	if(!wdcount){                   /* start of sub-rec */
		*recp++ = c;
		sreclen++;
		(*tp)++;                /* increment count */
		wdcount++;
		compress = 0;           /* can start compressing */
		return;
	}
	if(compress > 0){               /* we are currently compressing */
		if( (char)c == *(recp-1)){      /* ok still compressing */
			(*tp)++;
			if(++wdcount >= MAXSUBRECSIZ)
				goto flush_it;
			return;
		}
		/* No more compression - flush out all compressed data */
		sc = sreclen;
#ifdef  MACROS
		if(sc + net_wcount <= net_io.write_count){
			xp = net_io.write_buffer + net_wcount;
			net_wcount += sc;
			do{
				*xp++ = *tp++;
			}while(--sc);
		}
		else
#endif
		    do{
			netputc(*tp++);
		    }while(--sc);
		recp = recst;
		*recp++ =1;
		*recp++ =c;
		sreclen=2;
		wdcount=1;
		compress=0;
		return;
	}
/*
 * here we are not as yet compressing but we might be able to
 */
	else if((char)c != *(recp-1)){ /* not compressing + don't want to*/
		*recp++ =c;
		sreclen++;
		(*tp)++;
		if(++wdcount >= MAXSUBRECSIZ)
			goto flush_it;
		return;
	}
	else {  /* can compress now. flush out all unneeded bytes */
		if(wdcount != 1){
			sc = --sreclen;
			(*tp)--;
#ifdef  MACROS
			if(sc + net_wcount <= net_io.write_count){
				xp = net_io.write_buffer + net_wcount;
				net_wcount += sc;
				do{
					*xp++ = *tp++;
				}while(--sc);
			}
			else
#endif
			    do{
				netputc(*tp++);
			    }while(--sc);
			recp = recst;
			*recp++ =1;
			*recp++ =c;
			sreclen=2;
			wdcount=1;
		}
		compress++;
		(*recst)++;
		*recst |= COMPRESS_BIT;
		wdcount++;
	}
	return;

/*
 * get here is we are about to flush some data. sc and tp + recp
 * must be set up already. IF MACROS is set then we can do a direct copy
 * into the output buffer. Much quicker.
 */

flush_it:;
	sc = sreclen;
	recp = recst;
	sreclen=1;
	wdcount=0;
#ifdef  MACROS
	if(sc + net_wcount <= net_io.write_count){
		xp = net_io.write_buffer + net_wcount;
		net_wcount += sc;
		do{
			*xp++ = *tp++;
		}while(--sc);
	}
	else
#endif
	    do{
		netputc(*tp++);
	    }while(--sc);
	*recp++ = 0;
}

/*
 * add a whole block of data to a subrecord.
 * This is done to improve performance by reducing the amount of data movement
 * that is required.
 */

add_bto_rec(count,block)
int     count;
register unsigned char   *block;
{
	register unsigned char   *tp = recst;    /* speed it up a bit put a */
	register int sc;		 /* couple of values into registers */
	register unsigned char   *xp;

	if(wrd_bin_siz == 1)            /* do it here */
		reclen += count;
top:;
	if(compress < 0){               /* can't compress */
		if(wrd_bin_siz == 1){   /* single bytes. quickly does it */
			xp = block;
			do{
				*recp++ = *block++;
				if(++wdcount >= MAXSUBRECSIZ){/* buffer full*/
					sc = block - xp;
					(*tp) += sc;
					sreclen += sc;
					goto flush_it;
				}
			}while(--count);
			sc = block - xp;
			(*tp) += sc;
			sreclen += sc;
			return;
		}
		/*
		 * wordsize is > 8 bits i.e. binary.
		 */
		do{
			*recp++ = *block++;
			sreclen++;
			if(++wdbcount < wrd_bin_siz)
				continue;
			wdbcount=0;     /* another word is in the buffer */
			reclen++;               /* increment record length */
			(*tp)++;                /* increment count */
			if(++wdcount >= MAXSUBRECSIZ)   /* buffer full */
				goto flush_it;
		}while(--count);
		return;
	}

/*
 * get here only if possible to compress e.g. 8-bit data only
 */

	if(!wdcount){                   /* start of sub-rec */
		*recp++ = *block++;
		sreclen++;
		(*tp)++;                /* increment count */
		wdcount++;
		compress = 0;           /* can start compressing */
		if(!--count)
			return;
	}

	xp = recp-1;
	if(compress > 0){       /* we are currently compressing */
again1:;
		while(*block == *xp){   /* ok still compressing */
			(*tp)++;
			block++;
			if(++wdcount >= MAXSUBRECSIZ)
				goto flush_it;
			if(!--count)
				return;
		}
		/* No more compression - flush out all compressed data */
		sc = sreclen;
#ifdef  MACROS
		if(sc + net_wcount <= net_io.write_count){
			xp = net_io.write_buffer + net_wcount;
			net_wcount += sc;
			do{
				*xp++ = *tp++;
			}while(--sc);
		}
		else
#endif
		    do{
			netputc(*tp++);
		    }while(--sc);
		tp = recp = recst;
		*recp++ =1;
		xp = recp;
		*recp++ = *block++;
		sreclen=2;
		wdcount=1;
		compress=0;     /* turn off compression */
		if(!--count)
			return;
	}
/*
 * here we are not as yet compressing but we might be able to. compress == 0
 * this is the loop which is the limiting factor for speed.
 */
	for(sc = 0 ;*block != *xp++ ;){ /* not compressing + don't want to */
		*recp++ = *block++;
		sc++;
		if(++wdcount >= MAXSUBRECSIZ || !--count){
			sreclen += sc;
			(*tp) += sc;
			if(!count)
				return;
			goto flush_it;
		}
	}
	sreclen += sc;
	(*tp) += sc;

	/* can compress now. flush out all unneeded bytes */
	if(wdcount != 1){
		sc = --sreclen;
		(*tp)--;
#ifdef  MACROS
		if(sc + net_wcount <= net_io.write_count){
			xp = net_io.write_buffer + net_wcount;
			net_wcount += sc;
			do{
				*xp++ = *tp++;
			}while(--sc);
		}
		else
#endif
		    do{
			netputc(*tp++);
		    }while(--sc);
		tp = recp = recst;
		*recp++ =1;
		*recp++ = *block++;
		sreclen=2;
		wdcount=1;
	}
	else
		block++;
	compress++;
	(*tp)++;
	*tp |= COMPRESS_BIT;
	wdcount++;
	if(!--count)
		return;
	xp = recp-1;
	goto again1;

/*
 * get here is we are about to flush some data. tp + recp
 * must be set up already. IF MACROS is set then we can do a direct copy
 * into the output buffer. Much quicker.
 */

flush_it:;
	sc = sreclen;
	sreclen=1;
	wdcount=0;
#ifdef  MACROS
	if(sc + net_wcount <= net_io.write_count){
		xp = net_io.write_buffer + net_wcount;
		net_wcount += sc;
		do{             /* should be a single vax instruction */
			*xp++ = *tp++;
		}while(--sc);
	}
	else
#endif
	    do{
		netputc(*tp++);
	    }while(--sc);
	tp = recp = recst;
	*recp++ = 0;
	if(--count)             /* if more to do. try again. */
		goto top;
}


/*
 * terminate a record. Set the END OF RECORD bit and flush buffer.
 * Also clear all pointers
 */

end_rec()
{
	register unsigned char   *tp = recst;
	register sc = sreclen;

	*tp |= END_REC;

#ifdef  MACROS
	if(sc + net_wcount <= net_io.write_count){
		register unsigned char   *xp = net_io.write_buffer + net_wcount;
		net_wcount += sc;
		do{
			*xp++ = *tp++;
		}while(--sc);
	}
	else
#endif
	    do{
		netputc(*tp++);
	    }while(--sc);
	zap_record();
}

/*
 * force an end of a subrecord - This is used when marks are needed at this
 * point. The checks are so that it can be called from send_comm which
 * can be ( and is ) used on the recievers side. Strange results
 * occur otherwise
 */

end_sub_rec()
{
	register unsigned char   *tp;
	register sc;

	if(!(tp = recst) )
		return;
	if( (sc = sreclen) == 1) /* this is so that it will not output zero */
		return;
#ifdef  MACROS                  /* fast block move into output buffer */
	if(sc + net_wcount <= net_io.write_count){
		register unsigned char   *xp = net_io.write_buffer + net_wcount;
		net_wcount += sc;
		do{                   /* should be a single vax instruction */
			*xp++ = *tp++;
		}while(--sc);
	}
	else
#endif
	    do{
		netputc(*tp++);
	    }while(--sc);
	recp = recst;
	*recp++ =0;
	sreclen=1;
	wdcount=0;
}

/* clear all pointers so that there is no data comming or going */

zap_record()
{
	recst=0;
	rrecst=0;
	wrd_bin_siz = 1;        /* always reset so commands will work ok */
	last_cc=0;
	end_r=0;
}

/*
 * here is the routine for receiving a record.
 * It does not use the same variables as the sending of a record routines.
 * r_rec_byte can return any of the following things.
 *      1) A data octet ( Not sign extended ).
 *      2) A flag to say that there has been a level 2 command. This is so
 *         that higher level routines can change the state if need be.
 *      3) A flag to say that this is the end of the record.
 *      4) -1 if in the special case of it being in the wait for STOPACK
 *         state there is a network failure. This is to allow for
 *         compatability with version 77 systems.
 *  The flag bits are in the upper half of a word (PDP-11 terminology )
 *  so cannot be confused with data.
 *
 *      This routine has been modified to reduce the amount of data movement
 *      It now looks more ugly but then who cares if it is a lot faster.
 *         The only thing that slows it down is the speed of the processor
 */

r_rec_byte()
{
	register unsigned char   *ptr,*p;
	register int    l,j,k,i,c = 0;

	if(last_cc){            /* at end of sub record and this is the last*/
		last_cc =0;     /* of the record */
		if(end_r){
			end_r =0;
			return(END_REC_BIT);
		}
	}

	if(!rrecst){            /* no data in buffer */
		last_cc = 0;    /* say not last of sub-record */
		end_r =0;       /* for safeties sake */
	again:
		for(;;){
			if( (c = netgetc()) == -1){
				if(tstate == STOPACKs)
					return(-1);
				continue;       /* failure. lower routines */
			}                       /* dealt with it */

			if((i = (c & 077)) != 0)/* get size of buffer */
				break;
			if(c & END_REC)         /* Yeuch !! */
				return(END_REC_BIT);

			if(c & COMPRESS_BIT)    /* York Bug fix */
				continue;
						/* otherwise got a level 2 */
			if((k = tcc_command()) != 0){/* decode command */
				if(k==TCC_COMMAND)
					return(TCC_COMMAND);
				else if(tstate == STOPACKs)
					return(-1);
			}
		}

  /* if the size of the buffer is not the right size. Make it right */
		if(max_b_siz != wrd_bin_siz){
			if(mcored){
				free(rec_buf);
				mcored = 0;
			}
			if(wrd_bin_siz == 1)    /* to stop fragmentation */
				rec_buf = _l_buff;
			else {
				rec_buf = (unsigned char *)
					malloc(wrd_bin_siz*MAXSUBRECSIZ+1);
				mcored++;
			}
			max_b_siz = wrd_bin_siz;
		}

		rrecptr = rrecst = rec_buf;     /* start of buffer */
		rsreclen = i * wrd_bin_siz;     /* number of words in record*/

		/*
		 * there is a problem with non compressed data getting
		 * confused with compressed data. never accept the
		 * COMPRESS_BIT if we didn't negotiate it.
		 */
		if(c & COMPRESS_BIT){
			if( (facilities & (TCOMPRESS|BCOMPRESS)) == 0){
				L_WARN_0(L_GENERAL, 0, "Bad compress bit\n");
				c &= ~COMPRESS_BIT;
			}
		}
		if(!(c & COMPRESS_BIT)){        /* Not compressed data */
#ifdef  MACROS
			/*
			 * If all in the buffer, then just change pointers
			 */
			if(net_io.read_count - net_rcount >= rsreclen){
				rrecst = net_io.read_buffer + net_rcount;
				rrecptr = rrecst;
				net_rcount += rsreclen;
			}
			else            /* slow move into buffer */
#endif
				for(ptr = rrecst, j = rsreclen;  j ; j--)
					if( (k = netgetc()) != -1)
						*ptr++ = k;
					else
						goto no_go;
		}
		else if(wrd_bin_siz == 1){      /* compressed - usually */
			/*
			 * Since the other half ( above ) only compresses
			 * single bytes then this is usual.
			 * Just get the byte and then expand into the buffer
			 */
			j = i;
			ptr = rrecst;
			if( (k = netgetc()) == -1)
				goto no_go;
			else do {
				*ptr++ = k;
			     }while(--j);
		}
		else {                          /* multi word compression */
			/*
			 * Which system is sending me this ?? Oh well,
			 * a relatively efficient expansion
			 */
			ptr = rrecst;
			for(l = wrd_bin_siz ; l ; l--){ /*Get the first word*/
				if( (k = netgetc()) != -1)
					*ptr++ = k;
				else
					goto no_go;
			}
			for(j = i-1 ; j ; j--)  /* then expand into buffer */
				for(p=rrecst,l=wrd_bin_siz; l ;l--)
					*ptr++ = *p++;
		}
		if(c & END_REC)         /* say this is last of the record */
			end_r = 1;
	}
/*
 * at this point got rrecst pointing to a buffer
 * rrecptr is a pointer to the buffer
 */

	if(!--rsreclen){
		rrecst =0;
		last_cc =1;     /* say buffer is empty */
	}

	if(c) L_LOG_4(L_RECV_NET, 0, "Record %02x/%3d: %02x ... %02x\n",
		c, c & 0x3f, rrecptr[0], rrecptr[(c&0x40) ? 0 : (c&0x3f)-1]);
	return(*rrecptr++ & MASK);

/*
 * Get here if netgetc() returns an error.
 */
no_go:
	rrecst=0;
	if(tstate != STOPACKs)
		goto again;
	return(-1);
}

/*
 * small routine to read a single level 2 command. This is only used by
 * the transmitter.
 */

get_comm()
{
	register int    c;

	for(;;){
		while((c = netgetc()) == -1)
			L_LOG_0(L_RECV_NET, 0, "get_comm() found a -1\n");
		if(c){
			if(tstate != ESe)       /* don't want data here */
				prot_err(125, c, c);
			continue;
		}
		if( tcc_command() != -1)
			return;
	}
}
