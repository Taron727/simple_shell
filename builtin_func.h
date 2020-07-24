/* name:			builtin_func.h	
 * purpose: 		organising shell built-in functions
 * author:		    taron liew, pak kin lee
 * date:			02072020
 * last modified:	16072020
 */

#define P_HOME getenv("HOME") 	// user HOME path 
#define MAX_SHELL_CMDS 10		// maximum number of built-in shell commands; no user-control  
#define MAX_USRNAME_LEN 16		// maximum number of characters of user name for shell prompt

#include <time.h>
#include "command.h"

static time_t begin, end; 		// start time when shell launch; end time when shell dies (for exit())
static int debug_enable                 =0;		// toggle flag for debuging mode of shell
static size_t h_command_size            =0;		// h_commands current size 
static char sh_prompt[MAX_USRNAME_LEN]  ="%"; 	// user name variable for prompt  

enum H_CMDS { prompt, pwd, cd, eexit, set, fg };	// registration for built-in functions  

typedef int (*fexecute)(char** args);		// execute build-in functions function pointer 
typedef struct homemade_command{	// built-in function struct 
    int index;	// index in h_commands 
    char label[128];	// function name 
    fexecute exe; 	// function's fucntion 
}h_command;  

static h_command h_commands[MAX_SHELL_CMDS];	// a list of built-in functions 

/*
* purpose:		modification of path that consists of HOME symbol/ ~
* pre:			path cannot be NULL
* post:			1) modify path by replacing ~ to found home path 
* return value: 	1) returns -1 when home path is not set by the user 
* 				2) return 0 when path is modified successfully   
*/
int initHomePath(char* path); 

/*
* purpose:		initiatiase the environment of this module 
* pre:			nil
* post:			initialise h_commands  
* return value: 	nil 	
*/
void initBuiltInCommand(); 

/*
* purpose:		user API to execution command that is possible a built-in command
* pre:			cmd, tokens and comdIndx cannot be empty or NULL 
* post:			execute command if passed-in command matches built-in function
* return value: 	1) return -1 if passed-in command does not macth any built-in function 
* 				2) return 0 if function has been executed regardless failed or sucess 
*/
int exeBuiltInCommand(Command *cmd, char* tokens[], int comdIndx);

/*
* purpose:		log begin time of the shell
* pre:			shell is launch 
* post:			timestmap of when shell if launched is recorded 
* return value: 	nil  
*/
void logBTime();

/*
* purpose:		log end time of the shell
* pre:			shell is goind to die 
* post:			timestmap of shell after terminated by the user  
* return value: 	nil  
*/ 
void logETime(); 

/*
* purpose:		retrieve shell prompt variable; cannot retrieve directly due to static scope 
* pre:			nil	 
* post:			nil  
* return value: 	shell prompt variable   
*/
char* getShellPrompt(); 
