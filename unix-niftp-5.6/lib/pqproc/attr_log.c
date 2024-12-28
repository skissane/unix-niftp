#include "ftp.h"
#define	UNDEFSHORT	-2

void	log_attr( /* attribute, qual, ivalue, svalue */ );

#ifndef	SMALL_PROC
static char *attr_name ( /* attr */ );
static char *qual_name ( /* qual */ );
#endif	SMALL_PROC

log_iattr (a, q, ival)
u_char   a;
u_char   q;
unsigned short  ival;
{
    log_attr (a, q, (long) ival, (char *) 0);
}

log_sattr (a, q, sval)
u_char   a;
u_char   q;
char   *sval;
{
    log_attr ( a, q, (long) UNDEFSHORT, sval);
}

/*
 * Log details of a received NIFTP attribute.
 */
void log_attr (a, q, ival, sval)
u_char  a;
u_char  q;
long  ival;	/* Long so that an invalid value can be passed */
char   *sval;
{

#ifndef	SMALL_PROC
    char   *p;
    char    monitor = (q & MONFLAG);

    if ((p=attr_name (a)) != NULL)
    {    L_LOG_1(L_ALWAYS, L_CONTINUE, "%-12.12s", p); }
    else		   L_LOG_1(L_ALWAYS, L_CONTINUE, "(x)%02x       ", a);

    if ((p = qual_name (q & ~MONFLAG)) != NULL)
    {	 L_LOG_3(L_ALWAYS, L_CONTINUE, "%c%-5.5s%c ", (monitor) ? '{' : '[',
		p, (monitor) ? '}' : ']');
    }
    else L_LOG_1(L_ALWAYS, L_CONTINUE, "[(x)%02x] ", q);
#else	SMALL_PROC
    L_LOG_2(L_ALWAYS, L_CONTINUE, "(x)%02x[(x)%02x] ", a, q);
#endif	SMALL_PROC

 /* 	The format of the value is specified in the qualifier ... */
    switch (q & FORMAT) {
	case ATTRIBUNKNOWN: L_LOG_0(L_ALWAYS, L_CONTINUE, "unknown");	break;
	case NOVALUE:	L_LOG_0(L_ALWAYS, L_CONTINUE, "not specified");	break;
	case INTEGER:	if (ival == UNDEFSHORT)
			{    L_LOG_0(L_ALWAYS, L_CONTINUE, "unset"); }
			else L_LOG_1(L_ALWAYS, L_CONTINUE, "(x)%04x",
					(unsigned short) ival);		break;
	case STRING:	switch(a) {
/*		case USERNAME:
		L_LOG_0(L_ALWAYS, L_CONTINUE, "the user name");	break; */
		case USERPWD:
		L_LOG_0(L_ALWAYS, L_CONTINUE, "the user password");	break;
		case FILEPWD:
		L_LOG_0(L_ALWAYS, L_CONTINUE, "the file password");	break;
		case ACCNTPWD:
		L_LOG_0(L_ALWAYS, L_CONTINUE, "the account password");break;
		default:
		if (sval == NULL)
		{	    L_LOG_0(L_ALWAYS, L_CONTINUE, "unset");
		}
		else	    L_LOG_1(L_ALWAYS, L_CONTINUE, "'%s'", sval);
	    }
	    break;
    }
    L_LOG_0(L_ALWAYS, L_CONTINUE, "\n");
    return;
}

#ifndef	SMALL_PROC
static char *attr_name (attr)
u_char attr;
{
    switch (attr) {
	default:	return (char *) 0;
	case PROTOCOL:	return "ProtiD";	/* 00 */
	case ACCESS:	return "AccessMode";
	case TEXTCODE:	return "TextCode";
	case TEXTFORM:	return "TextFormat";
	case BINFORM:	return "BinFormat";
	case MTRS:	return "MaxRecSize";
	case TRANSLIM:	return "TransLimit";
	case DATAEST:	return "DataEstimate";
	case TRANSID:	return "XferId";
	case PTCN:	return "PrivXferCode";
	case ACKWIND:	return "AckWindow";
	case INITRESM:	return "Restart";
	case MINTIME:	return "MinTimeout";
	case FACIL:	return "Facilities";
	case STOFTRAN:	return "StateOfXfer";	/* 0f */
	case DATATYPE:	return "DataType";	/* 20 */
	case DELIMPRE:	return "DelimPres";
	case TEXTSTC:	return "TextCode";
	case HORIZTAB:	return "HorizTabs";
	case BINWORD:	return "WordSize";
	case MSTRECS:	return "RecordSize";
	case PAGEWID:	return "PageWidth";
	case PAGELEN:	return "PageLength";	/* 27 */
	case PRISTCD:	return "PrivStorCode";	/* 29 */
	case FILENAME:	return "FileName";	/* 40 */
	case USERNAME:	return "UserName";	/* 42 */
	case USERPWD:	return "UserPasswd";	/* 44 */
	case FILEPWD:	return "FilePasswd";	/* 45 */
	case ACCOUNT:	return "Accnt";		/* 4A */
	case ACCNTPWD:	return "AccntPw";	/* 4B */
	case OUTDEVT:	return "OutDevType";	/* 50 */
	case DEVTQ:	return "OutDevQual";	/* 51 */
	case FILESIZE:	return "FileSize";	/* 60 */
	case ACTMESS:	return "ActionMsg";	/* 70 */
	case INFMESS:	return "InfoMsg";	/* 71 */
	case SPECOPT:	return "Options";	/* 80 */
    }
}


static char *qual_name (qual_type)
u_char    qual_type;
{
    switch (qual_type) {
	default:	return (char *) 0;
	case NOVALUE|ANY:	return "==Any";
	case INTEGER|EQ:	return "==Int";
	case INTEGER|NE:	return "!=Int";
	case INTEGER|GE:	return ">=Int";
	case INTEGER|LE:	return "<=Int";
	case STRING |EQ:	return "==Str";
	case STRING |NE:	return "!=Str";
    }
}
#endif	SMALL_PROC
