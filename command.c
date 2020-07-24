/* command.h implementation */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <glob.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "pv.h"
#include "status.h"
#include "command.h"
#include "builtin_func.h"

// *******************private function declaration begin

/*
* purpose:	    SIGCHLD signal handler to claim zombies of foregound/backgound processes     
* pre:		    SIGCHLD signal of foreground/background process recieved   
* post:		    claim zombies and update zombies command status
* return value: nil
*/
void claim_child(int signo);

/*
* purpose:	    modify stdout or stdin to another file descriptor
* pre:		    tmp cannot be empty or NULL
* post:		    get 2bytes of tmp and modify stderr, stdout (>, >>) or stdin accordingly 
* return value: nil
*/
void modi_redirect(char* tmp);

/*
* purpose:	    command execution 
* pre:		    cmd cannot be NULL and isBig can either be 0 or 1
* post:		    execute command using execvp from child through forking; background or foreground process depending on isBg  
* return value: return -1 if execution failed 
*/
int execute(Command* cmd, int isBg); 

/*
* purpose:	    error handler for glob 
* pre:		    glob call failed 
* post:		    display error message depending on glob return value  
* return value: return 1 upon success call 
*/
int glob_handler(const char *epath, int errn); 

/*
* purpose:	    searching for redirection symbol and updating command structure redirection members 
* pre:		    cmd cannot be NULL and token cannot be empty 
* post:		    update command structure redirection members value in term of file path 
* return value: nil 
*/
void search_redirection(char *token[], Command *cmd); 

/*
* purpose:	    extract command argument from tokens to command struct 
* pre:		    token cannot be empty and cmd cannot be NULL  
* post:		    cmd arguments are populated with token value without creating any copies  
* return value: nil 
*/
void build_command_arguments(char *token[], Command *cmd); 

/*
* purpose:	    command struct builder  
* pre:		    bg cannot be <= end and cmd cannot be NULL 
* post:		    cmd arguments are populated with token value without creating any copies  
* return value: initialise command struct members  
*/
void build_Command(Command* cmd, int bg, int end, char sp);

/*
* purpose:	    checker for whether passed-in char is a valid separator   
* pre:		    nil 
* post:		    nil   
* return value: 1) return 1 if c matches separator 
*               2) return 0 if c does not match separator 
*/
int is_separator(char c){ return (c==124 || c==38 || c==59);  }

/*
* purpose:	    get argument length from a command struct   
* pre:		    nil  
* post:		    nil   
* return value: return the size of command arguments    
*/
unsigned short get_ArgLen(Command cmd){ return cmd.last-cmd.first-1; }

/*
* purpose:	    freeing pointers of per command struc   
* pre:		    cmd cannot be NULL  
* post:		    freeing pointers of per command struc
* return value: nil  
*/
void destroy_command(Command* cmd){ free(cmd->separator); free(cmd->arguments); free(cmd->stdin_file); free(cmd->stdout_file); }

/*
* purpose:	    update the size of jobs    
* pre:		    semaphore key must be existed and initialised 
* post:		    +1 or -1 size of jobs
* return value: nil  
*/
void update_jobs(int flg);

/*
* purpose:	    update the size of zombies    
* pre:		    semaphore key must be existed and initialised 
* post:		    +1 or -1 size of zombies
* return value: nil  
*/
void update_zombies(int flg);

// private function declaration end*******************

void destroyCommands(Command cmd[], int comdIndx){ for(int i=0;i<comdIndx;destroy_command(&(cmd[i++]))); }

void configGlob(unsigned int val) { 
    assert(val<2); 
    char ready[32];
    sprintf(ready, "glob expansion %s!\n", val?"enabled":"disabled"); 
    exitStatus(ready, 1); 
    glob_enable=val; 
}

