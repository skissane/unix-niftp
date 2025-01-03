/* unix-niftp lib/mmdf/mmdf.h $Revision: 5.5 $ $Date: 90/08/01 13:36:18 $ */
/*
 * $Log:	mmdf.h,v $
 * Revision 5.5  90/08/01  13:36:18  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:44  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:45:29  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*			Reply Codes for MMDF

 *  Based on: "Revised FTP Reply Codes", by Jon Postel & Nancy Neigus Arpanet
 *      RFC 640 / NIC 30843, in the "Arpanet Protocol Handbook", E.  Feinler
 *      and J. Postel (eds.), NIC 7104, Network Information Center, SRI
 *      International:  Menlo Park, CA.  (NTIS AD-A0038901)
 *
 *  Actual values are different, but scheme is same.  Codes must fit into
 *  8-bits (to pass on exit() calls); fields are packed 2-3-3 and interpreted
 *  as octal numbers.
 *
 *  Basic format:
 *
 *      0yz: positive completion; entire action done
 *      1yz: positive intermediate; only part done
 *      2yz: Transient negative completion; may work later
 *      3yz: Permanent negative completion; you lose forever
 *
 *      x0z: syntax
 *      x1z: general; doesn't fit any other category
 *      x2z: connections; truly transfer-related
 *      x3z: user/authentication/account
 *      x4x: mail
 *      x5z: file system
 *
 *      3-bit z field is unique to the reply.  In the following,
 *      the RP_xVAL defines are available for masking to obtain a field.
 */
/***************  FIELD DEFINITIONS & BASIC VALUES  ***************** */

/*          Field 1:  Basic degree of success (2-bits)                  */

#define RP_BTYP '\200'            /* good vs. bad; on => bad            */

#define RP_BVAL '\300'            /* basic degree of success            */

#define RP_BOK  '\000'            /* went fine; all done                */
#define RP_BPOK '\100'            /* only the first part got done       */
#define RP_BTNO '\200'            /* temporary failure; try later       */
#define RP_BNO  '\300'            /* not now, nor never; you lose       */

/*          Field 2:  Basic domain of discourse (3-bits)                */

#define RP_CVAL '\070'            /* basic category (domain) of reply   */

#define RP_CSYN '\000'            /* purely a matter of form            */
#define RP_CGEN '\010'            /* couldn't find anywhere else for it */
#define RP_CCON '\020'            /* data-transfer-related issue        */
#define RP_CUSR '\030'            /* pertaining to the user             */
#define RP_CMAI '\040'            /* specific to mail semantics         */
#define RP_CFIL '\050'            /* file system                        */
#define RP_CLIO '\060'            /* local i/o system                   */

/*          Field 3:  Specific value for this reply (3-bits)            */

#define RP_SVAL '\007'            /* specific value of reply            */
/*********************  SPECIFIC SUCCESS VALUES  ******************** */


/*                        Complete Success                              */

#define RP_DONE (RP_BOK | RP_CGEN | '\000')
				  /* done (e.g., w/transaction)         */
#define RP_OK   (RP_BOK | RP_CGEN | '\001')
				  /* general-purpose OK                 */

#define RP_MOK  (RP_BOK | RP_CMAI | '\000')
				  /* message is accepted (w/text)       */
#define RP_DOK  (RP_BOK | RP_CGEN | '\003')
			/* accepted for the delayed submission channel  */


/*                        Partial Success                               */

#define RP_MAST (RP_BPOK| RP_CGEN | '\000')
				  /* you are the requestor              */
#define RP_SLAV (RP_BPOK| RP_CGEN | '\001')
				  /* you are the requestee              */
#define RP_AOK  (RP_BPOK| RP_CMAI | '\000')
				  /* message address is accepted        */
#define RP_HOK  (RP_BPOK| RP_CMAI | '\001')
				  /* host processing completed          */
/*********************  SPECIFIC FALURE VALUES  ********************* */


/*                        Partial Failure                               */

#define RP_AGN  (RP_BTNO | RP_CGEN | '\000')
				  /* not now; maybe later               */
#define RP_TIME (RP_BTNO | RP_CGEN | '\001')
				  /* timeout                            */
#define RP_NOOP (RP_BTNO | RP_CGEN | '\002')
				  /* no-op; nothing done, this time     */
