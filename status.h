/* name:			status.h	
 * purpose: 		retriving status of per process including user-define and system-defined calls 
 * author:			taron liew, pak kin lee
 * date:			02072020
 * last modified:	16072020
 */

#define WARNING     "\x1B[1;33m" // yellow 
#define ERROR       "\x1B[0;31m" // red
#define DEFAULT     "\x1B[1;37m" // white

/*
* purpose:		retriving child process status from wait()
* pre:			child process terminated from wait()
* post:			forwarding terminated child status or exit code of child to a text display manager 
* return value: nil 
*/
void getChildStatus(int status);

/*
* purpose:		retriving zombie processes status from wait()
* pre:			child process terminated and was claimed by parent 
* post:			forwarding zombies status either done, killed, stopped, continued or unknown to a text display manager 
* return value: nil 
*/
char* getZombieStatus(int status);

/*
* purpose:		client API for indicating a progrma exit staus with program not returning success code 
* pre:			message and exit code; 0 for information, 1 for warning, 2 for error (critical) which causes a termination of the shell 
* post:			forwariding messages and exit codes (0, 1, 2) to text display manager
* return value: nil 
*/
void exitStatus(char* err, int n);

/*
* purpose:		retriving chdir() program status 
* pre:			chdir() return an exit code that indicates execution unsuccessful
* post:			forwarding error message interpretation to a text display manager 
* return value: nil
*/
void getDirStatus(int result);

/*
* purpose:		retriving glob expansion program status 
* pre:			glob() return an exit code that indicates execution unsuccessful
* post:			forwarding error message interpretation to a text display manager 
* return value: nil
*/
void exitGlob(int r); 
