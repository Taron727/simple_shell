/* name:			main.c	
 * purpose: 		client program for shell 
 * author:		    taron liew
 * date:			02072020
 * last modified:	16072020
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "shell.h"
#include "builtin_func.h"
#include "shell_env.h"
#include "status.h"
#include "pv.h"

int main(){
    int t_size, c_size, nr, k, x, y=0; 
    char* ready, argv[SIZE];

    initShellEnv();     // initialis shell environment 
    initBuiltInCommand();   // initialise built_in commands 
    logBTime();     // log timestamp for shell session 
    printf("\033[2J");// clear stdout 
    do{
        Command cmds[MAX_NUM_TOKENS];  
        ready=(char*)malloc(sizeof(char)*SIZE); 
        while(1){  
            nextRow(y); // update next row from terminal screen for input 
            shellInput(argv, getShellPrompt()); // shell input for user  
            nr=strlen(argv);
	    if(nr<=0 && errno==EINTR)
                continue;
            break;
        }
        getZombieBuff();    // dispaly zombie buffer status only after user input finished  
        if(argv[nr-1]=='\n') argv[nr-1]='\0'; 
        strcpy(ready, argv);    // directly using argv may encounter segmentaion problem 
        if((t_size=tokenise(ready, tokens))<=0) { getPos(&y, &x); continue; }   // if user does not enter any input then update cursor position and continue 
	    c_size=separateCommands(tokens, t_size, cmds); 
        if((k=exeBuiltInCommand(cmds, tokens, c_size))<0) eexecute(cmds, tokens, c_size);   // if not executed from built_in functions then execute from external program 
        getPos(&y, &x);
        destroyCommands(cmds, c_size);  // free resoures 
        free(ready);
    }while(isKeyLock());    // as long as key is locked 

    destroyShellEnv();  // free shell environment resources 
    return 0;
}
