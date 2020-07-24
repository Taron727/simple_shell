/* token.h implementation */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "status.h"
#include "token.h" 

// *******************private function declaration begin

int can_separate(char c);

// private function declaration end**********************

int can_separate(char c) {   // in ASCII, \t = 9, \s = 32, \n = 10
    return (c==9 || c==32 || c==10); 
}

int tokenise(char* line, char* token[]){
    char buf[64]; 
    int c, i, j, canBg, regFlg, size, regBk; 
    regFlg=canBg=regBk=i=c=j=0;
    
    for(i=0; line[i]!='\0'; i++) { //set token to line[i] alias
        if((canBg=!can_separate(line[i]))) { 
            regFlg=1; //register flag 
            if(!regBk){ //exceeds MAX_TOKEN_SIZE
                if(j>=MAX_NUM_TOKENS) { exitStatus("tokensize(): token overloaded", 1); return -1; } 
                token[j++]=(void*)&line[i]; //assign address of line[i]; must cast to void* 
                if( (line[i+1]=='\0'&&i==0) && !can_separate((line[i+1]))&&(
                    token[j-1][0]==124||token[j-1][0]==38||
                    token[j-1][0]==59||token[j-1][0]==60||
                    token[j-1][0]==62)){ //first char is separator && second char cannot be separated
                    sprintf(buf, "tokenise(): syntax error near token %s", token[j-1]);
                    exitStatus(buf, 1); 
                    return -2;
                }
                regBk=1; //register backup   
            } 
        }else if(regFlg) { //claimed flg 
            c++, regFlg=regBk=0; //reset flag, backup 
            line[i]='\0';  
        }
    }
    if(regFlg) c++; //check if there is flg haven't claimed 
    return c; 
}


