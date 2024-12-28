/* krbpw.c - check the password */
/* Used by various programs, for each use some program specific #define
 * and add an #ifdef block to define these macros.  If you are lucky you
 * can just -DKRB_PASSWD, to get a Kerberos password checker and,
 * and -DKRB_STD_INC to use the standard set of include files.
 *
 * These are they macros you need to think about:
 * KRB_PASSWD use Kerberos password checking, otherwise just uses crypt()
 * KRB_REPORT_TRACE(str) define only if you want detailed tracing
 * KRB_REPORT_SERIOUS(str) a serious error or a spoofing attempt found
 *
 *
 * To add this into your program look for the places where crypt() is
 * used to check the passwd.  Replace a test like:
 *
    if (strcmp (crypt (passwd_read_in, pw->pw_passwd), pw->pw_passwd) != 0)
        return FAILED;
 *
 * by
 *
#ifdef KRB_PASSWD
    if (!krb_pwcheck (pw->pw_name, pw->pw_passwd, passwd_read_in))
#else
    if (strcmp (crypt (passwd_read_in, pw->pw_passwd), pw->pw_passwd) != 0)
#endif
        return FAILED;
 *
 *
 * To compile this you will probably need to add -I/usr/local/include/kerberos
 * or something similar (depending where you're kerberos include files are).
 * You will also need to link with libkrb.a and probably libdes.a as well.
 */

/* LINTLIBRARY */

#ifdef TEST
 /* A test mode - generate a standalone program to try out the various
  * routines */

/* Use the standard include files */
#define KRB_STD_INC
#define KRB_PASSWD

#define KRB_REPORT_TRACE(str)	fprintf( stderr, "Watch-- %s\n", str );
#define KRB_REPORT_SERIOUS(str)	fprintf( stderr, "FATAL-- %s\n", str );
#endif /* end of TEST */


#ifdef KRB_STD_INC
 /* A catchall - no tracing and use syslog to report serious errors */
#include <stdio.h>
#include <krb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <syslog.h>
#include <netdb.h>

#endif /* end of KRB_STD_INC */


/* ------------ For use in the ISO Developement Environment ------------ */
#ifdef ISODE
#include <stdio.h>
#include "general.h"
#include "manifest.h"
#ifdef	KRB_PASSWD
#include <krb.h>
#include "internet.h"
#include "tailor.h"
#endif

#define KRB_REPORT_SERIOUS(str)	ll_log(compat_log, LLOG_EXCEPTIONS, NULLCP, str)
#endif /* end of ISODE */


/* ------------ For use in the Unix Niftp package ------------ */
#ifdef UNIXNIFTP
/* Need to compile with -DUNIXNIFTP -DKRB_PASSWD -DKRB_STD_INC */

#include "ftp.h"

#define KRB_REPORT_SERIOUS(str)	L_LOG_1(L_MAJOR_COM, 0, "%s\n", (str))
#endif /* end of UNIXNIFTP */



#ifndef KRB_REPORT_SERIOUS
#define KRB_REPORT_SERIOUS(str) syslog( LOG_NOTICE, (str))
#endif

#ifdef KERBEROS
# ifndef KRB_PASSWD
# define KRB_PASSWD
# endif
#endif

#ifdef KRB_PASSWD
/* Error code to string */
#define KRB_ERR_STR(e) ((e) >= 0 ? krb_err_txt[(e)] :"INVALID ERROR RETURN")
#endif /* End of UNIXNIFTP */



char   *crypt ();

/* S. Lacey. and L.McLoughlin Dept. of Computing,  Imperial College.
 * With substantial modifications by John T Kohl to avoid spoofing.
 *
 * Takes the username, the password from the password file, and the passwd
 * the user is trying to use.
 * Returns 1 if the passwd matches otherwise 0.
 */

#ifndef	KRB_PASSWD
/* ARGSUSED */
#endif
int krb_pwcheck ( usrname, pwpass, usrpass )
char   *usrname;
char   *pwpass;
char   *usrpass;
{
#ifdef	KRB_PASSWD
	char realm[REALM_SZ];
	int krbval;
	unsigned old_alarm;
	char obuf[128];

	/* 
	 * check to see if the passwd is `*krb*'
	 * if it is, use kerberos.  If not then call up crypt().
	 */

#ifdef KRB_REPORT_TRACE
	sprintf (obuf, "krb_pwcheck(%s,%s,SOMESTRING)", usrname, pwpass, usrpass);
	KRB_REPORT_TRACE (obuf);
#endif
	if (strcmp(pwpass, "*krb*") == 0) {
		/*
		 * use kerberos, first of all find the realm
		 */
		if (krb_get_lrealm(realm, 1) != KSUCCESS) {
			(void) strncpy(realm, KRB_REALM, sizeof(realm));
		}
#ifdef KRB_REPORT_TRACE
		sprintf (obuf, "Realm is %s", realm);
		KRB_REPORT_TRACE (obuf);
#endif

		/*
		 * now check the passwd
		 */
		krbval = krb_get_pw_in_tkt(usrname, "",
					   realm, "krbtgt",
					   realm,
					   DEFAULT_TKT_LIFE, usrpass);

		if( krbval != INTK_OK ){
			sprintf (obuf, "Initial kerberos check for %s failed with %d", usrname, krbval);
			KRB_REPORT_SERIOUS( obuf );

			/* it has failed to match */
			return 0;
		}

#ifdef KRB_REPORT_TRACE
		sprintf (obuf, "Initial kerberos check for %s succeeded", usrname);
		KRB_REPORT_TRACE (obuf);
#endif
		old_alarm = alarm(0);	/* Authentic, so don't time out. */
		if (verify_krb_tgt(realm) > 0){
			alarm (old_alarm);
#ifdef KRB_REPORT_TRACE
			sprintf (obuf, "Verify succeeded for %s", usrname);
			KRB_REPORT_TRACE ( obuf );
#endif
			return 1; /* Success!! */
		}
		
		/* Oops.  He tried to fool us.  Tsk, tsk. */
		alarm (old_alarm);
		sprintf (obuf, "Attempted hack on kerberos with username %s", usrname);
		KRB_REPORT_SERIOUS (obuf);
		return 0;
	}
#endif

	/*
	 * use passwd file password
	 */
	return (strcmp(crypt(usrpass, pwpass), pwpass) == 0);
}