#define RP_EOF  (RP_BTNO | RP_CGEN | '\003')
				  /* encountered an end of file         */

#define RP_NET  (RP_BTNO | RP_CCON | '\000')
				  /* channel went bad                   */
#define RP_BHST (RP_BTNO | RP_CCON | '\001')
				  /* foreign host screwed up            */
#define RP_DHST (RP_BTNO | RP_CCON | '\002')
				  /* host went away                     */
#define RP_NIO  (RP_BTNO | RP_CCON | '\004')
				  /* general net i/o problem            */
#define RP_NS	(RP_BTNO | RP_CCON | '\005')
				  /* temporary nameserver failure	*/

#define RP_FIO  (RP_BTNO | RP_CFIL | '\000')
				  /* error reading/writing file         */
#define RP_FCRT (RP_BTNO | RP_CFIL | '\001')
				  /* unable to create file              */
#define RP_FOPN (RP_BTNO | RP_CFIL | '\002')
				  /* unable to open file                */
#define RP_LIO  (RP_BTNO | RP_CLIO | '\000')
				  /* general local i/o problem          */
#define RP_LOCK (RP_BTNO | RP_CLIO | '\001')
				  /* resource currently locked          */


/*                       Complete Failure                               */

#define RP_MECH (RP_BNO | RP_CGEN | '\000')
				  /* bad mechanism/path; try alternate? */
#define RP_NO   (RP_BNO | RP_CGEN | '\001')
				  /* general-purpose NO                 */

#define RP_PROT (RP_BNO | RP_CCON | '\000')
				  /* general prototocol error           */

#define RP_RPLY (RP_BNO | RP_CCON | '\001')
				  /* bad reply code (PERMANENT ERROR)   */

#define RP_NAUTH (RP_BNO | RP_CUSR  | '\001')
				/* bad authorisation                    */
				/* SEK this will be used for user checks*/

#define RP_NDEL (RP_BNO | RP_CMAI | '\000')
				  /* couldn't deliver                   */

#define RP_HUH  (RP_BNO | RP_CSYN | '\000')
				  /* couldn't parse the request         */
#define RP_NCMD (RP_BNO | RP_CSYN | '\001')
				  /* no such command defined            */
#define RP_PARM (RP_BNO | RP_CSYN | '\002')
				  /* bad parameter                      */
#define RP_UCMD (RP_BNO | RP_CSYN | '\003')
				  /* command not implemented            */
#define RP_USER (RP_BNO | RP_CUSR | '\000')
				  /* unknown user                       */


/*                      STRUCTURE OF A REPLY STRING                     */

struct rp_construct               /* for constant reply conditions      */
{
    char    rp_cval;
    char    rp_cline[50];
};

struct rp_bufstruct               /* for reading reply strings          */
{
    char    rp_val;
    char    rp_line[256];
};

typedef struct rp_bufstruct RP_Buf;

#define rp_conlen(bufnam) (strlen (bufnam.rp_cline) + sizeof (bufnam.rp_cval))

/*              PSEUDO-FUNCTIONS TO ACCESS REPLY INFO                   */

#define rp_gval(val)    ((char) (val))
				  /* get the entire return value        */

/*  The next three give the field's bits, within the whole value        */

#define rp_gbval(val)   (rp_gval (val) & RP_BVAL)
				  /* get the basic part of return value */
#define rp_gcval(val)   (rp_gval (val) & RP_CVAL)
				  /* get the domain part of value       */
#define rp_gsval(val)   (rp_gval (val) & RP_SVAL)
				  /* get the specific part of value     */

/*  The next three give the numeric value withing the field             */

#define rp_gbbit(val)   ((rp_gval (val) >> 6) & 03)
				  /* get the basic part right-shifted   */
#define rp_gcbit(val)   ((rp_gval (val) >> 3 ) & 07)
				  /* get the domain part right-shifted  */
#define rp_gsbit(val)   (rp_gval (val) & 07)
				  /* get the specific part right-shifted */

/*  The following works with SIGNED or UNSIGNED chars!  */
#define rp_isgood(val)  (! rp_isbad(val))
				  /* is return value positive?          */
#define rp_isbad(val)   (rp_gval(val) & 0200)
				  /* is return value negative?          */
