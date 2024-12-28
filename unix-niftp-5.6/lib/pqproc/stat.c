#include "ftp.h"
#include "stat.h"

char	*openedfile;
int	stat_fd	= 0;
int	stat_pos= 0;
struct	s_stat	s_stat;

stat_init(file)
char *file;
{
	int i;

	if (!stat_fd)
	{	static char Logfile[ENOUGH+3];
		if (!file || !*file)
		{	
			sprintf(Logfile, "/tmp/ST.ni.%d", getpid());
			file = Logfile;
		}
		openedfile = file;
		stat_fd = creat(file, 0644);
		/* stat_pos = where(stat_fd); */
	}
	if (stat_fd < 0) return;
	for (i=0; i<sizeof s_stat; i++) ((char *) &s_stat)[i] = '+';
	s_stat.s_newl = '\n';

	s_stat.s_pid	= getpid();
	stat_name("<Unset>");
	stat_addr("<Unset>");
	stat_serv("<Unset>");
	stat_val(-1);
	stat_state(S_IDLE);
}

stat_close(file)
char *file;
{
	if (!file || !*file) file = openedfile;

	stat_state(S_DEAD);
	if (file && *file) unlink(file);
}

stat_state(state)
long state;
{
	if (state != s_stat.s_stat)
	{	s_stat.s_stat = state;
		time(&(s_stat.s_time));
	}
	stat_sync();
}

stat_sync()
{	int rc;
	if (stat_fd < 0) return;

	if (lseek(stat_fd, L_SET, stat_pos) < 0)
	{
		L_LOG_3(L_GENERAL, 0, "Seek on stat file %s (%d) failed %d\n",
			openedfile, stat_fd, errno);
	}
	else if ((rc=write(stat_fd, &s_stat, sizeof s_stat)) != sizeof s_stat)
	{
		L_LOG_4(L_GENERAL, 0, "Write on stat file %s (%d) failed %d/%d\n",
			openedfile, stat_fd, rc, errno);
	}
}

stat_name(name)
char *name;
{	strcpy(s_stat.s_name, name); }

stat_addr(addr)
char *addr;
{	strcpy(s_stat.s_addr, addr); }

stat_serv(serv)
char *serv;
{	strcpy(s_stat.s_serv, serv); }

stat_val(value)
long value;
{	s_stat.s_val = value; }
