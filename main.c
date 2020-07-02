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

#include "command.h"
#include "status.h"
#include "pv.h"

#define SHMKEY   0x00a00001
#define BUFF_KEY 0x00a00002
#define BUFFSIZE 256
#define MAX_PIPE 10

void debugPrint(Command* commands[], char* tokens[]);

/******************************************************/
void eexecute(Command* commands[], char* tokens[]);
void redirect(int old_fd, int new_fd);
int exe_pipe(Command* commands[], size_t cmdIdx, int fd_in);
void debugPrint(Command* commands[], char* tokens[]);
/******************************************************/

int main(){
    
    int t_size; 
    //"ls -l /dev | grep tty > test.txt &  ps -f > plist ; ls -l *list"; 
    //setup shared-memory, semaphores
    if((shmid=shmget(SHMKEY, sizeof(int), MASK|IPC_CREAT))<0) exitStatus("shmget", 1);
    if((zombies_size=(int*)shmat(shmid, NULL, 0))==(int*)-1) exitStatus("shmat", 1);
    shmctl(shmid, IPC_RMID, 0);
    fd=getsem(BUFF_KEY, 1);
    
    //stimulate input  
    int nr; 
    char* tmp;
    
    char argv[256]; 
    do{
        tmp = (char*)malloc(sizeof(char)*BUFFSIZE);
        Command* cmds[MAX_NUM_TOKENS];  
        
        write(1, "$: ", 3);
        while(1){ //handle slow system call 
            nr=read(0, argv, 256);
            if(nr<=0 && errno==EINTR)
                continue;
            break;
        }
        if(argv[nr-1]=='\n') argv[nr-1]='\0';
        if(strcmp(argv, "exit")==0) break; 
        
        //init command parser 
        strcpy(tmp, argv);
        if((t_size=tokenise(tmp, tokens))<0) break;    
        
        //process
        printf("no of cmd = %d\n", separateCommands(tokens, t_size, cmds));
        debugPrint(cmds, tokens);    
        eexecute(cmds, tokens);
        
        destroy_Commands(cmds);
        free(tmp);
    }while(strcmp(argv, "exit")!=0);

    //cleaning up 
    shmdt(zombies_size); p0(fd); rmsem(fd);
    return 0;
}

void eexecute(Command* commands[], char* tokens[]){
    Command* pipeCmds[MAX_PIPE], *tmpCmd; 
    int i=0, k=0, j=0, status, save_in, save_out; ; 
    pid_t pid; 
    while((tmpCmd=commands[i++])!=(Command*)NULL){
        while(strcmp(tmpCmd->separator,"|")==0){
            pipeCmds[k]=tmpCmd; 
            tmpCmd=commands[i]; 
            i++; k++; 
        }
        if(k!=0){ 
            pipeCmds[k]=commands[i-1];
            pipeCmds[k+1]=(Command*)NULL;    
            
            save_in = dup(0); save_out = dup(1);
            exe_pipe(pipeCmds, 0, 0);
            
            dup2(save_in, 0);dup2(save_out, 1);
            close(save_in);close(save_out);
        }
        //else if(strcmp(tmpCmd->separator,"&")==0)execute(tmpCmd,1); 
        else if(strcmp(tmpCmd->separator,"&")==0)extract_wild(tmpCmd, 1); 
        //else execute(tmpCmd, 0); 
        else extract_wild(tmpCmd, 0); 
    }
}

//thanks to https://gist.github.com/zed/7835043
int exe_pipe(Command* commands[], size_t cmdIdx, int fd_in){
    if(commands[cmdIdx+1]
            ==(Command*)NULL){  
        redirect(fd_in, 0); //check if is background process
        //if(strcmp(commands[cmdIdx]->separator,"&")==0)execute(commands[cmdIdx], 1); 
        if(strcmp(commands[cmdIdx]->separator,"&")==0) extract_wild(commands[cmdIdx], 1);
        //else execute(commands[cmdIdx], 0);
        else extract_wild(commands[cmdIdx], 0);
    }else {
        pid_t pid; 
        int status, fd[2];
        
        if(pipe(fd)<0) exitStatus("pipe", 1); 
        if((pid=fork())<0) exitStatus("fork", 1);
        else if(pid==0){ 
            close(fd[0]); 
            redirect(fd_in, 0); //read from fd_in
            redirect(fd[1], 1); //write to fd[1]
            //execute(commands[cmdIdx], 0);
            extract_wild(commands[cmdIdx], 0);
            exit(0);
        }else { 
            close(fd[1]);
            close(fd_in);
            exe_pipe(commands,cmdIdx+1,fd[0]);
        }          
    }
}

void debugPrint(Command* commands[], char* tokens[]){
    Command* tmpCmd; 
    int i=0, j=0; 
    while((tmpCmd=commands[i++])!=(Command*)NULL){
        printf("command=%s\n", tmpCmd->get_CmdPth(*tmpCmd));
        printf("arguments=");
        while(tmpCmd->arguments[j]!=(char*)NULL) { printf("%s ", tmpCmd->arguments[j++]); }
        j=0; 
        printf("\nseparator=%s\n\n", tmpCmd->separator);
    }
}

void redirect(int old_fd, int new_fd){
    if(old_fd==new_fd)return; 
    if(dup2(old_fd, new_fd)<0) exitStatus("dup2", 1);
    close(old_fd);
}
