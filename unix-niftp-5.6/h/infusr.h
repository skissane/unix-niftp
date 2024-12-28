/* unix-niftp h/infusr.h $Revision: 5.6.1.6 $ $Date: 1993/01/10 07:09:05 $ */
/*
* FILE:
*                       infusr.h
*
* last changed: 9-Jul-85
*
* $Log: infusr.h,v $
 * Revision 5.6.1.6  1993/01/10  07:09:05  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:00:07  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:33:10  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:41:14  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:25:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/
/* header file containing definitions and strings for informing
 * users of outcome of file transfers
*/

#define REASONLN 500    /* max length of reason for failure */
#define NETLNG    50    /* max length of full network name */

char    reason[REASONLN];               /* reason for failure */
char    whichhost[7];                   /* which host gave failure message */
char    network[NETLNG];                /* full name of network */
char    tofrom[5];                      /* direction of transfer */




/* text of subjects */
#define f_f_sub         "File Transfer Failure %s %s - %s"
#define f_warn_sub      "File Transfer Warning"
#define f_ok_sub        "File Transfer Notification"
#define m_f_sub         "Mail Delivery Failure to %s - %s"
#define j_f_sub         "Jtmp Transaction Failure"
#ifdef NEWS
#define n_f_sub		"News Delivery Failure"
#endif NEWS

/* text of messages */
#define f_f_text \
"The transfer of file %s\n\
%s host %s over %s failed.\n\
The reason given by the %s host was:\n\
\n\
%s\n\
%s\n"

#define  f_ok_text \
"The transfer of file %s\n\
%s host %s over %s was\n\
completed successfully\n"

#define   m_f_text  \
"The NIFTP process was unable to\n\
deliver your mail to host %s\n\
over %s.\n\
\n\
The reason given by the %s host was:\n\
\n\
%s\n\
%s\n"

#define   j_f_text  \
"The NIFTP process was unable to\n\
deliver your jtmp job to host %s\n\
over %s.\n\
\n\
The reason given by the %s host was:\n\
\n\
%s\n\
%s\n"

#ifdef NEWS
#define	  n_f_text  \
"The NIFTP process was unable to\n\
deliver your news file %s to host %s\n\
over %s.\n\
\n\
The reason given by the %s host was:\n\
\n\
%s\n\
%s\n"
#endif NEWS

#define f_r_text \
"The NIFTP process is unable to resolve your address because:-\n\
\n\
%s\n\
\n\
Please consult your local network expert\n"

#define m_r_text \
"Transfer rejected while doing a reverse lookup of calling address:-\r\n\
%s"
