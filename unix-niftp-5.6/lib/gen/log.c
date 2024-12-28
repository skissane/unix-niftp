#include "opts.h"
#include "log.h"
#include <stdio.h>

/*VARARGS4*/
logit(mask, flags, prefix, format, a1,a2,a3,a4)
char *format, *prefix;
{	static	long last_time = 0;
	long this_time = time(0);
	extern  char    *LOGDIR;        /* default directory for log files */
	FILE *out = stdout;

	/* We may not yet have a suitable log file, so open one ... */
	if ((flags & L_FILE) && LOGDIR && *LOGDIR)
	{	char fullname[1024];
		sprintf(fullname, "%s/log.ERROR", LOGDIR);
		out = fopen(fullname, "a+");
		if (!out) out = stdout;
	}

	if (!last_time) last_time = this_time;

	if (prefix && *prefix && !(flags & L_CONTINUE))
		fprintf(out, "%.3s.%05x.%04x.%03x: ",
			prefix, mask, getpid(), this_time - last_time);
	if (flags & L_TIME || flags & L_DATE)
	{	char *ctime(), *date;

		date = ctime( (int*)&this_time );
		date[3] = date[7] = date[10] = date[19] = date[24] = '\0';
		if (flags & L_TIME) fprintf(out, "%s: ", &(date[11]));
		if (flags & L_DATE) fprintf(out, "%s %s: ", &(date[4]), &(date[8]));
	}

	fprintf(out, format, a1, a2, a3, a4);
	fflush(out);
	if (out == stdout) last_time = this_time;
	else fclose(out);
}