void build_command_arguments(char *token[], Command *cmd) {
    int b_size, i, k=0;
    b_size=(cmd->last-cmd->first+2);
    b_size=cmd->stdout_file!=NULL ? b_size-2 :
        cmd->stdin_file!=NULL ? b_size-2 : b_size;
    assert(b_size>0);
    for(i=cmd->first; i<=cmd->last; i++){
        if( strcmp(token[i], "<")==0 || strcmp(token[i], ">")==0 ||
            strcmp(token[i], ">>")==0 || strcmp(token[i], "2>")==0 ) i++;   
        else {
            if((cmd->arguments=realloc(cmd->arguments, sizeof(char*)*b_size))==NULL) exitStatus("build_command_argument(): reallocation failed", 2);
            cmd->arguments[k]=token[i]; 
            k++; 
        }
    }
    cmd->arguments[k]=(char*)NULL; 
}

void search_redirection(char *token[], Command *cmd) { 
    int i, m=0;  
    for(i=cmd->first; i<=cmd->last; i++){
        
        if(strcmp(tokens[i], "<")==0||strcmp(tokens[i], "<<")==0) {
            m=0, i++; 
            cmd->stdin_file=realloc(cmd->stdin_file, sizeof(char)*strlen(tokens[i]));
            strcpy(cmd->stdin_file+2, tokens[i]);
            memmove(cmd->stdin_file, (char*)&m, 2);
            continue;   
        } 
        else if(strcmp(tokens[i], ">")==0) m=1, i++;    // stdout | create a file or clean file content if it exits 
        else if (strcmp(tokens[i], ">>")==0) m=11, i++; // stdout | pending (write at the last offste of a file)
        else if(strcmp(tokens[i], "2>")==0) m=2, i++;   // stderr
        
        if(m>0){
            cmd->stdout_file=realloc(cmd->stdout_file, sizeof(char)*strlen(tokens[i]));
            strcpy(cmd->stdout_file+2, tokens[i]);
            memmove(cmd->stdout_file, (char*)&m, 2);  
        }
    }
}

void build_Command(Command* cmd, int bg, int end, char sp) {
    assert(bg<=end);    // begin index must be > ending index 
    cmd->first=bg; 
    cmd->last=end; 
    cmd->separator=(char*)malloc(sizeof(char)*2);   
    cmd->separator[0]=sp, cmd->separator[1]='\0';
    cmd->arguments=NULL;
    cmd->stdout_file=NULL;
    cmd->stdin_file=NULL;
    cmd->get_ArgLen=get_ArgLen;
}

int separateCommands(char* tokens[], size_t token_size, Command commands[]) {
    int t_size, i, regFlg=1, bgIndx, endIndx=0, comdIndx=0; 
    int tk_len, isFrst=0, isLst=0; 
    char sp; 
    
    t_size = token_size; 
    if(t_size<0){ exitStatus("separateCommands(): tokens overloaded", 1); return -1; }
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
            
            build_Command(&(commands[comdIndx]), bgIndx, endIndx, sp);
            search_redirection(tokens, &(commands[comdIndx]));
            build_command_arguments(tokens, &(commands[comdIndx])); 
            regFlg=1; comdIndx++; 
        }
    }
    if(!regFlg){ //registered but haven't claimed
        build_Command(&(commands[comdIndx]), bgIndx, --i, '\0');
        search_redirection(tokens, &(commands[comdIndx]));
        build_command_arguments(tokens, &(commands[comdIndx]));
        comdIndx++; 
    }
    return comdIndx; 
}

void update_jobs(int flg) {
   p(jb);
   if(flg) *jobs_size+=1; 
   else *jobs_size-=1; 
   v(jb);
}

void update_zombies(int flg) {
   p(fd);
   if(flg) *zombies_size+=1; 
   else *zombies_size-=1; 
   v(fd);
}

void getZombieBuff(){ 
    if(strcmp(zombie_buff, "")!=0) {
        printf("\n%s\n", zombie_buff); 
        strcpy(zombie_buff, "");        
    } else puts("");
}

