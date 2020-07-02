#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "pv.h"
#include "status.h"
#include "command.h"

char* get_CmdPth(Command cmd){ return tokens[cmd.first]; }
unsigned short get_ArgLen(Command cmd){ return cmd.last-cmd.first-1; } 

int is_separator(char c){   //ASCII |=124, &=38, ;=59
    return (c==124 || c==38 || c==59); 
}

void build_Components(Command* cmd){ 
    int i, m, j=0, k=0; 
    for(i=cmd->first; i<=cmd->last; i++){
        cmd->arguments[j] = 
            (char*)malloc(sizeof(char)*BUFF_CMD);
            
        if(strcmp(tokens[i], "<")==0||strcmp(tokens[i], "<<")==0) m=0, i++; 
        else if(strcmp(tokens[i], ">")==0) m=1, i++;
        else if (strcmp(tokens[i], ">>")==0) m=11, i++;
        else if(strcmp(tokens[i], "2>")==0) m=2, i++;
        else {
            strcpy(cmd->arguments[j], tokens[i]);
            j++; continue;            
        }
        cmd->redirectFds[k] = 
            (char*)malloc(sizeof(char)*strlen(tokens[i]));
        strcpy(cmd->redirectFds[k]+2, tokens[i]);
        memmove(cmd->redirectFds[k], (char*)&m, 2);
        k++; 
    }
    cmd->arguments[j]=(char*)NULL;
    cmd->redirectFds[k]=(char*)NULL;
}

void build_Command(Command* cmd, int bg, int end, char sp){
    if(bg>end) exit(-1); //last index > first index 
    cmd->first = bg; 
    cmd->last = end; 
    cmd->separator = (char*)malloc(sizeof(char)*2);   
    cmd->separator[0]=sp, cmd->separator[1]='\0';
    cmd->get_CmdPth = get_CmdPth; 
    cmd->get_ArgLen = get_ArgLen;
    build_Components(cmd); 
}

void destroy_Command(Command* cmd){ 
    int j=0, k=0;
    free(cmd->separator); 
    while(cmd->arguments[j++]!=(char*)NULL) free(cmd->arguments[j]);
    while(cmd->redirectFds[k++]!=(char*)NULL) free(cmd->redirectFds[k]);
}

void destroy_Commands(Command* cmd[]){ 
    int i=0; 
    while(cmd[i]!=(Command*)NULL){
        destroy_Command(cmd[i]);
        free(cmd[i]);
        i++; 
    } 
}

int separateCommands(char* tokens[], size_t token_size, Command* commands[]){
    int t_size, i, regFlg=1, bgIndx, endIndx=0, comdIndx=0; 
    int tk_len, isFrst=0, isLst=0; 
    char sp; 
    
    t_size = token_size; 
    if(t_size<0){ puts("tokens overloaded"); return -1; }
    for(i=0; i<t_size; i++){
        if(regFlg) bgIndx=i, regFlg=0;
        
        tk_len=strlen(tokens[i])-1;
        isFrst=is_separator(tokens[i][0]);
        isLst=is_separator(tokens[i][tk_len]);
        
        if(isFrst||isLst){  //only take first/ last index of token i 
            sp=isLst ? tokens[i][tk_len]: tokens[i][0]; 
            if(tk_len!=0){
                assert((isFrst&&isLst)!=1);
                endIndx=i;    
                tokens[i][tk_len]='\0'; 
            }else endIndx=i-1; 
            
            commands[comdIndx]=(Command*)malloc(sizeof(Command)); 
            build_Command(commands[comdIndx], 
                        bgIndx, endIndx, sp);
            regFlg = 1;
            comdIndx++; 
        }
    }
    if(!regFlg){ //registered but haven't claimed
        commands[comdIndx] = (Command*)malloc(sizeof(Command)); 
        build_Command(commands[comdIndx], bgIndx, --i, '\0'); 
        comdIndx++; 
    }
    commands[comdIndx] = (Command*)NULL; //mark last index of array 
    return comdIndx; 
}

void update_zombies(int flg){
   p(fd);
   if(flg) *zombies_size+=1; 
   else *zombies_size-=1; 
   v(fd);
}

