#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/wait.h>
#include <stdarg.h>

#include "status.h"

void getChildStatus(int status){
    if(WIFEXITED(status)) {
        switch(status){
            case(1):;
            case(2):;
            case(126):;
            case(128):;
        }
    }
    else if(WIFSIGNALED(status)) printf("killed by signal %d\n", WTERMSIG(status));
    else if(WIFSTOPPED(status)) printf("stopped by signal %d\n", WSTOPSIG(status));
    else if(WIFCONTINUED(status)) printf("continued\n");
}

char* getZombieStatus(int status){
    //Stopped and Continued leave them to kill program
    //no way to stop a background job without either bring it foreground (bg) or use kill program  
    if(WIFEXITED(status)) return "Done";
    else if(WIFSIGNALED(status)) return "Killed";
    else return "Unknown";
}

void exitGlob(int result){
    switch(result){
            case(GLOB_NOSPACE):puts("running out of memory");break;
            case(GLOB_ABORTED):puts("read error");break;
            case(GLOB_NOMATCH):puts("no found matches");break;
            default:puts("unknwon");
        }
}

void exitStatus(char* err, int n) { perror(err); exit(n); }