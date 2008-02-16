#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef _USE_BSD
# define _USE_BSD
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "../include/os.h"
#include "../include/string.h"
#include "../include/prochandle.h"


static void SignalChildCB(int s);
static void *SignalAdd(int signum, void (*handler)(int));

/* Explode commands */
char **ExecExplodeCommand(const char *cmd, int *strc);

/* Process checking */
int ExecProcessExists(pid_t pid);

/* Load monitoring */
float ExecCPUGetLoad(int cpu_num);

/* Regular execute */
void Execute(const char *cmd);

/* Non-blocking executes */
static pid_t ExecNexus(
	const char *cmd,
	const char *stdout_path, const char *stderr_path,
	const char *stdout_mode, const char *stderr_mode
);
pid_t Exec(const char *cmd);
pid_t ExecO(const char *cmd, const char *stdout_path);
pid_t ExecOE(const char *cmd, const char *stdout_path, const char *stderr_path);
pid_t ExecAO(const char *cmd, const char *stdout_path);
pid_t ExecAOE(const char *cmd, const char *stdout_path, const char *stderr_path);

/* Blocking executes */
static pid_t ExecBNexus(
	const char *cmd,
	const char *stdout_path, const char *stderr_path,
	const char *stdout_mode, const char *stderr_mode,
	int *status
);
pid_t ExecB(const char *cmd);
pid_t ExecBO(const char *cmd, const char *stdout_path);
pid_t ExecBOE(const char *cmd, const char *stdout_path, const char *stderr_path);
pid_t ExecBAO(const char *cmd, const char *stdout_path);
pid_t ExecBAOE(const char *cmd, const char *stdout_path, const char *stderr_path);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define ABSOLUTE(x)	(((x) < 0) ? -(x) : (x))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : 1)


/*
 *	SIGCHLD. signal callback.
 */
static void SignalChildCB(int s)
{
	int status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) > 0);
}

/*
 *	BSD style signal(2).
 *
 *	Returns the old signal action handler of type
 *	void (*handler)(int).
 */
static void *SignalAdd(int signum, void (*handler)(int))
{
	struct sigaction act, old_act;

	act.sa_handler = handler;
	act.sa_flags = 0;

	if(sigaction(signum, &act, &old_act) == -1)
	    return((void *)SIG_ERR);
	else
	    return((void *)old_act.sa_handler);
}

/*
 *	Explodes the command string cmd and returns an array of
 *	dynamically allocated strings.
 *
 *	Arguments inside quotes ('"') with spaces will not be exploded.
 *
 *	Arguments will be delimited by one or more spaces (not tabs).
 *
 *	The calling function must free() the returned strings and the
 *	array.
 */
char **ExecExplodeCommand(const char *cmd, int *strc)
{
	const char *s = cmd, *s_end;
	int len, total = 0;
	char *s_cur, **strv = NULL;

#define STRIP_BACKSLASHES(s)		{		\
 while(*(s) != '\0')					\
 {							\
  /* Backslash? */					\
  if(*(s) == '\\')					\
  {							\
   char *s2 = (s),					\
	c = *(s2 + 1);	/* Record escaped char */	\
							\
   /* Strip backslash */				\
   do { *s2 = *(s2 + 1); s2++; }			\
    while(*s2 != '\0');					\
							\
   /* Replace with appropriate escapee */		\
   switch(c) {						\
    case 'n':						\
     *(s) = '\n';					\
     break;						\
    case 'r':						\
     *(s) = '\r';					\
     break;						\
    case 't':						\
     *(s) = '\t';					\
     break;						\
    case 'e':						\
     *(s) = '\e';					\
     break;						\
    case 'b':						\
     *(s) = '\b';					\
     break;						\
   }							\
   if(*(s) != '\0')					\
    (s)++;						\
  }							\
  else							\
  {							\
   (s)++;						\
  }							\
 }							\
}

