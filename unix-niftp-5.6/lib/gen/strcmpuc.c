/* unix-niftp lib/gen/strcmp.c $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/gen/strcmpuc.c,v 5.5 90/08/01 13:35:01 pb Exp $ */
/*
 * $Log:	strcmpuc.c,v $
 * Revision 5.5  90/08/01  13:35:01  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:45:49  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * Case insensitive comparisons.
 */

#include "opts.h"

char    lc_table[256] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
    'p','q','r','s','t','u','v','w','x','y','z',91, 92, 93, 94, 95,
    96, 97, 98, 99, 100,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
    176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
    208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

#ifndef	ASCII
int     table_made = 0;
void
check_table () {
    if (!table_made) {
	int     i;
	char   *p = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char   *q = "abcdefghijklmnopqrstuvwxyz";
	for (i = 0; i < sizeof lc_table; i++)
	    lc_table[i] = i;
	while (*p)
	    lc_table[*p++] = *q++;
	table_made++;
    }
}
#endif	ASCII

strncmpuc (s1, s2, n)
register char  *s1,
               *s2;
register    n;
{
#ifndef	ASCII
    if (!table_made) check_table ();
#endif	ASCII
    while (--n >= 0) {
	register char   c1 = lc_table[*s1++];
	register char   c2 = lc_table[*s2++];

	if (c1 != c2)
	    return (c1 - c2);
	if (c1 == '\0')
	    return (0);
    }
    return 0;
}

strcmpuc (s1, s2)
register char  *s1,
               *s2;
{
#ifndef	ASCII
    if (!table_made)	check_table ();
#endif	ASCII
    do
	if (lc_table[*s1] != lc_table[*s2++])
	    return 1;
    while (*s1++);
    return 0;
}