void claim_child(int signo){
    int status, i, idx; 
    pid_t pid=1; 
    char st; 
    while(pid>0){
        char* tmp=(char*)malloc(sizeof(char)*32);
        pid=waitpid(0, (int*)0, WNOHANG|WUNTRACED);
        if(pid>0){
            short pid_n; 
            for(i=0; i<*zombies_size&&(pid_n=*(short*)zombies[i])!=(int)pid; i++);
            if(i<*zombies_size) {  //if registered then print
                st=*zombies_size>2?' ': *zombies_size>1?'-':'+';
                idx=i!=*((short*)(zombies[i]+2))?*((short*)(zombies[i]+2)):i;
                sprintf(tmp, "[%d]%c  %s                %s",idx, st, 
                        getZombieStatus(status), zombies[i]+6);
                        
                printf("%s\n", tmp);
                update_zombies(0);
                for(i=i; i<*zombies_size; i++)memcpy(&zombies[i],&zombies[i+1],sizeof zombies[i+1]); 
                zombies[i]=(char*)NULL;
                free(zombies[i]);
            }
        }
        free(tmp);
    }
}

int glob_handler(const char *epath, int errn){
    printf("path %s failed with errno %d\b", epath, errn);
    return 1; 
}

int extract_wild(Command* cmd, int isBg){
    int j, k, rslt, extract=0, i=1, regFlg=0;
    const char* WILD_CARDS = "*?"; 
    char f_path[BUFF_CMD];
    glob_t glob_list;
    char* tmp; 
    
    while((tmp=cmd->arguments[i++])!=(char*)NULL){
        if(tmp[0]=='-') continue; //if is parameter then continue 
        for(j=0; j<strlen(tmp); j++){
            for(k=0; k<strlen(WILD_CARDS); k++){ //force length to last index
                if(tmp[j]==WILD_CARDS[k]) { extract=1; j=strlen(tmp); break; }
            }
        }
        j=k=0;
        if(extract){ 
            if((rslt=glob(tmp,GLOB_TILDE_CHECK,glob_handler,&glob_list))!=0){
                exitGlob(rslt); break ;     
            }
            regFlg=1; 
            for(j=0; j<glob_list.gl_pathc; j++){
                if(strcmp(getenv("HOME"),"/")==0 && //if home is / and user enter ~
                    tmp[0]=='~')strcpy(f_path,glob_list.gl_pathv[j]+1);  //avoid duplicate '/'
                else strcpy(f_path,glob_list.gl_pathv[j]); 
                
                printf("%s:\n", f_path);
                strcpy(cmd->arguments[i-1],f_path);
                execute(cmd, isBg);
                puts("");
            }  
        }
        extract=0; 
    } 
    if(!regFlg) execute(cmd, isBg);
    else globfree(&glob_list);
}

int execute(Command* cmd, int isBg){
    int status, fd_d, fd, i=0; 
    struct sigaction act; 
    pid_t pid; char* tmp; 
    
    if(isBg){
        act.sa_flags=SA_NOCLDSTOP; 
        act.sa_handler=claim_child; 
        sigemptyset(&act.sa_mask); 
        if(sigaction(SIGCHLD, &act, NULL)!=0) exitStatus("sigaction", 1);
    }
    
    if((pid=fork())<0) exitStatus("fork", 1);
    else if(pid>0){
        if(isBg){
            int k=0; 
            zombies[*zombies_size]=(char*)malloc(sizeof(char)*BUFF_CMD);
            strcpy(zombies[*zombies_size]+6,"");
            do{     /*first 2bytes reserved for pid no, next 2 bytes zombie index, remaining cmds*/
                sprintf(zombies[*zombies_size]+6, "%s %s", 
                        zombies[*zombies_size]+6, cmd->arguments[k]); 
            } while(cmd->arguments[++k]!=(char*)NULL); 
            printf("[%d] %d                %s\n", *zombies_size, pid, zombies[*zombies_size]+6);
            
            memmove(zombies[*zombies_size], (char*)&pid, 2);
            memmove(zombies[*zombies_size]+2, (char*)zombies_size, 2);
            update_zombies(1);
        }else { 
            while(1){ //handle slow system call 
                if(waitpid(pid, &status, 0)<0 && errno==EINTR)
                    continue;
                break;
            }
        }
    }else{
        while((tmp=cmd->redirectFds[i++])!=(char*)NULL){
            switch(fd_d=*((short*)tmp)){
                case 0: fd=open(tmp+2, O_RDONLY); break; 
                case 1: case 2: fd=open(tmp+2, O_RDWR|O_CREAT, MASK); break; 
                case 11: fd=open(tmp+2, O_RDWR|O_APPEND|O_CREAT, MASK), fd_d=1; break; 
            }
            if(fd<0) exitStatus("open", 1); 
            if(dup2(fd, fd_d)<0) exitStatus("dup2", 1);
        }
        exit(execvp(cmd->get_CmdPth(*cmd), cmd->arguments));
    }
}

