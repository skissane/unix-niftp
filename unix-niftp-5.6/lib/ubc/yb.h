/* unix-niftp lib/ubc/yb.h $Revision: 5.5 $ $Date: 90/08/01 13:39:08 $ */
/*
 *  Socket address structure for Yellow Book transport service
 *
 *  This form avoids kernal restrictions in socket lengths but is
 *  not suitable for use with front ends.
 *  wja@nott.cs
 *
 * $Log:	yb.h,v $
 * Revision 5.5  90/08/01  13:39:08  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:55:13  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:56:27  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

struct sockaddr_yb
	{
	short	syb_addrlen;
	char   *syb_addr;	/* Yellow book address */
	short	syb_qoslen;
	char   *syb_qos;	/* Quality of service (allways ignored) */
	short	syb_explen;
	char   *syb_exp;	/* explanatory text */
	};

#define PF_YBTS 1	/* dummy for now */
