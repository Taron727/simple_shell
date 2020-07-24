/* name:			shell.h	
 * purpose: 		initialise command execution environment such as background 
                    processes handling, semophores, shared_memory pipie file descriptor, signal blocking and redirection  
 * author:		    taron liew
 * date:			02072020
 * last modified:	16072020
 */

#include "command.h"

#define SHMKEY    0x00a00001 // share memory key for zombies  
#define SHMKEY2   0X00a00002 // share memory key for jobs 
#define BUFF_KEY  0x00a00003 // semaphore key for zombie 
#define BUFF_KEY2 0x00a00004 // semaphore key for jobs 
#define MAX_PIPE 20         // maximum number of commands for piping; the second command of per pipe is redundant thus 2 pipes have 4 commands instead of 3


/*
* purpose:	    initialise command execution environment     
* pre:		    nil  
* post:		    command execution environment is ready for command execution within this shell   
* return value: nil  
*/
void initShellEnv();

/*
* purpose:	    realese resources used in command execution environment-unblock signals and release shared memory     
* pre:		    keys have not been realiased yet   
* post:		    resrouces used in this shell is released 
*/
void destroyShellEnv(); 

/*
* purpose:	    command execution manager that handlin pipeline and nominal structure for execution  
* pre:		    size cannot be zero 
* post:		    commands are structured and pass to executor 
*/
void eexecute(Command commands[], char* tokens[], int size);
