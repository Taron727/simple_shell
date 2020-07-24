/* shell.h implementation */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>

#include "shell.h"
#include "status.h"
#include "pv.h"

// *******************private function declaration begin

/*
* purpose:	    dup2() implementation; duplicate old_fd to new_fd     
* pre:		    nil 
* post:		    nil  
*/
void redirect(int old_fd, int new_fd);

/*
* purpose:	    recursive pipe execution      
* pre:		    comdIdx > 0
* post:		    calling command execution while ammending stdin/ stdout recursively  
*/
int exe_pipe(Command* commands[], size_t cmdIdx, int fd_in);

// private function declaration end*********************

//setup shared-memory, semaphores
void initShellEnv() {
    // set up share-memory keys
    if((shmid=shmget(SHMKEY, sizeof(int), MASK|IPC_CREAT))<0) exitStatus("initShellEnv(): get shared-memory keys failed", 2); 
    if((shmid2=shmget(SHMKEY2, sizeof(int), MASK|IPC_CREAT))<0) exitStatus("initShellEnv(): get shared-memory keys failed", 2); 
    // attach share memory keys 
    if((zombies_size=(int*)shmat(shmid, NULL, 0))==(int*)-1) exitStatus("initShellEnv(): attach shared-memory keys failed ", 2); 
    if((jobs_size=(int*)shmat(shmid2, NULL, 0))==(int*)-1) exitStatus("initShellEnv(): attach shared-memory keys failed ", 2);
    // set up signal blocking for SIGINT, SIGQUIT
    if( sigemptyset(&signs)==0) {
        sigaddset(&signs, SIGINT);
        sigaddset(&signs, SIGQUIT);
    }else exitStatus("initShellEnv(): failed to empty ignorable signal set", 2); 
    // set up signal blocking fro SIGTSTP
    sigemptyset(&act_job.sa_mask);
    act_job.sa_handler=stopJob; 
    if(sigaction(SIGTSTP, &act_job, NULL)!=0) 
        exitStatus("execute(): stop_job signal handler set up failed", 1);
    // block SIGINT, SIGQUIT
    if(sigprocmask(SIG_SETMASK, &signs, NULL)<0) exitStatus("initSellEnv(): setmask", 2); 
    shmctl(shmid, IPC_RMID, 0); shmctl(shmid2, IPC_RMID, 0);
    // set up zombie, jobs adders 
    fd=getsem(BUFF_KEY, 1); jb=getsem(BUFF_KEY2, 1); 
}

void destroyShellEnv() { // skipped unblock signals for SIGINT, SIGQUIT
    shmdt(zombies_size); shmdt(jobs_size); 
    p0(fd); rmsem(fd); 
    p0(jb); rmsem(jb); 
}

void eexecute(Command commands[], char* tokens[], int size){
    Command* pipeCmds[MAX_PIPE], *tmpCmd; 
    int k=0, j=0, status, save_in, save_out;
    
    for(int i=0; i<size; i++){
        tmpCmd=&(commands[i]);
        while(strcmp(tmpCmd->separator,"|")==0){
            if(k>=MAX_PIPE) { // each pipe contains 2 cmds, the input is piped to previous input thus repeated
    		    exitStatus("eexecute(): pipe size overloaded", 1); 
    		    return; 
	        }
            pipeCmds[k]=&(commands[i]);
            tmpCmd=&(commands[++i]);
            pipeCmds[k+1]=&(commands[i]);
            k+=2;
        }
        if(k!=0){   // if no pipe is found then nominal execution 
	        pipeCmds[k]=(Command*)NULL;
            save_in = dup(0); save_out = dup(1);
            exe_pipe(pipeCmds, 0, 0);
            
            dup2(save_in, 0);dup2(save_out, 1);
            close(save_in);close(save_out);
            k=0; 
        }
        else if(strcmp(tmpCmd->separator,"&")==0) extractWild(tmpCmd, 1); 
        else extractWild(tmpCmd, 0);
    }
}

int exe_pipe(Command* commands[], size_t cmdIdx, int fd_in) {
    if(commands[cmdIdx+1]
            ==(Command*)NULL){  // breaker of recursive algo
        redirect(fd_in, 0); // read from fd_in; read will be blocked unitl fd[1] is not empty 
        if(strcmp(commands[cmdIdx]->separator,"&")==0) extractWild(commands[cmdIdx], 1); // background
        else extractWild(commands[cmdIdx], 0);  // foreground
    }else {
        pid_t pid; 
        int status, fd[2];
        
        if(pipe(fd)<0) exitStatus("exe_pipe(): pipe", 2); 
        if((pid=fork())<0) exitStatus("exe_pipe(): fork", 2);
        else if(pid==0){ 
            close(fd[0]); 
            redirect(fd_in, 0); // read from fd_in; read will be blocked unitl fd[1] is not empty 
            redirect(fd[1], 1); // write to fd[1]
            extractWild(commands[cmdIdx], 0);   // separator has to be | thus not background process 
            exit(0);
        }else { 
            close(fd[1]);
            close(fd_in);   // read end of pipe depending on caller 
            exe_pipe(commands,cmdIdx+1,fd[0]);  // execute next command
        }          
    }
}

void redirect(int old_fd, int new_fd){
    if(old_fd==new_fd) return;  // return immediately if old and new are ady same 
    if(dup2(old_fd, new_fd)<0) exitStatus("exe_pipe(): dup2", 2);     // duplicate old_fd and close new_fd
    close(old_fd);  // release extra resources
}
