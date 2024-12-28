case $CONFIG in
    '') . config.sh ;;
esac
case "$softpp" in '')	softpp=undef;; esac
case "$sigret" in '')	sigret=int;;   esac
outputfile=h/opts.h
rm -f $outputfile.$$
echo "Creating $outputfile (with variable substitutions)"
if cat >$outputfile.$$ <<!GROK!THIS!
/*
 * various options for the ftp
 */

#$novoid	void	int	/* use void if that's what is wanted */
#define	SIGRET	$sigret	/* what a signal routine returns */
#$strchr	index	strchr	/* Cultural */
#$strchr	rindex	strrchr	/* Differences */
#$novfork	vfork	fork	/* use vfork if that is what is wanted */
#$ascii	ASCII		/* This machine is ascii */
#$getwd	GETWD		/* has getwd built in */
#$bzero	BZERO		/* has bzero call */
#$bcopy	BCOPY		/* has bcopy call */
#$rename	RENAME		/* has rename built in */
#$fopena	FOPEN_A_APPENDS	/* fopen "a" really DOES append */
#$flock	FLOCK		/* Using BSD4.2 flock file locking */
#$fcntl	FCNTL		/* Using SYSV fcntl file locking */
#$uchar	UCHAR		/* <sys/types.h> defines u_char */
#$guest	GUEST_ACCOUNTS	/* allow guest operations */
#$restartsys	RESTARTSYS	/* restarts system calls */
#$dircompacts	DIR_COMPACTS	/* directory compacts when file is deleted */
#$getgroups	GETGROUPS	/* system has muliple groups */
#$creatrunc	CREATRUNC	/* can create & truncate while opening */
#$setlinebuf	SETLINEBUF	/* can use 4.2 setlinebuf() call */
#$setvbuf	SETVBUF		/* can use System V setvbuf() call */
#$seteuid	SETEUID		/* can use 4.2 seteuid() call */
#$usendir	USENDIR		/* include ndir.c? */
#$libndir	LIBNDIR		/* include /usr/include/ndir.h? */
#$cryptcruddy	CRUDDY_CRYPT_ONLY /* Cruddy crypt only */
#$cryptcompat	CRUDDY_CRYPT_COMPAT /* Cruddy and new */
#$kerberos	KERBEROS	/* Use Kerberos */

#$mail	MAIL		/* Include code for mail */
#$news	NEWS		/* Include code for news */
#$pp	PP		/* Include code for pp */
#$mailownsmail	MAILOWNSMAIL	/* mail spool dir isn't free access*/
#$rmtdbm	RMTDBM		/* Allow remote access to NRS dbm */
#define	DEF_PKTSIZE	$def_pktsize	/* Default packet size (or 0) */
#define	DEF_WNDSIZE	$def_wndsize	/* Default window size (or 0) */
#$debug	DEBUG		/* Include debugging cose */
#$softmail	ALL_MAIL_SOFT	/* All Q end mail failures are soft */
#$softpp	ALL_PP_SOFT	/* All Q end pp failures are soft */
#$softnews	ALL_NEWS_SOFT	/* All Q end news failures are soft */
#$freediskdef	FREEDISK	$freedisk	/* Ensure some free disk */
#$ucl	UCL		/* Include UCL specials */
#$ucl	UCL_V77		/* Include UCL specials */
#$ucl	UCL_STATS	/* Include UCL specials */
#$ucl	UCL_ISID	/* Include UCL specials */
#$ucl	EXTERNUSER	$externuser/* UCL special */
#$ukc	UKC		/* Include UKC specials */
#$ukc	KENT_MAILER	/* Include UKC specials */
#$macros	MACROS		/* macros for netgetc() & netputc(c)*/
#$maxfile	MAXFILESIZE	$maxfilesize/* call ulimit(2, n/512) */
#define	MAILER	"$mailer"	/* Which mailer to use */
#$catchall	CATCHALL	/* catch all errors in mail (??) */
#$mixedlines	MIXED_LINES	/* allow PSS or JANET on either line */
#$noleadingzeros	NOLEADINGZEROS	/* -- JUNK THIS -- */
#$camtechack	CAMTEC_HACK	/* Insert missing YBTS separator */
#$standalone	STANDALONE	/* ubc specific (??) */
#$testprotid	TEST_PID	/* ubc - Test Protocol ID */
#$small_proc	SMALL_PROC	/* keep the size down */

/*
These procedures are in the lib/gen library: 
	rename, lockfile, bzero, getwd, fullpath

These #defines should be REPLACED by the more specifec #define
_42	bin{cpf,dbencode,fpc,ftpspool,ftq,qft}
	rename/fullpath, rename, sys/file.h/flock RESTARTSYS SETLINEBUF
	rename/getwdir sys/file.h/TRUNCATE NGROUPS
STDV7	bin/{dbget,dbextract,get}	-- omit as void handled --
vax	bin{dbencode,dbpatch,dbget,get,setup}  gen/dbenter
					-- omit as void and bzero handled --
pdp11	bin/{dbencode,ftp} lib/gen/tailor -- omit as void handled --
					  -- use SIGIOT --
					  -- setbuf bug in PDP's -- ?? --
*/
/* HACK HACK HACK */
/* HACK HACK HACK */
/* until everything is switched to use the proper #defines .. */
#$hack42	_42	/* HACK */
/* HACK HACK HACK */
/* HACK HACK HACK */


#include <sys/types.h>
#ifndef	UCHAR
typedef	unsigned char u_char;
#endif	UCHAR
$ndircomm#include "$ndirh"			/* need ndir.h */
#include "log.h"			/* how to write logging info */
!GROK!THIS!
then if cmp $outputfile $outputfile.$$ > /dev/null 2>& 1
	then
		rm -f $outputfile.$$
		$echoq "	" $outputfile not affected
	elif mv $outputfile.$$ $outputfile
	then
		echo "	" $outputfile updated
	else
		echo "+++	" $outputfile left in $outputfile.$$
	fi
else
	echo "	" Failed to create $outputfile.$$
fi
