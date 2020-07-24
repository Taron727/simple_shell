/* name:			command.h	
 * purpose: 		building commands structure and commands execution based on command separator 
 * author:		    taron liew
 * date:			02072020
 * last modified:	16072020
 */

#include <stddef.h>
#include "token.h" 

#define WILD_CARDS "*?"     // acceptable wildcards for file expansion  
#define MAX_ARGS 1000       // maximum argument for each command struct 
#define BUFF_CMD 256        // maximum buffer size for zombie commands name and status
#define MASK 0777           // umask created for any redirected files creation 
#define JOB_SIZE 100

#ifndef COMMAND_H
#define COMMAND_H

int *zombies_size, *jobs_size, shmid, shmid2, fd, jb;   // curent zombie size 
static int glob_enable=1;       // toggle value for glob expansion 
static int job_type=0;		// indicator for either foreground or background job
static char zombie_buff[BUFF_CMD];      // accumulatd zombie name and status buffer for display after user input done (slow system call) 
char* zombies[JOB_SIZE], *jobs[JOB_SIZE], *tokens[MAX_NUM_TOKENS];    // zombies list; tokens (token is needed as commands only keep the addresses of token without copying any tokens)
sigset_t signs; struct sigaction act_job;     // signals to be blocks during shell session with user 

struct CommandStructure; 
typedef unsigned short(*getArgsLen)(struct CommandStructure);   // function pointer of get the argument length for each command 

typedef struct CommandStructure {
    int first; 		            // index of first token of the command in token array  
    int last; 		            // index of last token of the command in token array    
    char* separator; 	        // | , pipe to next command 
			                    // &, shell does not wait for this command 
			                    // or shell wait for this command 
    char** arguments;   // an array of tokens that forms a command
    char* stdin_file;   // if not NULL, points to the file name for stdin redirection                        
    char* stdout_file;  // if not NULL, points to the file name for stdout redirection
    getArgsLen get_ArgLen;  
}Command; 

/*
* purpose:	    break token into an array of command
* pre:		
* post:		    broken command struct will be stroed in commands  
* return value: 1) return the total number of command  
*               2) return -1 if token size < 0
*/
int separateCommands(char* tokens[], size_t token_size, Command command[]);

/*
* purpose:	    destroy all command struct by freeing pointer members  
* pre:		    comdIndx > 0
* post:		    free *cmd array and cmd->separator for each command
* reutrn value: nil 
*/
void destroyCommands(Command cmd[], int comdIndx);
int extractWild(Command* cmd, int isBg);

/*
* purpose:	    return zombie buffer to caller   
* pre:		    nil 
* post:		    nil 
* reutrn value: zombie buffer (static scope) 
*/
void getZombieBuff(); 

/*
* purpose:	    client API for toggling glob expansion function   
* pre:		    val cannot be a value other than 0 and 1 
* post:		    enable or disable gloab expansion 
* reutrn value: nil 
*/
void configGlob(unsigned int val); 

/*
* purpose:	    send SIGCONT to a stopped process    
* pre:		    stopped process stack cannot be empty 
* post:		    stopped process resumed
* reutrn value: nil 
*/
void resumeJob(); 

/*
* purpose:	    send SIGTSTP to a stopped process    
* pre:		    nil 
* post:		    sent SIGSTOP to process 
* reutrn value: nil 
*/
void stopJob(int signo);

#endif

