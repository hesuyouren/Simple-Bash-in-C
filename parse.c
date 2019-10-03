#include "parse.h"

void printPrompt (void) {
    char cwd[BUFFER_SIZE] = {0};
    getcwd(cwd, BUFFER_SIZE);
    char hostname[BUFFER_SIZE] = {0};
    gethostname(hostname, BUFFER_SIZE);
    fprintf(stdout, "%s@%s:%s$ ", getlogin(), hostname, cwd);
}

void parseInst (char *buffer, bash* shell) {
    int redirInLoc = BUFFER_SIZE, redirOutLoc = BUFFER_SIZE;
    char *redirIn = NULL, *redirOut = NULL;
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == '>') {
            redirOutLoc = i;
            if (buffer[i + 1] == '>') {
                redirOut = ">>"; i++;
            } else {
                redirOut = ">";
            }
        } else if (buffer[i] == '<') {
            redirInLoc = i;
            redirIn = "<";
        } else if (buffer[i] == ';') {
            buffer[i] = 0;
        } else if (buffer[i] == '\n') {
            buffer[i] = 0;
        }
    }

    // parse size
    int instSize = 0, redirInSize = 0, redirOutSize = 0;
    if (redirInLoc < redirOutLoc) {
        instSize = redirInLoc;
        redirInSize = redirOutLoc - redirInLoc;
        redirOutSize = BUFFER_SIZE - redirOutLoc;
    } else {
        instSize = redirOutLoc;
        redirOutSize = redirInLoc - redirOutLoc;
        redirInSize = BUFFER_SIZE - redirInLoc;
    }

    // parse instruction
    char inst[BUFFER_SIZE] = {0};
    strncpy(inst, buffer, instSize);
    // parse input source
    char redirInSrc[BUFFER_SIZE] = {0};
    if (redirIn) strncpy(redirInSrc, buffer+redirInLoc, redirInSize);
    // parse output source
    char redirOutDst[BUFFER_SIZE] = {0};
    if (redirOut) strncpy(redirOutDst, buffer+redirOutLoc, redirOutSize);

    // open src and dst files
    int src = -1, dst = -1;
    if (redirIn) src = open(strtok(redirInSrc, " <"), O_RDONLY, 0664);
    if (redirOut) {
        if (strcmp(redirOut, ">>") == 0)
            dst = open(strtok(redirOutDst, " >"), O_CREAT | O_WRONLY | O_APPEND, 0664);
        else if (strcmp(redirOut, ">") == 0)
            dst = open(strtok(redirOutDst, " >"), O_CREAT | O_WRONLY | O_TRUNC, 0664);
    }

    char fullinst[BUFFER_SIZE] = {0};
    strncpy(fullinst, inst, BUFFER_SIZE);
    execInst(inst, src, dst, shell, fullinst);

    close(src);
    close(dst);
}

void execInst (char *inst, int src, int dst, bash* shell, char *fullinst) {
    int argc = 0;
    char *argv[MAX_ARGS] = {NULL};
    char *firstInst = strtok(inst, "|");
    char *restInst = strtok(NULL, "");
    argv[0] = strtok(firstInst, " ");
    for (argc = 1; argc < MAX_ARGS; argc++) {
        argv[argc] = strtok(NULL, " "); /* get argument each time */
        if (!argv[argc]) break; /* If there's no other argument, exit the loop */
    }

    bool background = false;
    for (int i = 0; i < argc && argv[i]; i++) {
        if (strncmp(argv[i], "&", 1) == 0) {
            argv[i] = NULL;
            background = true;
        }
    }

    char* op = argv[0];
    char seq[BUFFER_SIZE];
    if (op == NULL) return;
    if (strncmp(op, "exit", 4) == 0) exit(0);
    else if (strncmp(op, "cd", 2) == 0) chdir(argv[1]);
    else if (strncmp(op, "jobs", 4) == 0) checkjobs(shell);
    else if (strncmp(op, "kill", 4) == 0) {
        int order;
        if (argv[1][0] == '%'){
            int i = 1;
            while(argv[1][i]){
                seq[i-1] = argv[1][i];
                i++;
            }
            order = (int)strtol(seq,NULL,10);
            kill_seq(shell,order);
        }
        else {
            order = (int)strtol(argv[1],NULL,10);
            kill_pid(shell,order);
        }
    }
    else {
        if (restInst) {
            int fd[2];
            if (pipe(fd) < 0) exit(-1);
            pid_t childPid = fork();
            if (childPid == 0) {
                if (src >= 0) dup2(src, STDIN_FILENO);
                if (fd[1] >= 0) dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                execvp(op, argv);
                exit(-1);
            } else {
                close(fd[1]);
                execInst(restInst, fd[0], dst, shell, fullinst);
                waitpid(childPid, NULL, 0);
            }
        } else {
            pid_t childPid = fork();
            if (childPid == 0) {
                if (src >= 0) dup2(src, STDIN_FILENO);
                if (dst >= 0) dup2(dst, STDOUT_FILENO);
                execvp(op, argv);
                exit(-1);
            } else {
                if (background) {
                    fullinst = strtok(fullinst, "&");
                    add_to_proc(fullinst,shell,childPid);
                } else waitpid(childPid, NULL, 0);
            }
        }
    }
}

