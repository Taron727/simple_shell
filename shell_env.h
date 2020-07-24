/* name:			shell_env.h	
 * purpose: 		creating an interative user input environment that allowing user to ammmend typed command using different keys  
 * author:		    user3121023, Alexis Wilke, purec, taron liew
 * date:			02072020
 * last modified:	16072020
 */

#define ESC          27
#define INSERT       50
#define DELETE       51
#define PGUP         53
#define PGDN         54
#define ARROWRIGHT   67
#define ARROWLEFT    68
#define END          70
#define HOME         72
#define OTHER        79
#define BRACKETLEFT  91
#define TILDE       126
#define BACKSPACE   127
#define SIZE        256

static const int STDIN  =0; // default stdin value 
static int  EXIT_KEY    =0; // defaut exit key value; exit key define whether shell can exit from interactive mode or not 
static int row          =1; // current row number of terminal console that are ready for user input 

/*
* purpose:	    check if the exit key is locked     
* pre:		    nil  
* post:		    nil 
* return value: 1) return 1 if the key is locked
*               2) return 0 if the key is unlocked 
*/
int isKeyLock();

/*
* purpose:	    get current position of terminal console    
* pre:		    y and x cannot be NULL 
* post:		    populate y as the y-axis of terminal console/ row and x as teh x-axis of terminal console/ column
* return value: 1) return 1 upon failed call 
*               2) return 0 upon success call 
*/
int getPos(int *y, int *x);

/*
* purpose:	    unlock the lock to exit user interactive mode     
* pre:		    nil 
* post:		    unlocked exit key 
* return value: nil  
*/
void unlockEKey(); 

/*
* purpose:	    update next row that is ready for user input     
* pre:		    nil  
* post:		    next row value is updated  
* return value: nil  
*/
void nextRow(int new_row);  

/*
* purpose:	    entering interative mode of shell      
* pre:		    input and sh_prompt cannot be NULL   
* post:		    allow user to interact with keyboard and ammend entered commands using INS, left/ right arrows and back space   
* return value: nil  
*/
void shellInput (char* input, char* sh_prompt);