#define APPEND_STRING(_s_,_len_)	{		\
 int i = total;						\
 total = i + 1;						\
 strv = (char **)realloc(				\
  strv, total * sizeof(char *)				\
 );							\
 if(strv == NULL) {					\
  if(strc != NULL)					\
   *strc = 0;						\
  errno = ENOMEM;					\
  return(NULL);						\
 }							\
 strv[i] = s_cur = (char *)malloc(			\
  ((_len_) + 1) * sizeof(char)				\
 );							\
 if((_len_) > 0)					\
  memcpy(s_cur, (_s_), (_len_) * sizeof(char));		\
 s_cur[(_len_)] = '\0';					\
}

	if(s == NULL)
	{
	    if(strc != NULL)
		*strc = 0;
	    errno = EINVAL;
	    return(strv);
	}

	/* Seek past initial spaces */
	while(ISBLANK(*s))
	    s++;

	/* Iterate through command string */
	while(*s != '\0')
	{
	    /* Argument in quotes? */
	    if(*s == '"')
	    {
		s++;

		/* Seek s_end to end of current argument (the next
		 * unescaped quote character)
		 */
		s_end = s;
		while(*s_end != '\0')
		{
		    if(*s_end == '\\')
		    {
			s_end++;
			if(*s_end != '\0')
			    s_end++;
		    }
		    else if(*s_end == '"')
		    {
			break;
		    }
		    else
		    {
			s_end++;
		    }
		}

		/* Calculate length of this argument and append it to
		 * the return list of strings
		 */
		len = s_end - s;
		if(len >= 0)
		{
		    APPEND_STRING(s, len);
		    STRIP_BACKSLASHES(s_cur);
		}

		/* Seek to next argument */
		s = s_end;
		if(*s == '"')
		    s++;
		while(ISBLANK(*s))
		    s++;
	    }
	    else
	    {
		/* Seek s_end to end of current argument */
		s_end = s;
		while(!ISBLANK(*s_end) && (*s_end != '\0'))
		    s_end++;

		/* Calculate length of this argument and append it to
		 * the return list of strings
		 */
		len = s_end - s;
		if(len >= 0)
		    APPEND_STRING(s, s_end - s);

		/* Seek to next argument */
		s = s_end;
		while(ISBLANK(*s))
		    s++;
	    }
	}

	/* Update return */
	if(strc != NULL)
	    *strc = total;

	errno = 0;
	return(strv);
#undef APPEND_STRING
#undef STRIP_BACKSLASHES
}


/*
 *	Returns true if the process is still running.
 */
int ExecProcessExists(pid_t pid)
{
#if defined(__linux__)
	char path[PATH_MAX + NAME_MAX];
	sprintf(path, "/proc/%i", pid);
	return(access(path, F_OK) ? 0 : 1);
#else
	struct sched_param sp;

	/* The pid cannot be 0, since that means "itself" to
	 * sched_getparam()
	 */
	if(pid == 0)
	{
	    errno = EINVAL;
	    return(0);
	}

	if(sched_getparam(
	    pid,
	    &sp
	) == 0)
	    return(1);
	else
	    return(0);
#endif
}


/*
 *	Returns the CPU load as a coefficient value from 0.0 to 1.0.
 *
 *	If cpu_num is -1 then the default cpu is used.
 */
float ExecCPUGetLoad(int cpu_num)
{
	static int n = 0;
	static int ct[2][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
	int i, t;
	float v;
	char *s = NULL, cpu_name[40], buf[256];
	int d[4];
	FILE *fp;

	/* Get cpu name */
	if(cpu_num > -1)
	    sprintf(cpu_name, "cpu%i", cpu_num);
	else
	    strcpy(cpu_name, "cpu");

	/* Open the stat file for reading */
	fp = fopen("/proc/stat", "rb");
	if(fp == NULL)
	    return(0);

	/* Read each line until we get to the one that contains the
	 * cpu information that we want
	 */
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
	    /* This line starts with cpu_name? */
	    if(strpfx(buf, cpu_name))
	    {
		s = buf + STRLEN(cpu_name);
		while(ISBLANK(*s))
		    s++;
		break;
	    }
	}
	fclose(fp);

	if(s == NULL)
	    return(0);

	sscanf(
	    s,
	    "%u %u %u %u",
	    &ct[n][0], &ct[n][1], &ct[n][2], &ct[n][3]
	);

	t = 0;
	for(i = 0; i < 4; i++)
	    t += (d[i] = ABSOLUTE(ct[n][i] - ct[1 - n][i]));
	if(t <= 0)
	    return(0.0f);

	v = (float)(t - d[3]) / (float)t;

	n = 1 - n;

	return(v);
}


/*
 *	Old execute, fork()s off process and uses system() to execute
 *	command.
 *
 *	This function is provided for backwards compatability, but has
 *	security issues because it uses the unsafe system().
 */
