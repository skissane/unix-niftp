/* unix-niftp lib/pqproc/perms.c $Revision: 5.6.1.2 $ $Date: 1993/01/10 07:00:46 $ */
/*
 * check for password if one is needed - version for ukc password system.
 * With hacks in for other systems.
 * $Log: perms.c,v $
 * Revision 5.6.1.2  1993/01/10  07:00:46  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:01:19  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:03  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:50:47  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:23:51  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:33:12  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:08:10  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:00:51  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:49:27  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "ftp.h"
#include "nrs.h"
#include "infusr.h"
#include "files.h"
#include <ctype.h>

char    *crypt();       /* to stop lint complaining */
char    *malloc();
char	*index();
extern  char    *regimodes;
int    restricted;
extern  char    unknown_host;
extern  char    *SECUREDIRS;
extern	char	lallow_tji;

#ifdef	INFOSTRUCT
#define	I_READ	1
#define	I_WRITE	2
struct	info {
	char	*i_name;
	char	*i_path;
	int	i_mode;
	char	*i_users;
	char	*i_hosts;
	char	*i_addresses;
}

init_info(infop)
struct info *infop;
{
	bzero(infop, sizeof (struct info));
	infop->i_mode = I_READ;
}
#endif	INFOSTRUCT

#define	REST_EXTERNAL		0x01
#define	REST_GUEST		0x02
#define	REST_UNKNOWN_HOST	0x04
#define	REST_ALTPWFILE		0x08
#define	REST_ALLUSERS		0x40
#define	REST_ALL		(0xff & ~REST_ALLUSERS)

#ifndef	R_NOPW		/* If this restiction & no pw in pw field, then reject */
#define	R_NOPW		(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE | REST_ALLUSERS)
#endif	/* R_NOPW */

#ifndef	R_CK_FILES	/* If this restiction, then we need to call check_secure */
#define	R_CK_FILES	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_CK_FILES */

#ifndef	R_CK_DOTDOT	/* in check_secure, disallow /../ */
#define	R_CK_DOTDOT	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_CK_DOTDOT */

#ifndef	R_CK_MODE	/* in check_secure, check the mode, e.g. no TJI */
#define	R_CK_MODE	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_CK_MODE */

#ifndef	R_CK_RELATIVE	/* in check_secure, disallow starting with / or ~user */
#define	R_CK_RELATIVE	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_CK_RELATIVE */

#ifndef	R_CK_SYMB_L	/* in check_secure, disallow symbolic links */
#define	R_CK_SYMB_L	(REST_EXTERNAL | REST_ALTPWFILE)
#endif	/* R_CK_SYMB_L */

#ifndef	R_NO_TWIDDLE	/* in check_secure, don't allow ~user  (e.g. ~root) */
#define	R_NO_TWIDDLE	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_NO_TWIDDLE */

#ifndef	R_CK_G_MODE	/* in check_secure, check mode for guess access */
#define	R_CK_G_MODE	(REST_EXTERNAL | REST_GUEST | REST_UNKNOWN_HOST | REST_ALTPWFILE)
#endif	/* R_CK_G_MODE */

extern struct  passwd  *getpwnam(), *getpwuid(), *find_user();

