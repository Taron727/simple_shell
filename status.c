#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <glob.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "status.h"
#include "shell.h"

// *******************private function declaration begin

void err_caller(unsigned short errn, char* errmsg); 

// private function declaration end*********************

//errn - 0=inof; 1=warn(user-defined); 2=error(system)
void err_caller(unsigned short errn, char* errmsg) {
    assert(errn<3);
    switch(errn){
        case(0):printf("[INFO]: %s\n", errmsg);break;
        case(1):printf("%s[WARNING]: %s\n", WARNING, errmsg); break;
        case(2):printf("%s[ERROR]: %s\n", ERROR, errmsg); perror(errmsg);break;
    }
    printf(DEFAULT);
}

void getChildStatus(int status){
    int ex_code; char buf[64], *t_buf=buf;
    strcpy(buf, "");
    if(WIFEXITED(status)) {
        if((ex_code=WEXITSTATUS(status))!=0) 
            sprintf(buf, "child exit code=%d", ex_code);
    }else {
        sprintf(buf, "process was terminated unvoluntarily:");
        if(WIFSIGNALED(status)) sprintf(buf, "%s killed by signal %d", t_buf, WTERMSIG(status));
        else if(WIFSTOPPED(status)) sprintf(buf, "%s stopped by signal %d", t_buf, WSTOPSIG(status));
        else if(WIFCONTINUED(status)) sprintf(buf, "%s continued", t_buf);        
    }
    if(strcmp(buf,"")!=0) err_caller(1, buf);
}

char* getZombieStatus(int status){
    //Stopped and Continued leave them to kill program
    //no way to stop a background job without either bring it foreground (bg) or use kill program  
    if(WIFEXITED(status)) return "Done";
    else if(WIFSIGNALED(status)) return "Killed";
    else if(WIFSTOPPED(status)) return "Stopped";
    else if(WIFCONTINUED(status)) return "Continued";
    else return "Unknown";
}

void exitGlob(int result){
    switch(result){
        case(GLOB_NOSPACE):err_caller(1,"running out of memory");break;
        case(GLOB_ABORTED):err_caller(1,"read error");break;
        case(GLOB_NOMATCH):err_caller(1,"no found matches");break;
    }
}

void getDirStatus(int result){
    switch(result){
       case(EIO):err_caller(1,"I/O error occurred");break;
       case(ENAMETOOLONG):err_caller(1,"path is too long");break;
       case(EACCES):err_caller(1,"search permission is denied");break;
       case(ENOENT):err_caller(1,"directory specified in path does not exist");break;
       case(ENOMEM):err_caller(1,"insufficient kernel memory was available");break;
       case(ENOTDIR):err_caller(1,"a component of path is not a directory");break;
       case(EFAULT):err_caller(1, "path points outside your accessible address space");break;
       case(ELOOP):err_caller(1,"too many symbolic links were encountered in resolving path");break;
    }
}

void exitStatus(char* err, int n) { 
    err_caller(n, err);  
    if(n>=2) { // release resources
	destroyShellEnv(); 
	exit(n);
    }
}
