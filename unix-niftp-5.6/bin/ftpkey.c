/* unix-niftp bin/ftpkey.c $Revision: 5.5 $ $Date: 90/08/01 13:30:23 $ */
/*
 *      ftpkey: Program to generate key for the ftp secure password system
 *
 * $Log:	ftpkey.c,v $
 * Revision 5.5  90/08/01  13:30:23  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:03:10  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:04:28  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:57:37  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:18:31  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include <stdio.h>

char	*crypt();
char kbuf[13+1];
extern  char    *KEYFILE;

main()
{
	char *getpass();
	char *pass;
	register i;

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise\n");
		exit(1);
	}
	do {
		pass = getpass("Overall ftp password: ");
		if(*pass == '\0')
			continue;
		/* keep compatability with C library getpass here */
		pass[8] = '\0';
		(void) strcpy(kbuf, pass);
		pass = getpass("Please input again: ");
		pass[8] = '\0';
	} while (strcmp(pass, kbuf));

	strncpy(kbuf, crypt(kbuf, kbuf), 13);
	if((i = creat(KEYFILE, (0660) & 0444)) < 0){
		fprintf(stderr, "Cannot create: %s\n", KEYFILE);
		fprintf(stderr, "Please delete existing file\n");
		exit(1);
	}
	(void) write(i, kbuf, 12);
	(void) write(i, "\n", 1);
	(void) close(i);
	printf("%s made\n", KEYFILE);
	exit(0);
}