int glob_handler(const char *epath, int errn) {
    char buf[64]; 
    sprintf(buf, "glob_handler(): path %s failed with errno %d", epath, errn);
    exitStatus(buf, 1);
    return 1; 
}

void modi_redirect(char* tmp) {
    int fd_d, fd;
    if(tmp==NULL) return;
    switch(fd_d=*((short*)tmp)){    // first 2 bytes of tmp
        case 0: fd=open(tmp+2, O_RDONLY); break; 
        case 1: case 2: fd=open(tmp+2, O_RDWR|O_CREAT|O_TRUNC, MASK); break; 
        case 11: fd=open(tmp+2, O_RDWR|O_APPEND|O_CREAT, MASK), fd_d=1; break; 
    }
    if(fd<0) exitStatus("modi_redirection(): cannot open file descriptor", 1); 
    if(dup2(fd, fd_d)<0) exitStatus("modi_redirection(): cannot duplicate stdout/ stdin", 1);
}

void stopJob(int signo){ 
    int j_idx=*jobs_size-1;
    if(j_idx<0) return; 
    printf("[%d]+  Stopped %15s\n", j_idx, jobs[j_idx]+6);
    kill(*(short*)(jobs[*jobs_size-1]), SIGSTOP);
}

void claim_child(int signo) {
    char st, *t_zombie_buff=zombie_buff;
    int status, i, idx; 
    pid_t pid=1; 
 
    while(pid>0){
        pid=waitpid(0, &status, WNOHANG|WUNTRACED);
        if(pid>0){
            for(i=0; i<*zombies_size&&*(short*)zombies[i]!=(int)pid; i++);
            if(i<*zombies_size) {  // if registered then print
                st=*zombies_size>2?' ': *zombies_size>1?'-':'+';
                
		idx=i!=*((short*)(zombies[i]+2))?*((short*)(zombies[i]+2)):i;
   		if(strlen(zombie_buff)<BUFF_CMD) {
                    sprintf(zombie_buff, "%s[%d]%c  %s                %s\n",
                            t_zombie_buff, idx, st, getZombieStatus(status), zombies[i]+6);
                    
                    update_zombies(0);
                    for(i=i; i<*zombies_size; i++)memcpy(&zombies[i],&zombies[i+1],sizeof zombies[i+1]);
                } else exitStatus("claim_child(): child buffer overloaded", 1);
                zombies[i]=(char*)NULL;
                free(zombies[i]); 
            }
        }
    }
}

void resumeJob(){
    int j_idx, status;
    j_idx= *jobs_size-1;
    
    if(j_idx>=0){ // if there is not stopped job in stack 
        kill(*(short*)(jobs[j_idx]), SIGCONT); 
        printf("[%d]+  Continued %15s\n", j_idx, jobs[j_idx]+6);    
        while(1){ 
            if(waitpid(*(short*)(jobs[j_idx]), &status, WUNTRACED)<0 && errno==EINTR) continue;
            if(WIFEXITED(status)){          // wait for this job finish up or stopped again
                if(!WIFSIGNALED(status)){   // if not terminated by signal assumed as process completed   
                    getChildStatus(status);
                    jobs[j_idx]=(char*)NULL;
					free(jobs[j_idx]);
                    update_jobs(0);
                }
            } 
            break;
        }
    } 
}