#ifdef	KRB_PASSWD
/*
 * Verify the Kerberos ticket-granting ticket just retrieved for the
 * user.  If the Kerberos server doesn't respond, assume the user is
 * trying to fake us out (since we DID just get a TGT from what is
 * supposedly our KDC).  If the rcmd.<host> service is unknown (i.e.,
 * the local /etc/srvtab doesn't have it), let her in.
 *
 * Returns 1 for confirmation, -1 for failure, 0 for uncertainty.
 */
int verify_krb_tgt (realm)
    char *realm;
{
    char hostname[MAXHOSTNAMELEN], phost[BUFSIZ];
    struct hostent *hp;
    KTEXT_ST ticket;
    AUTH_DAT authdata;
    unsigned long addr;
    static /*const*/ char rcmd[] = "rcmd";
    char key[8];
    int krbval, retval, have_keys;
    char obuf[128];

#ifdef ISODE
    (void) strcpy (hostname, getlocalhost ());
#else
    if (gethostname (hostname, sizeof(hostname)) == -1) {
        KRB_REPORT_SERIOUS( "cannot retrieve local hostname" );
	return -1;
    }
#endif
    strncpy (phost, krb_get_phost (hostname), sizeof (phost));
    phost[sizeof(phost)-1] = 0;
    hp = gethostbyname (hostname);
    if (!hp) {
        sprintf (obuf, "cannot retrieve local host address: %s", hostname);
        KRB_REPORT_SERIOUS (obuf);
	return -1;
    }
    bcopy ((char *)hp->h_addr, (char *) &addr, sizeof (addr));
    /* Do we have rcmd.<host> keys? */
    have_keys = read_service_key (rcmd, phost, realm, 0, "/etc/srvtab", key)
	? 0 : 1;
    krbval = krb_mk_req (&ticket, rcmd, phost, realm, 0);
    if (krbval == KDC_PR_UNKNOWN) {
	/*
	 * Our rcmd.<host> principal isn't known -- just assume valid
	 * for now?  This is one case that the user _could_ fake out.
	 */
	if (have_keys)
	    return -1;
	else
	    return 0;
    }
    else if (krbval != KSUCCESS) {
	sprintf (obuf, "Unable to verify Kerberos TGT: %s", KRB_ERR_STR(krbval));
        KRB_REPORT_SERIOUS (obuf);
	return -1;
    }
    /* got ticket, try to use it */
    krbval = krb_rd_req (&ticket, rcmd, phost, addr, &authdata, "");
#ifdef KRB_REPORT_TRACE
    sprintf (obuf, "got result from krb_rd_req of %d", krbval);
    KRB_REPORT_TRACE ( obuf );
#endif
    if (krbval != KSUCCESS) {
	char *err;

	if (krbval == RD_AP_UNDEC && !have_keys)
	    retval = 0;
	else {
	    retval = -1;
	}

	sprintf (obuf, "can't verify rcmd ticket: %s;%s",
		 KRB_ERR_STR(krbval),
		 retval
		 ? "srvtab found, assuming failure"
		 : "no srvtab found, assuming success");
        KRB_REPORT_SERIOUS (obuf);
    }
    else
	    /*
	     * The rcmd.<host> ticket has been received _and_ verified.
	     */
	    retval = 1;

    return retval;
}
#endif


#ifdef TEST
extern char *getpass();

main()
{
	char user[ 100 ];
	char passwd[ 100 ];
	
	printf( "Username? " ); fflush( stdout );
	gets( user );
	printf( "Password? " ); fflush( stdout );
	/* OK - so I should do ioctls.... this is quicker to type!
	 * Can't use getpass() as it only reads 8 chars and kerberos
	 * passwords can be longer */
	system( "stty -echo" );
	gets( passwd );
	system( "stty echo" );
	printf( "\n" );

	/* Force a kerberos check of the password */
	if( krb_pwcheck( user, "*krb*", passwd ) )
		printf( "OK\n" );
	else
		printf( "FAILED\n" );
}
#endif
