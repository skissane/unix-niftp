/* unix-niftp lib/gen/setcrypt.c $Revision: 5.6.1.1 $ $Date: 1992/10/17 06:13:34 $ */
/*
 * $Log: setcrypt.c,v $
 * Revision 5.6.1.1  1992/10/17  06:13:34  pb
 * Remote ftp_print
 *
 * Revision 5.6  1991/06/07  17:01:06  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:35:29  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:46:31  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:36:07  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#define	KEYSIZE	13

#ifndef	CRUDDY_CRYPT_ONLY
#include "des.h"
Key_schedule	key;
#else	/* CRUDDY_CRYPT_ONLY */
#define	keystring kkbuf
#ifndef	CRUDDY_CRYPT_COMPAT
#define	CRUDDY_CRYPT_COMPAT
#endif	/* CRUDDY_CRYPT_COMPAT */
#endif	/* CRUDDY_CRYPT_ONLY */

#ifdef	CRUDDY_CRYPT_COMPAT
extern	char kkbuf[KEYSIZE];
#endif	/* CRUDDY_CRYPT_COMPAT */

extern  char	*KEYFILE;

#ifdef KERBEROS
des_cblock default_ivec = { 0x12,0x34,0x56,0x78,0x90,0xab,0xcd,0xef };
unsigned char *ivec = (unsigned char *)default_ivec;
#endif

setcrypt()
{
	register infd;
	/*
	 * Find the key file
	 */
#ifndef	CRUDDY_CRYPT_ONLY
	char keystring[KEYSIZE+1];
#endif	/* CRUDDY_CRYPT_COMPAT */
	if((infd = open(KEYFILE, 0)) < 0){
		L_WARN_1(L_ALWAYS, 0, "Cannot find encryption key in %s\n", KEYFILE);
		return;
	}
	if(read(infd, keystring, KEYSIZE) != KEYSIZE){
		L_WARN_1(L_ALWAYS, 0, "Read error on key from %s\n", KEYFILE);
		close(infd);
		return;
	}
	close(infd);
#ifndef	CRUDDY_CRYPT_ONLY
#ifdef	CRUDDY_CRYPT_COMPAT
	bcopy(keystring, kkbuf, KEYSIZE);
#endif	/* CRUDDY_CRYPT_COMPAT */
	{	C_Block bkey;
		keystring[KEYSIZE] = '\0';
		string_to_key(keystring, &bkey);
#ifdef KERBEROS
		des_key_sched(&bkey, &key);
#else
		des_set_key(&bkey, &key);
#endif	
	}
#endif
}
