#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>


#ifndef PARSE_H
#define PARSE_H

#define MAX_ARGS 64
#define BUFFER_SIZE 1024


typedef struct process {
    char command[BUFFER_SIZE];
    __pid_t pid;
    int job_id;
    int terminated;
    int done;
    struct process* next;
} process;

typedef struct bash{
    process* head;
    int stack_num;
    int job_num;
} bash;


/// prompt
void printPrompt (void);
// parse
void parseInst (char *buffer, bash* shell);
// if builtin
bool isBuiltinInst (char *inst);
// builtin
void execBuiltinInst (char *inst);
// execvp
void execInst (char *inst, int src, int dst, bash* shell, char* fullinst);
//add to bg-list
void add_to_proc(char*buffer, bash* shell, pid_t childpid);
//check job condition after parsing each inst
void checkjobs(bash* shell);
//remove job from list
void remove_job(bash* shell, pid_t pid);
//kill job by stack order
void kill_seq(bash* shell, int seq);
//kill job by pid
void kill_pid(bash* shell, pid_t pid);
//find job by pid
process* get_job(bash* shell, pid_t pid);

#endif