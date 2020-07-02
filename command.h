#include <stddef.h>

#include "token.h" 

#define MAX_ARGS 1000
#define BUFF_CMD 256
#define MASK 0777

char* tokens[MAX_NUM_TOKENS];

int *zombies_size, shmid, fd;
char* zombies[100]; 

struct CommandStructure; 
typedef char* (*getCmdPth)(struct CommandStructure);
typedef int (*redirection)(char* fd_s, unsigned short fd);
typedef unsigned short(*getArgsLen)(struct CommandStructure);

typedef struct CommandStructure {
    int first; 		            //index of first token of the command in token array  
    int last; 		            //index of last token of the command in token array    
    char* separator; 	        //| , pipe to next command 
			                    // &, shell does not wait for this command 
			                    // or shell wait for this command 
    char* arguments[MAX_ARGS]; 
    char* redirectFds[MAX_ARGS/3]; 
    getCmdPth   get_CmdPth; 
    getArgsLen  get_ArgLen;
    redirection redirect; 
}Command; 

/*
* purpose:	break token into an array of command
* pre:		
* post:		1) broken command struct will be stroed in commands  
* 		return value: total number of command or -1 if token size < 0 
*/
int separateCommands(char* tokens[], size_t token_size, Command* command[]);

/*
* purpose:	destroy command struct 
* pre:		
* post:		1) fee *cmd array and cmd->separator  
*/
void destroy_Commands(Command* cmd[]); 

int execute(Command* cmd, int isBg); 
int extract_wild(Command* cmd, int isBg);