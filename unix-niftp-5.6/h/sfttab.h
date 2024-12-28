/* unix-niftp h/sftab.h $Revision: 5.5 $ $Date: 90/08/01 13:33:31 $ */
/* the sft table of entries  */
/* remember what the string attributes are */

/* sfftab.h
 * last changed 30-sep-83
 *
 * table which defines which attributes to send and to accept
 *
 * Revision 5.0  87/03/23  03:26:11  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

struct  sftparams sfts[] = {
	PROTOCOL,INTEGER|EQ,MUSTSEND,           0,0,0x0100,0x0100,
	ACCESS  ,INTEGER|EQ,USERSET|MUSTSEND|BIT,  0,0,0,0, /* special */
	TEXTCODE,INTEGER|LE,BIT,                0,0,0x0001,0x0001,
	TEXTFORM,INTEGER|LE,BIT,                0,0,0x00FF,0x0001,
	BINFORM ,INTEGER|EQ,BIT,                0,0,0x8002,0x8002,
	MTRS    ,INTEGER|LE,0,                  0,0,0xFFFF,0xFFFF,
	TRANSLIM,INTEGER|LE,0,                  0,0,0xFFFF,0xFFFF,
	DATAEST	,INTEGER|EQ,USERSET,            0,0,0xFFFF,0xFFFF,
	TRANSID ,INTEGER|EQ,0,                  0,0,0x0000,0,
	PTCN    ,STRING|ANY,NOSUPPORT,          0,0,0,0,        /* string */
	ACKWIND ,MONFLAG|INTEGER|LE,MUSTSEND,	0,0,0x0020,0x0020, /* UCL ? */
	INITRESM,INTEGER|LE,MUSTSEND,           0,0,0x0000,0,
	MINTIME ,INTEGER|ANY,0,                 0,0,0x0258,0x0258,
	FACIL   ,INTEGER|LE,MUSTSEND|BIT,       0,0,0x005F,0,/* facilities */
	STOFTRAN,INTEGER|EQ,BIT,                0,0,0x0000,0,/*what to do ??*/
	DATATYPE,INTEGER|LE,BIT,                0,0,0x000F,0x0001,
	DELIMPRE,INTEGER|LE,BIT,                0,0,0x8000,0,   /* could be */
	TEXTSTC ,INTEGER|ANY,BIT,               0,0,0x0000,0,
	HORIZTAB,STRING|EQ,0,                   0,0,1,1,     /* watch this */
	BINWORD ,INTEGER|LE,0,                  0,0,0x0008*128,0x0008,
	MSTRECS ,INTEGER|ANY,NOSUPPORT,         0,0,0x0000,0,
	PAGEWID ,INTEGER|ANY,NOSUPPORT,         0,0,0x0000,0,
	PAGELEN ,INTEGER|ANY,NOSUPPORT,         0,0,0x0000,0,
	PRISTCD ,STRING|ANY,NOSUPPORT,          0,0,2,2,
	FILENAME,STRING|EQ,MUSTSEND|USERSET|FROMSEL,0,0,3,3,
	USERNAME,STRING|EQ,MUSTSEND|USERSET|FROMSEL,0,0,4,4,
	USERPWD ,STRING|EQ,USERSET|FROMSEL,     0,0,5,5,
	FILEPWD ,STRING|EQ,USERSET|FROMSEL,     0,0,6,6,
	ACCOUNT ,STRING|EQ,USERSET|FROMSEL,     0,0,7,7,
	ACCNTPWD,STRING|EQ,USERSET|FROMSEL,     0,0,8,8,
	OUTDEVT ,STRING|EQ,USERSET|FROMSEL,     0,0,9,9,
	DEVTQ   ,STRING|EQ,USERSET|FROMSEL,     0,0,10,10,
	FILESIZE,INTEGER|EQ,USERSET,            0,0,0xFFFF,0,
	ACTMESS ,STRING|EQ,USERSET|FROMSEL,     0,0,11,11,
	INFMESS ,STRING|EQ,USERSET|FROMSEL,     0,0,12,12,
	SPECOPT ,STRING|EQ,USERSET|FROMSEL,     0,0,14,14,
	0xFF,   /* no value for this - terminator for searches */
	};

/*
 * the sft table. the most important bit of the protocol.
 * flags needed are:-
 *      USER SETABLE
 *      FROM SELECTORS
 *      MUST SEND
 *      TO SEND
 *      NEVER SEND
 *      NOT SUPPORTED
 *      FAILURE
 *      BITVALUE        - as opposed to unsigned integer
 */