chkpasswd()
{
	register char    *pt;
	register int    i;
	register struct passwd *pw;
	char    tbuff[256];
	int	using_altpwfile = 0;
	int	is_guest = 0;
	int	guest_ok = 0;

	if(!username || !filename){
		L_WARN_0(L_MAJOR_COM, 0, 
			"Didn't get both username and filename....\n");
		return(0);
	}
	/* horrible hack for euclid system - Yeuch !! */
	setpwent();
	pw = getpwnam(username);
	if (!pw && altpwfile && (pw = find_user(username, altpwfile)))
	{	L_LOG_2(L_GENERAL, 0, "Found %s in %s\n", username, altpwfile);
		using_altpwfile++;
	}
	if (!pw)
	{	int lower = 0;
		char *ptr = username;

L_LOG_2(L_GENERAL, 0, "Failed on '%s' by '%s'\n", filename, username);
		while(!lower && *ptr) if (islower(*ptr++)) lower++;
		if (!lower)	lowerfy(username);
		ptr = filename;
		while(!lower && *ptr) if (islower(*ptr++)) lower++;
		if (!lower)
		{	char *start = (*filename == '<') ?
				index(filename, '>') : (char *) 0;
			if (!start) start = filename;
			lowerfy(start);
		}
		if(lower || !(pw = getpwnam(username)))
		{	/* can't find name in password file */
			ecode = ER_BAD_PW;
			return(-1);     /* unknown user name */
		}
	}
	endpwent();
	uid = pw->pw_uid;
	gid = pw->pw_gid;
#ifdef  EXTERNUSER
	if(uid >= EXTERNUSER)
	{
		L_LOG_0(L_GENERAL, 0,"External user, so restricted access\n");
		if (*reason) (void) strcat(reason, MSGSEPSTR);
		(void) strcat(reason, "Guest restrictions apply");
		restricted |= REST_EXTERNAL;
	}
#endif
#ifdef  GUEST_ACCOUNTS
	/* "guest" name is now of the form
	 *	username[/prefix][=mode][,username[/prefix][=mode]]*
	 */
	{	char data[1024];
		char *this;
		char *next;

		strncpy(data, guest_name, sizeof data);
		data[sizeof data -1] = '\0';

		for (this=data; this; this=next) /* for each guest */
		{	char *prefix;
			char *mode;

			next = index(this, ',');
			if (next) *next++ = '\0';

			mode = index(this, '=');
			if (mode) *mode++ = '\0';

			prefix = index(this, '/');
			if (prefix) *prefix++ = '\0';

			if(strcmp(this, username)==0)
			{	
				char buff[1024];
				int   non_text = 0;
				char *ptr;
				char *at;
				is_guest++;
				if (!usrpaswd) 
				{
					L_ACCNT_1(L_ALWAYS, 0, 
					"No passwd for guest access (%s)\n",
						this);
					ecode = ER_NO_PASSWD;
					return -1;
				}
				for (ptr=usrpaswd; *ptr; ptr++)
				    if (iscntrl(*ptr) || !isgraph(*ptr))
				    {	non_text++;
					break;
				    }
				ptr = usrpaswd;
				if (!(at = index(ptr, '@')) || at == ptr ||
					!at[1] || non_text)
				{	priv_encode(buff, ptr, 1);
					if (index(buff, '@'))
					{	ptr = buff;
						L_LOG_0(L_GENERAL, 0,
					"[Password unix-niftp encoded (1)]\n");
					}
				}
				if (prefix)
				{	if (strncmp(ptr, prefix, strlen(prefix)))
					{	L_ACCNT_2(L_ALWAYS, 0,
						"Prefix failure on %s (%s)\n",
							this,
							(mode) ? mode : "");
						continue;
					}
					if (!mode || ((*mode) != '-'))
						ptr = ptr+strlen(prefix);
					else	mode++;
					
					restricted |= REST_GUEST;
				}
				else	restricted |= REST_GUEST;

				if (!(at =index(ptr, '@')) || at == ptr || ! at[1])
				{	ecode = ER_NO_AT_IN_PW;
					L_ACCNT_3(L_ALWAYS, 0, 
					"failed guest access by:- %s%s %s mode\n",
					ptr,
					(prefix) ? " with prefix," : "",
					(mode) ? mode : "no");
					return -1;
				}
				L_ACCNT_3(L_ALWAYS, 0, 
					"special guest access by:- %s%s %s mode\n",
					ptr,
					(prefix) ? " with prefix" : "",
					(mode) ? mode : "no");
				if (ptr != usrpaswd) strcpy(usrpaswd, ptr);
				guest_ok++;
				break;
			}
		}
	}
	if (is_guest)
	{	if (!guest_ok)
		{	L_ACCNT_1(L_ALWAYS, 0, "prefix not found for %s\n",
						username);
			ecode =(usrpaswd) ? ER_BAD_PW : ER_NO_PASSWD;
			return -1;
		}
	}
	else
#endif  GUEST_ACCOUNTS
	if(*pw->pw_passwd == 0 ){
		L_LOG_0(L_10, 0, "No password in /etc/passwd !!\n");
		if(restricted & R_NOPW || f_access != ACC_RDO){
			ecode = ER_NEED_PW;
			return(-2);
		}
	} else if(!usrpaswd){          /* no password given */
		ecode = ER_NEED_PW;    /* should have one if not UCL system */
		return(-2);
	}
#ifdef KERBEROS
	else if( !krb_pwcheck(username,pw->pw_passwd, usrpaswd))
#else
	else if(strcmp(pw->pw_passwd, crypt(usrpaswd, pw->pw_passwd)) != 0)
#endif
	{	char buffer[1024];
		priv_encode(buffer, usrpaswd, 1);
		if(strcmp(pw->pw_passwd, crypt(buffer, pw->pw_passwd)) != 0)
		{	ecode = ER_BAD_PW;
			return(-3);	/* passwords don't match */
		}
		L_LOG_0(L_GENERAL, 0, "[Password unix-niftp encoded (1)]\n");
	}
	/* must have a password and explicit allowance for tji !! */
	if(f_access == ACC_TJI && (!*pw->pw_passwd || !lallow_tji)) {
		if (!lallow_tji)
			L_WARN_0(L_GENERAL, 0, "Take Job Input not allowed\n");
		ecode = ER_BAD_PW;
		return(-3);
	}
#ifdef  PRINT_USER
	if(strcmp(username, PRINT_USER) == 0){
		if(f_access != ACC_TJO){ /* not tjo - reject */
			ecode = ER_FILE_ACCESS;
			return(-3);
		}
	}
#endif
	if (banned_file && *banned_file)
	{	FILE *banf = fopen(banned_file, "r");
		if (!banf)
		{	L_LOG_2(L_ALWAYS, 0,
				"Failed to open banned file '%s' %d\n",
				banned_file, errno);
			ecode = ER_NO_BANNEDFILE;
			return(-2);
		}
		else
		{	char buff[1024];
			while (fgets(buff, sizeof buff, banf) != NULL)
			{	int len = strlen(buff);
				if(buff[len-1] == '\n') buff[len-1] = '\0';
				if (!strcmp(buff, username))
				{	L_LOG_1(L_ALWAYS, 0,
						"User %s banned\n", username);
					ecode = ER_BANNED_USER;
					fclose(banf);
					return(-2);
				}
				L_LOG_2(L_10, 0,
					"User %s - %s - OK\n", buff, username);
			}
			fclose(banf);
		}
	}

	cur_user = pw;

	/*
	 * now do some security checks.
	 */
	if(unknown_host)
	{	if (*reason) (void) strcat(reason, MSGSEPSTR);
		(void) strcat(reason,
			"Unknown host, so guest restrictions apply");
		L_LOG_0(L_GENERAL, 0, "Unknown host, so restricted access\n");
		restricted |= REST_UNKNOWN_HOST;
	}

	if(restricted & R_CK_FILES || *filename == '<'){ /* do expansions + other things */
		switch(check_secure()){
		case 0: return(1);	/* have expanaded a <> */
		case 1: break;		/* OK so far ... */
		default: return(-1);	/* Failed */
		}
	}

/*
 * if file name does not start from root then it starts from the login
 * directory of the user. Get from rest of password file.
 */
	if(*filename != '/'){           /* full pathname already */
					/* now build the full path */
		struct	passwd *home = cur_user;
		char	xfer_name[20];
		char	*name = filename;
		int	restore	= 0;

		/* ~/<rest>	skip the ~/
		 * ~<user>/rest	look up user's home dir
		 *
		 * This is all a pain because getpwnam uses a static
		 * structure, so it has to be restored afterwards ....
		 */
		if (*filename == '~')	/* do the expansion .. */
		{	if (name = index(filename, '/'))
			{	*name++ = '\0';
				if (filename[1] &&
					strcmp(cur_user->pw_name, filename+1))
				{	if (restricted & R_NO_TWIDDLE)
					{
						return -1;
					}
					strcpy(xfer_name, cur_user->pw_name);
					home = getpwnam(filename+1);
					if (!home && altpwfile) home =
					    find_user(filename+1, altpwfile);
					if (home) restore++;
					else	  home=cur_user;
					
				}
			}
			else	name = filename; /* ~pb is a directory ? */
		}
		sprintf(tbuff, "%s/%s", home->pw_dir, name);
		free(filename);
		pt = malloc(strlen(tbuff)+1);     /* make space */
		filename = pt;
		strcpy(pt, tbuff);
		if (restore)
		{	cur_user = getpwnam(xfer_name);
			if (!cur_user && altpwfile)
				cur_user = find_user(xfer_name, altpwfile);
		}
	}
	return(1);
}

