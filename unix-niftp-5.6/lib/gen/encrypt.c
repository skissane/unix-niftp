/* unix-niftp lib/gen/encrypt.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:43 $ */
/*
 * $Log: encrypt.c,v $
 * Revision 5.6  1991/06/07  17:00:43  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:27  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:44:02  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:35:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"

#ifndef	CRUDDY_CRYPT_ONLY
#include <des.h>

#ifdef KERBEROS
extern des_cblock default_ivec;
extern unsigned char *ivec;
#endif

#ifndef	MAX_ENC
#define	MAX_ENC	199
#endif	/* MAX_ENC */

#ifndef	DES_HDR
#define	DES_HDR 2
#endif	/* DES_HDR */

#ifndef	DES_JUNK
#define	DES_JUNK	4
#endif	/* DES_JUNK */

#ifndef	DES_HDR_1C
#define	DES_HDR_1C '+'
#endif	/* DES_HDR_1C */

extern Key_schedule key;

#ifndef	succ
#define	succ(x) ((x) * 13)
#endif	/* succ */
#ifndef	succ
static long
succ(val)
{	return val * 13;
}
#endif	/* succ */

char *
docrypt(inp)
register char *inp;
{
	static char res[(MAX_ENC + 8 + 1) * 2 + 1 + DES_HDR + (DES_JUNK*2)];
	char buff[MAX_ENC+8+1 + (DES_JUNK*2)];
	int bytes;
	int i;
	char *ptr;
	long now = time((long *) 0);

	for(i=0; i<sizeof buff; i++) { buff[i] = now & 0xff; now = succ(now); }

	for (i=0, ptr = inp; '0' <= *ptr && *ptr <= '9'; ptr++)
		i = (i * 10) + *ptr - '0';
	if (ptr != inp && *ptr == '.') inp = ptr +1;
	bytes = strlen(inp) + 1;
	if (bytes > MAX_ENC) return (char *) 0;
	strcpy(buff+DES_JUNK, inp);
	bytes += (DES_JUNK*2) + 8 - (bytes % 8);
#ifdef KERBEROS
	des_cbc_encrypt(buff, buff, bytes, &key, ivec, DES_ENCRYPT);
#else
	des_cbc_encrypt(buff, buff, bytes, &key, 0, DES_ENCRYPT);
#endif
	sprintf(res, "0%c", DES_HDR_1C);
	for(i=0; i<bytes; i++) sprintf(res+DES_HDR+ 2*i, "%02x", buff[i]&0xff);
	return res;
}
#else	/* CRUDDY_CRYPT_ONLY */
#define ECHO	010
#define ROTORSZ 256
#ifndef MASK
#define MASK    0377
#endif

static char     t1[ROTORSZ];
static char     t2[ROTORSZ];
static char     t3[ROTORSZ];

char kkbuf[13];

static
seedkey(seed)
long seed;
{	register ic, i, k, temp;
	unsigned random;

	for (i=0; i<13; i++) seed = seed*kkbuf[i] + i;

	for(i=0;i<ROTORSZ;i++){ t1[i] = i; t2[i] = 0; t3[i] = 0; }
	for(i=0;i<ROTORSZ;i++){
		seed = 5*seed + kkbuf[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK)%(k+1);
		random >>= 8;
		temp = t1[k]; t1[k] = t1[ic]; t1[ic] = temp;
		if(t3[k]!=0) continue;
		ic = (random&MASK) % k;
		while(t3[ic]!=0) ic = (ic+1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for(i=0;i<ROTORSZ;i++) t2[t1[i]&MASK] = i;
}

char *
docrypt(inp)
register char *inp;
{
	register n1=0, n2=0, i;
	register char *q, *op;
	static  char outbf[200];

	seedkey( (long)13 );
	/* Get the output buffer */

	op = outbf;
	*op = '\0';
	for(q = inp; *q ; q++){
		i = (t2[(t3[(t1[(*q+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1)&MASK;
		*op++ = i%26 + 'a';
		*op++ = i/26 + 'a';
		n1++;
		if(n1==ROTORSZ) { n1 = 0; n2++; if(n2==ROTORSZ) n2 = 0; }
	}
	*op = 0;
	return(outbf);
}
#endif	/* CRUDDY_CRYPT_ONLY */