int execute(Command* cmd, int isBg) {
    int status, i=0, k=0; sigset_t t_sig;
    struct sigaction act, act_job; 
    pid_t pid; char* tmp; 

    act.sa_flags=SA_NOCLDSTOP;
    sigemptyset(&act.sa_mask); 
    act.sa_handler=claim_child;

   if(sigaction(SIGCHLD, &act, NULL)!=0) exitStatus("execute(): sig_child signal handler set up failed", 1);    
    if((pid=fork())<0) { exitStatus("execute(): forking failed", 1); return -1; }
    else if(pid>0){
        if(isBg){
            zombies[*zombies_size]=(char*)malloc(sizeof(char)*BUFF_CMD);
            strcpy(zombies[*zombies_size]+6,"");
            do{     /*first 2bytes reserved for pid no, next 2 bytes zombie index, remaining cmds*/
                sprintf(zombies[*zombies_size]+6, "%s %s", zombies[*zombies_size]+6, cmd->arguments[k]); 
            } while(cmd->arguments[++k]!=(char*)NULL); 
            printf("[%d] %d %15s\n", *zombies_size, pid, zombies[*zombies_size]+6);
            memmove(zombies[*zombies_size], (char*)&pid, 2);
            memmove(zombies[*zombies_size]+2, (char*)zombies_size, 2);
            update_zombies(1);
        }else { 
            if(strcmp(cmd->separator,"|")!=0){
                jobs[*jobs_size]=(char*)malloc(sizeof(char)*BUFF_CMD);
                strcpy(jobs[*jobs_size]+6,"");
                do{     
                    sprintf(jobs[*jobs_size]+6, "%s %s", jobs[*jobs_size]+6, cmd->arguments[k]); 
                } while(cmd->arguments[++k]!=(char*)NULL); 
                memmove(jobs[*jobs_size], (char*)&pid, 2);
                memmove(jobs[*jobs_size]+2, (char*)jobs_size, 2); update_jobs(1);
            }

           while(1){ // handle slow system call 
                if(waitpid(pid, &status, WUNTRACED|WCONTINUED)<0 && errno==EINTR) continue;
                else if(!WIFSTOPPED(status)){  //stopped not handled here 
                    getChildStatus(status);
					if(k>0){
						jobs[*jobs_size-1]=(char*)NULL;
						free(jobs[*jobs_size-1]);
						update_jobs(0); 
					}    
                }
                break;
            }
        }
    }else{
	if(!isBg){ // unblock SIGIGN, SIGQUIT, SIGTSTP for child process
	    if(sigprocmask(SIG_UNBLOCK, &signs, NULL)<0)
               exitStatus("execute(): unable to unblock mask for child process", 1);
	    }
        modi_redirect(cmd->stdin_file);
        modi_redirect(cmd->stdout_file);
        exit(execvp(cmd->arguments[0], cmd->arguments));
    }
}

int extractWild(Command* cmd, int isBg) {
    int j, k, rslt, extract=0, i=1, regFlg=0, homeIdx=-1;
    char f_path[PATH_MAX], *tmp;
    glob_t glob_list;
    
    while((tmp=cmd->arguments[i++])!=(char*)NULL){
        if(tmp[0]=='-') continue; // if is parameter then continue 
        for(j=0; j<strlen(tmp); j++){
            if(tmp[j]=='~') homeIdx=j;
            for(k=0; k<strlen(WILD_CARDS); k++){ // force length to last index
                if(tmp[j]==WILD_CARDS[k]) { extract=1; j=strlen(tmp); break; }
            }
        }
        if(!glob_enable) goto UNEXPANDABLE;     // if glob disabled jump to execution 
        j=k=0;
        if(extract){ 
            if((rslt=glob(tmp,GLOB_TILDE_CHECK,glob_handler,&glob_list))!=0){
                exitGlob(rslt); break;     
            }
            regFlg=1; 
            for(j=0; j<glob_list.gl_pathc; j++){
                if(strcmp(P_HOME,"/")==0 && //if home is / and user enter ~
                    tmp[0]=='~')strcpy(f_path,glob_list.gl_pathv[j]+1);  //avoid duplicate '/'
                else strcpy(f_path,glob_list.gl_pathv[j]); 
                strcpy(cmd->arguments[i-1],f_path);
		execute(cmd, isBg);
            }   
        }extract=0; 
    } 
    if(!regFlg) {   
        UNEXPANDABLE:
            if(homeIdx!=-1) initHomePath(cmd->arguments[i-2]);
            execute(cmd, isBg);
    }
    else globfree(&glob_list);
}