/*
 * extra routine to check for security.
 * Only allow files below users directory or with the specified prefix
 * E.g. Implements <DOCS>/filename
 * -1 -> failed, 0 -> <>, 1 -> relative path name
 */
check_secure()
{
	register char    *p;
	char    *index();

	/* If it is a directory system managed by sys admin,
	 * it is sufficient to check for ".." as the sys admin may want
	 * to put in symbolic links.
	 */
	if(restricted & R_CK_DOTDOT)         /* Don't allow ".." e.g. <X> */ 
	{	int failed = 0;
		
		if(*filename == '.' && *(filename+1) == '.')
			failed++;
		else for(p = filename; p = index(p, '/') ;p++){
			while(*(p+1) == '/') p++;
			if(*(p+1) != '.') continue;
			if(strncmp(p, "/..", 3) == 0 ){ failed++; break; }
		    }
		if (failed)
		{	L_WARN_1(L_GENERAL, 0, "found .. in pathname %s\n",
				filename);
			if (*reason) (void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, "badly formed pathname");
			return(-1);
		}
	}

	if(restricted & R_CK_MODE) switch(f_access){
	case ACC_TJO:
		L_WARN_1(L_GENERAL, 0, "Allowing  TJO%04x\n",f_access&0xFFFF);
		break;
	case ACC_TJI:
	case ACC_GJI:
	case ACC_GJO:
		L_WARN_1(L_GENERAL, 0, 
			"Cant perform %04x\n",f_access&0xFFFF);
		if (*reason) (void) strcat(reason, MSGSEPSTR);
		(void) sprintf(reason + strlen(reason),
		"%sRestricted account -- access mode %04x not allowed",
			(*reason) ? MSGSEPSTR : "", f_access);
		return(-1);
	}
	/* if we are useing a full pathname or a macro then
	 * the user can only read it.
	 */
	if(*filename == '/' || *filename == '<') return check_securedirs();

	/* Can't think of any other reason to reject it -- guess it is OK */
	return(1);
}

