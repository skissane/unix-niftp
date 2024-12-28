#include <stdio.h>
#include <pwd.h>
#define	PWSKIP(s)	while(*(s) && *(s) != ':' && *(s) != '\n') ++(s); \
			if (*(s)) *(s)++ = 0; else continue;

struct passwd *
find_user_id(user, uid, file)
	char *user;
	char *file;
{
	FILE *pwf;
	register char *p;
	static struct passwd pw;
	static char line[BUFSIZ+1];
	int is_name = (user != (char *) 0);

	if (!(pwf = fopen(file, "r"))) return NULL;
	while (fgets(p=line, BUFSIZ, pwf) != NULL) {
		pw.pw_name = p;	PWSKIP(p);
		if (is_name && strcmp(user, pw.pw_name)) continue;
		pw.pw_passwd = p;	PWSKIP(p);
		pw.pw_uid = atoi(p);	PWSKIP(p);
		if (!is_name && pw.pw_uid != uid) continue;
		pw.pw_gid = atoi(p);	PWSKIP(p);
		pw.pw_gecos = p;	PWSKIP(p);
		pw.pw_dir = p;		PWSKIP(p);
		pw.pw_shell = p;	while(*p && *p != '\n') p++;
		*p = '\0';
		(void) fclose(pwf);
		return (&pw);
	}
	(void) fclose(pwf);
	return NULL;
}

struct passwd *
find_user(user, file)
	char *user;
	char *file;
{	return find_user_id(user, -1, file);
}
