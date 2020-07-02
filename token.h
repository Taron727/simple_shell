/*
* token.h 	header file for toenise
*/
#define MAX_NUM_TOKENS 100

/*
* purpose:	takes a null-terminated line of characters input_line, and breaks it into a sequence of tokens
* pre:		1) token size >= input_line tokens 
* 		2) token must be seprated by either \n, \t or \s
* post:		1) it assigns starting address of i tokens into the array token[i]
* 		2) it replaces end character of token with \0  
* 		return value: total number of tokens or -1 if token[] size < input_line no.of tokens  
*/
int tokenise(char* input_line, char* token[]); 