/* Consult securedirs to ensure that the prefix is valid.
 * Side effect is that <foo> is expanded.
 * 0 -> OK, -1 -> not found.
 */
check_securedirs()
{
	register FILE    *Fp;
	char    tbuf[256];
	register c;
	int     i;
	register char    *p;
	char    *index();
	/*
	 * now check to see if the filename is accessable
	 */
	if((Fp = fopen(SECUREDIRS, "r")) == NULL){
		L_WARN_1(L_GENERAL, 0, 
			"Cannot open secure directory file %s\n", SECUREDIRS);
		if (*reason) (void) strcat(reason, MSGSEPSTR);
		(void) strcat(reason, "Internal error - security file failure");
		return(-1);
	}
	for(;;){
		for(p = tbuf ; (c = getc(Fp)) != EOF && c != '\n';*p++ = c);
		if(c == EOF) break;
		*p = 0;
		i = p - tbuf;

		/* Check that there are at least two : separated fields */
		if( (p = index(tbuf, ':')) != NULL) { i = p - tbuf; *p++ = 0; }

		/* first see if we have a filename expansion E.g. <RFC> */
		if(*filename == '<' && p)
		{	char *keys = index(p, ':');
			if (keys) *keys++ = 0;
			i = p - tbuf - 1 ;
			if(strncmp(filename, tbuf, i) || filename[i-1] != '>')
								continue;
			if (!check_guest_access(keys))		continue;
			/* Looking good .... */
			(void) strcpy(tbuf, p); /* move it down */
			(void) strcat(tbuf, "/");
			(void) strcat(tbuf, filename+i);
			free(filename);
			filename = malloc(strlen(tbuf) +1);
			(void) strcpy(filename, tbuf);
			fclose(Fp);
			return(0);
		}
		/* Otherwise it is a full pathname */
		if (!check_guest_access(p))	continue;
		if(strncmp(tbuf, filename, i))	continue;
		if(filename[i] != '/' && !index(p, filename[i])) continue;
		fclose(Fp);
		return(0);
	}
	if (*reason) (void) strcat(reason, MSGSEPSTR);
	(void) strcat(reason, "no permissions to access file");
	fclose(Fp);
	return(-1);
}

check_guest_access(keys)
char *keys;
{
	/* THIS IS JUST A HACK TO GET THE BALL ROLLING !!! */
	if (restricted & R_CK_G_MODE) switch (f_access)
	{
	default:	return 0;
	case ACC_RDO:	return 1;
	case ACC_TJO:	/* Well ?? */
	case ACC_MO:
	case ACC_ROM:
	case ACC_AO:
	case ACC_AOM:	return !!keys;
	}
	return 1;
}


/* It the Username is in upper case then convert all strings to lower
 * case since the user must be on an upper case only terminal.
 */

casechange()
{	register char   *p;
	register i;

	for(i = 0 ; i < STRINGCOUNT ; i++)
		if(p = sftstrings[i])
			for( ;*p ; p++)
				if(*p >= 'A' && *p <= 'Z')
					*p += 'a' - 'A';
}

lowerfy(string)
char *string;
{	for(; *string; string++) if (isupper(*string)) *string=tolower(*string);
}
