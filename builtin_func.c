/* builtin_func.h implementation */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#include "builtin_func.h"
#include "shell_env.h"
#include "status.h"

// *******************private function declaration begin

/*
* purpose:		built-in pwd function 
* pre:			args cannot be NULL	 
* post:			display current directory path   
* return value: 	nil    
*/
int f_pwd(char** args);

/*
* purpose:		    built-in set function 
* pre:			    args cannot be NULL	 
* post:			    enable/ disable debug mode or enable/ disable glob extension (useful when running command like ls -l | grep *.c)
* return value: 	1) return -1 if args is NULL, recieve invalid parameter or parameter size 
*				    2) return 0 if f_set successful 
*/
int f_set(char** args);

/*
* purpose:		    built-in exit function 
* pre:			    args cannot be NULL	 
* post:			    set exit key to 1 and exit user interface of shell 
* return value: 	return 0  
*/
int f_exit(char** args);

/*
* purpose:		    built-in directory walk/ cd function 
* pre:			    nil 	 
* post:			    attempt change current directory to args
* return value: 	1) return -1 if chdir() failed and display errno of chdir() accordingly 
*                   1) return 0 upon a success call
*/
int f_cd(char** args);

/*
* purpose:		    built-in prompt function
* pre:			    args cannot be NULL	 
* post:			    update sh_prompt variable to given args 
* return value: 	1) return -1 if args is NULL 
*                   2) return 0 upon a success call 
*/
int f_prompt(char** args); 

/*
* purpose:		    checker for is cmd matches built-in functions 
* pre:			    cmd cannot be NULL  
* post:			    nil  
* return value: 	1) return index of found matched function from h_commands  
*                   2) return -1 if no matches is found 
*/
int is_buitin_cmd(Command *cmd);

/*
* purpose:		    toggler for debug mode  
* pre:			    val cannot be value other than 0 and 1  
* post:			    enable or disable debug mode   
*/
void config_debug(unsigned int val); 

/*
* purpose:		    display commands contents in stdout; only meant for debugging purpose   
* pre:			    comdIndx cannot be 0 or less   
* post:			    nil  
*/
void debug_print_c(Command commands[], char* tokens[], int comdIndx);

// private function declaration end********************

void logBTime(){ time(&begin); }
void logETime(){ time(&end); } 
char* getShellPrompt(){ return sh_prompt; }

void config_debug(unsigned int val){
    assert(val<2);
    char ready[32];
    sprintf(ready, "debug mode %s!\n", val?"enabled":"disabled"); 
    exitStatus(ready, 1);
    debug_enable=val; 
}

int initHomePath(char* path){
    assert(path!=NULL);
    char t_path[PATH_MAX];
    strcpy(t_path, path+1);
    if(P_HOME==NULL) { exitStatus("initHomePath(): home not set",1); return -1; }
    if(strlen(path)==1) strcpy(path,P_HOME);
    else {
	if(strcmp(P_HOME,"/")==0) strcpy(path,t_path);
        else sprintf(path, "%s%s", P_HOME, t_path);
    }
    return 0; 
}

int f_set(char** args) { 
    int toggle;
    if(args[1]==NULL){
        exitStatus("f_set(): empty argument; current environment unchanged",1);
        printf("usage %s <-f|-x>\n", args[0]);
        return -1; 
    }
    if(strlen(args[1])==2){
        if(args[1][0]=='-')toggle=0; 
        else if(args[1][0]=='+')toggle=1;
        switch(args[1][1]){
            case('f'): configGlob(toggle); break;
            case('v'): config_debug(toggle); break;  
            default: exitStatus("f_set(): invalid parameter", 1); return -1; 
        }
        return 0; 
    }exitStatus("f_set(): invalid parameter size", 1); return -1;
}

int f_pwd(char** args) { 
    int i;
    char cur_dir[PATH_MAX];
    if(getcwd(cur_dir, sizeof cur_dir)==NULL) exitStatus("pwd(): home not set",1);
    else puts(cur_dir); 
    return 0; 
}

