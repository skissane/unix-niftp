/* unix-niftp lib/gen/decrypt.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:39 $ */
/*
 * $Log: decrypt.c,v $
 * Revision 5.6  1991/06/07  17:00:39  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 */

/* password decryption routines */
#include "ftp.h"

#ifndef	CRUDDY_CRYPT_ONLY
#include <des.h>
#endif

#ifdef KERBEROS
extern des_cblock default_ivec;
extern unsigned char *ivec;
#endif

#ifndef	MAX_ENC
#define	MAX_ENC	199
#endif	/* MAX_ENC */

#ifndef	DES_HDR
#define	DES_HDR	2
#endif	/* DES_HDR */

#ifndef	DES_JUNK
#define	DES_JUNK	4
#endif	/* DES_JUNK */

#ifndef	DES_HDR_1C
#define	DES_HDR_1C '+'
#endif	/* DES_HDR_1C */

#define hexval(x) (('0'<=x&&x<='9') ? x-'0' : ('a'<=x&&x<='f') ? x-'a'+10:-16)

#ifndef	CRUDDY_CRYPT_ONLY
extern Key_schedule key;
#else	/* CRUDDY_CRYPT_ONLY */
#ifndef	CRUDDY_CRYPT_COMPAT
#define	CRUDDY_CRYPT_COMPAT
#endif	/* CRUDDY_CRYPT_COMPAT */
#endif	/* CRUDDY_CRYPT_ONLY */

#ifdef	CRUDDY_CRYPT_COMPAT
#define ECHO    010
#define ROTORSZ 256
#ifndef MASK
#define MASK    0xFF
#endif	/* MASK */

static  char    t1[ROTORSZ];
static  char    t2[ROTORSZ];
static  char    t3[ROTORSZ];

char    kkbuf[13];
#endif	/* CRUDDY_CRYPT_COMPAT */

#ifdef	CRUDDY_CRYPT_COMPAT
static seedkey(seed)
long seed;
{	register ic, i, k, temp;
	unsigned random;

	for (i=0; i<13; i++) seed = seed*kkbuf[i] + i;
	for(i=0;i<ROTORSZ;i++){ t1[i] = i; t2[i] = 0; t3[i] = 0; }
	for(i=0;i<ROTORSZ;i++) {
		seed = 5*seed + kkbuf[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK)%(k+1);
		random >>= 8;
		temp = t1[k]; t1[k] = t1[ic]; t1[ic] = temp;
		if(t3[k]!=0) continue;
		ic = (random&MASK) % k;
		while(t3[ic]!=0) ic = (ic+1) % k;
		t3[k] = ic; t3[ic] = k;
	}
	for(i=0;i<ROTORSZ;i++) t2[t1[i]&MASK] = i;
}
#endif	/* CRUDDY_CRYPT_COMPAT */

char *
decrypt(inp)
register char *inp;
{
	static char res[MAX_ENC + 8 + 1 + (DES_JUNK * 2)];
#ifndef CRUDDY_CRYPT_ONLY
	char buff[MAX_ENC + 8 + 1];
	int i;
#endif
	int bytes = strlen(inp);

#ifndef	CRUDDY_CRYPT_ONLY
	if (bytes == DES_HDR || ((bytes -DES_HDR) & 15) || inp[1] != DES_HDR_1C)
#endif	/* CRUDDY_CRYPT_ONLY */
#ifdef	CRUDDY_CRYPT_COMPAT
	{	int l;
		register n1 = 0, n2 = 0;
		register char *p, *cp;
		L_LOG_2(L_GENERAL, 0, "Use cruddy crypt as pw=%d, c=%x\n",
			bytes, inp[1]);
		seedkey( (long) 13);

		for(cp = res, p = inp; *p ;){
			l = (*p++ - 'a');
			l += ((*p++ - 'a')*26);
			*cp++ = (t2[(t3[(t1[(l+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1)&0x7f;
			n1++;
			if(n1==ROTORSZ) {
				n1 = 0;
				n2++;
				if(n2==ROTORSZ)
					n2 = 0;
			}
		}
		*cp = '\0';
		return(res);
	}
#else	/* CRUDDY_CRYPT_COMPAT */
	{	L_LOG_0(L_GENERAL, 0,
			"Not compiled with compat, but old data\n");
		return (char *) 0;
	}
	L_LOG_0(L_GENERAL, 0, "Always use des as no compat!!\n");
#endif	/* CRUDDY_CRYPT_COMPAT */

#ifndef	CRUDDY_CRYPT_ONLY
	L_LOG_2(L_GENERAL, 0, "Use des as pw=%d, c=%x\n", bytes, inp[1]);
	inp   += DES_HDR;
	bytes -= DES_HDR;
	bytes /= 2;
	if (bytes > MAX_ENC + 8 + 1 + (DES_JUNK * 2))
	{	L_LOG_2(L_GENERAL, 0, "Bytes=%d (>%d)\n",
			bytes, MAX_ENC + 8 + 1 + (DES_JUNK * 2));
		return (char *) 0;
	}
	for (i=0; i<bytes; i++)
	{	int res = (hexval(inp[i*2]) * 16) + hexval(inp[i*2 +1]);
		if (res < 0)
		{	L_LOG_3(L_GENERAL, 0, "%x and %x gave %x\n",
				inp[i*2], inp[i*2 +1], res);
			return (char *) 0;
		}
		buff[i] = res;
	}
#ifdef KERBEROS
	des_cbc_encrypt(buff, res, bytes, &key, ivec, DES_DECRYPT);
#else
	des_cbc_encrypt(buff, res, bytes, &key, 0, DES_DECRYPT);
#endif
	return res + DES_JUNK;
#endif	/* CRUDDY_CRYPT_ONLY */
}
