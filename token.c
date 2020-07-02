/*
* token.h implementation 
*/

#include <stdlib.h>
#include <string.h>
#include "token.h" 

int can_separate(char c){   // in ASCII, \t = 9, \s = 32, \n = 10
    return (c==9 || c==32 || c==10); 
}

int tokenise(char* line, char* token[]){
    int c, i, j, canBg, regFlg, size, regBk; 
    regFlg = canBg = regBk = i = c = j = 0;
    
    for(i=0; line[i]!='\0'; i++) { //set token to line[i] alias
        if((canBg=!can_separate(line[i]))) { 
            regFlg=1; //register flag 
            if(!regBk){ 
                if(j>=MAX_NUM_TOKENS) return -1; //exceeds MAX_TOKEN_SIZE
                token[j++]=(void*)&line[i]; //assign address of line[i]; must cast to void* 
                regBk = 1; //register backup   
            } 
        }else if(regFlg) { //claimed flg 
            c++, regFlg=regBk=0; //reset flag, backup 
            line[i]='\0';  
        }
    }
    if(regFlg) c++; //check if there is flg haven't claimed 
    return c; 
}


