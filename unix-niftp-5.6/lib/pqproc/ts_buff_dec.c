#ifndef	lint			/* unix-niftp lib/x25b/ts_buff_decode.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/pqproc/RCS/ts_buff_dec.c,v 5.6 1991/06/07 17:02:07 pb Exp $";
#endif	lint

/*
 * file:	ts_buff_fecode.c
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: ts_buff_dec.c,v $
 * Revision 5.6  1991/06/07  17:02:07  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:43  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:53:48  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 *
 * This module decodes a TS arg string into (up to) 4 pointers.
 * It can use the input buffer for output, or can copy.
 */
ts_buff_decode(raw, buff, len, argc, a0, a1, a2, a3)
register unsigned char *raw, *buff;
char **a0, **a1, **a2, **a3;
register int len;
{   char **argv[4];
    int param;

    if (!buff) buff=raw;
    argv[0] = a0;
    argv[1] = a1;
    argv[2] = a2;
    argv[3] = a3;
    if (argc > sizeof argv / sizeof argv[0])
	argc = sizeof argv / sizeof argv[0];

    for (param=0; param < argc; param++)
    {	register int i;

	if (len-- ==0)		return param;
	if (!(*raw & 0x80))	return -(param+1);
	if ((i = (*raw++ & 0x3f)) > len) return (param+1) * -11;
	*argv[param] = (char *)buff;
	for (;i>0; len--, i--) *buff++ = (*raw++) & 0xff;
	*buff++ = '\0';
    }

    return param;
}