void add_to_proc(char *buffer, bash* shell, pid_t childpid){
    process* new_process;
    char* rear;

    if(shell->head) shell->job_num++;
	else shell->job_num = 1;
    
    new_process = (process*)malloc(sizeof(process));
    
    rear = buffer + strlen(buffer) -1;
    if (*rear == ' '){
    strncpy(new_process->command, buffer, rear-buffer);
    }
    

    new_process->pid = childpid;
    new_process->job_id = shell->job_num;
    new_process->terminated = 0;
    new_process->done = 0;
    new_process->next = NULL;
    
    
    if(shell->head == NULL){
        shell->head = new_process;
        shell->stack_num++;
    }
    else{
        
        process* cur = shell->head;
        process* temp;
        while(cur){
            temp = cur;
            cur = cur->next;
        }
        temp->next = new_process;
        shell->stack_num++;
    }
    
    // printf("[%d] %d\n", shell->job_num, childpid);
    
}

void checkjobs(bash* shell){

    if (shell->stack_num!=0){
        
        process* job = shell->head;
        process* temp;
        while(job){

            int status;
            pid_t jpid = waitpid(job->pid, &status, WNOHANG|WUNTRACED);
        
            if (jpid > 0){
                temp = job->next;
                if(jpid == job->pid){
                    if (WIFSIGNALED(status)){
                        if (job->next == NULL){
                            printf("[%d]+  Terminated              %s\n", job->job_id, job->command);
                            remove_job(shell,job->pid);
                            break;
                        }
                        else if (job->next->next == NULL){
                            printf("[%d]-  Terminated              %s\n", job->job_id, job->command);
                            remove_job(shell,job->pid);
                            job = temp;
                            continue;
                        }
                        else {
                            printf("[%d]   Terminated              %s\n", job->job_id, job->command);
                            
                            remove_job(shell,job->pid);
                            
                            job = temp;
                            
                            continue;
                        }
                    }
                    else{
                        if (job->next == NULL){
                            printf("[%d]+  Done                    %s\n", job->job_id, job->command);
                            remove_job(shell,job->pid);
                            break;
                        }
                        else if (job->next->next == NULL){
                            printf("[%d]-  Done                    %s\n", job->job_id, job->command);
                            remove_job(shell,job->pid);
                            job = temp;
                            continue;
                        }
                        else {
                            printf("[%d]   Done                    %s\n", job->job_id, job->command);
                            remove_job(shell,job->pid);
                            job = temp;
                            continue;
                        }
                    } 
                }
            }
            else{
                if (job->next == NULL){
                    printf("[%d]+  Running                 %s &\n", job->job_id, job->command);
                    break;
                }
                else if (job->next->next == NULL)
                    printf("[%d]-  Running                 %s &\n", job->job_id, job->command);
                
                else 
                    printf("[%d]   Running                 %s &\n", job->job_id, job->command);
            }
            job = job->next;
        }
    }
}


void remove_job(bash* shell, pid_t pid){
    
    process* cur = shell->head;
    process* temp;
    shell->stack_num--;
    if (cur->pid == pid){
        shell->head = cur->next;
        free(cur);
        return;
    }
    while (cur){
        if (cur->pid == pid){
            temp->next = cur->next;
            free(cur);
            return;
        }
        temp = cur;
        cur = cur->next;
    }
}

void kill_pid(bash* shell, pid_t pid){
    process* cur = shell->head;
    while(cur){
        if(cur->pid == pid){
            cur->terminated = 1;
            kill(cur->pid,SIGKILL);
        }
        cur = cur->next;
    }
}

void kill_seq(bash* shell, int seq){
    process* cur = shell->head;
    while(cur){
        if(cur->job_id == seq){
            cur->terminated = 1;
            kill(cur->pid,SIGKILL);
        }
        cur = cur->next;
    }
}

process* get_job(bash* shell, pid_t pid){
    process* cur = shell->head;
    while (cur){
        if (cur->pid == pid){
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}