int f_cd(char** args) { 
    int rsult; char t_home[PATH_MAX]; 
    if(args[1]==NULL){      // if null then set to current directory 
        args[1]=t_home;
        strcpy(args[1],"~");
    }
    if(args[2]!=NULL) exitStatus("f_cd(): invalid argument size",1);
    if(args[1][0]=='~') 
        if(initHomePath(args[1])<0) 
            exitStatus("f_cd(): cannot change directory to ~", 1);
    if((rsult=chdir(args[1]))<0){
	    getDirStatus(errno);
        return -1; 
    }return 0;
}

int f_exit(char** args) { 
    logETime();
    printf("\n\nsession ended at: %s\
session duration: %d seconds\n\
bye bye %s!\n",asctime(localtime(&end)),(int)(end-begin),sh_prompt); 
    unlockEKey(); 
    return 0;
}

int f_prompt(char** args) {
    int i=0, ttl_len=0; 
    char name[MAX_USRNAME_LEN], *t_name;
    t_name=name;
    if(args[1]==NULL) {
        exitStatus("empty argument; value unchanged",1); 
        printf("usage %s <name>\n", args[0]);
        return -1;   
    }
    strcpy(name,""); strcpy(sh_prompt, "!"); 
    while(args[++i]!=NULL) {
        ttl_len+=strlen(args[i]);
        if(ttl_len>=MAX_USRNAME_LEN){   // last string does not added into sh_prompt if oversize
            exitStatus("f_prompt(): oversize username", 1);
            break;
        }
        ttl_len+=1, sprintf(name,"%s%s-", t_name, args[i]);
    }
    name[strlen(name)-1]='\0';  // remove last -
    strcpy(sh_prompt, name); 
    return 0; 
}

int f_fg(char** args){ resumeJob(); }

void initBuiltInCommand() {
    int index, i; fexecute ex;  
    for(i=0;i<MAX_SHELL_CMDS;i++){  // no checker for same labels in h_commands; FIFO structure 
        switch(i){  // label is hardcoded and managed by programmer who added in new built-in function
            case(pwd):index=pwd;ex=f_pwd;strcpy(h_commands[i].label,"pwd");h_command_size++;break;
            case(set):index=set;ex=f_set;strcpy(h_commands[i].label,"set");h_command_size++;break;
            case(cd):index=cd;ex=f_cd;strcpy(h_commands[i].label,"cd");h_command_size++;break;
            case(eexit):index=eexit;ex=f_exit;strcpy(h_commands[i].label,"exit");h_command_size++;break;
            case(prompt):index=prompt;ex=f_prompt;strcpy(h_commands[i].label,"prompt");h_command_size++;break;
            case(fg):index=fg;ex=f_fg;strcpy(h_commands[i].label,"fg");h_command_size++;break;
        }
        h_commands[i].index=index; h_commands[i].exe=ex; 
    }
}

int is_buitin_cmd(Command *cmd) {
    assert(cmd!=NULL); 
    int i;
    h_command* tmp;
    for(i=0; i<h_command_size&&(tmp=&h_commands[i])!=NULL; i++){
        if(strcmp(cmd->arguments[0], h_commands[i].label)==0) return i;
    }
    return -1; 
}

int exeBuiltInCommand(Command *cmd, char* tokens[], int comdIndx){ // this acts as the command manager to either pass it in as homemade or system call
    int bc_index;
    if(debug_enable) debug_print_c(cmd, tokens, comdIndx);
    if(comdIndx>0){
        if((bc_index=is_buitin_cmd(cmd))<0) return -1; // execute using system call 
        else { h_commands[bc_index].exe(cmd->arguments); return 0; }    // execute built-in function and return 0 
    }
}

void debug_print_c(Command commands[], char* tokens[], int comdIndx) {  // does not show index number of commands 
    printf("no of cmd = %d\n", comdIndx);
    for(int i=0; i<comdIndx; i++){
        Command* tmpCmd=&(commands[i]);
        printf("command=%s\narguments=", tmpCmd->arguments[0]);
        int k=0; 
        char *cmd;
        while((cmd=tmpCmd->arguments[k++])!=(char*)NULL)printf("%s ", cmd);
        printf("\nredirection_in file=%s\n", tmpCmd->stdin_file==NULL?"NULL":tmpCmd->stdin_file+2);
        printf("redirection_out file=%s\n", tmpCmd->stdout_file==NULL?"NULL":tmpCmd->stdout_file+2);
        printf("separator=%s\n", tmpCmd->separator);
    }
}