void Execute(const char *cmd)
{
	if(cmd == NULL)
	{
	    errno = EINVAL;
	    return;
	}

        /* Set the SIGCHLD signal callback */
	SignalAdd(SIGCHLD, SignalChildCB);

	/* Fork off a process */
	switch(fork())
	{
	  /* Forking error */
	  case -1:
	    errno = ENOMEM;
	    return;
	    break;

	  /* We are the child: run then command then exit */  
	  case 0:
	    if(system(cmd) == -1)
		exit(-1);
	    else
		exit(0);
	    break;

	  /* We are the parent: do nothing */        
	  default:
	    errno = 0;
	    break;
	}
}


/*
 *	Main nexus for all Exec*() (non-blocking) functions.
 *
 *	If stdout_path and/or stderr_path are not NULL then their
 *	respective files will be opened and the child's stdout and/or
 *	stderr streams will be redirected to them.
 *
 *	The current working directory will be changed to the executed
 *	file's parent directory on the child process.
 *
 *	Returns the process ID or 0 on error.
 */
static pid_t ExecNexus(
	const char *cmd,
	const char *stdout_path, const char *stderr_path,
	const char *stdout_mode, const char *stderr_mode
)
{
	int argc, error_code;
	char **argv;
	FILE *stdout_fp, *stderr_fp;
	pid_t p;

	if(cmd == NULL)
	{
	    errno = EINVAL;
	    return(0);
	}

	/* Create stdout file (as needed) */
	if(!STRISEMPTY(stdout_path) && !STRISEMPTY(stdout_mode))
	    stdout_fp = fopen(stdout_path, stdout_mode);
	else
	    stdout_fp = NULL;

	/* Create stderr file (as needed) */
	if(!STRISEMPTY(stderr_path) && !STRISEMPTY(stderr_mode))
	    stderr_fp = fopen(stderr_path, stderr_mode);
	else
	    stderr_fp = NULL;

	/* Set stdout file stream options */
	if(stdout_fp != NULL)
	{
	    setvbuf(stdout_fp, NULL, _IOLBF, 0);
	}
	/* Set stderr file stream options */
	if(stderr_fp != NULL)
	{
	    setvbuf(stderr_fp, NULL, _IOLBF, 0);
	}

	/* Explode the command */
	argv = ExecExplodeCommand(cmd, &argc);
	if((argv == NULL) || (argc < 1))
	{
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = EINVAL;
	    return(0);
	}

	/* Set the last command to NULL */
	argv = strlistappend(argv, &argc, NULL);
	if(argv == NULL)
	{
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = ENOMEM;
	    return(0);
	}

#if 0
	/* Object specified by command not executable? */
	if(access(argv[0], X_OK))
	{
	    error_code = errno;
	    strlistfree(argv, argc);
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = error_code;
	    return(0);
	}
#endif

        /* Set the SIGCHLD signal callback */
	SignalAdd(SIGCHLD, SignalChildCB);

	/* Fork off a process */
	p = fork();
	switch(p)
	{
	  /* Forking error */
	  case -1:
	    error_code = errno;
	    strlistfree(argv, argc);
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = error_code;
	    return(0);
	    break;

	  /* We are the child: run the command then exit */
	  case 0:
	    /* Redirect child's stdout and stderr streams to our
	     * opened output files (if any)
	     */
	    if(stdout_fp != NULL)
		dup2(fileno(stdout_fp), fileno(stdout));
	    if(stderr_fp != NULL)
		dup2(fileno(stderr_fp), fileno(stderr));

	    /* Execute command and arguments */
	    execvp(argv[0], argv);
	    exit(0);
	    break;

	  /* We are the parent: Do nothing */
	  default:
	    break;
	}

	/* Delete the exploded command */
	strlistfree(argv, argc);

	/* Close the output files */
	if(stdout_fp != NULL)
	    fclose(stdout_fp);
	if(stderr_fp != NULL)
	    fclose(stderr_fp);

	/* Return the process id of child */
	return(p);
}


pid_t Exec(const char *cmd)
{
	return(ExecNexus(cmd, NULL, NULL, NULL, NULL));
}

pid_t ExecO(const char *cmd, const char *stdout_path)
{
	return(ExecNexus(cmd, stdout_path, NULL, "w", NULL));
}

pid_t ExecOE(const char *cmd, const char *stdout_path, const char *stderr_path)
{
	return(ExecNexus(cmd, stdout_path, stderr_path, "w", "w"));
}

pid_t ExecAO(const char *cmd, const char *stdout_path)
{
	return(ExecNexus(cmd, stdout_path, NULL, "a", NULL));
}

pid_t ExecAOE(const char *cmd, const char *stdout_path, const char *stderr_path)
{
	return(ExecNexus(cmd, stdout_path, stderr_path, "a", "a"));
}


