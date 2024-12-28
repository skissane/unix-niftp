#include "opts.h"
#include "db.h"
#include "nrs.h"

extern char *strcpy (), *strncpy (), *strcat ();

/* Reverse a somain name, uk <-> us order */
void reverse_name(to, from)
char *to;
char *from;
{
	char *rindex();
	char temp[1024];
	char *p;

	(void) strcpy(temp, from);
	*to = '\0';

	while ((p = rindex(temp, '.')) != NULL)
	{	*p++ = '\0';
		(void) strcat(to, p);
		(void) strcat(to, ".");
	}
	(void) strcat(to, temp);
}

/* Look up a host name, trying the domain names given in niftptailor */
/* If a buffer is supplied, pass back the actual result */
struct host_entry* dbase_find(host, buff, len)
char *host;
char *buff;
{	struct  host_entry      *hp;
	register char   **xp;
	char reversed[1024];
	char buff_[1024];

	*reversed = '\0';

	/* Try it ASIS first of all */
	if((hp = dbase_get(host)) != NULL)
	{	if (buff && len > 0)
		{	strncpy(buff, host, len);
			buff[len-1] = '\0';
		}
		return hp;
	}
	/* DNS convention is that trailing . means complete name */
	if(host[strlen(host)-1] == '.')
	{	reverse_name(buff_, host);
		hp = dbase_get(buff_+1);
		if (buff && len > 0)
		{	strncpy(buff, buff_+1, len);
			buff[len-1] = '\0';
		}
		return hp;
	}
	/* finally check possible domainless references */
	for(xp = NRSdomains; *xp ; xp++)
	{	if (**xp == '.')
		{	(void) strcpy(reversed, host);
			if (strcmp(*xp, ".")) (void) strcat(reversed, *xp);
			reverse_name(buff_, reversed);
		}
		else
		{	(void) strcpy(buff_, *xp);
			(void) strcat(buff_, host);
		}
		if((hp = dbase_get(buff_)) != NULL)
		{	if (buff && len > 0)
			{	strncpy(buff, buff_, len);
				buff[len-1] = '\0';
			}
			return hp;
		}
	}
	return (struct  host_entry*) 0;
}
