/*
	Process handling
 */


#ifndef PROCHANDLE_H
#define PROCHANDLE_H

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif


extern char **ExecExplodeCommand(const char *cmd, int *strc);

extern int ExecProcessExists(pid_t pid);

extern float ExecCPUGetLoad(int cpu_num);

/* Regular execute, uses system(). This is for backwards compatability,
 * but has security issues since it uses system()
 */
extern void Execute(const char *cmd);

/* Non-blocking executes. */
extern pid_t Exec(const char *cmd);
extern pid_t ExecO(
	const char *cmd, const char *stdout_path
);
extern pid_t ExecOE(
	const char *cmd, const char *stdout_path, const char *stderr_path
);
extern pid_t ExecAO(
	const char *cmd, const char *stdout_path
);
extern pid_t ExecAOE(
	const char *cmd, const char *stdout_path, const char *stderr_path
);

/* Blocking executes. */
extern pid_t ExecB(const char *cmd);
extern pid_t ExecBO(
	const char *cmd, const char *stdout_path
);
extern pid_t ExecBOE(
	const char *cmd, const char *stdout_path, const char *stderr_path
);
extern pid_t ExecBAO(
	const char *cmd, const char *stdout_path
);
extern pid_t ExecBAOE(
	const char *cmd, const char *stdout_path, const char *stderr_path
);


#ifdef __cplusplus
}
#endif

#endif	/* PROCHANDLE_H */