/*
 *	Main nexus for all ExecB*() (blocking) functions.
 *
 *	If the given stdout_path and/or stderr_path are not NULL then
 *	their respective files will be opened and the child's stdout
 *	and/or stderr streams will be redirected to them.
 *
 *	The current working directory will be changed to the executed
 *	file's parent directory on the child process.
 *
 *	Returns the process ID or 0 on error.
 */
static pid_t ExecBNexus(
	const char *cmd,
	const char *stdout_path, const char *stderr_path,
	const char *stdout_mode, const char *stderr_mode,
	int *status
)
{
	int argc, error_code;
	char **argv;
	FILE *stdout_fp, *stderr_fp;
	pid_t p;

	if(cmd == NULL)
	{
	    errno = EINVAL;
	    return(0);
	}

	/* Create stdout file (as needed) */
	if(!STRISEMPTY(stdout_path) && !STRISEMPTY(stdout_mode))
	    stdout_fp = fopen(stdout_path, stdout_mode);
	else
	    stdout_fp = NULL;

	/* Create stderr file (as needed) */
	if(!STRISEMPTY(stderr_path) && !STRISEMPTY(stderr_mode))
	    stderr_fp = fopen(stderr_path, stderr_mode);
	else
	    stderr_fp = NULL;

	/* Set stdout file stream options */
	if(stdout_fp != NULL)
	{
	    setvbuf(stdout_fp, NULL, _IOLBF, 0);
	}
	/* Set stderr file stream options */
	if(stderr_fp != NULL)
	{
	    setvbuf(stderr_fp, NULL, _IOLBF, 0);
	}

	/* Explode the command */
	argv = ExecExplodeCommand(cmd, &argc);
	if((argv == NULL) || (argc < 1))
	{
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = EINVAL;
	    return(0);
	}

	/* Set last argument pointer to be NULL */
	argv = (char **)realloc(argv, (argc + 1) * sizeof(char *));
	if(argv == NULL)
	{
	    error_code = errno;
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = error_code;
	    return(0);
	}
	argv[argc] = NULL;

	/* Object specified by command not executable? */
	if(((*(argv[0]) == '/') || (*(argv[0]) == '.')) && 
	   access(argv[0], X_OK)
	)
	{
	    error_code = errno;
	    strlistfree(argv, argc);
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = error_code;
	    return(0);
	}

	/* Fork off a process */
	p = fork();
	switch(p)
	{
	  /* Forking error */
	  case -1:
	    error_code = errno;
	    strlistfree(argv, argc);
	    if(stdout_fp != NULL)
		fclose(stdout_fp);
	    if(stderr_fp != NULL)
		fclose(stderr_fp);
	    errno = error_code;
	    return(0);
	    break;

	  /* We are the child: run the command then exit */
	  case 0:
	    /* Redirect child's stdout and stderr streams to our
	     * opened output files (if any)
	     */
	    if(stdout_fp != NULL)
	        dup2(fileno(stdout_fp), fileno(stdout));
	    if(stderr_fp != NULL)
		dup2(fileno(stderr_fp), fileno(stderr));

	    /* Execute the command */
	    execvp(argv[0], argv);
	    exit(0);
	    break;

	  /* We are the parent: wait for child to finish */
	  default:
	    waitpid(p, status, 0);
	    break;
	}

	/* Delete the exploded command */
	strlistfree(argv, argc);

	/* Close the output files */
	if(stdout_fp != NULL)
	    fclose(stdout_fp);
	if(stderr_fp != NULL)
	    fclose(stderr_fp);

	return(p);
}

pid_t ExecB(const char *cmd)
{
	return(ExecBNexus(cmd, NULL, NULL, NULL, NULL, NULL));
}

pid_t ExecBO(const char *cmd, const char *stdout_path)
{
	return(ExecBNexus(cmd, stdout_path, NULL, "w", NULL, NULL));
}

pid_t ExecBOE(const char *cmd, const char *stdout_path, const char *stderr_path)
{
	return(ExecBNexus(cmd, stdout_path, stderr_path, "w", "w", NULL));
}

pid_t ExecBAO(const char *cmd, const char *stdout_path)
{
	return(ExecBNexus(cmd, stdout_path, NULL, "a", NULL, NULL));
}

pid_t ExecBAOE(const char *cmd, const char *stdout_path, const char *stderr_path)
{
	return(ExecBNexus(cmd, stdout_path, stderr_path, "a", "a", NULL));
}
