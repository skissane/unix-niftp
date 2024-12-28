/*
 * rcsid[] = "$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/yorkbox/hhall.h,v 5.5 90/08/01 13:39:33 pb Exp $";
 *	YORK UNIX-X25 COMMUNICATIONS SOFTWARE
 *
 * These are things common to ALL programs which use yb at all
 *
 *	Copyright (c) 1986 The University of York, the Science and
 *	Engineering Research Council, and University College London.
 *
 *	$State: Exp $
 *	$Log:	hhall.h,v $
 * Revision 5.5  90/08/01  13:39:33  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:57:09  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:58:46  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 * Revision 22.2  86/08/29  07:37:31  pb
 * Re-instate the DEBUG code.
 * 
 *
 *	Revision 22.1	Release 2.2
 *
 */

/* Lets just get it once ...... */
#ifndef	HHALL_GOT
#define	HHALL_GOT

#ifndef	FILE
#include <stdio.h>
#endif	FILE

#ifdef	SMALL_PROC
typedef	char	FLAG;		/* for size... */
#else
typedef	int	FLAG;		/* for speed... */
#endif	SMALL_PROC

/* Failure reasons .... */

#define	TNOCHAN	1
#define	TIOERR	2
#define	TNETDWN	3
#define	TTIMOUT	4
#define	TNOBLK	5
#define	TADRER	6
#define	TCALREJ	7
#define	TACCERR	8
#define	TFILERR	9
#define	TABORT	10
#define	TNOFUNDS 11
#define	TCONGESTED 12
#define	TNOTSADDR 13		/* No '/' in TS address */
#define	TNOADDR 14		/* called or calling == 0 */
#define	TUNKNOWNADDR 15		/* Cannot map it to a valid address */
#define	TTSCUDF 16		/* You Cant have a CUDF in YBTS */
#define	TCUDFTOOBIG 17		/* The supplied CUDF was too big to fit !!
				   */

#if	defined(HHCP)
/*
 *	these codes are meant to try and be compatible with sendmails, to
 *	aid in software portability between york niftp -> mmdf and
 *	york niftp -> sendmail - hence the weird values.
 */

/*
 *	Return value for OK reception - the NIFTP has excepted
 *	the file + address without problems
 */
#define JNT_OK		0

/*
 *	Temporary error - a retry may be successful
 */
#define JNT_TEMP	75

/*
 *	Return value for JNT Mail permanent error - total reject.
 */
#define JNT_PERM	68

#endif	HHCP

/*
 * string lengths & offsets for using spool file & sequence numbers
 * you must change these in a consistent fashion.
 */

#define NTREE	2		/* length of tree suffix number */
#define TFORMAT	"%02d"		/* this must match the chosen length of NTREE */
#define NSEQ	6		/* length of trans-id sequence number */
#define SFORMAT	"%06d"		/* this must match the chosen length of NSEQ */
#define TMPFILLEN 8		/* size of eg PA000123 */


/*
	Some attributes ...
*/
#if	defined(HHP)  || defined(HHQ)
#define	SPECIAL_OPTIONS	0x80

#define	Q_MONITOR	0x80	/* Monitor this	 */

#define	Q_FORMAT_MASK	0x30	/* The mask to select the format     */
#define	Q_UNKNOWN	0x00	/* Attribute unknown (value absent)   */
#define	Q_NOVALUE	0x10	/* No value available (value absent) */
#define	Q_INT		0x20	/* 16 bit value (integer or bitfield */
#define	Q_NUMBER	0x20	/* 16 bit value (integer or bitfield */
#define	Q_STR		0x30	/* String */
#define	Q_ALPHA		0x30	/* String */

/*	Operators for qualifiers	*/
#define	Q_EQ		0x02
#define	Q_LE		0x03
#define	Q_NE		0x05
#define	Q_GE		0x06
#define	Q_ANY		0x07

#define	Q_INT_EQ	(Q_INT | Q_EQ)
#define	Q_INT_LE	(Q_INT | Q_LE)
#define	Q_STR_EQ	(Q_STR | Q_EQ)
#endif	HHP || HHQ

#define SPOOLDLEN 15		/* size of /usr/spool/hhcp */

/* Let's use REAL types !! */
#define	CNULL	((char *) 0)
#define	FNULL	((FILE *) 0)

/*
 * The debugging package:
 * By default have it ON:  may be turned OFF in local.h
 */
#ifdef	NODEBUG
#undef	DEBUG
#define deb(n, rest)
#else	NODEBUG
#define	DEBUG
#endif	NODEBUG

#define	D_USER	0
#define	D_ADDR	1
#define	D_NET	2
#define	D_SIZE	(D_NET + 1)

#ifndef	D_DEF
#define	D_DEF	D_USER
#endif	D_DEF
#define	COMMA ,
int     debug_v[D_SIZE];
#define	debug			debug_v[D_DEF]

/*
	SO: you can do:	cc -DD_DEF=<x>
	or:
			#define	D_DEF	<x> in header
*/
/*
 *	Define a deb(n, text) macro, and COMMA (to be used in place of ',' in
 * all deb statements. Typical use is:
 *	deb(10, "Simple text");
 *	deb(20, "Complex text" COMMA arg1 COMMA arg2)
 */
#ifndef	NODEBUG
#define deb(n, rest)		if (debug > n) fprintf(stderr, rest)
#define	set_low_deb(x)		{ debug_v[D_ADDR] = x; debug_v[D_NET] = x; }
#define deb_l(l, n, rest)	if (debug_v[l]     > n) fprintf(stderr, rest)
#endif	NODEBUG

#ifdef BSD4_2
 /* 
  * JR fix for multiple groups on 4.2
  */
#define	setpwdgid(pwd) ( initgroups(pwd->pw_name, pwd->pw_gid) || setgid(pwd->pw_gid) )
#else
#define	setpwdgid(pwd)	setgid(pwd->pw_gid)
#endif

#ifndef	HHP
#ifndef	HHQ
#define	COLON	((char) (':' | 0x80))
#endif	HHQ
#endif	HHP

#include <ctype.h>

#define	ISADTE(x) (   (*(x)) && (  ((*(x) >= '0') && (*(x) <= '9')) || ( !strncmp(x,"aa00",4) )  )    )

#endif	HHALL_GOT
